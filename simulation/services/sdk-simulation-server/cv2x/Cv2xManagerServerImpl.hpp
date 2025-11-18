/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       Cv2xManagerServerImpl.hpp
 *
 *
 */

#ifndef CV2X_MANAGER_SERVER_HPP
#define CV2X_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "protos/proto-src/cv2x_simulation.grpc.pb.h"

#include <telux/cv2x/Cv2xRadioManager.hpp>

#include "Cv2xHelperServer.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using cv2xStub::Cv2xManagerService;

class Cv2xManagerServerImpl final
    : public cv2xStub::Cv2xManagerService::Service {
public:
  Cv2xManagerServerImpl();
  ~Cv2xManagerServerImpl();
  grpc::Status initService(ServerContext *context,
                           const google::protobuf::Empty *request,
                           cv2xStub::GetServiceStatusReply *res);
  grpc::Status startCv2x(ServerContext *context,
                         const google::protobuf::Empty *request,
                         cv2xStub::Cv2xCommandReply *res);
  grpc::Status stopCv2x(ServerContext *context,
                        const google::protobuf::Empty *request,
                        cv2xStub::Cv2xCommandReply *res);
  grpc::Status setPeakTxPower(ServerContext *context,
                              const cv2xStub::Cv2xPeakTxPower *request,
                              ::cv2xStub::Cv2xCommandReply *res);
  grpc::Status requestCv2xStatus(ServerContext *context,
                                 const google::protobuf::Empty *request,
                                 ::cv2xStub::Cv2xRequestStatusReply *res);
  grpc::Status injectCoarseUtcTime(ServerContext *context,
                                   const ::cv2xStub::CoarseUtcTime *request,
                                   ::cv2xStub::Cv2xCommandReply *res);
  grpc::Status getSlssRxInfo(ServerContext *context,
                             const google::protobuf::Empty *request,
                             ::cv2xStub::SlssRxInfoReply *res);
  grpc::Status setL2Filters(ServerContext *context,
                            const ::cv2xStub::L2FilterInfos *request,
                            ::cv2xStub::Cv2xCommandReply *res);
  grpc::Status removeL2Filters(ServerContext *context,
                               const ::cv2xStub::L2Ids *request,
                               ::cv2xStub::Cv2xCommandReply *res);

private:
  std::shared_ptr<Cv2xServerEvtListener> evtListener_ = nullptr;
};

#endif // CV2X_MANAGER_SERVER_HPP
