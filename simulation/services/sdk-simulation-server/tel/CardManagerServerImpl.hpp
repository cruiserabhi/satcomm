/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/**
 * @file       CardManagerServerImpl.hpp
 *
 *
 */

#ifndef CARD_MANAGER_SERVER_HPP
#define CARD_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/CardDefines.hpp>
#include <telux/tel/CardFileHandler.hpp>
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "libs/tel/CardFileHandlerStub.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "libs/common/CommonUtils.hpp"
#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using telStub::CardService;
using commonStub::ServiceStatus;
using commonStub::GetServiceStatusReply;

enum CardRefreshStage {
    REFRESH_STAGE_UNKNOWN = -1,
    WAITING_FOR_VOTES = 0,
    STARTING = 1,
    ENDED_WITH_SUCCESS = 2,
    ENDED_WITH_FAILURE = 3,
};

struct clientSimRefreshPref {
    uint32_t clientId;
    int phoneId;
    telux::tel::RefreshParams sessionAid;
};

struct RefreshEventAndPending {
    ::telStub::RefreshEvent refreshEvent;
    uint32_t pendingAllow;
    uint32_t pendingComplete;
};

class CardManagerServerImpl final : public telStub::CardService::Service,
                                    public IServerEventListener,
                                    public std::enable_shared_from_this<CardManagerServerImpl> {
 public:
    CardManagerServerImpl();
    ~CardManagerServerImpl();
    grpc::Status InitService(ServerContext *context, const google::protobuf::Empty *request,
        commonStub::GetServiceStatusReply* response) override ;
    grpc::Status GetServiceStatus(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status IsSubsystemReady(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::IsSubsystemReadyReply* response) override;
    grpc::Status GetCardState(ServerContext *context, const telStub::GetCardStateRequest *request,
        telStub::GetCardStateReply *reply) override;
    grpc::Status ReadEFLinearFixed(ServerContext* context,
        const telStub::ReadEFLinearFixedRequest* request,
        telStub::ReadEFLinearFixedReply* response) override;
    grpc::Status ReadEFLinearFixedAll(ServerContext* context,
        const telStub::ReadEFLinearFixedAllRequest*
        request, telStub::ReadEFLinearFixedAllReply* response) override;
    grpc::Status ReadEFTransparent(ServerContext* context,
        const telStub::ReadEFTransparentRequest*
        request, telStub::ReadEFTransparentReply* response) override;
    grpc::Status WriteEFLinearFixed(ServerContext* context,
        const telStub::WriteEFLinearFixedRequest*
        request, telStub::WriteEFLinearFixedReply* response) override;
    grpc::Status WriteEFTransparent(ServerContext* context,
        const telStub::WriteEFTransparentRequest*
        request, telStub::WriteEFTransparentReply* response) override;
    grpc::Status RequestEFAttributes(ServerContext* context,
        const telStub::EFAttributesRequest*
        request, telStub::RequestEFAttributesReply* response) override;
    grpc::Status OpenLogicalChannel(ServerContext* context,
        const telStub::OpenLogicalChannelRequest*
        request, telStub::OpenLogicalChannelReply* response) override;
    grpc::Status CloseLogicalChannel(ServerContext* context,
        const telStub::CloseLogicalChannelRequest*
        request, telStub::CloseLogicalChannelReply* response) override;
    grpc::Status TransmitAPDU(ServerContext* context,
        const telStub::TransmitAPDURequest* request,
        telStub::TransmitAPDUReply* response) override;
    grpc::Status TransmitBasicAPDU(ServerContext* context,
        const telStub::TransmitBasicAPDURequest* request,
        telStub::TransmitBasicAPDUReply* response) override;
    grpc::Status exchangeSimIO(ServerContext* context,
        const ::telStub::exchangeSimIORequest* request,
        telStub::exchangeSimIOReply* response) override;
    grpc::Status requestEid(ServerContext* context,
        const ::telStub::requestEidRequest* request,
        telStub::requestEidReply* response) override;
    grpc::Status updateSimStatus(ServerContext* context,
        const ::telStub::updateSimStatusRequest* request,
        telStub::updateSimStatusReply* response) override;
    grpc::Status SetCardLock(ServerContext* context, const telStub::SetCardLockRequest* request,
        telStub::SetCardLockReply* response) override;
    grpc::Status QueryPin1Lock(ServerContext* context,
        const telStub::QueryPin1LockRequest* request,
        telStub::QueryPin1LockReply* response) override;
    grpc::Status ChangePinLock(ServerContext* context,
        const telStub::ChangePinLockRequest* request,
        telStub::ChangePinLockReply* response) override;
    grpc::Status UnlockByPin(ServerContext* context, const telStub::UnlockByPinRequest* request,
        telStub::UnlockByPinReply* response) override;
    grpc::Status UnlockByPuk(ServerContext* context, const telStub::UnlockByPukRequest* request,
        telStub::UnlockByPukReply* response) override;
    grpc::Status QueryFdnLock(ServerContext* context, const telStub::QueryFdnLockRequest* request,
        telStub::QueryFdnLockReply* response) override;
    grpc::Status CardPower(ServerContext* context, const ::telStub::CardPowerRequest* request,
        telStub::CardPowerResponse* response) override;
    grpc::Status IsNtnProfileActive(ServerContext* context,
        const ::telStub::IsNtnProfileActiveRequest* request,
        telStub::IsNtnProfileActiveReply* response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

    ::grpc::Status setupRefreshConfig(ServerContext* context,
        const ::telStub::RefreshConfigReq* request, telStub::TelCommonReply* response);
    ::grpc::Status allowCardRefresh(ServerContext* context,
        const ::telStub::AllowCardRefreshReq* request, telStub::TelCommonReply* response);
    ::grpc::Status confirmRefreshHandlingCompleted(ServerContext* context,
        const ::telStub::ConfirmRefreshHandlingCompleteReq* request,
        telStub::TelCommonReply* response);
    ::grpc::Status requestLastRefreshEvent(ServerContext* context,
        const ::telStub::RequestLastRefreshEventReq* request,
        telStub::RequestLastRefreshEventResp* response);

    template <typename T>
    commonStub::ErrorCode findmatchingrecordADF (Json::Value rootObj, T response,
    int& size, int& index, int& recordNum, uint16_t& fileId, int& i) {
        commonStub::ErrorCode error = commonStub::ErrorCode::ERROR_CODE_SUCCESS;
        while (i < size ) {
        uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
            [i]["fileId"].asInt();
        if (tmpfileId == fileId) {
            int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i]\
                ["numberOfRecords"].asInt();
            LOG(DEBUG, __FUNCTION__,"NumberOfRecords ", num);
            if(recordNum <= num ) {
                error = commonStub::ErrorCode::ERROR_CODE_SUCCESS;
                break;
            } else {
                error = commonStub::ErrorCode::GENERIC_FAILURE;
                LOG(DEBUG, __FUNCTION__, "Invalid Record");
                break;
            }
        } else {
            LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
            int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i]\
                ["numberOfRecords"].asInt();
            i = i + num + 1;
            LOG(DEBUG, __FUNCTION__,"Incremented value is ", i );
        }
    }
    if(i == size) {
        LOG(DEBUG, __FUNCTION__,"Valid record not found ", i );
        error = commonStub::ErrorCode::GENERIC_FAILURE;
    }
    return error;
    }
 private:
    Json::Value rootObjSystemStateSlot1_;
    Json::Value rootObjSystemStateSlot2_;
    Json::Value rootObjApiResponseSlot1_;
    Json::Value rootObjApiResponseSlot2_;
    std::map <int, Json::Value> jsonObjSystemStateSlot_;
    std::map <int, std::string> jsonObjSystemStateFileName_;
    std::map <int, Json::Value> jsonObjApiResponseSlot_;
    std::map <int, std::string> jsonObjApiResponseFileName_;
    std::map <int, RefreshEventAndPending> refreshEvtMap_;
    std::vector <clientSimRefreshPref> refreshRegisterClients_;
    std::vector <clientSimRefreshPref> refreshVotingClients_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    std::condition_variable cv_;
    std::mutex mutex_;
    bool exit_ = false;

    grpc::Status readJson();
    bool isCallbackNeeded(Json::Value rootObj, std::string apiname);
    bool findAppId(Json::Value rootObj, const char* appid, int& index);
    commonStub::ErrorCode findmatchingrecordDF (Json::Value rootObj, int& size, int& recordNum,
        uint16_t& fileId, int& i);
    commonStub::ErrorCode getTransparentFileAttributes(Json::Value rootObj,
        int& i, uint16_t fileId,
        telux::tel::FileAttributes& attributes, int& index);
    commonStub::ErrorCode getLinearfixedFileAttributes(Json::Value rootObj,
        int& i, uint16_t fileId,
        telux::tel::FileAttributes& attributes, int& index );
    void getJsonForSystemData (int phoneId, std::string& jsonfilename, Json::Value& rootObj );
    void getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
        Json::Value& rootObj );
    void getApiConfigureFromJson(const int slotId, const std::string apiname,
        telux::common::Status& status, telux::common::ErrorCode& ec, int& delay);
    void handleEvent(std::string token , std::string event);
    bool handleCardInfoChanged(std::string eventParams,
        ::eventService::EventResponse& notification);
    void onEventUpdate(std::string event);
    bool handleSimRefreshInjector(std::string eventParams,
        ::eventService::EventResponse& notification);
    void updateSimRefreshStage(int slotId, CardRefreshStage newStage, uint32_t delayMs,
        bool checkPendingUserAllow, bool checkPendingUserComplete);
    int getSlotBySessionType(telux::tel::SessionType st);
    bool requireConfirmComplete(const CardRefreshStage stage, const telux::tel::RefreshMode mode,
        const telux::tel::SessionType st);
    bool clientSimRefreshInfoPresent(std::vector <clientSimRefreshPref>& vector,
        const clientSimRefreshPref& entry);
    telux::common::ErrorCode updateClientSimRefresh(std::vector <clientSimRefreshPref>& vector,
        const clientSimRefreshPref& usrPref, bool enable);
    template <typename T>
    void getClientInfoFromRpc(const T* rpcMsg, clientSimRefreshPref& client);
};

#endif // CARD_MANAGER_SERVER_HPP
