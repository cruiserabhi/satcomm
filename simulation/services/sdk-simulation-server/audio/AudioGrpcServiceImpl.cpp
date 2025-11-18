/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * Receiving request:
 * 1. AudioGrpcServiceImpl::* receives a protobuf message from grpc client (local/remote).
 * 2. This class's onClientProcessReq() parses this message and converts from
 *    protobuf specific format to audio specific format (classes, structs etc.).
 * 3. This parsed data is then sent to audio service.
 * 4. Audio service finds 'Stream' for which this request has come and post it
 *    on its worker thread.
 * 5. This thread then performs audio operation in background.
 *
 * Sending responses:
 * 6. Audio service or 'Stream' has a result of the audio operation performed
 *    by the worker thread.
 * 7. They pass it to this class's corresponding *Response() method.
 * 8. This method converts result from audio specific format to protobuf specific
 *    format.
 * 9. This result is then sent to the grpc client using helper method
 *    from AudioGrpcServiceImpl::*.
 */

#include "AudioGrpcServiceImpl.hpp"

namespace telux {
namespace audio {

/*
 * AudioRequestHandler is a function pointer, pointing to a member function of
 * telux::audio::AudioGrpcServiceImpl class.
 */
using AudioRequestHandler = void (telux::audio::AudioGrpcServiceImpl::*)(
            google::protobuf::Any any,
            std::shared_ptr<AudioRequest> audioReq,
            std::shared_ptr<IAudioMsgListener> audioMsgListener);

/*
 * Request handler look up corresponding to the operation requested by audio client.
 * There are total 26 request types from 0x0001 to 0x001A. Extra 1 (in 26 + 1) is
 * because array indexing starts from 0 not from 1. GRPC request message ID numbering
 * starts from 1.
 */
AudioRequestHandler opLookup[26 + 1];

AudioGrpcServiceImpl::AudioGrpcServiceImpl() {

    telux::common::Status status;

    try {
        audioService_ = std::make_shared<AudioServiceImpl>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create AudioServiceImpl");
    }

    status = audioService_->initService();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't init Audio service");
    }

    try {
        jsonHelper_ = std::make_shared<AudioJsonHelper>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create AudioJsonHelper");
    }

    audioMsgListener_ = audioService_;

