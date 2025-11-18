/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "Stream.hpp"
#include "Alsa.hpp"

namespace telux {
namespace audio {

Stream::Stream( std::shared_ptr<IAudioBackend> audioBackend,
        std::shared_ptr<ClientCache> clientCache) {

    clientCache_ = clientCache;
    audioBackend_ = audioBackend;
    /* Every stream has a private worker thread that communicates with HAL/PAL,
     * performs actual audio operation in background and finally, sends result
     * of the operation to the application asynchronously. This thread also sends
     * stream events like write ready, drain complete and dtmf detected to the
     * application. */
    streamTaskExecutor_ = std::unique_ptr<telux::common::TaskDispatcher>(
        new telux::common::TaskDispatcher());
    /* Max no. of bufffers after which pipeline full notification is sent. */
    maxPipeLineLen = rand() % 100;
}

Stream::~Stream() {
    LOG(DEBUG, __FUNCTION__);
    streamTaskExecutor_ = nullptr;
}

telux::common::ErrorCode Stream::setupStream(StreamConfiguration config,
        uint32_t streamId, uint32_t& readMinSize, uint32_t& writeMinSize) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;
    StreamVolume volume{};
    ChannelVolume chlVol{};
    streamHandle_.type = config.streamConfig.type;

    streamParams_.config = config;
    streamParams_.streamId = streamId;
    streamParams_.streamEventListener = shared_from_this();
    switch(config.streamConfig.type) {
        case StreamType::VOICE_CALL:
            /* For voice call, canned responses are supported and hence storing the stream config*/
            if(config.streamConfig.deviceTypes.size() != 2){
                LOG(ERROR, __FUNCTION__, " can't create stream, missing sink or source device");
                return telux::common::ErrorCode::INVALID_ARG;
            }

            if((config.streamConfig.deviceTypes[0] == DEVICE_TYPE_BT_SCO_SPEAKER) ||
                (config.streamConfig.deviceTypes[0] == DEVICE_TYPE_BT_SCO_MIC)) {
                isBtStream = true;
            }
            chlVol.channelType = ChannelType::LEFT;
            chlVol.vol = 0.4f;
            volume.volume.push_back(chlVol);
            volume.dir = StreamDirection::RX;
            streamParams_.streamVols = volume;
            streamParams_.muteStatus.enable = false;
            streamParams_.muteStatus.dir = StreamDirection::RX;
            if(config.streamConfig.enableHpcm){
                isHpcmStream = true;
            }
            break;
        case StreamType::PLAY:
            chlVol.channelType = ChannelType::LEFT;
            chlVol.vol = 1.0f;
            volume.volume.push_back(chlVol);
            volume.dir = StreamDirection::RX;
            streamParams_.streamVols = volume;
            if(config.streamConfig.voicePaths.size() > 0){
                isIncallStream = true;
                writeMinSize = MAX_BUFFER_SIZE;
            }

            if(config.streamConfig.deviceTypes[0] == DEVICE_TYPE_BT_SCO_SPEAKER) {
                isBtStream = true;
                writeMinSize = MAX_BUFFER_SIZE;
            }

            streamParams_.muteStatus.enable = false;
            streamParams_.muteStatus.dir = StreamDirection::RX;
            if(config.streamConfig.enableHpcm){
                isHpcmStream = true;
                writeMinSize = MAX_BUFFER_SIZE;
            }
            switch (config.streamConfig.format) {
                case AudioFormat::AMRWB_PLUS:
                case AudioFormat::AMRWB:
                case AudioFormat::AMRNB:
                    streamHandle_.isAMR = true;
                    break;
                default:
                    streamHandle_.isAMR = false;
                    break;
            }
        case StreamType::CAPTURE:
            /*
             * Pre-allocate memory used for read to minimize memory allocation
             * during capture operations.
             */
            try {
                buffer_ = std::make_shared<std::vector<uint8_t>>(MAX_BUFFER_SIZE);
            } catch (const std::exception& e) {
                LOG(ERROR, __FUNCTION__, " can't allocate memory for stream");
                return telux::common::ErrorCode::NO_MEMORY;
            }
            chlVol.channelType = ChannelType::LEFT;
            chlVol.vol = 1.0f;
            volume.volume.push_back(chlVol);
            volume.dir = StreamDirection::TX;
            streamParams_.streamVols = volume;
            if(config.streamConfig.voicePaths.size() > 0){
                isIncallStream = true;
                readMinSize = MAX_BUFFER_SIZE;
            }

            if(config.streamConfig.deviceTypes[0] == DEVICE_TYPE_BT_SCO_MIC) {
                isBtStream = true;
                readMinSize = MAX_BUFFER_SIZE;
            }

            streamParams_.muteStatus.enable = false;
            streamParams_.muteStatus.dir = StreamDirection::TX;
            if(config.streamConfig.enableHpcm){
                isHpcmStream = true;
                readMinSize = MAX_BUFFER_SIZE;
            }
            break;
        case StreamType::LOOPBACK:
            break;
        case StreamType::TONE_GENERATOR:
            streamParams_.config.streamConfig.channelTypeMask = 1;
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid stream type ", static_cast<int>(
                config.streamConfig.type));
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    if(!isIncallStream && !isBtStream && !isHpcmStream) {
        ec = audioBackend_->createStream(streamHandle_, streamParams_, readMinSize, writeMinSize);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            buffer_ = nullptr;
        }
    }

    streamHandle_.privateStreamData = new (std::nothrow) PrivateStreamData();
    if (!streamHandle_.privateStreamData) {
        LOG(ERROR, __FUNCTION__, " can't allocate PrivateStreamData");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    streamHandle_.privateStreamData->streamId = streamId;
    streamHandle_.privateStreamData->streamEventListener = shared_from_this();

    return ec;
}

