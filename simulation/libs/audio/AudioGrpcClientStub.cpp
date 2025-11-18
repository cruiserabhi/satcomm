/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "AudioGrpcClientStub.hpp"

/* If the callback delay for a request is set to -1 in the api json, then it indicates that the
 * client is not waiting for the callback.
 */
#define SKIP_CALLBACK -1

using namespace telux::common;

namespace telux {
namespace audio {

std::atomic<bool> AudioGrpcClientStub::exitNow_;
std::mutex AudioGrpcClientStub::destructorGuard_;

/*
 * AudioResponseHandler is a function pointer, pointing to a member function of
 * AudioGrpcClientStub class.
 */
using AudioResponseHandler = void (telux::audio::AudioGrpcClientStub::*)(
            google::protobuf::Any any, int cmdId, ErrorCode err);

/*
 * Response handler look up corresponding to the operation performed on the server side.
 * There are total 26 response types from 0x0001 to 0x001A. Extra 1 (in 26 + 1) is
 * because array indexing starts from 0 not from 1. GRPC response message ID numbering
 * starts from 1.
 */
AudioResponseHandler opLookup[26 + 1];

/*
 * Install audio response handlers at their designated array offset. Offset is defined
 * by Grpc message ID.
 */
AudioGrpcClientStub::AudioGrpcClientStub() {
   LOG(DEBUG, __FUNCTION__);
   stub_ = CommonUtils::getGrpcStub<AudioService>();

    /* This maps GRPC response message ID to the corresponding audio operation. If this
     * lookup is modified, size of opLookup_ array should also be updated accordingly. */
    opLookup[GET_SUPPORTED_DEVICES_RESP] = &AudioGrpcClientStub::onGetDevices;
    opLookup[GET_SUPPORTED_STREAMS_RESP] = &AudioGrpcClientStub::onGetStreamTypes;
    opLookup[CREATE_STREAM_RESP]         = &AudioGrpcClientStub::onCreateStream;
    opLookup[DELETE_STREAM_RESP]         = &AudioGrpcClientStub::onDeleteStream;
    opLookup[STREAM_START_RESP]          = &AudioGrpcClientStub::onStartStream;
    opLookup[STREAM_STOP_RESP]           = &AudioGrpcClientStub::onStopStream;
    opLookup[STREAM_SET_DEVICE_RESP]     = &AudioGrpcClientStub::onSetDevice;
    opLookup[STREAM_GET_DEVICE_RESP]     = &AudioGrpcClientStub::onGetDevice;
    opLookup[STREAM_SET_VOLUME_RESP]     = &AudioGrpcClientStub::onSetVolume;
    opLookup[STREAM_GET_VOLUME_RESP]     = &AudioGrpcClientStub::onGetVolume;
    opLookup[STREAM_SET_MUTE_STATE_RESP] = &AudioGrpcClientStub::onSetMuteState;
    opLookup[STREAM_GET_MUTE_STATE_RESP] = &AudioGrpcClientStub::onGetMuteState;
    opLookup[STREAM_DTMF_START_RESP]     = &AudioGrpcClientStub::onPlayDtmfTone;
    opLookup[STREAM_DTMF_STOP_RESP]      = &AudioGrpcClientStub::onStopDtmfTone;
    opLookup[GET_CAL_INIT_STATUS_RESP]   = &AudioGrpcClientStub::onGetCalibrationInitStatus;
    opLookup[STREAM_WRITE_RESP]          = &AudioGrpcClientStub::onWrite;
    opLookup[STREAM_READ_RESP]           = &AudioGrpcClientStub::onRead;
    opLookup[STREAM_TONE_START_RESP]     = &AudioGrpcClientStub::onPlayTone;
    opLookup[STREAM_TONE_STOP_RESP]      = &AudioGrpcClientStub::onStopTone;
    opLookup[CREATE_TRANSCODER_RESP]     = &AudioGrpcClientStub::onCreateTranscoder;
    opLookup[DELETE_TRANSCODER_RESP]     = &AudioGrpcClientStub::onDeleteTranscoder;
    opLookup[STREAM_FLUSH_RESP]          = &AudioGrpcClientStub::onFlush;
    opLookup[STREAM_DRAIN_RESP]          = &AudioGrpcClientStub::onDrain;
}

AudioGrpcClientStub::~AudioGrpcClientStub() {
    std::lock_guard<std::mutex> ssrLock(AudioGrpcClientStub::destructorGuard_);
    LOG(DEBUG, __FUNCTION__);

    ClientContext context{};
    grpc::Status reqStatus;
    google::protobuf::Empty response;
    audioStub::AudioClientDisconnect request;

    request.set_clientid(getpid());
    reqStatus = stub_->ClientDisconnected(&context, request, &response);
     if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " Disconnecting client request failed");
    } else {
        AudioGrpcClientStub::exitNow_ = true;
        for (std::thread &th : runningThreads_) {
            if(th.joinable()){
                th.join();
            }
        }
    }
}

