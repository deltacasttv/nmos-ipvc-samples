/*
 * SPDX-FileCopyrightText: Copyright (c) DELTACAST.TV. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at * * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nmos_tools.h"
#include "nmos/model.h"
#include "nmos/log_model.h"
#include "nmos/log_gate.h"
#include "nmos/node_server.h"
#include "nmos/node_interfaces.h"
#include "nmos/node_resource.h"
#include "nmos/clock_name.h"
#include "nmos/settings.h"
#include "nmos/node_resources.h"
#include "nmos/interlace_mode.h"
#include "nmos/colorspace.h"
#include "nmos/transfer_characteristic.h"
#include "nmos/components.h"
#include "nmos/transport.h"
#include "nmos/connection_resources.h"
#include "nmos/capabilities.h"
#include "cpprest/host_utils.h"
#include "cpprest/json_ops.h"

#include <videomasterip/videomasterip_networkinterface.h>

#include "tools.h"

nmos_tools::NodeServer::NodeServer(nmos::node_model& node_model, nmos::experimental::node_implementation node_implementation, nmos::experimental::log_model& log_model, slog::base_gate& gate, const std::string device_name, const std::string device_description, std::string media_nic_name)
      : node_model(node_model), gate(gate), device_name(device_name), device_description(device_description), media_nic_name(media_nic_name), is_enabled(false), sdp(""),node_server(nmos::experimental::make_node_server(node_model, node_implementation, log_model, gate))
{
}

bool nmos_tools::NodeServer::get_ptp_system_parameters(NmosPtpSystemParameters &ptp_system_parameters)
{
   std::lock_guard lock(ptp_system_parameters_mutex);
   if(are_system_parameters_valid)
   {
      ptp_system_parameters = this->ptp_system_parameters;
      return true;
   }
   else
      return false;
}

void nmos_tools::NodeServer::start()
{
   node_server.open();
}

void nmos_tools::NodeServer::stop()
{
   node_server.close().wait();
}

bool nmos_tools::NodeServer::node_implementation_init()
{
   const unsigned int delay_millis{ 0 };

   nmos::write_lock lock = node_model.write_lock();

   const auto seed_id = nmos::experimental::fields::seed_id(node_model.settings);

   const auto clocks = web::json::value_of({ nmos::make_internal_clock(nmos::clock_names::clk0) });
   const auto host_interfaces = nmos::get_host_interfaces(node_model.settings);
   const auto interfaces = nmos::experimental::node_interfaces(host_interfaces);

   node_id = nmos::experimental::fields::seed_id(node_model.settings);

   auto node = nmos::make_node(node_id, clocks, nmos::make_node_interfaces(interfaces), node_model.settings);
   if (!insert_resource_after(node_model,lock,delay_millis, node_model.node_resources, std::move(node), gate)) throw node_implementation_init_exception("Failed to insert node resource");

   //Add one device
   device_id = nmos::make_repeatable_id(seed_id, utility::conversions::to_string_t(device_name));

   nmos::settings settings = node_model.settings;
   settings[nmos::fields::label] = web::json::value::string(utility::conversions::to_string_t(device_name));
   settings[nmos::fields::description] = web::json::value::string(utility::conversions::to_string_t(device_description));

   auto device = nmos::make_device(device_id, node_id,{},{}, settings);
   const std::pair<nmos::id, nmos::type> id_type{ device_id, device.type };
   if (!insert_resource(node_model.node_resources, std::move(device)).second) throw node_implementation_init_exception("Failed to insert device resource");

   return true;
}

void nmos_tools::NodeServer::system_params_handler(const web::uri &system_uri, const web::json::value &system_global)
{
   std::lock_guard lock(ptp_system_parameters_mutex);

   if (system_global.is_null())
   {
      are_system_parameters_valid = false;
      return;
   }
   else
   {
      are_system_parameters_valid = true;
      const auto& ptp = nmos::fields::ptp(system_global);
      ptp_system_parameters.domain_number = nmos::fields::domain_number(ptp);
      ptp_system_parameters.announce_receipt_timeout = nmos::fields::announce_receipt_timeout(ptp);
   }
}

bool nmos_tools::NodeServerSender::node_implementation_init()
{
   const unsigned int delay_millis{ 0 };
   uint32_t frame_heigth;
   uint32_t frame_width;
   uint32_t frame_rate;
   nmos::rational frame_rate_rational;
   bool8_t interlaced;
   bool8_t is_us;


   try
   {
      NodeServer::node_implementation_init();

      if(VMIP_GetVideoStandardInfo(vmip_info.essence_config.EssenceS2110_20Prop.VideoStandard, &frame_width, &frame_heigth, &frame_rate, &interlaced, &is_us) != VMIPERR_NOERROR)
         throw node_implementation_init_exception("Failed to get video standard info");

      if(is_us)
         frame_rate_rational = nmos::parse_rational(nmos::make_rational(frame_rate * 1000, 1001));
      else
         frame_rate_rational = frame_rate;

      const auto seed_id = nmos::experimental::fields::seed_id(node_model.settings);

      nmos::write_lock lock = node_model.write_lock(); // in order to update the resources

      //Start of sender specific part
      //Add one source
      const auto source_id = nmos::make_repeatable_id(seed_id, U("IPVC Video Source"));
      const auto flow_id = nmos::make_repeatable_id(seed_id, U("IPVC Video Flow"));
      const auto sender_id = nmos::make_repeatable_id(seed_id, U("IPVC Video Sender"));

      nmos::resource source = nmos::make_video_source(source_id, device_id, nmos::clock_names::clk0, frame_rate_rational, node_model.settings);
      source.data[nmos::fields::label] = web::json::value::string(U("IPVC Video Source"));
      source.data[nmos::fields::description] = web::json::value::string(U("IP Virtual Card Video Source"));

      nmos::resource flow = nmos::make_raw_video_flow(flow_id, source_id, device_id,
                                                      frame_rate_rational,
                                                      frame_width,frame_heigth,
                                                      interlaced ? nmos::interlace_modes::interlaced_tff : nmos::interlace_modes::progressive,
                                                      nmos::colorspaces::BT709,
                                                      nmos::transfer_characteristics::SDR,
                                                      nmos::chroma_subsampling::YCbCr422,
                                                      10,
                                                      node_model.settings);

      flow.data[nmos::fields::label] = web::json::value::string(U("IPVC Video Flow"));
      flow.data[nmos::fields::description] = web::json::value::string(U("IP Virtual Card Video Flow"));

      if (!insert_resource_after(node_model,lock,delay_millis, node_model.node_resources, std::move(source), gate)) throw node_implementation_init_exception("Failed to insert source resource");  
      if (!insert_resource_after(node_model,lock,delay_millis, node_model.node_resources, std::move(flow), gate)) throw node_implementation_init_exception("Failed to insert flow resource");
   
      const auto manifest_href = nmos::experimental::make_manifest_api_manifest(sender_id, node_model.settings);
      auto sender = nmos::make_sender(sender_id, flow_id, nmos::transports::rtp, device_id, manifest_href.to_string(),{utility::conversions::to_string_t(media_nic_name)} , node_model.settings);
      sender.data[nmos::fields::label] = web::json::value::string(U("IPVC Video Sender"));
      sender.data[nmos::fields::description] = web::json::value::string(U("IP Virtual Card Video Sender"));

      auto connection_sender = nmos::make_connection_rtp_sender(sender_id, false, utility::conversions::to_string_t(sdp));

      connection_sender.data[nmos::fields::endpoint_constraints][0][nmos::fields::source_ip]
         = web::json::value_of({
               { nmos::fields::constraint_enum,
               web::json::value_of({nmos_tools::ipv4_to_string(active_transport_params.ip_src)})},
                              });
      connection_sender.data[nmos::fields::endpoint_constraints][0][nmos::fields::source_port]
         = web::json::value_of({
               { nmos::fields::constraint_enum,
               web::json::value_of({active_transport_params.port_src})},
                              });

      if (!insert_resource_after(node_model,lock,delay_millis, node_model.node_resources, std::move(sender), gate)) throw node_implementation_init_exception("Failed to insert sender resource");
      if (!insert_resource_after(node_model,lock,delay_millis, node_model.connection_resources, std::move(connection_sender), gate)) throw node_implementation_init_exception("Failed to insert connection sender resource");
   }
   catch (const node_implementation_init_exception& e)
   {
      // most likely from incorrect value types in the command line settings
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "Node implementation error: " << e.what();
      return false;
   }
   catch (const web::json::json_exception& e)
   {
      // most likely from incorrect value types in the command line settings
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();
      return false;
   }
   catch (const std::system_error& e)
   {
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "System error: " << e.what() << " [" << e.code() << "]";
      return false;
   }
   catch (const std::runtime_error& e)
   {
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error: " << e.what();
      return false;
   }
   catch (const std::exception& e)
   {
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what();
      return false;
   }
   catch (...)
   {
      slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception";
      return false;
   }

   return true;
}

bool nmos_tools::NodeServerReceiver::node_implementation_init()
{
   const unsigned int delay_millis{ 0 };

   try
   {
      NodeServer::node_implementation_init();

      const auto seed_id = nmos::experimental::fields::seed_id(node_model.settings);

      auto lock = node_model.write_lock(); // in order to update the resources

      //start of receiver specific part
      const auto receiver_id = nmos::make_repeatable_id(seed_id, U("IPVC Video Receiver"));

      const std::vector<utility::string_t> media_interfaces = { utility::conversions::to_string_t(media_nic_name) };
      auto receiver = nmos::make_video_receiver(receiver_id, device_id, nmos::transports::rtp, media_interfaces, node_model.settings);
      receiver.data[nmos::fields::label] = web::json::value::string(U("IPVC Video Receiver"));
      receiver.data[nmos::fields::description] = web::json::value::string(U("IP Virtual Card Video Receiver"));

      const auto interlace_modes = std::vector<utility::string_t>{ nmos::interlace_modes::progressive.name, nmos::interlace_modes::interlaced_tff.name };
      receiver.data[nmos::fields::caps][nmos::fields::constraint_sets] = generate_constraints();

      auto connection_receiver = nmos::make_connection_rtp_receiver(receiver_id, false);
      connection_receiver.data[nmos::fields::endpoint_constraints].as_array()[0].as_object()[nmos::fields::interface_ip] =
      web::json::value_of({
         { nmos::fields::constraint_enum,
         web::json::value_of({ web::json::value(ipv4_to_string(active_transport_params.ip_interface)) })
         }
      });

      if (!insert_resource_after(node_model, lock, delay_millis, node_model.node_resources, std::move(receiver), gate)) throw node_implementation_init_exception("Failed to insert receiver resource");
      if (!insert_resource_after(node_model, lock, delay_millis, node_model.connection_resources, std::move(connection_receiver), gate)) throw node_implementation_init_exception("Failed to insert connection receiver resource");

   }
   catch (const node_implementation_init_exception& e)
   {
      // most likely from incorrect value types in the command line settings
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "Node implementation error: " << e.what();
      return false;
   }
   catch (const web::json::json_exception& e)
   {
      // most likely from incorrect value types in the command line settings
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();
      return false;
   }
   catch (const std::system_error& e)
   {
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "System error: " << e.what() << " [" << e.code() << "]";
      return false;
   }
   catch (const std::runtime_error& e)
   {
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error: " << e.what();
      return false;
   }
   catch (const std::exception& e)
   {
      slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what();
      return false;
   }
   catch (...)
   {
      slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception";
      return false;
   }

   return true;
}

std::string nmos_tools::NodeServerReceiver::get_sdp()
{
   return sdp;
}

bool nmos_tools::NodeServer::insert_resource_after(nmos::node_model &node_model, nmos::write_lock &lock, unsigned int milliseconds, nmos::resources &resources, nmos::resource &&resource, slog::base_gate &gate)
{
   if (nmos::details::wait_for(this->node_model.shutdown_condition, lock, std::chrono::milliseconds(milliseconds), [&] { return this->node_model.shutdown; }))
      return false;

   const std::pair<nmos::id, nmos::type> id_type{ resource.id, resource.type };
   const bool success = insert_resource(resources, std::move(resource)).second;

   if (success)
      slog::log<slog::severities::info>(gate, SLOG_FLF) << "Updated model with " << id_type;
   else
      slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Model update error: " << id_type;

   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Notifying node behaviour thread"; // and anyone else who cares...
   this->node_model.notify();

   return success;
}

bool nmos_tools::NodeServer::is_field_auto(const web::json::value &object, const web::json::field_as_value_or &field_name)
{
   return object.has_field(field_name) &&
          object.at(field_name).is_string() &&
          object.at(field_name).as_string() == U("auto");
}

nmos_tools::NodeServerSender::NodeServerSender(nmos::node_model &node_model, nmos::experimental::log_model &log_model, slog::base_gate &gate, const std::string device_name, const std::string device_description, VmipInfo vmip_info, TransportParams &resolve_auto_transport_params, TransportParams &active_transport_params, std::string media_nic_name, std::string sdp)
: NodeServer(node_model, make_node_implementation(), log_model, gate, device_name, device_description, media_nic_name), vmip_info(vmip_info), resolve_auto_transport_params(resolve_auto_transport_params), active_transport_params(active_transport_params)
{
   this->sdp = sdp;
}

nmos::experimental::node_implementation nmos_tools::NodeServerSender::make_node_implementation()
{
   auto node_implementation = nmos::experimental::node_implementation();
   node_implementation.on_system_changed(std::bind(&NodeServerSender::system_params_handler, this, std::placeholders::_1, std::placeholders::_2));
   node_implementation.on_resolve_auto(std::bind(&NodeServerSender::resolve_auto, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
   node_implementation.on_connection_activated(std::bind(&NodeServerSender::connection_activation, this, std::placeholders::_1, std::placeholders::_2));
   node_implementation.on_validate_connection_resource_patch(std::bind(&NodeServerSender::patch_validator,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
   node_implementation.on_set_transportfile(std::bind(&NodeServerSender::transportfile_setter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
   return node_implementation;
}

void nmos_tools::NodeServerSender::resolve_auto(const nmos::resource &resource, const nmos::resource &connection_resource, web::json::value &transport_params)
{
   // debug log the json objects
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "resolve_auto_sender: resource: " << std::endl << resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "resolve_auto_sender: connection_resource: " << std::endl << connection_resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "resolve_auto_sender: transport_params: " << std::endl << transport_params.serialize();

   for (auto& transport_param : transport_params.as_array())
   {
      if (is_field_auto(transport_param, nmos::fields::destination_ip))
      {
         transport_param[nmos::fields::destination_ip] = web::json::value::string(ipv4_to_string(resolve_auto_transport_params.ip_dst));
      }
      if (is_field_auto(transport_param, nmos::fields::destination_port))
      {
         transport_param[nmos::fields::destination_port] = web::json::value::number(resolve_auto_transport_params.port_dst);
      }
      if (is_field_auto(transport_param, nmos::fields::source_ip))
      {
         transport_param[nmos::fields::source_ip] = web::json::value::string(ipv4_to_string(resolve_auto_transport_params.ip_src));
      }
      if (is_field_auto(transport_param, nmos::fields::source_port))
      {
         transport_param[nmos::fields::source_port] = web::json::value::number(resolve_auto_transport_params.port_src);
      }
   }
   
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "resolve_auto_sender: resolved_transport_params: " << std::endl << transport_params.serialize();

}

nmos_tools::NodeServerReceiver::NodeServerReceiver(nmos::node_model &node_model, nmos::experimental::log_model &log_model, slog::base_gate &gate, const std::string device_name, const std::string device_description, TransportParams &resolve_auto_transport_params, TransportParams &active_transport_params, std::string media_nic_name)
: NodeServer(node_model, make_node_implementation(), log_model, gate, device_name, device_description, media_nic_name), resolve_auto_transport_params(resolve_auto_transport_params), active_transport_params(active_transport_params)
{
}

nmos::experimental::node_implementation nmos_tools::NodeServerReceiver::make_node_implementation()
{
   auto node_implementation = nmos::experimental::node_implementation();
   node_implementation.on_system_changed(std::bind(&NodeServerReceiver::system_params_handler, this, std::placeholders::_1, std::placeholders::_2));
   node_implementation.on_resolve_auto(std::bind(&NodeServerReceiver::resolve_auto, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
   node_implementation.on_connection_activated(std::bind(&NodeServerReceiver::connection_activation, this, std::placeholders::_1, std::placeholders::_2));
   node_implementation.on_validate_connection_resource_patch(std::bind(&NodeServerReceiver::patch_validator,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
   node_implementation.on_parse_transport_file(std::bind(&NodeServerReceiver::transportfile_parser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
   return node_implementation;
}

void nmos_tools::NodeServerReceiver::resolve_auto(const nmos::resource &resource, const nmos::resource &connection_resource, web::json::value &transport_params)
{
      // debug log the json objects
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "resolve_auto_sender: resource: " << std::endl << resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "resolve_auto_sender: connection_resource: " << std::endl << connection_resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "resolve_auto_sender: transport_params: " << std::endl << transport_params.serialize();

   for (auto& transport_param : transport_params.as_array())
   {
      if (is_field_auto(transport_param, nmos::fields::interface_ip))
      {
         transport_param[nmos::fields::interface_ip] = web::json::value::string(ipv4_to_string(resolve_auto_transport_params.ip_interface));
      }
      if (is_field_auto(transport_param, nmos::fields::destination_port))
      {
         transport_param[nmos::fields::destination_port] = web::json::value::number(resolve_auto_transport_params.port_dst);
      }
      if (is_field_auto(transport_param, nmos::fields::multicast_ip))
      {
         transport_param[nmos::fields::multicast_ip] = web::json::value::string(ipv4_to_string(resolve_auto_transport_params.ip_multicast));
      }
      if (is_field_auto(transport_param, nmos::fields::source_ip))
      {
         transport_param[nmos::fields::source_ip] = web::json::value::string(ipv4_to_string(resolve_auto_transport_params.ip_src));
      }
   }
   
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "resolve_auto_sender: resolved_transport_params: " << std::endl << transport_params.serialize();
}

void nmos_tools::NodeServerSender::patch_validator(const nmos::resource &resource, const nmos::resource &connection_resource, const web::json::value &endpoint_staged)
{
   // debug log the json objects
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "patch_validator: resource: " << std::endl << resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "patch_validator: connection_resource: " << std::endl << connection_resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "patch_validator: endpoint_staged: " << std::endl << endpoint_staged.serialize();

}

void nmos_tools::NodeServerReceiver::patch_validator(const nmos::resource &resource, const nmos::resource &connection_resource, const web::json::value &endpoint_staged)
{
   // debug log the json objects
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "patch_validator: resource: " << std::endl << resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "patch_validator: connection_resource: " << std::endl << connection_resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "patch_validator: endpoint_staged: " << std::endl << endpoint_staged.serialize();

   //check if the sdp is not null
   if(endpoint_staged.at(nmos::fields::transport_file).at(nmos::fields::data).is_null())
      throw web::json::json_exception("sdp is null");

}

void nmos_tools::NodeServerSender::connection_activation(const nmos::resource& resource, const nmos::resource& connection_resource)
{
   // debug log the json objects
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "connection_activation_sender: resource: " << std::endl << resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "connection_activation_sender: connection_resource: " << std::endl << connection_resource.data.serialize();

   //parameters have been activated, communicate those modifications to the sample in order to reflect the model changes

   is_enabled = connection_resource.data.at(nmos::fields::active).at(nmos::fields::master_enable).as_bool();

   const web::json::array& active_transport_params_array = connection_resource.data.at(nmos::fields::active).at(nmos::fields::transport_params).as_array();
   const web::json::object& active_transport_params_object = active_transport_params_array.at(0).as_object();

   active_transport_params.ip_dst = string_to_ipv4(active_transport_params_object.at(nmos::fields::destination_ip).as_string());
   active_transport_params.port_dst = active_transport_params_object.at(nmos::fields::destination_port).as_integer();
   active_transport_params.ip_src = string_to_ipv4(active_transport_params_object.at(nmos::fields::source_ip).as_string());
   active_transport_params.port_src = active_transport_params_object.at(nmos::fields::source_port).as_integer();

}

void nmos_tools::NodeServerReceiver::connection_activation(const nmos::resource &resource, const nmos::resource &connection_resource)
{
   // debug log the json objects
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "connection_activation_receiver: resource: " << std::endl << resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "connection_activation_receiver: connection_resource: " << std::endl << connection_resource.data.serialize();

   //parameters have been activated, communicate those modifications to the sample in order to reflect the model changes

   is_enabled = connection_resource.data.at(nmos::fields::active).at(nmos::fields::master_enable).as_bool();

   const web::json::array& active_transport_params_array = connection_resource.data.at(nmos::fields::active).at(nmos::fields::transport_params).as_array();
   const web::json::object& active_transport_params_object = active_transport_params_array.at(0).as_object();

   if(active_transport_params_object.find(nmos::fields::interface_ip) != active_transport_params_object.end() && active_transport_params_object.at(nmos::fields::interface_ip).is_string())
      active_transport_params.ip_interface = string_to_ipv4(active_transport_params_object.at(nmos::fields::interface_ip).as_string());
   else
      active_transport_params.ip_interface = 0;
   if(active_transport_params_object.find(nmos::fields::multicast_ip) != active_transport_params_object.end() && active_transport_params_object.at(nmos::fields::multicast_ip).is_string())
      active_transport_params.ip_multicast = string_to_ipv4(active_transport_params_object.at(nmos::fields::multicast_ip).as_string());
   else
      active_transport_params.ip_multicast = 0;
   if(active_transport_params_object.find(nmos::fields::source_ip) != active_transport_params_object.end() && active_transport_params_object.at(nmos::fields::source_ip).is_string())
      active_transport_params.ip_src = string_to_ipv4(active_transport_params_object.at(nmos::fields::source_ip).as_string());
   else
      active_transport_params.ip_src = 0;
   active_transport_params.port_dst = active_transport_params_object.at(nmos::fields::destination_port).as_integer();

   const web::json::object& transport_file = connection_resource.data.at(nmos::fields::active).at(nmos::fields::transport_file).as_object();
   if(transport_file.find(nmos::fields::data) != transport_file.end() && transport_file.at(nmos::fields::data).is_string())
      sdp = utility::conversions::to_utf8string(transport_file.at(nmos::fields::data).as_string());
   else
      sdp = ""; //this should not happen, sdp is checked by nmos-cpp before connection is activated
}

void nmos_tools::NodeServerSender::transportfile_setter(const nmos::resource& sender, const nmos::resource& connection_sender, web::json::value& endpoint_transportfile)
{
   // debug log the json objects
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "transportfile_setter: sender: " << std::endl << sender.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "transportfile_setter: connection_sender: " << std::endl << connection_sender.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "transportfile_setter: endpoint_transportfile: " << std::endl << endpoint_transportfile.serialize();

   //update sdp to reflect changes in transport_params

   const web::json::array& active_transport_params = connection_sender.data.at(U("active")).at(U("transport_params")).as_array();

   vmip_info.stream_network_config.pPathParameters[0].DestinationIp = string_to_ipv4(active_transport_params.at(0).at(U("destination_ip")).as_string());
   vmip_info.stream_network_config.pPathParameters[0].UdpPort = active_transport_params.at(0).at(U("destination_port")).as_integer();
   //sdp must be set before the stream restart, so we have to call generate sdp here

   //generate sdp
   VMIP_ERRORCODE result = generate_sdp(vmip_info.vcs_context, vmip_info.stream_network_config, vmip_info.essence_config, &sdp);

   if(result == VMIPERR_NOERROR){
      //update sdp
      endpoint_transportfile.at(U("data")) = web::json::value::string(utility::conversions::to_string_t(sdp));
   }
   else{
      throw web::json::json_exception("Error while generating SDP");
   }

}

web::json::value nmos_tools::NodeServerReceiver::transportfile_parser(const nmos::resource &resource, const nmos::resource &connection_resource, const utility::string_t &transportfile_type, const utility::string_t &transportfile_data)
{
   // debug log the json objects
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "transportfile_parser: resource: " << resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "transportfile_parser: connection_resource: " << connection_resource.data.serialize();
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "transportfile_parser: transportfile_type: " << transportfile_type;
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "transportfile_parser: transportfile_data: " << transportfile_data;

   web::json::value res = nmos::parse_rtp_transport_file(resource, connection_resource, transportfile_type, transportfile_data, gate);
   slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "transportfile_parser: result: " << res.serialize();
   return res;
}

web::json::value nmos_tools::NodeServerReceiver::generate_constraints()
{
   uint32_t frame_heigth;
   uint32_t frame_width;
   uint32_t frame_rate;
   nmos::rational frame_rate_rational;
   bool8_t interlaced;
   bool8_t is_us;

   web::json::value constraints = web::json::value::array();
   for( uint32_t i = 0; i < NB_VMIP_VIDEO_STANDARD; i++){

      if(VMIP_GetVideoStandardInfo(static_cast<VMIP_VIDEO_STANDARD>(i), &frame_width, &frame_heigth, &frame_rate, &interlaced, &is_us) != VMIPERR_NOERROR)
         throw node_implementation_init_exception("Error while getting video standard info");

      if(is_us)
         frame_rate_rational = nmos::parse_rational(nmos::make_rational(frame_rate * 1000, 1001));
      else
         frame_rate_rational = frame_rate;

      web::json::value constraint = web::json::value::object();
      constraint[nmos::caps::format::grain_rate] = nmos::make_caps_rational_constraint({frame_rate_rational});
      constraint[nmos::caps::format::frame_width] = nmos::make_caps_integer_constraint({frame_width});
      constraint[nmos::caps::format::frame_height] = nmos::make_caps_integer_constraint({frame_heigth});
      constraint[nmos::caps::format::interlace_mode] = nmos::make_caps_string_constraint({interlaced ? nmos::interlace_modes::interlaced_tff.name : nmos::interlace_modes::progressive.name});
      constraint[nmos::caps::format::component_depth] = nmos::make_caps_integer_constraint({10});
      constraint[nmos::caps::format::color_sampling] = nmos::make_caps_string_constraint({U("YCbCr-4:2:2")});
      constraint[nmos::caps::format::transfer_characteristic] = nmos::make_caps_string_constraint({U("SDR")});
      constraints.as_array()[i] = constraint;
   }

   return constraints;
}

utility::string_t nmos_tools::ipv4_to_string(uint32_t ipv4_network_byte_order)
{
   std::stringstream ss;
   ss << ((ipv4_network_byte_order >> 24) & 0xFF) << "."
      << ((ipv4_network_byte_order >> 16) & 0xFF) << "."
      << ((ipv4_network_byte_order >> 8) & 0xFF) << "."
      << (ipv4_network_byte_order & 0xFF);
   return utility::conversions::to_string_t(ss.str());
}

uint32_t nmos_tools::string_to_ipv4(const utility::string_t& ipv4_string)
{
   std::string utf8_str = utility::conversions::to_utf8string(ipv4_string);
   uint32_t ipv4_network_byte_order = 0;
   int octets[4];
   int result = sscanf(utf8_str.c_str(), "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]);
   if (result == 4)
   {
       ipv4_network_byte_order = ((uint32_t)octets[0] << 24) |
                              ((uint32_t)octets[1] << 16) |
                              ((uint32_t)octets[2] << 8) |
                              (uint32_t)octets[3];
   }
   return ipv4_network_byte_order;
}
