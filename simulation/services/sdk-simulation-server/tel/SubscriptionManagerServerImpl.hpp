/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @file       SubscriptionManagerServerImpl.hpp
 *             It handles solicited requests and formulates responses to get the subscription
 *             information and updates new subscription information injected by event injector
 *             utility.
 *
 */

#ifndef SUBSCRIPTION_MANAGER_SERVER_HPP
#define SUBSCRIPTION_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "event/ServerEventManager.hpp"
#include "libs/common/CommonUtils.hpp"
#include "event/EventService.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using telStub::SubscriptionService;
using commonStub::ServiceStatus;

class SubscriptionManagerServerImpl final : public telStub::SubscriptionService::Service,
                                            public IServerEventListener,
                                            public
                                    std::enable_shared_from_this<SubscriptionManagerServerImpl> {

public:
    SubscriptionManagerServerImpl();
    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status GetServiceStatus(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status IsSubsystemReady(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::IsSubsystemReadyReply* response) override;
    grpc::Status GetSubscription(ServerContext* context,
        const ::telStub::GetSubscriptionRequest* request,
        telStub::Subscription* response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message);
private:
    Json::Value rootObj;
    grpc::Status readJson();
    void handleEvent(std::string token , std::string event);
    void handlesubscriptionInfoChanged(std::string eventParams);
    void onEventUpdate(std::string event);
};

#endif // SUBSCRIPTION_MANAGER_SERVER_HPP
