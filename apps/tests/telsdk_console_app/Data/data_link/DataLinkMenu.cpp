/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>
#include <sstream>

#include <telux/common/DeviceConfig.hpp>
#include <telux/data/DataFactory.hpp>
#include <Utils.hpp>

#include "DataLinkMenu.hpp"
#include "../DataUtils.hpp"
#include "DataLinkListener.hpp"

using namespace std;

#define PRINT_NOTIFICATION std::cout << "\n\033[1;35mNOTIFICATION: \033[0m"

DataLinkMenu::DataLinkMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    std::cout << "DataLinkMenu constructed" << std::endl;
    dataLinkManager_ = nullptr;
    addMenuCmds_ = false;
    subSystemStatusUpdated_ = false;
    dataLinkListener_ = std::make_shared<DataLinkListener>();
}

DataLinkMenu::~DataLinkMenu() {
    std::cout << "DataLinkMenu destructed" << std::endl;
}

bool DataLinkMenu::init() {
    std::cout << "DataLinkMenu init" << std::endl;
    bool initStat = initDataLinkManagerAndListener();

    if (addMenuCmds_ == false) {
        addMenuCmds_ = true;
        std::shared_ptr<ConsoleAppCommand> getEthCapability =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "get_eth_capability", {},
            std::bind(&DataLinkMenu::getEthCapability, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setPeerEthCapability =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "set_peer_eth_capability",
            {}, std::bind(&DataLinkMenu::setPeerEthCapability, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setPeerModeChangeRequestStatus =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3",
            "set_peer_mode_change_request_status", {},
            std::bind(&DataLinkMenu::setPeerModeChangeRequestStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> registerListener =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "register_listener", {},
            std::bind(&DataLinkMenu::registerListener, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> deregisterListener =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "deregister_listener", {},
            std::bind(&DataLinkMenu::deregisterListener, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setLocalEthOperatingMode =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6",
            "set_local_eth_operating_mode", {},
            std::bind(&DataLinkMenu::setLocalEthOperatingMode, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setEthDataLink =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7",
            "set_eth_datalink", {},
            std::bind(&DataLinkMenu::setEthDataLink, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {getEthCapability,
            setPeerEthCapability, setPeerModeChangeRequestStatus, registerListener,
            deregisterListener, setLocalEthOperatingMode, setEthDataLink};
        addCommands(commandsList);
    }

    ConsoleApp::displayMenu();
    return initStat;
}

bool DataLinkMenu::initDataLinkManagerAndListener() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool retValue = false;
    subSystemStatusUpdated_ = false;
    auto initCb = std::bind(&DataLinkMenu::onInitCompleted, this,
        std::placeholders::_1);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto dataLinkManager =
        dataFactory.getDataLinkManager(initCb);
    if(dataLinkManager) {
        std::cout << "\nInitializing Data Link Manager, Please wait..." << std::endl;
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
        subSystemStatus = dataLinkManager->getServiceStatus();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nData Link Manager is ready" << std::endl;
            retValue = true;
        }
        else {
            std::cout << "\nData Link Manager is not ready" << std::endl;
            //If manager exist - deregister and remove it
            if (dataLinkManager_) {
                dataLinkManager_->deregisterListener(dataLinkListener_);
                dataLinkManager_ = nullptr;
            }
            retValue = false;
        }

        //If it is new manager and initialization passed
        if ((retValue == true) && (!dataLinkManager_)) {
            dataLinkManager_ = dataLinkManager;
            dataLinkManager_->registerListener(dataLinkListener_);
        }
    }
    return retValue;
}

void DataLinkMenu::onInitCompleted(telux::common::ServiceStatus status) {
    std::cout << "DataLinkMenu onInitCompleted " << std::endl;
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void DataLinkMenu::getEthCapability(std::vector<std::string> inputCommand) {
    std::cout << __FUNCTION__ << std::endl;
    EthCapability ethCapability;
    telux::common::Status status = telux::common::Status::FAILED;
    status = dataLinkManager_->getEthCapability(ethCapability);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to get eth capability" << std::endl;
        return;
    } else {
        if(ethCapability.ethModes == 0) {
            std::cout << " empty ethernet capability" << std::endl;
        } else {
            std::cout << " ethernet capability: " ;
            for (int i = 0; ethCapability.ethModes >= (1 << i); ++i ) {
                if (ethCapability.ethModes & (1 << i)) {
                    telux::data::EthModeType ethModeType =
                       static_cast<telux::data::EthModeType>(1 << i);
                    std::cout << DataLinkListener::ethModeTypeToString(ethModeType) << ", ";
               }
            }
            std::cout << std::endl;
        }
    }
}

void DataLinkMenu::setPeerEthCapability(std::vector<std::string> inputCommand) {

    char delimiter = '\n';
    std::string capabilities = "";
    telux::data::EthModes ethModes = 0;
    std::cout << "Available eth capability: " << std::endl;
    for (int i = 0; i <= 8; ++i) {
        telux::data::EthModeType ethModeType = static_cast<telux::data::EthModeType>(1 << i);
        std::cout << i << " - " << DataLinkListener::ethModeTypeToString(ethModeType) << std::endl;
    }

    std::cout << "Enter peer eth capabilities\n(For example: enter 0,7 "
                 "for USXGMII 10G & SGMII 1G data rate supported): ";
    std::getline(std::cin, capabilities, delimiter);

    std::stringstream ss(capabilities);
    int i = -1;
    while(ss >> i) {
        if(i >= 0 && i <= 8) {
            ethModes = ethModes | (1 << i);
            if(ss.peek() == ',' || ss.peek() == ' ')
                ss.ignore();
        } else {
            std::cout << "ERROR: invalid input please retry with valid input";
            return;
        }
    }

    telux::common::Status status = telux::common::Status::FAILED;
    EthCapability ethCapability;
    ethCapability.ethModes = ethModes;
    std::cout << " set peer Eth Capability as "<< ethModes << std::endl;
    status = dataLinkManager_->setPeerEthCapability(ethCapability);

    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to set peer Eth capability" << std::endl;
        return;
    }

}

void DataLinkMenu::setEthDataLink(std::vector<std::string> inputCommand) {
    int ethState;
    std::cout << " Set Eth data link, Enter 1 - for UP  and 0 - DOWN" << std::endl;
    std::cin >> ethState;
    Utils::validateInput(ethState, {0, 1});
    LinkState linkState;

    if (ethState == 0) {
        linkState = LinkState::DOWN;
    } else if (ethState == 1) {
        linkState = LinkState::UP;
    } else {
        std::cout << " Invalid input ..." << std::endl;
        return;
    }

    telux::common::ErrorCode errCode = telux::common::ErrorCode::GENERIC_FAILURE;
    errCode = dataLinkManager_->setEthDataLinkState(linkState);

    if (errCode != telux::common::ErrorCode::SUCCESS) {
        std::cout << " *** ERROR - Failed to set Eth datalink" << std::endl;
        return;
    }
    std::cout << " *** Set Eth datalink request sent" << std::endl;
}

void DataLinkMenu::setLocalEthOperatingMode(std::vector<std::string> inputCommand) {
    std::cout << " Set local Eth operating mode: " << std::endl;
    for (int i = 0; i <= 8; ++i) {
        telux::data::EthModeType ethModeType = static_cast<telux::data::EthModeType>(1 << i);
        std::cout << i << " - " << DataLinkListener::ethModeTypeToString(ethModeType) << std::endl;
    }

    int ethMode;
    std::cin >> ethMode;
    Utils::validateInput(ethMode, {0, 1, 2, 3, 4, 5, 6, 7, 8});
    telux::data::EthModeType ethModeType = static_cast<telux::data::EthModeType>(1 << ethMode);

    telux::common::Status status = telux::common::Status::FAILED;
    status = dataLinkManager_->setLocalEthOperatingMode(
        ethModeType, [](telux::common::ErrorCode error) {
           std::cout << " *** Set local Eth operating mode request completed"
                     << std::endl;
        }
    );
    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to set peer Eth capability" << std::endl;
        return;
    }
    std::cout << " *** Set local Eth operating mode request sent" << std::endl;
}

void DataLinkMenu::setPeerModeChangeRequestStatus(std::vector<std::string> inputCommand) {
    std::cout << " set mode change request status " << std::endl;
    std::cout << " 1. Request accepted\n 2. Request completed\n 3. Request failed\n" <<
        " 4. Request rejected" << std::endl;

    int reqStatus;
    std::cin >> reqStatus;
    Utils::validateInput(reqStatus, {1, 2, 3, 4});

    telux::data::LinkModeChangeStatus sdkEthStatus;

    switch (reqStatus) {
    case 1:
        sdkEthStatus = telux::data::LinkModeChangeStatus::ACCEPTED;
        break;
    case 2:
        sdkEthStatus = telux::data::LinkModeChangeStatus::COMPLETED;
        break;
    case 3:
        sdkEthStatus = telux::data::LinkModeChangeStatus::FAILED;
        break;
    case 4:
        sdkEthStatus = telux::data::LinkModeChangeStatus::REJECTED;
        break;
    default:
        sdkEthStatus = telux::data::LinkModeChangeStatus::UNKNOWN;
        break;
    }

    telux::common::Status status = telux::common::Status::FAILED;
    status = dataLinkManager_->setPeerModeChangeRequestStatus(sdkEthStatus);
}

void DataLinkMenu::registerListener(std::vector<std::string> inputCommand) {
    std::cout << " register data link listener " << std::endl;
    dataLinkManager_->registerListener(dataLinkListener_);
}

void DataLinkMenu::deregisterListener(std::vector<std::string> inputCommand) {
    std::cout << " deregister data link listener " << std::endl;
    dataLinkManager_->deregisterListener(dataLinkListener_);
}