    /* This maps GRPC message ID to the corresponding audio operation. If this lookup is modified,
     * size of opLookup array should also be updated accordingly. */
    opLookup[GET_SUPPORTED_DEVICES_REQ] = &AudioGrpcServiceImpl::getSupportedDevices;
    opLookup[GET_SUPPORTED_STREAMS_REQ] = &AudioGrpcServiceImpl::getSupportedStreamTypes;
    opLookup[CREATE_STREAM_REQ]         = &AudioGrpcServiceImpl::createStream;
    opLookup[DELETE_STREAM_REQ]         = &AudioGrpcServiceImpl::deleteStream;
    opLookup[STREAM_START_REQ]          = &AudioGrpcServiceImpl::start;
    opLookup[STREAM_STOP_REQ]           = &AudioGrpcServiceImpl::stop;
    opLookup[STREAM_SET_DEVICE_REQ]     = &AudioGrpcServiceImpl::setDevice;
    opLookup[STREAM_GET_DEVICE_REQ]     = &AudioGrpcServiceImpl::getDevice;
    opLookup[STREAM_SET_VOLUME_REQ]     = &AudioGrpcServiceImpl::setVolume;
    opLookup[STREAM_GET_VOLUME_REQ]     = &AudioGrpcServiceImpl::getVolume;
    opLookup[STREAM_SET_MUTE_STATE_REQ] = &AudioGrpcServiceImpl::setMuteState;
    opLookup[STREAM_GET_MUTE_STATE_REQ] = &AudioGrpcServiceImpl::getMuteState;
    opLookup[STREAM_DTMF_START_REQ]     = &AudioGrpcServiceImpl::startDtmf;
    opLookup[STREAM_DTMF_STOP_REQ]      = &AudioGrpcServiceImpl::stopDtmf;
    opLookup[GET_CAL_INIT_STATUS_REQ]   = &AudioGrpcServiceImpl::getCalibrationStatus;
    opLookup[STREAM_WRITE_REQ]          = &AudioGrpcServiceImpl::write;
    opLookup[STREAM_READ_REQ]           = &AudioGrpcServiceImpl::read;
    opLookup[STREAM_TONE_START_REQ]     = &AudioGrpcServiceImpl::startTone;
    opLookup[STREAM_TONE_STOP_REQ]      = &AudioGrpcServiceImpl::stopTone;
    opLookup[CREATE_TRANSCODER_REQ]      = &AudioGrpcServiceImpl::createTranscoder;
    opLookup[DELETE_TRANSCODER_REQ]      = &AudioGrpcServiceImpl::deleteTranscoder;
    opLookup[STREAM_FLUSH_REQ]           = &AudioGrpcServiceImpl::flush;
    opLookup[STREAM_DRAIN_REQ]           = &AudioGrpcServiceImpl::drain;
}

AudioGrpcServiceImpl::~AudioGrpcServiceImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status AudioGrpcServiceImpl::ClientConnected(::grpc::ServerContext* context, const
    ::audioStub::AudioClientConnect* request, ::commonStub::GetServiceStatusReply* response) {

    std::shared_ptr<AudioClient> audioClient;

    try {
        audioClient = std::make_shared<AudioClient>(request->clientid(), shared_from_this());
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create AudioClient");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "can't create AudioClient");
    }

    auto sp = audioMsgListener_.lock();
    if (!sp) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "can't get IAudioMsgListener");
    }

    sp->onClientConnected(audioClient, shared_from_this());

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    serviceStatus_ = jsonHelper_->initServiceStatus();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(serviceStatus_));
    int subSysDelay = jsonHelper_->getSubsystemReadyDelay();

    switch(serviceStatus_) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_FAILED:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_FAILED);
            break;
        default:
            LOG(ERROR, __FUNCTION__, ":: Invalid service status");
    }
    response->set_delay(subSysDelay);
    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::GetStreamTypes(::grpc::ServerContext* context, const ::audioStub::AudioRequest*
        request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getStreamTypes");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        /* For all use cases bail out early if user configured error in json.
         * If we start actual use case through alsa but return error to the
         * application, it may happen that sound is coming out from speaker
         * while application believes api failed.
         */
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::GetCalibrationInitStatus(::grpc::ServerContext*
    context, const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getCalibrationInitStatus");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS && apiResp.error !=
        telux::common::ErrorCode::NOT_SUPPORTED) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::GetDevices(::grpc::ServerContext*
    context, const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response){

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getDevices");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::CreateStream(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "createStream");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::StartAudio(::grpc::ServerContext* context, const
    ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "startAudio");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status  AudioGrpcServiceImpl::StopAudio(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "stopAudio");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}


grpc::Status AudioGrpcServiceImpl::PlayDtmfTone(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "playDtmfTone");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::StopDtmfTone(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "stopDtmfTone");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::GetStreamDevices(grpc::ServerContext* context, const
    ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getDevice");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::SetStreamDevices(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "setDevice");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}


grpc::Status AudioGrpcServiceImpl::GetStreamMuteStatus(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getMute");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::SetStreamMute(::grpc::ServerContext* context, const
    ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "setMute");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::GetStreamVolume(::grpc::ServerContext* context, const
    ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getVolume");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::SetStreamVolume(::grpc::ServerContext* context, const
    ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "setVolume");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::DeleteStream(::grpc::ServerContext* context, const
    ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "deleteStream");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::Write(::grpc::ServerContext* context, const
    ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "write");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}


