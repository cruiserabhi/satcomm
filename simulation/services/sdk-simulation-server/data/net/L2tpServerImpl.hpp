/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef L2TP_MANAGER_SERVER_HPP
#define L2TP_MANAGER_SERVER_HPP

#include <telux/data/net/L2tpManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class L2tpServerImpl final:
    public dataStub::L2tpManager::Service {
public:
    L2tpServerImpl();
    ~L2tpServerImpl();

    grpc::Status InitService(ServerContext* context,
        const google::protobuf::Empty* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status SetConfig(ServerContext* context,
        const dataStub::SetConfigRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestConfig(ServerContext* context,
        const google::protobuf::Empty* request,
        dataStub::RequestConfigReply* response) override;

    grpc::Status AddTunnel(ServerContext* context,
        const ::dataStub::AddTunnelRequest* request,
        ::dataStub::DefaultReply* response) override;

    grpc::Status RemoveTunnel(ServerContext* context,
        const ::dataStub::RemoveTunnelRequest* request,
        ::dataStub::DefaultReply* response) override;

    grpc::Status AddSession(ServerContext* context,
        const ::dataStub::AddSessionRequest* request,
        ::dataStub::DefaultReply* response) override;

    grpc::Status RemoveSession(ServerContext* context,
        const ::dataStub::RemoveSessionRequest* request,
        ::dataStub::DefaultReply* response) override;

    grpc::Status BindSessionToBackhaul(ServerContext* context,
        const dataStub::SessionConfigRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status UnBindSessionToBackhaul(ServerContext* context,
        const dataStub::SessionConfigRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status QueryBindSessionToBackhaul(ServerContext* context,
        const dataStub::QueryBindSessionRequest* request,
        dataStub::QueryBindSessionReply* response) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

    bool configExists(const Json::Value& config, int configID, int &idx);
    bool bindingExists(const Json::Value& bindingsObj,
        const dataStub::SessionConfigRequest* request, int &idx);
    bool isL2tpEnabled(const JsonData& data);
};

#endif //L2TP_MANAGER_SERVER_HPP