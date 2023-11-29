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

#pragma once
/*!
   @file Tools.h
   @brief This file contains some helping functions used in the samples.
*/

#include <videomasterip/videomasterip_core.h>
#include <videomasterip/videomasterip_networkinterface.h>
#include <videomasterip/videomasterip_stream.h>
#include <videomasterip/videomasterip_ptp.h>
#include <videomasterip/videomasterip_cpu.h>

#ifdef __GNUC__
#include <stdint-gcc.h>
#else
#include <stdint.h>
#endif

#include <iostream>
#include <string>
#include <vector>

/*!
   @brief This function prints the information regarding the detected CPU's. CPU's mentionned as forbidden in the configuration file will not appear.

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE print_cpu_cores_info(HANDLE vcs_context /*!< [in] Context of the VCS session */);

/*!
   @brief This function prints the information regarding the detected network interfaces. Network interfaces mentionned as forbidden in the configuration file will not appear.

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE print_nics_info(HANDLE vcs_context /*!< [in] Context of the VCS session */);

/*!
   @brief The function provides the network interface ID based on its OS name.

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE get_nic_id_from_name(HANDLE vcs_context /*!< [in] Context of the VCS session */
   , std::string nic_name /*!< [in] Name of the network interface */
   , uint64_t* nic_id /*!< [out] ID of the network interface */
);

/*!
   @brief This function provides a string representation of the VMIP_NETWORK_ITF_DUPLEX_MODE enumeration value.

   @returns a string representation of the VMIP_NETWORK_ITF_DUPLEX_MODE enumeration value.
*/
std::string to_string(VMIP_NETWORK_ITF_DUPLEX_MODE duplex_mode /*!< [in] Duplex mode that we want to stringify. */);

/*!
   @brief This function provides a string representation of the VMIP_PTP_STATE enumeration value.

   @returns a string representation of the VMIP_PTP_STATE enumeration value.
*/
std::string to_string(VMIP_PTP_STATE ptp_state /*!< [in] PTP state that we want to stringify.*/);

/*!
   @brief This function provides a string representation of the VMIP_CONDUCTOR_AVAILABILITY enumeration value.

   @returns a string representation of the VMIP_CONDUCTOR_AVAILABILITY enumeration value.
*/
std::string to_string(VMIP_CONDUCTOR_AVAILABILITY value);

/*!
   @brief This function allows to configure a network interface and provides the corresponding ID.

   @detail The call to this function is optional. If it is not called, the OS configuration will not be modified.

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE configure_nic(HANDLE vcs_context /*!< [in] Context of the VCS session */
   , const std::string nic_name /*!< [in] Name of the interface that we want to configure */
   , const uint32_t local_ip_address /*!< [in] Ip address that will be configured on the NIC.*/
   , const uint32_t local_subnet_mask /*!< [in] Subnet mask that will be configured on the NIC.*/
   , uint64_t* nic_id /*!< [out] ID of the NIC*/
);

/*!
   @brief This function manages conductor creation and configuration

   @details
   This core will be busy at 100% during the stream.
   Using the Cpu core 0 should be avoided as interruption on this core can disrupt the traffic shaping.
   In TX, the conductor will be responsible of sending the packets on the network at the correct rate, in order to be compliant with the ST2110-21 norm.
   In RX, the conductor will be responsible of receiving the packets as fast as possible.

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE configure_conductor(const uint32_t conductor_cpu_core_os_id /*!< [in] Index of the Cpu on which the conductor will run.*/
   , HANDLE vcs_context /*!< [in] Context of the VCS session */
   , uint64_t* conductor_id /*!< [out] Id of the conductor that was created.*/
);

/*!
   @brief This function manages stream creation and configuration

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE configure_stream(HANDLE vcs_context /*!< [in] Context of the VCS session */
                               , VMIP_STREAMTYPE stream_type /*!< [in] Type of the stream (RX or TX)*/
                               , const std::vector<uint32_t>& processing_cpu_core_os_id /*!< [in]  This will be used to know on which core processing job will be launched.
                                                                                                 Putting the conductor core in this list should be avoided.
                                                                                                 If left empty, all the core of the system will be used. */
                               , const std::vector<uint32_t>& destination_addresses /*!< [in] In TX : The packets will be sent toward those addresses.
                                                                                              In RX : This corresponds to the addresses the packets are received on, can be unicast, multicast.
                                                                                              If this is set to 0, wo filtering will be done on the addresses.
                                                                                              The size of the vector indicates the number of different path that will be used. Using multiple paths allows seamless
                                                                                              switching in RX and TX as specified in the norm ST2022-7. The size must correspond to the size of destination_udp_ports and nic_ids */
                               , const std::vector<uint16_t>& destination_udp_ports /*!< [in] In TX : The packets will be sent toward those ports. The size of the table must correspond to NbPathST2022_7.
                                                                                             In RX : Those are the local ports that will be opened.
                                                                                             The size of the vector indicates the number of different path that will be used. Using multiple paths allows seamless
                                                                                             switching in RX and TX as specified in the norm ST2022-7. The size must correspond to the size of destination_addresses and nic_ids */
                               , const uint32_t destination_ssrc /*!< [in] In TX : The packets will be sent toward this ssrc.
                                                                          In RX : Ssrc of the received packet.
                                                                                  If this is set to 0, wo filtering will be done on the SSRC.    */
                               , const VMIP_VIDEO_STANDARD video_standard /*!< [in] Video standard of the stream. */
                               , const std::vector<uint64_t>& nic_ids /*!< [in] Ids of the NICs used by the stream
                                                                          The size of the vector indicates the number of different path that will be used. Using multiple paths allows seamless
                                                                          switching in RX and TX as specified in the norm ST2022-7. The size must correspond to the size of destination_addresses and destination_udp_ports */
                               , uint64_t conductor_id /*!< [in] Id of the conductor used by the stream. */
                               , uint32_t management_thread_cpu_core_os_id /*!< [in] CPU core on wich the management thread will be pinned. */
                               , HANDLE stream /*!< [out] Handle of the created stream */
);

