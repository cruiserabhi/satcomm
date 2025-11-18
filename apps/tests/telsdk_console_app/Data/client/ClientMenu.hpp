/*
 *  Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * This is a Client Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to set create, remove, bind, unbind, and query existing Clients
 */

#ifndef CLIENTMENU_HPP
#define CLIENTMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>


#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/ClientManager.hpp>

#include "ClientListener.hpp"

using namespace telux::data;
using namespace telux::common;

class ClientMenu : public ConsoleApp,
                 public IClientListener,
                 public std::enable_shared_from_this<ClientMenu> {
public:
    // initialize menu and sdk
    bool init();

    // Initialization Callback
    void onInitComplete(telux::common::ServiceStatus status);

    // Client Manager APIs
    void getDeviceDataUsageStats(std::vector<std::string> inputCommand);
    void resetDataUsageStats(std::vector<std::string> inputCommand);
    ClientMenu(std::string appName, std::string cursor);
    ~ClientMenu();
private:
    bool initClientManager();

    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<telux::data::IClientManager> clientManager_;
    std::shared_ptr<ClientListener> clientListener_;
};
#endif
