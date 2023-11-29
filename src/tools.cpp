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

#define _USE_MATH_DEFINES
#include <cmath>

#include "tools.h"

#include <videomasterip/videomasterip_conductor.h>
#include <videomasterip/videomasterip_networkinterface.h>
#include <videomasterip/videomasterip_cpu.h>
#include <videomasterip/videomasterip_sdp.h>

#include <iostream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <array>
#include <thread>

constexpr uint32_t print_tab_space = 10;

VMIP_ERRORCODE print_cpu_cores_info(HANDLE vcs_context)
{
   uint64_t* cpu_cores_id_list = new uint64_t[MAX_HANDLE_ARRAY_SIZE];
   uint32_t nb_cpu_core = 0;
   VMIP_CPUCORE_STATUS cpu_status;

   VMIP_ERRORCODE result = VMIP_GetCPUCoresHandlesList(vcs_context, cpu_cores_id_list, &nb_cpu_core);
   if (result == VMIPERR_NOERROR)
   {
      std::cout << std::endl << nb_cpu_core << "  CPU cores detected." << std::endl;
      std::cout << std::setfill(' ')
         << std::setw(print_tab_space) << "Index"
         << std::setw(print_tab_space) << "OsId"
         << std::setw(print_tab_space) << "Usage (%)"
         << std::setw(2 * print_tab_space) << "MaxSpeed (MHz)"
         << std::setw(print_tab_space * 2) << "Conductor Avail."
         << std::setw(print_tab_space) << "Numa" << std::endl;

      for (uint32_t i = 0; i < nb_cpu_core; i++)
      {
         result = VMIP_GetCpuCoreStatus(vcs_context, cpu_cores_id_list[i], &cpu_status);
         if (result != VMIPERR_NOERROR)
         {
            std::cout << std::endl << "Error while getting the status of CPUCore " << i << "%u.[" << to_string(result) << "]." <<
               std::endl;
            break;
         }
         else
         {
            std::cout << std::setfill(' ')
               << std::setw(print_tab_space) << i
               << std::setw(print_tab_space) << cpu_status.CoreId
               << std::setw(print_tab_space) << uint32_t(cpu_status.CoreUsage)
               << std::setw(2 * print_tab_space) << cpu_status.MaxSpeed
               << std::setw(print_tab_space * 2) << to_string(cpu_status.ConductorAvailability)
               << std::setw(print_tab_space) << cpu_status.NumaNode << std::endl;
         }
      }
   }
   else
      std::cout << "\n Error while getting the number of CPUCore [" << to_string(result) << "]" << std::endl;

   delete[] cpu_cores_id_list;

   return result;
}

