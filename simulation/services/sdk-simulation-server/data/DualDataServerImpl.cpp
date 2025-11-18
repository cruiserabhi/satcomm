/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "DualDataServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"

#define DUAL_DATA_MANAGER_API_JSON "api/data/IDualDataManager.json"
#define DUAL_DATA_MANAGER_STATE_JSON "system-state/data/IDualDataManagerState.json"

#define DUAL_DATA_FILTER "dual_data"
#define CAPABILITY_CHANGE_EVENT "capabilityChange"
#define RECOMMENDATION_CHANGE_EVENT "recommendationChange"
#define DEFAULT_DELIMITER " "

DualDataServerImpl::DualDataServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

DualDataServerImpl::~DualDataServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status DualDataServerImpl::InitService(ServerContext* context,
    const dataStub::InitRequest* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = DUAL_DATA_MANAGER_API_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IDualDataManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IDualDataManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {DUAL_DATA_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status DualDataServerImpl::GetDualDataCapability(ServerContext* context,
    const ::google::protobuf::Empty* request, dataStub::GetDualDataCapabilityReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem = "IDualDataManager";
    std::string method = "getDualDataCapability";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        response->set_capability(data.stateRootObj[subsystem]["dppdCapability"].asBool());
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status DualDataServerImpl::GetDualDataUsageRecommendation(ServerContext* context,
    const ::google::protobuf::Empty* request,
    dataStub::GetDualDataUsageRecommendationReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem = "IDualDataManager";
    std::string method = "getDualDataUsageRecommendation";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        response->mutable_usage_recommendation()->set_recommendation(
            convertUsageRecommendationStringToEnum(
            data.stateRootObj[subsystem]["dppdUsageRecommendation"].asString()));
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

void DualDataServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == DUAL_DATA_FILTER) {
        onEventUpdate(message.event());
    }
}

void DualDataServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__,"String is ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__,"String is ", token );
    if (CAPABILITY_CHANGE_EVENT == token) {
        handleCapabilityChangeRequest(event);
    } else if(RECOMMENDATION_CHANGE_EVENT == token) {
        handleRecommendationChnageRequest(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

::dataStub::UsageRecommendation::Recommendation
    DualDataServerImpl::convertUsageRecommendationStringToEnum(
    std::string recommendation){
    LOG(DEBUG, __FUNCTION__);
    if (recommendation == "ALLOWED") {
        return ::dataStub::UsageRecommendation::ALLOWED;
    } else if (recommendation == "NOT_ALLOWED") {
        return ::dataStub::UsageRecommendation::NOT_ALLOWED;
    } else if (recommendation == "NOT_RECOMMENDED") {
        return ::dataStub::UsageRecommendation::NOT_RECOMMENDED;
    }

    return ::dataStub::UsageRecommendation::ALLOWED;
}

void DualDataServerImpl::handleCapabilityChangeRequest(std::string event) {
    LOG(DEBUG, __FUNCTION__);

    bool capability = true;
    std::string param = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    try {
        capability = (std::stoi(param) == 1)? true : false;
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        return;
    }

    std::string apiJsonPath = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem = "IDualDataManager";
    std::string method = "getDualDataCapability";
    JsonData data;
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj[subsystem]["dppdCapability"] = capability;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    ::dataStub::DualDataCapabilityEvent dualDataCapabilityEvent;
    ::eventService::EventResponse anyResponse;

    dualDataCapabilityEvent.set_capability(capability);

    anyResponse.set_filter(DUAL_DATA_FILTER);
    anyResponse.mutable_any()->PackFrom(dualDataCapabilityEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void DualDataServerImpl::handleRecommendationChnageRequest(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    std::string recommendation = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);

    std::string apiJsonPath = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem = "IDualDataManager";
    std::string method = "getDualDataUsageRecommendation";
    JsonData data;
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj[subsystem]["dppdUsageRecommendation"] = recommendation;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    ::dataStub::DualDataUsageRecommendationEvent dualDataRecommendationEvent;
    ::eventService::EventResponse anyResponse;

    dualDataRecommendationEvent.set_recommendation(recommendation);

    anyResponse.set_filter(DUAL_DATA_FILTER);
    anyResponse.mutable_any()->PackFrom(dualDataRecommendationEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}