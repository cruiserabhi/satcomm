/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef WLANDEVICEMANAGERMENU_HPP
#define WLANDEVICEMANAGERMENU_HPP

#include <iostream>
#include <memory>

#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>
#include <condition_variable>

#include "console_app_framework/ConsoleApp.hpp"
#include "WlanUtils.hpp"
#include "../../common/utils/Utils.hpp"

class WlanDeviceManagerMenu : public ConsoleApp ,
                              public telux::wlan::IWlanListener,
                              public std::enable_shared_from_this<WlanDeviceManagerMenu> {

 public:
    WlanDeviceManagerMenu(std::string appName, std::string cursor);
    ~WlanDeviceManagerMenu();

    bool isSubSystemReady();
    bool init();

    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    void enableWlan(std::vector<std::string> userInput);
    void setMode(std::vector<std::string> userInput);
    void getConfig(std::vector<std::string> userInput);
    void getStatus(std::vector<std::string> userInput);
    void setActiveCountry(std::vector<std::string> userInput);
    void getRegulatoryParams(std::vector<std::string> userInput);
    void setTxPower(std::vector<std::string> userInput);
    void getTxPower(std::vector<std::string> userInput);

    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onEnableChanged(bool enable) override;
    void onTempCrossed(float temperature, telux::wlan::DevicePerfState perfState) override;
 private:
    bool menuOptionsAdded_;
    std::shared_ptr<telux::wlan::IWlanDeviceManager> wlanDeviceManager_ = nullptr;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
#endif