grpc::Status AudioGrpcServiceImpl::Read(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "read");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::PlayTone(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "startTone");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::StopTone(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    ApiResponse apiResp{};

    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "stopTone");

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    if (apiResp.error != telux::common::ErrorCode::SUCCESS) {
        LOG(INFO, __FUNCTION__, " Request dropped as per Json error configuration");
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::CreateTranscoder(::grpc::ServerContext* context,
        const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    status = jsonHelper_->getApiRequestStatus("createTranscoder");

    response->set_status(static_cast<commonStub::Status>(status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::DeleteTranscoder(::grpc::ServerContext* context,
        const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    status = jsonHelper_->getApiRequestStatus("deleteTranscoder");

    response->set_status(static_cast<commonStub::Status>(status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::Flush(::grpc::ServerContext* context,
    const ::audioStub::AudioRequest* request, ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    status = jsonHelper_->getApiRequestStatus("flush");

    response->set_status(static_cast<commonStub::Status>(status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::Drain(::grpc::ServerContext* context, const ::audioStub::AudioRequest* request,
    ::commonStub::StatusMsg* response) {

    telux::common::Status status;
    status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, ":: Json not found");
    }

    status = jsonHelper_->getApiRequestStatus("drain");

    response->set_status(static_cast<commonStub::Status>(status));
    if(status != telux::common::Status::SUCCESS){
        return grpc::Status::OK;
    }

    telux::common::ErrorCode error = onClientProcessReq(request);
    if(error != telux::common::ErrorCode::SUCCESS){
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
    }

    return grpc::Status::OK;

}

grpc::Status AudioGrpcServiceImpl::SetupAsyncResponseStream(::grpc::ServerContext* context,
        const ::audioStub::AudioClientConnect* request,
            ::grpc::ServerWriter< ::audioStub::AsyncResponseMessage>* writer) {
    std::unique_lock<std::mutex> lk(streamWriterMtx_);
    int clientId = request->clientid();
    LOG(ERROR, __FUNCTION__, " Setting up, server Side stream for client: ", clientId);
    audioStub::AsyncResponseMessage resp;
    serverStreamMap_[clientId] = writer;
    resp.set_msgid(0);
    writer->Write(resp);
    cv_.wait(lk, [&]{return (serverStreamMap_.find(clientId) == serverStreamMap_.end());});
    return grpc::Status::OK;
}

grpc::Status AudioGrpcServiceImpl::ClientDisconnected(::grpc::ServerContext* context, const
    ::audioStub::AudioClientDisconnect* request, ::google::protobuf::Empty* response) {

    int clientId = request->clientid();
    LOG(ERROR, __FUNCTION__, " Disconnecting, server Side stream for client: ", clientId);

    auto sp = audioMsgListener_.lock();
    if (sp) {
        auto audioClient = audioService_->getClientCache()->getAudioClientFromClientId(clientId);
        if (!audioClient) {
            LOG(ERROR, __FUNCTION__, " can't find AudioClient");
            return grpc::Status(grpc::StatusCode::CANCELLED, "can't find AudioClient");
        }

        if(sp->onClientDisconnected(audioClient) != telux::common::Status::SUCCESS) {
            return grpc::Status(grpc::StatusCode::CANCELLED, ":: Cannot process request");
        }
    }

    std::map<uint16_t, grpc::ServerWriter<audioStub::AsyncResponseMessage>*>::iterator it;
    {
        std::lock_guard<std::mutex> lk(streamWriterMtx_);
        if (serverStreamMap_.find(clientId) != serverStreamMap_.end()) {
            serverStreamMap_.erase(clientId);
        }
        cv_.notify_all();
    }

    return grpc::Status::OK;
}

telux::common::ErrorCode AudioGrpcServiceImpl::onClientProcessReq(const ::audioStub::AudioRequest*
        request) {

    uint32_t msgId;
    std::shared_ptr<AudioRequest> audioReq;
    std::shared_ptr<IAudioMsgListener> audioMsgListener;

    msgId = request->msgid();
    audioMsgListener = audioMsgListener_.lock();
    if (!audioMsgListener) {
        LOG(ERROR, __FUNCTION__, " request dropped, can't get IAudioMsgListener");
        return telux::common::ErrorCode::NO_MEMORY;
    }
    int clientId = request->clientid();
    LOG(ERROR, __FUNCTION__, " Client id for req:", clientId);
    try {
        audioReq = std::make_shared<AudioRequest>(request->cmdid(), msgId, clientId,
        shared_from_this());
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " request dropped, can't create AudioRequest");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    if (audioMsgListener->isSSRInProgress()) {
        LOG(ERROR, __FUNCTION__, " can't service request, ssr is in progress");
        /* silently drop request as application will be busy in cleanup or
            * is about to begin cleaning up soon */
        return telux::common::ErrorCode::CANCELLED;
    }


    (this->*opLookup[msgId])(request->any(), audioReq, audioMsgListener);

    return telux::common::ErrorCode::SUCCESS;
}


void AudioGrpcServiceImpl::broadcastServiceStatus(uint32_t newStatus) {

    commonStub::GetServiceStatusReply response{};
    audioStub::AsyncResponseMessage resp{};

    switch(newStatus) {
        case AUDIO_SERVICE_ONLINE:
            response.set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
            break;
        case AUDIO_SERVICE_OFFLINE:
            response.set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);
            break;
        default:
            LOG(ERROR, __FUNCTION__, ":: Invalid service status");
    }

    resp.set_msgid(AUDIO_STATUS_IND);
    resp.mutable_any()->PackFrom(response);

    for (auto it = serverStreamMap_.begin(); it != serverStreamMap_.end(); it++){
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            it->second->Write(resp);
        }
    }

    LOG(DEBUG, __FUNCTION__, " service new status: ", newStatus);
}

void AudioGrpcServiceImpl::getSupportedDevices(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioMsgListener->getSupportedDevices(audioReq);
}

void AudioGrpcServiceImpl::sendGetSupportedDevicesResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        std::vector<DeviceType> const &devices,
        std::vector<DeviceDirection> const &devicesDirection) {

    audioStub::GetDevicesResponse response{};
    audioStub::AsyncResponseMessage resp{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getDevices");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    for ( uint32_t index = 0; index < devices.size(); ++index ){
        audioStub::SubsystemDevice *subsystemdevices = response.add_devices();
        subsystemdevices->mutable_devicetype()->set_type(
            static_cast<audioStub::DeviceType_Type>(devices[index]));
        subsystemdevices->mutable_direction()->set_type(
            static_cast<audioStub::DeviceDirection_Type>(devicesDirection[index]));
    }
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));
    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::getSupportedStreamTypes(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioMsgListener->getSupportedStreamTypes(audioReq);
}

void AudioGrpcServiceImpl::sendGetSupportedStreamTypesResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        std::vector<StreamType> const &streamTypes) {

    audioStub::GetStreamTypesResponse response{};
    audioStub::AsyncResponseMessage resp{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getStreamTypes");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    for ( uint32_t index = 0; index < streamTypes.size(); ++index ){
        audioStub::StreamType *streamType = response.add_streamtypes();
        streamType->set_type(static_cast<audioStub::StreamType_Type>(streamTypes[index]));
    }
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::createStream(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    StreamConfiguration config{};
    audioStub::CreateStreamRequest request{};
    any.UnpackTo(&request);

    /* Store the stream parameters passed */
    config.streamConfig.type = static_cast<telux::audio::StreamType>(
        request.streamconfig().streamtype().type());
    config.streamConfig.slotId = static_cast<SlotId>(request.streamconfig().slotid());
    config.streamConfig.sampleRate = static_cast<uint32_t>(request.streamconfig().samplerate());
    config.streamConfig.channelTypeMask = static_cast<telux::audio::ChannelTypeMask>(
        request.streamconfig().channeltype().type());
    config.streamConfig.format = static_cast<telux::audio::AudioFormat>(
        request.streamconfig().audioformat().type());
    config.streamConfig.ecnrMode = static_cast<telux::audio::EcnrMode>(
        request.streamconfig().ecnrmode().type());
    config.streamConfig.enableHpcm = request.streamconfig().enablehpcm();
    for (auto dev : request.streamconfig().devicetypes()) {
        config.streamConfig.deviceTypes.push_back(static_cast<telux::audio::DeviceType>(
            dev.type()));
    }

    for (auto voicePath : request.streamconfig().voicepaths()) {
        config.streamConfig.voicePaths.push_back(static_cast<telux::audio::Direction>(
            voicePath.type()));
    }

    audioMsgListener->createStream(audioReq, config);
}

void AudioGrpcServiceImpl::sendCreateStreamResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId, StreamType streamType, uint32_t readMinSize,
        uint32_t writeMinSize) {

    audioStub::CreateStreamResponse response{};
    audioStub::AsyncResponseMessage resp{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "createStream");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.mutable_createdstreaminfo()->set_streamid(streamId);
    response.mutable_createdstreaminfo()->mutable_streamtype()->set_type(
        static_cast<::audioStub::StreamType_Type>(streamType));
    response.mutable_createdstreaminfo()->set_readminsize(readMinSize);
    response.mutable_createdstreaminfo()->set_readmaxsize(MAX_BUFFER_SIZE);
    response.mutable_createdstreaminfo()->set_writeminsize(writeMinSize);
    response.mutable_createdstreaminfo()->set_writemaxsize(MAX_BUFFER_SIZE);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, " Client Id ", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
    }
}

void AudioGrpcServiceImpl::deleteStream(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {
    audioStub::DeleteStreamRequest request{};
    any.UnpackTo(&request);

    audioMsgListener->deleteStream(audioReq, request.streamid());
}

void AudioGrpcServiceImpl::sendDeleteStreamResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::DeleteStreamResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "deleteStream");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::start(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioStub::StartStreamRequest request{};
    any.UnpackTo(&request);
    audioMsgListener->start(audioReq, request.streamid());
}

void AudioGrpcServiceImpl::sendStartResponse(
        std::shared_ptr<AudioRequest> audioReq,
        telux::common::ErrorCode ec, uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::StartStreamResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "startAudio");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::stop(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioStub::StopStreamRequest request{};
    any.UnpackTo(&request);
    audioMsgListener->stop(audioReq, request.streamid());
}

void AudioGrpcServiceImpl::sendStopResponse(
        std::shared_ptr<AudioRequest> audioReq,
        telux::common::ErrorCode ec, uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::StopStreamResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "stopAudio");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::setDevice(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    std::vector<DeviceType> devices{};
    audioStub::SetDeviceRequest request{};
    any.UnpackTo(&request);

    for (auto dev : request.devicetypes()) {
        devices.push_back(static_cast<telux::audio::DeviceType>(dev.type()));
    }

    audioMsgListener->setDevice(audioReq, request.streamid(), devices);
}

void AudioGrpcServiceImpl::sendSetDeviceResponse(
        std::shared_ptr<AudioRequest> audioReq,
        telux::common::ErrorCode ec, uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::SetDeviceResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "setDevice");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::getDevice(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioStub::GetDeviceRequest request{};
    any.UnpackTo(&request);

    audioMsgListener->getDevice(audioReq, request.streamid());
}

void AudioGrpcServiceImpl::sendGetDeviceResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId, std::vector<DeviceType> const &devices) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::GetDeviceResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getDevice");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
   for(auto device : devices){
        audioStub::DeviceType *dev = response.add_devicetypes();
        dev->set_type(static_cast<audioStub::DeviceType_Type>(
                    device));
    }
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::setVolume(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    StreamDirection direction{};
    ChannelVolume chlVol{};
    std::vector<ChannelVolume> channelsVolume{};
    audioStub::SetVolumeRequest request{};
    any.UnpackTo(&request);

    direction = static_cast<telux::audio::StreamDirection>(
            request.volume().direction().type());
    for(auto chVol : request.volume().volume()){
        chlVol.channelType = static_cast<telux::audio::ChannelType>(
                chVol.channeltype().type());
        chlVol.vol = chVol.vol();
        if ((chlVol.vol< 0.0) || (chlVol.vol > 1.0)) {
            LOG(ERROR, __FUNCTION__, " out-of-range volume value");
        }
        channelsVolume.emplace_back(chlVol);
    }

    audioMsgListener->setVolume(audioReq, request.streamid(), direction, channelsVolume);
}

void AudioGrpcServiceImpl::sendSetVolumeResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::SetVolumeResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "setVolume");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::getVolume(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    StreamDirection direction{};
    audioStub::GetVolumeRequest request{};
    any.UnpackTo(&request);

    direction = static_cast<telux::audio::StreamDirection>(request.dir().type());

    audioMsgListener->getVolume(audioReq, request.streamid(), direction);
}

