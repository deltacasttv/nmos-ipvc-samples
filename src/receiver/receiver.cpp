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

#include <iostream>
#include <vector>
#include <string>
#include <thread>

#ifdef __GNUC__
#include <stdint-gcc.h>
#else
#include <stdint.h>
#endif

#if defined (__linux__) || defined (__APPLE__)
#include "../keyboard.h"
#else
#include <conio.h>
#define init_keyboard()
#define close_keyboard()
#endif

#include "nmos/model.h"
#include "nmos/log_model.h"
#include "nmos/log_gate.h"
#include "nmos/server.h"
#include "nmos/node_server.h"

#include "../tools.h"
#include "../nmos_tools.h"

#include "videoviewer/videoviewer.hpp"

#include <videomasterip/videomasterip_core.h>
#include <videomasterip/videomasterip_conductor.h>
#include <videomasterip/videomasterip_stream.h>

void render_video(Deltacast::VideoViewer& viewer,int window_width, int window_height, const char* window_title, int texture_width, int texture_height, Deltacast::VideoViewer::InputFormat input_format, int frame_rate_in_ms)
{
   if (viewer.init(window_width, window_height, window_title, texture_width, texture_height, input_format))
   {
      viewer.render_loop(frame_rate_in_ms);
      viewer.release();
   }
   else
      std::cout << "VideoViewer initialization failed" << std::endl;
}

