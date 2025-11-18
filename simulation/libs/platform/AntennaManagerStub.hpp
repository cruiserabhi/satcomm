/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ANTENNAMANAGERSTUB_HPP
#define ANTENNAMANAGERSTUB_HPP

#include <grpcpp/grpcpp.h>

#include "telux/platform/hardware/AntennaManager.hpp"
#include "telux/platform/hardware/AntennaListener.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "SimulationManagerStub.hpp"
#include "event-manager/ClientEventManager.hpp"
#include "protos/proto-src/platform_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;

using platformStub::AntennaManagerService;

namespace telux {
namespace platform {
namespace hardware {

using namespace telux::common;

class AntennaManagerStub : public IAntennaManager,
                           public IEventListener,
                           public SimulationManagerStub<AntennaManagerService>,
                           public std::enable_shared_from_this<AntennaManagerStub> {
 public:

    using SimulationManagerStub::init;

    AntennaManagerStub();
    ~AntennaManagerStub();

    ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<IAntennaListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IAntennaListener> listener) override;

    telux::common::Status setActiveAntenna(
        int antIndex, telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status getActiveAntenna(GetActiveAntCb callback) override;

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
    std::shared_ptr<telux::common::ListenerManager<IAntennaListener>> listenerMgr_;
    std::mutex mutex_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    // Enable the antenna switch function, this action is only triggered once after boot-up.
    bool isAntSwitchEnabled_;
    int antIndex_;
    ClientEventManager &clientEventMgr_;

    void onActiveAntennaChange(int antIndex);
    void onSetAntConfigResponse(int antIndex, ResponseCallback callback, ErrorCode errorCode);
    void onGetAntConfigResponse(GetActiveAntCb callback, ErrorCode errorCode);
    void createListener();
    void onEventUpdate(google::protobuf::Any event);

    void handleSSREvent(google::protobuf::Any event);
    void onAntennaManagerServiceStatusChange(telux::common::ServiceStatus srvcStatus);
};

}  // end of namespace hardware
}  // end of namespace platform
}  // end of namespace telux

#endif  // ANTENNAMANAGERSTUB_HPP