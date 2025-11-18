/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "NetworkSelectionManagerServerImpl.hpp"

#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include <telux/common/DeviceConfig.hpp>
#include "TelUtil.hpp"

#define JSON_PATH1 "api/tel/INetworkSelectionManagerSlot1.json"
#define JSON_PATH2 "api/tel/INetworkSelectionManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/INetworkSelectionManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/INetworkSelectionManagerStateSlot2.json"
#define MANAGER "INetworkSelectionManager"
#define SLOT_1 1
#define SLOT_2 2

#define NETWORK_SELECTION_EVENT_SELECTION_MODE_CHANGE          "selectionModeUpdate"
#define NETWORK_SELECTION_EVENT_NETWORK_SCAN_RESULTS_CHANGE    "networkScanResultsUpdate"

#define NETWORK_SCAN_RESULTS_OPERATOR_INFO_START_INDEX    2

NetworkSelectionManagerServerImpl::NetworkSelectionManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

NetworkSelectionManagerServerImpl::~NetworkSelectionManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

grpc::Status NetworkSelectionManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj[MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_NETWORK_SELECTION_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    std::string srvStatus = rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::RequestNetworkSelectionMode(ServerContext* context,
    const ::telStub::RequestNetworkSelectionModeRequest* request,
    telStub::RequestNetworkSelectionModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestNetworkSelectionMode";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int mode =  data.stateRootObj[MANAGER]["NetworkSelectionMode"]\
            ["networkSelectionMode"].asInt();
        std::string mcc = data.stateRootObj[MANAGER]["NetworkSelectionMode"]["mcc"].asString();
        std::string mnc = data.stateRootObj[MANAGER]["NetworkSelectionMode"]["mnc"].asString();
        response->set_mcc(mcc);
        response->set_mnc(mnc);
        response->set_mode(static_cast<telStub::NetworkSelectionMode_Mode>(mode));
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::SetNetworkSelectionMode(ServerContext* context,
    const ::telStub::SetNetworkSelectionModeRequest* request,
    telStub::SetNetworkSelectionModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "setNetworkSelectionMode";
    JsonData data;
    ::telStub::SelectionModeChangeEvent selectionModeEvent;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        std::string mcc = request->mcc();
        std::string mnc = request->mnc();
        int mode = static_cast<int>(request->mode());
        data.stateRootObj[MANAGER]["NetworkSelectionMode"]["networkSelectionMode"] = mode;
        data.stateRootObj[MANAGER]["NetworkSelectionMode"]["mcc"] = mcc;
        data.stateRootObj[MANAGER]["NetworkSelectionMode"]["mnc"] = mnc;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);

        selectionModeEvent.set_phone_id(request->phone_id());
        selectionModeEvent.set_mode(static_cast<telStub::NetworkSelectionMode_Mode>(mode));
        selectionModeEvent.set_mcc(mcc);
        selectionModeEvent.set_mnc(mnc);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    auto f = std::async(std::launch::async, [this, selectionModeEvent]() {
        this->triggerNetworkSelectionModeEvent(selectionModeEvent);
    }).share();
    taskQ_->add(f);
    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::SetPreferredNetworks(ServerContext* context,
    const ::telStub::SetPreferredNetworksRequest* request,
    telStub::SetPreferredNetworksReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "setPreferredNetworks";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        std::vector<telux::tel::PreferredNetworkInfo> prefNwInfos;
        for(auto nwInfo: request->preferred_networks_info()) {
            prefNwInfos.push_back(parsePreferredNetworkInfo(nwInfo));
        }
        setPreferredNetworks(request->phone_id(), prefNwInfos, request->clear_previous());
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

telux::tel::PreferredNetworkInfo NetworkSelectionManagerServerImpl::parsePreferredNetworkInfo(
    telStub::PreferredNetworkInfo input) {
    telux::tel::PreferredNetworkInfo nwInfo;
    nwInfo.mcc = input.mcc();
    nwInfo.mnc = input.mnc();
    int size = (input.types()).size();
    for(int i = 0 ; i < size ; i++) {
        nwInfo.ratMask.set(static_cast<int>(input.types(i)));
    }
    return nwInfo;
}

void NetworkSelectionManagerServerImpl::setPreferredNetworks(int phoneId,
    std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo,
    bool clearPrevPreferredNetworks) {
    std::string apiJsonPath = (phoneId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    JsonData rootObj;
    std::string subsystem = MANAGER;
    std::string method = "setPreferredNetworks";
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, rootObj);
    int size = preferredNetworksInfo.size();
    if(clearPrevPreferredNetworks) {
        Json::Value newRoot;
        rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"] = newRoot["PreferredNetworksInfo"];
        JsonParser::writeToJsonFile(rootObj.stateRootObj, stateJsonPath);
        for (int j = 0; j < size; j++) {
            rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][j]["mcc"] =
                preferredNetworksInfo[j].mcc;
            rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][j]["mnc"] =
                preferredNetworksInfo[j].mnc;
            std::vector<uint8_t> data;
            int dataSize = preferredNetworksInfo[j].ratMask.size();
            for (int k = 0; k < dataSize; k++) {
                if(preferredNetworksInfo[j].ratMask.test(k)) {
                    data.emplace_back(k);
                }
            }
            std::string input = CommonUtils::convertVectorToString(data, false);
            rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][j]["ratTypes"] = input;
            JsonParser::writeToJsonFile(rootObj.stateRootObj, stateJsonPath);
        }
    } else {
        Json::Value newData;
        int currentCount = rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"].size();
        LOG(DEBUG, __FUNCTION__,"Current Count is : ", currentCount);
        for (int i = 0 ; i < size; i++) {
            newData[i]["mcc"] = preferredNetworksInfo[i].mcc;
            newData[i]["mnc"] = preferredNetworksInfo[i].mnc;
            std::vector<uint8_t> data;
            int dataSize = preferredNetworksInfo[i].ratMask.size();
            for (int k = 0; k < dataSize; k++) {
                if(preferredNetworksInfo[i].ratMask.test(k)) {
                    data.emplace_back(k);
                }
            }
            std::string input = CommonUtils::convertVectorToString(data, false);
            newData[i]["ratTypes"] = input;
            rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][currentCount + i] = newData[i];
            JsonParser::writeToJsonFile(rootObj.stateRootObj, stateJsonPath);
        }
        sortDatabase(phoneId, newData, size);
    }
}

