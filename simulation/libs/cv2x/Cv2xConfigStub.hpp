/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2X_CONFIG_STUB_HPP
#define CV2X_CONFIG_STUB_HPP

#include <future>

#include "Cv2xRadioHelperStub.hpp"
#include "common/ListenerManager.hpp"
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xConfig.hpp>

#include "protos/proto-src/cv2x_simulation.grpc.pb.h"

namespace telux {

namespace common {
template <typename T>
class AsyncTaskQueue;
}

namespace cv2x {

class ConfigChangedListener : public telux::common::IEventListener {
 public:
    void onEventUpdate(google::protobuf::Any event) override;
    telux::common::Status registerListener(
        std::weak_ptr<telux::cv2x::ICv2xConfigListener> listener);
    telux::common::Status deregisterListener(
        std::weak_ptr<telux::cv2x::ICv2xConfigListener> listener);

 private:
    telux::common::ListenerManager<telux::cv2x::ICv2xConfigListener> listenerMgr_;
    void onConfigChanged(const ConfigEventInfo &info);
};

class Cv2xConfigStub : public ICv2xConfig {
 public:
    Cv2xConfigStub();
    ~Cv2xConfigStub();

    bool isReady();
    std::future<bool> onReady();
    telux::common::ServiceStatus getServiceStatus();

    telux::common::Status init(telux::common::InitResponseCb callback = nullptr);

    telux::common::Status updateConfiguration(
        const std::string &configFilePath, telux::common::ResponseCallback cb);
    telux::common::Status retrieveConfiguration(
        const std::string &configFilePath, telux::common::ResponseCallback cb);
    telux::common::Status registerListener(std::weak_ptr<ICv2xConfigListener> listener);
    telux::common::Status deregisterListener(std::weak_ptr<ICv2xConfigListener> listener);

 private:
    std::unique_ptr<::cv2xStub::Cv2xConfigService::Stub> stub_;
    std::atomic<telux::common::ServiceStatus> serviceStatus_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE};

    void initSync(telux::common::InitResponseCb callback);

    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> exiting_;
    std::vector<std::weak_ptr<ICv2xConfigListener>> listeners_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<ConfigChangedListener> configEvtListener_;
};

}  // namespace cv2x
}  // namespace telux

#endif  // CV2X_CONFIG_STUB_HPP
