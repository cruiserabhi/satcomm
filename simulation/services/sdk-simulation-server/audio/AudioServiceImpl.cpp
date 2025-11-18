/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "libs/common/Logger.hpp"

#include "AudioServiceImpl.hpp"
#include <algorithm>

/* Used to check if a voice call is initiated on Slot Id 1 & Slot Id 2. */
std::vector<int> voiceCallList(3,0);

namespace telux {
namespace audio {

AudioServiceImpl::AudioServiceImpl() {
    LOG(DEBUG, __FUNCTION__);
}

AudioServiceImpl::~AudioServiceImpl() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status AudioServiceImpl::initService() {

    telux::common::ErrorCode ec;
    try {
        streamCache_ = std::make_shared<StreamCache>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create StreamCache");
        return telux::common::Status::FAILED;
    }

    try {
        clientCache_ = std::make_shared<ClientCache>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create ClientCache");
        return telux::common::Status::FAILED;
    }

    try {
        audioBackend_ = std::make_shared<Alsa>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create audio backend");
        return telux::common::Status::FAILED;
    }

    ec = audioBackend_->init(shared_from_this());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't audio backend init failed");
        return telux::common::Status::FAILED;
    }

    serviceCommonTaskExecutor_ = std::unique_ptr<
        telux::common::TaskDispatcher>(new telux::common::TaskDispatcher());

