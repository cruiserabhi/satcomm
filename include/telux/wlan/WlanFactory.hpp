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
 * @file       WlanFactory.hpp
 *
 * @brief      WlanFactory is the central factory to create all wlan managers instances such as
 *             WlanDeviceManager, WlanApInterfaceManager, and WlanStaInterfaceManager
 *
 */

#ifndef TELUX_WLAN_WLANFACTORY_HPP
#define TELUX_WLAN_WLANFACTORY_HPP

#include <map>
#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/wlan/WlanDeviceManager.hpp>
#include <telux/wlan/ApInterfaceManager.hpp>
#include <telux/wlan/StaInterfaceManager.hpp>

namespace telux {
namespace wlan {

/** @addtogroup telematics_wlan
 * @{ */

/**
 *@brief WlanFactory is the central factory to create all wlan classes
 *
 */
class WlanFactory {
 public:
    /**
     * Get Wlan Factory instance.
     */
    static WlanFactory &getInstance();

    /**
     * Get Wlan Device Manager
     *
     * @param [in] clientCallback       Optional callback to get the initialization status of
     *                                  WlanDeviceManager @ref telux::common::InitResponseCb
     * @returns instance of IWlanDeviceManager
     *
     */
    virtual std::shared_ptr<IWlanDeviceManager> getWlanDeviceManager(
       telux::common::InitResponseCb clientCallback = nullptr) = 0;

    /**
     * Get Access Point Interface Manager
     *
     * @returns instance of IApInterfaceManager
     *
     */
    virtual std::shared_ptr<IApInterfaceManager> getApInterfaceManager() = 0;

    /**
     * Get Station Interface Manager
     *
     * @returns instance of IStaInterfaceManager
     *
     */
    virtual std::shared_ptr<IStaInterfaceManager> getStaInterfaceManager() = 0;

 protected:
    WlanFactory();
    virtual ~WlanFactory();

 private:
    WlanFactory(const WlanFactory &) = delete;
    WlanFactory &operator=(const WlanFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_wlan */
}
}

#endif // TELUX_WLAN_WLANFACTORY_HPP
