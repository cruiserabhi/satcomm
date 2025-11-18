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

#define DATA_CONTROL_MANAGER_API_JSON "api/data/IDataControlManagerSlot.json"

#define DATA_CONTROL "data_control"
#define DATA_CONTROL_SSR_FILTER "data_control_ssr"

DataControlServerImpl::DataControlServerImpl()
    : serverEvent_(ServerEventManager::getInstance())
    , clientEvent_(EventService::getInstance()) {
    LOG(DEBUG, __FUNCTION__);
}

DataControlServerImpl::~DataControlServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status DataControlServerImpl::registerDefaultIndications() {
    LOG(DEBUG, __FUNCTION__);

    auto status = serverEvent_.registerListener(shared_from_this(), DATA_CONTROL);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications with QMS failed");
        return status;
    }

    return status;
}

void DataControlServerImpl::onSSREvent(telux::common::ServiceStatus srvStatus) {
    commonStub::GetServiceStatusReply ssrResp;
    ::eventService::EventResponse anyResponse;
    setResponse(srvStatus, &ssrResp);
    anyResponse.set_filter(DATA_CONTROL_SSR_FILTER);
    anyResponse.mutable_any()->PackFrom(ssrResp);
    clientEvent_.updateEventQueue(anyResponse);
}

void DataControlServerImpl::notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
        std::string srvStatusStr) {
    LOG(DEBUG, __FUNCTION__, ":: Service status Changed to ", srvStatusStr);
    onSSREvent(srvStatus);
}

telux::common::ServiceStatus DataControlServerImpl::getServiceStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

void DataControlServerImpl::setServiceStatus(telux::common::ServiceStatus srvStatus) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (serviceStatus_ != srvStatus) {
        serviceStatus_ = srvStatus;
        std::string srvStrStatus = CommonUtils::mapServiceString(srvStatus);
        notifyServiceStateChanged(serviceStatus_, srvStrStatus);
    }
}

void DataControlServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event) {
    if (event.filter() == DATA_CONTROL) {
        onEventUpdate(event.event());
    }
}

/* Get Notification for SSR */
void DataControlServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The thermal event: ", event);
    std::string token;

    /** INPUT-event:
     * (1) ssr SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     * OUTPUT-token:
     * (1) ssr
     **/
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    /** INPUT-token:
     * (1) ssr
     * INPUT-event:
     * (1) SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     **/
    handleEvent(token, event);
}

/** INPUT-token:
  * (1) ssr
  * INPUT-event:
  * (1) SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
 */
void DataControlServerImpl::handleEvent(std::string token,std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The data control event type is: ", token,
            "The leftover string is: ", event);

    if (token == "ssr") {
        //INPUT-token: ssr
        //INPUT-event: SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
        handleSSREvent(event);
    } else {
        LOG(DEBUG, __FUNCTION__, ":: Invalid event ! Ignoring token: ",
                token, ", event: ", event);
    }
}

void DataControlServerImpl::handleSSREvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, ":: SSR event: ", eventParams);

    telux::common::ServiceStatus srvcStatus =
        telux::common::ServiceStatus::SERVICE_FAILED;
    if (eventParams == "SERVICE_AVAILABLE") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    } else if (eventParams == "SERVICE_UNAVAILABLE") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (eventParams == "SERVICE_FAILED") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    } else {
        // Ignore
        LOG(DEBUG, __FUNCTION__, ":: INVALID SSR event: ", eventParams);
        return;
    }

    setServiceStatus(srvcStatus);
}

grpc::Status DataControlServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty *request, commonStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;

    auto status = registerDefaultIndications();
    if (status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::CANCELLED,
                ":: Could not register indication with EventMgr");
    }

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
    telux::common::ServiceStatus srvcStatus = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvcStatus));

    setServiceStatus(srvcStatus);

    return setResponse(srvcStatus,response);
}

grpc::Status DataControlServerImpl::GetServiceStatus(ServerContext* context,
        const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ServiceStatus srvStatus = getServiceStatus();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));

    return setResponse(srvStatus,response);
}

grpc::Status DataControlServerImpl::setResponse(telux::common::ServiceStatus srvStatus,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;
    int subSysDelay = rootObj["IDataControlManager"]["IsSubsystemReadyDelay"].asInt();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemDelay: ", subSysDelay);

    switch(srvStatus) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_FAILED:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_FAILED);
            break;
        default:
            LOG(ERROR, __FUNCTION__, ":: Invalid service status");
            return grpc::Status(grpc::StatusCode::CANCELLED, ":: set service status failed");
    }
    response->set_delay(subSysDelay);
    return grpc::Status::OK;
}

grpc::Status DataControlServerImpl::SetDataStallParams(ServerContext* context,
    const dataStub::SetDataStallParamsRequest *request, dataStub::SetDataStallParamsReply* response) {

    LOG(DEBUG, __FUNCTION__);

    int slotId = request->slot_id();
    LOG(DEBUG, __FUNCTION__, "slotId: ", slotId);

    std::string subsystem = "IDataControlManager";
    std::string method = "setDataStallParams";

    Json::Value mgrApi;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(mgrApi, DATA_CONTROL_MANAGER_API_JSON);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    std::string errorStr = mgrApi[subsystem][method][slotId-1]["error"].asString();
    auto errCode = CommonUtils::mapErrorCode(errorStr);
    response->set_error(static_cast<commonStub::ErrorCode>(errCode));

    return grpc::Status::OK;
}