VMIP_ERRORCODE print_nics_info(HANDLE vcs_context)
{
   uint64_t* network_interface_id_list = new uint64_t[MAX_HANDLE_ARRAY_SIZE];
   uint32_t nb_network_interface = 0;
   VMIP_NETWORK_ITF_STATUS network_interface_status;

   std::string nic_name = "";
   std::string driver_name = "";

   VMIP_ERRORCODE result = VMIP_GetNetworkInterfaceHandlesList(vcs_context, network_interface_id_list, &nb_network_interface);
   if (result == VMIPERR_NOERROR)
   {
      std::cout << std::endl << nb_network_interface << " network interfaces detected." << std::endl;
      std::cout << std::setfill(' ') << std::setw(print_tab_space) << "Index"
         << std::setfill(' ') << std::setw(2 * print_tab_space) << "Name"
         << std::setfill(' ') << std::setw(print_tab_space) << "Numa"
         << std::setfill(' ') << std::setw(print_tab_space) << "Duplex"
         << std::setfill(' ') << std::setw(2 * print_tab_space) << "Link Status"
         << std::setfill(' ') << std::setw(2 * print_tab_space) << "Speed (Mbits/sec)"
         << std::setfill(' ') << std::setw(2 * print_tab_space) << "MAC"
         << std::setfill(' ') << std::setw(print_tab_space) << "OsId"
         << std::setfill(' ') << std::setw(2 * print_tab_space) << "Driver" << std::endl;

      for (uint32_t i = 0; i < nb_network_interface; i++)
      {
         result = VMIP_GetNetworkInterfaceStatus(vcs_context, network_interface_id_list[i], &network_interface_status);
         if (result != VMIPERR_NOERROR)
         {
            std::cout << std::endl << "Error while getting the status of Interface %u " << i << "%u.[" << to_string(result) << "]." << std::endl;
            break;
         }
         else
         {
            nic_name = std::string(network_interface_status.pName);
            driver_name = std::string(network_interface_status.pDriverVersion);
            std::cout
               << std::setfill(' ') << std::setw(print_tab_space) << i
               << std::setfill(' ') << std::setw(2 * print_tab_space) << nic_name
               << std::setfill(' ') << std::setw(print_tab_space) << network_interface_status.NumaNode
               << std::setfill(' ') << std::setw(print_tab_space) << to_string(network_interface_status.DuplexMode)
               << std::setfill(' ') << std::setw(2 * print_tab_space) << uint32_t(network_interface_status.LinkStatus)
               << std::setfill(' ') << std::setw(2 * print_tab_space) << network_interface_status.LinkSpeed
               << "        " << std::setfill('0') << std::setw(12) << std::hex << network_interface_status.MacAddress << std::dec
               << std::setfill(' ') << std::setw(print_tab_space) << network_interface_status.OsId
               << std::setfill(' ') << std::setw(2 * print_tab_space) << driver_name << std::endl;
         }
      }
   }
   else
      std::cout << std::endl << " Error while getting the number of NetInterfaces [" << to_string(result) << "]." << std::endl;

   delete[] network_interface_id_list;

   return result;
}

VMIP_ERRORCODE get_nic_id_from_name(HANDLE vcs_context, std::string nic_name, uint64_t* nice_id)
{
   uint64_t* network_interface_id_list = new uint64_t[MAX_HANDLE_ARRAY_SIZE];
   uint32_t nb_network_interface = 0;
   VMIP_NETWORK_ITF_STATUS network_interface_status;

   if (nice_id == nullptr)
   {
      std::cout << std::endl << "The  nice_id pointer is invalid" << std::endl;
      return VMIPERR_INVALIDPOINTER;
   }

   VMIP_ERRORCODE result = VMIP_GetNetworkInterfaceHandlesList(vcs_context, network_interface_id_list, &nb_network_interface);
   if (result == VMIPERR_NOERROR)
   {
      for (uint32_t i = 0; i < nb_network_interface; i++)
      {
         result = VMIP_GetNetworkInterfaceStatus(vcs_context, network_interface_id_list[i], &network_interface_status);
         if (result != VMIPERR_NOERROR)
         {
            std::cout << std::endl << "Error while getting the status of Interface %u " << i << "%u.[" << to_string(result) << "]." << std::endl;
            break;
         }
         else
         {
            if (strcmp(network_interface_status.pName, nic_name.c_str()) == 0)
               *nice_id = network_interface_id_list[i];
         }
      }
      if (*nice_id == 0)
      {
         std::cout << std::endl << " No NIC with the name " << nic_name << " was found." << std::endl;
         result = VMIPERR_NOTFOUND;
      }
   }
   else
      std::cout << std::endl << " Error while getting the number of NetInterfaces [" << to_string(result) << "]." << std::endl;

   delete[] network_interface_id_list;

   return result;
}

std::string to_string(VMIP_NETWORK_ITF_DUPLEX_MODE duplex_mode)
{
   std::string result = "Undefined";

   switch (duplex_mode)
   {
   case VMIP_NETWORK_ITF_DUPLEX_MODE_HALF: result = "Half";
      break;
   case VMIP_NETWORK_ITF_DUPLEX_MODE_FULL: result = "Full";
      break;
   case VMIP_NETWORK_ITF_DUPLEX_MODE_UNKNOWN: result = "Unknown";
      break;
   case NB_NETWORK_ITF_DUPLEX_MODE:
   default: break;
   }

   return result;
}

