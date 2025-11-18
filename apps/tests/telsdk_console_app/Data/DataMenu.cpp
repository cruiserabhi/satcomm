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

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include <telux/common/DeviceConfig.hpp>

#include "../../common/utils/Utils.hpp"

#include "DataUtils.hpp"
#include "DataMenu.hpp"
#include "DataResponseCallback.hpp"

using namespace std;
using namespace telux::data::net;

DataMenu::DataMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DataMenu::~DataMenu() {
}

bool DataMenu::initializeSDK() {
    std::chrono::time_point<std::chrono::steady_clock> startTime, endTime;
    startTime = std::chrono::steady_clock::now();

    // Instantiate Data Connection Managers
    dataConnectionMenu_ = std::make_shared<DataConnectionMenu>("Data Connection Manager Menu",
        "data_connection> ");
    bool dcmSubSystemStatus = dataConnectionMenu_->init();

    // Instantiate Data Profile Managers
    dataProfileMenu_ = std::make_shared<DataProfileMenu>("Data Profile Manager Menu",
        "data_profile> ");
    bool dpmSubSystemStatus = dataProfileMenu_->init();

    // Instantiate Dual Data Manager
    dualDataManagementMenu_ = std::make_shared<DualDataManagementMenu>(
        "Dual Data Manager Menu", "dual_data> ");
    bool dualDataSubSystemStatus = dualDataManagementMenu_->init();

    // Check if the SDK is able to initialize data subsystems
    if ((dcmSubSystemStatus) && (dpmSubSystemStatus) && (dualDataSubSystemStatus)) {
        endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                  << std::endl;
    } else {
        std::cout << "Unable to initialize all subSystems" << std::endl;
    }
    return true;
}

