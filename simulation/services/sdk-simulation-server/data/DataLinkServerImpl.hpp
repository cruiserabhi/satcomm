/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_LINK_SERVER_HPP
#define DATA_LINK_SERVER_HPP

#include <telux/data/DataLinkManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"
#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"

#define DEFAULT_DELIMITER " "

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using dataStub::DataLinkManager;
using commonStub::ServiceStatus;
using commonStub::GetServiceStatusReply;

class DataLinkServerImpl final:
    public dataStub::DataLinkManager::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<DataLinkServerImpl> {

public:
    DataLinkServerImpl();
    ~DataLinkServerImpl();

    grpc::Status InitService(ServerContext* context,
        const google::protobuf::Empty *request,
        commonStub::GetServiceStatusReply* response) override;

    grpc::Status SetEthDataLinkState(ServerContext* context,
            const dataStub::SetEthDatalinkStateRequest *request,
            dataStub::SetEthDatalinkStateReply* response) override;

    grpc::Status GetServiceStatus(ServerContext* context, const google::protobuf::Empty* request,
            commonStub::GetServiceStatusReply* response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent event);

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    ServerEventManager &serverEvent_;
    EventService &clientEvent_;
    std::mutex mutex_;
    telux::common::ServiceStatus serviceStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

    telux::common::Status registerDefaultIndications();
    void onSSREvent(telux::common::ServiceStatus srvStatus);
    void notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
            std::string srvStatusStr);
    telux::common::ServiceStatus getServiceStatus();
    void setServiceStatus(telux::common::ServiceStatus srvStatus);
    void onEventUpdate(std::string event);
    void handleEvent(std::string token,std::string event);
    void handleSSREvent(std::string eventParams);

    grpc::Status setResponse(telux::common::ServiceStatus srvStatus,
            commonStub::GetServiceStatusReply* response);
};

#endif //DATA_LINK_SERVER_HPP
