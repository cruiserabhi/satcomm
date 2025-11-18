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

#include "WlanApInterfaceManagerMenu.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanApInterfaceManagerMenu::WlanApInterfaceManagerMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
       menuOptionsAdded_ = false;
}

WlanApInterfaceManagerMenu::~WlanApInterfaceManagerMenu() {
    if (wlanApInterfaceManager_) {
        wlanApInterfaceManager_ = nullptr;
    }
}

bool WlanApInterfaceManagerMenu::init() {
    if (wlanApInterfaceManager_ == nullptr) {
        auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
        wlanApInterfaceManager_ = wlanFactory.getApInterfaceManager();
        if (wlanApInterfaceManager_ == nullptr) {
            //Return immediately
            std::cout <<
                "\nError encountered in initializing Wlan Ap Interface Manager" << std::endl;
            return false;
        }
    }
    wlanApInterfaceManager_->registerListener(shared_from_this());
    return true;
}

void WlanApInterfaceManagerMenu::showMenu() {
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        int stepID = 1;
        std::shared_ptr<ConsoleAppCommand> setConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_config", {}, std::bind(&WlanApInterfaceManagerMenu::setConfig, this,
            std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setSecurityConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_security_config", {}, std::bind(&WlanApInterfaceManagerMenu::setSecurityConfig,
            this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setSsid
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_ssid", {}, std::bind(&WlanApInterfaceManagerMenu::setSsid, this,
            std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setVisibility
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_visibility", {}, std::bind(&WlanApInterfaceManagerMenu::setVisibility, this,
            std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> configureElementInfo
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "configure_elementInfo", {}, std::bind(&WlanApInterfaceManagerMenu::configureElementInfo,
            this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setPassPhrase
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_passphrase", {}, std::bind(&WlanApInterfaceManagerMenu::setPassPhrase,
            this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_config", {},
            std::bind(&WlanApInterfaceManagerMenu::getConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_status", {},
            std::bind(&WlanApInterfaceManagerMenu::getStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getConnectedDevices
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_connected_devices", {},
            std::bind(&WlanApInterfaceManagerMenu::getConnectedDevices,
            this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> manageApService
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "manage_service", {},
            std::bind(&WlanApInterfaceManagerMenu::manageApService,
            this, std::placeholders::_1)));
        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            setConfig, setSecurityConfig, setSsid, setVisibility, configureElementInfo, setPassPhrase,
            getConfig, getStatus, getConnectedDevices, manageApService};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
}

void WlanApInterfaceManagerMenu::setConfig(std::vector<std::string> userInput) {

    std::cout << "Set AP Configuration \n";
    telux::wlan::ApConfig config;
    int apId = 1;
    std::cout << "Enter Wlan Ap Id \
            (1-PRIMARY, 2-SECONDARY, 3-TERTIARY): ";
    std::cin >> apId;
    WlanUtils::validateInput(apId, {1, 2, 3});
    std::cout << std::endl;
    config.id = WlanUtils::convertIntToWlanId(apId);
    int venue = 0;
    std::cout << "Enter Venue Type: ";
    std::cin >> venue;
    std::cout << std::endl;
    config.venue.type = venue;
    venue = 0;
    std::cout << "Enter Venue Group: ";
    std::cin >> venue;
    std::cout << std::endl;
    config.venue.group = venue;

    int userResp = 0;
    std::cout << "Enter Ap Band type \
                    (1-2.4GHz, 2-5 GHz, 3-6GHz): ";
    std::cin >> userResp;
    WlanUtils::validateInput(userResp, {1, 2, 3});
    std::cout << std::endl;
    telux::wlan::ApNetConfig apNetConfig = {};
    if(userResp == 1) {
        apNetConfig.info.apRadio = telux::wlan::BandType::BAND_2GHZ;
    } else if (userResp == 2) {
        std::cout << "Ap configured for 5 GHz band" << std::endl;
        apNetConfig.info.apRadio = telux::wlan::BandType::BAND_5GHZ;
    } else {
        std::cout << "Ap configured for 6 GHz band" << std::endl;
        apNetConfig.info.apRadio = telux::wlan::BandType::BAND_6GHZ;
    }
    populateApConfigNet(apNetConfig);
    config.network.push_back(apNetConfig);

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->setConfig(config);
    std::cout << "\nSetting AP Configuration Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanApInterfaceManagerMenu::populateApConfigNet(telux::wlan::ApNetConfig& netConfig) {
    int apType {};
    std::cout << "Enter AP Type\
            (1-PRIVATE, 2-GUEST): ";
    std::cin >> apType;
    WlanUtils::validateInput(apType, {1, 2});
    std::cout << std::endl;
    netConfig.info.apType = WlanUtils::convertIntToApType(apType);

    std::string ssid = "";
    std::cout << "Enter SSID (Without Quotes): ";
    std::cin >> ssid;
    Utils::validateInput(ssid);
    netConfig.ssid = ssid;

    int visible {};
    std::cout << "Make AP SSID visible (0-YES, 1-NO)?: ";
    std::cin >> visible;
    WlanUtils::validateInput(visible, {0, 1});
    std::cout << std::endl;
    netConfig.isVisible = (visible)? false:true;

    populateApElementInfo(netConfig.elementInfoConfig);

    int apInterworking {};
    std::cout << "Enter AP network access\
            (0-INTERNET_ACCESS, 1-FULL_ACCESS): ";
    std::cin >> apInterworking;
    WlanUtils::validateInput(apInterworking, {0, 1});
    std::cout << std::endl;
    netConfig.interworking = WlanUtils::convertIntToInterworking(apInterworking);

    int secMode {};
    std::cout << "Enter AP security mode (0-OPEN, 1-WEP, 2-WPA, 3-WPA2, 4-WPA3): ";
    std::cin >> secMode;
    WlanUtils::validateInput(secMode, {0, 1, 2, 3, 4});
    std::cout << std::endl;
    netConfig.apSecurity.mode = WlanUtils::convertIntToSecMode(secMode);

    int secAuth {};
    std::cout << "Enter Authentication method (0-NONE, 1-PSK, 2-EAP_SIM, 3-EAP_AKA, 4-EAP_LEAP,";
    std::cout << " 5-EAP_TLS, 6-EAP_TTLS, 7-EAP_PEAP, 8-EAP_FAST, 9-EAP_PSK, 10-SAE): ";
    std::cin >> secAuth;
    WlanUtils::validateInput(secAuth, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    std::cout << std::endl;
    netConfig.apSecurity.auth = WlanUtils::convertIntToSecAuth(secAuth);

    int secEncrypt {};
    std::cout << "Enter AP security encryption (0-RC4, 1-TKIP, 2-AES, 3-GCMP): ";
    std::cin >> secEncrypt;
    WlanUtils::validateInput(secEncrypt, {0, 1, 2, 3});
    std::cout << std::endl;
    netConfig.apSecurity.encrypt = WlanUtils::convertIntToSecEncrypt(secEncrypt);

    std::string passPhrase = "";
    std::cout << "Enter AP passphrase (Without Quotes): ";
    std::cin >> passPhrase;
    Utils::validateInput(passPhrase);
    netConfig.passPhrase = passPhrase;
}

void WlanApInterfaceManagerMenu::getConfig(std::vector<std::string> userInput) {
    std::vector<telux::wlan::ApConfig> config;
    telux::common::ErrorCode retCode = wlanApInterfaceManager_->getConfig(config);

    std::cout << "\nrequest AP Configuration Response"
                << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(retCode)
                << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        for (auto &cfg : config) {
            std::cout << "------------------------------------------" << std::endl;
            std::cout << "AP Id: " << WlanUtils::getWlanId(cfg.id) << std::endl;
            std::cout << "AP Venue Type : " << cfg.venue.type << std::endl;
            std::cout << "AP Venue Group: " << cfg.venue.group << std::endl;
            for (auto &netCfg : cfg.network) {
                std::cout << "AP Type: "
                    << WlanUtils::getWlanApType(netCfg.info.apType) << std::endl;
                std::cout << "AP Radio: "
                    << WlanUtils::apRadioTypeToString(netCfg.info.apRadio) << std::endl;
                std::cout << "AP SSID: " << netCfg.ssid << std::endl;
                std::cout << "AP is Visible: "
                    << ((netCfg.isVisible)? "Yes":"No") << std::endl;
                WlanUtils::printApElementInfo(netCfg.elementInfoConfig);
                std::cout << "AP Interworking: "
                    <<  WlanUtils::apAccessToString(netCfg.interworking) << std::endl;
                std::cout << "AP Security: " << std::endl;
                std::cout << "    Mode: "
                    << WlanUtils::apSecurityModeToString(netCfg.apSecurity.mode) << std::endl;
                std::cout << "    Authorization: "
                    << WlanUtils::apSecurityAuthToString(netCfg.apSecurity.auth) << std::endl;
                std::cout << "    Encryption: "
                    << WlanUtils::apSecurityEncryptToString(netCfg.apSecurity.encrypt) << std::endl;
                std::cout << "AP Passphrase: " << netCfg.passPhrase << std::endl;
            }
        }
    }
}

void WlanApInterfaceManagerMenu::getStatus(std::vector<std::string> userInput) {
    std::vector<telux::wlan::ApStatus> status;
    std::cout << "Request AP Status" << std::endl;

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->getStatus(status);
    std::cout << "\nRequest AP Status Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        WlanUtils::printAPStatus(status);
    }
}

