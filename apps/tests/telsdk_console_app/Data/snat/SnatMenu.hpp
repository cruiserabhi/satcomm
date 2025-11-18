/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a Snat Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to add, remove and request Snat entries.
 */

#ifndef SNATMENU_HPP
#define SNATMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>


#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/NatManager.hpp>


using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;

class SnatMenu : public ConsoleApp ,
                 public INatListener,
                 public std::enable_shared_from_this<SnatMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // Snat Manager APIs
    void addStaticNatEntry(std::vector<std::string> inputCommand);
    void removeStaticNatEntry(std::vector<std::string> inputCommand);
    void requestStaticNatEntries(std::vector<std::string> inputCommand);

    void addStaticNatEntry_V1(std::vector<std::string> inputCommand);
    void removeStaticNatEntry_V1(std::vector<std::string> inputCommand);
    void requestStaticNatEntries_V1(std::vector<std::string> inputCommand);

    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    SnatMenu(std::string appName, std::string cursor);
    ~SnatMenu();
 private:
    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::shared_ptr<telux::data::net::INatManager> snatManager_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
#endif