    return telux::common::Status::SUCCESS;
}

void AudioServiceImpl::handleClientConnect(
        std::shared_ptr<AudioClient> audioClient) {
    clientCache_->cacheClient(audioClient->getClientId(), audioClient);
}

telux::common::Status AudioServiceImpl::onClientConnected(
        std::shared_ptr<AudioClient> audioClient,
        std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher) {

    audioMsgDispatcher_ = audioMsgDispatcher;
    serviceCommonTaskExecutor_->submitTask([=]{ handleClientConnect(audioClient); });

    LOG(DEBUG, __FUNCTION__, " client connected ", audioClient);
    return telux::common::Status::SUCCESS;
}

void AudioServiceImpl::handleClientDisconnect(std::shared_ptr<AudioClient> audioClient) {
    std::map<StreamType, std::vector<uint32_t>> clientStreamIdsList;

    clientStreamIdsList = audioClient->getAssociatedStreamIdList();
    for (auto itr = clientStreamIdsList.rbegin(); itr != clientStreamIdsList.rend(); ++itr) {
        for(auto streamId : itr->second) {
            clientCache_->disassociateStream(streamId);
            {
                std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
                if (stream) {
                    stream->cleanupStream(voiceCallList);
                }
            }
            streamCache_->unCacheStream(streamId);
            streamCache_->releaseStreamId(streamId);
        }
    }

    clientCache_->unCacheClient(audioClient);

    LOG(DEBUG, __FUNCTION__, " client disconnected ", audioClient);
}

telux::common::Status AudioServiceImpl::onClientDisconnected(
        std::shared_ptr<AudioClient> audioClient) {

    /* Execution must be in separate thread for following reasons
     (1) SSR event must serialize with disconnect event.
     (2) onclientDisconnected executes from service main thread,
         service main thread should not hold for long period of time.
     (3) service specific task may executes any QCSI APIs. to complete
         those APIs, service main task has to return back to QCSI.
     */
    serviceCommonTaskExecutor_->submitTask([=]{ handleClientDisconnect(audioClient); });
    return telux::common::Status::SUCCESS;
}

/*
 * 1. SSR event is received by HAL/PAL audio backend.
 * 2. Backend translates HAL/PAL specific event value (Hal::AUDIO_ON/OFFLINE) to server
 *    specific value (SSREvent::AUDIO_ON/OFFLINE).
 * 3. Backend calls onSSREvent() callback in server using ISSREventListener.
 * 4. This event is scheduled to execute on common server thread so that if
 *    (a) SSR happens 1st and then request to create stream comes, it will be dropped.
 *    (b) If stream create request comes before SSR, then 1st stream will be created
 *        then SSR will be handled.
 * 5. If Q6 has crashed, set ssrInProgress_ to true so that any subsequent create stream
 *    request is dropped until Q6 comes back online. Instruct all existing stream dispatchers
 *    to close stream and release resources. At the time, we want to close stream, there
 *    might be an operation executing on that stream which may result in segfault, if resources
 *    disappears abruptly. Therefore, cleanup is scheduled on stream specific dispatchers.
 *    Finally, send SSR event to all the managers on the client side (application).
 * 6. If the Q6 has recovered from SSR, unset ssrInProgress_ and inform all managers that
 *    audio service is online now. If there is any stream, which is still not cleaned up
 *    for some reason, it will eventually get cleaned when its dispatcher runs. New stream
 *    creation is done on common dispatcher thread and Q6 is online so anyway there is
 *    no problem.
 */
void AudioServiceImpl::handleSSREvent(SSREvent event) {

    std::map<int,std::shared_ptr<AudioClient>> audioClientsList;
    std::shared_ptr<IAudioMsgDispatcher> audioMsgDispatcher;

    audioMsgDispatcher = audioMsgDispatcher_.lock();

    /*** Handling service online case ***/

    if (event == SSREvent::AUDIO_ONLINE) {
        /* Server informs application that service is online but just before ssrInProgress_
         * flag is set to false, current thread is scheduled out. Now, if the application
         * sends a valid request, it will be dropped because as per this flag we are still
         * processing SSR. Therefore, first mark server is ready and then inform all
         * applications. */
        ssrInProgress_ = false;
        audioMsgDispatcher->broadcastServiceStatus(AUDIO_SERVICE_ONLINE);
        return;
    }

    /*** Handling service offline case ***/

    ssrInProgress_ = true;

    /* Update applications; SSR occurred, stop sending request and do cleanup at their end. */
    audioMsgDispatcher->broadcastServiceStatus(AUDIO_SERVICE_OFFLINE);

    /* Now while application-side is cleaning up, we will clean up the server-side */
    audioClientsList = clientCache_->getClientsList();
    for (auto it = audioClientsList.begin(); it != audioClientsList.end(); it++){
        auto clientStreamIdsList = it->second->getAssociatedStreamIdList();
        for (auto itr = clientStreamIdsList.rbegin(); itr != clientStreamIdsList.rend(); ++itr) {
            for(auto streamId : itr->second) {
                {
                    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
                    if (stream) {
                        stream->cleanupStream(voiceCallList);
                    }
                }
                streamCache_->unCacheStream(streamId);
            }
        }
    }

    clientCache_->disassociateAllStreams();
    streamCache_->purgeAllStreamIds();
}


/*
 * Post ssr event on server's common dispatcher thread for further processing when
 * HAL/PAL sends SSR state updates to us.
 *
 * 1. ADSP crashed but Q6 running: HAL/PAL is responsible for sending SSR
 *    event to us.
 * 2. Q6 crashed but ADSP running: Application should subscribe with telephony
 *    APIs to get SSR events.
 */
void AudioServiceImpl::onSSREvent(SSREvent event) {
    serviceCommonTaskExecutor_->submitTask([=]{ handleSSREvent(event); });
}

inline bool AudioServiceImpl::isSSRInProgress(void) {
    return ssrInProgress_;
}

/*
 * Create a list of audio devices like mic, speaker etc. and send it to the application.
 */
void AudioServiceImpl::doGetSupportedDevices(std::shared_ptr<AudioRequest> audioReq) {
    telux::common::ErrorCode ec;
    std::vector<DeviceType> devices;
    std::vector<DeviceDirection> devicesDirection;
    std::shared_ptr<IAudioMsgDispatcher> audioMsgDispatcher;

    ec = audioBackend_->getSupportedDevices(devices, devicesDirection);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        goto result;
    }

    if (devices.size() != devicesDirection.size()) {
        LOG(ERROR, __FUNCTION__, " mismatched number of devices ");
        ec = telux::common::ErrorCode::MISSING_RESOURCE;
        goto result;
    }

    LOG(DEBUG, __FUNCTION__, " total supported devices: ", devices.size());

result:
    audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }
    audioMsgDispatcher->sendGetSupportedDevicesResponse(audioReq, ec, devices, devicesDirection);
}

void AudioServiceImpl::getSupportedDevices(std::shared_ptr<AudioRequest> audioReq) {
    serviceCommonTaskExecutor_->submitTask([=]{ doGetSupportedDevices(audioReq); });
}

/*
 * Create a list of supported stream types and send it to the application.
 */
