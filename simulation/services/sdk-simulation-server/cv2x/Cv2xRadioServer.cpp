/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cstdlib>
#include <ctime>
#include "event/EventService.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "Cv2xRadioServer.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"

Cv2xRadioServer::Cv2xRadioServer() {
  LOG(DEBUG, __FUNCTION__);
  evtListener_ = Cv2xServerEvtListener::getInstance();
}

Cv2xRadioServer::~Cv2xRadioServer() {
  LOG(DEBUG, __FUNCTION__);
  std::vector<std::string> filters = {CV2X_EVENT_RADIO_FILTER};
  ServerEventManager::getInstance().deregisterListener(evtListener_, filters);
}

void Cv2xRadioServer::init(
    std::weak_ptr<telux::cv2x::ICv2xListener> self) {
  LOG(DEBUG, __FUNCTION__);
  if (evtListener_) {
    std::vector<std::string> filters = {CV2X_EVENT_RADIO_FILTER};
    ServerEventManager::getInstance().registerListener(evtListener_, filters);

    evtListener_->registerListener(self);
  }
}

void Cv2xRadioServer::onStatusChanged(
    telux::cv2x::Cv2xStatus status) {
  LOG(DEBUG, __FUNCTION__, static_cast<int>(status.rxStatus),
      static_cast<int>(status.txStatus));
  if (status.rxStatus != telux::cv2x::Cv2xStatusType::ACTIVE &&
      status.rxStatus != telux::cv2x::Cv2xStatusType::SUSPENDED) {
    std::lock_guard<std::mutex> lock(rxSubsMtx_);
    rxSubsVec_.clear();
  }
  if (status.txStatus != telux::cv2x::Cv2xStatusType::ACTIVE &&
      status.txStatus != telux::cv2x::Cv2xStatusType::SUSPENDED) {
    {
      std::lock_guard<std::mutex> lock(flowMtx_);
      spsFlows_.clear();
      evtFlows_.clear();
    }
  }
}

grpc::Status Cv2xRadioServer::requestCv2xStatus(
    ServerContext *context, const google::protobuf::Empty *request,
    ::cv2xStub::Cv2xRequestStatusReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT, "requestCv2xStatus",
                                res);
  ::cv2xStub::Cv2xStatus *resp = res->mutable_cv2xstatus();
  if (resp) {
    *resp = evtListener_->getCv2xStatus();
  }
  return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::addRxSubscription(
    ServerContext *context, const cv2xStub::RxSubscription *request,
    cv2xStub::Cv2xCommandReply *res) {
  bool conflict = false;

  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT, "addRxSubscription",
                                res);
  if (::commonStub::Status::SUCCESS != res->status()) {
    return grpc::Status::OK;
  }
  if (not request) {
    LOG(ERROR, __FUNCTION__, " not request.");
    res->set_status(::commonStub::Status::FAILED);
    res->set_error(::commonStub::ErrorCode::MISSING_RESOURCE);
    return grpc::Status::OK;
  }

  std::lock_guard<std::mutex> lock(rxSubsMtx_);

  if (not rxSubsVec_.empty()) {
    for (cv2xStub::RxSubscription itr : rxSubsVec_) {
      if (itr.iptype() == request->iptype()) {
        if (0 == request->ids_size() ||
            (request->ids_size() > 0 && 0 == itr.ids_size())) {
          /*allow only 1 wildcard rx subscription*/
          conflict = true;
          LOG(ERROR, __FUNCTION__, " wildcard rx subscription policy conflict");
          break;
        } else if (itr.portnum() == request->portnum()) {
          /*port number can not be the same*/
          conflict = true;
          LOG(ERROR, __FUNCTION__, " port number conflict ",
              request->portnum());
          break;
        } else {
          uint32_t reqIdsSize = request->ids_size();
          uint32_t existingIdsSize = itr.ids_size();
          if (reqIdsSize > 0 && existingIdsSize > 0) {
            for (uint32_t i = 0; i < reqIdsSize; ++i) {
              for (uint32_t j = 0; j < existingIdsSize; ++j) {
                if (request->ids(i) == itr.ids(j)) {
                  /*rx subscription id conflict*/
                  conflict = true;
                  LOG(ERROR, __FUNCTION__, " rx subscription id conflict ",
                      request->ids(i));
                  break;
                }
              }
              if (conflict) {
                break;
              }
            }
            if (conflict) {
              break;
            }
          }
        }
      }
    }
  }

  if (not conflict) {
    auto newItr = *request;
    rxSubsVec_.emplace_back(newItr);
    LOG(DEBUG, __FUNCTION__, " iptype ", request->iptype(), ", port ",
        request->portnum(), " success.");
  } else {
    res->set_status(::commonStub::Status::FAILED);
    res->set_error(
        ::commonStub::ErrorCode::V2X_ERR_SRV_ID_L2_ADDRS_NOT_COMPATIBLE);
  }
  return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::delRxSubscription(
    ServerContext *context, const cv2xStub::RxSubscription *request,
    cv2xStub::Cv2xCommandReply *res) {
  LOG(ERROR, __FUNCTION__);
  bool success = false;

  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT, "delRxSubscription",
                                res);
  if (::commonStub::Status::SUCCESS != res->status()) {
    return grpc::Status::OK;
  }

  if (not request) {
    LOG(ERROR, __FUNCTION__, " not request.");
    res->set_status(::commonStub::Status::FAILED);
    res->set_error(::commonStub::ErrorCode::MISSING_RESOURCE);
    return grpc::Status::OK;
    ;
  }
  LOG(DEBUG, __FUNCTION__, " iptype ", request->iptype(), ", port ",
      request->portnum());
  {
    std::lock_guard<std::mutex> lock(rxSubsMtx_);
    for (auto it = rxSubsVec_.begin(); it != rxSubsVec_.end();) {
      if (it->iptype() == request->iptype() &&
          it->portnum() == request->portnum()) {
        /*rx subscription id list is maintained/checked in radio, do not check
         * it here*/
        it = rxSubsVec_.erase(it);
        success = true;
        break;
      } else {
        ++it;
      }
    }
  }

  if (not success) {
    res->set_status(::commonStub::Status::FAILED);
    res->set_error(::commonStub::ErrorCode::NO_SUCH_ELEMENT);
    LOG(ERROR, __FUNCTION__, " fail erase port ", request->portnum());
  }
  return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::enableRxMetaDataReport(
    ServerContext *context, const cv2xStub::RxSubscription *request,
    cv2xStub::Cv2xCommandReply *res) {
  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT,
                                "enableRxMetaDataReport", res);
  return grpc::Status::OK;
}

