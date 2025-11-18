/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef FS_MANAGER_STUB_HPP
#define FS_MANAGER_STUB_HPP

#include "telux/platform/FsDefines.hpp"
#include "telux/platform/FsManager.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "SimulationManagerStub.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/platform_simulation.grpc.pb.h"
#include "common/CommandCallbackManager.hpp"

using grpc::Channel;
using grpc::ClientContext;

using platformStub::FsManagerService;

namespace telux {
namespace platform {

using namespace telux::common;

class FsManagerStub : public IFsManager,
                      public IEventListener,
                      public SimulationManagerStub<FsManagerService>,
                      public std::enable_shared_from_this<FsManagerStub> {
 public:
    using SimulationManagerStub::init;

    FsManagerStub();
    ~FsManagerStub();

    /**
     * Overridden from IFsManager
     */
    ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<IFsListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IFsListener> listener) override;

    /**
     * Overridden from IFsManager
     */
    telux::common::Status startEfsBackup() override;

    telux::common::Status prepareForEcall() override;

    telux::common::Status eCallCompleted() override;

    telux::common::Status prepareForOta(
        OtaOperation otaOperation, telux::common::ResponseCallback responseCb) override;

    telux::common::Status otaCompleted(
        OperationStatus operationStatus, telux::common::ResponseCallback responseCb) override;

    telux::common::Status startAbSync(telux::common::ResponseCallback responseCb) override;

    void handleFsEventReply(::platformStub::FsEventReply event);

    void handleSSREvent(::commonStub::GetServiceStatusReply ssrResp);

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
    std::shared_ptr<telux::common::ListenerManager<IFsListener>> listenerMgr_;
    std::mutex mutex_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    ClientEventManager &clientEventMgr_;
    std::mutex callbackMutex_;
    telux::common::CommandCallbackManager cmdCallbackMgr_;

    int otaStartCmdCallbackId_;
    int otaResumeCmdCallbackId_;
    int otaEndCmdCallbackId_;
    int abSyncCmdCallbackId_;

    void handleOtaEvent(::platformStub::FsEventReply event);
    void handleEfsBackupEvent(::platformStub::FsEventReply event);
    void handleEfsRestoreEvent(::platformStub::FsEventReply event);
    void handleFsOpImminentEvent(::platformStub::FsEventReply event);
    void onEfsBackupEvent(std::string fsEventName, ErrorCode error);
    void onEfsRestoreEvent(std::string fsEventName, ErrorCode error);
    void onOtaABSyncEvent(std::string fsEventName, ErrorCode error);
    void onFsOpImminentEvent(uint32_t timeToExpiry);
    int getCmdCallbackId(int &cmdId, std::shared_ptr<ICommandCallback> &callback);
    void onFsServiceStatusChange(ServiceStatus srvcStatus);
    void createListener();
    void onEventUpdate(google::protobuf::Any event);
    void handleSSREvent(google::protobuf::Any event);
    void onFsManagerServiceStatusChange(telux::common::ServiceStatus srvcStatus);

};

}  // end of namespace platform
}  // end of namespace telux

#endif  // FS_MANAGER_STUB_HPP