void WlanApInterfaceManagerMenu::getConnectedDevices(std::vector<std::string> userInput) {
    std::vector<telux::wlan::DeviceInfo> clientsInfo;
    std::cout << "Request Connected Devices" << std::endl;

    telux::common::ErrorCode retCode =
        wlanApInterfaceManager_->getConnectedDevices(clientsInfo);
    std::cout << "\nRequest Connected Devices Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        WlanUtils::printDeviceInfo(clientsInfo);
    }
}

void WlanApInterfaceManagerMenu::manageApService(std::vector<std::string> userInput) {
    std::cout << "Manage Ap Service" << std::endl;

    int apId = 1, opr = 0;
    std::cout << "Select AP Service Operation\
            (0-STOP, 1-START, 2-RESTART): ";
    std::cin >> opr;
    WlanUtils::validateInput(opr, {0, 1, 2});
    std::cout << std::endl;

    std::cout << "Select Ap Id \
            (1-PRIMARY, 2-SECONDARY, 3-TERTIARY): ";
    std::cin >> apId;
    WlanUtils::validateInput(apId, {1, 2, 3});
    std::cout << std::endl;

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->manageApService(
        static_cast<telux::wlan::Id>(apId),
        static_cast<telux::wlan::ServiceOperation>(opr));

    std::cout << "\nManage Ap Service Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanApInterfaceManagerMenu::onApBandChanged(telux::wlan::BandType band) {
   PRINT_NOTIFICATION << " ** Wlan onApOperBandChanged **\n";

   if(band == telux::wlan::BandType::BAND_2GHZ) {
       std::cout << "AP has switched to 2.4G band" << std::endl;
   } else if(band == telux::wlan::BandType::BAND_5GHZ) {
       std::cout << "AP has switched to 5G band" << std::endl;
   } else {
       std::cout << "AP has switched to 6G band" << std::endl;
   }
}

