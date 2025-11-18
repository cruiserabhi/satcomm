/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "WlanDeviceManagerMenu.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanDeviceManagerMenu::WlanDeviceManagerMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
       menuOptionsAdded_ = false;
}

WlanDeviceManagerMenu::~WlanDeviceManagerMenu() {
    if (wlanDeviceManager_) {
        wlanDeviceManager_ = nullptr;
    }
}

bool WlanDeviceManagerMenu::isSubSystemReady()  {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (wlanDeviceManager_ == nullptr) {
        auto initCb =
            std::bind(&WlanDeviceManagerMenu::onInitComplete, this, std::placeholders::_1);
        auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
        wlanDeviceManager_ = wlanFactory.getWlanDeviceManager(initCb);
        if (wlanDeviceManager_ == nullptr) {
            //Return immediately
            std::cout << "\nError encountered in initializing Wlan Device Manager" << std::endl;
            return false;
        }
        wlanDeviceManager_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        //WlanDeviceManager is guaranteed to be valid pointer at this point. If manager init
        //fails and factory invalidated it's own pointer to WlanDevicemanager before reaching this
        //point, reference count of WlanDevicemanager should still be 1
        std::cout << "\nInitializing WlanDeviceManager, Please wait ..." << std::endl;
        cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
        subSystemStatus = wlanDeviceManager_->getServiceStatus();
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nWlan Device Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nWlan Device Manager initialization failed" << std::endl;
            wlanDeviceManager_ = nullptr;
            return false;
        }
    }
    return true;
}