bool AudioGrpcClientStub::isReady() {
   return (serviceReady_ == common::ServiceStatus::SERVICE_AVAILABLE);
}

bool AudioGrpcClientStub::waitForInitialization() {
    std::unique_lock<std::mutex> cvLock(grpcClientMutex_);
    audioStub::AudioClientConnect request{};
    commonStub::GetServiceStatusReply response{};
    ClientContext context{};
    telux::common::ServiceStatus cbStatus = common::ServiceStatus::SERVICE_UNAVAILABLE;
    int cbDelay;
    grpc::Status reqStatus;

    request.set_clientid(getpid());
    reqStatus = stub_->ClientConnected(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " InitService request failed");
            return false;
    }

    serviceReady_ = static_cast<telux::common::ServiceStatus>(response.service_status());
    cbDelay = static_cast<int>(response.delay());
    cbStatus = static_cast<common::ServiceStatus>(response.service_status());
    LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));

    if (cbDelay != SKIP_CALLBACK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
    }

   return (serviceReady_ == common::ServiceStatus::SERVICE_AVAILABLE);
}

std::future<bool> AudioGrpcClientStub::onReady() {
   auto f = std::async(std::launch::async, [&] { return waitForInitialization(); });
   return f;
}

void AudioGrpcClientStub::createServerStreaming() {
    ClientContext context{};
    ::audioStub::AudioClientConnect request;
    audioStub::AsyncResponseMessage resp;

    request.set_clientid(getpid());
    std::unique_ptr<ClientReader<::audioStub::AsyncResponseMessage>>
        asyncRespReader(stub_->SetupAsyncResponseStream(&context, request));
    /* Blocking call till data is available on the ClientReader object. */
    while(asyncRespReader->Read(&resp)){
        int msgId = resp.msgid();
        if (msgId == 0) {
            LOG(INFO, __FUNCTION__, " Setting up Server Side stream done");
            continue;
        }

        if(msgId == STREAM_DTMF_DETECTED_IND) {
            audioStub::DtmfTone dtmfDetectionEvent;
            resp.any().UnpackTo(&dtmfDetectionEvent);

            serverMsgProcessor_->submitTask([=] {
                onDtmfToneDetected(dtmfDetectionEvent);;
            });
            continue;
        } else if (msgId == AUDIO_STATUS_IND) {
            commonStub::GetServiceStatusReply serviceStatus;
            resp.any().UnpackTo(&serviceStatus);
            onSSRUpdate(serviceStatus);
            continue;
        } else if (msgId == STREAM_DRAIN_IND){
            audioStub::DrainEvent drainEvent;
            resp.any().UnpackTo(&drainEvent);
            serverMsgProcessor_->submitTask([=] {
                onDrainDone(drainEvent);
            });
            continue;
        } else if (msgId == STREAM_WRITE_IND){
            audioStub::WriteReadyEvent writeReadyEvent;
            resp.any().UnpackTo(&writeReadyEvent);
            serverMsgProcessor_->submitTask([=] {
                onWriteReady(writeReadyEvent);
            });
            continue;
        }

        {
            std::lock_guard<std::mutex> lk(update_);
            serverMsgProcessor_->submitTask([=]{(this->*opLookup[msgId])(resp.any(),
                resp.cmdid(), static_cast<ErrorCode>(resp.error()));});
        }
    }

    grpc::Status status = asyncRespReader->Finish();
    if (status.ok()) {
        LOG(INFO, __FUNCTION__, " Closing server Side stream");
    }
};