int main(int argc, char* argv[])
{
   //Stream parameters
   const std::string media_nic_name = "eno1";  //Streaming network interface controller name

   //VMIP parameters
   const uint32_t conductor_cpu_core_os_id = 1; //IPVC Conductor CPU core index
   const std::vector<uint32_t> processinge_cpu_core_os_id = { 2 }; //IPVC Processing CPU core indexes list
   const uint32_t management_thread_cpu_core_os_id = 3; //IPVC Management Thread CPU core index
   
   //NMOS parameters
   const std::string management_nic_name = "eno2";  //Management network interface controller name
   const std::string management_nic_ip = "192.168.0.10"; //Management network interface controller
   const uint32_t default_destination_address = 0xe0010107; //default IP destination address used for resolving "auto" nmos parameter
   const uint16_t default_destination_udp_port = 1025; //default UDP destination port used for resolving "auto" nmos parameter IP address

   //Node parameters
   const std::string node_name = "IPVC Rx Node";
   const std::string node_description = "IP Virtual Card NMOS RX Demonstration Sample";
   const std::string device_name = "IPVC Rx Device";
   const std::string device_description = "IP Virtual Card RX Device";
   const std::string node_domain = "local.domain."; //domain name where your machine is located
   const int node_api_port = 3212; //port used by the node to expose its registry API
   const int connection_api_port = 3215; //port used by the node to expose its connection API


   HANDLE vcs_context = nullptr, stream = nullptr, slot = nullptr;
   VMIP_VCS_STATUS vcs_status;
   uint64_t media_nic_id = (uint64_t)-1;
   VMIP_STREAMTYPE stream_type = VMIP_ST_RX;
   VMIP_STREAM_COMMON_STATUS stream_status;
   uint64_t conductor_id = (uint64_t)-1;
   VMIP_ERRORCODE result = VMIPERR_NOERROR;
   Deltacast::VideoViewer viewer;

   uint32_t frame_width;
   uint32_t frame_height;
   uint32_t frame_rate;
   bool8_t interlaced;
   bool8_t is_us;

   //Buffer that will be created and filled by the API
   uint8_t* buffer = nullptr;
   uint32_t buffer_size = 0, index = 0;

   bool exit = false;

   init_keyboard();

   std::cout << "IP VIRTUAL CARD NMOS ST2110-20 RECEPTION SAMPLE APPLICATION\n(c) DELTACAST\n--------------------------------------------------------" << std::endl << std::endl;

   //Creates the context in which the stream will be created. This handle will be needed for all following calls.
   result = VMIP_CreateVCSContext("http://localhost:8080/", &vcs_context);
   if(result == VMIPERR_NOERROR)
   {
      result = VMIP_GetVCSStatus(vcs_context, &vcs_status);
      if (result != VMIPERR_NOERROR)
      {
         std::cout << "Error when getting the VCS status"<< " [" << to_string(result) << "]" << std::endl;
      }
      else
         std::cout << "Connection established to VCS " << version_to_string(vcs_status.VcsVersion) << std::endl;
   }
   else
      std::cout << "Error when Creating the context" << " [" << to_string(result) << "]" << std::endl;

   if(result == VMIPERR_NOERROR)
   {
      print_cpu_cores_info(vcs_context);

      print_nics_info(vcs_context);

      //Conductor creation and configuration.
      //The conductor will be responsible of receiving the packets at the fastest possible rate.
      result = configure_conductor(conductor_cpu_core_os_id, vcs_context, &conductor_id);
      if(result == VMIPERR_NOERROR)
      {
         //Conductor start. At this point, the associated CPU core will be used 100%
         result = VMIP_StartConductor(vcs_context, conductor_id);
         if (result != VMIPERR_NOERROR)
            std::cout << "Error when starting conductor" << " [" << to_string(result) << "]" << std::endl;
      }
   }

   if(result == VMIPERR_NOERROR)
   {
      result = get_nic_id_from_name(vcs_context, media_nic_name, &media_nic_id);
      
      if (result == VMIPERR_NOERROR)
      {
         result = VMIP_CreateStream(vcs_context, stream_type, VMIP_ET_ST2110_20, &stream);
         if (result != VMIPERR_NOERROR)
            std::cout << "Error when creating stream" << " [" << to_string(result) << "]" << std::endl;
      }
   }

   nmos_tools::NodeServerReceiver::TransportParams resolve_auto_transport_params;

   if(result == VMIPERR_NOERROR)
   {
      result = get_nic_id_from_name(vcs_context, media_nic_name, &media_nic_id);
      if(result != VMIPERR_NOERROR)
      {
         std::cout << "Error when getting the NIC ID from name" << " [" << to_string(result) << "]" << std::endl;
      }
      else
      {
         VMIP_NETWORK_ITF_CONFIG network_interface_config;
         result = VMIP_GetNetworkInterfaceConfig(vcs_context, media_nic_id, &network_interface_config);
         if(result != VMIPERR_NOERROR)
         {
            std::cout << "Error when getting the network interface configuration" << " [" << to_string(result) << "]" << std::endl;
         }
         else
         {
            resolve_auto_transport_params.ip_interface = network_interface_config.InterfaceIpAddress;
         }
      }
      resolve_auto_transport_params.ip_multicast = default_destination_address;
      resolve_auto_transport_params.ip_src = 0; //no filtering on source ip
      resolve_auto_transport_params.port_dst = default_destination_udp_port;
   }

   nmos::node_model node_model;
   nmos::experimental::log_model log_model;
   nmos::experimental::node_implementation node_implementation;

   std::filebuf error_log_buf;
   std::ostream error_log(std::cerr.rdbuf());
   std::filebuf access_log_buf;
   std::ostream access_log(&access_log_buf);

   nmos::experimental::log_gate gate(error_log, access_log, log_model);

   web::json::value hostAddresses = web::json::value::array(); 
   hostAddresses[0] = web::json::value::string(utility::conversions::to_string_t(management_nic_ip));
   node_model.settings[nmos::fields::host_addresses] = hostAddresses;
   nmos::insert_node_default_settings(node_model.settings);
   node_model.settings[nmos::fields::label] = web::json::value::string(utility::conversions::to_string_t(node_name));
   node_model.settings[nmos::fields::description] = web::json::value::string(utility::conversions::to_string_t(node_description));
   node_model.settings[nmos::fields::domain] = web::json::value::string(utility::conversions::to_string_t(node_domain));
   node_model.settings[nmos::fields::node_port] = web::json::value::number(node_api_port);
   node_model.settings[nmos::fields::connection_port] = web::json::value::number(connection_api_port);
   node_model.settings[nmos::fields::logging_level] = slog::severities::warning; //change this to change the logging level

   log_model.settings = node_model.settings;
   log_model.level = nmos::fields::logging_level(log_model.settings);

   nmos_tools::NodeServerReceiver::TransportParams active_transport_params = resolve_auto_transport_params;
   nmos_tools::NodeServerReceiver node_server(node_model, log_model, gate, device_name, device_description, resolve_auto_transport_params, active_transport_params, media_nic_name);  
   
   if(!node_server.node_implementation_init())
   {
      result = VMIPERR_OPERATIONFAILED;
      std::cout << "Error when initializing the node server" << " [" << to_string(result) << "]" << std::endl;
   }

   if (result == VMIPERR_NOERROR)
   {
      node_server.start();
      std::cout << "NMOS: node ready for connections" << std::endl;
   }

   nmos_tools::NodeServerReceiver::TransportParams previous_transport_params = resolve_auto_transport_params;
   std::string previous_sdp = "INVALID SDP";

   //Get the system parameters and apply new PTP parameters
   nmos_tools::NmosPtpSystemParameters ptp_system_parameters = {};
   nmos_tools::NmosPtpSystemParameters previous_ptp_system_parameters = {0, 0};

   while(result == VMIPERR_NOERROR && !exit){

      //Wait for the stream to be enabled
      while(node_server.is_enabled == false)
      {
         if (_kbhit())
         { 
            _getch();
            exit = true;
            break;
         }
         if (viewer.window_request_close())
         {
            exit = true;
            break;
         }

         //While the stream is disabled, we have to react to PTP changes
         if(node_server.get_ptp_system_parameters(ptp_system_parameters)) //if get_ptp_system_parameters returns false, it means that the PTP system parameters are not available
         {
            if(memcmp(&ptp_system_parameters, &previous_ptp_system_parameters, sizeof(nmos_tools::NmosPtpSystemParameters)) != 0)
            {
               //ptp_system_parameters were changed, we need to update the ptp configuration
               result = apply_ptp_parameters(vcs_context,static_cast<uint8_t>(ptp_system_parameters.domain_number), static_cast<uint8_t>(ptp_system_parameters.announce_receipt_timeout), media_nic_id);
               if(result != VMIPERR_NOERROR)
               {   
                  exit = true;
                  break;
               }

               previous_ptp_system_parameters = ptp_system_parameters;
            }
            print_ptp_status(vcs_context, static_cast<uint8_t>(ptp_system_parameters.domain_number), static_cast<uint8_t>(ptp_system_parameters.announce_receipt_timeout));
         }

         std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      //to not start and stop the transmission
      if(exit)
         break;

      std::string sdp = node_server.get_sdp();
      if(memcmp(&previous_transport_params, &active_transport_params, sizeof(nmos_tools::NodeServerReceiver::TransportParams)) != 0 || sdp != previous_sdp)
      {
         //active parameters were changed, we need to update the stream
         result = VMIP_DestroyStream(stream);
         if (result != VMIPERR_NOERROR)
            std::cout << "Error when destroying the stream" << " [" << to_string(result) << "]" << std::endl;

         if (result == VMIPERR_NOERROR)
         {
            result = VMIP_CreateStream(vcs_context, stream_type, VMIP_ET_ST2110_20, &stream);
            if (result != VMIPERR_NOERROR)
               std::cout << "Error when creating stream" << " [" << to_string(result) << "]" << std::endl;
         }
         if(result == VMIPERR_NOERROR)
         {
            result = configure_stream_from_sdp(vcs_context, stream_type, processinge_cpu_core_os_id,
                                          sdp, active_transport_params.ip_multicast , active_transport_params.port_dst, {media_nic_id}, conductor_id, management_thread_cpu_core_os_id, stream);
            previous_transport_params = active_transport_params;
            previous_sdp = sdp;
         }
         if(result == VMIPERR_NOERROR)
         {
            VMIP_STREAM_ESSENCE_CONFIG essence_config;
            result = VMIP_GetStreamEssenceConfig(stream, &essence_config);
            if(result == VMIPERR_NOERROR)
            {
               result = VMIP_GetVideoStandardInfo(essence_config.EssenceS2110_20Prop.VideoStandard, &frame_width, &frame_height, &frame_rate, &interlaced, &is_us);
            }
         }

      }

      if(result == VMIPERR_NOERROR)
      {
         result = VMIP_StartStream(stream);
         if (result != VMIPERR_NOERROR)
         {
            std::cout << "Error when starting stream" << " [" << to_string(result) << "]" << std::endl;
         }
      }

      if(result == VMIPERR_NOERROR)
      {
         std::cout << std::endl << "Received Sdp : " << std::endl << sdp << std::endl;
         std::cout << std::endl << "Reception started, press any key to stop..." << std::endl;

         uint32_t slot_timeout = 0;

         //start viewer
         std::thread viewerthread(render_video, std::ref(viewer), 800, 600, node_name.c_str(), frame_width, frame_height, Deltacast::VideoViewer::InputFormat::ycbcr_422_10_be, 100);
         bool stop_monitoring = false;
         std::thread monitoring_thread (monitor_rx_stream_status, stream, &stop_monitoring, &slot_timeout);
         uint8_t* data = nullptr;
         uint64_t size = 0;
         
         //Reception loop
         while (1)
         {
            if (_kbhit())
            {
               _getch();
               exit = true;
               break;
            }
            if (viewer.window_request_close())
            {
               exit = true;
               break;
            }

            sdp = node_server.get_sdp();
            if(!node_server.is_enabled || memcmp(&previous_transport_params, &active_transport_params, sizeof(nmos_tools::NodeServerReceiver::TransportParams)) != 0 || sdp != previous_sdp)
               break;

            //Try to lock the next slot.
            result = VMIP_LockSlot(stream, &slot);
            if (result != VMIPERR_NOERROR)
            {
               if (result == VMIPERR_TIMEOUT)
               {
                  slot_timeout++;
                  result = VMIPERR_NOERROR; //After the above print message, timeout error is considered as handled
                  continue;
               }
               std::cout << "Error when locking slot " << index << " [" << to_string(result) << "]" << std::endl;
               break;
            }

            //Get the video buffer associated to the slot.
            result = VMIP_GetSlotBuffer(stream, slot, VMIP_ST2110_20_BT_VIDEO, &buffer, &buffer_size);
            if (result != VMIPERR_NOERROR)
            {
               std::cout << "Error when getting slot buffer at slot " << index << " [" << to_string(result) << "]" << std::endl;
            }

            viewer.lock_data(&data, &size);
            if(size != buffer_size)
            {
               std::cout << "Buffer size ("<< buffer_size <<") does not match with videoviewer data size (" << size << ")" << std::endl;
            }
            else
            {
               memcpy(data, buffer, size);
            }
            viewer.unlock_data();

            //Unlock the slot. buffer wont be available anymore
            result = VMIP_UnlockSlot(stream, slot);
            if (result != VMIPERR_NOERROR)
            {
               std::cout << "Error when unlocking slot at slot " << index << " [" << to_string(result) << "]" << std::endl;
               break;
            }

            index++;
         }

         stop_monitoring = true;
         monitoring_thread.join();

         viewer.stop();
         viewerthread.join();

         VMIP_ERRORCODE result_stop_stream; //temporary variable to not overwrite result if an error occured in the transmission loop

         result_stop_stream = VMIP_StopStream(stream);
         if (result_stop_stream != VMIPERR_NOERROR)
         {
            std::cout << "Error when stopping the stream" << " [" << to_string(result_stop_stream) << "]" << std::endl;
            result = result_stop_stream;
         }
      }
   }
   
   if(stream)
   {
      result = VMIP_DestroyStream(stream);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when destroying the stream" << " [" << to_string(result) << "]" << std::endl;
   }

   if(conductor_id != (uint64_t)-1)
   {
      result = VMIP_StopConductor(vcs_context, conductor_id);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when stopping the conductor" << " [" << to_string(result) << "]" << std::endl;

      result = VMIP_DestroyConductor(vcs_context, conductor_id);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when destroying the conductor" << " [" << to_string(result) << "]" << std::endl;
   }

   if(vcs_context)
   {
      result = VMIP_DestroyVCSContext(vcs_context);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when destroying the context" << " [" << to_string(result) << "]" << std::endl;
   }

   close_keyboard();

   node_server.stop();

   return 0;
}