/*!
   @brief This function manages stream creation and configuration based on a SDP

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE configure_stream_from_sdp(HANDLE vcs_context /*!< [in] Context of the VCS session */
                                      , VMIP_STREAMTYPE stream_type /*!< [in] Type of the stream (RX or TX)*/
                                      , const std::vector<uint32_t>& processing_cpu_core_os_id /*!< [in]  This will be used to know on which core processing job will be launched.
                                                                                                        Putting the conductor core in this list should be avoided.
                                                                                                        If left empty, all the core of the system will be used. */
                                      , std::string sdp /*!< [in] SDP from which informations will be extracted*/
                                      , const uint32_t destination_ip_overrides /*!< [in] To override the destination IP contained in the SDP.*/
                                      , const uint16_t destination_udp_port_ovverrides /*!< [in] To override the destination UDP port contained in the SDP.*/
                                      , const std::vector<uint64_t>& nic_ids /*!< [in] Ids of the NICs used by the stream. Multiple NICs are used in case we use multiple paths as specified in the norm ST2022-7 */
                                      , uint64_t conductor_id /*!< [in] Id of the conductor used by the stream. */
                                      , uint32_t management_thread_cpu_core_os_id /*!< [in] CPU core on wich the management thread will be pinned. */
                                      , HANDLE stream /*!< [out] Handle of the created stream */
);

/*!
   @brief Print the status of the PTP service

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE print_ptp_status(HANDLE vcs_context /*!< [in] Handle to the VCS context.*/);

/*!
   @brief Stringify the VCS version

   @returns A string containing the formated VCS version as M.m.b, where M is the MAJOR version, m is the MINOR version and b is the BUILD version.
*/
std::string version_to_string(uint64_t version /*!< [in] version that will be stringified*/);

/*!
   @brief Stringify the VCS error code

   @returns A string containing the formated VCS error code
*/
std::string to_string(VMIP_ERRORCODE error_code /*!< [in] Error code that will be stringified*/);

/*!
   @brief This function generates an SDP based on a stream configuration.

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE generate_sdp(HANDLE vcs_context,   /*!< [in] VCS context used by the stream */
                           HANDLE stream,       /*!< [in] stream used for the SDP generation */
                           std::string* sdp    /*!< [out] SDP generated*/
);

/*!
   @brief This function generates an SDP based on the stream configuration structures.

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE generate_sdp(HANDLE vcs_context,   /*!< [in] VCS context used by the stream */
                           VMIP_STREAM_NETWORK_CONFIG stream_network_config, /*!< [in] Network configuration of the stream */
                           VMIP_STREAM_ESSENCE_CONFIG essence_config, /*!< [in] Essence configuration of the stream */
                           std::string* sdp    /*!< [out] SDP generated*/
);

/*! 
   @brief This function applies the PTP parameters.

   @returns The function returns the status of its execution as VMIP_ERRORCODE

*/
VMIP_ERRORCODE apply_ptp_parameters(HANDLE vcs_context /*!< [in] Handle to the VCS context.*/,
                                    uint8_t domain_number /*!< [in] Domain number of the PTP service*/,
                                    uint8_t announce_receipt_timeout /*!< [in] Announce receipt timeout in seconds*/,
                                    uint64_t network_interface_id /*!< [in] Id of the network interface used by the PTP service*/
);

/*!
   @brief This function prints the status of the PTP service

   @returns The function returns the status of its execution as VMIP_ERRORCODE
*/
VMIP_ERRORCODE print_ptp_status(HANDLE vcs_context /*!< [in] Handle to the VCS context.*/,
                               uint8_t domain_number /*!< [in] Domain number of the PTP service*/,
                               uint8_t announce_receipt_timeout /*!< [in] Announce receipt timeout in seconds*/

);

/*!
   @brief This function monitor RX stream status
*/
void monitor_rx_stream_status(HANDLE stream, bool* request_stop, uint32_t* timeout);

/*!
   @brief This function monitor TX stream status
*/
void monitor_tx_stream_status(HANDLE stream, bool* request_stop);