telux::common::Status AudioGrpcClientStub::setup() {

    /* When a client connects, setup a server streaming for async response and indications.
       This thread sets up server streaming rpc and listens for messages from grpc server.*/
    std::thread asyncRespThread(&AudioGrpcClientStub::createServerStreaming, this);
    runningThreads_.emplace_back(std::move(asyncRespThread));

    try {
        voiceListenerMgr_ = std::make_shared<
            telux::common::ListenerManager<telux::audio::IVoiceStreamEventsCb>>();

        playListenerMgr_ = std::make_shared<
            telux::common::ListenerManager<telux::audio::IPlayStreamEventsCb>>();

        serviceStatusListenerMgr_ = std::make_shared<
            telux::common::ListenerManager<telux::audio::IServiceStatusEventsCb>>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't setup ListenerManager");
        return telux::common::Status::FAILED;
    }


    /*
     * Responses and indications received from audio server are processed on a dedicated
     * background thread. For both responses and indications, same thread is used to
     * serialize their processing so that the client library has a consistent view of
     * current overall operational state. This also streamlines SSR and audio server
     * connection/disconnection/crash handling.
     */
    try {
        serverMsgProcessor_ = std::unique_ptr<
            telux::common::TaskDispatcher>(new telux::common::TaskDispatcher());
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create TaskDispatcher");
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 * Register listener for SSR and connection to server events. AudioManagerImpl
 * always remains registered with AudioGrpcClientStub for service status change events,
 * irrespective of whether an application registered or not for SSR events.
 * Therefore, there is no deRegisterForServiceStatusEvents() method in this class.
 */
telux::common::Status AudioGrpcClientStub::registerForServiceStatusEvents(
        std::weak_ptr<telux::audio::IServiceStatusEventsCb> listener) {

    if (!serviceStatusListenerMgr_) {
        LOG(ERROR, __FUNCTION__, " invalid listener mgr");
        return telux::common::Status::INVALIDSTATE;
    }
    return serviceStatusListenerMgr_->registerListener(listener);
}

void AudioGrpcClientStub::onSSRUpdate(commonStub::GetServiceStatusReply serviceStatus) {

    telux::common::ServiceStatus newStatus;
    std::vector<std::weak_ptr<telux::audio::IServiceStatusEventsCb>> listeners;

    newStatus = static_cast<telux::common::ServiceStatus>(
        serviceStatus.service_status());

    if (!serviceStatusListenerMgr_) {
        LOG(ERROR, __FUNCTION__, " invalid listener mgr");
        return;
    }

    serviceStatusListenerMgr_->getAvailableListeners(listeners);

    for (auto &listener : listeners) {
        if(auto sp = listener.lock()) {
            sp->onQ6SSRUpdate(newStatus);
        }
    }
}

/*
 * Register listener for dtmf detected event.
 */
telux::common::Status AudioGrpcClientStub::registerForVoiceStreamEvents(uint32_t streamId,
    std::weak_ptr<telux::audio::IVoiceStreamEventsCb> listener)  {

    if (!voiceListenerMgr_) {
        LOG(ERROR, __FUNCTION__, " invalid listener mgr");
        return telux::common::Status::INVALIDSTATE;
    }

    return voiceListenerMgr_->registerListener(listener);
}

void AudioGrpcClientStub::onDtmfToneDetected(::audioStub::DtmfTone dtmfTone) {

    DtmfTone dtmfToneEvent{};
    std::vector<std::weak_ptr<telux::audio::IVoiceStreamEventsCb>> listeners;

    dtmfToneEvent.direction = static_cast<StreamDirection>(dtmfTone.direction().type());
    dtmfToneEvent.lowFreq = static_cast<DtmfLowFreq>(dtmfTone.lowfreq().type());
    dtmfToneEvent.highFreq = static_cast<DtmfHighFreq>(dtmfTone.highfreq().type());

    voiceListenerMgr_->getAvailableListeners(listeners);

    for (auto &listener : listeners) {
        if(auto sp = listener.lock()) {
            sp->onDtmfToneDetected(dtmfToneEvent);
        }
    }
}

/*
 * Register listener for write ready and drain done events.
 */
telux::common::Status AudioGrpcClientStub::registerForPlayStreamEvents(
        std::weak_ptr<telux::audio::IPlayStreamEventsCb> listener) {

    if (!playListenerMgr_) {
        LOG(ERROR, __FUNCTION__, " invalid listener mgr");
        return telux::common::Status::INVALIDSTATE;
    }

    return playListenerMgr_->registerListener(listener);
}

telux::common::Status AudioGrpcClientStub::flush(uint32_t streamId,
        std::shared_ptr<telux::audio::IFlushCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    audioStub::FlushRequest req;
    commonStub::StatusMsg response;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_FLUSH_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->Flush(&context,request,&response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " grpc request failed");
        {
            std::lock_guard<std::mutex> lock(update_);
            callbackMap_.erase({streamId, cmdId});
        }
        return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onFlush(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::FlushResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IFlushCb>(sp);
        cb->onFlushResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }

}

telux::common::Status AudioGrpcClientStub::drain(uint32_t streamId,
        std::shared_ptr<telux::audio::IDrainCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    audioStub::DrainRequest req;
    commonStub::StatusMsg response;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_DRAIN_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->Drain(&context,request,&response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " grpc request failed");
        {
            std::lock_guard<std::mutex> lock(update_);
            callbackMap_.erase({streamId, cmdId});
        }
        return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onDrain(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::DrainResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IDrainCb>(sp);
        cb->onDrainResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}


void AudioGrpcClientStub::onDrainDone(audioStub::DrainEvent drainEvent) {

    std::vector<std::weak_ptr<telux::audio::IPlayStreamEventsCb>> listeners;

    playListenerMgr_->getAvailableListeners(listeners);

    for (auto &listener : listeners) {
        if(auto sp = listener.lock()) {
            sp->onDrainDone(drainEvent.streamid());
        }
    }
}

void AudioGrpcClientStub::onWriteReady(audioStub::WriteReadyEvent writeReadyEvent) {

    std::vector<std::weak_ptr<telux::audio::IPlayStreamEventsCb>> listeners;

    playListenerMgr_->getAvailableListeners(listeners);

    for (auto &listener : listeners) {
        if(auto sp = listener.lock()) {
            sp->onWriteReady(writeReadyEvent.streamid());
        }
    }
}

telux::common::Status AudioGrpcClientStub::getDevices(
        std::shared_ptr<telux::audio::IGetDevicesCb> resultListener, int cmdId) {
    ::audioStub::AudioRequest request;
    ::commonStub::StatusMsg response;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    //Cache the resultListener with cmdId to invoke when response is received.
    callbackMap_[std::make_pair(-1, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(GET_SUPPORTED_DEVICES_REQ);
    request.set_cmdid(cmdId);
    reqStatus = stub_->GetDevices(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({-1, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onGetDevices(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    std::vector<telux::audio::DeviceType> deviceTypes;
    std::vector<telux::audio::DeviceDirection> deviceDirections;
    audioStub::GetDevicesResponse response;
    any.UnpackTo(&response);

    if (ec == telux::common::ErrorCode::SUCCESS) {
        for (auto dev : response.devices()) {
            deviceTypes.emplace_back(
                static_cast<telux::audio::DeviceType>(dev.devicetype().type()));
            deviceDirections.emplace_back(
                static_cast<telux::audio::DeviceDirection>(dev.direction().type()));
        }
    }

    auto sp = callbackMap_[{-1, cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IGetDevicesCb>(sp);
        cb->onGetDevicesResult(ec, deviceTypes, deviceDirections, cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::getStreamTypes(
        std::shared_ptr<telux::audio::IGetStreamsCb> resultListener, int cmdId) {

    ::audioStub::AudioRequest request;
    ::commonStub::StatusMsg response;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(-1, cmdId)] = resultListener;
    request.set_clientid(getpid());
    request.set_msgid(GET_SUPPORTED_STREAMS_REQ);
    request.set_cmdid(cmdId);

    reqStatus = stub_->GetStreamTypes(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({-1, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onGetStreamTypes(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    std::vector<telux::audio::StreamType> streamTypes;
    audioStub::GetStreamTypesResponse response;
    any.UnpackTo(&response);

    for(auto streamVal : response.streamtypes()){
        streamTypes.push_back(static_cast<telux::audio::StreamType>(streamVal.type()));
    }

    auto sp = callbackMap_[{-1, cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IGetStreamsCb>(sp);
        cb->onGetStreamsResult(ec, streamTypes, cmdId);
    }

    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::getCalibrationInitStatus(
    std::shared_ptr<telux::audio::IGetCalInitStatusCb> resultListener, int cmdId) {

    ::audioStub::AudioRequest request;
    ::commonStub::StatusMsg response;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(-1, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(GET_CAL_INIT_STATUS_REQ);
    request.set_cmdid(cmdId);
    reqStatus = stub_->GetCalibrationInitStatus(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({-1, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onGetCalibrationInitStatus(google::protobuf::Any any, int cmdId,
        ErrorCode ec) {

    audioStub::GetCalibrationInitStatusResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{-1, cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IGetCalInitStatusCb>(sp);
        cb->onGetCalInitStatusResult(ec,
           static_cast<telux::audio::CalibrationInitStatus>(response.calstatus().type()), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::createStream(telux::audio::StreamConfig streamConfig,
        std::shared_ptr<telux::audio::ICreateStreamCb> resultListener, int cmdId) {

    audioStub::AudioRequest request{};
    audioStub::CreateStreamRequest req{};
    commonStub::StatusMsg response{};
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    /*
     * When server sends async response, this result listener is invoked with response.
     * Protobuf doesn't support pointer type. Hence, these are cached on the library side with the
     * cmd Id and used when the response arrives.
     */
    callbackMap_[std::make_pair(-1, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(CREATE_STREAM_REQ);
    request.set_cmdid(cmdId);

    req.mutable_streamconfig()->mutable_streamtype()->set_type(static_cast<
        audioStub::StreamType_Type>(streamConfig.type));
    req.mutable_streamconfig()->set_slotid(streamConfig.slotId);
    req.mutable_streamconfig()->set_samplerate(streamConfig.sampleRate);
    req.mutable_streamconfig()->mutable_channeltype()->set_type(
        static_cast<::audioStub::ChannelType_Type>(streamConfig.channelTypeMask));
    req.mutable_streamconfig()->mutable_audioformat()->set_type(
            static_cast<::audioStub::AudioFormat_Type>(streamConfig.format));
    req.mutable_streamconfig()->mutable_ecnrmode()->set_type(
            static_cast<::audioStub::EcnrMode_Type>(streamConfig.ecnrMode));
    req.mutable_streamconfig()->set_enablehpcm(streamConfig.enableHpcm);

    for(auto dev : streamConfig.deviceTypes) {
        audioStub::DeviceType* deviceType = req.mutable_streamconfig()->add_devicetypes();
        deviceType->set_type(static_cast<::audioStub::DeviceType_Type>(dev));
    }


    for(auto voicePath : streamConfig.voicePaths) {
        audioStub::Direction* voicePathReq = req.mutable_streamconfig()->add_voicepaths();
        voicePathReq->set_type(static_cast<::audioStub::Direction_Type>(voicePath));
    }

    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->CreateStream(&context,request,&response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " grpc request failed");
        {
            std::lock_guard<std::mutex> lock(update_);
            callbackMap_.erase({-1, cmdId});
        }
        return telux::common::Status::FAILED;
    }

    /* API request status read from IAudioManager.json for audio request. */
    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onCreateStream(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    telux::audio::CreatedStreamInfo createdStreamInfo;
    audioStub::CreateStreamResponse response;
    any.UnpackTo(&response);

    createdStreamInfo.streamType = static_cast<telux::audio::StreamType>(
            response.createdstreaminfo().streamtype().type());
    createdStreamInfo.streamId = response.createdstreaminfo().streamid();
    createdStreamInfo.writeMinSize = response.createdstreaminfo().writeminsize();
    createdStreamInfo.writeMaxSize = response.createdstreaminfo().writemaxsize();
    createdStreamInfo.readMinSize = response.createdstreaminfo().readminsize();
    createdStreamInfo.readMaxSize = response.createdstreaminfo().readmaxsize();

    auto sp = callbackMap_[{-1, cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::ICreateStreamCb>(sp);
        cb->onCreateStreamResult(ec, createdStreamInfo, cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::deleteStream(uint32_t streamId,
        std::shared_ptr<telux::audio::IDeleteStreamCb> resultListener, int cmdId) {

    ::audioStub::AudioRequest request;
    ::audioStub::DeleteStreamRequest req;
    ::commonStub::StatusMsg response;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(-1, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(DELETE_STREAM_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->DeleteStream(&context,request,&response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " grpc request failed");
        {
            std::lock_guard<std::mutex> lock(update_);
            callbackMap_.erase({-1, cmdId});
        }
        return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onDeleteStream(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::DeleteStreamResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{-1, cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IDeleteStreamCb>(sp);
        cb->onDeleteStreamResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::createTranscoder(
        telux::audio::FormatInfo inInfo, telux::audio::FormatInfo outInfo,
        std::shared_ptr<telux::audio::ITranscodeCreateCb> resultListener, int cmdId) {

    audioStub::AudioRequest request{};
    audioStub::FormatInfo req{};
    commonStub::StatusMsg response{};
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    telux::audio::AmrwbpParams *inputParams;
    telux::audio::AmrwbpParams *outputParams;

    callbackMap_[std::make_pair(-1, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(CREATE_TRANSCODER_REQ);
    request.set_cmdid(cmdId);

    req.set_insamplerate(inInfo.sampleRate);
    req.mutable_inchanneltype()->set_type(
        static_cast<::audioStub::ChannelType_Type>(inInfo.mask));
    req.mutable_inaudioformat()->set_type(
            static_cast<::audioStub::AudioFormat_Type>(inInfo.format));
    inputParams = static_cast<telux::audio::AmrwbpParams *>(inInfo.params);
    req.mutable_inparams()->set_bitwidth(inputParams->bitWidth);
    req.mutable_inparams()->mutable_frameformat()->set_type(
        static_cast<::audioStub::AmrwbpFrameFormat_Type>(inputParams->frameFormat));

    req.set_outsamplerate(outInfo.sampleRate);
    req.mutable_outchanneltype()->set_type(
        static_cast<::audioStub::ChannelType_Type>(outInfo.mask));
    req.mutable_outaudioformat()->set_type(
            static_cast<::audioStub::AudioFormat_Type>(outInfo.format));

    if (outInfo.params) {
        outputParams = static_cast<telux::audio::AmrwbpParams *>(outInfo.params);
        req.mutable_outparams()->set_bitwidth(outputParams->bitWidth);
        req.mutable_outparams()->mutable_frameformat()->set_type(
            static_cast<::audioStub::AmrwbpFrameFormat_Type>(outputParams->frameFormat));
    } else {
        req.mutable_outparams()->set_bitwidth(0);
        req.mutable_outparams()->mutable_frameformat()->set_type(
            static_cast<::audioStub::AmrwbpFrameFormat_Type>(-1));
    }

    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->CreateTranscoder(&context,request,&response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " grpc request failed");
        {
            std::lock_guard<std::mutex> lock(update_);
            callbackMap_.erase({-1, cmdId});
        }
        return telux::common::Status::FAILED;
    }

    /* API request status read from IAudioManager.json for audio request. */
    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onCreateTranscoder(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::CreatedTranscoderInfo response;
    any.UnpackTo(&response);
    telux::audio::CreatedTranscoderInfo transcoderInfo;

    if (ec == telux::common::ErrorCode::SUCCESS) {
        transcoderInfo.inStreamId = response.instreamid();
        transcoderInfo.outStreamId = response.outstreamid();
        transcoderInfo.readMinSize = response.readminsize();
        transcoderInfo.readMaxSize = response.readmaxsize();
        transcoderInfo.writeMinSize = response.writeminsize();
        transcoderInfo.writeMaxSize = response.writemaxsize();
    }

    inTranscodeStreamId_ = transcoderInfo.inStreamId;
    outTranscodeStreamId_ = transcoderInfo.outStreamId;

    auto sp = callbackMap_[{-1, cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::ITranscodeCreateCb>(sp);
        cb->onCreateTranscoderResult(ec, transcoderInfo, cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({-1, cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::deleteTranscoder(
        uint32_t inStreamId, uint32_t outStreamId,
        std::shared_ptr<telux::audio::ITranscodeDeleteCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    audioStub::DeleteTranscoder req;
    commonStub::StatusMsg response;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(inStreamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(DELETE_TRANSCODER_REQ);
    request.set_cmdid(cmdId);
    req.set_instreamid(inStreamId);
    req.set_outstreamid(outStreamId);
    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->DeleteTranscoder(&context,request,&response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " grpc request failed");
        {
            std::lock_guard<std::mutex> lock(update_);
            callbackMap_.erase({inStreamId, cmdId});
        }
        return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({inStreamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onDeleteTranscoder(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::DeleteTranscoder response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.instreamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::ITranscodeDeleteCb>(sp);
        cb->onDeleteTranscoderResult(ec, response.instreamid(), response.outstreamid(), cmdId);
    }

    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.instreamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::startStream(uint32_t streamId,
        std::shared_ptr<telux::audio::IStartStreamCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::StartStreamRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_START_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    request.mutable_any()->PackFrom(req);
    reqStatus = stub_->StartAudio(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onStartStream(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::StartStreamResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IStartStreamCb>(sp);
        cb->onStreamStartResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::stopStream(uint32_t streamId,
        std::shared_ptr<telux::audio::IStopStreamCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::StopStreamRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_STOP_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    request.mutable_any()->PackFrom(req);
    reqStatus = stub_->StopAudio(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onStopStream(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::StopStreamResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IStopStreamCb>(sp);
        cb->onStreamStopResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::playDtmfTone(telux::audio::DtmfTone dtmfTone,
        uint16_t duration, uint16_t gain, uint32_t streamId,
        std::shared_ptr<telux::audio::IDTMFCb> resultListener, int cmdId) {

    ::audioStub::AudioRequest request;
    audioStub::StartDtmfToneRequest tone;
    ::commonStub::StatusMsg response;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_DTMF_START_REQ);
    request.set_cmdid(cmdId);

    tone.mutable_dtmftone()->mutable_lowfreq()->set_type(
            static_cast<::audioStub::DtmfLowFreq_Type>(dtmfTone.lowFreq));
    tone.mutable_dtmftone()->mutable_highfreq()->set_type(
            static_cast<::audioStub::DtmfHighFreq_Type>(dtmfTone.highFreq));
    tone.mutable_dtmftone()->mutable_direction()->set_type(
            static_cast<::audioStub::StreamDirection_Type>(StreamDirection::RX));

    request.mutable_any()->PackFrom(tone);

    reqStatus = stub_->PlayDtmfTone(&context,request,&response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " grpc request failed");
        {
            std::lock_guard<std::mutex> lock(update_);
            callbackMap_.erase({streamId, cmdId});
        }
        return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onPlayDtmfTone(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::StartDtmfToneResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IDTMFCb>(sp);
        cb->onPlayDtmfResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::stopDtmfTone(telux::audio::StreamDirection direction,
        uint32_t streamId, std::shared_ptr<telux::audio::IDTMFCb> resultListener,
        int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::StopDtmfToneRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_DTMF_STOP_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    req.mutable_dir()->set_type(static_cast<::audioStub::StreamDirection_Type>(direction));
    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->StopDtmfTone(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onStopDtmfTone(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::StopDtmfToneResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IDTMFCb>(sp);
        cb->onStopDtmfResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::playTone(uint32_t streamId, std::vector<uint16_t>
        frequency, uint16_t duration, uint16_t gain,
        std::shared_ptr<telux::audio::IToneCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::PlayToneRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_TONE_START_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    req.set_duration(duration);
    req.set_gain(gain);
    for(auto f : frequency) {
        req.add_freq(f);
    }
    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->PlayTone(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onPlayTone(google::protobuf::Any any, int cmdId, ErrorCode ec) {
    audioStub::PlayToneResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IToneCb>(sp);
        cb->onToneStartResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::stopTone(uint32_t streamId,
        std::shared_ptr<telux::audio::IToneCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::StopToneRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_TONE_STOP_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    request.mutable_any()->PackFrom(req);
    reqStatus = stub_->StopAudio(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onStopTone(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::StopToneResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IToneCb>(sp);
        cb->onToneStopResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::setDevice(uint32_t streamId,
        std::vector<telux::audio::DeviceType> devices,
        std::shared_ptr<telux::audio::ISetGetDeviceCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::SetDeviceRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_SET_DEVICE_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);

    for(auto dev : devices) {
        audioStub::DeviceType* deviceType = req.add_devicetypes();
        deviceType->set_type(static_cast<::audioStub::DeviceType_Type>(dev));
    }

    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->SetStreamDevices(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onSetDevice(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::SetDeviceResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::ISetGetDeviceCb>(sp);
        cb->onSetDeviceResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::getDevice(uint32_t streamId,
        std::shared_ptr<telux::audio::ISetGetDeviceCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::GetDeviceRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_GET_DEVICE_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    request.mutable_any()->PackFrom(req);
    reqStatus = stub_->GetStreamDevices(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onGetDevice(google::protobuf::Any any, int cmdId, ErrorCode ec) {
    audioStub::GetDeviceResponse response;
    any.UnpackTo(&response);

    std::vector<telux::audio::DeviceType> devices;

    if (ec == telux::common::ErrorCode::SUCCESS) {
        for (auto dev : response.devicetypes()) {
            devices.emplace_back(static_cast<telux::audio::DeviceType>(dev.type()));
        }
    }

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::ISetGetDeviceCb>(sp);
        cb->onGetDeviceResult(ec, response.streamid(), devices, cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::setVolume(uint32_t streamId,
        telux::audio::StreamVolume volume,
        std::shared_ptr<telux::audio::ISetGetVolumeCb> resultListener,
        int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::SetVolumeRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_SET_VOLUME_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    for(auto vol : volume.volume){
        ::audioStub::ChannelVolume* chVol = req.mutable_volume()->add_volume();
        chVol->mutable_channeltype()->set_type(static_cast<::audioStub::ChannelType_Type>(
                vol.channelType));
        chVol->set_vol(vol.vol);
    }

    req.mutable_volume()->mutable_direction()->set_type(
            static_cast<audioStub::StreamDirection_Type>(volume.dir));
    request.mutable_any()->PackFrom(req);
    reqStatus = stub_->GetStreamDevices(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onSetVolume(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::SetVolumeResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::ISetGetVolumeCb>(sp);
        cb->onSetVolumeResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::getVolume(uint32_t streamId,
        telux::audio::StreamDirection direction,
        std::shared_ptr<telux::audio::ISetGetVolumeCb> resultListener,
        int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::GetVolumeRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_GET_VOLUME_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    req.mutable_dir()->set_type(static_cast<audioStub::StreamDirection_Type>(direction));
    request.mutable_any()->PackFrom(req);
    reqStatus = stub_->GetStreamDevices(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onGetVolume(google::protobuf::Any any, int cmdId, ErrorCode ec) {
    audioStub::GetVolumeResponse response;
    any.UnpackTo(&response);
    telux::audio::StreamVolume streamVolume{};
    telux::audio::ChannelVolume chVol;

    if (ec == telux::common::ErrorCode::SUCCESS) {
        streamVolume.dir = static_cast<telux::audio::StreamDirection>(
                response.volumeinfo().direction().type());

        for (auto vol : response.volumeinfo().volume()){
            chVol.channelType = static_cast<telux::audio::ChannelType>(vol.channeltype().type());
            chVol.vol = vol.vol();
            streamVolume.volume.push_back(chVol);
        }
    }

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::ISetGetVolumeCb>(sp);
        cb->onGetVolumeResult(ec, response.streamid(), streamVolume, cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::setMute(uint32_t streamId,
        telux::audio::StreamMute mute,
        std::shared_ptr<telux::audio::ISetGetMuteCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::SetMuteRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;
    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_SET_MUTE_STATE_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    req.mutable_mutestatus()->set_enable(mute.enable);
    req.mutable_mutestatus()->mutable_direction()->set_type(
            static_cast<audioStub::StreamDirection_Type>(mute.dir));

    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->SetStreamMute(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onSetMuteState(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::SetMuteResponse response;
    any.UnpackTo(&response);

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::ISetGetMuteCb>(sp);
        cb->onSetMuteResult(ec, response.streamid(), cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::getMute(uint32_t streamId,
        telux::audio::StreamDirection direction,
        std::shared_ptr<telux::audio::ISetGetMuteCb> resultListener, int cmdId) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::GetMuteRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;

    callbackMap_[std::make_pair(streamId, cmdId)] = resultListener;

    request.set_clientid(getpid());
    request.set_msgid(STREAM_GET_MUTE_STATE_REQ);
    request.set_cmdid(cmdId);
    req.set_streamid(streamId);
    req.mutable_dir()->set_type(static_cast<audioStub::StreamDirection_Type>(direction));
    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->GetStreamMuteStatus(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, cmdId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, cmdId});
    }

    return status;
}

void AudioGrpcClientStub::onGetMuteState(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    audioStub::GetMuteResponse response;
    any.UnpackTo(&response);
    telux::audio::StreamMute streamMute{};


    if (ec == telux::common::ErrorCode::SUCCESS) {
        streamMute.enable = response.mutestatus().enable();
        streamMute.dir = static_cast<telux::audio::StreamDirection>(
            response.mutestatus().direction().type());
    }

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::ISetGetMuteCb>(sp);
        cb->onGetMuteResult(ec, response.streamid(), streamMute, cmdId);
    }
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({response.streamid(), cmdId});
    }
}

telux::common::Status AudioGrpcClientStub::write(uint32_t streamId, uint8_t *transportBuffer,
        uint32_t isLastBuffer, std::shared_ptr<telux::audio::IWriteCb> resultListener,
        telux::audio::AudioUserData *userData, uint32_t dataLength) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::writeRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_[std::make_pair(streamId, userData->cmdCallbackId)] = resultListener;
        userDataMap_[std::make_pair(streamId, userData->cmdCallbackId)] = userData;
    }
    request.set_clientid(getpid());
    request.set_msgid(STREAM_WRITE_REQ);
    request.set_cmdid(userData->cmdCallbackId);
    req.set_streamid(streamId);
    req.set_islastbuffer(isLastBuffer);
    req.set_datalength(dataLength);
    //Check another method to populate buffer
    req.set_buffer(transportBuffer, dataLength);
    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->Write(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, userData->cmdCallbackId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());
    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, userData->cmdCallbackId});
    }

    return status;
}

void AudioGrpcClientStub::onWrite(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    AudioUserData *audioUserData;
    audioStub::writeResponse response;
    any.UnpackTo(&response);

    {
        std::lock_guard<std::mutex> lock(update_);
        audioUserData = userDataMap_[{response.streamid(), cmdId}];
    }

    auto sp = callbackMap_[std::make_pair(response.streamid(), cmdId)].lock();

    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IWriteCb>(sp);
        cb->onWriteResult(ec, response.streamid(),
                response.datalength(), audioUserData);
    }

    {
        std::lock_guard<std::mutex> lock(update_);

        userDataMap_.erase({response.streamid(), cmdId});
        callbackMap_.erase({response.streamid(), cmdId});
    }
}


telux::common::Status AudioGrpcClientStub::read(uint32_t streamId, uint32_t numBytesToRead,
        uint8_t *transportBuffer, std::shared_ptr<telux::audio::IReadCb> resultListener,
        telux::audio::AudioUserData *audioUserData) {

    audioStub::AudioRequest request;
    commonStub::StatusMsg response;
    audioStub::readRequest req;
    ClientContext context{};
    telux::common::Status status;
    grpc::Status reqStatus;
    {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_[{streamId, audioUserData->cmdCallbackId}] = resultListener;
        userDataMap_[{streamId, audioUserData->cmdCallbackId}] = audioUserData;
    }
    request.set_clientid(getpid());
    request.set_msgid(STREAM_READ_REQ);
    request.set_cmdid(audioUserData->cmdCallbackId);
    req.set_streamid(streamId);
    req.set_numbytestoread(numBytesToRead);
    request.mutable_any()->PackFrom(req);

    reqStatus = stub_->Read(&context, request, &response);
    if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " grpc request failed");
            {
                std::lock_guard<std::mutex> lock(update_);
                callbackMap_.erase({streamId, audioUserData->cmdCallbackId});
            }
            return telux::common::Status::FAILED;
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(update_);
        callbackMap_.erase({streamId, audioUserData->cmdCallbackId});
    }

    return status;
}

void AudioGrpcClientStub::onRead(google::protobuf::Any any, int cmdId, ErrorCode ec) {

    AudioUserData *audioUserData;
    audioStub::readResponse response;
    any.UnpackTo(&response);

    {
        std::lock_guard<std::mutex> lock(update_);
        audioUserData = userDataMap_[{response.streamid(), cmdId}];
    }

    if(response.streamid()!=outTranscodeStreamId_) {
        uint8_t* data = audioUserData->streamBuffer->getTransportBuffer();
        std::string buffer = response.buffer();
        for (uint32_t i = 0; i < buffer.size(); i++) {
            data[i] = buffer[i];
        }
    }

    auto sp = callbackMap_[{response.streamid(), cmdId}].lock();
    if (sp) {
        auto cb = std::static_pointer_cast<telux::audio::IReadCb>(sp);
        cb->onReadResult(ec, response.streamid(), response.datalength(), audioUserData);

    }

    {
        std::lock_guard<std::mutex> lock(update_);
        userDataMap_.erase({response.streamid(), cmdId});
        callbackMap_.erase({response.streamid(), cmdId});
    }

    /* No need to free data here, it is freed from StreamBufferImpl destructor */
}

} // End of namespace audio
} // End of namespace telux