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

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/common/DeviceConfig.hpp>
#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "SnatMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

SnatMenu::SnatMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    snatManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

SnatMenu::~SnatMenu() {
}

bool SnatMenu::init() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (snatManager_ == nullptr) {
        auto initCb = std::bind(&SnatMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();
        //Try both local and remote operation type. If operation type is not supported,
        // nullptr is returned. snatManager_ pointer will be associated with valid return pointer
        auto localSnatMgr = dataFactory.getNatManager(
            telux::data::OperationType::DATA_LOCAL, initCb);
        if (localSnatMgr) {
            snatManager_ = localSnatMgr;
        }
        auto remoteSnatMgr = dataFactory.getNatManager(
            telux::data::OperationType::DATA_REMOTE, initCb);
        if (remoteSnatMgr) {
            snatManager_ = remoteSnatMgr;
        }
        if(snatManager_ == nullptr ) {
            //Return immediately
            std::cout << "\nError encountered in initializing SNAT Manager" << std::endl;
            return false;
        }
        snatManager_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        //Snat Manager is guaranteed to be valid pointer at this point. If manager initialization
        //fails and factory invalidated it's own pointer to snat manager before reaching this point,
        //reference count of Snat manager should still be 1
        telux::common::ServiceStatus subSystemStatus = snatManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing SNAT Manager, Please wait ..." << std::endl;
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = snatManager_->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nSNAT Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nSNAT Manager initialization failed" << std::endl;
            snatManager_ = nullptr;
            return false;
        }
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> addStaticNatEntry
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "add_static_nat", {},
                std::bind(&SnatMenu::addStaticNatEntry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeStaticNatEntry
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "remove_static_nat", {},
                std::bind(&SnatMenu::removeStaticNatEntry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> reqStaticNatEntries
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "request_static_nat_entries",
                {}, std::bind(&SnatMenu::requestStaticNatEntries, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> addStaticNatEntry_V1
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "add_static_nat_v1", {},
                std::bind(&SnatMenu::addStaticNatEntry_V1, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeStaticNatEntry_V1
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "remove_static_nat_v1", {},
                std::bind(&SnatMenu::removeStaticNatEntry_V1, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> reqStaticNatEntries_V1
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6",
                        "request_static_nat_entries_v1",
                {}, std::bind(&SnatMenu::requestStaticNatEntries_V1, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {addStaticNatEntry,
            removeStaticNatEntry, reqStaticNatEntries, addStaticNatEntry_V1,
            removeStaticNatEntry_V1, reqStaticNatEntries_V1};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void SnatMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void SnatMenu::addStaticNatEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "Add Static NAT entry\n";
    telux::data::BackhaulInfo bhInfo {};
    DataUtils::populateBackhaulInfo(bhInfo);

    char delimiter = '\n';
    std::string privIpAddr;
    std::cout << "Enter Private IP address: ";
    std::getline(std::cin, privIpAddr, delimiter);

    std::string protoStr;
    std::cout << "Enter Protocol (TCP, UDP, ICMP, ESP): ";
    std::getline(std::cin, protoStr, delimiter);

    int privPort = 0 , globPort = 0;
    if (protoStr != "ICMP" && protoStr != "ESP") {
        std::cout << "Enter Private port: ";
        std::cin >> privPort;
        Utils::validateInput(privPort);

        std::cout << "Enter Global port: ";
        std::cin >> globPort;
        Utils::validateInput(globPort);
    }

    telux::data::IpProtocol proto = DataUtils::getProtcol(protoStr);
    struct NatConfig natConfig;
    natConfig.addr = privIpAddr;
    natConfig.port = (uint16_t)privPort;
    natConfig.globalPort = (uint16_t)globPort;
    natConfig.proto = (uint8_t)proto;

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "addStaticNatEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = snatManager_->addStaticNatEntry(bhInfo, natConfig, respCb);
    Utils::printStatus(retStat);
}

void SnatMenu::removeStaticNatEntry(std::vector<std::string> inputCommand) {
    std::cout << "Remove Static NAT entry\n";
    telux::common::Status retStat;

    telux::data::BackhaulInfo bhInfo {};
    DataUtils::populateBackhaulInfo(bhInfo);

    char delimiter = '\n';
    std::string privIpAddr;
    std::cout << "Enter Private IP address: ";
    std::getline(std::cin, privIpAddr, delimiter);

    std::string protoStr;
    std::cout << "Enter Protocol (TCP, UDP, ICMP, ESP): ";
    std::getline(std::cin, protoStr, delimiter);

    int privPort = 0 , globPort = 0;
    if (protoStr != "ICMP" && protoStr != "ESP") {
        std::cout << "Enter Private port: ";
        std::cin >> privPort;
        Utils::validateInput(privPort);

        std::cout << "Enter Global port: ";
        std::cin >> globPort;
        Utils::validateInput(globPort);
    }

    telux::data::IpProtocol proto = DataUtils::getProtcol(protoStr);
    struct NatConfig natConfig;
    natConfig.addr = privIpAddr;
    natConfig.port = (uint16_t)privPort;
    natConfig.globalPort = (uint16_t)globPort;
    natConfig.proto = (uint8_t)proto;

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "removeStaticNatEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = snatManager_->removeStaticNatEntry(bhInfo, natConfig, respCb);
    Utils::printStatus(retStat);
}

void SnatMenu::requestStaticNatEntries(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "List Static NAT entries\n";
    telux::data::BackhaulInfo bhInfo {};
    DataUtils::populateBackhaulInfo(bhInfo);

    auto respCb = [](const std::vector<NatConfig> &snatEntries, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestStaticNatEntries Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (snatEntries.size() > 0) {
            std::cout << "==========================================\n";
        }
        for (auto entry : snatEntries) {
            std::cout << "Private IP address: " << entry.addr << "\nPrivate port: " << entry.port
                      << "\nGlobal port: " << entry.globalPort
                      << "\nProtocol: " << DataUtils::protocolToString(entry.proto)
                      << "\n==========================================\n";
        }
    };
    retStat = snatManager_->requestStaticNatEntries(bhInfo, respCb);
    Utils::printStatus(retStat);
}

void SnatMenu::addStaticNatEntry_V1(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "Add Static NAT entry\n";

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    char delimiter = '\n';
    std::string privIpAddr;
    std::cout << "Enter Private IP address: ";
    std::getline(std::cin, privIpAddr, delimiter);

    std::string protoStr;
    std::cout << "Enter Protocol (TCP, UDP, ICMP, ESP): ";
    std::getline(std::cin, protoStr, delimiter);

    int privPort = 0 , globPort = 0;
    if (protoStr != "ICMP" && protoStr != "ESP") {
        std::cout << "Enter Private port: ";
        std::cin >> privPort;
        Utils::validateInput(privPort);

        std::cout << "Enter Global port: ";
        std::cin >> globPort;
        Utils::validateInput(globPort);
    }

    telux::data::IpProtocol proto = DataUtils::getProtcol(protoStr);
    struct NatConfig natConfig;
    natConfig.addr = privIpAddr;
    natConfig.port = (uint16_t)privPort;
    natConfig.globalPort = (uint16_t)globPort;
    natConfig.proto = (uint8_t)proto;

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "addStaticNatEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = snatManager_->addStaticNatEntry(profileId, natConfig, respCb,
            static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

void SnatMenu::removeStaticNatEntry_V1(std::vector<std::string> inputCommand) {
    std::cout << "Remove Static NAT entry\n";
    telux::common::Status retStat;

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    char delimiter = '\n';
    std::string privIpAddr;
    std::cout << "Enter Private IP address: ";
    std::getline(std::cin, privIpAddr, delimiter);

    std::string protoStr;
    std::cout << "Enter Protocol (TCP, UDP, ICMP, ESP): ";
    std::getline(std::cin, protoStr, delimiter);

    int privPort = 0 , globPort = 0;
    if (protoStr != "ICMP" && protoStr != "ESP") {
        std::cout << "Enter Private port: ";
        std::cin >> privPort;
        Utils::validateInput(privPort);

        std::cout << "Enter Global port: ";
        std::cin >> globPort;
        Utils::validateInput(globPort);
    }

    telux::data::IpProtocol proto = DataUtils::getProtcol(protoStr);
    struct NatConfig natConfig;
    natConfig.addr = privIpAddr;
    natConfig.port = (uint16_t)privPort;
    natConfig.globalPort = (uint16_t)globPort;
    natConfig.proto = (uint8_t)proto;

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "removeStaticNatEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = snatManager_->removeStaticNatEntry(
        profileId, natConfig, respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

void SnatMenu::requestStaticNatEntries_V1(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "List Static NAT entries\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto respCb = [](const std::vector<NatConfig> &snatEntries, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestStaticNatEntries Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (snatEntries.size() > 0) {
            std::cout << "==========================================\n";
        }
        for (auto entry : snatEntries) {
            std::cout << "Private IP address: " << entry.addr << "\nPrivate port: " << entry.port
                      << "\nGlobal port: " << entry.globalPort
                      << "\nProtocol: " << DataUtils::protocolToString(entry.proto)
                      << "\n==========================================\n";
        }
    };
    retStat = snatManager_->requestStaticNatEntries(profileId, respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}
