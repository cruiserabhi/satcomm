/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2X_RADIO_MANAGER_STUB_HPP
#define CV2X_RADIO_MANAGER_STUB_HPP

#include <future>

#include "Cv2xRadioHelperStub.hpp"
#include "Cv2xRadioStub.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

#include "protos/proto-src/cv2x_simulation.grpc.pb.h"

namespace telux {
namespace cv2x {

class Cv2xRadioManagerStub : public ICv2xRadioManager {
 public:
    Cv2xRadioManagerStub();
    explicit Cv2xRadioManagerStub(bool runAsIPCServer);
    ~Cv2xRadioManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);
    telux::common::ServiceStatus getServiceStatus() override;
    bool isReady() override;
    std::future<bool> onReady() override;

    telux::common::Status startCv2x(StartCv2xCallback cb);
    telux::common::Status stopCv2x(StopCv2xCallback cb);
    telux::common::Status requestCv2xStatus(RequestCv2xStatusCallback cb);
    telux::common::Status requestCv2xStatus(RequestCv2xStatusCallbackEx cb);
    telux::common::Status registerListener(std::weak_ptr<ICv2xListener> listener);
    telux::common::Status deregisterListener(std::weak_ptr<ICv2xListener> listener);
    telux::common::Status setPeakTxPower(int8_t txPower, common::ResponseCallback cb);
    telux::common::Status setL2Filters(
        const std::vector<L2FilterInfo> &filterList, common::ResponseCallback cb);
    telux::common::Status removeL2Filters(
        const std::vector<uint32_t> &l2IdList, common::ResponseCallback cb);
    telux::common::Status getSlssRxInfo(GetSlssRxInfoCallback cb);
    telux::common::Status injectCoarseUtcTime(uint64_t utc, common::ResponseCallback cb);
    std::shared_ptr<ICv2xRadio> getCv2xRadio(
        TrafficCategory category, telux::common::InitResponseCb cb = nullptr);

 private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::condition_variable initializedCv_;
    std::mutex mutex_;
    std::vector<telux::common::InitResponseCb> cv2xRadioInitCallbacks_;
    std::weak_ptr<ICv2xRadio> radio_;
    telux::common::ServiceStatus cv2xRadioInitStatus_
        = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::mutex radioMutex_;
    std::condition_variable radioCv_;
    std::atomic<bool> exiting_;

    std::unique_ptr<::cv2xStub::Cv2xManagerService::Stub> stub_;
    std::shared_ptr<Cv2xEvtListener> pEvtListener_;
    std::atomic<telux::common::ServiceStatus> serviceStatus_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE};

    void initSync(telux::common::InitResponseCb callback);
    void onGetCv2xRadioResponse(telux::common::ServiceStatus status);
};

}  // namespace cv2x
}  // namespace telux

#endif  // CV2X_RADIO_MANAGER_STUB_HPP
