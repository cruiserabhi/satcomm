/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DEVICE_INFO_MANAGER_STUB_HPP
#define DEVICE_INFO_MANAGER_STUB_HPP

#include <grpcpp/grpcpp.h>

#include "telux/platform/DeviceInfoManager.hpp"
#include "telux/platform/DeviceInfoListener.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "SimulationManagerStub.hpp"
#include "event-manager/ClientEventManager.hpp"
#include "protos/proto-src/platform_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;

using platformStub::DeviceInfoManagerService;

namespace telux {
namespace platform {

using namespace telux::common;

class DeviceInfoManagerStub : public IDeviceInfoManager,
                              public IDeviceInfoListener,
                              public IEventListener,
                              public SimulationManagerStub<DeviceInfoManagerService>,
                              public std::enable_shared_from_this<DeviceInfoManagerStub> {
 public:

    using SimulationManagerStub::init;

    DeviceInfoManagerStub();

    /**
     * Overridden from IDeviceInfoManager
     */
    ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<IDeviceInfoListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IDeviceInfoListener> listener) override;
    ~DeviceInfoManagerStub();
    telux::common::Status getPlatformVersion(PlatformVersion &pv) override;
    telux::common::Status getIMEI(std::string &imei) override;

    telux::common::Status initSyncComplete(
            telux::common::ServiceStatus srvcStatus) override;

 protected:
    telux::common::Status init() override;
    void cleanup() override;
    void setInitCbDelay(uint32_t cbDelay) override;
    uint32_t getInitCbDelay() override;
    void notifyServiceStatus(telux::common::ServiceStatus srvcStatus) override;
    telux::common::Status registerDefaultIndications();

 private:
    uint32_t cbDelay_ = 0;
    std::shared_ptr<telux::common::ListenerManager<IDeviceInfoListener>> listenerMgr_;
    std::mutex mutex_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    ClientEventManager &clientEventMgr_;

    void createListener();
    void onEventUpdate(google::protobuf::Any event);
    void handleSSREvent(google::protobuf::Any event);
    void onDmsServiceStatusChange(telux::common::ServiceStatus srvcStatus);
};

}  // end of namespace platform
}  // end of namespace telux

#endif  // DEVICE_INFO_MANAGER_STUB_HPP