void WlanApInterfaceManagerMenu::onApDeviceStatusChanged(
    telux::wlan::ApDeviceConnectionEvent event, std::vector<telux::wlan::DeviceIndInfo> info) {
    PRINT_NOTIFICATION << " ** Wlan onApDeviceStatusChanged **\n";
    std::cout << "Event: ";
    switch(event) {
        case telux::wlan::ApDeviceConnectionEvent::CONNECTED:
            std::cout << "New Device is connected" << std::endl;
            break;
        case telux::wlan::ApDeviceConnectionEvent::DISCONNECTED:
            std::cout << "Existing Device is disconnected" << std::endl;
            break;
        case telux::wlan::ApDeviceConnectionEvent::IPV4_UPDATED:
            std::cout << "Existing Device IPv4 is Updated" << std::endl;
            break;
        case telux::wlan::ApDeviceConnectionEvent::IPV6_UPDATED:
            std::cout << "Existing Device IPv6 is Updated" << std::endl;
            break;
        default:
            break;
    }
    if(info.size() > 0) {
        std::cout << "List of connected devices:" << std::endl;
        for(auto& dev:info) {
            std::cout << "----------------------------------------------" << std::endl;
            std::cout << "Associated AP       : " << WlanUtils::getWlanId(dev.id) << std::endl;
            std::cout << "Device MAC Address  : " << dev.macAddress << std::endl;
        }
    }
}

void WlanApInterfaceManagerMenu::onApConfigChanged(telux::wlan::Id apId) {
    PRINT_NOTIFICATION << " ** Wlan onApConfigChanged **\n";
    std::cout << "Configuration has changed for AP: " << static_cast<int>(apId) << std::endl;
}

