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

#ifndef WLANTESTAPP_HPP
#define WLANTESTAPP_HPP

#include <iostream>
#include <memory>

#include <telux/common/Version.hpp>
#include "WlanDeviceManagerMenu.hpp"
#include "WlanApInterfaceManagerMenu.hpp"
#include "WlanStaInterfaceManagerMenu.hpp"

#include "console_app_framework/ConsoleApp.hpp"

class WlanTestApp : public ConsoleApp {
 public:
    /**
     * Initialize commands and SDK
     */
    bool init();

    WlanTestApp(std::string appName, std::string cursor);
    ~WlanTestApp();

    void wlanDeviceManagerMenu(std::vector<std::string> inputCommand);
    void wlanApInterfaceManagerMenu(std::vector<std::string> inputCommand);
    void wlanStaInterfaceManagerMenu(std::vector<std::string> inputCommand);

 private:
    bool initWlan();
    std::shared_ptr<WlanDeviceManagerMenu> wlanDeviceManagerMenu_ = nullptr;
    std::shared_ptr<WlanApInterfaceManagerMenu> wlanApInterfaceManagerMenu_ = nullptr;
    std::shared_ptr<WlanStaInterfaceManagerMenu> wlanStaInterfaceManagerMenu_ = nullptr;
};
#endif
