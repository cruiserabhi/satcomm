/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOGRPCSERVICETIMPL_HPP
#define AUDIOGRPCSERVICETIMPL_HPP

#include <cstdlib>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "protos/proto-src/audio_simulation.grpc.pb.h"
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#include <telux/audio/AudioDefines.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/audio/AudioListener.hpp>
#include <telux/audio/AudioManager.hpp>
#include "AudioServiceImpl.hpp"
#include "AudioJsonHelper.hpp"
#include "AudioDefinesInternal.hpp"

namespace telux {
namespace audio {

class AudioGrpcServiceImpl : public IAudioMsgDispatcher,
                             public ::audioStub::AudioService::Service,
                             public std::enable_shared_from_this<AudioGrpcServiceImpl> {
 public:
    AudioGrpcServiceImpl();
    ~AudioGrpcServiceImpl();

    grpc::Status ClientConnected(::grpc::ServerContext* context, const
    ::audioStub::AudioClientConnect* request, ::commonStub::GetServiceStatusReply* response);

    telux::common::ErrorCode onClientProcessReq(const ::audioStub::AudioRequest* request);

    grpc::Status GetStreamTypes(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);
    grpc::Status SetupAsyncResponseStream(::grpc::ServerContext* context,
        const ::audioStub::AudioClientConnect* request,
            ::grpc::ServerWriter< ::audioStub::AsyncResponseMessage>* writer);
    grpc::Status ClientDisconnected(::grpc::ServerContext* context, const
        ::audioStub::AudioClientDisconnect* request, ::google::protobuf::Empty* response);
    grpc::Status GetDevices(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);
    grpc::Status GetCalibrationInitStatus(::grpc::ServerContext* context, const
        ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response);

    grpc::Status CreateStream(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);
    grpc::Status StartAudio(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);

    grpc::Status StopAudio(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);

    grpc::Status PlayDtmfTone(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);
    grpc::Status StopDtmfTone(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);
    grpc::Status GetStreamDevices(grpc::ServerContext* context, const
        ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response);
    grpc::Status SetStreamDevices(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);
    grpc::Status GetStreamMuteStatus(::grpc::ServerContext* context, const
        ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response);
    grpc::Status SetStreamMute(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);
    grpc::Status GetStreamVolume(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);
    grpc::Status SetStreamVolume(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response);

    grpc::Status DeleteStream(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
    request, ::commonStub::StatusMsg* response);

    grpc::Status Write(::grpc::ServerContext* context, const ::audioStub::AudioRequest* request,
        ::commonStub::StatusMsg* response);

    grpc::Status Read(::grpc::ServerContext* context, const ::audioStub::AudioRequest* request,
        ::commonStub::StatusMsg* response);

    grpc::Status PlayTone(::grpc::ServerContext* context, const ::audioStub::AudioRequest* request,
        ::commonStub::StatusMsg* response);

    grpc::Status StopTone(::grpc::ServerContext* context, const ::audioStub::AudioRequest* request,
        ::commonStub::StatusMsg* response);

    grpc::Status CreateTranscoder(::grpc::ServerContext* context,
        const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response);

    grpc::Status DeleteTranscoder(::grpc::ServerContext* context,
        const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response);

    grpc::Status Flush(::grpc::ServerContext* context, const ::audioStub::AudioRequest* request,
        ::commonStub::StatusMsg* response);

    grpc::Status Drain(::grpc::ServerContext* context, const ::audioStub::AudioRequest* request,
        ::commonStub::StatusMsg* response);

    /*** Overrides - IAudioMsgDispatcher ***/

    void broadcastServiceStatus(uint32_t newStatus) override;

    void sendGetSupportedDevicesResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            std::vector<DeviceType> const &devices,
            std::vector<DeviceDirection> const &devicesDirection) override;

    void sendGetSupportedStreamTypesResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            std::vector<StreamType> const &streamTypes) override;

    void sendCreateStreamResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            uint32_t streamId, StreamType streamType, uint32_t readMinSize,
            uint32_t writeMinSize) override;

    void sendDeleteStreamResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendStartResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendStopResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendSetDeviceResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendGetDeviceResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId,
            std::vector<DeviceType> const &devices) override;

    void sendSetVolumeResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendGetVolumeResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            uint32_t streamId, StreamDirection direction,
            std::vector<ChannelVolume> channelsVolume) override;

    void sendSetMuteStateResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendGetMuteStateResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId,
            StreamMute muteInfo) override;

    void sendReadResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            uint32_t streamId, std::shared_ptr<std::vector<uint8_t>> data,
            uint32_t dataLength, uint32_t offset, int64_t timeStamp, bool isIncallStream,
            bool isHpcmStream) override;

    void sendWriteResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            uint32_t streamId, uint32_t dataLength, bool isIncallStream,
            bool isHpcmStream) override;

    void sendStartDtmfResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendStopDtmfResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendStartToneResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendStopToneResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendDrainResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendFlushResponse(
            std::shared_ptr<AudioRequest> audioReq,
            telux::common::ErrorCode ec, uint32_t streamId) override;

    void sendCreateTranscoderResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            CreatedTranscoderInfo createdTranscoderInfo);

    void sendDeleteTranscoderResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            uint32_t inStreamId, uint32_t outStreamId);

    void sendGetCalibrationStatusResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            CalibrationInitStatus status) override;

    void sendDrainDoneEvent(
        int clientId, uint32_t streamId) override;

    void sendWriteReadyEvent(
        int clientId, uint32_t streamId) override;

    void sendDTMFDetectedEvent(
        int clientId,
        uint32_t streamId, uint32_t lowFreq, uint32_t highFreq,
        StreamDirection streamDirection) override;

 private:
    std::shared_ptr<AudioServiceImpl> audioService_;
    telux::common::ServiceStatus serviceStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

    void getSupportedDevices(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void getSupportedStreamTypes(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void createStream(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void deleteStream(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void createTranscoder(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void deleteTranscoder(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void start(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void stop(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void setDevice(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void getDevice(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void setVolume(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void getVolume(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void setMuteState(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void getMuteState(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void startDtmf(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void stopDtmf(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void getCalibrationStatus(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void write(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void read(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener);

   void startTone(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void stopTone(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void drain(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    void flush(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

    std::mutex streamWriterMtx_;
    std::condition_variable cv_;
    std::shared_ptr<AudioJsonHelper> jsonHelper_;
    std::weak_ptr<IAudioMsgListener> audioMsgListener_;
    std::unordered_map<int, grpc::ServerWriter<audioStub::AsyncResponseMessage>*> serverStreamMap_;
};

} // end namespace audio
} // end namespace telux

#endif  // AUDIOGRPCSERVICETIMPL_HPP
