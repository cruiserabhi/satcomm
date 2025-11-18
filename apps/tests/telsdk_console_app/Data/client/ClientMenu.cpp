/*
 *  Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <telux/common/DeviceConfig.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "ClientMenu.hpp"

using namespace std;

ClientMenu::ClientMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   menuOptionsAdded_ = false;
   subSystemStatusUpdated_ = false;
}

ClientMenu::~ClientMenu() {
}

bool ClientMenu::init() {
    bool initStatus = initClientManager();
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

    //If both local and remote vlan managers fail, exit
    if (not initStatus) {
        return false;
    }
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> getDeviceDataUsageStatsCmd
            = std::make_shared<ConsoleAppCommand>(
                ConsoleAppCommand("1", "Get_Device_Data_Usage_Stats",
                {}, std::bind(&ClientMenu::getDeviceDataUsageStats, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> resetDataUsageStatsCmd
            = std::make_shared<ConsoleAppCommand>(
                ConsoleAppCommand("2", "Reset_Data_Usage_Stats",
                {}, std::bind(&ClientMenu::resetDataUsageStats, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = { getDeviceDataUsageStatsCmd,
            resetDataUsageStatsCmd };

        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

bool ClientMenu::initClientManager() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    bool retVal = false;

    auto initCb = std::bind(&ClientMenu::onInitComplete, this, std::placeholders::_1);
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto clientMgr = dataFactory.getClientManager(initCb);
    // std:: string opTypeStr = (opType == telux::data::OperationType::DATA_LOCAL)? "Local" : "Remote";
    if (clientMgr) {
        std::unique_lock<std::mutex> lck(mtx_);
        telux::common::ServiceStatus subSystemStatus = clientMgr->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing Client Manager subsystem, Please wait \n";
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = clientMgr->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nClient Manager is ready" << std::endl;
            retVal = true;
            clientManager_ = clientMgr;
            clientListener_ = std::make_shared<ClientListener>();
            telux::common::Status status = clientMgr->registerListener(clientListener_);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "Unable to register client listener" << std::endl;
            }
        }
        else {
            std::cout << "\nClient Manager is not ready" << std::endl;
        }
    }
    return retVal;
}

void ClientMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void ClientMenu::getDeviceDataUsageStats(std::vector<std::string> inputCommand) {

    std::cout << "\nGet Device Data Usage Stats" << std::endl;
    telux::common::ErrorCode retStat = telux::common::ErrorCode::SUCCESS;

    std::vector<DeviceDataUsage> devicesDataUsage;
    retStat = clientManager_->getDeviceDataUsageStats(devicesDataUsage);

    if (retStat == telux::common::ErrorCode::SUCCESS) {
        std::cout << " RESPONSE: getDeviceDataUsageStats is successful" << std::endl;
        for (const telux::data::DeviceDataUsage x : devicesDataUsage) {
            std::cout <<"macAddress: "<< x.macAddress << std::endl;
            std::cout <<"bytesRx: "<< x.usage.bytesRx << std::endl;
            std::cout <<"bytesTx: "<< x.usage.bytesTx << std::endl << std::endl;
        }
    } else {
        std::cout << " RESPONSE: getDeviceDataUsageStats failed"
            << ", ErrorCode: " << static_cast<int>(retStat)
            << ", description: " << Utils::getErrorCodeAsString(retStat) << std::endl;
    }
}

void ClientMenu::resetDataUsageStats(std::vector<std::string> inputCommand) {
    std::cout << "\nReset device data usage stats" << std::endl;
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;

    error = clientManager_->resetDataUsageStats();
    if (error == telux::common::ErrorCode::SUCCESS) {
        std::cout << " RESPONSE: resetDataUsageStats is successful" << std::endl;
    } else {
        std::cout << " RESPONSE: resetDataUsageStats failed"
            << ", ErrorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}
