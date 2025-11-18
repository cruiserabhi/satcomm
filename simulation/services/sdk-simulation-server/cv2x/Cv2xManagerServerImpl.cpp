/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       Cv2xManagerServerImpl.cpp
 *
 *
 */

#include "Cv2xManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "event/EventService.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#define SET_MEMBER(st,member,type,token) st.member(static_cast<type>(token))

#define PARSE_STR_SET_STRUCT(str,st,member,type) \
{ \
    std::string strToken = EventParserUtil::getNextToken(str, " "); \
    if (not strToken.empty()) { \
        try { \
            auto token = std::stoi(strToken); \
            SET_MEMBER(st, member, type, token); \
        } catch (exception const &ex) { \
            LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what()); \
            return false; \
        } \
    } else { \
        LOG(DEBUG, __FUNCTION__, " strToken is empty."); \
        return false; \
   } \
}

#define VALIDATE_STR_SET_STRUCT(str,st,member,type,validation) \
{ \
    std::string strToken = EventParserUtil::getNextToken(str, " "); \
    if (not strToken.empty()) { \
        try { \
            auto token = std::stoi(strToken); \
            if (validation(token)) { \
                SET_MEMBER(st, member, type, token); \
            } else { \
                LOG(ERROR, __FUNCTION__, " token is invalid."); \
                return false; \
            } \
        } catch (exception const &ex) { \
            LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what()); \
            return false; \
        } \
    } else { \
        LOG(DEBUG, __FUNCTION__, " strToken is empty."); \
        return false; \
   } \
}

telux::common::ErrorCode
Cv2xServerUtil::stateJasonRead(std::string stateCfgFile, Json::Value &data) {
  telux::common::ErrorCode err =
      JsonParser::readFromJsonFile(data, stateCfgFile);
  if (err != ErrorCode::SUCCESS) {
    LOG(ERROR, __FUNCTION__, " Reading JSON File ", stateCfgFile, " failed! ");
    return err;
  }

  return err;
}

cv2xStub::Cv2xStatus_StatusType Cv2xServerUtil::strToStatus(std::string str) {
  if (str.compare("inactive") == 0) {
    return cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_INACTIVE;
  } else if (str.compare("active") == 0) {
    return cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_ACTIVE;
  } else if (str.compare("suspended") == 0) {
    return cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_SUSPENDED;
  }
  return cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_Status_UNKNOWN;
}

Cv2xServerEvtListener::Cv2xServerEvtListener() {
  LOG(DEBUG, __FUNCTION__);
  readDefaultStatus();
  injectEvtHdls_[CV2X_STATUS_EVENT]         = std::bind(&Cv2xServerEvtListener::OnCv2xStatusChange,
      this, std::placeholders::_1, std::placeholders::_2);
  injectEvtHdls_[SLSS_RX_INFO_EVT]          = handleSlssRxInfoInject;
  injectEvtHdls_[SRC_L2_ID_EVT]             = handleSrcL2IdUpdateInject;
  injectEvtHdls_[SPS_SCHEDULE_CHANGE_EVT]   = handleSpsScheduleInject;
  injectEvtHdls_[MAC_ADDR_CLONE_ATTACK_EVT] = handleMacCloneAttackInject;
  injectEvtHdls_[RADIO_CAPABILITIES_EVT]    = handleCapabilitiesInject;
}

void Cv2xServerEvtListener::readDefaultStatus() {
  std::string method = "cv2xDefaultStatus";
  Json::Value data;

  if (telux::common::ErrorCode::SUCCESS ==
      Cv2xServerUtil::stateJasonRead(RADIO_STATE_JSON, data)) {
    stubStatus_.set_rxstatus(Cv2xServerUtil::strToStatus(
        data[RADIO_ROOT][method]["rxStatus"].asString()));
    stubStatus_.set_txstatus(Cv2xServerUtil::strToStatus(
        data[RADIO_ROOT][method]["txStatus"].asString()));

    try {
      auto num = data[RADIO_ROOT][method]["rxCause"].asInt();
      stubStatus_.set_rxcause(static_cast<::cv2xStub::Cv2xStatus_Cause>(num));
    } catch (exception const &ex) {
      LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
      return;
    }

    try {
      auto num = data[RADIO_ROOT][method]["txCause"].asInt();
      stubStatus_.set_txcause(static_cast<::cv2xStub::Cv2xStatus_Cause>(num));
    } catch (exception const &ex) {
      LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
      return;
    }
  }
}

const shared_ptr<Cv2xServerEvtListener> &Cv2xServerEvtListener::getInstance() {
  static shared_ptr<Cv2xServerEvtListener> instance(new Cv2xServerEvtListener);
  return instance;
}

