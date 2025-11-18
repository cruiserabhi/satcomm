/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "VoiceStreamImpl.hpp"
#include "PlayStreamImpl.hpp"
#include "CaptureStreamImpl.hpp"
#include "LoopbackStreamImpl.hpp"
#include "ToneGeneratorStreamImpl.hpp"
#include "AudioManagerImpl.hpp"

namespace telux {
namespace audio {

std::atomic<bool> AudioManagerImpl::exitNow_;
std::mutex AudioManagerImpl::serviceStatusGuard_;

AudioManagerImpl::AudioManagerImpl() {
    AudioManagerImpl::exitNow_ = false;
}

AudioManagerImpl::~AudioManagerImpl() {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lock(AudioManagerImpl::serviceStatusGuard_);
    AudioManagerImpl::exitNow_ = true;
}

/*
 * Setup/initiate connection to audio grpc server.
 * Complete non-blocking initializations and schedule blocking ones.
 */
telux::common::Status AudioManagerImpl::init(telux::common::InitResponseCb initResultListener) {

    telux::common::Status status;
    std::shared_future<void> future;

    /* Setup AudioGrpcClient */
    transportClient_ = std::make_shared<AudioGrpcClientStub>();
    if (!transportClient_) {
        return telux::common::Status::FAILED;
    }

    status = transportClient_->setup();
    if (status != telux::common::Status::SUCCESS) {
        return status;
    }

    try {
        serviceStatusListenerMgr_ = std::make_shared<
            telux::common::ListenerManager<telux::audio::IAudioListener>>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't setup ListenerManager");
        return telux::common::Status::FAILED;
    }

	initCb_ = initResultListener;

    /* Schedule blocking initializations */
    future = std::async(std::launch::async, [this]() { this->initSync(); }).share();

    /* Hold onto future's reference until initSync() finishes */
    status = asyncTaskQueue_.add(future);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't add to queue");
        return status;
    }