std::string to_string(VMIP_PTP_STATE ptp_state)
{
   std::string result = "Undefined";

   switch (ptp_state)
   {
   case VMIP_PTP_STATE_INITIZALIZING: result = "Initializing"; break;
   case VMIP_PTP_STATE_FAULTY: result = "Faulty"; break;
   case VMIP_PTP_STATE_DISABLED: result = "Disabled"; break;
   case VMIP_PTP_STATE_LISTENING: result = "Listening"; break;
   case VMIP_PTP_STATE_PREMASTER: result = "Pre-Master"; break;
   case VMIP_PTP_STATE_MASTER: result = "Master"; break;
   case VMIP_PTP_STATE_PASSIVE: result = "Passive"; break;
   case VMIP_PTP_STATE_UNCALIBRATED: result = "Uncalibrated"; break;
   case VMIP_PTP_STATE_SLAVE: result = "Slave"; break;
   case NB_VMIP_PTP_STATE:
   default: break;
   }

   return result;
}

std::string to_string(VMIP_CONDUCTOR_AVAILABILITY value)
{
   std::string result = "Undefined";

   switch (value)
   {
   case VMIP_CONDUCTOR_AVAILABLE: result = "Available"; break;
   case VMIP_CONDUCTOR_NOT_AVAILABLE: result = "NotAvail."; break;
   case VMIP_CONDUCTOR_ALREADY_USED: result = "Alr Used"; break;
   case NB_VMIP_CONDUCTOR_AVAILABILITY: break;
   default:;
   }

   return result;
}

VMIP_ERRORCODE configure_conductor(const uint32_t conductor_cpu_core_os_id, HANDLE vcs_context,
   uint64_t* conductor_id)
{
   VMIP_ERRORCODE result = VMIPERR_NOERROR;

   if (conductor_id == nullptr)
   {
      std::cout << std::endl << "The  conductor_id pointer is invalid" << std::endl;
      return VMIPERR_INVALIDPOINTER;
   }

   //We create the conductor. The conductor will be responsible of sending the packet at the correct rate to be compliant with the ST2110-20 norm.
   //A conductor can be shared by Vcs Context but this is not supported in this Beta.
   if (result == VMIPERR_NOERROR)
   {
      result = VMIP_CreateConductor(vcs_context, false, conductor_id);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when creating conductor." << std::endl;
   }

   //We associate the conductor to a Cpu Core by setting its configuration
   if (result == VMIPERR_NOERROR)
   {
      VMIP_CONDUCTOR_CONFIG conductor_config;

      //We get the cpu Core Id from the Os ID
      result = VMIP_GetCPUCoreFromOSId(vcs_context, conductor_cpu_core_os_id, &conductor_config.CpuCoreId);
      if (result != VMIPERR_NOERROR)
         std::cout << "Could not get the CpuId for Os Id = " << conductor_cpu_core_os_id << " [" << to_string(result) << "]" << std::endl;
      else
      {
         result = VMIP_SetConductorConfig(vcs_context, *conductor_id, conductor_config);
         if (result != VMIPERR_NOERROR)
            std::cout << "Error when setting conductor config." << std::endl;
      }
   }

   return result;
}

