/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SuppServicesManagerServerImpl.hpp
 *
 */

#ifndef SUPP_SERVICES_MANAGER_SERVER_HPP
#define SUPP_SERVICES_MANAGER_SERVER_HPP

#include <telux/common/CommonDefines.hpp>

#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#include "protos/proto-src/tel_simulation.grpc.pb.h"

#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"

class SuppServicesManagerServerImpl final : public telStub::SuppServicesService::Service,
                                    public IServerEventListener,
                                    public
                                    std::enable_shared_from_this<SuppServicesManagerServerImpl> {

public:
    SuppServicesManagerServerImpl();
    grpc::Status InitService(ServerContext* context,
        const ::commonStub::GetServiceStatusRequest* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status GetServiceStatus(ServerContext* context,
        const ::commonStub::GetServiceStatusRequest* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status CleanUpService(ServerContext* context,
        const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) override;
    grpc::Status SetCallWaitingPref(ServerContext* context,
        const ::telStub::SetCallWaitingPrefRequest* request,
        telStub::SetCallWaitingPrefReply* response) override;
    grpc::Status RequestCallWaitingPref(ServerContext* context,
        const ::telStub::RequestCallWaitingPrefRequest* request,
        telStub::RequestCallWaitingPrefReply* response) override;
    grpc::Status SetForwardingPref(ServerContext* context,
        const ::telStub::SetForwardingPrefRequest* request,
        telStub::SetForwardingPrefReply* response) override;
    grpc::Status RequestForwardingPref(ServerContext* context,
        const ::telStub::RequestForwardingPrefRequest* request,
        telStub::RequestForwardingPrefReply* response) override;
    grpc::Status SetOirPref(ServerContext* context,
        const ::telStub::SetOirPrefRequest* request,
        telStub::SetOirPrefReply* response) override;
    grpc::Status RequestOirPref(ServerContext* context,
        const ::telStub::RequestOirPrefRequest* request,
        telStub::RequestOirPrefReply* response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message) override;

private:
    void onEventUpdate(std::string event);

};

#endif // SUPP_SERVICES_MANAGER_SERVER_HPP