void NetworkSelectionManagerServerImpl::sortDatabase(int phoneId, Json::Value newData, int index) {
    LOG(DEBUG, __FUNCTION__,"Index is : ", index);
    std::string apiJsonPath = (phoneId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    JsonData rootObj;
    std::string subsystem = MANAGER;
    std::string method = "setPreferredNetworks";
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, rootObj);
    int currentCount = rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"].size();
    LOG(DEBUG, __FUNCTION__,"Current count is : ", currentCount);
    for (int i = currentCount - index - 1; i >= 0 ; --i ) {
        rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][i + index] =
        rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][i];
    }
    for (int i = 0; i < index ; i++ ) {
        rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][i] = newData[i];
    }
    JsonParser::writeToJsonFile(rootObj.stateRootObj, stateJsonPath);
}

grpc::Status NetworkSelectionManagerServerImpl::RequestPreferredNetworks(ServerContext* context,
    const ::telStub::RequestPreferredNetworksRequest* request,
    telStub::RequestPreferredNetworksReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestPreferredNetworks";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo;
        std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo;
        requestPreferredNetworks(request->phone_id(), preferredNetworks3gppInfo,
        staticPreferredNetworksInfo);
        for(auto prefNwInfo : preferredNetworks3gppInfo) {
            telStub::PreferredNetworkInfo *nwInfo = response->add_preferred();
            createPreferredNetworkInfo(prefNwInfo, nwInfo);
        }
        // Set static network info
        for(auto staticNwInfo : staticPreferredNetworksInfo) {
            telStub::PreferredNetworkInfo *nwInfo = response->add_static_preferred();
            createPreferredNetworkInfo(staticNwInfo, nwInfo);
        }
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void NetworkSelectionManagerServerImpl::requestPreferredNetworks(int phoneId,
    std::vector<telux::tel::PreferredNetworkInfo>& preferredNetworks3gppInfo,
    std::vector<telux::tel::PreferredNetworkInfo>& staticPreferredNetworksInfo) {
    std::string apiJsonPath = (phoneId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    JsonData rootObj;
    std::string subsystem = MANAGER;
    std::string method = "requestPreferredNetworks";
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, rootObj);
    for (int i = 0 ; i < 2 ; i++) { // To fill the data of two JSON objects
        std::string networkInfoType = "";
        if(i == 0) {
            networkInfoType = "PreferredNetworksInfo";
        } else {
            networkInfoType = "PreferredNetworksStaticInfo";
        }
        int size = rootObj.stateRootObj[MANAGER][networkInfoType].size();
        for (int j = 0; j < size; j++) {
            telux::tel::PreferredNetworkInfo info;
            std::vector<int> data;
            std::string input = rootObj.stateRootObj[MANAGER][networkInfoType]\
                [j]["ratTypes"].asString();
            info.mcc = rootObj.stateRootObj[MANAGER][networkInfoType][j]["mcc"].asInt();
            info.mnc = rootObj.stateRootObj[MANAGER][networkInfoType][j]["mnc"].asInt();
            data = CommonUtils::convertStringToVector(input);
            int dataSize = data.size();
            for (int k = 0; k < dataSize; k++) {
                info.ratMask.set(data[k]);
            }
            if(i == 0) {
                preferredNetworks3gppInfo.emplace_back(info);
            } else {
                staticPreferredNetworksInfo.emplace_back(info);
            }
        }
    }
    }

void NetworkSelectionManagerServerImpl::createPreferredNetworkInfo(
    telux::tel::PreferredNetworkInfo input, telStub::PreferredNetworkInfo* output) {
    output->set_mcc(input.mcc);
    output->set_mnc(input.mnc);
    int size = input.ratMask.size();
    // Converting RATMask bitset to vector.
    for (int i = 0; i < size ; i++) {
        if (input.ratMask.test(i)) {
            output->add_types(static_cast<telStub::RatType_Type>(i));
        }
    }
}

::telStub::RadioTechnology NetworkSelectionManagerServerImpl::converRatTypeToRadioTechnology
    (::telStub::RatType_Type rat) {
    switch(rat) {
        case ::telStub::RatType::UMTS:
           return ::telStub::RadioTechnology::RADIO_TECH_UMTS;
        case ::telStub::RatType::LTE:
           return ::telStub::RadioTechnology::RADIO_TECH_LTE;
        case ::telStub::RatType::GSM:
           return ::telStub::RadioTechnology::RADIO_TECH_EDGE;
        case ::telStub::RatType::NR5G:
           return ::telStub::RadioTechnology::RADIO_TECH_NR5G;
        default:
           return ::telStub::RadioTechnology::RADIO_TECH_UNKNOWN;
    }
}

::telStub::RadioTechnology NetworkSelectionManagerServerImpl::converRatPrefTypeToRadioTechnology
    (::telStub::RatPrefType rat) {
    switch(rat) {
        case ::telStub::RatPrefType::PREF_WCDMA:
           return ::telStub::RadioTechnology::RADIO_TECH_UMTS;
        case ::telStub::RatPrefType::PREF_LTE:
           return ::telStub::RadioTechnology::RADIO_TECH_LTE;
        case ::telStub::RatPrefType::PREF_GSM:
           return ::telStub::RadioTechnology::RADIO_TECH_EDGE;
        case ::telStub::RatPrefType::PREF_TDSCDMA:
           return ::telStub::RadioTechnology::RADIO_TECH_TD_SCDMA;
        case ::telStub::RatPrefType::PREF_NR5G:
        case ::telStub::RatPrefType::PREF_NR5G_NSA:
        case ::telStub::RatPrefType::PREF_NR5G_SA:
           return ::telStub::RadioTechnology::RADIO_TECH_NR5G;
        default:
           return ::telStub::RadioTechnology::RADIO_TECH_UNKNOWN;
    }
}

grpc::Status NetworkSelectionManagerServerImpl::PerformNetworkScan(ServerContext* context,
    const ::telStub::PerformNetworkScanRequest* request,
    telStub::PerformNetworkScanReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string apiJsonPath = (phoneId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "performNetworkScan";
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);
    std::vector<telStub::OperatorInfo> infos = {};
    std::string OperatorInfosType = "";
    std::vector<int> ratData;
    ::telStub::NetworkScanResultsChangeEvent networkScanResultsEvent;

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if(data.status == telux::common::Status::SUCCESS) {
        int size = 0;
        int rat = 0;
        if(request->scan_type() == telStub::NetworkScanType::CURRENT_RAT_PREFERENCE) {
            if(telux::tel::TelUtil::readRatPreferenceFromJsonFile(phoneId, ratData)
                != ErrorCode::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
                return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
            }
            OperatorInfosType = "NetworkScanResultsForSpecRats";
            size = ratData.size();
        } else if(request->scan_type() == telStub::NetworkScanType::USER_SPECIFIED_RAT) {
            OperatorInfosType = "NetworkScanResultsForSpecRats";
            size = request->rat_types().size();
        } else {
            OperatorInfosType = "NetworkScanResultsForAllRats";
            size = data.stateRootObj[MANAGER][OperatorInfosType].size();
        }
        for (int j = 0; j < size; j++) {
            Json::Value config;
            if(request->scan_type() == telStub::NetworkScanType::CURRENT_RAT_PREFERENCE) {
                config = data.stateRootObj[MANAGER][OperatorInfosType];
                rat = static_cast<int>(converRatPrefTypeToRadioTechnology(
                    static_cast<telStub::RatPrefType>(ratData[j])));
            } else if(request->scan_type() == telStub::NetworkScanType::USER_SPECIFIED_RAT) {
                config = data.stateRootObj[MANAGER][OperatorInfosType];
                rat = static_cast<int>(converRatTypeToRadioTechnology(request->rat_types(j)));
            } else {
                config = data.stateRootObj[MANAGER][OperatorInfosType][j];
                rat = config["rat"].asInt();
            }
            std::string operatorName = config["networkName"].asString();
            std::string mcc = config["mcc"].asString();
            std::string mnc = config["mnc"].asString();
            int inUseStatus = config["inUse"].asInt();
            int roamingStatus = config["roaming"].asInt();
            int forbiddenStatus = config["forbidden"].asInt();
            int preferredStatus = config["preferred"].asInt();

            telStub::OperatorInfo *result = networkScanResultsEvent.add_operator_infos();
            result->set_name(operatorName);
            result->set_mcc(mcc);
            result->set_mnc(mnc);
            result->set_rat(static_cast<telStub::RadioTechnology>(rat));
            result->mutable_operator_status()->set_inuse
                (static_cast<telStub::InUseStatus_Status>(inUseStatus));
            result->mutable_operator_status()->set_roaming
                (static_cast<telStub::RoamingStatus_Status>(roamingStatus));
            result->mutable_operator_status()->set_forbidden
                (static_cast<telStub::ForbiddenStatus_Status>(forbiddenStatus));
            result->mutable_operator_status()->set_preferred
                (static_cast<telStub::PreferredStatus_Status>(preferredStatus));
        }
    }
    networkScanResultsEvent.set_phone_id(phoneId);
    networkScanResultsEvent.set_status(telStub::NetworkScanStatus::COMPLETE);

    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    auto f = std::async(std::launch::async, [this, networkScanResultsEvent]() {
        this->triggerNetworkScanResultsEvent(networkScanResultsEvent);
    }).share();
    taskQ_->add(f);

    return grpc::Status::OK;
}

void NetworkSelectionManagerServerImpl::triggerNetworkScanResultsEvent(
    ::telStub::NetworkScanResultsChangeEvent event) {
    ::eventService::EventResponse anyResponse;

    anyResponse.set_filter(telux::tel::TEL_NETWORK_SELECTION_FILTER);
    anyResponse.mutable_any()->PackFrom(event);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void NetworkSelectionManagerServerImpl::handleSelectionModeChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    Json::Value rootObj;
    std::string jsonfilename;
    ::telStub::SelectionModeChangeEvent selectionModeEvent;
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }
        if(phoneId == SLOT_2) {
            if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
                LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
                return;
            }
        }
        // Read string to get selection mode
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int selectionMode = std::stoi(token);
        if (selectionMode < (static_cast<int>(telStub::NetworkSelectionMode::UNKNOWN)) ||
            selectionMode > (static_cast<int>(telStub::NetworkSelectionMode::MANUAL))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for selection mode");
            return;
        }

        // Read string to get MCC and MNC
        std::string mcc = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        std::string mnc = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);

        jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return;
        }
        rootObj[MANAGER]["NetworkSelectionMode"]["networkSelectionMode"] = selectionMode;
        rootObj[MANAGER]["NetworkSelectionMode"]["mcc"] = mcc;
        rootObj[MANAGER]["NetworkSelectionMode"]["mnc"] = mnc;
        selectionModeEvent.set_phone_id(phoneId);
        selectionModeEvent.set_mode(static_cast<telStub::NetworkSelectionMode_Mode>(selectionMode));
        selectionModeEvent.set_mcc(mcc);
        selectionModeEvent.set_mnc(mnc);
        LOG(DEBUG, __FUNCTION__, " selectionMode: ", selectionMode, " MCC: ", mcc, " MNC: ", mnc);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }

    if (JsonParser::writeToJsonFile(rootObj, jsonfilename) == telux::common::ErrorCode::SUCCESS) {
        auto f = std::async(std::launch::async, [this, selectionModeEvent]() {
            this->triggerNetworkSelectionModeEvent(selectionModeEvent);
        }).share();
        taskQ_->add(f);
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write selection mode");
    }
}