telux::common::ErrorCode Stream::setupInTranscodeStream(TranscodingFormatInfo inInfo,
        CreatedTranscoderInfo *createdTranscoderInfo) {

    telux::common::ErrorCode ec;

    streamHandle_.type = StreamType::PLAY;

    try {
        buffer_ = std::make_shared<std::vector<uint8_t>>(MAX_BUFFER_SIZE);
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't allocate memory for stream");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    ec = audioBackend_->setupInTranscodeStream(streamHandle_,
            createdTranscoderInfo->inStreamId, inInfo,
            shared_from_this(), createdTranscoderInfo->writeMinSize);

    if (ec != telux::common::ErrorCode::SUCCESS) {
        buffer_ = nullptr;
    }

    return ec;
}

telux::common::ErrorCode Stream::setupOutTranscodeStream(TranscodingFormatInfo outInfo,
        CreatedTranscoderInfo *createdTranscoderInfo) {

    telux::common::ErrorCode ec;

    streamHandle_.type = StreamType::CAPTURE;

    try {
        buffer_ = std::make_shared<std::vector<uint8_t>>(MAX_BUFFER_SIZE);
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't allocate memory for stream");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    ec = audioBackend_->setupOutTranscodeStream(streamHandle_,
            createdTranscoderInfo->outStreamId, outInfo,
            shared_from_this(), createdTranscoderInfo->readMinSize);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        buffer_ = nullptr;
    }

    return ec;
}

telux::common::ErrorCode Stream::cleanupStream(std::vector<int>& voiceCallList) {
    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    streamTaskExecutor_->shutdown();
    if(!isIncallStream && !isBtStream && !isHpcmStream) {
        ec = audioBackend_->deleteStream(streamHandle_);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't close stream");
        }
    }

    if(streamHandle_.type == telux::audio::StreamType::VOICE_CALL) {
        voiceCallList[streamParams_.config.streamConfig.slotId] = 0;
    }

    return ec;
}

/*
 *   -------------------------------------------------------
 *  |  Stream type   | Start/Stop                           |
 *   -------------------------------------------------------
 *  | Voice call     | Y                                    |
 *  | Playback       | N/A                                  |
 *  | Capture        | N/A                                  |
 *  | Loopback       | Y                                    |
 *  | Tone generator | N/A                                  |
 *   -------------------------------------------------------
 */
void Stream::doStart(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    if (streamHandle_.streamStarted) {
        ec = telux::common::ErrorCode::INVALID_ARG;
        LOG(ERROR, __FUNCTION__, " stream already started");
        goto result;
    }

    if (streamHandle_.type == StreamType::LOOPBACK) {
        ec = audioBackend_->start(streamHandle_);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            goto result;
        }
    }

    streamHandle_.streamStarted = true;
    LOG(DEBUG, __FUNCTION__, " stream started, strmid: ", streamId);

result:
    audioMsgDispatcher->sendStartResponse(audioReq, ec, streamId);
}

void Stream::start(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    streamTaskExecutor_->submitTask( [=]{ doStart(audioReq, streamId); });
}

