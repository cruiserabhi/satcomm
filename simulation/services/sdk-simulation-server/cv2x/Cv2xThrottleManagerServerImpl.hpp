/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       Cv2xThrottleManagerServerImpl.hpp
 *
 *
 */

#ifndef CV2X_THROTTLE_MANAGER_SERVER_HPP
#define CV2X_THROTTLE_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <telux/cv2x/Cv2xThrottleManager.hpp>

#include "protos/proto-src/cv2x_simulation.grpc.pb.h"

#include "event/ServerEventManager.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "libs/common/AsyncTaskQueue.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using cv2xStub::Cv2xThrottleManagerService;

class Cv2xThrottleManagerServerImpl final
    : public cv2xStub::Cv2xThrottleManagerService::Service,
      public IServerEventListener,
      public std::enable_shared_from_this<Cv2xThrottleManagerServerImpl> {
public:
  Cv2xThrottleManagerServerImpl();
  ~Cv2xThrottleManagerServerImpl();
  grpc::Status initService(ServerContext *context,
                           const google::protobuf::Empty *request,
                           cv2xStub::GetServiceStatusReply *res);

  grpc::Status getServiceStatus(ServerContext *context,
                                const google::protobuf::Empty *request,
                                cv2xStub::GetServiceStatusReply *res);

  grpc::Status setVerificationLoad(ServerContext *context,
                                   const cv2xStub::UintNum *request,
                                   ::cv2xStub::Cv2xCommandReply *res);
  void onEventUpdate(::eventService::UnsolicitedEvent event) override;

private:
  void onEventUpdate(std::string event);
  void handleEvent(std::string token, std::string event);
  void handleFilterUpdateEvent(std::string event);
  void handleSanityUpdateEvent(std::string event);

  telux::common::ServiceStatus serviceStatus_ =
      telux::common::ServiceStatus::SERVICE_FAILED;

  std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

};

#endif // CV2X_THROTTLE_MANAGER_SERVER_HPP
