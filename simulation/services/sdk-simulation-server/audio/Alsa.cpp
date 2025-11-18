/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "Alsa.hpp"
extern "C" {
#include <alsa/asoundlib.h>
#include <alsa/control.h>
}
#include "libs/common/SimulationConfigParser.hpp"

#define DEFAULT_DEVICE "default"

/* #define DDEBUG 1 */

namespace telux {
namespace audio {

/*
 * Audio stream types supported.
 */
const std::vector<StreamType> Alsa::SUPPORTED_STREAM_TYPES = {
        StreamType::VOICE_CALL, StreamType::PLAY, StreamType::CAPTURE,
        StreamType::LOOPBACK, StreamType::TONE_GENERATOR};

/*
 * Telsdk audio device to PAL audio device mapping. When the audio server is run, it will
 * check if mappings are defined in tel.conf or not. If not defined, this default mapping
 * will be used.
 *
 *   -------------------------------------------------------------------------------------
 *  |        Telsdk device       | Device direction |      Mapped PAL device              |
 *   -------------------------------------------------------------------------------------
 *  | DEVICE_TYPE_SPEAKER        |       RX         | PAL_DEVICE_OUT_SPEAKER              |
 *  | DEVICE_TYPE_SPEAKER_2      |       RX         | PAL_DEVICE_OUT_HANDSET              |
 *  | DEVICE_TYPE_SPEAKER_3      |       RX         | PAL_DEVICE_OUT_WIRED_HEADSET        |
 *  | DEVICE_TYPE_BT_SCO_SPEAKER |       RX         | PAL_DEVICE_OUT_BLUETOOTH_SCO        |
 *  | DEVICE_TYPE_PROXY_SPEAKER  |       RX         | PAL_DEVICE_OUT_PROXY                |
 *  | DEVICE_TYPE_MIC            |       TX         | PAL_DEVICE_IN_SPEAKER_MIC           |
 *  | DEVICE_TYPE_MIC_2          |       TX         | PAL_DEVICE_IN_HANDSET_MIC           |
 *  | DEVICE_TYPE_MIC_3          |       TX         | PAL_DEVICE_IN_WIRED_HEADSET         |
 *  | DEVICE_TYPE_BT_SCO_MIC     |       TX         | PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET |
 *  | DEVICE_TYPE_PROXY_MIC      |       TX         | PAL_DEVICE_IN_PROXY                 |
 *   -------------------------------------------------------------------------------------
 */
const DeviceMappingTable Alsa::DEFAULT_DEVS_TABLE = {10,
        {DeviceDirection::RX, DeviceDirection::RX, DeviceDirection::RX, DeviceDirection::RX,
         DeviceDirection::RX, DeviceDirection::TX, DeviceDirection::TX, DeviceDirection::TX,
         DeviceDirection::TX, DeviceDirection::TX},
        {DeviceType::DEVICE_TYPE_SPEAKER, DeviceType::DEVICE_TYPE_SPEAKER_2,
         DeviceType::DEVICE_TYPE_SPEAKER_3, DeviceType::DEVICE_TYPE_BT_SCO_SPEAKER,
         DeviceType::DEVICE_TYPE_PROXY_SPEAKER, DeviceType::DEVICE_TYPE_MIC,
         DeviceType::DEVICE_TYPE_MIC_2, DeviceType::DEVICE_TYPE_MIC_3,
         DeviceType::DEVICE_TYPE_BT_SCO_MIC, DeviceType::DEVICE_TYPE_PROXY_MIC}
};

#ifdef __cplusplus
extern "C" {
#endif

Alsa::Alsa() {
    LOG(DEBUG, __FUNCTION__);
     try{
        config_ = std::make_shared<SimulationConfigParser>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create SimulationConfigParser");
    }
    runLoopback_ = false;
    runTone_ = false;
    pipelineLen = rand() % 10;
    sendWriteReady = 0;
}

Alsa::~Alsa() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::ErrorCode Alsa::init(std::shared_ptr<ISSREventListener> ssrEventListener) {
    LOG(DEBUG, __FUNCTION__);
    int ret;
    telux::common::Status status;

    ret = loadUserDeviceMapping();
    if (ret < 0) {
        /* Use default device mapping, if the user doesn't override it through
         * tel.conf or an error occurs while parsing tel.conf for mappings. */
         finalDevicesTable_ = Alsa::DEFAULT_DEVS_TABLE;
         pcmDevice_ = DEFAULT_DEVICE;
         sndCardCtlDevice_ = DEFAULT_DEVICE;
         LOG(INFO, __FUNCTION__, " default device mapping loaded");
    }

    /* Register for event-injection notification for audio filter. */
    auto &serverEventManager = ServerEventManager::getInstance();
    status = serverEventManager.registerListener(shared_from_this(), AUDIO_FILTER);
    if(status != telux::common::Status::SUCCESS){
        LOG(ERROR, __FUNCTION__, "Failed to register for event: ", AUDIO_FILTER);
    }

    /* Register for SSR event notification */
    ssrListenerMap_.push_back(ssrEventListener);

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::deinit() {

    telux::common::Status status;

    if(privateSSRData_) {
        delete privateSSRData_;
    }

    auto &serverEventManager = ServerEventManager::getInstance();
    status = serverEventManager.deregisterListener(shared_from_this(), AUDIO_FILTER);
    if(status != telux::common::Status::SUCCESS){
        LOG(ERROR, __FUNCTION__, "Failed to deregister for event: ", AUDIO_FILTER);
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::getSupportedDevices(std::vector<DeviceType>& devices,
        std::vector<DeviceDirection>& devicesDirection) {

    for (uint32_t x = 0; x < finalDevicesTable_.numDevices; x++) {
        devices.push_back(finalDevicesTable_.deviceType[x]);
        devicesDirection.push_back(finalDevicesTable_.deviceDir[x]);
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::getSupportedStreamTypes(std::vector<StreamType>& streamTypes) {

    streamTypes = SUPPORTED_STREAM_TYPES;
    return telux::common::ErrorCode::SUCCESS;
}


telux::common::ErrorCode Alsa::mapStreamType(StreamType streamType, snd_pcm_stream_t& stream) {

    switch(streamType) {
        case StreamType::TONE_GENERATOR:
        case StreamType::PLAY:
            stream = SND_PCM_STREAM_PLAYBACK;
            break;
        case StreamType::CAPTURE:
            stream = SND_PCM_STREAM_CAPTURE;
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid stream type ", static_cast<int>(streamType));
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::mapStreamChannelMask( uint32_t channelTypeMask, int& channels) {

    switch(channelTypeMask) {
        case ChannelType::LEFT:
            channels = 1;
            break;
        case ChannelType::RIGHT:
            channels = 1;
            break;
        case (ChannelType::LEFT | ChannelType::RIGHT):
                channels = 2;
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid channel type ", channelTypeMask);
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::createStream(StreamHandle& streamHandle,
        StreamParams streamParams, uint32_t& readBufferMinSize,
        uint32_t& writeBufferMinSize) {

    telux::common::ErrorCode ec;
    /* Currently, only PCM format is supported. Therefore, setting format as PCM. */
    snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;;
    snd_pcm_hw_params_t *params;
    snd_pcm_stream_t stream;
    int ret;
    ChannelVolume vol;
    std::vector<ChannelVolume> channelsVolume{};

    if(StreamType::VOICE_CALL == streamHandle.type) {
        /*
         * Enable DTMF detection. Currently only RX path is supported.
         */
        dtmfIndicationListenerMap_[streamParams.streamId] = streamParams.streamEventListener;
        LOG(DEBUG, __FUNCTION__,"Registered listener ");
        return telux::common::ErrorCode::SUCCESS;
    }

    ec = mapStreamChannelMask(streamParams.config.streamConfig.channelTypeMask,
            streamHandle.channels);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        return ec;
    }

    if(StreamType::LOOPBACK == streamHandle.type){
        ret = snd_pcm_open (&streamHandle.loopbackPlayHandle, pcmDevice_.c_str(), SND_PCM_STREAM_PLAYBACK,
            0);
        if (ret < 0) {
            LOG(ERROR, __FUNCTION__, "Can't open PCM device: ", pcmDevice_.c_str());
            return telux::common::ErrorCode::SYSTEM_ERR;
        }

        ret = snd_pcm_set_params(streamHandle.loopbackPlayHandle, format,
            SND_PCM_ACCESS_RW_INTERLEAVED, streamHandle.channels,
            streamParams.config.streamConfig.sampleRate, 1, 500000);
        if (ret < 0) {
            LOG(ERROR, __FUNCTION__,"Loopback playback open error:");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }

        ret = snd_pcm_open (&streamHandle.loopbackCaptureHandle, pcmDevice_.c_str(), SND_PCM_STREAM_CAPTURE,
            0);
        if (ret < 0) {
            LOG(ERROR, __FUNCTION__, "Can't open PCM device: ", pcmDevice_.c_str());
            return telux::common::ErrorCode::SYSTEM_ERR;
        }

        ret = snd_pcm_set_params(streamHandle.loopbackCaptureHandle, format,
            SND_PCM_ACCESS_RW_INTERLEAVED, streamHandle.channels,
            streamParams.config.streamConfig.sampleRate, 1, 500000);
        if (ret < 0) {
            LOG(ERROR, __FUNCTION__,"Loopback capture open error:");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }

        return telux::common::ErrorCode::SUCCESS;
    }

    ec = mapStreamType(streamHandle.type, stream);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        return ec;
    }

    ret = snd_pcm_open(&streamHandle.pcmHandle, pcmDevice_.c_str(), stream, 0);
    if (ret < 0) {
        LOG(ERROR, __FUNCTION__, "Can't open PCM device: ", pcmDevice_.c_str());
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    /* Allocate an invalid snd_pcm_hw_params_t using standard alloca */
    snd_pcm_hw_params_alloca(&params);

    /* Fill params with a full configuration space for PCM */
    snd_pcm_hw_params_any(streamHandle.pcmHandle, params);

    /* Set snd_pcm_readi/snd_pcm_writei access. A pulse code modulated (PCM) signal consists of a
     * stream of samples. If there is more than one channel, the channels will be interleaved.
     * In the case of stereo data: left sample, right sample, left, right.
     */
    ret = snd_pcm_hw_params_set_access(streamHandle.pcmHandle, params,
        SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) {
        LOG(ERROR, __FUNCTION__,"Can't set interleaved mode.");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    if(StreamType::TONE_GENERATOR == streamHandle.type) {
        ret = snd_pcm_hw_params_set_format(streamHandle.pcmHandle, params, SND_PCM_FORMAT_FLOAT);
        if (ret < 0){
            LOG(ERROR, __FUNCTION__,"Can't set format.");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
    } else {
        /* Restrict a configuration space to contain only one format. */
        ret = snd_pcm_hw_params_set_format(streamHandle.pcmHandle, params, format);
        if (ret < 0){
            LOG(ERROR, __FUNCTION__,"Can't set format.");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
    }

    /* Restrict a configuration space to contain only given no channels count.*/
    ret = snd_pcm_hw_params_set_channels(streamHandle.pcmHandle, params, streamHandle.channels);
    if (ret < 0) {
        LOG(ERROR, __FUNCTION__,"Can't set channels number.");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    /*Restrict a configuration space to have rate nearest to a target.*/
    ret = snd_pcm_hw_params_set_rate_near(streamHandle.pcmHandle, params,
        &streamParams.config.streamConfig.sampleRate, 0);
    if (ret < 0){
        LOG(ERROR, __FUNCTION__,"Can't set rate.");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    /* Install one PCM hardware configuration chosen from a configuration space and
     * snd_pcm_prepare it.
     */
    ret = snd_pcm_hw_params(streamHandle.pcmHandle, params);
    if (ret  < 0){
        LOG(ERROR, __FUNCTION__,"Can't set harware parameters. ");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    /* Allocate buffer to hold single period. Extract period size from a configuration space.
       Sets the no of frames in the period */
    snd_pcm_hw_params_get_period_size(params, &streamHandle.frames, 0);

    /* Set volume for stream as 1 as default for the stream.*/
    vol.channelType = ChannelType::LEFT;
    vol.vol = 1;
    channelsVolume.push_back(vol);
    vol.channelType = ChannelType::RIGHT;
    vol.vol = 1;
    channelsVolume.push_back(vol);


    /* Set buffer size for a particular stream.*/
    switch (streamHandle.type)
    {
    case StreamType::PLAY:
        writeBufferMinSize = streamHandle.frames * streamHandle.channels * 2;

        ec = setVolume(streamHandle, StreamDirection::RX , channelsVolume);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
        break;
    case StreamType::CAPTURE:
        readBufferMinSize = streamHandle.frames * streamHandle.channels * 2;
        ec = setVolume(streamHandle, StreamDirection::RX , channelsVolume);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
    default:
        /* Default values of writeBufferMinSize and readBufferMinSize set to 0.*/
        break;
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::deleteStream(StreamHandle& streamHandle) {

    int ret;

    switch (streamHandle.type) {
    case StreamType::VOICE_CALL:
        return telux::common::ErrorCode::SUCCESS;
        break;
    case StreamType::TONE_GENERATOR:
        for(std::thread &th: toneThread_) {
            if(th.joinable()){
                th.join();
            }
        }
    case StreamType::PLAY:
        if(streamHandle.inTranscodeStreamId == inTranscodeStreamId_) {
            return telux::common::ErrorCode::SUCCESS;
        }

        ret = snd_pcm_drop(streamHandle.pcmHandle);
        if (ret  < 0){
            LOG(ERROR, __FUNCTION__,"Can't drain PCM. ");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }

        ret = snd_pcm_close(streamHandle.pcmHandle);
        if (ret  < 0){
            LOG(ERROR, __FUNCTION__,"Can't close PCM stream. ");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
        break;
    case StreamType::CAPTURE:
        if(streamHandle.outTranscodeStreamId == outTranscodeStreamId_) {
            return telux::common::ErrorCode::SUCCESS;
        }

        ret = snd_pcm_close(streamHandle.pcmHandle);
        if (ret  < 0){
            LOG(ERROR, __FUNCTION__,"Can't close PCM stream. ");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
        break;
    case StreamType::LOOPBACK:
        for(std::thread &th: loopThread_) {
            if(th.joinable()){
                th.join();
            }
        }
        //Delete loopback play stream
        ret = snd_pcm_drain(streamHandle.loopbackPlayHandle);
        if (ret  < 0){
            LOG(ERROR, __FUNCTION__,"Can't drain PCM. ");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
        ret = snd_pcm_close(streamHandle.loopbackPlayHandle);
        if (ret  < 0){
            LOG(ERROR, __FUNCTION__,"Can't close PCM stream. ");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
        //Delete loopback capture stream
        ret = snd_pcm_close(streamHandle.loopbackCaptureHandle);
        if (ret  < 0){
            LOG(ERROR, __FUNCTION__,"Can't close PCM stream. ");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
        break;

    default:
        LOG(ERROR, __FUNCTION__,"Invalid stream type: ", static_cast<int>(streamHandle.type));
        return telux::common::ErrorCode::SYSTEM_ERR;
        break;
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::startLoopback(snd_pcm_t *captureHandle, snd_pcm_t *playHandle,
    int channels) {
    int64_t actualReadLength, actualLengthWritten;
    unsigned char buf[MAX_BUFFER_SIZE];
    int buf_frames = MAX_BUFFER_SIZE / (channels * 2);

    while(runLoopback_) {
        actualReadLength = snd_pcm_readi(captureHandle, buf, buf_frames);
        if (actualReadLength == -EPIPE || actualReadLength == -ESTRPIPE) {
            LOG(ERROR, __FUNCTION__,"read error: ", snd_strerror(actualReadLength));
            return telux::common::ErrorCode::SYSTEM_ERR;
        }

        actualLengthWritten = snd_pcm_writei (playHandle, buf, buf_frames);
        if (actualLengthWritten == -EPIPE) {
            snd_pcm_prepare(playHandle);
            return telux::common::ErrorCode::SUCCESS;
        }
        if(actualLengthWritten == -ESTRPIPE) {
            LOG(ERROR, __FUNCTION__,"write error: ", snd_strerror(actualLengthWritten));
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::start(StreamHandle streamHandle) {
    /*Used to start loopback.*/
    runLoopback_ = true;

    std::thread loopThread(&Alsa::startLoopback, this, streamHandle.loopbackCaptureHandle,
        streamHandle.loopbackPlayHandle, streamHandle.channels);

    loopThread_.emplace_back(std::move(loopThread));

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::stop(StreamHandle streamHandle) {
    /*Used to start loopback.*/
    runLoopback_ = false;
    for(std::thread &th: loopThread_) {
        if(th.joinable()){
            th.join();
        }
    }
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * By default, secondary MI2S is used for handset, teritiary MI2S is
 * used for headset. Both are used as mono.
 */
telux::common::ErrorCode Alsa::setDevice(StreamHandle streamHandle,
        std::vector<DeviceType>& deviceTypes) {
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::getDevice(StreamHandle streamHandle,
        std::vector<DeviceType>& deviceTypes) {

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::setVolume(StreamHandle streamHandle,
        StreamDirection direction, std::vector<ChannelVolume> channelsVolume) {

    int err;
    long vol;
    long min, max;
    uint32_t numChannels = 0;
    snd_mixer_t *h_mixer;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem ;

    numChannels = channelsVolume.size();

    if ((err = snd_mixer_open(&h_mixer, 0)) < 0) {
        LOG(ERROR, __FUNCTION__,"Mixer open error: ", err);
    }

    if ((err = snd_mixer_attach(h_mixer, sndCardCtlDevice_.c_str())) < 0) {
        LOG(ERROR, __FUNCTION__,"Mixer attach error: ", err);
    }

    if ((err = snd_mixer_selem_register(h_mixer, NULL, NULL)) < 0) {
        LOG(ERROR, __FUNCTION__,"Mixer simple element register error:", err);
    }

    if ((err = snd_mixer_load(h_mixer)) < 0) {
        LOG(ERROR, __FUNCTION__,"Mixer load error:", err);
    }

    switch(streamHandle.type) {
        case StreamType::PLAY:
            snd_mixer_selem_id_alloca(&sid);
            snd_mixer_selem_id_set_index(sid, 0);
            snd_mixer_selem_id_set_name(sid, "Master");

            if ((elem = snd_mixer_find_selem(h_mixer, sid)) == NULL) {
                LOG(ERROR, __FUNCTION__,"Cannot find simple element");
            }

            for (uint32_t x = 0; x < numChannels; x++) {
                snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
                vol = channelsVolume.at(x).vol * max;
                switch(channelsVolume.at(x).channelType) {
                    case ChannelType::LEFT:
                        snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, vol);
                        break;
                    case ChannelType::RIGHT:
                        snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, vol);
                        break;
                    default:
                        LOG(ERROR, __FUNCTION__, " invalid channel type ",
                            channelsVolume.at(x).channelType);
                        return telux::common::ErrorCode::INVALID_ARGUMENTS;
                }
            }
            break;
        case StreamType::CAPTURE:
            snd_mixer_selem_id_alloca(&sid);
            snd_mixer_selem_id_set_index(sid, 0);
            snd_mixer_selem_id_set_name(sid, "Capture");

            if ((elem = snd_mixer_find_selem(h_mixer, sid)) == NULL) {
                LOG(ERROR, __FUNCTION__,"Cannot find simple element");
            }

            for (uint32_t x = 0; x < numChannels; x++) {
                snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
                vol = channelsVolume.at(x).vol*max;
                switch(channelsVolume.at(x).channelType) {
                    case ChannelType::LEFT:
                        snd_mixer_selem_set_capture_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, vol);

                        break;
                    case ChannelType::RIGHT:
                        snd_mixer_selem_set_capture_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, vol);

                        break;
                    default:
                        LOG(ERROR, __FUNCTION__, " invalid channel type ",
                            channelsVolume.at(x).channelType);
                        return telux::common::ErrorCode::INVALID_ARGUMENTS;
                }
            }
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid stream type ", static_cast<int>(streamHandle.type));
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    snd_mixer_close(h_mixer);

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::getVolume(StreamHandle streamHandle,
        int channelTypeMask, std::vector<ChannelVolume>& channelsVolume) {

    int err;
    long vol;
    long min, max;
    ChannelVolume tmp{};
    snd_mixer_t *h_mixer;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem ;
    std::vector<snd_mixer_selem_channel_id_t> channels{};

    switch(channelTypeMask) {
        case ChannelType::LEFT:
            channels.push_back(SND_MIXER_SCHN_FRONT_LEFT);
            break;
        case ChannelType::RIGHT:
            channels.push_back(SND_MIXER_SCHN_FRONT_RIGHT);
            break;
        case (ChannelType::LEFT | ChannelType::RIGHT):
            channels.push_back(SND_MIXER_SCHN_FRONT_LEFT);
            channels.push_back(SND_MIXER_SCHN_FRONT_RIGHT);
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid channel type ", channelTypeMask);
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    if ((err = snd_mixer_open(&h_mixer, 0)) < 0) {
        LOG(ERROR, __FUNCTION__,"Mixer open error: ", err);
    }

    if ((err = snd_mixer_attach(h_mixer, sndCardCtlDevice_.c_str())) < 0) {
        LOG(ERROR, __FUNCTION__,"Mixer attach error: ", err);
    }

    if ((err = snd_mixer_selem_register(h_mixer, NULL, NULL)) < 0) {
        LOG(ERROR, __FUNCTION__,"Mixer simple element register error:", err);
    }

    if ((err = snd_mixer_load(h_mixer)) < 0) {
        LOG(ERROR, __FUNCTION__,"Mixer load error:", err);
    }

    switch(streamHandle.type) {
        case StreamType::PLAY:
            snd_mixer_selem_id_alloca(&sid);
            snd_mixer_selem_id_set_index(sid, 0);
            snd_mixer_selem_id_set_name(sid, "Master");

            if ((elem = snd_mixer_find_selem(h_mixer, sid)) == NULL) {
                LOG(ERROR, __FUNCTION__,"Cannot find simple element");
            }

            for(auto channel : channels) {
                snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
                snd_mixer_selem_get_playback_volume(elem, channel, &vol);
                switch(channel) {
                    case SND_MIXER_SCHN_FRONT_LEFT:
                        tmp.channelType = ChannelType::LEFT;
                        break;
                    case SND_MIXER_SCHN_FRONT_RIGHT:
                        tmp.channelType = ChannelType::RIGHT;
                        break;
                    default:
                        LOG(ERROR, __FUNCTION__, " invalid channel type ", channel);
                        return telux::common::ErrorCode::INVALID_ARGUMENTS;
                }
                tmp.vol = std::ceil((float)vol/max*10.0)/10.0;
                channelsVolume.emplace_back(tmp);
            }
            break;
        case StreamType::CAPTURE:
            snd_mixer_selem_id_alloca(&sid);
            snd_mixer_selem_id_set_index(sid, 0);
            snd_mixer_selem_id_set_name(sid, "Capture");

            if ((elem = snd_mixer_find_selem(h_mixer, sid)) == NULL) {
                LOG(ERROR, __FUNCTION__,"Cannot find simple element");
            }

            for(auto channel : channels) {
                snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
                snd_mixer_selem_get_capture_volume(elem, channel, &vol);
                switch(channel) {
                    case SND_MIXER_SCHN_FRONT_LEFT:
                        tmp.channelType = ChannelType::LEFT;
                        break;
                    case SND_MIXER_SCHN_FRONT_RIGHT:
                        tmp.channelType = ChannelType::RIGHT;
                        break;
                    default:
                        LOG(ERROR, __FUNCTION__, " invalid channel type ", channel);
                        return telux::common::ErrorCode::INVALID_ARGUMENTS;
                }
                tmp.vol = std::ceil((float)vol/max*10.0)/10.0;
                channelsVolume.emplace_back(tmp);
            }
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid stream type ", static_cast<int>(streamHandle.type));
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    snd_mixer_close(h_mixer);

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::setMuteState(StreamHandle streamHandle, StreamMute muteInfo,
    std::vector<ChannelVolume> channelsVolume, bool prevMuteState) {

    telux::common::ErrorCode ec;

    if (prevMuteState ==  muteInfo.enable){
        return telux::common::ErrorCode::SUCCESS;
    } else if (muteInfo.enable == true){
        /* Set volume for stream as 0 for muting the stream.*/
        for (uint32_t x = 0; x < channelsVolume.size(); x++) {
            channelsVolume.at(x).vol = 0;
        }
        ec = setVolume(streamHandle, muteInfo.dir, channelsVolume);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
    }
     else {
        /* If muteInfo.enable is false then restore the volume for the stream.*/
        ec = setVolume(streamHandle, muteInfo.dir, channelsVolume);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::getMuteState(StreamHandle streamHandle,
        StreamMute& muteInfo, StreamDirection direction) {
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::write(StreamHandle& streamHandle,
        uint8_t *data, uint32_t writeLengthRequested,
        uint32_t offset, int64_t timeStamp, bool isLastBuffer,
        int64_t& actualLengthWritten) {

    if(streamHandle.inTranscodeStreamId == inTranscodeStreamId_) {
        sendWriteReady++;
        if(sendWriteReady%pipelineLen == 0 && (!isLastBuffer)){
            auto streamEventListener = streamHandle.privateStreamData->streamEventListener.lock();
            if (streamEventListener) {
                streamEventListener->onWriteReadyEvent(inTranscodeStreamId_);
            }

            actualLengthWritten = 0;

            return telux::common::ErrorCode::SUCCESS;
        }

        actualLengthWritten = writeLengthRequested;
        if(isLastBuffer){
            auto streamEventListener = streamHandle.privateStreamData->streamEventListener.lock();
            if (streamEventListener) {
                streamEventListener->onDrainDoneEvent(inTranscodeStreamId_);
            }
            sendWriteReady = 0;
        }
        return telux::common::ErrorCode::SUCCESS;
    }

    /* Returns the no of frames written successfully. */
    actualLengthWritten = snd_pcm_writei(streamHandle.pcmHandle, data, streamHandle.frames);
    if (actualLengthWritten == -EPIPE) {
        snd_pcm_prepare(streamHandle.pcmHandle);
        free(data);
        actualLengthWritten = 0;
        return telux::common::ErrorCode::SUCCESS;
    }

    if(actualLengthWritten == -ESTRPIPE) {
        LOG(ERROR, __FUNCTION__,"write error: ", snd_strerror(actualLengthWritten));
        free(data);
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    free(data);
    LOG(DEBUG, __FUNCTION__,"written frames ", actualLengthWritten);
    /*
     * Set the actualLengthWritten to writeLengthRequested as the value for the last buffer size
     * will be lesser than streamHandle.frames * streamHandle.channels * 2. And this will result in
     * ambigous data being sent from server.
     */
    actualLengthWritten = writeLengthRequested;

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::read(StreamHandle& streamHandle,
        std::shared_ptr<std::vector<uint8_t>> data, uint32_t readLengthRequested,
        int64_t& actualReadLength) {

    if(streamHandle.outTranscodeStreamId == outTranscodeStreamId_) {
        actualReadLength = readLengthRequested;
        return telux::common::ErrorCode::SUCCESS;
    }

    uint8_t* bufferPtr = data->data();
    actualReadLength = snd_pcm_readi(streamHandle.pcmHandle, bufferPtr, streamHandle.frames);

    if (actualReadLength == -EPIPE || actualReadLength == -ESTRPIPE) {
        LOG(ERROR, __FUNCTION__,"read error: ", snd_strerror(actualReadLength));
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    LOG(DEBUG, __FUNCTION__,"read frames ", actualReadLength);
    actualReadLength = actualReadLength * streamHandle.channels * 2;

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::drain(StreamHandle streamHandle) {

    auto streamEventListener = streamHandle.privateStreamData->streamEventListener.lock();
    if (streamEventListener) {
        streamEventListener->onDrainDoneEvent(streamHandle.privateStreamData->streamId);
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::flush(StreamHandle streamHandle) {

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Configure and start playing dtmf tone.
 */
telux::common::ErrorCode Alsa::startDtmf(StreamHandle streamHandle, uint16_t gain,
        uint16_t duration, DtmfTone dtmfTone) {
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::stopDtmf(StreamHandle streamHandle, StreamDirection direction) {
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Enable DTMF detection. Currently only RX path is supported.
 */
telux::common::ErrorCode Alsa::registerDTMFDetection(StreamHandle streamHandle) {

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Disable DTMF detection.
 */
telux::common::ErrorCode Alsa::deRegisterDTMFDetection(StreamHandle streamHandle) {

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * AMR* data
 *  Input(from telsdk's perspective)/Play(from PAL's perspective)
 *                                 v
 *                            ------------
 *                           | Transcoder |
 *                            ------------
 *                                 v
 *  Output(from telsdk's perspective)/Capture(from PAL's perspective)
 * PCM data
 */
telux::common::ErrorCode Alsa::setupInTranscodeStream(StreamHandle& streamHandle,
        uint32_t streamId, TranscodingFormatInfo inInfo,
        std::shared_ptr<IStreamEventListener> streamEventListener,
        uint32_t& writeMinSize) {
    streamHandle.inTranscodeStreamId = streamId;
    inTranscodeStreamId_ = streamId;

    streamHandle.privateStreamData = new (std::nothrow) PrivateStreamData();
    if (!streamHandle.privateStreamData) {
        LOG(ERROR, __FUNCTION__, " can't allocate PrivateStreamData");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    streamHandle.privateStreamData->streamId = streamId;
    streamHandle.privateStreamData->streamEventListener = streamEventListener;

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::setupOutTranscodeStream(StreamHandle& streamHandle,
        uint32_t streamId, TranscodingFormatInfo outInfo,
        std::shared_ptr<IStreamEventListener> streamEventListener,
        uint32_t& readMinSize) {

    streamHandle.outTranscodeStreamId = streamId;
    outTranscodeStreamId_ = streamId;

     streamHandle.privateStreamData = new (std::nothrow) PrivateStreamData();
    if (!streamHandle.privateStreamData) {
        LOG(ERROR, __FUNCTION__, " can't allocate PrivateStreamData");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    streamHandle.privateStreamData->streamId = streamId;
    streamHandle.privateStreamData->streamEventListener = streamEventListener;

    return telux::common::ErrorCode::SUCCESS;
}


float Alsa::generateSignal(float t1, float t2) {
    //Z transform used for 2 frequency tone.
    float ret = 2;

    InFreq1_ = 2*cos(t1)*RegFreq1_[0]-RegFreq1_[1];
    RegFreq1_[1] = RegFreq1_[0];
    RegFreq1_[0] = InFreq1_;

    ret += sin(t1)*RegFreq1_[1];

    InFreq2_ = 2*cos(t2)*RegFreq2_[0]-RegFreq2_[1];
    RegFreq2_[1] = RegFreq2_[0];
    RegFreq2_[0] = InFreq2_;

    ret += sin(t2)*RegFreq2_[1];

    return ret;
}

void Alsa::genTone(std::vector<uint16_t> toneFrequency, int channels, uint32_t sampleRate,
    uint16_t gain, float buf[]) {

    int noOfFreq = toneFrequency.size();
    float t, t1, t2;

    switch(noOfFreq) {
        case 1:
            t = 2*M_PI*toneFrequency[0]/(sampleRate*channels);
            for (uint32_t i=0; i < sampleRate; i++) {
                buf[i] = sin(t*i);
            }
            break;
        case 2:
            t1 = 2*M_PI*toneFrequency[0]/(sampleRate*channels);
            t2 = 2*M_PI*toneFrequency[1]/(sampleRate*channels);
            for (uint32_t i=0; i < sampleRate ; i++) {
                buf[i] = generateSignal(t1,t2);
            }
            break;
        default:
            break;
    }
}

telux::common::ErrorCode Alsa::generateTone(StreamHandle streamHandle, uint32_t sampleRate,
    uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequency) {
    int ret;
    float buf[sampleRate];
    int nbSamples = sampleRate * streamHandle.channels * (duration/1000);
    int nbTimes = nbSamples / sampleRate;
    int restFrames = nbSamples % sampleRate;

    if (nbSamples >0) {
        genTone(toneFrequency, streamHandle.channels, sampleRate, gain, buf);
        if (nbTimes > 0) {
            for (int i=0; i < nbTimes; i++) {
                if(!runTone_){
                    break;
                }
                ret = snd_pcm_writei(streamHandle.pcmHandle, buf, sampleRate);
                if (ret == -EPIPE || ret == -ESTRPIPE) {
                    LOG(ERROR, __FUNCTION__,"write error: ", snd_strerror(ret));
                    snd_pcm_prepare(streamHandle.pcmHandle);
                    return telux::common::ErrorCode::SYSTEM_ERR;
                }
            }
        }
        if (restFrames > 0 && runTone_) {
            ret = snd_pcm_writei(streamHandle.pcmHandle, buf, restFrames);
            if (ret == -EPIPE || ret == -ESTRPIPE) {
                LOG(ERROR, __FUNCTION__,"write error: ", snd_strerror(ret));
                snd_pcm_prepare(streamHandle.pcmHandle);
                return telux::common::ErrorCode::SYSTEM_ERR;
            }
        }
    }
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Configure and start playing tone.
 */
telux::common::ErrorCode Alsa::startTone(StreamHandle& streamHandle, uint32_t sampleRate,
        uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequency) {

    if(runTone_) {
        stopTone(streamHandle);
    }

    runTone_ = true;
    streamHandle.streamStarted = true;
    std::thread toneThread(&Alsa::generateTone, this, streamHandle, sampleRate, gain,
        duration, toneFrequency);

    toneThread_.emplace_back(std::move(toneThread));

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Stop playing tone.
 */
telux::common::ErrorCode Alsa::stopTone(StreamHandle& streamHandle) {
    runTone_ = false;
    streamHandle.streamStarted = false;
    for(std::thread &th: toneThread_) {
        if(th.joinable()){
            th.join();
        }
    }

    RegFreq1_[0] = 1;
    RegFreq1_[1] = 0;
    RegFreq2_[0] = 1;
    RegFreq2_[1] = 0;
    snd_pcm_prepare(streamHandle.pcmHandle);

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode Alsa::getCalibrationStatus(CalibrationInitStatus& status) {

    LOG(INFO, __FUNCTION__, " not supported");
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

/*
 * By-default buffer size is set to maximum size IPC/RPC framework can support.
 * This is overridden, if PAL says buffer size should be less than this.
 */
telux::common::ErrorCode Alsa::setBufferSize(StreamHandle streamHandle,
        size_t& inSize, size_t& outSize) {
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Extract comma separated values and convert them into their telsdk/Pal specific values.
 */
int Alsa::loadMappingArray(std::string key, MappedValueType mappedValueType,
        uint32_t numOfValues, DeviceMappingTable& deviceTbl) {

    uint32_t mappedValue, x = 0;
    std::string commaSeparatedValues;
    std::stringstream valuesStream;

    if(config_){
        commaSeparatedValues = config_->getValue(key);
        if (commaSeparatedValues.empty()) {
            LOG(ERROR, __FUNCTION__, " can't read value of ", key);
            return -1;
        }

        valuesStream = std::stringstream(commaSeparatedValues);

        while ((valuesStream >> mappedValue) && (x < numOfValues)) {
            switch (mappedValueType) {
                case MappedValueType::DEVICE_TYPE:
                    deviceTbl.deviceType[x] = static_cast<DeviceType>(mappedValue);
                    break;
                case MappedValueType::DEVICE_DIR:
                    deviceTbl.deviceDir[x] = static_cast<DeviceDirection>(mappedValue);
                    break;
                default:
                    LOG(ERROR, __FUNCTION__, " invalid mapped value type");
            }

            ++x;
            if (valuesStream.peek() == ',') {
                valuesStream.ignore();
            }
        }

        if (x != numOfValues) {
            LOG(ERROR, __FUNCTION__, " invalid number of values for ", key);
            return -1;
        }
    }

    return 0;
}

int Alsa::loadUserDeviceMapping() {

    int ret;
    uint32_t numDevices;
    DeviceMappingTable deviceTbl{};

    if(config_){

        pcmDevice_ = config_->getValue("PCM_DEVICE");
        if (pcmDevice_.empty()) {
            return -1;
        }

        sndCardCtlDevice_ = config_->getValue("SND_CARD_CTL_DEVICE");
        if (sndCardCtlDevice_.empty()) {
            return -1;
        }

        std::string numOfDevices = config_->getValue("NUM_DEVICES");
        if (numOfDevices.empty()) {
            return -1;
        }

        try {
            numDevices = std::stoul(numOfDevices, nullptr, 10);
        } catch (const std::exception& e) {
            LOG(ERROR, __FUNCTION__, " can't interpret NUM_DEVICES");
            return -1;
        }

        if (numDevices > MAX_DEVICES) {
            LOG(ERROR, __FUNCTION__," NUM_DEVICES more then supported");
            return -1;
        }
        deviceTbl.numDevices = numDevices;

        ret = loadMappingArray("DEVICE_TYPE", MappedValueType::DEVICE_TYPE,
                numDevices, deviceTbl);
        if (ret < 0) {
            return ret;
        }

        ret = loadMappingArray("DEVICE_DIR", MappedValueType::DEVICE_DIR,
                numDevices, deviceTbl);
        if (ret < 0) {
            return ret;
        }

        finalDevicesTable_ = deviceTbl;
        LOG(INFO, __FUNCTION__, " device mapping from tel.conf loaded");
        return 0;
    }

    return -1;
}


void Alsa::onEventUpdate(::eventService::UnsolicitedEvent event) {
    if (event.filter() == AUDIO_FILTER) {
        onEventUpdate(event.event());
        return;
    }

    LOG(ERROR, __FUNCTION__, "invalid event ", event.filter());
}

void Alsa::onEventUpdate(std::string event) {
    std::string token;
    /*
     * getNextToken() modifies event string after extracting next token. This function searches a
     * substring for a token and copies that token to a target item. Refer to Events.json for event
     * string format.
     */

    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);

    if (DTMF_EVENT == token) {
        /* INPUT-token: dtmf_tone
         * INPUT-event: lowFreq highFreq
         */
        handleDTMFDetectedEvent(event);
    } else if (SSR_EVENT == token) {
        /* INPUT-token: ssr
         * INPUT-event: SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
         */
        handleSSREvent(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void Alsa::handleDTMFDetectedEvent(std::string eventParams) {
    uint32_t lowFreq, highFreq;

    lowFreq = std::stoi(EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER));

    switch (static_cast<DtmfLowFreq>(lowFreq)) {
        case DtmfLowFreq::FREQ_697:
        case DtmfLowFreq::FREQ_770:
        case DtmfLowFreq::FREQ_852:
        case DtmfLowFreq::FREQ_941:
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid low frquency ",
                lowFreq, " ,dropping event");
            return;
    }

    highFreq = std::stoi(EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER));
    switch (static_cast<DtmfHighFreq>(highFreq)) {
        case DtmfHighFreq::FREQ_1209:
        case DtmfHighFreq::FREQ_1336:
        case DtmfHighFreq::FREQ_1477:
        case DtmfHighFreq::FREQ_1633:
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid high frquency ",
                highFreq, " ,dropping event");
            return;
    }
    LOG(DEBUG, __FUNCTION__,"Registered listener, sending notifcation ");
    for(auto it = dtmfIndicationListenerMap_.begin(); it != dtmfIndicationListenerMap_.end(); it++)
    {
        it->second->onDTMFDetectedEvent(it->first, lowFreq, highFreq, StreamDirection::RX);
    }
}


void Alsa::handleSSREvent(std::string eventParams) {

    if (eventParams == "SERVICE_AVAILABLE") {
        for(auto sp : ssrListenerMap_){
            sp->onSSREvent(SSREvent::AUDIO_ONLINE);
        }
    } else if (eventParams == "SERVICE_UNAVAILABLE" || eventParams == "SERVICE_FAILED") {
        for(auto sp : ssrListenerMap_){
            sp->onSSREvent(SSREvent::AUDIO_OFFLINE);
        }
    } else {
        /* Drop the event. */
        LOG(ERROR, __FUNCTION__, " invalid SSR event: ", eventParams);
        return;
    }
}

#ifdef __cplusplus
}
#endif

}  // end of namespace audio
}  // end of namespace telux