void AudioServiceImpl::doGetSupportedStreamTypes(std::shared_ptr<AudioRequest> audioReq) {

    telux::common::ErrorCode ec;
    std::vector<StreamType> streamTypes;
    std::shared_ptr<IAudioMsgDispatcher> audioMsgDispatcher;

    ec = audioBackend_->getSupportedStreamTypes(streamTypes);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't get stream types, err:", static_cast<int>(ec));
        goto result;
    }

    LOG(DEBUG, __FUNCTION__, " total supported stream types: ", streamTypes.size());

result:
    audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }
    audioMsgDispatcher->sendGetSupportedStreamTypesResponse(audioReq, ec, streamTypes);
}

void AudioServiceImpl::getSupportedStreamTypes(std::shared_ptr<AudioRequest> audioReq) {
    serviceCommonTaskExecutor_->submitTask([=]{ doGetSupportedStreamTypes(audioReq); });
}

/*
 * Get info whether ACDB settings are effective or not and send the result to the application.
 */
void AudioServiceImpl::doGetCalibrationStatus(std::shared_ptr<AudioRequest> audioReq) {

    telux::common::ErrorCode ec;
    CalibrationInitStatus status;
    std::shared_ptr<IAudioMsgDispatcher> audioMsgDispatcher;

    ec = audioBackend_->getCalibrationStatus(status);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        goto result;
    }

    LOG(DEBUG, __FUNCTION__, " calibration status read ", static_cast<int>(status));

result:
    audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }
    audioMsgDispatcher->sendGetCalibrationStatusResponse(audioReq, ec, status);
}

void AudioServiceImpl::getCalibrationStatus(std::shared_ptr<AudioRequest> audioReq) {
    serviceCommonTaskExecutor_->submitTask([=]{ doGetCalibrationStatus(audioReq); });
}

/*
 * 1. Reserve a unique identifier for stream.
 * 2. Create an audio stream.
 * 3. Cache stream at server-side. Stream is associated with identifier now.
 * 4. Associate this stream with the audio client.
 */
telux::common::ErrorCode AudioServiceImpl::doCreateStream(
        std::shared_ptr<AudioRequest> audioReq, StreamConfiguration config,
        TranscodingFormatInfo inInfo, TranscodingFormatInfo outInfo,
        StreamPurpose streamPurpose, CreatedTranscoderInfo *createdTranscoderInfo) {

    uint32_t streamId=0;
    uint32_t readMinSize=0, writeMinSize=0;
    std::shared_ptr<Stream> stream;
    telux::common::ErrorCode ec = telux::common::ErrorCode::GENERIC_FAILURE;

    std::shared_ptr<AudioClient> audioClient;
    std::shared_ptr<IAudioMsgDispatcher> audioMsgDispatcher;

    audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return ec;
    }

    /* Bail very early, if Q6/ADSP is currently undergoing SSR */
    if (ssrInProgress_) {
        return ec;
    }

    if(config.streamConfig.type == telux::audio::StreamType::VOICE_CALL){
        voiceCallList[config.streamConfig.slotId] = 1;
    }

    try {
        stream = std::make_shared<Stream>(audioBackend_, clientCache_);
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't allocate Stream");
        ec = telux::common::ErrorCode::NO_MEMORY;
        goto result;
    }

    /* Reserve a unique identifier for this stream */
    ec = streamCache_->getNextAvailableStreamID(streamId);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        goto result;
    }

    /* Create playback/capture/voicecall/loopback/tone stream */
    switch (streamPurpose) {
        case StreamPurpose::TRANSCODER_IN:
            /* Create transcoder input stream */
            createdTranscoderInfo->inStreamId = streamId;
            ec = stream->setupInTranscodeStream(inInfo, createdTranscoderInfo);
            break;
        case StreamPurpose::TRANSCODER_OUT:
            /* Create transcoder output stream */
            createdTranscoderInfo->outStreamId = streamId;
            ec = stream->setupOutTranscodeStream(outInfo, createdTranscoderInfo);
            break;
        default:
            /* Create playback/capture/voicecall/loopback/tone stream */
            ec = stream->setupStream(config, streamId, readMinSize, writeMinSize);
    }

    if (ec != telux::common::ErrorCode::SUCCESS) {
        streamCache_->releaseStreamId(streamId);
        goto result;
    }

    /* Associate stream with stream id */
    streamCache_->cacheStream(streamId, stream);

    /* Associate stream id with client */
    audioClient = clientCache_->getAudioClientFromClientId(audioReq->getClientId());
    clientCache_->associateStream(audioClient, config.streamConfig.type, streamId);

    LOG(INFO, __FUNCTION__, " stream created, strmid: ", streamId, " type ",
        static_cast<uint32_t>(config.streamConfig.type), " read min size ", readMinSize,
        " write min size ", writeMinSize, " read max size ", MAX_BUFFER_SIZE,
        " write max size ", MAX_BUFFER_SIZE);

