/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef FS_MANAGER_SERVER_HPP
#define FS_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

#include "event/EventService.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "event/ServerEventManager.hpp"
#include "protos/proto-src/platform_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using platformStub::FsManagerService;

class FsManagerServerImpl final :
    public platformStub::FsManagerService::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<FsManagerServerImpl> {
 public:
    FsManagerServerImpl();
    ~FsManagerServerImpl();

    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response);
    grpc::Status GetServiceStatus(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status StartEfsBackup(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::DefaultReply* response);
    grpc::Status PrepareForEcall(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::DefaultReply* response);
    grpc::Status ECallCompleted(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::DefaultReply* response);
    grpc::Status PrepareForOta(ServerContext* context,
        const ::platformStub::FsEventName* request,
        platformStub::DefaultReply* response);
    grpc::Status OtaCompleted(ServerContext* context,
        const google::protobuf::Empty* request,
        platformStub::DefaultReply* response);
    grpc::Status StartAbSync(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::DefaultReply* response);
    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    void apiJsonReader(std::string apiName, platformStub::DefaultReply* response);
    void onEventUpdate(std::string event);
    void handleEvent(std::string token, std::string event);
    void handleOtaAbSyncEvent(std::string eventParams);
    void handleEfsBackup(std::string eventParams);
    void handleEfsRestore(std::string eventParams);
    void handleFsOpImminentEvent(std::string eventParams);
    void triggerFsEvent(std::string fsEventName, platformStub::DefaultReply* response);
    void checkRebootDuringOTA();
    void updateFsStateMachine(std::string eventName, bool state);
    void updateSystemStateJson();
    telux::common::Status registerDefaultIndications();
    void updateFsEventReply(platformStub::DefaultReply* source,
        platformStub::DefaultReply* destination);
    grpc::Status setSrvcResponse(telux::common::ServiceStatus srvStatus,
        commonStub::GetServiceStatusReply* response);
    telux::common::ServiceStatus getServiceStatus();
    void handleSSREvent(std::string eventParams);
    void onSSREvent(telux::common::ServiceStatus srvStatus);
    grpc::Status setResponse(telux::common::ServiceStatus srvStatus,
        commonStub::GetServiceStatusReply* response);
    void notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
        std::string srvStatusStr);
    void setServiceStatus(telux::common::ServiceStatus srvStatus);
    bool isValidKey(const std::string& key);

    telux::common::ServiceStatus serviceStatus_ =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::map<std::string, bool> fsEventsMap_;
    std::mutex mutex_;
    std::mutex eventMutex_;
    bool otaSession_;
    bool abSyncState_;
    ServerEventManager &serverEvent_;
    EventService &clientEvent_;
    int cbDelay_ = 100;
};
#endif  // FS_MANAGER_SERVER_HPP