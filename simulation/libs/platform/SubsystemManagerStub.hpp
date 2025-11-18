/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SUBSYSTEM_MANAGER_STUB_HPP
#define SUBSYSTEM_MANAGER_STUB_HPP

#include <vector>
#include <mutex>
#include <memory>
#include <bitset>

#include <telux/platform/SubsystemManager.hpp>

#include "common/event-manager/ClientEventManager.hpp"
#include "SimulationManagerStub.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/platform_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;

using platformStub::DeviceInfoManagerService;

namespace telux {
namespace platform {

using namespace telux::common;

class SubsystemManagerStub : public ISubsystemManager,
                             public IEventListener,
                             public SimulationManagerStub<DeviceInfoManagerService>,
                             public std::enable_shared_from_this<SubsystemManagerStub> {

 public:
    using SimulationManagerStub::init;

    SubsystemManagerStub();
    ~SubsystemManagerStub();

    telux::common::ErrorCode registerListener(std::weak_ptr<ISubsystemListener> listener,
        std::vector<telux::common::SubsystemInfo> subsystems) override;

    telux::common::ErrorCode deRegisterListener(
        std::weak_ptr<ISubsystemListener> listener) override;

    telux::common::ServiceStatus getServiceStatus(void) override;

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
    std::shared_ptr<telux::common::ListenerManager<ISubsystemListener>> q6ListenerMgr_;
    std::shared_ptr<telux::common::ListenerManager<ISubsystemListener>> a7ListenerMgr_;
    std::mutex mutex_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    ClientEventManager &clientEventMgr_;
    // Define a type for the combination of Subsystem and ProcType
    using Combination = std::pair<Subsystem, ProcType>;
    // Set to store supported combinations
    std::set<Combination> supportedCombinations_;

    void sendNewStatusToClients(telux::common::OperationalStatus newOpStatus,
        Subsystem subsystem, ProcType procType);
    telux::common::ErrorCode registerForMpss(
        std::weak_ptr<ISubsystemListener> listener, telux::common::ProcType location);
    telux::common::ErrorCode registerForApss(
        std::weak_ptr<ISubsystemListener> listener, telux::common::ProcType location);
    void onDmsServiceStatusChange(telux::common::ServiceStatus newStatus);
    void createListener();
    void onEventUpdate(google::protobuf::Any event);
    void handleSSREvent(google::protobuf::Any event);
    void handleSubsystemEvent(google::protobuf::Any event);
    void registerCombination(Subsystem subsystem, ProcType procType);
    bool isSupported(Subsystem subsystem, ProcType procType);
    void resetCombination();
};

}  // End of namespace platform
}  // End of namespace telux

#endif  // SUBSYSTEM_MANAGER_STUB_HPP