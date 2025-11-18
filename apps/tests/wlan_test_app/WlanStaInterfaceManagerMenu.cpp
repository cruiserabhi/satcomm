/*
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "WlanStaInterfaceManagerMenu.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanStaInterfaceManagerMenu::WlanStaInterfaceManagerMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
       menuOptionsAdded_ = false;
}

WlanStaInterfaceManagerMenu::~WlanStaInterfaceManagerMenu() {
    if (wlanStaInterfaceManager_) {
        wlanStaInterfaceManager_ = nullptr;
    }
}

bool WlanStaInterfaceManagerMenu::init() {
    if (wlanStaInterfaceManager_ == nullptr) {
        auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
        wlanStaInterfaceManager_ = wlanFactory.getStaInterfaceManager();
        if (wlanStaInterfaceManager_ == nullptr) {
            //Return immediately
            std::cout <<
                "\nError encountered in initializing Wlan Station Interface Manager" << std::endl;
            return false;
        }
        wlanStaInterfaceManager_->registerListener(shared_from_this());
    }
    return true;
}

void WlanStaInterfaceManagerMenu::showMenu() {
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
		  int stepID = 1;
        std::shared_ptr<ConsoleAppCommand> setIpConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_ip_config", {},
            std::bind(&WlanStaInterfaceManagerMenu::setIpConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setBridgeMode
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_bridge_mode", {},
            std::bind(&WlanStaInterfaceManagerMenu::setBridgeMode, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> enableHotspot2
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "enable_hotspot2_support", {},
            std::bind(&WlanStaInterfaceManagerMenu::enableHotspot2, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_config", {},
            std::bind(&WlanStaInterfaceManagerMenu::getConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_status", {},
            std::bind(&WlanStaInterfaceManagerMenu::getStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> manageStaService
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "manage_service", {},
            std::bind(&WlanStaInterfaceManagerMenu::manageStaService, this, std::placeholders::_1)));
        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            setIpConfig, setBridgeMode, enableHotspot2, getConfig, getStatus, manageStaService};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
}

void WlanStaInterfaceManagerMenu::setIpConfig(std::vector<std::string> userInput) {

    int staId = 1, ipConfig = 1;
    telux::wlan::StaIpConfig staIpConfig = {};
    telux::wlan::StaStaticIpConfig staticIpConfig;

    std::cout << "Set Station IP Configuration" << std::endl;

    std::cout << "Select Station IP Type (1-Dynamic IP, 2-Static IP): ";
    std::cin >> ipConfig;
    WlanUtils::validateInput(ipConfig, {1, 2});

    if(ipConfig == 2){
        staIpConfig = telux::wlan::StaIpConfig::STATIC_IP;
        std::string userInput{};
        std::cout << "Enter IPv4 Address: ";
        std::cin >> userInput;
        Utils::validateInput(userInput);
        staticIpConfig.ipAddr = userInput;
        std::cout << std::endl;

        std::cout << "Enter Gateway IPv4 Address: ";
        std::cin >> userInput;
        Utils::validateInput(userInput);
        staticIpConfig.gwIpAddr = userInput;
        std::cout << std::endl;

        std::cout << "Enter Subnet Mask: ";
        std::cin >> userInput;
        Utils::validateInput(userInput);
        staticIpConfig.netMask = userInput;
        std::cout << std::endl;

        std::cout << "Enter DNS IPv4 Address: ";
        std::cin >> userInput;
        Utils::validateInput(userInput);
        staticIpConfig.dnsAddr = userInput;
        std::cout << std::endl;
    }else{
        staIpConfig = telux::wlan::StaIpConfig::DYNAMIC_IP;
    }
    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->setIpConfig(
        static_cast<telux::wlan::Id>(staId), staIpConfig, staticIpConfig);

    std::cout << "\nSet Station IP Configuration Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanStaInterfaceManagerMenu::setBridgeMode(std::vector<std::string> userInput) {

    std::cout << "Set Station Bridge Mode" << std::endl;

    int staId = 1, bridgeMode = 0;
    std::shared_ptr<telux::wlan::StaStaticIpConfig> staticIpConfig;

    std::cout << "Enter Bridge Mode (0-Router Mode, 1-Bridge Mode): ";
    std::cin >> bridgeMode;
    WlanUtils::validateInput(bridgeMode, {0, 1});

    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->setBridgeMode(
        static_cast<telux::wlan::Id>(staId), static_cast<telux::wlan::StaBridgeMode>(bridgeMode));

    std::cout << "\nSet Bridge Mode Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanStaInterfaceManagerMenu::enableHotspot2(std::vector<std::string> userInput) {

    std::cout << "Enable Support For Hotspot 2.0" << std::endl;

    int hotspotEnable;

    std::cout << "Enable/Disable Hotspot 2.0 Support (1-enable, 0-disable): ";
    std::cin >> hotspotEnable;
    std::cout << std::endl;
    WlanUtils::validateInput(hotspotEnable, {0, 1});

    telux::common::ErrorCode retCode =
        wlanStaInterfaceManager_->enableHotspot2(
            telux::wlan::Id::PRIMARY, static_cast<bool>(hotspotEnable));
    std::cout << "\nEnable Hotspot2 Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}


void WlanStaInterfaceManagerMenu::getConfig(std::vector<std::string> userInput) {
    std::vector<telux::wlan::StaConfig> config;

    std::cout << "Request Station Configuration" << std::endl;
    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->getConfig(config);

    std::cout << "\nrequest Station Configuration Response"
                << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(retCode)
                << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        for (auto &cfg : config) {
            std::cout << "------------------------------------------" << std::endl;
            std::cout << "Id         : " << WlanUtils::getWlanId(cfg.staId) << std::endl;
            std::cout << "IP config  : "
                      << ((cfg.ipConfig == telux::wlan::StaIpConfig::DYNAMIC_IP)?
                            "DYNAMIC":"STATIC") << std::endl;
            if(cfg.ipConfig == telux::wlan::StaIpConfig::STATIC_IP) {
                std::cout << "IPv4 Addr        : " << cfg.staticIpConfig.ipAddr << std::endl;
                std::cout << "Gateway IPv4 Addr: " << cfg.staticIpConfig.gwIpAddr << std::endl;
                std::cout << "Subnet Mask      : " << cfg.staticIpConfig.netMask << std::endl;
                std::cout << "DNS IPv4 Addr    : " << cfg.staticIpConfig.dnsAddr << std::endl;
            }
            std::cout << "Bridge Mode: "
                        << ((cfg.bridgeMode == telux::wlan::StaBridgeMode::BRIDGE)?
                            "Bridge":"Router") << std::endl;
        }
    }
}

void WlanStaInterfaceManagerMenu::getStatus(std::vector<std::string> userInput) {
    std::vector<telux::wlan::StaStatus> status;
    std::cout << "Request Station Status" << std::endl;

    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->getStatus(status);
    std::cout << "\nRequest Station Status Response"
                << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(retCode)
                << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        WlanUtils::printStaStatus(status);
    }
}

void WlanStaInterfaceManagerMenu::manageStaService(std::vector<std::string> userInput) {

    std::cout << "\nManage Station Service" << std::endl;
    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->manageStaService(
        telux::wlan::Id::PRIMARY, telux::wlan::ServiceOperation::RESTART);

    std::cout << "Manage Station Service Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanStaInterfaceManagerMenu::onStationStatusChanged(
    std::vector<telux::wlan::StaStatus> status) {
    PRINT_NOTIFICATION << " ** Wlan onStationStatusChange **\n";
    WlanUtils::printStaStatus(status);
}

void WlanStaInterfaceManagerMenu::onStationBandChanged(telux::wlan::BandType radio) {
   PRINT_NOTIFICATION << " ** Wlan onStationOperationBandChanged **\n";

   if(radio == telux::wlan::BandType::BAND_2GHZ) {
       std::cout << "Station has switched to 2G band" << std::endl;
   } else if(radio == telux::wlan::BandType::BAND_5GHZ) {
       std::cout << "Station has switched to 5G band" << std::endl;
   } else {
       std::cout << "Station has switched to 6G band" << std::endl;
   }
}

