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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:

 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "BridgeMenu.hpp"

using namespace std;

BridgeMenu::BridgeMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    bridgeMgr_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

BridgeMenu::~BridgeMenu() {
}

bool BridgeMenu::init() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (bridgeMgr_ == nullptr) {
        auto initCb = std::bind(&BridgeMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();
        bridgeMgr_ = dataFactory.getBridgeManager(initCb);
        if (bridgeMgr_ == nullptr) {
            std::cout << "\nError encountered in initializing Bridge Manager" << std::endl;
            return false;
        }
        bridgeMgr_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        //Bridge Manager is guaranteed to be valid pointer at this point. If manager initialization
        //fails and factory invalidated it's own pointer to Bridge manager before reaching this
        //point, reference count of L2TP manager should still be 1
        telux::common::ServiceStatus subSystemStatus = bridgeMgr_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing Bridge Manager, Please wait ..." << std::endl;
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = bridgeMgr_->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nBridge Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nBridge Manager initialization failed" << std::endl;
            bridgeMgr_ = nullptr;
            return false;
        }
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> enableBridge
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Set_Bridge_State", {},
                std::bind(&BridgeMenu::enableBridge, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> addBridge
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Add_Bridge", {},
                std::bind(&BridgeMenu::addBridge, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getBridgeInfo
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Get_Bridge_Info", {},
                std::bind(&BridgeMenu::getBridgeInfo, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeBridge
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Remove_Bridge", {},
                std::bind(&BridgeMenu::removeBridge, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setInterfaceBridge
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5",
                "Set_Interface_Bridge", {}, std::bind(&BridgeMenu::setInterfaceBridge,
                this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getInterfaceBridge
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Get_Interface_Bridge",
                {}, std::bind(&BridgeMenu::getInterfaceBridge,
                this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {enableBridge, addBridge,
            getBridgeInfo, removeBridge, setInterfaceBridge, getInterfaceBridge};

        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void BridgeMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void BridgeMenu::enableBridge(std::vector<std::string> inputCommand) {
    bool enableBridge = false;
    telux::common::Status retStat;
    int temp = 0;
    std::cout << "Enter the desired state \n (1-enable, 0-disable): ";
    std::cin >> temp;
    Utils::validateInput(temp);
    if(temp == 1) {
        enableBridge = true;
    } else if(temp == 0) {
        enableBridge = false;
    } else {
        std::cout << "Invalid bridge state input, try again" << std::endl;
        return;
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Set bridge state request is"
                  << (error == telux::common::ErrorCode::SUCCESS ? " successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = bridgeMgr_->enableBridge(enableBridge, respCb);
    Utils::printStatus(retStat);
}

void BridgeMenu::addBridge(std::vector<std::string> inputCommand) {
    int temp;
    char delimiter = '\n';
    BridgeInfo config;
    telux::common::Status retStat;

    std::cout << "Enter Interface name(ex:wlan0, eth0, etc.) : ";
    std::getline(std::cin, config.ifaceName, delimiter);

    std::cout << "Enter Interface Type\n (1-WLAN_AP, 2-WLAN_STA, 3-ETH): ";
    std::cin >> temp;
    Utils::validateInput(temp, {static_cast<int>(telux::data::net::BridgeIFaceType::WLAN_AP),
        static_cast<int>(telux::data::net::BridgeIFaceType::WLAN_STA),
        static_cast<int>(telux::data::net::BridgeIFaceType::ETH)});
    config.ifaceType = static_cast<telux::data::net::BridgeIFaceType>(temp);

    std::cout << "Enter required bandwidth(max 900Mbps) : ";
    std::cin >> temp;
    Utils::validateInput(temp);
    config.bandwidth = temp;

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Add software bridge request"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    retStat= bridgeMgr_->addBridge(config, respCb);
    Utils::printStatus(retStat);
}

void BridgeMenu::getBridgeInfo(std::vector<std::string> inputCommand) {

    telux::common::Status retStat;
    auto respCb = [](const std::vector<BridgeInfo> &configs, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Get software bridge info request"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        for (auto c : configs) {
            std::cout << "Iface name: " << c.ifaceName << ", ifaceType: " << (int)c.ifaceType
                      << ", bandwidth: " << c.bandwidth << std::endl;
        }
    };
    retStat= bridgeMgr_->requestBridgeInfo(respCb);
    Utils::printStatus(retStat);
}

void BridgeMenu::removeBridge(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::string ifaceName;
    char delimiter = '\n';
    std::cout << "Enter Interface name(ex:wlan0, eth0, etc.) : ";
    std::getline(std::cin, ifaceName, delimiter);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Delete software bridge request"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = bridgeMgr_->removeBridge(ifaceName, respCb);
    Utils::printStatus(retStat);
}

void BridgeMenu::setInterfaceBridge(std::vector<std::string> userInput) {
    std::cout << "Set Interface Bridge\n";
    uint32_t bridgeId = 0;

    int ifaceType;
    std::cout << "Enter Interface Type\n (1-WLAN, 2-ETH, 3-ECM, 4-RNDIS, 5-MHI,\n"
              << "6-VMTAP0, 7-VMTAP1, 8-ETH2, 9-AP_PRIMARY,\n"
              << "10-AP_SECONDARY, 11-AP_TERTIARY, 12-AP_QUATERNARY) :\n";
    std::cin >> ifaceType;
    Utils::validateInput(ifaceType, {static_cast<int>(telux::data::InterfaceType::WLAN),
        static_cast<int>(telux::data::InterfaceType::ETH),
        static_cast<int>(telux::data::InterfaceType::ECM),
        static_cast<int>(telux::data::InterfaceType::RNDIS),
        static_cast<int>(telux::data::InterfaceType::MHI),
        static_cast<int>(telux::data::InterfaceType::ETH2),
        static_cast<int>(telux::data::InterfaceType::VMTAP0),
        static_cast<int>(telux::data::InterfaceType::VMTAP1),
        static_cast<int>(telux::data::InterfaceType::AP_PRIMARY),
        static_cast<int>(telux::data::InterfaceType::AP_SECONDARY),
        static_cast<int>(telux::data::InterfaceType::AP_TERTIARY),
        static_cast<int>(telux::data::InterfaceType::AP_QUATERNARY)});
    telux::data::InterfaceType infType = static_cast<telux::data::InterfaceType>(ifaceType);
    std::cout << std::endl;

    std::cout << "Enter BridgeId: ";
    std::cin >> bridgeId;
    Utils::validateInput(bridgeId);
    std::cout << std::endl;

    telux::common::ErrorCode retCode = bridgeMgr_->setInterfaceBridge(
        static_cast<telux::data::InterfaceType>(infType), bridgeId);
    std::cout << "\nsetInterfaceBridge Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void BridgeMenu::getInterfaceBridge(std::vector<std::string> userInput) {
    std::cout << "Get Interface BridgeId \n";
    uint32_t bridgeId = 0;

    int ifaceType;
    std::cout << "Enter Interface Type\n (1-WLAN, 2-ETH, 3-ECM, 4-RNDIS, 5-MHI,\n"
              << "6-VMTAP0, 7-VMTAP1, 8-ETH2, 9-AP_PRIMARY,\n"
              << "10-AP_SECONDARY, 11-AP_TERTIARY, 12-AP_QUATERNARY) :\n";
    std::cin >> ifaceType;
    Utils::validateInput(ifaceType, {static_cast<int>(telux::data::InterfaceType::WLAN),
        static_cast<int>(telux::data::InterfaceType::ETH),
        static_cast<int>(telux::data::InterfaceType::ECM),
        static_cast<int>(telux::data::InterfaceType::RNDIS),
        static_cast<int>(telux::data::InterfaceType::MHI),
        static_cast<int>(telux::data::InterfaceType::VMTAP0),
        static_cast<int>(telux::data::InterfaceType::VMTAP1),
        static_cast<int>(telux::data::InterfaceType::ETH2),
        static_cast<int>(telux::data::InterfaceType::AP_PRIMARY),
        static_cast<int>(telux::data::InterfaceType::AP_SECONDARY),
        static_cast<int>(telux::data::InterfaceType::AP_TERTIARY),
        static_cast<int>(telux::data::InterfaceType::AP_QUATERNARY)});
    telux::data::InterfaceType infType = static_cast<telux::data::InterfaceType>(ifaceType);
    std::cout << std::endl;

    telux::common::ErrorCode retCode = bridgeMgr_->getInterfaceBridge(
        static_cast<telux::data::InterfaceType>(ifaceType), bridgeId);
    std::cout << "\ngetInterfaceBridge Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if (retCode == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Bridge Id: " << bridgeId << std::endl;
    }
}