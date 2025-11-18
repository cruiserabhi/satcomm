/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2X_RADIO_HELPER_STUB_HPP
#define CV2X_RADIO_HELPER_STUB_HPP

#include "common/ListenerManager.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

#include "protos/proto-src/cv2x_simulation.grpc.pb.h"

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1
#define DEFAULT_NOTIFICATION_DELAY 2000
#define RPC_FAIL_SUFFIX " RPC Request failed - "

const std::string CV2X_EVENT_RADIO_MGR_FILTER  = "cv2x_radio_manager";
const std::string CV2X_EVENT_RADIO_FILTER      = "cv2x_radio";


#define RPC_TO_CV2X_STATUS(rpc, res)                                             \
    {                                                                            \
        res.rxStatus = static_cast<telux::cv2x::Cv2xStatusType>(rpc.rxstatus()); \
        res.txStatus = static_cast<telux::cv2x::Cv2xStatusType>(rpc.txstatus()); \
        res.rxCause  = static_cast<telux::cv2x::Cv2xCauseType>(rpc.rxcause());   \
        res.txCause  = static_cast<telux::cv2x::Cv2xCauseType>(rpc.txcause());   \
    }

#define CALL_RPC(RPC_API, request, res, response, delay)               \
    ClientContext context;                                             \
    ::grpc::Status reqstatus = RPC_API(&context, request, &response);  \
    if (reqstatus.ok()) {                                              \
        res   = static_cast<telux::common::Status>(response.status()); \
        delay = static_cast<int>(response.delay());                    \
    } else {                                                           \
        res = telux::common::Status::FAILED;                           \
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());           \
    }

#define CALL_RPC_AND_RESPOND(RPC_API, request, res, cb, taskq)                     \
    {                                                                              \
        ::cv2xStub::Cv2xCommandReply response;                                     \
        int delay = DEFAULT_DELAY;                                                 \
        CALL_RPC(RPC_API, request, res, response, delay);                          \
                                                                                   \
        if (cb && taskq                                                            \
            && telux::common::Status::SUCCESS                                      \
                   == static_cast<telux::common::Status>(response.status())) {     \
            auto f = std::async(std::launch::async, [=]() {                        \
                if (delay > 0) {                                                   \
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay)); \
                }                                                                  \
                cb(static_cast<telux::common::ErrorCode>(response.error()));       \
            }).share();                                                            \
            taskq->add(f);                                                         \
        }                                                                          \
    }

#define NOTIFY_LISTENER(listenerMgr,type,cb,payload)  \
    {                                                 \
        std::vector<std::weak_ptr<type>> listeners;   \
        listenerMgr.getAvailableListeners(listeners); \
        for (auto &wp : listeners) {                  \
            if (auto sp = wp.lock()) {                \
                sp->cb(payload);                      \
            }                                         \
        }                                             \
    }


namespace telux {
namespace cv2x {

class Cv2xEvtListener : public telux::common::IEventListener {
 public:
    void onEventUpdate(google::protobuf::Any event) override;
    telux::common::Status registerListener(std::weak_ptr<telux::cv2x::ICv2xListener> listener);
    telux::common::Status deregisterListener(std::weak_ptr<telux::cv2x::ICv2xListener> listener);
    uint32_t listenersSize();

 private:
    telux::common::ListenerManager<telux::cv2x::ICv2xListener> listenerMgr_;
    void onCv2xStatusChange(telux::cv2x::Cv2xStatus &status);
    void onSlssRxInfoChange(const ::cv2xStub::SyncRefUeInfo& rcpSlssUe);
};

class Cv2xRadioHelper {
 public:
     static void resetV2xStatusEx(Cv2xStatusEx &statusEx) {
        statusEx.status.rxStatus      = Cv2xStatusType::INACTIVE;
        statusEx.status.txStatus      = Cv2xStatusType::INACTIVE;
        statusEx.status.rxCause       = Cv2xCauseType::UNKNOWN;
        statusEx.status.txCause       = Cv2xCauseType::UNKNOWN;
        statusEx.status.cbrValue      = 255;
        statusEx.status.cbrValueValid = false;
        statusEx.poolStatus.clear();
        statusEx.timeUncertaintyValid = false;
    }
    static void resetV2xStatus(Cv2xStatus &status) {
        status.rxStatus      = Cv2xStatusType::INACTIVE;
        status.txStatus      = Cv2xStatusType::INACTIVE;
        status.rxCause       = Cv2xCauseType::UNKNOWN;
        status.txCause       = Cv2xCauseType::UNKNOWN;
        status.cbrValue      = 255;
        status.cbrValueValid = false;
    }
    static void rpcSlssInfoToSlssInfo (const ::cv2xStub::SyncRefUeInfo& rcpSlss,
        cv2x::SyncRefUeInfo& refInfo) {
        refInfo.slssId     = rcpSlss.slssid();
        refInfo.inCoverage = rcpSlss.incoverage();
        refInfo.pattern    = static_cast<telux::cv2x::SlssSyncPattern>(rcpSlss.pattern());
        refInfo.rsrp       = rcpSlss.rsrp();
        refInfo.selected   = rcpSlss.selected();
    }
};

}  // namespace cv2x
}  // namespace telux
#endif