void AudioGrpcServiceImpl::sendGetVolumeResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId, StreamDirection direction,
        std::vector<ChannelVolume> channelsVolume) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::GetVolumeResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getVolume");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    for(auto chVol : channelsVolume){
        audioStub::ChannelVolume *channelVol = response.mutable_volumeinfo()->add_volume();
        channelVol->mutable_channeltype()->set_type(static_cast<::audioStub::ChannelType_Type>(
            chVol.channelType));
        channelVol->set_vol(chVol.vol);
    }
    response.mutable_volumeinfo()->mutable_direction()->set_type(
        static_cast<audioStub::StreamDirection_Type>(direction));
    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
        LOG(ERROR, __FUNCTION__, "Client Id not found" );
    }
}

void AudioGrpcServiceImpl::setMuteState(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    StreamMute muteInfo{};
    audioStub::SetMuteRequest request{};
    any.UnpackTo(&request);

    muteInfo.enable = static_cast<bool>(request.mutestatus().enable());
    muteInfo.dir = static_cast<StreamDirection>(request.mutestatus().direction().type());

    audioMsgListener->setMuteState(audioReq, request.streamid(), muteInfo);
}

void AudioGrpcServiceImpl::sendSetMuteStateResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::SetMuteResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "setMute");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::getMuteState(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    StreamDirection direction{};
    audioStub::GetMuteRequest request{};
    any.UnpackTo(&request);

    direction = static_cast<StreamDirection>(request.dir().type());

    audioMsgListener->getMuteState(audioReq, request.streamid(), direction);
}