void NetworkSelectionManagerServerImpl::handleNetworkScanResultsChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    Json::Value rootObj;
    std::string jsonfilename;
    ::telStub::NetworkScanResultsChangeEvent networkScanResultsEvent;

    // Split the event string into parameters (phoneId ,scanStatus ,operatorInfo1 ,operatorInfo2...)
    // based on delimeter as ","
    std::stringstream ss(eventParams);
    std::vector<string> params;
    while (getline(ss, eventParams, ',')) {
        params.emplace_back(eventParams);
    }
    for(std::string str:params) {
        LOG(DEBUG, __FUNCTION__," Param: ", str);
    }

    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(params[0], DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }
        if(phoneId == SLOT_2) {
            if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
                LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
                return;
            }
        }
        // Read string to get scan status
        token = EventParserUtil::getNextToken(params[1], DEFAULT_DELIMITER);
        int scanStatus = std::stoi(token);
        if (scanStatus < (static_cast<int>(telStub::NetworkScanStatus::COMPLETE)) ||
            scanStatus > (static_cast<int>(telStub::NetworkScanStatus::FAILED))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for scan status");
            return;
        }
        networkScanResultsEvent.set_phone_id(phoneId);
        networkScanResultsEvent.set_status
            (static_cast<telStub::NetworkScanStatus>(scanStatus));

        // Read string to get operator info
        int infoCount = params.size() - 1;
        for (int i = NETWORK_SCAN_RESULTS_OPERATOR_INFO_START_INDEX; i <= infoCount; i++) {
            LOG(DEBUG, " Parsing Params:" , params[i]);
            std::string operatorName = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            LOG(DEBUG, __FUNCTION__,  " operatorName is: ", operatorName, " MCC is: ", mcc,
                " MNC is: ", mnc);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int rat = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Rat is: ", rat);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int inUseStatus = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " InUseStatus is: ", inUseStatus);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int roamingStatus = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " RoamingStatus is: ", roamingStatus);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int forbiddenStatus = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " ForbiddenStatus is: ", forbiddenStatus);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int  preferredStatus = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " PreferredStatus is: ", preferredStatus);

            telStub::OperatorInfo *result = networkScanResultsEvent.add_operator_infos();
            result->set_name(operatorName);
            result->set_mcc(mcc);
            result->set_mnc(mnc);
            result->set_rat(static_cast<telStub::RadioTechnology>(rat));
            result->mutable_operator_status()->set_inuse
                (static_cast<telStub::InUseStatus_Status>(inUseStatus));
            result->mutable_operator_status()->set_roaming
                (static_cast<telStub::RoamingStatus_Status>(roamingStatus));
            result->mutable_operator_status()->set_forbidden
                (static_cast<telStub::ForbiddenStatus_Status>(forbiddenStatus));
            result->mutable_operator_status()->set_preferred
                (static_cast<telStub::PreferredStatus_Status>(preferredStatus));
        }
        triggerNetworkScanResultsEvent(networkScanResultsEvent);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }
}