VMIP_ERRORCODE configure_stream(HANDLE vcs_context, VMIP_STREAMTYPE stream_type,
                               const std::vector<uint32_t>& processing_cpu_core_os_id,
                               const std::vector<uint32_t>& destination_addresses,
                               const std::vector<uint16_t>& destination_udp_ports,
                               const uint32_t destination_ssrc,
                               const VMIP_VIDEO_STANDARD video_standard, 
                               const std::vector<uint64_t>& nic_ids,
                               uint64_t conductor_id, uint32_t management_thread_cpu_core_os_id, HANDLE stream)
{
   VMIP_STREAM_COMMON_CONFIG stream_common_config;
   VMIP_STREAM_NETWORK_CONFIG stream_network_config;
   VMIP_STREAM_ESSENCE_CONFIG essence_config;
   VMIP_ERRORCODE result = VMIPERR_NOERROR;

   if (stream == nullptr)
   {
      std::cout << std::endl << "The pStream pointer is invalid" << std::endl;
      return VMIPERR_INVALIDPOINTER;
   }

   if(destination_addresses.size() != destination_udp_ports.size() || destination_addresses.size() != nic_ids.size())
   {
      std::cout << std::endl << "The sizes of destination_addresses and destination_udp_ports do not correspond" << std::endl;
      return VMIPERR_BAD_CONFIGURATION;
   }

   //We get the stream common config. It is also a way to initialize the structure stream_common_config.
   if (result == VMIPERR_NOERROR)
   {
      result = VMIP_GetStreamCommonConfig(stream, &stream_common_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting the stream common config" << std::endl;
   }

   // We get the CPU id for the management thread CPU core.
   if (result == VMIPERR_NOERROR)
   {
      result = VMIP_GetCPUCoreFromOSId(vcs_context, management_thread_cpu_core_os_id, &stream_common_config.ManagementThreadCpuCoreId);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting the CPU id for OS id " << management_thread_cpu_core_os_id << " [" << to_string(result) << "]" << std::endl;
   }

   //We set the stream common config.
   if (result == VMIPERR_NOERROR)
   {
      //We associate a stream to a conductor.
      stream_common_config.ConductorId = conductor_id;

      //We associate the stream to an array of Cpu core. The processing tasks will run on those Cpu cores.
      uint64_t tmp_cpu_id = 0;
      stream_common_config.NbCpuCoreForProcessing = 0;

      for (auto cpu_os_id : processing_cpu_core_os_id)
      {
         if (VMIP_GetCPUCoreFromOSId(vcs_context, cpu_os_id, &tmp_cpu_id) == VMIPERR_NOERROR)
         {
            stream_common_config.pCpuCoreIdxForProcessing[stream_common_config.NbCpuCoreForProcessing] = tmp_cpu_id;
            stream_common_config.NbCpuCoreForProcessing++;
         }
      }

      result = VMIP_SetStreamCommonConfig(stream, stream_common_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when setting stream common config" << " [" << to_string(result) << "]" << std::endl;
   }

   //We get the stream network config. It is also a way to initialize the structure stream_network_config.
   if (result == VMIPERR_NOERROR)
   {
      result = VMIP_GetStreamNetworkConfig(stream, &stream_network_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting stream network config" << " [" << to_string(result) << "]" << std::endl;
   }

   //We set the network configuration.
   if (result == VMIPERR_NOERROR)
   {
      stream_network_config.PathParameterSize = destination_addresses.size();
      for(uint32_t i = 0; i < destination_addresses.size(); i++)
      {
         stream_network_config.pPathParameters[i].DestinationIp = destination_addresses[i];
         stream_network_config.pPathParameters[i].UdpPort = destination_udp_ports[i];
         //We associate the stream to one or multiples NICs. The packets will be issued on this/those NICs.
         stream_network_config.pPathParameters[i].InterfaceId = nic_ids[i];
      }
      stream_network_config.PayloadType = 96;
      stream_network_config.Ssrc = destination_ssrc;

      result = VMIP_SetStreamNetworkConfig(stream, stream_network_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when setting stream network config" << " [" << to_string(result) << "]" << std::endl;
   }

   //We get the stream essence config. It is also a way to initialize the structure essence_config.
   if (result == VMIPERR_NOERROR)
   {
      essence_config.EssenceType = VMIP_ET_ST2110_20;
      result = VMIP_GetStreamEssenceConfig(stream, &essence_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting stream essence config" << " [" << to_string(result) << "]" << std::endl;
   }

   //We set the stream essence config.
   if (result == VMIPERR_NOERROR)
   {
      essence_config.EssenceS2110_20Prop.NetworkBitDepth = VMIP_VIDEO_DEPTH_10BIT;
      essence_config.EssenceS2110_20Prop.NetworkBitSampling = VMIP_VIDEO_SAMPLING_YCBCR_422;
      essence_config.EssenceS2110_20Prop.UserBitDepth = VMIP_VIDEO_DEPTH_10BIT;
      essence_config.EssenceS2110_20Prop.UserBitSampling = VMIP_VIDEO_SAMPLING_YCBCR_422;
      essence_config.EssenceS2110_20Prop.UserBitPadding = VMIP_VIDEO_NO_PADDING;
      essence_config.EssenceS2110_20Prop.UserEndianness = VMIP_VIDEO_BIG_ENDIAN;
      essence_config.EssenceS2110_20Prop.UseDefaultTrOffset = true;
      essence_config.EssenceS2110_20Prop.VideoStandard = video_standard;

      if (result == VMIPERR_NOERROR)
      {
         result = VMIP_SetStreamEssenceConfig(stream, essence_config);
         if (result != VMIPERR_NOERROR)
            std::cout << "Error when setting stream essence config"<< " [" << to_string(result) << "]" << std::endl;
      }
      else
         std::cout << "Error in Essence type" << std::endl;
   }

   return result;
}

VMIP_ERRORCODE configure_stream_from_sdp(HANDLE vcs_context, VMIP_STREAMTYPE stream_type,
                                      const std::vector<uint32_t>& rProcessingeCpuCoreOsId, std::string Sdp,
                                      const uint32_t destination_ip_overrides, const uint16_t destination_udp_port_ovverrides,
                                      const std::vector<uint64_t>& nic_ids, uint64_t conductor_id,
                                      uint32_t management_thread_cpu_core_os_id, HANDLE stream)
{
   VMIP_STREAM_COMMON_CONFIG stream_common_config;
   VMIP_STREAM_NETWORK_CONFIG stream_network_config;
   VMIP_STREAM_ESSENCE_CONFIG essence_config;
   VMIP_SDP_ADDITIONAL_INFORMATION sdp_additional_information;
   VMIP_ERRORCODE result = VMIPERR_NOERROR;

   if (stream == nullptr)
   {
      std::cout << std::endl << "The  pStream pointer is invalid" << std::endl;
      return VMIPERR_INVALIDPOINTER;
   }

   //Fill the structure with the information contained in the SDP
   if (result == VMIPERR_NOERROR)
   {
      result = VMIP_ReadSDP(Sdp.c_str(), Sdp.size(), &stream_network_config, &essence_config, &sdp_additional_information);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when reading the SDP" << " [" << to_string(result) << "]" << std::endl;
   }

   //We get the stream common config. It is also a way to initialize the structure stream_common_config.
   if (result == VMIPERR_NOERROR)
   {
      result = VMIP_GetStreamCommonConfig(stream, &stream_common_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting the stream common config" << " [" << to_string(result) << "]" << std::endl;
   }

   // We get the CPU id for the management thread CPU core.
   if (result == VMIPERR_NOERROR)
   {
      result = VMIP_GetCPUCoreFromOSId(vcs_context, management_thread_cpu_core_os_id, &stream_common_config.ManagementThreadCpuCoreId);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting the CPU id for OS id " << management_thread_cpu_core_os_id << " [" << to_string(result) << "]" << std::endl;
   }

   //We set the stream common config.
   if (result == VMIPERR_NOERROR)
   {
      //We fill parameters that are not present in the SDP.

      //We associate a stream to a conductor.
      stream_common_config.ConductorId = conductor_id;

      //We associate the stream to an array of Cpu core. The processing tasks will run on those Cpu cores.
      uint64_t tmp_cpu_id = 0;
      stream_common_config.NbCpuCoreForProcessing = 0;

      for (auto cpu_os_id : rProcessingeCpuCoreOsId)
      {
         if (VMIP_GetCPUCoreFromOSId(vcs_context, cpu_os_id, &tmp_cpu_id) == VMIPERR_NOERROR)
         {
            stream_common_config.pCpuCoreIdxForProcessing[stream_common_config.NbCpuCoreForProcessing] = tmp_cpu_id;
            stream_common_config.NbCpuCoreForProcessing++;
         }
      }

      result = VMIP_SetStreamCommonConfig(stream, stream_common_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when setting stream common config" << " [" << to_string(result) << "]" << std::endl;
   }

   if(nic_ids.size() != stream_network_config.PathParameterSize)
   {
      std::cout << std::endl << "The number of path configured in the SDP and the number of Nic do not correspond" << std::endl;
      result = VMIPERR_BAD_CONFIGURATION;
   }

   //Wet set the network configuration.
   if (result == VMIPERR_NOERROR)
   {
      //We fill parameters that are not present in the SDP.
      stream_network_config.Ssrc = 0;
      for (uint32_t path_index = 0; path_index < stream_network_config.PathParameterSize; path_index++)
      {
         stream_network_config.pPathParameters[path_index].InterfaceId = nic_ids[path_index];
         stream_network_config.pPathParameters[path_index].IgmpFilteringType = VMIP_FILTERING_BLACKLIST;
         stream_network_config.pPathParameters[path_index].IgmpSourceListSize = 0;
      }

      stream_network_config.pPathParameters[0].DestinationIp = destination_ip_overrides;
      stream_network_config.pPathParameters[0].UdpPort = destination_udp_port_ovverrides;

      result = VMIP_SetStreamNetworkConfig(stream, stream_network_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when setting stream network config." << " [" << to_string(result) << "]" << std::endl;
   }

   //We set the stream essence config.
   if (result == VMIPERR_NOERROR)
   {
      switch (essence_config.EssenceType)
      {
      case VMIP_ET_ST2110_20:
         essence_config.EssenceS2110_20Prop.UserBitDepth = essence_config.EssenceS2110_20Prop.NetworkBitDepth;
         essence_config.EssenceS2110_20Prop.UserBitSampling = essence_config.EssenceS2110_20Prop.NetworkBitSampling;
         essence_config.EssenceS2110_20Prop.UserBitPadding = VMIP_VIDEO_NO_PADDING;
         essence_config.EssenceS2110_20Prop.UserEndianness = VMIP_VIDEO_BIG_ENDIAN;
         essence_config.EssenceS2110_20Prop.UseDefaultTrOffset = true;
         break;
      default:
         result = VMIPERR_BADARG;
         break;
      }

      if (result == VMIPERR_NOERROR)
      {
         result = VMIP_SetStreamEssenceConfig(stream, essence_config);
         if (result != VMIPERR_NOERROR)
            std::cout << "Error when setting stream essence config." << " [" << to_string(result) << "]" << std::endl;
      }
      else
         std::cout << "Error in Essence type" << std::endl;
   }

   return result;
}

VMIP_ERRORCODE print_ptp_status(HANDLE vcs_context)
{
   VMIP_ERRORCODE result;
   VMIP_PTP_STATUS ptp_status = {};

   result = VMIP_GetPTPStatus(vcs_context, &ptp_status);
   if (result != VMIPERR_NOERROR)
      std::cout << std::endl << "Error when getting the PTP status" << " [" << to_string(result) << "]" << std::endl;
   else
   {
      std::cout << "PTP State = " << to_string(ptp_status.PortDS.PortState) << " (Offset : " << ptp_status.CurrentDS.OffsetFromMaster
         << " seconds)        \r" << std::flush;
   }
   return result;
}

std::string version_to_string(uint64_t version /*!< [in] Version that will be stringified*/)
{
   std::string major_version = std::to_string((version & 0xffff000000000000) >> 48);
   std::string minor_version = std::to_string((version & 0xffff00000000) >> 32);
   std::string build_version = std::to_string(version & 0xffffffff);

   return major_version + "." + minor_version + "." + build_version;
}

std::string to_string(VMIP_ERRORCODE error_code)
{
   char error_string[50];
   VMIP_ErrorToString(error_code, error_string);
   return std::string(error_string);
}

VMIP_ERRORCODE generate_sdp(HANDLE vcs_context, HANDLE stream, std::string* sdp)
{
   VMIP_ERRORCODE result = VMIPERR_NOERROR;

   VMIP_STREAM_NETWORK_CONFIG stream_network_config;
   VMIP_STREAM_ESSENCE_CONFIG essence_config;

   if (sdp == nullptr)
   {
      std::cout << std::endl << "The Sdp pointer is invalid" << std::endl;
      return VMIPERR_INVALIDPOINTER;
   }

   if (result == VMIPERR_NOERROR)
   {
      result = VMIP_GetStreamNetworkConfig(stream, &stream_network_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting the stream network config" << " [" << to_string(result) << "]" << std::endl;
   }

   if (result == VMIPERR_NOERROR)
   {
      result = VMIP_GetStreamEssenceConfig(stream, &essence_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting the essence config" << " [" << to_string(result) << "]" << std::endl;
   }

   if (result == VMIPERR_NOERROR)
      result = generate_sdp(vcs_context, stream_network_config, essence_config, sdp);

   return result;
}

VMIP_ERRORCODE generate_sdp(HANDLE vcs_context, VMIP_STREAM_NETWORK_CONFIG stream_network_config,
   VMIP_STREAM_ESSENCE_CONFIG essence_config, std::string* sdp)
{
   VMIP_ERRORCODE result = VMIPERR_NOERROR;


   VMIP_SDP_ADDITIONAL_INFORMATION sdp_additional_information;

   if (sdp == nullptr)
   {
      std::cout << std::endl << "The Sdp pointer is invalid" << std::endl;
      return VMIPERR_INVALIDPOINTER;
   }

   if (result == VMIPERR_NOERROR)
   {
      VMIP_NETWORK_ITF_CONFIG nic_config;
      result = VMIP_GetNetworkInterfaceConfig(vcs_context,stream_network_config.pPathParameters[0].InterfaceId, &nic_config);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting the network interface config" << " [" << to_string(result) << "]" << std::endl;
      else
         stream_network_config.pPathParameters[0].SourceIp = nic_config.InterfaceIpAddress;
   }

   if (result == VMIPERR_NOERROR)
   {
      VMIP_NETWORK_ITF_STATUS nic_status;
      result = VMIP_GetNetworkInterfaceStatus(vcs_context,stream_network_config.pPathParameters[0].InterfaceId, &nic_status);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when getting the network interface status" << " [" << to_string(result) << "]" << std::endl;
      else
         sdp_additional_information.TrafficShapingProfile = nic_status.TrafficShapingProfile;
   }

   if (result == VMIPERR_NOERROR)
   {
      sdp_additional_information.MediaClockType = VMIP_MEDIA_CLOCK_DIRECT;
      sdp_additional_information.TimestampMode = VMIP_TS_MODE_NEW;
      sdp_additional_information.ReferenceClock.ReferenceClockType = VMIP_REF_CLOCK_PTP_GRANDMASTER_ID;
      sdp_additional_information.ReferenceClock.Grandmaster.Domain = 127;
      const std::array<uint8_t, 8> master_id = { 0xd0, 0xcb, 0x07, 0xfe, 0xff, 0x94, 0xa7, 0x39 };
      memcpy(sdp_additional_information.ReferenceClock.Grandmaster.pId, master_id.data(), master_id.size());
      sdp_additional_information.HasTimestampDelay = true;
      sdp_additional_information.TimestampDelay = 1000;
      snprintf(sdp_additional_information.pSessionName, MAX_CHAR_ARRAY_SIZE, "Sample Media Stream");
      snprintf(sdp_additional_information.pSessionDescription, MAX_CHAR_ARRAY_SIZE, "media stream created with Deltacast IPVirtualCard");
      if (essence_config.EssenceType == VMIP_ET_ST2110_30)
         snprintf(sdp_additional_information.pAudioChannelOrder, MAX_CHAR_AUDIO_CHANNEL_ORDER_SIZE, "SMPTE2110.(M,M,M,M,ST,U02)");

      const uint32_t max_sdp_size = 65536;
      uint32_t sdp_size = max_sdp_size;
      char sdp_char[max_sdp_size];
      result = VMIP_WriteSDP(stream_network_config, essence_config, sdp_additional_information, sdp_char, &sdp_size);
      if (result != VMIPERR_NOERROR)
         std::cout << "Error when writing the SDP" << std::endl;
      else
         *sdp = std::string(sdp_char);
   }

   return result;
}

VMIP_ERRORCODE apply_ptp_parameters(HANDLE vcs_context, uint8_t domain_number, uint8_t announce_receipt_timeout, uint64_t network_interface_id)
{
   VMIP_ERRORCODE result = VMIPERR_NOERROR;
   VMIP_PTP_CONFIG ptp_config = {};
   result = VMIP_CreatePTPDSFromProfile(VMIP_PTP_PROFILE_ST2059_2, &ptp_config.DefaultDS, &ptp_config.PortDS);
   if (result != VMIPERR_NOERROR)
      std::cout << std::endl << "Error when creating the PTP profile" << " [" << to_string(result) << "]" << std::endl;
   else
   {
      ptp_config.DefaultDS.DomainNumber = domain_number;
      ptp_config.PortDS.AnnounceReceiptTimeout = announce_receipt_timeout;
      ptp_config.NetworkInterfaceId = network_interface_id;
      result = VMIP_SetPTPConfig(vcs_context, ptp_config);
      if (result != VMIPERR_NOERROR)
         std::cout << std::endl << "Error when setting the PTP config" << " [" << to_string(result) << "]" << std::endl;
   }
   return result;
}

VMIP_ERRORCODE print_ptp_status(HANDLE vcs_context, uint8_t domain_number, uint8_t announce_receipt_timeout)
{
   VMIP_ERRORCODE result;
   VMIP_PTP_STATUS ptp_status = {};

   result = VMIP_GetPTPStatus(vcs_context, &ptp_status);
   if (result != VMIPERR_NOERROR)
      std::cout << std::endl << "Error when getting the PTP status. " << std::endl;
   else
   {
      std::cout << "PTP : Domain Number = " << static_cast<int>(domain_number) << " Announce Receipt Timeout = " << static_cast<int>(announce_receipt_timeout) << " State = " << to_string(ptp_status.PortDS.PortState) << " (Offset : " << ptp_status.CurrentDS.OffsetFromMaster
         << " seconds)                 \r" << std::flush;
   }
   return result;
}

using namespace std::chrono_literals;
void monitor_rx_stream_status(HANDLE stream, bool* request_stop, uint32_t* timeout)
{
   VMIP_STREAM_COMMON_STATUS stream_common_status;
   VMIP_STREAM_NETWORK_STATUS stream_network_status;

   while(!*request_stop)
   {
      VMIP_GetStreamCommonStatus(stream, &stream_common_status);
      VMIP_GetStreamNetworkStatus(stream, &stream_network_status);

      std::cout << "SlotCount: " << stream_common_status.SlotCount << " - SlotFilling: " << stream_common_status.ApplicativeBufferQueueFilling << " - SlotDropped: " << stream_common_status.SlotDropped << " - PacketLost:  " << stream_network_status.PacketLost << " - Timeout: " << *timeout <<"                      \r" << std::flush;

      std::this_thread::sleep_for(100ms);
   }
}

void monitor_tx_stream_status(HANDLE stream, bool* request_stop)
{
   VMIP_STREAM_COMMON_STATUS stream_common_status;
   VMIP_STREAM_NETWORK_STATUS stream_network_status;

   while(!*request_stop)
   {
      VMIP_GetStreamCommonStatus(stream, &stream_common_status);
      VMIP_GetStreamNetworkStatus(stream, &stream_network_status);

      std::cout << "SlotCount: " << stream_common_status.SlotCount << " - SlotFilling: " << stream_common_status.ApplicativeBufferQueueFilling << " - SlotDropped: " << stream_common_status.SlotDropped << " - PacketUnderrun:  " << stream_network_status.PacketUnderrun<< " - PacketDrop:  " << stream_network_status.PacketDrop << "                      \r" << std::flush;

      std::this_thread::sleep_for(100ms);
   }

   std::cout << std::endl;
}