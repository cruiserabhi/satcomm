/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       Cv2xConfigServerImpl.hpp
 *
 *
 */

#ifndef CV2X_CONFIG_SERVER_HPP
#define CV2X_CONFIG_SERVER_HPP

#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "protos/proto-src/cv2x_simulation.grpc.pb.h"

#include "event/ServerEventManager.hpp"
#include "common/AsyncTaskQueue.hpp"
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using cv2xStub::Cv2xConfigService;

class Cv2xConfigServerImpl final : public cv2xStub::Cv2xConfigService::Service {
public:
  Cv2xConfigServerImpl();
  ~Cv2xConfigServerImpl();
  grpc::Status initService(ServerContext *context,
                           const google::protobuf::Empty *request,
                           cv2xStub::GetServiceStatusReply *res);
  grpc::Status updateConfiguration(ServerContext *context,
                                   const cv2xStub::Cv2xConfigPath *request,
                                   ::cv2xStub::Cv2xCommandReply *res);
  grpc::Status retrieveConfiguration(ServerContext *context,
                                     const cv2xStub::Cv2xConfigPath *request,
                                     ::cv2xStub::Cv2xCommandReply *res);

private:
  std::string path_;
  telux::common::AsyncTaskQueue<void> taskQ_;
};

#endif // CV2X_CONFIG_SERVER_HPP
