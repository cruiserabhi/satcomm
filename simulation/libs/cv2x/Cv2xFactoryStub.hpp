/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2X_FACTORY_STUB_IMPL_HPP
#define CV2X_FACTORY_STUB_IMPL_HPP

#include <telux/cv2x/Cv2xFactory.hpp>

namespace telux {

namespace common {
template <typename T>
class AsyncTaskQueue;
}

namespace cv2x {

class Cv2xFactoryStub : public Cv2xFactory {
 public:
    static Cv2xFactory &getInstance();
    static Cv2xFactoryStub &getCv2xFactoryStub();

    std::shared_ptr<ICv2xRadioManager> getCv2xRadioManager(
        telux::common::InitResponseCb cb = nullptr) override;
    std::shared_ptr<ICv2xRadioManager> getCv2xRadioManager(
        bool ipcServerMode, telux::common::InitResponseCb cb = nullptr);
    std::shared_ptr<ICv2xConfig> getCv2xConfig(telux::common::InitResponseCb cb = nullptr) override;
    std::shared_ptr<ICv2xThrottleManager> getCv2xThrottleManager(
        telux::common::InitResponseCb cb = nullptr) override;

 private:
    Cv2xFactoryStub();
    ~Cv2xFactoryStub();

    void onGetCv2xConfigResponse(telux::common::ServiceStatus status);
    void onGetCv2xRadioManagerResponse(telux::common::ServiceStatus status);
    void onGetCv2xThrottleManagerResponse(telux::common::ServiceStatus status);

    std::mutex mutex_;
    std::weak_ptr<ICv2xRadioManager> radioManager_;
    std::weak_ptr<ICv2xConfig> config_;
    std::weak_ptr<ICv2xThrottleManager> throttleManager_;
    std::vector<telux::common::InitResponseCb> cv2xManagerInitCallbacks_;
    std::vector<telux::common::InitResponseCb> cv2xConfigInitCallbacks_;
    std::vector<telux::common::InitResponseCb> cv2xThrottleMgrInitCallbacks_;
    telux::common::ServiceStatus cv2xManagerInitStatus_;
    telux::common::ServiceStatus cv2xConfigInitStatus_;
    telux::common::ServiceStatus cv2xThrottleMgrInitStatus_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_ = nullptr;
};

}  // namespace cv2x
}  // namespace telux

#endif  // #ifndef CV2X_FACTORY_STUB_IMPL_HPP
