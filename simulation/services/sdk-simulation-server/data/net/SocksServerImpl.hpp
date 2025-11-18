/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SOCKS_MANAGER_SERVER_HPP
#define SOCKS_MANAGER_SERVER_HPP

#include <telux/data/net/SocksManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class SocksServerImpl final:
    public dataStub::SocksManager::Service {
public:
    SocksServerImpl();
    ~SocksServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::InitRequest* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status enableSocks(ServerContext* context,
        const dataStub::EnableSocksRequest* request,
        dataStub::DefaultReply* response) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};

#endif //SOCKS_MANAGER_SERVER_HPP