/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ANTENNA_MANAGER_SERVER_HPP
#define ANTENNA_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

#include "../event/ServerEventManager.hpp"
#include "../event/EventService.hpp"
#include "protos/proto-src/platform_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using platformStub::AntennaManagerService;

class AntennaManagerServerImpl final :
    public IServerEventListener,
    public platformStub::AntennaManagerService::Service,
    public std::enable_shared_from_this<AntennaManagerServerImpl> {
 public:
    AntennaManagerServerImpl();
    ~AntennaManagerServerImpl();

    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response);

    grpc::Status GetServiceStatus(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;

    grpc::Status GetActiveAntenna(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::DefaultReply* response);

    grpc::Status SetActiveAntenna(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::DefaultReply* response);

 private:
    void apiJsonReader(std::string apiName, platformStub::DefaultReply* response);
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

    telux::common::ServiceStatus serviceStatus_ =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::mutex mutex_;
    ServerEventManager &serverEvent_;
    EventService &clientEvent_;
    int cbDelay_ = 100;
};
#endif  // ANTENNA_MANAGER_SERVER_HPP