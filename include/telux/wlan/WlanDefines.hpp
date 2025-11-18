/*
 * Copyright (c) 2022-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       WlanDefines.hpp
 * @brief      WlanDefines contains enumerations and variables used for wlan services
 *
 */

#ifndef TELUX_WLAN_WLANDEFINES_HPP
#define TELUX_WLAN_WLANDEFINES_HPP

#include <string>
#include <vector>
#include <bitset>

namespace telux {
namespace wlan {

/** @addtogroup telematics_wlan
 * @{ */

/**
 * Radio Band Types:
 */
enum class BandType {
    BAND_5GHZ   = 1,
    BAND_2GHZ   = 2,
    BAND_6GHZ   = 3,
};

/**
 * Connection Status
 */
enum class ConnectionStatus {
    UNKNOWN              = 0,           /**< Device connection is unknown    */
    CONNECTED            = 1,           /**< Device is connected             */
    DISCONNECTED         = 2,           /**< Device is disconnected          */
};

/**
 * Identifiers for Ap, Sta, P2p
 */
enum class Id {
    PRIMARY     = 1,
    SECONDARY   = 2,
    TERTIARY    = 3,
    QUATERNARY  = 4,
};

/**
 * AP Types:
 */
enum class ApType {
    UNKNOWN      = 0,
    PRIVATE      = 1,
    GUEST        = 2,
};

/**
 * Station Interface Status
 */
enum class StaInterfaceStatus {
    UNKNOWN              = 0,           /**< Station interface is unknown                  */
    CONNECTING           = 1,           /**< Station interface is connecting               */
    CONNECTED            = 2,           /**< Station interface is connected                */
    DISCONNECTED         = 3,           /**< Station interface is disconnected             */
    ASSOCIATION_FAILED   = 4,           /**< Station is unable to associate with AP        */
    IP_ASSIGNMENT_FAILED = 5,           /**< Station in unable to get IP address via DHCP  */
};

/**
 * AP Info - captures ap type (private/guest)
 */
struct ApInfo {
    BandType        apRadio;            /**< Radio type (2.4/5.0/6.0 GHz) */
    ApType          apType;             /**< Ap type (private/guest) */
};

/**
 * Ap Network Info
 */
struct ApNetInfo {
    ApInfo          info;               /**< Ap information (AP type)              */
    std::string     ssid;               /**< SSID associated with this network     */
};

/**
 * AP Status for enabled Networks
 */
struct ApStatus {
    Id              id;              /**< AP id                                 */
    std::string     name;            /**< AP network interface name             */
    std::string     ipv4Address;     /**< Local AP IP V4 address                */
    std::string     macAddress;      /**< AP MAC address                        */
    std::vector<ApNetInfo> network;  /**< Settings for AP info                  */
};

/**
 * Station Status
 */
struct StaStatus {
    Id                  id;              /**< Station Id                       */
    std::string         name;            /**< Network interface name           */
    std::string         ipv4Address;     /**< Public IP V4 address             */
    std::string         ipv6Address;     /**< Public IP V6 address             */
    std::string         macAddress;      /**< MAC address                      */
    StaInterfaceStatus  status;          /**< Interface status                 */
};

/**
 * This applies in architectures where the modem is attached to an External Application
 * Processor(EAP). An API that sets or configure Wlan can be invoked from the EAP or from
 * the modems Internal Application Processor (IAP). This type  specifies where the operation
 * should be carried out.
 */
enum class OperationType {
    WLAN_LOCAL = 0, /**< Perform the operation on the processor where the API is invoked.*/
    WLAN_REMOTE,    /**< Perform the operation on the application processor other than where
                           the API is invoked. */
};

/**
 * Preferred IP family for the connection
 */
enum class IpFamilyType {
    UNKNOWN = -1,
    IPV4 = 0x04,   /**< IPv4 data connection */
    IPV6 = 0x06,   /**< IPv6 data connection */
    IPV4V6 = 0x0A, /**< IPv4 and IPv6 data connection */
};

/**
 * Service operations to be performed
 */
enum class ServiceOperation {
    STOP      = 0x00,      /**<  Stop service       */
    START     = 0x01,      /**<  Start service      */
    RESTART   = 0x02,      /**<  Restart service    */
};

/** @} */ /* end_addtogroup telematics_wlan */
}
}

#endif // TELUX_WLAN_WLANDEFINES_HPP
