/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       Cv2xThrottleManagerServerImpl.cpp
 *
 *
 */

#include "Cv2xThrottleManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "event/EventService.hpp"
#include "Cv2xHelperServer.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#define DEFAULT_DELIMITER " "

static const std::string CV2X_THROTTLE_MGR_API_JSON = "api/cv2x/ICv2xThrottleManager.json";
static const std::string CV2X_THROTTLE_MGR_NODE = "ICv2xThrottleManager";

static const std::string CV2X_THROTTLE_FILTER = "throttle_mgr";
static const std::string CV2X_THROTTLE_EVENT_FILTER_UPDATE = "filter_update";
static const std::string CV2X_THROTTLE_EVENT_SANITY_UPDATE = "sanity_update";

Cv2xThrottleManagerServerImpl::Cv2xThrottleManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
}

Cv2xThrottleManagerServerImpl::~Cv2xThrottleManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
}

grpc::Status
Cv2xThrottleManagerServerImpl::initService(ServerContext *context,
                                           const google::protobuf::Empty *request,
                                           cv2xStub::GetServiceStatusReply *res) {
  LOG(DEBUG, __FUNCTION__);
  int cbDelay = 100;
  Json::Value rootNode;
  telux::common::ErrorCode errorCode =
      JsonParser::readFromJsonFile(rootNode, CV2X_THROTTLE_MGR_API_JSON);
  if (errorCode == ErrorCode::SUCCESS) {
    cbDelay = rootNode[CV2X_THROTTLE_MGR_NODE]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootNode[CV2X_THROTTLE_MGR_NODE]["IsSubsystemReady"].asString();
    serviceStatus_ = CommonUtils::mapServiceStatus(cbStatus);

    std::vector<std::string> filters = {CV2X_THROTTLE_FILTER};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.registerListener(shared_from_this(), filters);

  } else {
    LOG(ERROR, "Unable to read Cv2xThrottleManager JSON");
  }
  res->set_status(static_cast<::commonStub::ServiceStatus>(serviceStatus_));
  res->set_delay(cbDelay);
  taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
  return grpc::Status::OK;
}

grpc::Status
Cv2xThrottleManagerServerImpl::getServiceStatus(ServerContext *context,
                                                const google::protobuf::Empty *request,
                                                cv2xStub::GetServiceStatusReply *res) {
  LOG(DEBUG, __FUNCTION__);
  int cbDelay = 100;

  res->set_status(static_cast<::commonStub::ServiceStatus>(serviceStatus_));
  res->set_delay(cbDelay);
  return grpc::Status::OK;
}

grpc::Status
Cv2xThrottleManagerServerImpl::setVerificationLoad(ServerContext *context,
                                                   const cv2xStub::UintNum *request,
                                                   cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);

  Cv2xServerUtil::apiJsonReader(CV2X_THROTTLE_MGR_API_JSON,
                                CV2X_THROTTLE_MGR_NODE,
                                "setVerificationLoad",
                                res);

  return grpc::Status::OK;
}

void Cv2xThrottleManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event){
    LOG(DEBUG, __FUNCTION__);
    if (event.filter() == CV2X_THROTTLE_FILTER) {
        onEventUpdate(event.event());
    }
}

void Cv2xThrottleManagerServerImpl::onEventUpdate(std::string event){
    LOG(DEBUG, __FUNCTION__, event);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    handleEvent(token,event);
}

void Cv2xThrottleManagerServerImpl::handleEvent(std::string token, std::string event){
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", event);
    if (token == CV2X_THROTTLE_EVENT_FILTER_UPDATE) {
        handleFilterUpdateEvent(event);
    } else if (token == CV2X_THROTTLE_EVENT_SANITY_UPDATE){
        handleSanityUpdateEvent(event);
    }
}

void Cv2xThrottleManagerServerImpl::handleFilterUpdateEvent(std::string event){
    LOG(DEBUG, __FUNCTION__, " new filter is: ", event);
    auto f = std::async(std::launch::async, [event](){
        ::cv2xStub::FilterEvent filterEvent;
        ::eventService::EventResponse anyResponse;
        filterEvent.set_filter(std::stoul(event,nullptr,10));
        anyResponse.set_filter("throttle_mgr");
        anyResponse.mutable_any()->PackFrom(filterEvent);
        //posting the event to EventService event queue
        auto& eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    }).share();
    taskQ_->add(f);
}

void Cv2xThrottleManagerServerImpl::handleSanityUpdateEvent(std::string event){
    LOG(DEBUG, __FUNCTION__, " new sanity is: ", event);
    auto f = std::async(std::launch::async, [event](){
        ::cv2xStub::SanityEvent sanityEvent;
        ::eventService::EventResponse anyResponse;
        sanityEvent.set_state(event == "true" ? 1 : 0);
        anyResponse.set_filter("throttle_mgr");
        anyResponse.mutable_any()->PackFrom(sanityEvent);
        //posting the event to EventService event queue
        auto& eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    }).share();
    taskQ_->add(f);
}
