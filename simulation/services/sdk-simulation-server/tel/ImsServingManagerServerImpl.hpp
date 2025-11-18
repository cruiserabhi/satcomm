/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ImsServingManagerServerImpl.hpp
 *
 */

#ifndef IMS_SERVING_SYSTEM_MANAGER_SERVER_HPP
#define IMS_SERVING_SYSTEM_MANAGER_SERVER_HPP

#include <telux/common/CommonDefines.hpp>

#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#include "protos/proto-src/tel_simulation.grpc.pb.h"

#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"
#include <thread>

class ImsServingManagerServerImpl final : public telStub::ImsServingSystem::Service,
                                          public IServerEventListener,
                                          public
                                    std::enable_shared_from_this<ImsServingManagerServerImpl> {

public:
    ImsServingManagerServerImpl();
    ~ImsServingManagerServerImpl();
    grpc::Status InitService(ServerContext* context,
        const ::commonStub::GetServiceStatusRequest* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status GetServiceStatus(ServerContext* context,
        const ::commonStub::GetServiceStatusRequest* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status RequestRegistrationInfo(ServerContext* context,
        const ::telStub::RequestRegistrationInfoRequest* request,
        telStub::RequestRegistrationInfoReply* response) override;
    grpc::Status RequestServiceInfo(ServerContext* context,
        const ::telStub::RequestServiceInfoRequest* request,
        telStub::RequestServiceInfoReply* response) override;
    grpc::Status RequestPdpStatus(ServerContext* context,
    const ::telStub::RequestPdpStatusRequest* request,
        telStub::RequestPdpStatusReply* response) override;
    grpc::Status CleanUpService(ServerContext* context,
        const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message) override;

private:
    void handleImsRegStatusChanged(std::string eventParams);
    void handleImsServiceInfoChanged(std::string eventParams);
    void handleImsPdpStatusInfoChanged(std::string eventParams);
    void triggerChangeEvent(::eventService::EventResponse anyResponse);
    void onEventUpdate(std::string event);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};

#endif // IMS_SERVING_SYSTEM_MANAGER_SERVER_HPP