::commonStub::Status Cv2xRadioServer::saveFlowInfo(
    std::map<uint32_t, cv2xStub::FlowInfo> &flows, cv2xStub::FlowInfo &flow,
    const uint32_t base, const uint32_t max, int32_t &flowId) {
  if (flows.size() < max) {
    for (uint32_t id = base; id < base + max; ++id) {
      if (flows.find(id) == flows.end()) {
        flows[id] = flow;
        flowId = static_cast<int32_t>(id);
        LOG(INFO, __FUNCTION__, " new flow with id ", id);
        return ::commonStub::Status::SUCCESS;
      }
    }
  }
  LOG(ERROR, __FUNCTION__, " existing flows reached to max ", max);
  return ::commonStub::Status::FAILED;
}

::commonStub::Status Cv2xRadioServer::removeFlowInfo(
    std::map<uint32_t, cv2xStub::FlowInfo> &flows, int32_t flowId) {
  auto itr = flows.find(static_cast<uint32_t>(flowId));
  if (itr != std::end(flows)) {
    flows.erase(itr);
    return ::commonStub::Status::SUCCESS;
  }
  LOG(ERROR, __FUNCTION__, " not found flowId ", flowId);
  return ::commonStub::Status::FAILED;
}

grpc::Status
Cv2xRadioServer::registerFlow(ServerContext *context,
                                        const cv2xStub::FlowInfo *request,
                                        cv2xStub::Cv2xRadioFlowReply *res) {
  int32_t id = -1;
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT, "registerFlow",
                                res);
  if (::commonStub::Status::SUCCESS != res->status()) {
    return grpc::Status::OK;
  }

  ::commonStub::Status status = ::commonStub::Status::FAILED;
  cv2xStub::FlowInfo flow = *request;
  {
    std::lock_guard<std::mutex> lock(flowMtx_);
    if (request->spsport() > 0) {
      status =
          saveFlowInfo(spsFlows_, flow, 0, SIMULATION_SPS_MAX_NUM_FLOWS, id);
    } else if (request->eventport() > 0) {
      status =
          saveFlowInfo(evtFlows_, flow, SIMULATION_EVT_FLOW_BASE,
              SIMULATION_NON_SPS_MAX_NUM_FLOWS, id);
    }
  }

  res->set_status(status);
  if (status != ::commonStub::Status::SUCCESS) {
    res->set_error(::commonStub::ErrorCode::MODEM_ERR);
  } else {
    res->set_flowid(id);
  }
  return grpc::Status::OK;
}