void Stream::doStop(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    if (!streamHandle_.streamStarted) {
        ec = telux::common::ErrorCode::INVALID_ARG;
        LOG(ERROR, __FUNCTION__, " stream already stopped");
        goto result;
    }

    if (streamHandle_.type == StreamType::LOOPBACK) {
        ec = audioBackend_->stop(streamHandle_);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            goto result;
        }
    }

    streamHandle_.streamStarted = false;
    LOG(DEBUG, __FUNCTION__, " stream stopped, strmid: ", streamId);

result:
    audioMsgDispatcher->sendStopResponse(audioReq, ec, streamId);
}

void Stream::stop(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    streamTaskExecutor_->submitTask( [=]{ doStop(audioReq, streamId); });
}

/*
 * Generate a single frequency audio tone. Gain and frequency values are not validated
 * here knowingly to remain flexible.
 *
 * @gain - defines the volume of speaker on which tone will be heard
 * @duration - possible upto 65 seconds
 * @toneFrequencies - 1st value in the vector is used as frequency value, rest ignored
 */
void Stream::doStartTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequencies) {

    telux::common::ErrorCode ec;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    if ((toneFrequencies.size() == 0) || (toneFrequencies.size() > 2)) {
        ec = telux::common::ErrorCode::INVALID_ARG;
        goto result;
    }

    ec = audioBackend_->startTone(streamHandle_, streamParams_.config.streamConfig.sampleRate, gain,
        duration, toneFrequencies);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        goto result;
    }

    LOG(DEBUG, __FUNCTION__, " tone started, strmid: ", streamId);

result:
    audioMsgDispatcher->sendStartToneResponse(audioReq, ec, streamId);
}

void Stream::startTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequencies) {
    streamTaskExecutor_->submitTask( [=]{ doStartTone(audioReq, streamId, gain,
        duration, toneFrequencies); });
}

void Stream::doStopTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    telux::common::ErrorCode ec;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    if (!streamHandle_.streamStarted) {
        ec = telux::common::ErrorCode::INVALID_ARG;
        LOG(ERROR, __FUNCTION__, " stream already stopped");
        goto result;
    }

    ec = audioBackend_->stopTone(streamHandle_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        goto result;
    }

    streamHandle_.streamStarted = false;

    LOG(DEBUG, __FUNCTION__, " tone stopped, strmid: ", streamId);

result:
    audioMsgDispatcher->sendStopToneResponse(audioReq, ec, streamId);
}

void Stream::stopTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {
    streamTaskExecutor_->submitTask( [=]{ doStopTone(audioReq, streamId); });
}

/*
 *   -------------------------------------------------------
 *  |  Stream type   | Get/Set Device                       |
 *   -------------------------------------------------------
 *  | Voice call     | Y                                    |
 *  | Playback       | Y                                    |
 *  | Capture        | Y                                    |
 *  | Loopback       | N/A                                  |
 *  | Tone generator | N/A                                  |
 *   -------------------------------------------------------
 *
 * For playback, if invalid device is given, audio packets AFE routing will not happen.
 * For capture, if invalid device is given, default mic will be used.
 * For voice call, stream must be started to make the set device effective.
 */
void Stream::doSetDevice(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        std::vector<DeviceType> devices) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    switch (streamHandle_.type) {
        case StreamType::VOICE_CALL:
            if(devices.size() != 2){
                LOG(ERROR, __FUNCTION__, " missing sink or source device");
                ec = telux::common::ErrorCode::INVALID_ARG;
            }
        case StreamType::PLAY:
        case StreamType::CAPTURE:
            streamParams_.config.streamConfig.deviceTypes = devices;
            break;
        default:
            ec = telux::common::ErrorCode::INVALID_ARG;
            LOG(ERROR, __FUNCTION__, " can't set device, invalid stream type");
            goto result;
    };

    if (devices.size() == 0) {
        ec = telux::common::ErrorCode::INVALID_ARG;
        LOG(ERROR, __FUNCTION__, " can't set device, no devices given");
        goto result;
    }

    LOG(DEBUG, __FUNCTION__, " stream's device set, strmid: ", streamId);

result:
    audioMsgDispatcher->sendSetDeviceResponse(audioReq, ec, streamId);
}

void Stream::setDevice(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        std::vector<DeviceType> const &devices) {

    streamTaskExecutor_->submitTask( [=]{ doSetDevice(audioReq, streamId, devices); });
}