result:
    if (streamPurpose != StreamPurpose::DEFAULT) {
        /* When creating transcoder-stream, response will be sent by doCreateTranscoder() */
        return ec;
    }

    audioMsgDispatcher->sendCreateStreamResponse(audioReq, ec, streamId,
        config.streamConfig.type, readMinSize, writeMinSize);

    return ec;
}

void AudioServiceImpl::createStream(std::shared_ptr<AudioRequest> audioReq,
        StreamConfiguration config) {

    TranscodingFormatInfo transcodeInfo{};

    serviceCommonTaskExecutor_->submitTask([=]{ doCreateStream(audioReq,
        config, transcodeInfo, transcodeInfo, StreamPurpose::DEFAULT, nullptr); });
}

/*
 * 1. Remove stream from server cache.
 * 2. Release identifer associated with this stream.
 * 3. Release resources allocated for this stream and finally close HAL/PAL
 *    stream.
 */
telux::common::ErrorCode AudioServiceImpl::doDeleteStream(
        std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, bool sendResponse) {

    std::shared_ptr<Stream> stream;
    std::shared_ptr<IAudioMsgDispatcher> audioMsgDispatcher;
    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    stream = streamCache_->retrieveStream(streamId);
    if (!stream) {
        LOG(DEBUG, __FUNCTION__, " can't find stream, strmid:", streamId);
        ec = telux::common::ErrorCode::INVALID_ARGUMENTS;
        goto result;
    }

    streamCache_->unCacheStream(streamId);
    streamCache_->releaseStreamId(streamId);
    clientCache_->disassociateStream(streamId);

    ec = stream->cleanupStream(voiceCallList);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " can't close stream, strmid:", streamId);
        goto result;
    }

    LOG(DEBUG, __FUNCTION__, " stream closed, strmid:", streamId);

result:
    if (!sendResponse) {
        /* When deleting transcoder-stream, response will be sent by doDeleteTranscoder() */
        return ec;
    }

    audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return ec;
    }

    audioMsgDispatcher->sendDeleteStreamResponse(audioReq, ec, streamId);
    return ec;
}

void AudioServiceImpl::deleteStream(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId) {

    serviceCommonTaskExecutor_->submitTask([=] {
            doDeleteStream(audioReq, streamId, true); });
}


void AudioServiceImpl::doCreateTranscoder(std::shared_ptr<AudioRequest> audioReq,
        TranscodingFormatInfo inInfo, TranscodingFormatInfo outInfo) {

    telux::common::ErrorCode ec;
    StreamConfiguration config{};
    CreatedTranscoderInfo createdTranscoderInfo{};
    std::shared_ptr<IAudioMsgDispatcher> audioMsgDispatcher;

    config.streamConfig.type = StreamType::PLAY;
    ec = doCreateStream(audioReq, config, inInfo, outInfo,
            StreamPurpose::TRANSCODER_IN, &createdTranscoderInfo);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        goto result;
    }

    config.streamConfig.type = StreamType::CAPTURE;
    ec = doCreateStream(audioReq, config, inInfo, outInfo,
            StreamPurpose::TRANSCODER_OUT, &createdTranscoderInfo);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        doDeleteStream(audioReq, createdTranscoderInfo.inStreamId, false);
    }

result:
    audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    audioMsgDispatcher->sendCreateTranscoderResponse(audioReq, ec, createdTranscoderInfo);
}

void AudioServiceImpl::createTranscoder(std::shared_ptr<AudioRequest> audioReq,
        TranscodingFormatInfo inInfo, TranscodingFormatInfo outInfo) {

    serviceCommonTaskExecutor_->submitTask([=]{ doCreateTranscoder(audioReq,
        inInfo, outInfo); });
}