grpc::Status
Cv2xRadioServer::deregisterFlow(ServerContext *context,
                                          const cv2xStub::FlowInfo *request,
                                          cv2xStub::Cv2xRadioFlowReply *res) {
  int32_t id = request->flowid();
  LOG(DEBUG, __FUNCTION__, " with flow id ", id);
  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT, "deregisterFlow",
                                res);
  if (::commonStub::Status::SUCCESS != res->status()) {
    return grpc::Status::OK;
  }

  ::commonStub::Status status = ::commonStub::Status::FAILED;
  {
    std::lock_guard<std::mutex> lock(flowMtx_);
    if (request->spsport() > 0) {
      status = removeFlowInfo(spsFlows_, id);
    } else if (request->eventport() > 0) {
      status = removeFlowInfo(evtFlows_, id);
    }
  }

  res->set_status(status);
  if (status != ::commonStub::Status::SUCCESS) {
    res->set_error(::commonStub::ErrorCode::MODEM_ERR);
  }

  return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::updateSrcL2Info(
    ServerContext *context, const google::protobuf::Empty *request,
    cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT, "updateSrcL2Info",
                                res);
  if (::commonStub::Status::SUCCESS == res->status()) {
      if (evtListener_) {
          ::eventService::EventResponse srcL2IdChangeInd;
          evtListener_->handleSrcL2IdUpdateInject("", srcL2IdChangeInd);
          srcL2IdChangeInd.set_filter(CV2X_EVENT_RADIO_FILTER);
          // posting the event to EventService event queue
          EventService::getInstance().updateEventQueue(srcL2IdChangeInd);
      } else {
          res->set_status(::commonStub::Status::NOMEMORY);
      }
  }
  return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::updateTrustedUEList(
    ServerContext *context, const google::protobuf::Empty *request,
    cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT,
                                "updateTrustedUEList", res);

  return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::getIfaceNameFromIpType(
    ServerContext *context, const cv2xStub::IpType *request,
    cv2xStub::IfaceNameReply *res) {
  LOG(DEBUG, __FUNCTION__);

  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT,
                                "getIfaceNameFromIpType", res);
  if (::commonStub::Status::SUCCESS != res->status()) {
    return grpc::Status::OK;
  }

  std::string ipType = (request->type() == TRAFFIC_NON_IP) ? "nonIP" : "IP";
  Json::Value data;
  if (telux::common::ErrorCode::SUCCESS ==
      Cv2xServerUtil::stateJasonRead(RADIO_STATE_JSON, data)) {
    res->set_name(data[RADIO_ROOT]["ifaceName"][ipType].asString());
  }

  return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::enableTxStatusReport(
    ServerContext *context, const cv2xStub::UintNum *request,
    cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);

  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT,
                                "enableTxStatusReport", res);
  if (::commonStub::Status::SUCCESS != res->status()) {
    return grpc::Status::OK;
  }
  uint32_t port = request->num();
  {
    std::lock_guard<std::mutex> lock(txStatusReportMtx_);
    auto itr = txStatusReportEnable_.find(port);
    if (itr == txStatusReportEnable_.end() || false == itr->second) {
      txStatusReportEnable_[port] = true;
    } else {
      LOG(DEBUG, __FUNCTION__, port, " TxStatus already enabled.");
      res->set_status(::commonStub::Status::ALREADY);
      res->set_error(::commonStub::ErrorCode::NO_EFFECT);
    }
  }
  return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::disableTxStatusReport(
    ServerContext *context, const cv2xStub::UintNum *request,
    cv2xStub::Cv2xCommandReply *res) {

  auto port = request->num();
  LOG(DEBUG, __FUNCTION__, " port ", port);
  Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT,
                                "disableTxStatusReport", res);
  if (::commonStub::Status::SUCCESS != res->status()) {
    return grpc::Status::OK;
  }
  {
    std::lock_guard<std::mutex> lock(txStatusReportMtx_);
    auto itr = txStatusReportEnable_.find(request->num());
    if (itr != txStatusReportEnable_.end()) {
      txStatusReportEnable_.erase(itr);
    } else {
      LOG(DEBUG, __FUNCTION__, " not found the port num");
      res->set_status(::commonStub::Status::NOSUCH);
      res->set_error(::commonStub::ErrorCode::NO_EFFECT);
    }
  }
  return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::setGlobalIPInfo(ServerContext *context,
    const google::protobuf::Empty *request, ::cv2xStub::Cv2xCommandReply *res) {
    LOG(DEBUG, __FUNCTION__);
    Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT, "setGlobalIPInfo",
        res);

    return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::setGlobalIPUnicastRoutingInfo(ServerContext *context,
    const google::protobuf::Empty *request, ::cv2xStub::Cv2xCommandReply *res) {
    LOG(DEBUG, __FUNCTION__);
    Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT, "setGlobalIPUnicastRoutingInfo",
        res);

    return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::requestDataSessionSettings(ServerContext *context,
    const google::protobuf::Empty *request, ::cv2xStub::Cv2xCommandReply *res) {
    LOG(DEBUG, __FUNCTION__);
    Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT, "requestDataSessionSettings",
        res);

    return grpc::Status::OK;
}

grpc::Status Cv2xRadioServer::injectVehicleSpeed(ServerContext *context,
    const cv2xStub::UintNum *request,
    cv2xStub::Cv2xCommandReply *res) {
    uint32_t speed = request->num();
    LOG(DEBUG, __FUNCTION__, " speed ", speed);

    Cv2xServerUtil::apiJsonReader(RADIO_API_JSON, RADIO_ROOT,
                                  "injectVehicleSpeed", res);

    return grpc::Status::OK;
}
