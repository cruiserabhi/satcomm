/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TIME_MANAGER_STUP_HPP
#define TIME_MANAGER_STUP_HPP

#include <atomic>
#include <mutex>

#include <telux/platform/TimeManager.hpp>
#include <telux/loc/LocationManager.hpp>

#include "common/AsyncTaskQueue.hpp"

namespace telux {

namespace platform {

class TimeManagerStub : public ITimeManager,
                        public telux::loc::ILocationListener,
                        public std::enable_shared_from_this<TimeManagerStub> {
 public:
    TimeManagerStub();

    /**
     * Overridden from ITimeManager
     */
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(
        std::weak_ptr<ITimeListener> listener, TimeTypeMask mask) override;
    telux::common::Status deregisterListener(
        std::weak_ptr<ITimeListener> listener, TimeTypeMask mask) override;

    /**
     * Overridden from ILocationListener
     */
    void onCapabilitiesInfo(const telux::loc::LocCapability capabilityMask) override;
    void onBasicLocationUpdate(
        const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) override;

    /**
     * Internal function to initialize utc info management service.
     */
    telux::common::Status init(telux::common::InitResponseCb initCb = nullptr);

    ~TimeManagerStub();

 private:
    void initSync(telux::common::InitResponseCb callback);
    telux::common::Status startGnssUtcReport();
    telux::common::Status stopGnssUtcReport();
    void cleanup();
    void initSync();

    std::mutex mutex_;
    std::condition_variable cv_;
    telux::common::InitResponseCb initCb_;
    std::atomic<telux::common::ServiceStatus> serviceStatus_
        = {telux::common::ServiceStatus::SERVICE_UNAVAILABLE};
    bool exiting_                                         = false;
    bool timeCap_                                         = false;
    std::shared_ptr<telux::loc::ILocationManager> locMgr_ = nullptr;
    std::mutex listenerMtx_;
    std::shared_ptr<telux::common::ListenerManager<ITimeListener, TimeTypeMask>> listenerMgr_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};

}  // end of namespace platform
}  // end of namespace telux

#endif  // TIME_MANAGER_STUP_HPP