void Stream::doGetDevice(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;
    std::vector<DeviceType> devices;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    switch (streamHandle_.type) {
        case StreamType::VOICE_CALL:
        case StreamType::PLAY:
        case StreamType::CAPTURE:
            break;
        default:
            ec = telux::common::ErrorCode::INVALID_ARG;
            LOG(ERROR, __FUNCTION__, " can't get device, invalid stream type");
            goto result;
    };


    LOG(DEBUG, __FUNCTION__, " got stream's device, strmid: ", streamId);

result:
    audioMsgDispatcher->sendGetDeviceResponse(audioReq, ec, streamId,
        streamParams_.config.streamConfig.deviceTypes);
}

void Stream::getDevice(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    streamTaskExecutor_->submitTask( [=]{ doGetDevice(audioReq, streamId); });
}

/*
 *   -------------------------------------------------------
 *  |  Stream type   | Get/Set Volume                       |
 *   -------------------------------------------------------
 *  | Voice call     | Y - direction RX, N/A - direction TX |
 *  | Playback       | Y                                    |
 *  | Capture        | Y                                    |
 *  | Loopback       | N/A                                  |
 *  | Tone generator | N/A                                  |
 *   -------------------------------------------------------
 *
 * ADSP/Q6 sets volume in step of 0.2 for voice call stream type. For other stream
 * types any valid value can be given. Given value is rounded to the nearest ceil
 * or floor value. Valid range for volume's value is 0.0 <= volume <= 1.0.
 *
 * For playback and capture stream types, get/set volume can be called any time because
 * volume is set directly with command in kernel. However, for voice call stream type,
 * the stream has to be started first and then set/get volume operation must be performed.
 * This is because we use volume based on ACDB calibration as volume change needs to
 * change other PP parameters.
 */
void Stream::doSetVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        StreamDirection direction, std::vector<ChannelVolume> channelsVolume) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;
    StreamVolume volume;
    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    switch (streamHandle_.type) {
        /* For voice call, canned responses are supported. Hence for setVolume,
           the values are stored in streamParams_.streamVols structure and the function
           returns SUCCESS.*/
        case StreamType::VOICE_CALL:
            if (!streamHandle_.streamStarted) {
                ec = telux::common::ErrorCode::INVALID_STATE;
                LOG(ERROR, __FUNCTION__, " stream not started");
                goto result;
            }
            if ((channelsVolume.at(0).vol < 0.0) ||
                (channelsVolume.at(0).vol > 1.0)) {
                ec = telux::common::ErrorCode::INVALID_ARG;
                LOG(ERROR, __FUNCTION__, " out-of-range volume value");
                goto result;
            }

            volume.volume =  channelsVolume;
            volume.dir = direction;
            streamParams_.streamVols = volume;
            break;
        /* For playback and capture, ALSA respone is supported. Hence, the volume is set at the ALSA
           layer. If playback and capture streams are incall, BT or HPCM use case streams, in that
           case values are stored in streamParams_.streamVols structure and the function
           returns SUCCESS.*/
        case StreamType::PLAY:
        case StreamType::CAPTURE:
            if ((channelsVolume.size() > 1) &&
                (channelsVolume.at(0).vol != channelsVolume.at(1).vol)) {
                ec = telux::common::ErrorCode::INVALID_ARG;
                LOG(ERROR, __FUNCTION__, " mismatched left & right values");
                goto result;
            }
            if ((channelsVolume.at(0).vol < 0.0) ||
                (channelsVolume.at(0).vol > 1.0)) {
                ec = telux::common::ErrorCode::INVALID_ARG;
                LOG(ERROR, __FUNCTION__, " out-of-range volume value");
                goto result;
            }

            volume.volume =  channelsVolume;
            volume.dir = direction;
            streamParams_.streamVols = volume;

            if(!isIncallStream && !isBtStream && !isHpcmStream) {
                ec = audioBackend_->setVolume(streamHandle_, direction, channelsVolume);
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    goto result;
                }
            }
            break;
        default:
            ec = telux::common::ErrorCode::INVALID_ARG;
            LOG(ERROR, __FUNCTION__, " can't set volume, invalid stream type");
            goto result;
    };

    LOG(DEBUG, __FUNCTION__, " volume set, strmid: ", streamId);

result:
    audioMsgDispatcher->sendSetVolumeResponse(audioReq, ec, streamId);
}

