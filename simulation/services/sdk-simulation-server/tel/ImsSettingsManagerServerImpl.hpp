/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ImsSettingsManagerServerImpl.hpp
 *
 */

#ifndef IMS_SETTINGS_MANAGER_SERVER_HPP
#define IMS_SETTINGS_MANAGER_SERVER_HPP

#include <telux/common/CommonDefines.hpp>

#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#include "protos/proto-src/tel_simulation.grpc.pb.h"

#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"

class ImsSettingsManagerServerImpl final : public telStub::ImsService::Service,
                                     public IServerEventListener,
                                     public
                                     std::enable_shared_from_this<ImsSettingsManagerServerImpl> {

public:
    ImsSettingsManagerServerImpl();
    grpc::Status InitService(ServerContext* context,
        const ::commonStub::GetServiceStatusRequest* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status GetServiceStatus(ServerContext* context,
        const ::commonStub::GetServiceStatusRequest* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status RequestServiceConfig(ServerContext* context,
        const ::telStub::RequestServiceConfigRequest* request,
        telStub::RequestServiceConfigReply* response) override;
    grpc::Status SetServiceConfig(ServerContext* context,
        const ::telStub::SetServiceConfigRequest* request,
        telStub::SetServiceConfigReply* response) override;
    grpc::Status RequestSipUserAgent(ServerContext* context,
        const ::telStub::RequestSipUserAgentRequest* request,
        telStub::RequestSipUserAgentReply* response) override;
    grpc::Status SetSipUserAgent(ServerContext* context,
        const ::telStub::SetSipUserAgentRequest* request,
        telStub::SetSipUserAgentReply* response) override;
    grpc::Status RequestVonr(ServerContext* context,
        const ::telStub::RequestVonrRequest* request,
        telStub::RequestVonrReply* response) override;
    grpc::Status SetVonr(ServerContext* context,
        const ::telStub::SetVonrRequest* request,
        telStub::SetVonrReply* response) override;
    grpc::Status CleanUpService(ServerContext* context,
        const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message) override;

private:
    void handleImsServiceConfigsChange(std::string eventParams);
    void handleImsSipUserAgentChange(std::string eventParams);
    void onEventUpdate(std::string event);
    void triggerImsServiceConfigsChange(int slotId, bool prevImsServiceEnabled,
        bool imsServiceEnabled, bool prevVoImsEnabled,  bool voImsEnabled, bool prevSmsEnabled,
        bool smsEnabled, bool prevRttEnabled, bool rttEnabled);
    void triggerImsSipUserAgentChange(int slotId, std::string prevSipUserAgent,
        std::string sipUserAgent);
};

#endif // IMS_SETTINGS_MANAGER_SERVER_HPP