bool Cv2xServerEvtListener::stringToStatus(std::string &str,
                                           cv2xStub::Cv2xStatus &result,
                                           bool &needParseCause, bool rx) {
  std::string strToken = EventParserUtil::getNextToken(str, " ");
  cv2xStub::Cv2xStatus_StatusType token;

  if (not strToken.empty()) {
    try {
      token = Cv2xServerUtil::strToStatus(strToken);
    } catch (exception const &ex) {
      LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
      return false;
    }
  } else {
    LOG(DEBUG, __FUNCTION__, " strToken is empty.");
    return false;
  }

  if (cv2xStub::Cv2xStatus_StatusType_IsValid(static_cast<int>(token))) {
    if (rx) {
      result.set_rxstatus(token);
    } else {
      result.set_txstatus(token);
    }
    if (cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_INACTIVE ==
            token ||
        cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_SUSPENDED ==
            token) {
      needParseCause = true;
    }
    return true;
  } else {
    LOG(DEBUG, __FUNCTION__, " StatusTyp is not in range ",
        static_cast<int>(token));
  }
  return false;
}

bool Cv2xServerEvtListener::stringToCause(std::string &str,
                                          cv2xStub::Cv2xStatus &result,
                                          bool rx) {
  if (rx) {
      VALIDATE_STR_SET_STRUCT(str,result,set_rxcause,::cv2xStub::Cv2xStatus_Cause,
          ::cv2xStub::Cv2xStatus_Cause_IsValid);
  } else {
      VALIDATE_STR_SET_STRUCT(str,result,set_txcause,::cv2xStub::Cv2xStatus_Cause,
          ::cv2xStub::Cv2xStatus_Cause_IsValid);
  }
  return true;
}

bool Cv2xServerEvtListener::OnCv2xStatusChange(std::string str,
  ::eventService::EventResponse& ind) {
  bool hasUpdate = false;
  bool needRxCause = false;
  bool needTxCause = false;
  cv2xStub::Cv2xStatus tmpStatus;
  LOG(DEBUG, __FUNCTION__, str);

  do {
    tmpStatus.set_rxcause(::cv2xStub::Cv2xStatus_Cause::Cv2xStatus_Cause_CAUSE_UNKNOWN);
    tmpStatus.set_txcause(::cv2xStub::Cv2xStatus_Cause::Cv2xStatus_Cause_CAUSE_UNKNOWN);
    if (not stringToStatus(str, tmpStatus, needRxCause, true)) {
      break;
    }
    if (not stringToStatus(str, tmpStatus, needTxCause, false)) {
      break;
    }

    if (needRxCause) {
      if (not stringToCause(str, tmpStatus, true)) {
        break;
      }
    }
    if (needTxCause) {
      if (not stringToCause(str, tmpStatus, false)) {
        break;
      }
    }
    hasUpdate = true;
  } while (0);

  if (hasUpdate) {
    {
      std::lock_guard<std::mutex> lock(statusMtx_);
      stubStatus_ = tmpStatus;
    }
    notifyListeners(tmpStatus);

    ind.mutable_any()->PackFrom(tmpStatus);
  } else {
    LOG(INFO, __FUNCTION__, " Cv2x status assume no change");
  }
  return hasUpdate;
}

bool Cv2xServerEvtListener::handleMacCloneAttackInject(std::string str,
    ::eventService::EventResponse& ind) {
    LOG(DEBUG, __FUNCTION__);
    ::cv2xStub::MacAddrCloneAttach detected;

    PARSE_STR_SET_STRUCT(str, detected, set_detected, bool);

    ind.mutable_any()->PackFrom(detected);
    return true;
}

bool Cv2xServerEvtListener::handleSlssRxInfoInject(std::string str,
    ::eventService::EventResponse& ind) {
    LOG(DEBUG, __FUNCTION__);
    ::cv2xStub::SyncRefUeInfo slssUe;

    PARSE_STR_SET_STRUCT(str, slssUe, set_slssid, uint32_t);
    PARSE_STR_SET_STRUCT(str, slssUe, set_incoverage, bool);
    VALIDATE_STR_SET_STRUCT(str, slssUe, set_pattern, ::cv2xStub::SyncRefUeInfo_SlssSyncPattern,
        ::cv2xStub::SyncRefUeInfo_SlssSyncPattern_IsValid);
    PARSE_STR_SET_STRUCT(str, slssUe, set_rsrp, uint32_t);
    PARSE_STR_SET_STRUCT(str, slssUe, set_selected, bool);

    ind.mutable_any()->PackFrom(slssUe);
    return true;
}