void WlanApInterfaceManagerMenu::setSecurityConfig(std::vector<std::string> userInput) {
    std::cout << "Set AP Security Configuration" << std::endl;
    int id = 0;
    std::cout << "Enter Wlan AP Id (1-PRIMARY, 2-SECONDARY, 3-TERTIARY): ";
    std::cin >> id;
    std::cout << std::endl;
    WlanUtils::validateInput(id, {1, 2, 3});

    telux::wlan::ApSecurity security = {};
    int input = 0;
    std::cout << "Enter Security Mode (0-OPEN, 1-WEP, 2-WPA, 3-WPA2, 4-WPA3): ";
    std::cin >> input;
    std::cout << std::endl;
    WlanUtils::validateInput(input, {0, 1, 2, 3, 4});
    security.mode = static_cast<telux::wlan::SecMode>(input);
    std::cout << "Enter Authentication method (0-NONE, 1-PSK, 2-EAP_SIM, 3-EAP_AKA, 4-EAP_LEAP,";
    std::cout << " 5-EAP_TLS, 6-EAP_TTLS, 7-EAP_PEAP, 8-EAP_FAST, 9-EAP_PSK, 10-SAE): ";
    std::cin >> input;
    std::cout << std::endl;
    WlanUtils::validateInput(input, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    security.auth = static_cast<telux::wlan::SecAuth>(input);
    std::cout << "Enter Encryption Method (0-RC4, 1-TKIP, 2-AES, 3-GCMP): ";
    std::cin >> input;
    std::cout << std::endl;
    WlanUtils::validateInput(input, {0, 1, 2, 3});
    security.encrypt = static_cast<telux::wlan::SecEncrypt>(input);

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->setSecurityConfig(
        static_cast<telux::wlan::Id>(id), security);

    std::cout << "\nSet AP Security Config Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanApInterfaceManagerMenu::setSsid(std::vector<std::string> userInput) {
    std::cout << "Set AP SSID" << std::endl;
    int id = 0;
    std::cout << "Enter Wlan AP Id (1-PRIMARY, 2-SECONDARY, 3-TERTIARY): ";
    std::cin >> id;
    std::cout << std::endl;
    WlanUtils::validateInput(id, {1, 2, 3});
    std::string input{};
    std::cout << "Enter SSID (Without Quotes): ";
    std::cin >> input;
    std::cout << std::endl;
    Utils::validateInput(input);

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->setSsid(
        static_cast<telux::wlan::Id>(id), input);

    std::cout << "\nSet AP SSID Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanApInterfaceManagerMenu::setVisibility(std::vector<std::string> userInput) {
    std::cout << "Set AP Visibility" << std::endl;
    int id = 0;
    std::cout << "Enter Wlan AP Id (1-PRIMARY, 2-SECONDARY, 3-TERTIARY): ";
    std::cin >> id;
    std::cout << std::endl;
    WlanUtils::validateInput(id, {1, 2, 3});
    int input{};
    std::cout << "Enter AP SSID Visibility (0-INVISIBLE, 1-VISIBLE): ";
    std::cin >> input;
    std::cout << std::endl;
    WlanUtils::validateInput(input, {0, 1});

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->setVisibility(
        static_cast<telux::wlan::Id>(id), static_cast<bool>(input));

    std::cout << "\nSet AP SSID Visibility Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanApInterfaceManagerMenu::configureElementInfo(std::vector<std::string> userInput) {
    std::cout << "Enable AP Element Info" << std::endl;
    int id = 0;
    std::cout << "Enter Wlan AP Id (1-PRIMARY, 2-SECONDARY, 3-TERTIARY): ";
    std::cin >> id;
    std::cout << std::endl;
    WlanUtils::validateInput(id, {1, 2, 3});
    telux::wlan::ApElementInfoConfig ElemInfoConfig {};
    populateApElementInfo(ElemInfoConfig);

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->setElementInfoConfig(
        static_cast<telux::wlan::Id>(id), ElemInfoConfig);

    std::cout << "\nEnable AP Element Info Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanApInterfaceManagerMenu::setPassPhrase(std::vector<std::string> userInput) {
    std::cout << "Set AP SSID Passphrase" << std::endl;
    int id = 0;
    std::cout << "Enter Wlan AP Id (1-PRIMARY, 2-SECONDARY, 3-TERTIARY): ";
    std::cin >> id;
    std::cout << std::endl;
    WlanUtils::validateInput(id, {1, 2, 3});
    std::string input{};
    std::cout << "Enter SSID Passphrase (Without Quotes): ";
    std::cin >> input;
    std::cout << std::endl;
    Utils::validateInput(input);

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->setPassPhrase(
        static_cast<telux::wlan::Id>(id), input);

    std::cout << "\nSet AP SSID Passphrase Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanApInterfaceManagerMenu::populateApElementInfo(
    telux::wlan::ApElementInfoConfig& ElementInfoConfig) {

    int input = 0;
    std::cout << "Enable AP Element Info (0-DISABLE, 1-ENABLE): ";
    std::cin >> input;
    std::cout << std::endl;
    WlanUtils::validateInput(input, {0, 1});
    ElementInfoConfig.isEnabled = false;
    if(input) {
        ElementInfoConfig.isEnabled = true;
        input = 0;
        std::cout << "Is Interworking Enabled (0-NO, 1-YES): ";
        std::cin >> input;
        std::cout << std::endl;
        WlanUtils::validateInput(input, {0, 1});
        ElementInfoConfig.isInterworkingEnabled = false;
        if(input) {
            ElementInfoConfig.isInterworkingEnabled = true;
        }

        input = 0;
        std::cout << "Enter Network Access Type (0-PRIVATE, 1-PRIVATE_WITH_GUEST, ";
        std::cout << "2-CHARGEABLE_PUBLIC, 3-FREE_PUBLIC, 4-PERSONAL_DEVICE, ";
        std::cout << "5-EMERGENCY_SERVICES_ONLY, 6-TEST_OR_EXPERIMENTAL, 7-WILDCARD): ";
        std::cin >> input;
        std::cout << std::endl;
        WlanUtils::validateInput(input, {0, 1, 2, 3, 4, 5, 6, 7});
        ElementInfoConfig.netAccessType = static_cast<telux::wlan::NetAccessType>(input);

        input = 0;
        std::cout << "Does network provide connectivity to internet (0-UNSPECIFIED, 1-YES): ";
        std::cin >> input;
        std::cout << std::endl;
        WlanUtils::validateInput(input, {0, 1});
        ElementInfoConfig.internet = false;
        if(input) {
            ElementInfoConfig.internet = true;
        }

        input = 0;
        std::cout << "Is additional step required for access (0-NO, 1-YES): ";
        std::cin >> input;
        std::cout << std::endl;
        WlanUtils::validateInput(input, {0, 1});
        ElementInfoConfig.asra = false;
        if(input) {
            ElementInfoConfig.asra = true;
        }

        input = 0;
        std::cout << "Is emergency services reachable (0-NO, 1-YES): ";
        std::cin >> input;
        std::cout << std::endl;
        WlanUtils::validateInput(input, {0, 1});
        ElementInfoConfig.esr = false;
        if(input) {
            ElementInfoConfig.esr = true;
        }

        input = 0;
        std::cout << "Is unauthenticated emergency service accessible (0-NO, 1-YES): ";
        std::cin >> input;
        std::cout << std::endl;
        WlanUtils::validateInput(input, {0, 1});
        ElementInfoConfig.uesa = false;
        if(input) {
            ElementInfoConfig.uesa = true;
        }

        int userPrompt = 0;
        std::cout << "Do you want to enter venue info (0-NO, 1-YES)?: ";
        std::cin >> userPrompt;
        std::cout << std::endl;
        WlanUtils::validateInput(userPrompt, {0, 1});

        if(userPrompt) {
            input = 0;
            std::cout << "Enter venue group as defined in IEEE Std 802.11u-2011, 7.3.1.34: ";
            std::cin >> input;
            std::cout << std::endl;
            WlanUtils::validateInput(input, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
            ElementInfoConfig.venueGroup = input;

            input = 0;
            std::cout << "Enter venue type as defined in IEEE Std 802.11u-2011, 7.3.1.34: ";
            std::cin >> input;
            std::cout << std::endl;
            ElementInfoConfig.venueType = input;
        }

        userPrompt = 0;
        std::cout << "Do you want to enter Homogeneous ESS identifier (0-NO, 1-YES)?: ";
        std::cin >> userPrompt;
        std::cout << std::endl;
        WlanUtils::validateInput(userPrompt, {0, 1});
        std::string inStr = "";
        if(userPrompt) {
            std::cout << "Enter input Homogeneous ESS identifier (without quotes): ";
            std::cin >> inStr;
            std::cout << std::endl;
            ElementInfoConfig.hessid = inStr;
        }

        inStr = "";
        std::cout << "Enter additional vendor elements for Beacon and Probe response ";
        std::cout << "frames (without quotes): ";
        std::cin >> inStr;
        std::cout << std::endl;
        ElementInfoConfig.vendorElements = inStr;

        inStr = "";
        std::cout << "Enter additional vendor elements for (Re)Association Response frames ";
        std::cout << "(without quotes): ";
        std::cin >> inStr;
        std::cout << std::endl;
        ElementInfoConfig.assocRespElements = inStr;
    }
}