void AudioGrpcServiceImpl::sendGetMuteStateResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId, StreamMute muteInfo) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::GetMuteResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getMute");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    response.mutable_mutestatus()->set_enable(muteInfo.enable);
    response.mutable_mutestatus()->mutable_direction()->set_type(
            static_cast<audioStub::StreamDirection_Type>(muteInfo.dir));
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }
    LOG(ERROR, __FUNCTION__, "Client Id not found" );

}

void AudioGrpcServiceImpl::read(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioStub::readRequest request{};
    any.UnpackTo(&request);

    audioMsgListener->read(audioReq, request.streamid(), request.numbytestoread());
}

void AudioGrpcServiceImpl::sendReadResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId, std::shared_ptr<std::vector<uint8_t>> data,
        uint32_t actualReadLength, uint32_t offset, int64_t timeStamp, bool isIncallStream,
        bool isHpcmStream) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::readResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }
    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "read");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    response.set_datalength(actualReadLength);
    //Get raw pointer to data buffer
    uint8_t* bufferPtr = data->data();
    response.set_buffer(bufferPtr, actualReadLength);
    resp.mutable_any()->PackFrom(response);

    if(isIncallStream || isHpcmStream) {
        std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));
    }

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, " Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
    }
}

void AudioGrpcServiceImpl::write(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    bool isLastBuffer;
    audioStub::writeRequest request{};
    any.UnpackTo(&request);

    uint8_t *data = new uint8_t[request.datalength()]{};

    std::string buffer = request.buffer();
    for (uint32_t i = 0; i < buffer.size(); i++) {
        data[i] = buffer[i];
    }

    isLastBuffer = static_cast<bool>(request.islastbuffer());

    audioMsgListener->write(audioReq, request.streamid(), data,
        request.datalength(), request.offset(), request.timestamp(), isLastBuffer);
}