void AudioServiceImpl::doDeleteTranscoder(std::shared_ptr<AudioRequest> audioReq,
        uint32_t inStreamId, uint32_t outStreamId) {

    telux::common::ErrorCode ec;
    std::shared_ptr<IAudioMsgDispatcher> audioMsgDispatcher;
    telux::common::ErrorCode finalErrorCode = telux::common::ErrorCode::SUCCESS;

    ec = doDeleteStream(audioReq, inStreamId, false);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        /* In case error happens, we still proceed further to minimize resource
         * leak, but preserve original error code to make application aware that
         * an error has occurred. */
        finalErrorCode = ec;
    }

    ec = doDeleteStream(audioReq, outStreamId, false);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        finalErrorCode = ec;
    }

    audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    audioMsgDispatcher->sendDeleteTranscoderResponse(audioReq, finalErrorCode,
        inStreamId, outStreamId);
}

void AudioServiceImpl::deleteTranscoder(std::shared_ptr<AudioRequest> audioReq,
        uint32_t inStreamId, uint32_t outStreamId) {

    serviceCommonTaskExecutor_->submitTask([=]{ doDeleteTranscoder(audioReq,
        inStreamId, outStreamId); });
}

void AudioServiceImpl::start(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->start(audioReq, streamId);
    }
}

void AudioServiceImpl::stop(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->stop(audioReq, streamId);
    }
}

void AudioServiceImpl::setDevice(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, std::vector<DeviceType> const &devices) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->setDevice(audioReq, streamId, devices);
    }
}

void AudioServiceImpl::getDevice(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->getDevice(audioReq, streamId);
    }
}

void AudioServiceImpl::setVolume(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, StreamDirection direction,
        std::vector<ChannelVolume> channelsVolume) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->setVolume(audioReq, streamId, direction, channelsVolume);
    }
}

void AudioServiceImpl::getVolume(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, StreamDirection direction) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->getVolume(audioReq, streamId, direction);
    }
}

void AudioServiceImpl::setMuteState(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, StreamMute muteInfo) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->setMuteState(audioReq, streamId, muteInfo);
    }
}

void AudioServiceImpl::getMuteState(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, StreamDirection direction) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->getMuteState(audioReq, streamId, direction);
    }
}

void AudioServiceImpl::write(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, uint8_t *data, uint32_t writeLengthRequested,
        uint32_t offset, int64_t timeStamp, bool isLastBuffer) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->write(audioReq, streamId, data, writeLengthRequested, offset,
            timeStamp, isLastBuffer, voiceCallList);
    }
}

void AudioServiceImpl::read(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, uint32_t readLengthRequested) {
            LOG(ERROR, __FUNCTION__);
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->read(audioReq, streamId, readLengthRequested, voiceCallList);
    }
}

void AudioServiceImpl::startDtmf(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, uint16_t gain, uint16_t duration,
        DtmfTone dtmfTone) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->startDtmf(audioReq, streamId, gain, duration, dtmfTone);
    }
}

void AudioServiceImpl::stopDtmf(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, StreamDirection direction) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->stopDtmf(audioReq, streamId, direction);
    }
}

void AudioServiceImpl::startTone(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, uint16_t gain, uint16_t duration,
        std::vector<uint16_t> toneFrequencies) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->startTone(audioReq, streamId, gain, duration, toneFrequencies);
    }
}

void AudioServiceImpl::stopTone(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId) {
    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->stopTone(audioReq, streamId);
    }
}

void AudioServiceImpl::drain(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId) {

    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->drain(audioReq, streamId);
        return;
    }
}

void AudioServiceImpl::flush(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId) {

    std::shared_ptr<Stream> stream = streamCache_->retrieveStream(streamId);
    if (stream) {
        stream->flush(audioReq, streamId);
        return;
    }
}

void AudioServiceImpl::registerForIndication(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, uint32_t indicationType) {
}

void AudioServiceImpl::deRegisterForIndication(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, uint32_t indicationType) {
}

std::shared_ptr<ClientCache> AudioServiceImpl::getClientCache() {
    return clientCache_;
}

} // end namespace audio
} // end namespace telux