    return telux::common::Status::SUCCESS;
}

/*
 * Completes blocking initializations establishing physical GRPC connection
 * with the audio server.
 *
 * Following 24 cases are possible with few more possible due to thread scheduling. These
 * are handled by the overall implementation of the subsystem readiness design. Audio server
 * can report service is available/unavailable from Q6/ADSP SSR point of view. GRPC framework
 * reports whether it is able to find intended GRPC service or not, hence reporting service
 * available/unavailable. SSR and GRPC are independent of each other, therefore, below
 * combinations are possible.
 *
 * [1] SSR-available, SSR-unavailable,     [2] GRPC-available,   GRPC-unavailable
 * SSR-unavailable,   SSR-available,       [2] GRPC-available,   GRPC-unavailable
 * [3] GRPC-available, SSR-available,       SSR-unavailable,     GRPC-unavailable
 * SSR-available,     [2] GRPC-available,   SSR-unavailable,     GRPC-unavailable
 * SSR-unavailable,   [2] GRPC-available,   SSR-available,       GRPC-unavailable
 * GRPC-available,     SSR-unavailable,     SSR-available,       GRPC-unavailable
 * GRPC-available,     SSR-unavailable,     GRPC-unavailable,     SSR-available
 * SSR-unavailable,   [2] GRPC-available,   GRPC-unavailable,     SSR-available
 * GRPC-unavailable,   GRPC-available,       SSR-unavailable,     SSR-available
 * GRPC-available,     GRPC-unavailable,     [4] SSR-unavailable, SSR-available
 * SSR-unavailable,   GRPC-unavailable,     GRPC-available,       [6] SSR-available
 * GRPC-unavailable,   [4] SSR-unavailable, GRPC-available,       SSR-available
 * GRPC-unavailable,   [4] SSR-available,   GRPC-available,       SSR-unavailable
 * SSR-available,     GRPC-unavailable,     GRPC-available,       SSR-unavailable
 * GRPC-available,     GRPC-unavailable,     SSR-available,       SSR-unavailable
 * GRPC-unavailable,   GRPC-available,       SSR-available,       SSR-unavailable
 * SSR-available,     [5] GRPC-available,   GRPC-unavailable,     SSR-unavailable
 * GRPC-available,     SSR-available,       GRPC-unavailable,     SSR-unavailable
 * SSR-unavailable,   SSR-available,       GRPC-unavailable,     GRPC-available
 * SSR-available,     SSR-unavailable,     GRPC-unavailable,     GRPC-available
 * GRPC-unavailable,   [4] SSR-unavailable, SSR-available,       GRPC-available
 * SSR-unavailable,   GRPC-unavailable,     [4] SSR-available,   GRPC-available
 * SSR-available,     GRPC-unavailable,     [4] SSR-unavailable, GRPC-available
 * GRPC-unavailable,   [4] SSR-available,   SSR-unavailable,     GRPC-available
 *
 * [1] Q6 SSR occurred, and then audio server is launched. So, it missed unavailable event.
 *     Now, server gets service available from HAL/PAL and delivers it to the client.
 * [2] Since SSR event is received, GRPC-connection exist, therefore, further sequence
 *     is invalid.
 * [3] This is possible when first application is run and then audio server is launched.
 * [4] After GRPC connection becomes unavailable, SSR event from server will not reach
 *     client, therefore, further sequence is invalid.
 * [5] If SSR reports available, GRPC service must be available already, therefore,
 *     further sequence is invalid.
 * [6] Q6 crashed followed by the server crash. If the server doesn't start early enough to
 *     receive service available event from PAL, application will never receive service
 *     available event since it is not sent by the server itself.
 */
void AudioManagerImpl::initSync() {
    LOG(DEBUG, __FUNCTION__);

    bool isSvcReady;
    std::future<bool> future;

    /* Block until connected to the audio server */
    isSvcReady = transportClient_->isReady();
    if (!isSvcReady) {
        future = transportClient_->onReady();
        isSvcReady = future.get();
    }

    transportClient_->registerForServiceStatusEvents(shared_from_this());

    /* Once connected, update local copy of the current service state */
    {
        std::lock_guard<std::mutex> lock(serviceStatusGuard_);
        if (AudioManagerImpl::exitNow_) {
            LOG(WARNING, __FUNCTION__, " dropping initSync");
            return;
        }

        /*
         * The serviceStatusGuard_ along with statusFromQ6SSRUpdate_ and statusFromGRPCConnection_,
         * ensures that the initSync(), onQ6SSRUpdate() and application have consistent view;
         * either the service is available or unavailable. The initSync() and onQ6SSRUpdate()/
         * onTransportStatusUpdate() executes on different threads therefore, need to kept in
         * sync when deciding service is available or not.
         */
        if (isSvcReady
            && (statusFromQ6SSRUpdate_ == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            && (statusFromGRPCConnection_ == telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
            /*
             * Connection with server established, neither SSR occurred nor server crashed.
             * Inform the client interested in the service's status, we are live.
             */
            serviceCurrentStatus_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;
            if (initCb_) {
                initCb_(serviceCurrentStatus_);
            }
        } else {
            /* Currently, service is unavailable, let the SSR callback update application later */
            serviceCurrentStatus_ = telux::common::ServiceStatus::SERVICE_FAILED;
        }

        isInitComplete_ = true;
    }

    cv_.notify_all();
}

/*
 * Gives current state of the audio service.
 */
telux::common::ServiceStatus AudioManagerImpl::getServiceStatus() {

    std::lock_guard<std::mutex> lock(serviceStatusGuard_);

    return serviceCurrentStatus_;
}

/*
 * Application registration for service status events.
 */
telux::common::Status AudioManagerImpl::registerListener(std::weak_ptr<IAudioListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return serviceStatusListenerMgr_->registerListener(listener);
}

/*
 * Application de-registration for service status events.
 */
telux::common::Status AudioManagerImpl::deRegisterListener(std::weak_ptr<IAudioListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return serviceStatusListenerMgr_->deRegisterListener(listener);
}

/*
 * Audio server subsystem-restart process is either started or finished.
 * Update application about it.
 *
 * AudioManagerImpl::onQ6SSRUpdate() and AudioManagerImpl::onTransportStatusUpdate()
 * are called from the same dispatcher thread therefore serialized. Therefore,
 * flag serviceCurrentStatus_ will have a valid value at any instant of time.
 */
void AudioManagerImpl::onQ6SSRUpdate(telux::common::ServiceStatus newStatus) {
    LOG(DEBUG, __FUNCTION__, " status ", static_cast<int>(newStatus));

    {
        std::lock_guard<std::mutex> lock(serviceStatusGuard_);

        statusFromQ6SSRUpdate_ = newStatus;

        if (AudioManagerImpl::exitNow_) {
            LOG(WARNING, __FUNCTION__, " dropped update");
            return;
        }

        if (!isInitComplete_) {
            LOG(WARNING, __FUNCTION__, " dropped event");
            return;
        }

        if ((statusFromQ6SSRUpdate_ == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            && (statusFromGRPCConnection_ == telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
            serviceCurrentStatus_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;
        } else {
            serviceCurrentStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
        }
    }

    /*
     * Inform application via getAudioManager's init response callback that we are live.
     * This update is sent as part of the subsystem readiness design. Note, once callbacks
     * referred by initCb_ are invoked that list is cleared, so invoking it again will not
     * result in application's init response callback getting called two or more times.
     */
    if (initCb_ && (serviceCurrentStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
        initCb_(serviceCurrentStatus_);
    }

    /*
     * This update to the application is sent, if it has registered for the SSR events.
     */
    sendNewStatusToClients(serviceCurrentStatus_);
}

/*
 * We are connected/disconnected from server. Update application about it.
 */
void AudioManagerImpl::onTransportStatusUpdate(telux::common::ServiceStatus newStatus) {

    telux::common::Status status = telux::common::Status::FAILED;

    LOG(DEBUG, __FUNCTION__, " status ", static_cast<int>(newStatus));

    {
        std::lock_guard<std::mutex> lock(serviceStatusGuard_);

        statusFromGRPCConnection_ = newStatus;

        if (AudioManagerImpl::exitNow_) {
            LOG(WARNING, __FUNCTION__, " dropped status");
            return;
        }

        if (!isInitComplete_) {
            LOG(WARNING, __FUNCTION__, " dropped event");
            return;
        }

        if ((statusFromQ6SSRUpdate_ == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            && (statusFromGRPCConnection_ == telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
            serviceCurrentStatus_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;
        } else {
            serviceCurrentStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
        }

        if (statusFromGRPCConnection_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            /* Re-subscribe for server-connection events */
            auto f = std::async(std::launch::async, [&]() { this->initSync(); }).share();

            status = asyncTaskQueue_.add(f);
            if (status != telux::common::Status::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " can't add to queue");
                return;
            }
        }
    }

    if (initCb_ && (serviceCurrentStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
        initCb_(serviceCurrentStatus_);
    }

    sendNewStatusToClients(serviceCurrentStatus_);
}

/*
 * Update clients with the new service status.
 */
void AudioManagerImpl::sendNewStatusToClients(telux::common::ServiceStatus newStatus) {

    std::vector<std::weak_ptr<IAudioListener>> applisteners;

    if (newStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        /* Send new service status to all *StreamImpl object for internal state cleanup */
        for(auto &wp : createdStreams_){
            if (auto sp = wp.lock()) {
                sp->onServiceStatusChange();
            }
        }
        createdStreams_.clear();

        for (auto &wp : createdTranscoders_) {
            if (auto sp = wp.lock()) {
                sp->onServiceStatusChange();
            }
        }
        createdTranscoders_.clear();
    }

    /*
     * Handle two or more consecutive service available or unavailable events.
     *
     * 1. SSR happens, service becomes unavailable, we sent unavailable
     *    status to the applcation.
     * 2. Application is now waiting for service available status.
     * 3. Server crashed, connection lost, AudioManagerImpl::onServiceStatusChange()
     *    invoked, leading to second consecutive service unavailable status message
     *    sent to the application. Prevent sending this 2nd same status as there is
     *    no advantage of sending it to the application.
     *
     * This scenario may further complicate things if it happens during initSync().
     * statusFromGRPCConnection_ is used in initSync() to address this.
     */
    if (lastServiceStatusSent_ == newStatus) {
        LOG(DEBUG, __FUNCTION__, " dropped repeated status");
        return;
    }
    lastServiceStatusSent_ = newStatus;

    /* Send new service status to all registered application listeners */
    if (!serviceStatusListenerMgr_) {
        LOG(ERROR, __FUNCTION__, " invalid listener mgr");
        return;
    }
    serviceStatusListenerMgr_->getAvailableListeners(applisteners);

    if (!applisteners.empty()) {
        for (auto &wp : applisteners) {
            if (auto sp = std::dynamic_pointer_cast<IAudioListener>(wp.lock())) {
                sp->onServiceStatusChange(newStatus);
            }
        }
        LOG(DEBUG, __FUNCTION__, " sent status ", static_cast<int>(newStatus));
    } else {
        LOG(DEBUG, __FUNCTION__, " no status listener");
    }

    if (newStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        /*
         * When application called an API and passed callback, its reference was cached
         * with CommandCallbackManager. We will never call this callback now, therefore
         * remove it.
         *
         * When service becomes unavailable, application should give up references to
         * all types of streams (*StreamImpl) as all of them have become invalid now.
         * Resources allocated to streams will be released in their destructors therefore
         * no explicit cleanup is required here.
         */
        cmdCallbackMgr_.reset();
    }
}

/*
 * Gives a list of currently supported audio device types like mic and speaker etc.
 */
telux::common::Status AudioManagerImpl::getDevices(GetDevicesResponseCb callback) {

    int cmdId;
    telux::common::Status status;

    /*
     * For all getFoo() APIs, ideally application should pass callback. However,
     * since it is an optional parameter and application may choose to not pass
     * it for any reason, we don't check for !callback case here. Requested info
     * is fetched from audio server and dropped at library level.
     */
    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    /*
     * CommandCallbackManager, internally saves memory address of the user provided
     * location as key and callback as the value in a map. Instead of passing memory
     * location, we are passing cmdId (integer) after typecasting it. Because integer
     * is 32 bit wide while pointer can be 32 or 64 bit wide, when typecasting back
     * from void * to integer, we get the actual value. Using this approach,
     * performance and memory gain is obtained by not allocating memory for every
     * API when sending async request.
     */
    status = transportClient_->getDevices(shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }
    return status;

}

void AudioManagerImpl::onGetDevicesResult(telux::common::ErrorCode ec,
        std::vector<DeviceType> deviceTypes,
        std::vector<DeviceDirection> deviceDirections, int cmdId) {

    uint32_t size = 0;
    std::shared_ptr<telux::audio::AudioDeviceImpl> device;
    std::shared_ptr<telux::common::ICommandCallback> resultListener;
    std::vector<std::shared_ptr<telux::audio::AudioDeviceImpl>> devices;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        /* Unexpected as we made supplying resultListener mandatory */
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    size = deviceTypes.size();

    for (uint32_t x = 0; x < size; ++x) {
        try {
            device = std::make_shared<telux::audio::AudioDeviceImpl>(
                        deviceTypes.at(x), deviceDirections.at(x));
        } catch (const std::exception& e) {
            LOG(ERROR, __FUNCTION__, " can't create AudioDeviceImpl");
            /* can't do anything, continue to help debugging */
            continue;
        }

        devices.emplace_back(device);
    }

    cmdCallbackMgr_.executeCallback(resultListener, devices, ec);
}

/*
 * Gives a list of currently supported audio stream types like playback and voice-call etc.
 */
telux::common::Status AudioManagerImpl::getStreamTypes(GetStreamTypesResponseCb callback) {

    int cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    cmdId = cmdCallbackMgr_.addCallback(callback);

    status = transportClient_->getStreamTypes(shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

void AudioManagerImpl::onGetStreamsResult(telux::common::ErrorCode ec,
        std::vector<StreamType> streams, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, streams, ec);
}

/*
 * Applicable only for HAL, gives ACDB loaing and init status as obtained from HAL.
 */
telux::common::Status AudioManagerImpl::getCalibrationInitStatus(
        GetCalInitStatusResponseCb callback) {

    intptr_t cmdId;
    telux::common::Status status;

    if (!callback) {
        return telux::common::Status::INVALIDPARAM;
    }

    cmdId = cmdCallbackMgr_.addCallback(callback);

    status = transportClient_->getCalibrationInitStatus(shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

void AudioManagerImpl::onGetCalInitStatusResult(telux::common::ErrorCode ec,
        CalibrationInitStatus calibrationStatus, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, calibrationStatus, ec);
}

/*
 * Creates an audio stream with parameters specified by streamConfig. This method
 * causes stream creation on the server side whose ID is obtained in method
 * AudioManagerImpl::onCreateStreamResult().
 */
telux::common::Status AudioManagerImpl::createStream(
    StreamConfig streamConfig, CreateStreamResponseCb callback) {

    intptr_t cmdId;
    telux::common::Status status;

    if (!callback) {
        /*
         * When an audio stream has been created successfully on the server side,
         * corresponding *StreamImpl object is created on the client side to represent
         * this stream. Callback is the only way through which an application can
         * retrive this stream object and execute further operations on it. Therefore,
         * application must provide this callback.
         */
        return telux::common::Status::INVALIDPARAM;
    }

    if (streamConfig.deviceTypes.size() > MAX_DEVICES) {
        LOG(ERROR, __FUNCTION__, " exceeded maximum device count");
        return telux::common::Status::INVALIDPARAM;
    }

    if (streamConfig.voicePaths.size() > MAX_VOICE_PATH) {
        LOG(ERROR, __FUNCTION__, " exceeded maximum voice path count");
        return telux::common::Status::INVALIDPARAM;
    }

    cmdId = cmdCallbackMgr_.addCallback(callback);

    status = transportClient_->createStream(streamConfig, shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * Creates an audio stream with parameters specified by createdStreamInfo. This method
 * causes stream creation on the client side.
 *
 * If the stream creation is successfull on server-side but failure happens on client-side
 * delete the stream on server-side and return error to the application.
 */
void AudioManagerImpl::onCreateStreamResult(telux::common::ErrorCode ec,
        CreatedStreamInfo createdStreamInfo, int cmdId) {

    telux::common::Status status;
    std::shared_ptr<IAudioStream> audioStream;
    std::shared_ptr<PlayStreamImpl> playStream;
    std::shared_ptr<VoiceStreamImpl> voiceStream;
    std::shared_ptr<CaptureStreamImpl> captureStream;
    std::shared_ptr<LoopbackStreamImpl> loopbackStream;
    std::shared_ptr<ToneGeneratorStreamImpl> toneStream;
    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        if (ec == telux::common::ErrorCode::SUCCESS) {
            ec = telux::common::ErrorCode::INVALID_ARGUMENTS;
            goto error2;
        } else {
            goto error1;
        }
    }

    if (ec != telux::common::ErrorCode::SUCCESS) {
        /* Stream creation failed on server-side itself, update application */
        cmdCallbackMgr_.executeCallback(resultListener, nullptr, ec);
        return;
    }

    /*
     * Stream created successfully on the server side, create corresponding *StreamImpl
     * proxy on the client side. Associate streamId with it to uniquely identify it.
     */
    try {
        switch(createdStreamInfo.streamType) {
            case StreamType::VOICE_CALL:
                voiceStream = std::make_shared<VoiceStreamImpl>(createdStreamInfo.streamId,
                            transportClient_);
                status = voiceStream->init();
                if (status != telux::common::Status::SUCCESS) {
                    ec = telux::common::ErrorCode::GENERIC_FAILURE;
                    goto error2;
                }
                audioStream = voiceStream;
                createdStreams_.push_back(voiceStream);
                break;
            case StreamType::PLAY:
                playStream = std::make_shared<PlayStreamImpl>(createdStreamInfo.streamId,
                            createdStreamInfo.writeMinSize,
                            createdStreamInfo.writeMaxSize, transportClient_);
                status = playStream->init();
                if (status != telux::common::Status::SUCCESS) {
                    ec = telux::common::ErrorCode::GENERIC_FAILURE;
                    goto error2;
                }
                audioStream = playStream;
                createdStreams_.push_back(playStream);
                break;
            case StreamType::CAPTURE:
                captureStream = std::make_shared<CaptureStreamImpl>(createdStreamInfo.streamId,
                            createdStreamInfo.readMinSize,
                            createdStreamInfo.readMaxSize, transportClient_);
                audioStream = captureStream;
                createdStreams_.push_back(captureStream);
                break;
            case StreamType::LOOPBACK:
                loopbackStream = std::make_shared<LoopbackStreamImpl>(createdStreamInfo.streamId,
                    transportClient_);
                audioStream = loopbackStream;
                createdStreams_.push_back(loopbackStream);
                break;
            case StreamType::TONE_GENERATOR:
                toneStream = std::make_shared<ToneGeneratorStreamImpl>(createdStreamInfo.streamId,
                    transportClient_);
                audioStream = toneStream;
                createdStreams_.push_back(toneStream);
                break;
            default:
                LOG(ERROR, __FUNCTION__, " invalid stream type ",
                    static_cast<int>(createdStreamInfo.streamType));
                ec = telux::common::ErrorCode::INVALID_ARGUMENTS;
                goto error2;
        }
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create *StreamImpl");
        ec = telux::common::ErrorCode::NO_MEMORY;
        goto error2;
    }

    /* Update application stream created successfully and pass reference to it */
    cmdCallbackMgr_.executeCallback(resultListener, audioStream, ec);
    return;

error2:
    /* Delete stream on the server side */
    transportClient_->deleteStream(createdStreamInfo.streamId, nullptr, 0);

error1:
    /* Update application stream creation failed */
    cmdCallbackMgr_.executeCallback(resultListener, nullptr, ec);
}

/*
 * Closes stream and release all resources allocated.
 */
telux::common::Status AudioManagerImpl::deleteStream(
    std::shared_ptr<IAudioStream> stream, DeleteStreamResponseCb callback) {

    intptr_t cmdId;
    uint32_t streamId;
    telux::common::Status status;
    std::shared_ptr<AudioStreamImpl> audioStreamImpl;

    if (!stream) {
        LOG(ERROR, __FUNCTION__, " no stream given");
        return telux::common::Status::INVALIDPARAM;
    }

    audioStreamImpl = std::dynamic_pointer_cast<AudioStreamImpl>(stream);

    streamId = audioStreamImpl->getStreamId();

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->deleteStream(streamId, shared_from_this(), cmdId);
    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

void AudioManagerImpl::onDeleteStreamResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    if ((cmdId) == INVALID_COMMAND_ID) {
        /* Caller not interested in knowing whether deleting stream
         * was successfull or failed */
        return;
    }

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Creates two audio streams, playback and capture and configures them for transcoding.
 */
telux::common::Status AudioManagerImpl::createTranscoder(
        FormatInfo input, FormatInfo output, CreateTranscoderResponseCb callback) {

    intptr_t cmdId;
    telux::common::Status status;

    if (!input.params) {
        return telux::common::Status::INVALIDPARAM;
    }

    if (!callback) {
        /*
         * When transcoder streams (playback/capture) are created successfully on the
         * server side, corresponding *StreamImpl objects are created on the client side
         * to represent these streams (encapsulated in TranscoderImpl). Callback is the
         * only way through which an application can use these streams via ITranscoder.
         * Therefore, application must provide this callback.
         */
        return telux::common::Status::INVALIDPARAM;
    }

    cmdId = cmdCallbackMgr_.addCallback(callback);

    status = transportClient_->createTranscoder(input, output, shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

void AudioManagerImpl::onCreateTranscoderResult(telux::common::ErrorCode ec,
        CreatedTranscoderInfo transcoderInfo, int cmdId) {

    telux::common::Status status;
    std::shared_ptr<TranscoderImpl> transcoder;
    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    if (ec != telux::common::ErrorCode::SUCCESS) {
        /* Transcoder creation failed at server-side */
        cmdCallbackMgr_.executeCallback(resultListener, nullptr, ec);
        return;
    }

    try {
        transcoder = std::make_shared<TranscoderImpl>(transcoderInfo, transportClient_);
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create TranscoderImpl");
        cmdCallbackMgr_.executeCallback(resultListener, nullptr,
            telux::common::ErrorCode::NO_MEMORY);
        return;
    }

    status = transcoder->init();
    if (status != telux::common::Status::SUCCESS) {
        cmdCallbackMgr_.executeCallback(resultListener, nullptr,
            telux::common::ErrorCode::GENERIC_FAILURE);
        return;
    }

    createdTranscoders_.push_back(transcoder);

    cmdCallbackMgr_.executeCallback(resultListener, transcoder, ec);
}

/* deprecated */
bool AudioManagerImpl::isSubsystemReady() {
    LOG(WARNING, __FUNCTION__, " deprecated API used!");
    return (getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE);
}

bool AudioManagerImpl::waitForInitialization() {
    if (isSubsystemReady()) {
        return true;
    }

    std::unique_lock<std::mutex> cvLock(serviceStatusGuard_);
    cv_.wait(cvLock,
        [&] { return (serviceCurrentStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE); });

    return true;
}

/* deprecated */
std::future<bool> AudioManagerImpl::onSubsystemReady() {
    LOG(WARNING, __FUNCTION__, " deprecated API used!");
    return std::async(std::launch::async, [&] { return waitForInitialization(); });
}

}  // End of namespace audio
}  // End of namespace telux