void AudioGrpcServiceImpl::sendWriteResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId, uint32_t actualDataLengthWritten, bool isIncallStream,
        bool isHpcmStream) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::writeResponse response{};
    ApiResponse apiResp{};


    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }
    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "write");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    response.set_datalength(actualDataLengthWritten);
    resp.mutable_any()->PackFrom(response);

    if(isIncallStream || isHpcmStream) {
        std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));
    }

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()) {
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::startDtmf(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    DtmfTone dtmfTone{};
    StreamConfiguration config{};
    audioStub::StartDtmfToneRequest request{};
    any.UnpackTo(&request);

    dtmfTone.direction = static_cast<StreamDirection>(request.dtmftone().direction().type());
    dtmfTone.lowFreq = static_cast<DtmfLowFreq>(request.dtmftone().lowfreq().type());
    dtmfTone.highFreq = static_cast<DtmfHighFreq>(request.dtmftone().highfreq().type());

    audioMsgListener->startDtmf(audioReq, request.streamid(), request.gain(),
        request.duration(), dtmfTone);
}

void AudioGrpcServiceImpl::sendStartDtmfResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::StartDtmfToneResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "playDtmfTone");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::stopDtmf(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    StreamDirection direction{};
    audioStub::StopDtmfToneRequest request{};
    any.UnpackTo(&request);

    direction = static_cast<StreamDirection>(request.dir().type());

    audioMsgListener->stopDtmf(audioReq, request.streamid(), direction);
}

void AudioGrpcServiceImpl::sendStopDtmfResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::StopDtmfToneResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "stopDtmfTone");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}