bool Cv2xServerEvtListener::handleSpsScheduleInject(std::string str,
    ::eventService::EventResponse& ind) {
    LOG(DEBUG, __FUNCTION__);
    ::cv2xStub::SpsSchedulingInfo scheduleInfo;
    PARSE_STR_SET_STRUCT(str, scheduleInfo, set_spsid, uint32_t);
    PARSE_STR_SET_STRUCT(str, scheduleInfo, set_utctime, uint64_t);
    PARSE_STR_SET_STRUCT(str, scheduleInfo, set_periodicity, uint32_t);

    ind.mutable_any()->PackFrom(scheduleInfo);
    return true;
}

bool Cv2xServerEvtListener::handleSrcL2IdUpdateInject(std::string str,
    ::eventService::EventResponse& ind) {
    LOG(DEBUG, __FUNCTION__);
    // Initialize random seed
    std::srand(std::time(0));
    // Generate random number with 24 bits
    ::cv2xStub::SrcL2Id srcL2Id;
    srcL2Id.set_id(static_cast<uint32_t>(std::rand() % 0x1000000));

    ind.mutable_any()->PackFrom(srcL2Id);
    return true;
}

bool Cv2xServerEvtListener::handleCapabilitiesInject(std::string str,
    ::eventService::EventResponse& ind) {
    LOG(DEBUG, __FUNCTION__);
    ::cv2xStub::RadioCapabilites radioCaps;
    ::cv2xStub::TxPoolIdInfo pool;

    SET_MEMBER(pool,set_poolid, uint32_t, 0);
    PARSE_STR_SET_STRUCT(str, pool, set_minfreq, uint32_t);
    PARSE_STR_SET_STRUCT(str, pool, set_maxfreq, uint32_t);
    if (pool.minfreq() > 0 && pool.minfreq() < 0x00FFFF &&
        pool.maxfreq() > 0 && pool.maxfreq() < 0x00FFFF) {
        ::cv2xStub::TxPoolIdInfo* uePool = radioCaps.add_pools();
        if (uePool) {
            uePool->set_poolid(pool.poolid());
            uePool->set_minfreq(pool.minfreq());
            uePool->set_maxfreq(pool.maxfreq());
        } else {
            return false;
        }
    } else {
        return false;
    }

    if (str.length() >= 3) {
        LOG(DEBUG, __FUNCTION__, str);
        SET_MEMBER(pool,set_poolid, uint32_t, 1);
        PARSE_STR_SET_STRUCT(str, pool, set_minfreq, uint32_t);
        PARSE_STR_SET_STRUCT(str, pool, set_maxfreq, uint32_t);
        if (pool.minfreq() > 0 && pool.minfreq() < 0x00FFFF &&
            pool.maxfreq() > 0 && pool.maxfreq() < 0x00FFFF) {
            ::cv2xStub::TxPoolIdInfo* uePool = radioCaps.add_pools();
            if (uePool) {
                uePool->set_poolid(pool.poolid());
                uePool->set_minfreq(pool.minfreq());
                uePool->set_maxfreq(pool.maxfreq());
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    ind.mutable_any()->PackFrom(radioCaps);
    return true;
}


void Cv2xServerEvtListener::onEventUpdate(
    ::eventService::UnsolicitedEvent message) {
  bool hasUpdate = false;
  ::eventService::EventResponse ind;
  auto event = message.event();

  LOG(DEBUG, __FUNCTION__, message.filter(), " ", event);
  if (event.empty()) {
      return;
  }

  std::string strToken = EventParserUtil::getNextToken(event, " ");
  if (injectEvtHdls_.find(strToken) != injectEvtHdls_.end()) {
      hasUpdate = injectEvtHdls_[strToken](event, ind);
  } else {
      LOG(DEBUG, __FUNCTION__, " no handler for ", strToken);
  }

  if (hasUpdate) {
      // posting the event to EventService event queue
      ind.set_filter(message.filter());
      EventService::getInstance().updateEventQueue(ind);
  }
}

cv2xStub::Cv2xStatus Cv2xServerEvtListener::getCv2xStatus() {
  std::lock_guard<std::mutex> lock(statusMtx_);
  return stubStatus_;
}

void Cv2xServerEvtListener::notifyListeners(cv2xStub::Cv2xStatus &stub) {
  std::vector<std::weak_ptr<telux::cv2x::ICv2xListener>> lists;
  telux::cv2x::Cv2xStatus status;

  status.rxStatus = static_cast<telux::cv2x::Cv2xStatusType>(stub.rxstatus());
  status.txStatus = static_cast<telux::cv2x::Cv2xStatusType>(stub.txstatus());
  status.rxCause = static_cast<telux::cv2x::Cv2xCauseType>(stub.rxcause());
  status.txCause = static_cast<telux::cv2x::Cv2xCauseType>(stub.txcause());

  listenerMgr_.getAvailableListeners(lists);
  for (auto &wp : lists) {
    if (auto sp = wp.lock()) {
      sp->onStatusChanged(status);
    }
  }
}

telux::common::Status Cv2xServerEvtListener::registerListener(
    std::weak_ptr<telux::cv2x::ICv2xListener> l) {
  return listenerMgr_.registerListener(l);
}

telux::common::Status Cv2xServerEvtListener::deregisterListener(
    std::weak_ptr<telux::cv2x::ICv2xListener> l) {
  return listenerMgr_.deRegisterListener(l);
}

Cv2xManagerServerImpl::Cv2xManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
  evtListener_ = Cv2xServerEvtListener::getInstance();
  if (evtListener_) {
    std::vector<std::string> filters = {CV2X_EVENT_RADIO_MGR_FILTER};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.registerListener(evtListener_, filters);
  }
}

Cv2xManagerServerImpl::~Cv2xManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
  if (evtListener_) {
    std::vector<std::string> filters = {CV2X_EVENT_RADIO_MGR_FILTER};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.deregisterListener(evtListener_, filters);
  }
}

grpc::Status
Cv2xManagerServerImpl::initService(ServerContext *context,
                                   const google::protobuf::Empty *request,
                                   cv2xStub::GetServiceStatusReply *res) {
  LOG(DEBUG, __FUNCTION__);
  int cbDelay = 100;
  telux::common::ServiceStatus serviceStatus =
      telux::common::ServiceStatus::SERVICE_FAILED;
  Json::Value rootNode;
  telux::common::ErrorCode errorCode =
      JsonParser::readFromJsonFile(rootNode, CV2X_MGR_API_JSON);
  if (errorCode == ErrorCode::SUCCESS) {
    cbDelay = rootNode[CV2X_MGR_NODE]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootNode[CV2X_MGR_NODE]["IsSubsystemReady"].asString();
    serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
  } else {
    LOG(ERROR, "Unable to read Cv2xManager JSON");
  }
  res->set_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
  res->set_delay(cbDelay);
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::startCv2x(ServerContext *context,
                                 const google::protobuf::Empty *request,
                                 cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE, "startCv2x",
                                res);
  evtListener_->readDefaultStatus();
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::stopCv2x(ServerContext *context,
                                const google::protobuf::Empty *request,
                                cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE, "stopCv2x",
                                res);
  ::eventService::EventResponse dummy;
  evtListener_->OnCv2xStatusChange("inactive inactive 2 2", dummy);
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::setPeakTxPower(ServerContext *context,
                                      const cv2xStub::Cv2xPeakTxPower *request,
                                      ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "setPeakTxPower", res);
  return grpc::Status::OK;
}

grpc::Status Cv2xManagerServerImpl::injectCoarseUtcTime(
    ServerContext *context, const ::cv2xStub::CoarseUtcTime *request,
    ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__, " utc: ", request->utc());
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "injectCoarseUtcTime", res);
  return grpc::Status::OK;
}