void Stream::setVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        StreamDirection direction, std::vector<ChannelVolume> channelsVolume) {

    streamTaskExecutor_->submitTask( [=]{ doSetVolume(audioReq, streamId,
            direction, channelsVolume); });
}

void Stream::doGetVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        StreamDirection direction) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;
    std::vector<ChannelVolume> channelsVolume{};

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    switch (streamHandle_.type) {
        /* For voice call, canned responses are supported. Hence for getVolume,
           the values are stored in streamParams_.streamVols structure are returned. */
        case StreamType::VOICE_CALL:
            if (!streamHandle_.streamStarted) {
                ec = telux::common::ErrorCode::INVALID_STATE;
                LOG(ERROR, __FUNCTION__, " stream not started");
                goto result;
            }

            channelsVolume = streamParams_.streamVols.volume;
            break;
        /* For playback and capture, ALSA respone is supported. Hence, the volume set at the ALSA
           layer is returned. If playback and capture streams are incall, BT or HPCM use case
           streams, in that case values stored in streamParams_.streamVols structure is returned. */
        case StreamType::PLAY:
        case StreamType::CAPTURE:
            if(!isIncallStream && !isBtStream && !isHpcmStream) {
                ec = audioBackend_->getVolume(streamHandle_,
                    streamParams_.config.streamConfig.channelTypeMask, channelsVolume);
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    goto result;
                }
            } else {
                channelsVolume = streamParams_.streamVols.volume;
            }
            break;
        default:
            ec = telux::common::ErrorCode::INVALID_ARG;
            LOG(ERROR, __FUNCTION__, " can't get volume, invalid stream type");
            goto result;
    };

    LOG(DEBUG, __FUNCTION__, " volume retrieved, strmid: ", streamId);

result:
    audioMsgDispatcher->sendGetVolumeResponse(audioReq, ec, streamId, direction,
        channelsVolume);
}

void Stream::getVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        StreamDirection direction) {

    streamTaskExecutor_->submitTask( [=]{ doGetVolume(audioReq, streamId,
        direction); });
}

/*
 *   -------------------------------------------------------
 *  |  Stream type   | Get/Set Mute state                   |
 *   -------------------------------------------------------
 *  | Voice call     | Y - direction RX, N/A - direction TX |
 *  | Playback       | Y                                    |
 *  | Capture        | Y                                    |
 *  | Loopback       | N/A                                  |
 *  | Tone generator | N/A                                  |
 *   -------------------------------------------------------
 *
 *  Mute/Unmute given stream based on the value of 'muteInfo.enable'.
 *
 *  For voice call stream, stream has to be started before get/set mute. It is
 *  because mute information is fetched from lower layers, whereas for playback
 *  and capture, cached info is returned.
 */
void Stream::doSetMuteState(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        StreamMute muteInfo) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    switch (streamHandle_.type) {
        case StreamType::VOICE_CALL:
            if (!streamHandle_.streamStarted) {
                ec = telux::common::ErrorCode::INVALID_STATE;
                LOG(ERROR, __FUNCTION__, " stream not started");
                goto result;
            }
            break;
        case StreamType::PLAY:
        case StreamType::CAPTURE:
            if(!isIncallStream && !isBtStream && !isHpcmStream) {
                ec = audioBackend_->setMuteState(streamHandle_, muteInfo,
                    streamParams_.streamVols.volume, streamParams_.muteStatus.enable);
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    goto result;
                }
            }
            break;
        default:
            ec = telux::common::ErrorCode::INVALID_ARG;
            LOG(ERROR, __FUNCTION__, " can't mute/unmute audio, invalid stream type");
            goto result;
    };

    streamParams_.muteStatus = muteInfo;
    LOG(DEBUG, __FUNCTION__, " mute state set, strmid: ", streamId);

result:
    audioMsgDispatcher->sendSetMuteStateResponse(audioReq, ec, streamId);
}

void Stream::setMuteState(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, StreamMute muteInfo) {

    streamTaskExecutor_->submitTask( [=]{ doSetMuteState(audioReq, streamId, muteInfo); });
}

