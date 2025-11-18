/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 *
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "../../../../common/utils/Utils.hpp"
#include "../DataUtils.hpp"

#include "DataConnectionMenu.hpp"

using namespace std;

DataConnectionMenu::DataConnectionMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   subSystemStatusUpdated_ = false;
}

DataConnectionMenu::~DataConnectionMenu() {
    for (auto& conMgr : dataConnectionManagerMap_) {
        conMgr.second->deregisterListener(dataListeners_[conMgr.first]);
    }
    dataConnectionManagerMap_.clear();
    dataListeners_.clear();
}

bool DataConnectionMenu::init() {
    bool dcmSubSystemStatus = initConnectionManagerAndListener(DEFAULT_SLOT_ID);
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        dcmSubSystemStatus |= initConnectionManagerAndListener(SLOT_ID_2);
    }

    std::shared_ptr<ConsoleAppCommand> startDataCall
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "start_data_call", {},
            std::bind(&DataConnectionMenu::startDataCall, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> stopDataCall
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "stop_data_call", {},
            std::bind(&DataConnectionMenu::stopDataCall, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> reqDataCallStats
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "request_datacall_statistics",
            {}, std::bind(&DataConnectionMenu::requestDataCallStatistics, this,
            std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> resetDataCallStats
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "reset_datacall_statistics",
            {}, std::bind(&DataConnectionMenu::resetDataCallStatistics, this,
            std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> reqDataCallList
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "5", "request_datacall_list", {}, std::bind(static_cast<void(DataConnectionMenu::*)()>(
            &DataConnectionMenu::requestDataCallList), this)));
    std::shared_ptr<ConsoleAppCommand> setDefaultProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "6", "set_default_profile", {}, std::bind(
            &DataConnectionMenu::setDefaultProfile, this)));
    std::shared_ptr<ConsoleAppCommand> getDefaultProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "7", "get_default_profile", {}, std::bind(
            &DataConnectionMenu::getDefaultProfile, this)));
    std::shared_ptr<ConsoleAppCommand> reqDataCallBitRate
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "request_datacall_bit_rate",
            {}, std::bind(&DataConnectionMenu::requestDataCallBitRate, this,
            std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setRoamingMode
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "set_roaming_mode",
            {}, std::bind(&DataConnectionMenu::setRoamingMode, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestRoamingMode
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "request_roaming_mode",
            {}, std::bind(&DataConnectionMenu::requestRoamingMode, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestTrafficFlowTemplate =
        std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "11", "request_traffic_flow_template", {},
            std::bind(&DataConnectionMenu::requestTrafficFlowTemplate, this,
                      std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startDataCall_V1
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("12", "start_data_call_v1", {},
            std::bind(&DataConnectionMenu::startDataCall_V1, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> stopDataCall_V1
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("13", "stop_data_call_v1", {},
            std::bind(&DataConnectionMenu::stopDataCall_V1, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestThrottledAPNInfo
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "14", "request_throttled_apn_info", {}, std::bind(
            &DataConnectionMenu::requestThrottledApnsInfo, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {startDataCall, stopDataCall,
        reqDataCallStats, resetDataCallStats, reqDataCallList, setDefaultProfile,
        getDefaultProfile, reqDataCallBitRate, setRoamingMode, requestRoamingMode,
        requestTrafficFlowTemplate, startDataCall_V1, stopDataCall_V1, requestThrottledAPNInfo};

    addCommands(commandsList);
    return dcmSubSystemStatus;
}

bool DataConnectionMenu::displayMenu() {
    bool retVal = true;
    if ((dataConnectionManagerMap_.find(DEFAULT_SLOT_ID) != dataConnectionManagerMap_.end()) &&
        (telux::common::ServiceStatus::SERVICE_AVAILABLE ==
        dataConnectionManagerMap_[DEFAULT_SLOT_ID]->getServiceStatus())) {
        std::cout << "\nData Connection Manager on slot "<< DEFAULT_SLOT_ID <<
        " is ready" << std::endl;
    }
    else {
        std::cout << "\nData Connection Manager on slot "<< DEFAULT_SLOT_ID <<
        " is not ready" << std::endl;
        retVal = false;;
    }
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        if ((dataConnectionManagerMap_.find(SLOT_ID_2) != dataConnectionManagerMap_.end()) &&
            (telux::common::ServiceStatus::SERVICE_AVAILABLE ==
            dataConnectionManagerMap_[SLOT_ID_2]->getServiceStatus())) {
            std::cout << "\nData Connection Manager on slot "<< SLOT_ID_2 <<
            " is ready" << std::endl;
            retVal = true;
        }
        else {
            std::cout << "\nData Connection Manager on slot "<< SLOT_ID_2 <<
            " is not ready" << std::endl;
            //Intentionally did not set retVal = false to not overwrite slot 1 value
        }
    }
    ConsoleApp::displayMenu();
    return retVal;
}

bool DataConnectionMenu::initConnectionManagerAndListener(SlotId slotId){
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool retValue = false;
    subSystemStatusUpdated_ = false;
    auto initCb = std::bind(&DataConnectionMenu::onInitCompleted, this, std::placeholders::_1);
    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto conMgr = telux::data::DataFactory::getInstance().getDataConnectionManager(slotId, initCb);

    if (conMgr) {
        //If this is newly created Manager
        // Register before sub-system comes up to get all the notifications
        if (dataConnectionManagerMap_.find(slotId) == dataConnectionManagerMap_.end()) {
            dataConnectionManagerMap_.emplace(slotId, conMgr);
            auto dataListener = std::make_shared<DataListener>(slotId);
            if (dataListener == nullptr) {
                std::cout <<
                "ERROR - Unable to allocate listeners .. terminate application" << std::endl;
                exit(1);
            }
            dataListeners_.emplace(slotId, dataListener);
            dataConnectionManagerMap_[slotId]->registerListener(dataListeners_[slotId]);
        }
        // Initialize data connection manager
        std::cout << "\n\nInitializing Data connection manager subsystem on slot " <<
            slotId << ", Please wait ..." << endl;
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
        subSystemStatus = conMgr->getServiceStatus();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nData Connection Manager on slot "<< slotId << " is ready" << std::endl;
            retValue = true;
        }
        else {
            std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
            return false;
        }

        //Update dataListener_'s data call list
        requestDataCallList(OperationType::DATA_LOCAL, slotId,
            std::bind(&DataListener::initDataCallListResponseCb, dataListeners_[slotId],
            std::placeholders::_1, std::placeholders::_2));
        requestDataCallList(OperationType::DATA_REMOTE, slotId,
            std::bind(&DataListener::initDataCallListResponseCb, dataListeners_[slotId],
            std::placeholders::_1, std::placeholders::_2));
    }
    else {
        std::cout << "Data Connection Manager failed to initialize" << std::endl;
    }
    return retValue;
}

void DataConnectionMenu::onInitCompleted(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void DataConnectionMenu::startDataCall(std::vector<std::string> inputCommand) {
    std::cout << "\nStart data call" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    telux::data::DataCallParams params;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    int ipFamilyType;
    int operationType;

    getDataCallParams(params.profileId, ipFamilyType, operationType);

    int userChoice;
    std::cout << "Start data call on specific interface name? (1-Yes, 0-No): ";
    std::cin>> userChoice;
    Utils::validateInput(userChoice);
    std::cout << std::endl;
    if(userChoice) {
        std::cout << "Enter interface name: ";
        std::cin >> params.interfaceName;
        Utils::validateInput(params.interfaceName);
        std::cout << std::endl;
    }

    params.ipFamilyType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    params.operationType = static_cast<telux::data::OperationType>(operationType);

    retStat =
        dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->startDataCall(params,
        MyDataCallResponseCallback::startDataCallResponseCallBack);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::stopDataCall(std::vector<std::string> inputCommand) {
    std::cout << "\nStop data call" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    telux::data::DataCallParams params;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    int ipFamilyType;
    int operationType;

    getDataCallParams(params.profileId, ipFamilyType, operationType);

    params.ipFamilyType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    params.operationType = static_cast<telux::data::OperationType>(operationType);
    retStat =
        dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->stopDataCall(params,
        MyDataCallResponseCallback::stopDataCallResponseCallBack);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::startDataCall_V1(std::vector<std::string> inputCommand) {
    std::cout << "\nStart data call" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int profileId;
    int ipFamilyType;
    int operationType;

    getDataCallParams(profileId, ipFamilyType, operationType);

    telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    retStat =
        dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->startDataCall(profileId, ipFamType,
        MyDataCallResponseCallback::startDataCallResponseCallBack, opType);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::stopDataCall_V1(std::vector<std::string> inputCommand) {
    std::cout << "\nStop data call" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    int profileId;
    int ipFamilyType;
    int operationType;

    getDataCallParams(profileId, ipFamilyType, operationType);

    telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    retStat =
        dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->stopDataCall(profileId, ipFamType,
        MyDataCallResponseCallback::stopDataCallResponseCallBack, opType);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::getDataCallParams(int &profileId, int &ipFamilyType,
    int &operationType) {
    std::cout << "Enter Profile Id : ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType, {static_cast<int>(telux::data::IpFamilyType::IPV4),
        static_cast<int>(telux::data::IpFamilyType::IPV6),
        static_cast<int>(telux::data::IpFamilyType::IPV4V6)});

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType, {static_cast<int>(telux::data::OperationType::DATA_LOCAL),
        static_cast<int>(telux::data::OperationType::DATA_REMOTE)});
}

void DataConnectionMenu::requestDataCallStatistics(std::vector<std::string> inputCommand) {
    std::cout << "\nRequest DataCall Statistics" << std::endl;

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto dataCall = dataListeners_[static_cast<SlotId>(slotId)]->getDataCall(
        static_cast<SlotId>(slotId), profileId);
    if (dataCall) {
        dataCall->requestDataCallStatistics(
            &DataCallStatisticsResponseCb::requestStatisticsResponse);
    } else {
        std::cout << "Unable to find DataCall, Please start_data_call" << std::endl;
    }
}

void DataConnectionMenu::resetDataCallStatistics(std::vector<std::string> inputCommand) {
    std::cout << "\nReset DataCall Statistics" << std::endl;

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto dataCall = dataListeners_[static_cast<SlotId>(slotId)]->getDataCall(
        static_cast<SlotId>(slotId), profileId);
    if (dataCall) {
        dataCall->resetDataCallStatistics(&DataCallStatisticsResponseCb::resetStatisticsResponse);
    } else {
        std::cout << "Unable to find DataCall, Please start_data_call" << std::endl;
    }
}

void DataConnectionMenu::requestDataCallList(OperationType operationType,
    SlotId slotId, DataCallListResponseCb cb) {
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    if (dataConnectionManagerMap_.find(slotId) == dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }
    if (dataConnectionManagerMap_[slotId]) {
        telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
        retStat = dataConnectionManagerMap_[slotId]->requestDataCallList(opType,cb);
    }
}

void DataConnectionMenu::requestDataCallList() {
    std::cout << "\nRequest DataCall List" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType, {static_cast<int>(telux::data::OperationType::DATA_LOCAL),
        static_cast<int>(telux::data::OperationType::DATA_REMOTE)});

    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    retStat = dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->requestDataCallList(
        opType,MyDataCallResponseCallback::dataCallListResponseCb);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::setDefaultProfile() {
    std::cout << "\nSet Default Profile" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType, {static_cast<int>(telux::data::OperationType::DATA_LOCAL),
        static_cast<int>(telux::data::OperationType::DATA_REMOTE)});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);
    bool profileFound = validateProfile(slotId,profileId);
    // if profile does not exist , dont allow it to be set as default profile
    if (!profileFound) {
        std::cout << "\nCannot set "<< profileId
            << " as default profile, Profile does not exist" << std::endl;
        return;
    }

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "setDefaultProfile Response"
                    << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                    << ". ErrorCode: " << static_cast<int>(error)
                    << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->setDefaultProfile(
        opType, profileId, respCb);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::requestDataCallBitRate(std::vector<std::string> inputCommand) {
    std::cout << "\nRequest Data Call Bit Rate" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
                                        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto dataCall = dataListeners_[static_cast<SlotId>(slotId)]->getDataCall(
        static_cast<SlotId>(slotId), profileId);
    if (dataCall) {
        // Callback
        auto respCb = [](
            telux::data::BitRateInfo& bitRate, telux::common::ErrorCode error) {
            std::cout << std::endl << std::endl;
            std::cout << "CALLBACK: "
                      << "RequestDataCallBitRate Response"
                      << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                      << ". ErrorCode: " << static_cast<int>(error)
                      << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
            if (error == telux::common::ErrorCode::SUCCESS) {
                std::cout << std::endl;
                std::cout << "Maximum Tx Rate (bits/sec): " << bitRate.maxTxRate << std::endl;
                std::cout << "Maximum Rx Rate (bits/sec): " << bitRate.maxRxRate << std::endl;
            }
        };
        retStat = dataCall->requestDataCallBitRate(respCb);
    } else {
        std::cout << "Unable to find DataCall, Please start_data_call" << std::endl;
    }
    Utils::printStatus(retStat);
}

void DataConnectionMenu::getDefaultProfile() {
    std::cout << "\nGet Default Profile" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    SlotId slotId = DEFAULT_SLOT_ID;
    if (dataConnectionManagerMap_.find(slotId) == dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }
    int profileId;
    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType, {static_cast<int>(telux::data::OperationType::DATA_LOCAL),
        static_cast<int>(telux::data::OperationType::DATA_REMOTE)});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    // Callback
    auto respCb = [](int profileId, SlotId slotId, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "GetDefaultProfile Response"
                    << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                    << ". ErrorCode: " << static_cast<int>(error)
                    << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Slot Id: " << slotId << endl
                      << "Profile Id: " << profileId << endl;
        }
    };

    retStat = dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->getDefaultProfile(
        opType, respCb);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::setRoamingMode(std::vector<std::string> inputCommand) {
    std::cout << "\nSet Roaming Mode" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    bool roamEnable = false;

    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    int enableRoamFlag;
    std::cout << "Set Roaming Mode (1 - On, 0 - Off): ";
    std::cin >> enableRoamFlag;
    Utils::validateInput(enableRoamFlag, {0, 1});
    if (enableRoamFlag) {
        roamEnable = true;
    }

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType, {static_cast<int>(telux::data::OperationType::DATA_LOCAL),
        static_cast<int>(telux::data::OperationType::DATA_REMOTE)});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "setRoamingMode Response"
                    << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                    << ". ErrorCode: " << static_cast<int>(error)
                    << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    bool profileFound = validateProfile(slotId,profileId);
    if (!profileFound) {
        std::cout << "\nProfile not found with profileId: "<< profileId
        << "  and slotId: "<< slotId << std::endl;
        Utils::printStatus(telux::common::Status::INVALIDPARAM);
        return;
    }

    retStat = dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->setRoamingMode(
        roamEnable, profileId, opType, respCb);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::requestRoamingMode(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "request Roaming Mode\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType, {static_cast<int>(telux::data::OperationType::DATA_LOCAL),
        static_cast<int>(telux::data::OperationType::DATA_REMOTE)});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    auto respCb = [](bool enable, int profileId, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestRoamingMode Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Roaming mode on profile: " << profileId << " is "
                  << (enable ? "enabled" : "disabled") << "\n";
        }
    };

    bool profileFound = validateProfile(slotId,profileId);
    if (!profileFound) {
        std::cout << "\nProfile not found with profileId: "<< profileId
        << "and slotId: "<< slotId << std::endl;
        Utils::printStatus(telux::common::Status::INVALIDPARAM);
        return;
    }

    retStat = dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->requestRoamingMode(
        profileId, opType, respCb);
    Utils::printStatus(retStat);
}

bool DataConnectionMenu::validateProfile(int slotId, int profileId) {

    if (!initalizeDPM(static_cast<SlotId>(slotId))) {
        return false;
    }

    std::promise<telux::common::ErrorCode> prom;
    std::vector<std::shared_ptr<telux::data::DataProfile>> profileList{};
    std::shared_ptr<MyDefaultProfilesCallback> profileListCb  =
        std::make_shared<MyDefaultProfilesCallback>();

    if (profileListCb == nullptr) {
        std::cout << "ERROR - Unable to allocate profile list callback" << std::endl;
        return false;
    }

    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->requestProfileList(
            std::shared_ptr<telux::data::IDataProfileListCallback>(profileListCb));

    telux::common::ErrorCode errCode = profileListCb->prom_.get_future().get();
    if (errCode != telux::common::ErrorCode::SUCCESS) {
        std::cout << "\nError retriving profile list ErrorCode: " << static_cast<int>(errCode)
            << std::endl;
        return false;
    }
    for(auto it : profileListCb->profileList_) {
        if (profileId == it->getId()) {
            return true;
        }
    }
    return false;
}

bool DataConnectionMenu::initalizeDPM(SlotId slotId) {

    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool retValue = false;
    std::promise<telux::common::ServiceStatus> prom;

    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto profMgr = dataFactory.getDataProfileManager(slotId,
        [&prom](telux::common::ServiceStatus status) { prom.set_value(status); });

    if (profMgr) {
        //  Initialize data profile manager
        std::cout << "\n\nInitializing Data profile manager subsystem on slot " <<
            slotId << ", Please wait ..." << endl;
        subSystemStatus = prom.get_future().get();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nData Profile Manager on slot "<< slotId << " is ready" << std::endl;
            retValue = true;
        } else {
            std::cout << "\nData Profile Manager on slot "<< slotId << " is not ready" << std::endl;
            return false;
        }

        //If this is newly created Manager
        if (dataProfileManagerMap_.find(slotId) == dataProfileManagerMap_.end()) {
            dataProfileManagerMap_.emplace(slotId, profMgr);
        }
    } else {
        std::cout << "Data Profile Manager failed to initialize" << std::endl;
    }
    return retValue;
}

void DataConnectionMenu::requestTrafficFlowTemplate(std::vector<std::string> inputCommand) {
    std::cout << "\nRequest traffic flow template" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
                                        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType, {static_cast<int>(telux::data::IpFamilyType::IPV4),
        static_cast<int>(telux::data::IpFamilyType::IPV6),
        static_cast<int>(telux::data::IpFamilyType::IPV4V6)});
    telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);

    auto dataCall = dataListeners_[static_cast<SlotId>(slotId)]->getDataCall(
        static_cast<SlotId>(slotId), profileId);
    if (dataCall) {
        // Callback
        auto respCb = [](const std::vector<std::shared_ptr<TrafficFlowTemplate>> &tfts,
            telux::common::ErrorCode error) {
            std::cout << "\n onTFTResponse" << std::endl;

            if (error == telux::common::ErrorCode::SUCCESS) {
               for (auto tft : tfts) {
                  std::cout << " ----------------------------------------------"
                               "------------\n";
                  std::cout << " ** TFT Details **\n";
                  std::cout << " Flow State: "
                            << DataUtils::flowStateEventToString(
                                   QosFlowStateChangeEvent::ACTIVATED)
                            << std::endl;
                  DataUtils::logQosDetails(tft);
                  std::cout << " ----------------------------------------------"
                               "------------\n\n";
               }
            } else {
               std::cout << "ErrorCode: " << static_cast<int>(error)
                         << ", description: "
                         << Utils::getErrorCodeAsString(error) << std::endl;
            }
        };

        dataCall->requestTrafficFlowTemplate(ipFamType, respCb);
        Utils::printStatus(retStat);
    } else {
        std::cout << "No data call is active. Please start a data call to "
                     "request TFT info on that data call."
                  << std::endl;
    };
}

void DataConnectionMenu::requestThrottledApnsInfo() {
    std::cout << "\nRequest Throttled APN Info" << std::endl;
    telux::common::Status retStat;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataConnectionManagerMap_.find(static_cast<SlotId>(slotId)) ==
                                        dataConnectionManagerMap_.end()) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }
    retStat = dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->requestThrottledApnInfo(
        MyDataCallResponseCallback::requestThrottledApnInfoCb);
    Utils::printStatus(retStat);
}