void DataMenu::init() {
    std::shared_ptr<ConsoleAppCommand> dataConnectionMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "1", "Data_Connection_Management_Menu",
            {}, std::bind(&DataMenu::dataConnectionMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> dataFilterMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Data_Filter",
            {}, std::bind(&DataMenu::openDataFilterMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> snatMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Snat_Menu",
            {}, std::bind(&DataMenu::snatMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> firewallMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Firewall_Menu",
            {}, std::bind(&DataMenu::firewallMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> vlanMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Vlan_Menu",
            {}, std::bind(&DataMenu::vlanMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> bridgeMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Bridge_Menu",
            {}, std::bind(&DataMenu::bridgeMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> socksMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Socks_Menu",
            {}, std::bind(&DataMenu::socksMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> l2tpMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "L2tp_Menu",
            {}, std::bind(&DataMenu::l2tpMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> servingSystemMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Serving_System_Menu",
            {}, std::bind(&DataMenu::servingSystemMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> dataProfileManagerMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "10", "Data_Profile_Management_Menu",
            {}, std::bind(&DataMenu::dataProfileMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> dataSettingsMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "11", "Data_Settings_Menu",
            {}, std::bind(&DataMenu::dataSettingsMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> clientManagerMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "12", "Client_Menu",
            {}, std::bind(&DataMenu::clientMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> dualDataManagementMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "13", "Dual_Data_Management_Menu",
            {}, std::bind(&DataMenu::dualDataManagementMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> dataControlMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "14", "Data_Control_Menu",
            {}, std::bind(&DataMenu::dataControlMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> qosManagementMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "15", "QoS_Management_Menu",
            {}, std::bind(&DataMenu::qosManagementMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> dataLinkMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "16", "Data_Link_Menu",
            {}, std::bind(&DataMenu::dataLinkMenu, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {dataConnectionMenu,
        dataFilterMenu, snatMenuCommand, firewallMenuCommand, vlanMenuCommand, bridgeMenuCommand,
        socksMenuCommand, l2tpMenuCommand, servingSystemMenuCommand, dataProfileManagerMenuCommand,
        dataSettingsMenuCommand, clientManagerMenuCommand, dualDataManagementMenuCommand,
        dataControlMenuCommand, qosManagementMenuCommand, dataLinkMenuCommand};

    addCommands(commandsList);

    if (DataMenu::initializeSDK()) {
        ConsoleApp::displayMenu();
    }
}

void DataMenu::dataConnectionMenu(std::vector<std::string> userInput) {
    if(dataConnectionMenu_) {
        if (dataConnectionMenu_->displayMenu()) {
            dataConnectionMenu_->mainLoop();
        }
    }
    else {
        std::cout << "Error Creating Data Connection Manager" << std::endl;
    }
    ConsoleApp::displayMenu();
}

void DataMenu::openDataFilterMenu(std::vector<std::string> userInput)
{
    dataFilterMenu_ = std::make_shared<DataFilterMenu>("Data Filter Menu", "data_filter> ");

    if (dataFilterMenu_->init())
    {
        dataFilterMenu_->mainLoop();
    }
    dataFilterMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::commandCallback(ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Command initiated successfully " << std::endl;
    } else {
        std::cout << " Command failed." << std::endl;
    }
}

void DataMenu::dataProfileMenu(std::vector<std::string> userInput) {
    if(dataProfileMenu_) {
        if (dataProfileMenu_->displayMenu()) {
            dataProfileMenu_->mainLoop();
        }
    }
    else {
        std::cout << "Error Creating Data Profile Manager" << std::endl;
    }
    ConsoleApp::displayMenu();
}

void DataMenu::bridgeMenu(std::vector<std::string> userInput) {
    bridgeMenu_ =  std::make_shared<BridgeMenu>("Software Bridge Menu", "bridge> ");
    if (bridgeMenu_->init()) {
        bridgeMenu_->mainLoop();
    }
    bridgeMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::l2tpMenu(std::vector<std::string> userInput) {
    l2tpMenu_ = std::make_shared<L2tpMenu>("L2TP Menu", "l2tp> ");
    if(l2tpMenu_->init()) {
        l2tpMenu_->mainLoop();
    }
    l2tpMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::dataSettingsMenu(std::vector<std::string> userInput) {
    settingsMenu_ = std::make_shared<DataSettingsMenu>("Data Settings Menu", "settings> ");
    if(settingsMenu_->init()) {
        settingsMenu_->mainLoop();
    }
    settingsMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::dataLinkMenu(std::vector<std::string> userInput) {
    dataLinkMenu_ = std::make_shared<DataLinkMenu>("Data Link Menu", "link> ");
    if(dataLinkMenu_->init()) {
        dataLinkMenu_->mainLoop();
    }
    dataLinkMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::snatMenu(std::vector<std::string> userInput) {
    snatMenu_ = std::make_shared<SnatMenu>("SNAT Menu", "snat> ");
    if(snatMenu_->init()) {
        snatMenu_->mainLoop();
    }
    snatMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::firewallMenu(std::vector<std::string> userInput) {
    firewallMenu_ = std::make_shared<FirewallMenu>("Firewall Menu", "firewall> ");
    if(firewallMenu_->init()) {
        firewallMenu_->mainLoop();
    }
    firewallMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::vlanMenu(std::vector<std::string> userInput) {
    vlanMenu_ = std::make_shared<VlanMenu>("Vlan Menu", "vlan> ");
    if(vlanMenu_->init()) {
        vlanMenu_->mainLoop();
    }
    vlanMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::socksMenu(std::vector<std::string> userInput) {
    socksMenu_ = make_shared<SocksMenu>("Socks Menu", "socks> ");
    if(socksMenu_->init()) {
        socksMenu_->mainLoop();
    }
    socksMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::servingSystemMenu(std::vector<std::string> userInput) {
    dataServingSystemMenu_ =
        make_shared<DataServingSystemMenu>("Serving System Menu", "serving_system> ");
    if(dataServingSystemMenu_->init()) {
        dataServingSystemMenu_->mainLoop();
    }
    dataServingSystemMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::clientMenu(std::vector<std::string> userInput) {
    clientMenu_ = std::make_shared<ClientMenu>("Client Menu", "client> ");
    if(clientMenu_->init()) {
        clientMenu_->mainLoop();
    }
    clientMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::dualDataManagementMenu(std::vector<std::string> userInput) {
    if(dualDataManagementMenu_) {
        if (dualDataManagementMenu_->displayMenu()) {
            dualDataManagementMenu_->mainLoop();
        }
    }
    else {
        std::cout << "Error in creating dual data manager menu" << std::endl;
    }
    ConsoleApp::displayMenu();
}

void DataMenu::dataControlMenu(std::vector<std::string> userInput) {
    dataControlMenu_ =
        make_shared<DataControlMenu>("Data Control Menu", "data_control> ");
    if(dataControlMenu_->init()) {
        dataControlMenu_->mainLoop();
    }
    dataControlMenu_ = nullptr;
    ConsoleApp::displayMenu();
}

void DataMenu::qosManagementMenu(std::vector<std::string> userInput) {
    qosManagementMenu_ =
        make_shared<QoSManagementMenu>("QoS Management Menu", "QoS> ");
    if(qosManagementMenu_->init()) {
        qosManagementMenu_->mainLoop();
    }
    qosManagementMenu_ = nullptr;
    ConsoleApp::displayMenu();
}
