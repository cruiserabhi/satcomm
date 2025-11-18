/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_PROFILE_SERVER_HPP
#define DATA_PROFILE_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using dataStub::DataProfileManager;

class DataProfileServerImpl final:
    public dataStub::DataProfileManager::Service {
public:
    DataProfileServerImpl();
    ~DataProfileServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::SlotInfo* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status RequestProfileById(ServerContext* context,
        const dataStub::RequestProfileByIdRequest* request,
        dataStub::RequestProfileByIdReply* response) override;

    grpc::Status CreateProfile(ServerContext* context,
        const dataStub::CreateProfileRequest* request,
        dataStub::CreateProfileReply* response) override;

    grpc::Status DeleteProfile(ServerContext* context,
        const dataStub::DeleteProfileRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status ModifyProfile(ServerContext* context,
        const dataStub::ModifyProfileRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestProfileList(ServerContext* context,
        const dataStub::RequestProfileListRequest* request,
        dataStub::RequestProfileListReply* response) override;

    grpc::Status QueryProfile(ServerContext* context,
        const dataStub::QueryProfileRequest* request,
        dataStub::QueryProfileReply* response) override;
};

#endif //DATA_PROFILE_SERVER_HPP