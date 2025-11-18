/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ServingManagerServerImpl.hpp
 *
 */

#ifndef SERVING_SYSTEM_MANAGER_SERVER_HPP
#define SERVING_SYSTEM_MANAGER_SERVER_HPP

#include "protos/proto-src/tel_simulation.grpc.pb.h"

#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"

#include <telux/tel/ServingSystemManager.hpp>

class ServingManagerServerImpl final : public telStub::ServingSystemService::Service,
                                       public IServerEventListener,
                                       public
                                       std::enable_shared_from_this<ServingManagerServerImpl> {

public:
    ServingManagerServerImpl();
    ~ServingManagerServerImpl();
    grpc::Status InitService(ServerContext* context,
        const ::commonStub::GetServiceStatusRequest* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status GetServiceStatus(ServerContext* context,
        const ::commonStub::GetServiceStatusRequest* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status RequestRATPreference(ServerContext* context,
        const ::telStub::RequestRATPreferenceRequest* request,
        telStub::RequestRATPreferenceReply* response) override;
    grpc::Status SetRATPreference(ServerContext* context,
        const ::telStub::SetRATPreferenceRequest* request,
        telStub::SetRATPreferenceReply* response) override;
    grpc::Status RequestServiceDomainPreference(ServerContext* context,
        const ::telStub::RequestServiceDomainPreferenceRequest* request,
        telStub::RequestServiceDomainPreferenceReply* response) override;
    grpc::Status SetServiceDomainPreference(ServerContext* context,
        const ::telStub::SetServiceDomainPreferenceRequest* request,
        telStub::SetServiceDomainPreferenceReply* response) override;
    grpc::Status GetSystemInfo(ServerContext* context,
        const ::telStub::GetSystemInfoRequest* request,
        telStub::GetSystemInfoReply* response) override;
    grpc::Status GetDcStatus(ServerContext* context,
        const ::telStub::GetDcStatusRequest* request,
        telStub::GetDcStatusReply* response) override;
    grpc::Status RequestNetworkTime(ServerContext* context,
        const ::telStub::RequestNetworkTimeRequest* request,
        telStub::RequestNetworkTimeReply* response) override;
    grpc::Status RequestRFBandInfo(ServerContext* context,
        const ::telStub::RequestRFBandInfoRequest* request,
        telStub::RequestRFBandInfoReply* response) override;
    grpc::Status GetNetworkRejectInfo(ServerContext* context,
        const ::telStub::GetNetworkRejectInfoRequest* request,
        telStub::GetNetworkRejectInfoReply* response) override;
    grpc::Status GetCallBarringInfo(ServerContext* context,
        const ::telStub::GetCallBarringInfoRequest* request,
        telStub::GetCallBarringInfoReply* response) override;
    grpc::Status GetSmsCapabilityOverNetwork(ServerContext* context,
        const ::telStub::GetSmsCapabilityOverNetworkRequest* request,
        telStub::GetSmsCapabilityOverNetworkReply* response);
    grpc::Status GetLteCsCapability(ServerContext* context,
        const ::telStub::GetLteCsCapabilityRequest* request,
        telStub::GetLteCsCapabilityReply* response) override;
    grpc::Status RequestRFBandPreferences(ServerContext* context,
        const ::telStub::RequestRFBandPreferencesRequest* request,
        telStub::RequestRFBandPreferencesReply* response) override;
    grpc::Status SetRFBandPreferences(ServerContext* context,
        const ::telStub::SetRFBandPreferencesRequest* request,
        telStub::SetRFBandPreferencesReply* response) override;
    grpc::Status RequestRFBandCapability(ServerContext* context,
        const ::telStub::RequestRFBandCapabilityRequest* request,
        telStub::RequestRFBandCapabilityReply* response) override;
    grpc::Status CleanUpService(ServerContext* context,
        const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message) override;
    void onServerEvent(google::protobuf::Any event) override;

private:
    void triggerChangeEvent(::eventService::EventResponse anyResponse);
    void onEventUpdate(std::string event);
    void handleSystemSelectionPreferenceChanged(std::string eventParams);
    void handleSystemInfoUpdateEvent(std::string eventParams);
    void handleNetworkTimeUpdateEvent(std::string eventParams);
    void handleRfBandInfoUpdateEvent(std::string eventParams);
    void handleNetworkRejectionUpdateEvent(std::string eventParams);
    void triggerSystemSelectionPreferenceEvent(int slotId,
        std::vector<uint8_t> ratPrefs, int domain,
        std::vector<int> gsmBandPrefs,std::vector<int> wcdmaBandPrefs,
        std::vector<int> lteBandPrefs,std::vector<int> nsaBandPrefs,
        std::vector<int> saBandPrefs);
    std::vector<int> readBandPreferenceFromEvent(std::string eventParams);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

};

#endif // SERVING_SYSTEM_MANAGER_SERVER_HPP
