/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/**
 * Data Connection Manager Sample Application using Telematics SDK
 * This is used to demonstrate data connection manager APIs like start/stop data
 * calls and profile management
 */

#ifndef DATAMENU_HPP
#define DATAMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>

#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataProfile.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/tel/PhoneManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"

#include "DataListener.hpp"
#include "DataResponseCallback.hpp"
#include "MyProfileListener.hpp"
#include "data_connection/DataConnectionMenu.hpp"
#include "data_profile/DataProfileMenu.hpp"
#include "serving_system/ServingSystemMenu.hpp"
#include "bridge/BridgeMenu.hpp"
#include "l2tp/L2tpMenu.hpp"
#include "snat/SnatMenu.hpp"
#include "firewall/FirewallMenu.hpp"
#include "vlan/VlanMenu.hpp"
#include "socks/SocksMenu.hpp"
#include "data_filter/DataFilterMenu.hpp"
#include "data_settings/DataSettingsMenu.hpp"
#include "client/ClientMenu.hpp"
#include "dual_data/DualDataManagementMenu.hpp"
#include "data_control/DataControlMenu.hpp"
#include "qos/QoSManagementMenu.hpp"
#include "data_link/DataLinkMenu.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>

#include "ConfigParser.hpp"

using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;

class DataMenu : public IDataFilterListener, public ConsoleApp {
 public:
    bool initializeSDK();

    // initialize menu and sdk
    void init();
    void openDataFilterMenu(std::vector<std::string> userInput);

    ResponseCallback responseCb;
    void commandCallback(ErrorCode errorCode);

    DataMenu(std::string appName, std::string cursor);
    ~DataMenu();

    void dataConnectionMenu(std::vector<std::string> inputCommand);
    void dataProfileMenu(std::vector<std::string> inputCommand);
    void servingSystemMenu(std::vector<std::string> inputCommand);
    void snatMenu(std::vector<std::string> inputCommand);
    void firewallMenu(std::vector<std::string> inputCommand);
    void vlanMenu(std::vector<std::string> inputCommand);
    void socksMenu(std::vector<std::string> inputCommand);
    void bridgeMenu(std::vector<std::string> inputCommand);
    void l2tpMenu(std::vector<std::string> inputCommand);
    void dataSettingsMenu(std::vector<std::string> inputCommand);
    void clientMenu(std::vector<std::string> inputCommand);
    void dualDataManagementMenu(std::vector<std::string> inputCommand);
    void dataControlMenu(std::vector<std::string> inputCommand);
    void qosManagementMenu(std::vector<std::string> inputCommand);
    void dataLinkMenu(std::vector<std::string> inputCommand);
 private:
    std::shared_ptr<telux::tel::IPhoneManager> phoneManager_;

    std::shared_ptr<DataConnectionMenu> dataConnectionMenu_;
    std::shared_ptr<DataProfileMenu> dataProfileMenu_;
    std::shared_ptr<DataServingSystemMenu> dataServingSystemMenu_;
    std::shared_ptr<BridgeMenu> bridgeMenu_;
    std::shared_ptr<L2tpMenu> l2tpMenu_;
    std::shared_ptr<SnatMenu> snatMenu_;
    std::shared_ptr<FirewallMenu> firewallMenu_;
    std::shared_ptr<VlanMenu> vlanMenu_;
    std::shared_ptr<SocksMenu> socksMenu_;
    std::shared_ptr<DataFilterMenu> dataFilterMenu_;
    std::shared_ptr<DataSettingsMenu> settingsMenu_;
    std::shared_ptr<ClientMenu> clientMenu_;
    std::shared_ptr<DualDataManagementMenu> dualDataManagementMenu_;
    std::shared_ptr<DataControlMenu> dataControlMenu_;
    std::shared_ptr<QoSManagementMenu> qosManagementMenu_;
    std::shared_ptr<DataLinkMenu> dataLinkMenu_;
};
#endif