void AudioGrpcServiceImpl::startTone(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    std::vector<uint16_t> toneFrequency;
    audioStub::PlayToneRequest request{};
    any.UnpackTo(&request);

    for (auto freq : request.freq()) {
        toneFrequency.emplace_back(freq);
    }

    audioMsgListener->startTone(audioReq, request.streamid(), request.gain(), request.duration(),
        toneFrequency);
}

void AudioGrpcServiceImpl::sendStartToneResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::PlayToneResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "startTone");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::stopTone(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioStub::StopToneRequest request{};
    any.UnpackTo(&request);

    audioMsgListener->stopTone(audioReq, request.streamid());
}

void AudioGrpcServiceImpl::sendStopToneResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::StopToneResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "stopTone");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::drain(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioStub::DrainRequest request{};
    any.UnpackTo(&request);

    audioMsgListener->drain(audioReq, request.streamid());
}

void AudioGrpcServiceImpl::sendDrainResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::StopStreamResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "drain");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::flush(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioStub::FlushRequest request{};
    any.UnpackTo(&request);

    audioMsgListener->flush(audioReq, request.streamid());
}

void AudioGrpcServiceImpl::sendFlushResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t streamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::StopStreamResponse response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "flush");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.set_streamid(streamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::createTranscoder(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    TranscodingFormatInfo inInfo{};
    TranscodingFormatInfo outInfo{};
    // CreatedTranscoderInfo createdTranscoderInfo{};
    audioStub::FormatInfo request{};
    any.UnpackTo(&request);

    inInfo.sampleRate = request.insamplerate();
    inInfo.mask = static_cast<telux::audio::ChannelTypeMask>(
        request.inchanneltype().type());
    inInfo.format = static_cast<AudioFormat>(request.inaudioformat().type());
    inInfo.bitWidth = request.inparams().bitwidth();
    inInfo.frameFormat = static_cast<AmrwbpFrameFormat>(request.inparams().frameformat().type());

    outInfo.sampleRate = request.outsamplerate();
    outInfo.mask = static_cast<telux::audio::ChannelTypeMask>(
        request.outchanneltype().type());
    outInfo.format = static_cast<AudioFormat>(request.outaudioformat().type());
    outInfo.bitWidth = request.outparams().bitwidth();
    outInfo.frameFormat = static_cast<AmrwbpFrameFormat>(request.outparams().frameformat().type());

    audioMsgListener->createTranscoder(audioReq, inInfo, outInfo);
}

void AudioGrpcServiceImpl::sendCreateTranscoderResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        CreatedTranscoderInfo createdTranscoderInfo) {

    audioStub::CreatedTranscoderInfo response{};
    audioStub::AsyncResponseMessage resp{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "createTranscoder");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    if(ec == telux::common::ErrorCode::SUCCESS){
        resp.set_error(static_cast<commonStub::ErrorCode>(apiResp.error));
    } else{
        resp.set_error(static_cast<commonStub::ErrorCode>(ec));
    }

    response.set_instreamid(createdTranscoderInfo.inStreamId);
    response.set_outstreamid(createdTranscoderInfo.outStreamId);
    response.set_readminsize(createdTranscoderInfo.readMinSize);
    response.set_readmaxsize(MAX_BUFFER_SIZE);
    response.set_writeminsize(createdTranscoderInfo.writeMinSize);
    response.set_writemaxsize(MAX_BUFFER_SIZE);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, " Client Id ", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
    }
}

void AudioGrpcServiceImpl::deleteTranscoder(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {
    audioStub::DeleteTranscoder request{};
    any.UnpackTo(&request);

    audioMsgListener->deleteTranscoder(audioReq, request.instreamid(), request.outstreamid());

}

void AudioGrpcServiceImpl::sendDeleteTranscoderResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t inStreamId, uint32_t outStreamId) {

    audioStub::AsyncResponseMessage resp{};
    audioStub::DeleteTranscoder response{};
    ApiResponse apiResp{};

    telux::common::Status status = jsonHelper_->loadJson();
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "deleteStream");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    if(ec == telux::common::ErrorCode::SUCCESS){
        resp.set_error(static_cast<commonStub::ErrorCode>(apiResp.error));
    } else{
        resp.set_error(static_cast<commonStub::ErrorCode>(ec));
    }

    response.set_instreamid(inStreamId);
    response.set_outstreamid(outStreamId);
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, "Client Id not found" );
}

