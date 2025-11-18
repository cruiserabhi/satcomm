/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "AntennaManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#define ANTENNA_MANAGER_API_JSON "api/platform/IAntennaManager.json"
#define ANTENNA_MANAGER_FILTER "antenna_manager"
#define DEFAULT_DELIMITER " "

AntennaManagerServerImpl::AntennaManagerServerImpl()
    : serverEvent_(ServerEventManager::getInstance())
    , clientEvent_(EventService::getInstance()) {
    LOG(DEBUG, __FUNCTION__);
}

AntennaManagerServerImpl::~AntennaManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
}

telux::common::Status AntennaManagerServerImpl::registerDefaultIndications() {
    LOG(DEBUG, __FUNCTION__);

    auto status = serverEvent_.registerListener(shared_from_this(), ANTENNA_MANAGER_FILTER);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }

    return status;
}

void AntennaManagerServerImpl::notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
        std::string srvStatusStr) {
    LOG(DEBUG, __FUNCTION__, ":: Service status Changed to ", srvStatusStr);
    onSSREvent(srvStatus);
}

telux::common::ServiceStatus AntennaManagerServerImpl::getServiceStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

void AntennaManagerServerImpl::setServiceStatus(telux::common::ServiceStatus srvStatus) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (serviceStatus_ != srvStatus) {
        serviceStatus_ = srvStatus;
        std::string srvStrStatus = CommonUtils::mapServiceString(srvStatus);
        notifyServiceStateChanged(serviceStatus_, srvStrStatus);
    }
}

void AntennaManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event) {
    if (event.filter() == ANTENNA_MANAGER_FILTER) {
        onEventUpdate(event.event());
    }
}

/* Get Notification for SSR*/
void AntennaManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The antenna manager event: ", event);
    std::string token;

    /** INPUT-event:
     * ssr SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     * OUTPUT-token:
     * ssr
     **/
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    /** INPUT-token:
     * ssr
     * INPUT-event:
     * SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     **/
    handleEvent(token, event);
}

/** INPUT-token:
  * ssr
  * INPUT-event:
  * SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
 */
void AntennaManagerServerImpl::handleEvent(std::string token,std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The antenna event type is: ", token,
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

void AntennaManagerServerImpl::handleSSREvent(std::string eventParams) {
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

grpc::Status AntennaManagerServerImpl::setResponse(telux::common::ServiceStatus srvStatus,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG,__FUNCTION__);

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

    response->set_delay(cbDelay_);

    return grpc::Status::OK;
}

void AntennaManagerServerImpl::onSSREvent(telux::common::ServiceStatus srvStatus) {
    LOG(DEBUG,__FUNCTION__);

    commonStub::GetServiceStatusReply ssrResp;
    ::eventService::EventResponse anyResponse;

    setResponse(srvStatus, &ssrResp);

    anyResponse.set_filter(ANTENNA_MANAGER_FILTER);
    anyResponse.mutable_any()->PackFrom(ssrResp);
    clientEvent_.updateEventQueue(anyResponse);
}

grpc::Status AntennaManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG,__FUNCTION__);

    telux::common::Status status = telux::common::Status::SUCCESS;
    telux::common::ServiceStatus srvStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;

    status = registerDefaultIndications();
    if (status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::CANCELLED,
                ":: Could not register indication with EventMgr");
    }

    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, ANTENNA_MANAGER_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        std::lock_guard<std::mutex> lock(mutex_);
        cbDelay_ = rootNode["IAntennaManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["IAntennaManager"]["IsSubsystemReady"].asString();
        srvStatus = CommonUtils::mapServiceStatus(cbStatus);
    } else {
        LOG(ERROR, "Unable to read AntennaManager JSON");
    }

    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));
    setServiceStatus(srvStatus);
    response->set_delay(cbDelay_);

    return setResponse(srvStatus,response);
}

grpc::Status AntennaManagerServerImpl::GetServiceStatus(ServerContext* context,
        const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ServiceStatus srvStatus = getServiceStatus();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));

    return setResponse(srvStatus,response);
}

grpc::Status AntennaManagerServerImpl::SetActiveAntenna(ServerContext* context,
    const google::protobuf::Empty* request, platformStub::DefaultReply* response) {
    LOG(DEBUG,__FUNCTION__);

    apiJsonReader("SetActiveAntenna", response);
    return grpc::Status::OK;
}

grpc::Status AntennaManagerServerImpl::GetActiveAntenna(ServerContext* context,
    const google::protobuf::Empty* request, platformStub::DefaultReply* response) {
    LOG(DEBUG,__FUNCTION__);

    apiJsonReader("GetActiveAntenna", response);
    return grpc::Status::OK;
}

void AntennaManagerServerImpl::apiJsonReader(
    std::string apiName, platformStub::DefaultReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, ANTENNA_MANAGER_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;

    CommonUtils::getValues(rootNode, "IAntennaManager", apiName, status, errorCode, cbDelay);

    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
}