grpc::Status Cv2xManagerServerImpl::requestCv2xStatus(
    ServerContext *context, const google::protobuf::Empty *request,
    ::cv2xStub::Cv2xRequestStatusReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "requestCv2xStatus", res);
  if (evtListener_) {
    ::cv2xStub::Cv2xStatus *resp = res->mutable_cv2xstatus();
    if (resp) {
      *resp = evtListener_->getCv2xStatus();
    }
  }
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::getSlssRxInfo(ServerContext *context,
                                     const google::protobuf::Empty *request,
                                     ::cv2xStub::SlssRxInfoReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "getSlssRxInfo", res);

  Json::Value rootNode;
  uint32_t slssId = 1;
  bool inCoverage = true;
  telux::cv2x::SlssSyncPattern pattern = telux::cv2x::SlssSyncPattern::OFFSET_IND_1;
  uint32_t rsrp = 1;
  bool selected = true;

  ::cv2xStub::SyncRefUeInfo *ueInfo = res->add_info();

  ueInfo->set_slssid(slssId);
  ueInfo->set_incoverage(inCoverage);
  ueInfo->set_pattern(
      static_cast<::cv2xStub::SyncRefUeInfo::SlssSyncPattern>(pattern));
  ueInfo->set_rsrp(rsrp);
  ueInfo->set_selected(selected);

  LOG(DEBUG, __FUNCTION__, " slssId: ", ueInfo->slssid(),
      " inCoverage: ", ueInfo->incoverage());

  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::setL2Filters(ServerContext *context,
                                    const ::cv2xStub::L2FilterInfos *request,
                                    ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "setL2Filters", res);
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::removeL2Filters(ServerContext *context,
                                       const ::cv2xStub::L2Ids *request,
                                       ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "removeL2Filters", res);
  return grpc::Status::OK;
}