void AudioGrpcServiceImpl::getCalibrationStatus(
        google::protobuf::Any any,
        std::shared_ptr<AudioRequest> audioReq,
        std::shared_ptr<IAudioMsgListener> audioMsgListener) {

    audioMsgListener->getCalibrationStatus(audioReq);
}


void AudioGrpcServiceImpl::sendGetCalibrationStatusResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        CalibrationInitStatus status) {

    audioStub::GetCalibrationInitStatusResponse response{};
    audioStub::AsyncResponseMessage resp{};
    ApiResponse apiResp{};

    telux::common::Status resStatus = jsonHelper_->loadJson();
    if (resStatus != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
    }

    jsonHelper_->getApiResponse(&apiResp, "IAudioManager", "getCalibrationInitStatus");

    if (apiResp.cbDelay == SKIP_CALLBACK) {
        LOG(INFO, __FUNCTION__, " Dropping response based on json config");
        return;
    }

    resp.set_msgid(audioReq->getMsgId());
    resp.set_cmdid(audioReq->getCmdId());
    resp.set_error(static_cast<commonStub::ErrorCode>(ec));

    response.mutable_calstatus()->set_type(
        static_cast<audioStub::CalibrationInitStatus_Type>(status));
    resp.mutable_any()->PackFrom(response);

    std::this_thread::sleep_for(std::chrono::milliseconds(apiResp.cbDelay));

    if(serverStreamMap_.find(audioReq->getClientId())!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", audioReq->getClientId());
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[audioReq->getClientId()]->Write(resp);
        }
    }
}

void AudioGrpcServiceImpl::sendWriteReadyEvent(
        int clientId,
        uint32_t streamId) {

    audioStub::WriteReadyEvent writeReadyEvent{};
    audioStub::AsyncResponseMessage resp{};

    writeReadyEvent.set_streamid(streamId);
    resp.mutable_any()->PackFrom(writeReadyEvent);
    resp.set_msgid(STREAM_WRITE_IND);

    /* Send the indication on audio server grpc stream */
    if(serverStreamMap_.find(clientId)!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", clientId);
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[clientId]->Write(resp);
        }
    }
}

void AudioGrpcServiceImpl::sendDrainDoneEvent(
        int clientId,
        uint32_t streamId) {

    audioStub::DrainEvent drainEvent{};
    audioStub::AsyncResponseMessage resp{};

    drainEvent.set_streamid(streamId);
    resp.mutable_any()->PackFrom(drainEvent);
    resp.set_msgid(STREAM_DRAIN_IND);

    /* Send the indication on audio server grpc stream */
    if(serverStreamMap_.find(clientId)!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", clientId);
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[clientId]->Write(resp);
        }
    }
}

void AudioGrpcServiceImpl::sendDTMFDetectedEvent(
        int clientId,
        uint32_t streamId, uint32_t lowFreq, uint32_t highFreq,
        StreamDirection streamDirection) {

    audioStub::DtmfTone dtmfDetectionEvent{};
    audioStub::AsyncResponseMessage resp{};

    dtmfDetectionEvent.mutable_lowfreq()->set_type(
            static_cast<::audioStub::DtmfLowFreq_Type>(lowFreq));
    dtmfDetectionEvent.mutable_highfreq()->set_type(
            static_cast<::audioStub::DtmfHighFreq_Type>(highFreq));
    dtmfDetectionEvent.mutable_direction()->set_type(
            static_cast<::audioStub::StreamDirection_Type>(StreamDirection::RX));
    resp.mutable_any()->PackFrom(dtmfDetectionEvent);

    resp.set_msgid(STREAM_DTMF_DETECTED_IND);

    /* Send the indication on audio server grpc stream */
    if(serverStreamMap_.find(clientId)!=serverStreamMap_.end()){
        LOG(ERROR, __FUNCTION__, "Client Id", clientId);
        {
            std::lock_guard<std::mutex> lk(streamWriterMtx_);
            serverStreamMap_[clientId]->Write(resp);
        }
    }

}

} // end namespace audio
} // end namespace telux
