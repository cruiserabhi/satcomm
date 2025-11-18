/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_CV2X_RADIO_SIMULATION_SERVER_HPP
#define TELUX_CV2X_RADIO_SIMULATION_SERVER_HPP

#include <iostream>
#include <memory>
#include <mutex>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "protos/proto-src/cv2x_simulation.grpc.pb.h"

#include "Cv2xHelperServer.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

#define TRAFFIC_IP (0)
#define TRAFFIC_NON_IP (1)

// Max number of SPS flows supported
static constexpr int32_t SIMULATION_SPS_MAX_NUM_FLOWS = 2u;
// Max number of Non-SPS flows supported
static constexpr int32_t SIMULATION_NON_SPS_MAX_NUM_FLOWS = 255u;
static constexpr uint32_t SIMULATION_EVT_FLOW_BASE = 100u;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using cv2xStub::Cv2xRadioService;

class Cv2xRadioServer final
    : public cv2xStub::Cv2xRadioService::Service,
      public telux::cv2x::ICv2xListener {
public:
  Cv2xRadioServer();
  ~Cv2xRadioServer();
  void init(std::weak_ptr<telux::cv2x::ICv2xListener> self);
  grpc::Status requestCv2xStatus(ServerContext *context,
                                 const google::protobuf::Empty *request,
                                 ::cv2xStub::Cv2xRequestStatusReply *res);
  grpc::Status addRxSubscription(ServerContext *context,
                                 const cv2xStub::RxSubscription *request,
                                 cv2xStub::Cv2xCommandReply *res);
  grpc::Status delRxSubscription(ServerContext *context,
                                 const cv2xStub::RxSubscription *request,
                                 cv2xStub::Cv2xCommandReply *res);
  grpc::Status enableRxMetaDataReport(ServerContext *context,
                                      const cv2xStub::RxSubscription *request,
                                      cv2xStub::Cv2xCommandReply *res);
  grpc::Status registerFlow(ServerContext *context,
                            const cv2xStub::FlowInfo *request,
                            cv2xStub::Cv2xRadioFlowReply *res);
  grpc::Status deregisterFlow(ServerContext *context,
                              const cv2xStub::FlowInfo *request,
                              cv2xStub::Cv2xRadioFlowReply *res);
  grpc::Status updateSrcL2Info(ServerContext *context,
                               const google::protobuf::Empty *request,
                               cv2xStub::Cv2xCommandReply *res);
  grpc::Status updateTrustedUEList(ServerContext *context,
                                   const google::protobuf::Empty *request,
                                   cv2xStub::Cv2xCommandReply *res);
  grpc::Status getIfaceNameFromIpType(ServerContext *context,
                                      const cv2xStub::IpType *request,
                                      cv2xStub::IfaceNameReply *res);
  grpc::Status enableTxStatusReport(ServerContext *context,
                                    const cv2xStub::UintNum *request,
                                    cv2xStub::Cv2xCommandReply *res);
  grpc::Status disableTxStatusReport(ServerContext *context,
                                     const cv2xStub::UintNum *request,
                                     cv2xStub::Cv2xCommandReply *res);
  grpc::Status setGlobalIPInfo(ServerContext *context,
                               const google::protobuf::Empty *request,
                               ::cv2xStub::Cv2xCommandReply *res);
  grpc::Status setGlobalIPUnicastRoutingInfo(ServerContext *context,
                                             const google::protobuf::Empty *request,
                                             ::cv2xStub::Cv2xCommandReply *res);
  grpc::Status requestDataSessionSettings(ServerContext *context,
                                          const google::protobuf::Empty *request,
                                          ::cv2xStub::Cv2xCommandReply *res);
  grpc::Status injectVehicleSpeed(ServerContext *context,
                                  const cv2xStub::UintNum *request,
                                  cv2xStub::Cv2xCommandReply *res);


private:
  void onStatusChanged(telux::cv2x::Cv2xStatus status) override;
  inline ::commonStub::Status
  saveFlowInfo(std::map<uint32_t, cv2xStub::FlowInfo> &flows,
               cv2xStub::FlowInfo &flow, const uint32_t base, const uint32_t max, int32_t &flowId);
  inline ::commonStub::Status
  removeFlowInfo(std::map<uint32_t, cv2xStub::FlowInfo> &flows, int32_t flowId);

  std::shared_ptr<Cv2xServerEvtListener> evtListener_ = nullptr;
  std::vector<cv2xStub::RxSubscription> rxSubsVec_;
  std::mutex rxSubsMtx_;
  std::map<uint32_t, cv2xStub::FlowInfo> spsFlows_;
  std::map<uint32_t, cv2xStub::FlowInfo> evtFlows_;
  std::mutex flowMtx_;

  std::map<uint32_t, bool> txStatusReportEnable_;
  std::mutex txStatusReportMtx_;
};

#endif // TELUX_CV2X_RADIO_SIMULATION_SERVER_HPP
