/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DEVICE_INFO_MANAGER_SERVER_HPP
#define DEVICE_INFO_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <telux/common/CommonDefines.hpp>

#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"
#include "protos/proto-src/platform_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using platformStub::DeviceInfoManagerService;

class DeviceInfoManagerServerImpl final :
    public IServerEventListener,
    public platformStub::DeviceInfoManagerService::Service,
    public std::enable_shared_from_this<DeviceInfoManagerServerImpl> {
 public:
    DeviceInfoManagerServerImpl();
    ~DeviceInfoManagerServerImpl();

    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response);

    grpc::Status GetServiceStatus(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;

    grpc::Status GetPlatformVersion(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::PlatformVersionInfo* response);

    grpc::Status GetIMEI(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::PlatformImeiInfo* response);

 private:
    void onSSREvent(telux::common::ServiceStatus srvStatus);
    grpc::Status setResponse(telux::common::ServiceStatus srvStatus,
        commonStub::GetServiceStatusReply* response);
    telux::common::Status registerDefaultIndications();
    void notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
        std::string srvStatusStr);
    void setServiceStatus(telux::common::ServiceStatus srvStatus);
    telux::common::ServiceStatus getServiceStatus();
    void onEventUpdate(::eventService::UnsolicitedEvent event);
    void onEventUpdate(std::string event);
    void handleEvent(std::string token,std::string event);
    void handleSSREvent(std::string eventParams);
    void onDeviceInfoEventUpdate(std::string event);
    void onSubsystemEventUpdate(std::string event);
    void onSubsystemEvent(int subsystem, int procType, commonStub::OperationalStatus opStatus);
    void handleOperationalStatusEvent(std::string eventParams);
    bool isValidProcType(int procType);
    bool isValidSubsystem(int subsystem);

    telux::common::ServiceStatus serviceStatus_ =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::mutex mutex_;
    ServerEventManager &serverEvent_;
    EventService &clientEvent_;
    int cbDelay_ = 100;
};
#endif  // DEVICE_INFO_MANAGER_SERVER_HPP