void Stream::doGetMuteState(std::shared_ptr<AudioRequest> audioReq,
        uint32_t streamId, StreamDirection direction) {

    StreamMute muteInfo;
    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    switch (streamHandle_.type) {
        case StreamType::VOICE_CALL:
            if (!streamHandle_.streamStarted) {
                ec = telux::common::ErrorCode::INVALID_STATE;
                LOG(ERROR, __FUNCTION__, " stream not started");
                goto result;
            }
        case StreamType::PLAY:
        case StreamType::CAPTURE:
            muteInfo = streamParams_.muteStatus;
            break;
        default:
            ec = telux::common::ErrorCode::INVALID_ARG;
            LOG(ERROR, __FUNCTION__, " can't get mute state, invalid stream type");
            goto result;
    };

    LOG(DEBUG, __FUNCTION__, " mute state retrieved, strmid: ", streamId);

result:
    audioMsgDispatcher->sendGetMuteStateResponse(audioReq, ec, streamId, muteInfo);
}

void Stream::getMuteState(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        StreamDirection direction) {

    streamTaskExecutor_->submitTask( [=]{ doGetMuteState(audioReq,
        streamId, direction); });
}

/*
 *   ---------------------------------------
 *  |  Stream type   | DTMF generate/detect |
 *   ---------------------------------------
 *  | Voice call     | Y - direction RX/TX  |
 *  | Playback       | N/A                  |
 *  | Capture        | N/A                  |
 *  | Loopback       | N/A                  |
 *  | Tone generator | N/A                  |
 *   ---------------------------------------
 *
 * (a) On a voice call, playDtmfTone() generates DTMF tone on local speaker. This
 *     same signal is also sent to far-end device connected to cellular network.
 * (b) On a voice call, registerListener() registers with HAL/PAL for DTMF signal
 *     detection. When it detects DTMF, an event is sent to the application.
 * (c) Telephony also has an API to generate DTMF signal. When invoked, it sends
 *     character to cellular network which in turn actually generates corresponding
 *     DTMF tone.
 *
 * To generate a DTMF tone corresponding to a given key, a particular pair of the
 * low and high frequency is used, as shown in the table below.
 *
 *   -----------------------------------------------
 *  |                   |    High frequencies       |
 *  |                   | 1209 | 1336 | 1477 | 1633 |
 *   -----------------------------------------------
 *  | Low          697  |  1   |  2   |  3   |  A   |
 *  | frequencies  770  |  4   |  5   |  6   |  B   |
 *  |              852  |  7   |  8   |  9   |  C   |
 *  |              941  |  *   |  0   |  #   |  D   |
 *   -----------------------------------------------
 */
void Stream::doStartDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        uint16_t gain, uint16_t duration, DtmfTone dtmfTone) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    if (dtmfTone.direction != StreamDirection::RX) {
        ec = telux::common::ErrorCode::INVALID_ARG;
        LOG(ERROR, __FUNCTION__, " invalid stream direction ",
            static_cast<int>(dtmfTone.direction));
        goto result;
    }

    switch (dtmfTone.highFreq) {
        case DtmfHighFreq::FREQ_1209:
        case DtmfHighFreq::FREQ_1336:
        case DtmfHighFreq::FREQ_1477:
        case DtmfHighFreq::FREQ_1633:
            break;
        default:
            ec = telux::common::ErrorCode::INVALID_ARG;
            LOG(ERROR, __FUNCTION__, " invalid high frquency ",
                static_cast<int>(dtmfTone.highFreq));
            goto result;
    }

    switch (dtmfTone.lowFreq) {
        case DtmfLowFreq::FREQ_697:
        case DtmfLowFreq::FREQ_770:
        case DtmfLowFreq::FREQ_852:
        case DtmfLowFreq::FREQ_941:
            break;
        default:
            ec = telux::common::ErrorCode::INVALID_ARG;
            LOG(ERROR, __FUNCTION__, " invalid low frquency ",
                static_cast<int>(dtmfTone.lowFreq));
            goto result;
    }

    LOG(DEBUG, __FUNCTION__, " dtmf started, strmid: ", streamId);

result:
    audioMsgDispatcher->sendStartDtmfResponse(audioReq, ec, streamId);
}

void Stream::startDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        uint16_t gain, uint16_t duration, DtmfTone dtmfTone) {

    streamTaskExecutor_->submitTask( [=]{ doStartDtmf(audioReq, streamId, gain,
        duration, dtmfTone); });
}

void Stream::doStopDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        StreamDirection direction) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }


    LOG(DEBUG, __FUNCTION__, " dtmf stopped, strmid: ", streamId);

    audioMsgDispatcher->sendStopDtmfResponse(audioReq, ec, streamId);
}

