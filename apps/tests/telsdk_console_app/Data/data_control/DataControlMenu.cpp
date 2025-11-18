/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <sstream>

extern "C" {
#include "unistd.h"
}

#include "DataControlMenu.hpp"
#include <telux/common/DeviceConfig.hpp>
#include <telux/data/DataFactory.hpp>
#include "Utils.hpp"

DataControlMenu::DataControlMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

DataControlMenu::~DataControlMenu() {
}

bool DataControlMenu::init() {
    bool initStatus = initDataControlManager();

    if (not initStatus) {
        return false;
    }
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> setDataStallParamsCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "set_data_stall_params",
                        {}, std::bind(&DataControlMenu::setDataStallParams, this,
                            std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
            = {setDataStallParamsCommand};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

bool DataControlMenu::initDataControlManager() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;

    bool retVal = false;
    auto initCb = std::bind(&DataControlMenu::onInitComplete, this, std::placeholders::_1);
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto dataControl = dataFactory.getDataControlManager(initCb);

    if (dataControl) {
        dataControl->registerListener(shared_from_this());
        std::unique_lock<std::mutex> lck(mtx_);

        telux::common::ServiceStatus subSystemStatus = dataControl->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing "
                      << " DataControl Manager subsystem, Please wait \n";
            cv_.wait(lck, [this] { return this->subSystemStatusUpdated_; });
            subSystemStatus = dataControl->getServiceStatus();
        }

        // At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\n"
                      << " DataControl Manager is ready" << std::endl;
            retVal = true;
            dataControlManager_ = dataControl;
        } else {
            std::cout << "\n"
                      << " DataControl Manager is not ready" << std::endl;
        }
    }
    return retVal;
}

void DataControlMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void DataControlMenu::setDataStallParams(
    std::vector<std::string> &inputCommand) {
    std::cout << "setting data stall parameters" << std::endl;

    int slotId = DEFAULT_SLOT_ID, trafficDir=0, appType =
        static_cast<int>(telux::data::ApplicationType::UNSPECIFIED);
    bool dataStallStatus = false;

    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    auto slotID = static_cast<SlotId>(slotId);
    telux::data::DataStallParams params;

    std::cout << "Enter data stall direction: (1-UPLINK, 2-DOWNLINK)" << std::endl;
    std::cin >> trafficDir;
    std::cout << std::endl;
    Utils::validateInput(trafficDir, {1,2});
    params.trafficDir = static_cast<telux::data::Direction>(trafficDir);

    std::cout << "Enter application type: (0-UNSPECIFIED, 1-CONV_AUDIO, 2-CONV_VIDEO,"
        << "3-STREAMING_AUDIO, 4-STREAMING_VIDEO, 5-TYPE_GAMING, 6-WEB_BROWSING, 7-FILE_TRANSFER)"
        << std::endl;
    std::cin >> appType;
    std::cout << std::endl;
    Utils::validateInput(appType, {0,1,2,3,4,5,6,7});
    params.appType = static_cast<telux::data::ApplicationType>(appType);

    std::cout << "Enter data stall status: (0-FALSE, 1-TRUE)" << std::endl;
    std::cin >> dataStallStatus;
    std::cout << std::endl;
    Utils::validateInput(dataStallStatus);
    params.dataStall = dataStallStatus;

    telux::common::ErrorCode errorCode =
        dataControlManager_->setDataStallParams(slotID, params);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "\nSet data stall params succeed" << std::endl;
    } else {
        std::cout << "\nSet data stall params failed, err: "
                  << Utils::getErrorCodeAsString(errorCode) << std::endl;
    }
}