grpc::Status NetworkSelectionManagerServerImpl::SetLteDubiousCell(ServerContext* context,
    const ::telStub::SetLteDubiousCellRequest* request,
    ::telStub::SetLteDubiousCellReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string subsystem = MANAGER;
    std::string method = "setLteDubiousCell";
    JsonData data;
    Json::Value rootObj;

    auto error =
        JsonParser::readFromJsonFile(rootObj, apiJsonPath);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    std::string errStr = rootObj[subsystem][method]["error"].asString();
    auto errCode = CommonUtils::mapErrorCode(errStr);
    response->set_error(static_cast<commonStub::ErrorCode>(errCode));

    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::SetNrDubiousCell(ServerContext* context,
    const ::telStub::SetNrDubiousCellRequest* request, ::telStub::SetNrDubiousCellReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string subsystem = MANAGER;
    std::string method = "setNrDubiousCell";
    JsonData data;
    Json::Value rootObj;

    auto error =
        JsonParser::readFromJsonFile(rootObj, apiJsonPath);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    std::string errStr = rootObj[subsystem][method]["error"].asString();
    auto errCode = CommonUtils::mapErrorCode(errStr);
    response->set_error(static_cast<commonStub::ErrorCode>(errCode));

    return grpc::Status::OK;
}

void NetworkSelectionManagerServerImpl::triggerNetworkSelectionModeEvent(
    ::telStub::SelectionModeChangeEvent event) {
    ::eventService::EventResponse anyResponse;

    anyResponse.set_filter(telux::tel::TEL_NETWORK_SELECTION_FILTER);
    anyResponse.mutable_any()->PackFrom(event);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void NetworkSelectionManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == telux::tel::TEL_NETWORK_SELECTION_FILTER) {
        std::string event = message.event();
        onEventUpdate(event);
    }
}

void NetworkSelectionManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__," Event: ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__," Token: ", token );
    if (NETWORK_SELECTION_EVENT_SELECTION_MODE_CHANGE == token) {
        handleSelectionModeChanged(event);
    } else if (NETWORK_SELECTION_EVENT_NETWORK_SCAN_RESULTS_CHANGE == token) {
        handleNetworkScanResultsChanged(event);
    } else {
        LOG(ERROR, __FUNCTION__, " Event not supported");
    }
}
