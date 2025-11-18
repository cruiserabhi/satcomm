/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       StaInterfaceManager.hpp
 *
 * @brief      Primary interface for Wi-Fi Station Mode.
 *             it provide APIs for Wi-Fi Station mode configurations and management.
 *
 */

#ifndef TELUX_WLAN_STAINTERFACEMANAGER_HPP
#define TELUX_WLAN_STAINTERFACEMANAGER_HPP

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanDefines.hpp>
#include "WlanDeviceManager.hpp"

namespace telux {
namespace wlan {

//Forward declaration
class IStaListener;

/** @addtogroup telematics_wlan_station
 * @{ */

/**
 * Station Connection IP Type
 */
enum class StaIpConfig {
    DYNAMIC_IP   = 1,   /**< Station is configured with dynamic IP */
    STATIC_IP    = 2,   /**< Station is configured with Static IP  */
};

/**
 * Bridge/Router Mode
 */
enum class StaBridgeMode {
    ROUTER = 0,    /**<  Station is in Router Mode      */
    BRIDGE = 1     /**<  Station is in Bridge Mode      */
};

/**
 * Static IP Configuration
 */
struct StaStaticIpConfig {
    std::string ipAddr;       /**<   IPv4 address to be assigned. */
    std::string gwIpAddr;     /**<   IPv4 address of the gateway. */
    std::string netMask;      /**<   Subnet mask.                 */
    std::string dnsAddr;      /**<   DNS IPv4 address.            */
};

/**
 * Station Configuration
 */
struct StaConfig {
    Id                  staId;            /**< Id of station backhaul                 */
    StaIpConfig         ipConfig;         /**< IP configuration of station backhaul   */
    StaStaticIpConfig   staticIpConfig;   /**< Static IP configuration if selected    */
    StaBridgeMode       bridgeMode;       /**< Station configuration as Router/bridge */
};

/** @addtogroup telematics_wlan_station
 * @{ */

/**
 * @brief  Manager class for configuring Wlan Station Mode
 */
class IStaInterfaceManager {
 public:
    /**
     * Set Station IP Configurations: Set Station IP configuration dynamic/static and static IP
     * address if selected. If API is called when WLAN is disabled, changes will take effect when
     * WLAN is enabled using @ref telux::wlan::IWlanDeviceManager::enable API.
     * If API is called when WLAN is enabled, changes will take effect after restarting
     * wpa_supplicant by calling @ref telux::wlan::IStaInterfaceManager::manageStaService
     *
     * @param [in] staId                   Station Identifier @ref telux::wlan::Id
     * @param [in] ipConfig                Static/Dynamic IP configuration
     *                                     @ref telux::wlan::StaIpConfig.
     * @param [in] staticIpConfig          Static IP configuration, not used if station was
     *                                     configured to use dynamic IP.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode setIpConfig(Id staId, StaIpConfig ipConfig,
        StaStaticIpConfig staticIpConfig) = 0;

    /**
     * Set Station backhaul to act as router or bridge: Sets Station to act as router or bridge
     * where station internal clients get public IP addresses.
     * If API is called when WLAN is disabled, changes will take effect when WLAN is enabled using
     * @ref telux::wlan::IWlanDeviceManager::enable API.
     * If API is called when WLAN is enabled, changes will take effect after restarting
     * wpa_supplicant by calling @ref telux::wlan::IStaInterfaceManager::manageStaService
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId                   Station Identifier @ref telux::wlan::Id
     * @param [in] bridgeMode              bridgeMode @ref telux::wlan::StaBridgeMode
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode setBridgeMode(Id staId, StaBridgeMode bridgeMode) = 0;

    /**
     * Enable Hotspot 2.0 Support
     *
     * @param [in] staId                   Station Identifier @ref telux::wlan::Id
     * @param [in] enable                  True: enable Hotspot support, False disable support
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode enableHotspot2(Id staId, bool enable) = 0;

    /**
     * Request current station configurations: Returns configurations set by
     * @ref telux::wlan::IStaInterfaceManager::setIpConfig and
     * @ref telux::wlan::IStaInterfaceManager::setBridgeMode
     *
     * @param [in] config         Station configurations @ref telux::wlan::StaConfig
      *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getConfig(std::vector<StaConfig>& config) = 0;

    /**
     * Request current station status: Returns current Sta interface status such as network
     * interface name and IP address.
     *
     * @param [in] status         Station Status @ref telux::wlan::StaStatus
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getStatus(std::vector<StaStatus>& status) = 0;

    /**
     * Execute an operation on wpa_supplicant service. Provides ability for client to either
     * stop/start or restart wpa_supplicant service for selected station.
     * Restarting wpa_supplicant service is required for any changes made to wpa_supplicant.conf
     * file to take effect.
     * Station selected to execute operation on, will temporarily go out of service when this
     * API is called.
     * This API should be called only when station mode is configured through
     * @ref telux::wlan::IDeviceManager::setMode
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId         Station identifier to execute operation on. @ref telux::wlan::Id
     * @param [in] opr           Operation to be performed on wpa_supplicant
     *                           @ref telux::wlan::ServiceOperation
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode manageStaService(Id staId, ServiceOperation opr) = 0;

    /**
     * Register as a listener for specific events defined in telux::wlan::IStaListener
     *
     * @param [in] listener    pointer of IStaListener object that processes the
     * notification
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<IStaListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IStaListener object that needs to be removed
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode deregisterListener(std::weak_ptr<IStaListener> listener) = 0;

    virtual ~IStaInterfaceManager(){};
};

class IStaListener : public telux::common::ISDKListener {
public:
    /**
     * This function is called when Station Status Changes
     *
     * @param [in] status     List of station state @ref telux::wlan::StaStatus
     */
    virtual void onStationStatusChanged(std::vector<StaStatus> staStatus) {}

    /**
     * This function is called when Station switch to different operation band
     *
     * @param [in] radio        New Station operation band @ref telux::wlan::BandType
     */
    virtual void onStationBandChanged(BandType radio) {}

    virtual ~IStaListener() {}
};

/** @} */ /* end_addtogroup telematics_wlan_station */
}
}
#endif // TELUX_WLAN_STAINTERFACEMANAGER_HPP
