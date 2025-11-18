/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_CV2X_SIMULATION_HELPER_SERVER_HPP
#define TELUX_CV2X_SIMULATION_HELPER_SERVER_HPP

#include <string>

#include "event/ServerEventManager.hpp"
#include <common/ListenerManager.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"

const std::string CV2X_MGR_API_JSON = "api/cv2x/ICv2xManager.json";
const std::string CV2X_MGR_NODE = "ICv2xManager";

const std::string RADIO_STATE_JSON = "system-state/cv2x/ICv2xRadio.json";
const std::string RADIO_API_JSON = "api/cv2x/ICv2xRadio.json";
const std::string RADIO_ROOT = "ICv2xRadio";

const std::string CV2X_EVENT_RADIO_MGR_FILTER = "cv2x_radio_manager";
const std::string CV2X_STATUS_EVENT           = "status";
const std::string SLSS_RX_INFO_EVT            = "slss_rx_info";

const std::string CV2X_EVENT_RADIO_FILTER   = "cv2x_radio";
const std::string SRC_L2_ID_EVT             = "src_l2_id";
const std::string SPS_SCHEDULE_CHANGE_EVT   = "sps_schedule_change";
const std::string MAC_ADDR_CLONE_ATTACK_EVT = "mac_addr_clone_attack";
const std::string RADIO_CAPABILITIES_EVT    = "capabilities";

using InjectEvtHandler = std::function<bool (std::string, ::eventService::EventResponse&)>;

class Cv2xServerUtil {
public:
  static cv2xStub::Cv2xStatus_StatusType strToStatus(std::string str);
  static telux::common::ErrorCode stateJasonRead(std::string stateCfgFile,
                                                 Json::Value &data);
  template <typename T>
  static void apiJsonReader(std::string cfg, std::string subsys,
                            std::string apiName, T *res) {
    Json::Value rootNode;
    telux::common::Status status = telux::common::Status::NOSUCH;
    telux::common::ErrorCode err = JsonParser::readFromJsonFile(rootNode, cfg);
    int cbDelay = 100;
    if (telux::common::ErrorCode::SUCCESS == err) {
      telux::common::CommonUtils::getValues(rootNode, subsys, apiName, status,
                                            err, cbDelay);
      res->set_delay(cbDelay);
    } else {
      LOG(ERROR, __FUNCTION__, cfg, ".", subsys, ".", apiName, " failed.");
    }
    res->set_status(static_cast<::commonStub::Status>(status));
    res->set_error(static_cast<::commonStub::ErrorCode>(err));
  }
};

class Cv2xServerEvtListener final : public IServerEventListener {
public:
  Cv2xServerEvtListener();
  static const std::shared_ptr<Cv2xServerEvtListener> &getInstance();

  void onEventUpdate(::eventService::UnsolicitedEvent event) override;
  cv2xStub::Cv2xStatus getCv2xStatus();
  telux::common::Status
  registerListener(std::weak_ptr<telux::cv2x::ICv2xListener> l);
  telux::common::Status
  deregisterListener(std::weak_ptr<telux::cv2x::ICv2xListener> l);
  bool OnCv2xStatusChange(std::string str, ::eventService::EventResponse& ind);
  static bool handleSrcL2IdUpdateInject(std::string str, ::eventService::EventResponse& ind);
  void readDefaultStatus();

private:
  bool stringToStatus(std::string &str, cv2xStub::Cv2xStatus &result,
                      bool &needParseCause, bool rx);
  bool stringToCause(std::string &str, cv2xStub::Cv2xStatus &result, bool rx);
  void notifyListeners(cv2xStub::Cv2xStatus &stub);

  static bool handleMacCloneAttackInject(std::string str, ::eventService::EventResponse& ind);
  static bool handleSlssRxInfoInject(std::string str, ::eventService::EventResponse& ind);
  static bool handleSpsScheduleInject(std::string str, ::eventService::EventResponse& ind);
  static bool handleCapabilitiesInject(std::string str, ::eventService::EventResponse& ind);

  cv2xStub::Cv2xStatus stubStatus_;
  std::mutex statusMtx_;
  telux::common::ListenerManager<telux::cv2x::ICv2xListener> listenerMgr_;
  std::map<std::string, InjectEvtHandler> injectEvtHdls_;
};

#endif