void Stream::stopDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        StreamDirection direction) {

    streamTaskExecutor_->submitTask( [=]{ doStopDtmf(audioReq, streamId, direction); });
}

void Stream::doRead(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        uint32_t readLengthRequested, std::vector<int> voiceCallList) {

    int64_t actualReadLength = 0;
    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    if(!isIncallStream && !isBtStream && !isHpcmStream) {
        ec = audioBackend_->read(streamHandle_, buffer_, readLengthRequested, actualReadLength);
    } else {
        if(!voiceCallList[streamParams_.config.streamConfig.slotId] && isIncallStream) {
            ec = telux::common::ErrorCode::SYSTEM_ERR;
        } else {
            actualReadLength = readLengthRequested;
        }
    }

    /* Don't log on data path */
    LOG(DEBUG, __FUNCTION__,
          " stream data read, strmid: ", streamId, " length ", actualReadLength);

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    audioMsgDispatcher->sendReadResponse(audioReq, ec, streamId, buffer_,
        actualReadLength, 0, 0, isIncallStream, isHpcmStream);
}

void Stream::read(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        uint32_t readLengthRequested, std::vector<int> &voiceCallList) {

    streamTaskExecutor_->submitTask( [=]{ doRead(audioReq, streamId, readLengthRequested,
        voiceCallList); });
}

/*
 * PCM format write flow:
 *
 * In a nutshell just keep sending buffers back-to-back until all
 * of them are not played. Fill the next buffer while the previous
 * one is getting played. Write complete in flow below refers to the
 * async qmi response received as response to the previous async qmi
 * write request.
 *
 * 1. Create a playback audio stream.
 * 2. Get minimum and maximum buffer size for this stream.
 * 3. Decide actual size of buffer to use. If minimum size is 0,
 *    use maximum otherwise use minimum size.
 * 4. Allocate two buffers to operate in ping-pong fashion.
 * 5. Get raw buffer and copy data to be played into 1st buffer.
 * 6. Call write() method to send this buffer to HAL/PAL.
 * 7. Fill 2nd buffer and call write() method to send this buffer
 *    to HAL/PAL.
 * 8. Write complete response callback will be invoked as a response to write
 *    complete for 1st buffer. In this callback fill the 1st buffer again
 *    and send it for playing by calling write().
 * 9. When write complete happens for 2nd buffer, fill it again and send
 *    for playback. Step 5 to 9 are repeated until all buffers are played.
 * 10. Delete the audio playback stream.
 *
 * AMR* format write flow:
 *
 * All steps are same as for PCM playback except when application should call write.
 * a. If the "number of bytes actually written == 0" OR "number of bytes actually written < number
 * of bytes to write" application should wait for write ready indication. Once received it should
 * send next buffer to play.
 * b. If the number of to write and number of bytes written are exactly same, application should
 * just send next buffer to write and should not wait for write ready indication.
 * c. If the write() returns and error, it should be treated as error and handled as per
 * application's business logic.
 */
void Stream::doWrite(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        uint32_t writeLengthRequested, uint32_t offset, int64_t timeStamp,
        bool isLastBuffer, uint8_t *data, std::vector<int> voiceCallList) {

    int64_t actualLengthWritten = 0;
    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    if(streamHandle_.isAMR) {
        pipelineLength += 1;
        sendPipelineFull += 1;
        if(sendPipelineFull%maxPipeLineLen == 0 && (!isLastBuffer) && (pipelineLength>0)){
            auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
            if (!audioMsgDispatcher) {
                return;
            }

            audioMsgDispatcher->sendWriteResponse(audioReq, ec, streamId, actualLengthWritten,
                isIncallStream, isHpcmStream);

            pipelineLength -= 1;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            onWriteReadyEvent(streamId);

            return;
        }
    }

    if(!isIncallStream && !isBtStream && !isHpcmStream) {
        ec = audioBackend_->write(streamHandle_, data, writeLengthRequested, offset,
            timeStamp, isLastBuffer, actualLengthWritten);
    } else {
        if(!voiceCallList[streamParams_.config.streamConfig.slotId] && isIncallStream) {
            ec = telux::common::ErrorCode::SYSTEM_ERR;
        } else {
            actualLengthWritten = writeLengthRequested;
        }
    }

    /* Don't log on data path */
    /* LOG(DEBUG, __FUNCTION__,
        " data written, strmid: ", streamId, " length ", actualLengthWritten); */

    if(streamHandle_.isAMR && (actualLengthWritten == 0)) {
        onWriteReadyEvent(streamId);
    }

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    audioMsgDispatcher->sendWriteResponse(audioReq, ec, streamId, actualLengthWritten,
        isIncallStream, isHpcmStream);

    pipelineLength -= 1;

    if(isLastBuffer){
        sendPipelineFull = 0;
        pipelineLength = 0;
    }
}

