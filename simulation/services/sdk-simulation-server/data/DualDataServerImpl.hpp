/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DUAL_DATA_MANAGER_SERVER_HPP
#define DUAL_DATA_MANAGER_SERVER_HPP

#include <telux/data/DualDataManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"
#include "event/ServerEventManager.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class DualDataServerImpl final:
    public dataStub::DualDataManager::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<DualDataServerImpl> {
public:
    DualDataServerImpl();
    ~DualDataServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::InitRequest* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status GetDualDataCapability(ServerContext* context,
        const ::google::protobuf::Empty* request,
        dataStub::GetDualDataCapabilityReply* response) override;

    grpc::Status GetDualDataUsageRecommendation(ServerContext* context,
        const ::google::protobuf::Empty* request,
        dataStub::GetDualDataUsageRecommendationReply* response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

    ::dataStub::UsageRecommendation::Recommendation
        convertUsageRecommendationStringToEnum(std::string recommendation);

    void onEventUpdate(std::string event);
    void handleCapabilityChangeRequest(std::string event);
    void handleRecommendationChnageRequest(std::string event);
};

#endif //DUAL_DATA_MANAGER_SERVER_HPP