bool WlanDeviceManagerMenu::init() {
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> enableWlan
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "enable_wlan", {},
                std::bind(&WlanDeviceManagerMenu::enableWlan, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setMode
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "set_mode", {},
                std::bind(&WlanDeviceManagerMenu::setMode, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "get_config", {},
                std::bind(&WlanDeviceManagerMenu::getConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "get_status", {},
                std::bind(&WlanDeviceManagerMenu::getStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setActiveCountry
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "set_active_country", {},
                std::bind(&WlanDeviceManagerMenu::setActiveCountry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getRegulatoryParams
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "get_regulatory_params",
            {}, std::bind(&WlanDeviceManagerMenu::getRegulatoryParams, this,
            std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setTxPower
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "set_tx_power", {},
                std::bind(&WlanDeviceManagerMenu::setTxPower, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getTxPower
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "get_tx_power", {},
                std::bind(&WlanDeviceManagerMenu::getTxPower, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {enableWlan, setMode,
            getConfig, getStatus, setActiveCountry, getRegulatoryParams, setTxPower, getTxPower};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void WlanDeviceManagerMenu::enableWlan(std::vector<std::string> userInput) {
    int wlanEnable;

    std::cout << "Wlan Enablement \n";
    std::cout << "Enable/Disable Wlan (1-enable, 0-disable): ";
    std::cin >> wlanEnable;
    std::cout << std::endl;
    WlanUtils::validateInput(wlanEnable, {0, 1});

    telux::common::ErrorCode retCode =  wlanDeviceManager_->enable(static_cast<bool>(wlanEnable));
    std::cout << "\nWlan Enable Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanDeviceManagerMenu::setMode(std::vector<std::string> userInput) {
    int numAps;
    int numSta;

    std::cout << "Setting Wlan Mode \n";
    std::cout << "Enter Number of APs to be enabled: ";
    std::cin >> numAps;
    WlanUtils::validateInput(numAps, {0, 1, 2, 3});
    std::cout << std::endl;

    std::cout << "Enter Number of Stations to be enabled: ";
    std::cin >> numSta;
    WlanUtils::validateInput(numSta, {0, 1, 2});
    std::cout << std::endl;

    telux::common::ErrorCode retCode = wlanDeviceManager_->setMode(numAps, numSta);
    std::cout << "\nSetting Wlan Mode Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanDeviceManagerMenu::getConfig(std::vector<std::string> userInput) {

    int numAp, numSta;

    telux::common::ErrorCode retCode =  wlanDeviceManager_->getConfig(numAp, numSta);
    std::cout << "\nrequest Wlan Config Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        if(numAp) {
            std::cout << "Num of configured AP: " << numAp << std::endl;
        } else {
            std::cout << "No AP is configured" << std::endl;
        }
        if(numSta) {
            std::cout << "Num of configured Sta: " << numSta << std::endl;
        } else {
            std::cout << "No Station is configured" << std::endl;
        }
    }
}

void WlanDeviceManagerMenu::getStatus(std::vector<std::string> userInput) {
    std::vector<telux::wlan::InterfaceStatus> status;
    bool isEnabled;

    std::cout << "Request Wlan Status \n";
    telux::common::ErrorCode retCode =  wlanDeviceManager_->getStatus(isEnabled, status);
    std::cout << "\nrequest Wlan Status Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Wlan is " <<((isEnabled)? "enabled":"disabled") << std::endl;
        if(status.size() > 0) {
            for (auto &ifStatus : status) {
                std::cout << "------------------------------------------" << std::endl;
                std::cout << "device: "
                        << WlanUtils::getWlanDeviceName(ifStatus.device) << std::endl;
                WlanUtils::printAPStatus(ifStatus.apStatus);
                WlanUtils::printStaStatus(ifStatus.staStatus);
            }
        } else {
            std::cout << "No AP or station is currently active" << std::endl;
        }
    }
}

void WlanDeviceManagerMenu::setActiveCountry(std::vector<std::string> userInput) {
    std::cout << "Set Active Country" << std::endl;
    std::string input("");

    std::cout << "Enter country name: ";
    std::cin >> input;
    Utils::validateInput(input);
    std::cout << std::endl;

    telux::common::ErrorCode retCode = wlanDeviceManager_->setActiveCountry(input);
    std::cout << "\nSetting Active Country Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanDeviceManagerMenu::getRegulatoryParams(std::vector<std::string> userInput) {
    std::cout << "Get Regulatory Parameters" << std::endl;
    telux::wlan::RegulatoryParams regulatoryParams{};

    telux::common::ErrorCode retCode = wlanDeviceManager_->getRegulatoryParams(regulatoryParams);
    std::cout << "\nGet Regulatory Parameters Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is Successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Current Regulatory Parameters: \n";
        std::cout << "Country: " << regulatoryParams.country << std::endl;
        std::cout << "Operating Channel: " << regulatoryParams.opChannel << std::endl;
        for(auto cls:regulatoryParams.opClass) {
            std::cout << "Operating Class: " << cls << std::endl;
        }
        std::cout << "Transmit Power (MilliWatts): " << regulatoryParams.txPowerMw << std::endl;
    }
}

void WlanDeviceManagerMenu::setTxPower(std::vector<std::string> userInput) {
    std::cout << "Set Transmit Power" << std::endl;

    int txPower = 0;
    std::cout << "Enter Desired Transmit Power (milliwatts): ";
    std::cin >> txPower;
    Utils::validateInput(txPower);
    std::cout << std::endl;
    telux::common::ErrorCode retCode = wlanDeviceManager_->setTxPower(
        static_cast<uint32_t>(txPower));
    std::cout << "\nSet Transmit Power Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is Successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanDeviceManagerMenu::getTxPower(std::vector<std::string> userInput) {
    std::cout << "Get Current Transmit Power" << std::endl;

    uint32_t txPower = 0;
    telux::common::ErrorCode retCode = wlanDeviceManager_->getTxPower(txPower);
    std::cout << "Get Current Transmit Power Response"
              << (retCode == telux::common::ErrorCode::SUCCESS? " is Successful":" failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Current transmit power is " << txPower << std::endl;
    }
}

void WlanDeviceManagerMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void WlanDeviceManagerMenu::onServiceStatusChange(telux::common::ServiceStatus status) {
   PRINT_NOTIFICATION << " ** Wlan onServiceStatusChange **\n";
   switch(status) {
      case telux::common::ServiceStatus::SERVICE_AVAILABLE:
         std::cout << " SERVICE_AVAILABLE\n";
         break;
      case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
         std::cout << " SERVICE_UNAVAILABLE\n";
         break;
      default:
         std::cout << " Unknown service status \n";
         break;
   }
}

void WlanDeviceManagerMenu::onEnableChanged(bool enable) {
   PRINT_NOTIFICATION << " ** Wlan onEnableChanged **\n";
   if(enable) {
       std::cout << "Wlan is enabled" << std::endl;
   } else {
       std::cout << "Wlan is disabled" << std::endl;
   }
}

void WlanDeviceManagerMenu::onTempCrossed(
   float temperature, telux::wlan::DevicePerfState perfState) {
   PRINT_NOTIFICATION << " ** Wlan onTempCrossed **\n";
   std::cout << "Current device temperature: " << temperature << std::endl;
   std::cout << "Device Performance is ";
   switch(perfState) {
    case telux::wlan::DevicePerfState::FULL:
        std::cout << "Full" << std::endl;
        break;
    case telux::wlan::DevicePerfState::REDUCED:
        std::cout << "Reduced" << std::endl;
        break;
    case telux::wlan::DevicePerfState::SHUTDOWN:
        std::cout << "Shutdown" << std::endl;
        break;
    default:
        std::cout << "Unkown" << std::endl;
        break;
   };
}