void Stream::write(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        uint8_t *data, uint32_t writeLengthRequested, uint32_t offset,
        int64_t timeStamp, bool isLastBuffer, std::vector<int> &voiceCallList) {

    streamTaskExecutor_->submitTask( [=]{ doWrite(audioReq, streamId, writeLengthRequested,
        offset, timeStamp, isLastBuffer, data, voiceCallList); });
}

/*
 * Finish playing current buffer and then discard all the buffers queued for playing.
 */
void Stream::doDrain(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    telux::common::ErrorCode ec;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    ec = audioBackend_->drain(streamHandle_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        goto result;
    }

    LOG(DEBUG, __FUNCTION__, " stream drained, strmid: ", streamId);

result:
    audioMsgDispatcher->sendDrainResponse(audioReq, ec, streamId);
}

void Stream::drain(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    streamTaskExecutor_->submitTask( [=]{ doDrain(audioReq, streamId); });
}

/*
 * Discard all the buffers currently queued for playing unconditionally.
 */
void Stream::doFlush(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    telux::common::ErrorCode ec;

    auto audioMsgDispatcher = audioReq->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    ec = audioBackend_->flush(streamHandle_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        goto result;
    }

    LOG(DEBUG, __FUNCTION__, " stream flushed, strmid: ", streamId);

result:
    audioMsgDispatcher->sendFlushResponse(audioReq, ec, streamId);
}

void Stream::flush(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) {

    streamTaskExecutor_->submitTask( [=]{ doFlush(audioReq, streamId); });
}


/*
 * ADSP/Q6 is about to finish playing audio samples. Inform application about
 * this state.
 */
void Stream::doOnDrainDoneEvent(uint32_t streamId) {

    std::shared_ptr<AudioClient> audioClient;

    audioClient = clientCache_->getAudioClientByStreamId(streamId);

    auto audioMsgDispatcher = audioClient->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    audioMsgDispatcher->sendDrainDoneEvent(audioClient->getClientId(), streamId);
}

void Stream::onDrainDoneEvent(uint32_t streamId) {
    streamTaskExecutor_->submitTask( [=]{ doOnDrainDoneEvent(streamId); });
}


/*
 * ADSP/Q6 just finished playing current buffer. It is not ready to accept the
 * next audio samples buffer to play. Inform application about this.
 */
void Stream::doOnWriteReadyEvent(uint32_t streamId) {
    LOG(DEBUG, __FUNCTION__);

    std::shared_ptr<AudioClient> audioClient;

    audioClient = clientCache_->getAudioClientByStreamId(streamId);

    auto audioMsgDispatcher = audioClient->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    audioMsgDispatcher->sendWriteReadyEvent(audioClient->getClientId(), streamId);
}

void Stream::onWriteReadyEvent(uint32_t streamId) {
    streamTaskExecutor_->submitTask( [=]{ doOnWriteReadyEvent(streamId); });
}

/*
 * DTMF signal has been detected on a given audio stream. Send this signal
 * to the applcation.
 */
void Stream::doOnDTMFDetectedEvent(uint32_t streamId, uint32_t lowFreq,
        uint32_t highFreq, StreamDirection streamDirection) {

    std::shared_ptr<AudioClient> audioClient;

    audioClient = clientCache_->getAudioClientByStreamId(streamId);

    auto audioMsgDispatcher = audioClient->getAudioMsgDispatcher().lock();
    if (!audioMsgDispatcher) {
        return;
    }

    audioMsgDispatcher->sendDTMFDetectedEvent(audioClient->getClientId(), streamId, lowFreq,
        highFreq, streamDirection);
}

void Stream::onDTMFDetectedEvent(uint32_t streamId, uint32_t lowFreq,
        uint32_t highFreq, StreamDirection streamDirection) {

    streamTaskExecutor_->submitTask( [=]{ doOnDTMFDetectedEvent(streamId,
        lowFreq, highFreq, streamDirection); });
}

}  // end of namespace audio
}  // end of namespace telux
