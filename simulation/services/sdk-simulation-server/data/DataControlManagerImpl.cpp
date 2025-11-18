/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "DataControlServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"

#define DATA_CONTROL_MANAGER_API_JSON1 "api/data/IDataControlManagerSlot1.json"
#define DATA_CONTROL_MANAGER_API_JSON2 "api/data/IDataControlManagerSlot2.json"

#define DATA_CONTROL_FILTER "dual_data"
//#define RECOMMENDATION_CHANGE_EVENT "recommendationChange"
#define DEFAULT_DELIMITER " "

DataControlServerImpl::DataControlServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

DataControlServerImpl::~DataControlServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status DataControlServerImpl::InitService(ServerContext* context,
    const dataStub::InitRequest* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = DATA_CONTROL_MANAGER_API_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IDataControlManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IDataControlManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {DATA_CONTROL_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataControlServerImpl::SetDataStallParams(ServerContext* context,
    dataStub::SetDataStallParamsReply request, dataStub::SetDataStallParamsReply* response) {

    LOG(DEBUG, __FUNCTION__);

    int slotId = static_cast<int>(request.slot_id());
    if (slotId == 1) {
        std::string apiJsonPath = DATA_CONTROL_MANAGER_API_JSON1;
    } else {
        std::string apiJsonPath = DATA_CONTROL_MANAGER_API_JSON2;
    }

    std::string subsystem = "IDataControlManager";
    std::string method = "setDataStallParams";

    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

/*
void DataControlServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == DATA_CONTROL_FILTER) {
        onEventUpdate(message.event());
    }
}

void DataControlServerImpl::onEventUpdate(std::string event) {
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


void DataControlServerImpl::handleRecommendationChnageRequest(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    std::string recommendation = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);

    std::string apiJsonPath = DATA_CONTROL_MANAGER_API_JSON;
    std::string stateJsonPath = DATA_CONTROL_MANAGER_STATE_JSON;
    std::string subsystem = "IDataControlManager";
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

    anyResponse.set_filter(DATA_CONTROL_FILTER);
    anyResponse.mutable_any()->PackFrom(dualDataRecommendationEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}
*/
