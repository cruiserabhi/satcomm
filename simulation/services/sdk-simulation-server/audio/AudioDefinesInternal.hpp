/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIODEFINESINTERNAL_HPP
#define AUDIODEFINESINTERNAL_HPP

#include <memory>
#include <vector>
#include <telux/audio/AudioDefines.hpp>
#include <alsa/asoundlib.h>
#include "IStreamEventListener.hpp"

#define GET_SUPPORTED_DEVICES_REQ 1
#define GET_SUPPORTED_STREAMS_REQ 2
#define CREATE_STREAM_REQ 3
#define DELETE_STREAM_REQ 4
#define STREAM_START_REQ 5
#define STREAM_STOP_REQ 6
#define STREAM_SET_DEVICE_REQ 7
#define STREAM_GET_DEVICE_REQ 8
#define STREAM_SET_VOLUME_REQ 9
#define STREAM_GET_VOLUME_REQ 10
#define STREAM_SET_MUTE_STATE_REQ 11
#define STREAM_GET_MUTE_STATE_REQ 12
#define STREAM_READ_REQ 13
#define STREAM_WRITE_REQ 14
#define STREAM_DTMF_START_REQ 15
#define STREAM_DTMF_STOP_REQ 16
#define GET_CAL_INIT_STATUS_REQ 17
#define STREAM_TONE_START_REQ 18
#define STREAM_TONE_STOP_REQ 19
#define DELETE_TRANSCODER_REQ 20
#define CREATE_TRANSCODER_REQ  21
#define STREAM_FLUSH_REQ 22
#define STREAM_DRAIN_REQ 23
#define STREAM_DTMF_DETECTED_IND 26
#define AUDIO_STATUS_IND 27
#define STREAM_WRITE_IND 28
#define STREAM_DRAIN_IND 29

#define SKIP_CALLBACK -1
#define JSON_AUDIO_API "api/audio/IAudioManager.json"

#define DEFAULT_DELIMITER " "
#define DTMF_EVENT "dtmf_tone"
#define SSR_EVENT "ssr"
#define AUDIO_FILTER "audio"

namespace telux {
namespace audio {

/*
 * 1. On platform where Q6 runs both audio code and modem code (ex; sa515m),
 *    it represents Q6 crashed.
 * 2. On platform with separate Q6 and ADSP hardware (ex; sa410m), it represents
 *    ADSP crashed.
 *
 * Purpose of SSREvent is to indicate latest SSR state. On the other hand, purpose
 * of AudioServiceImpl::ssrInProgress_ is to influence what to do when SSR updates
 * are detected.
 */
enum SSREvent {
    AUDIO_ONLINE,
    AUDIO_OFFLINE
};

/*
 * Defines the purpose of the stream based on which it is created and configured
 * in a certain way.
 */
enum StreamPurpose {
    TRANSCODER_IN,
    TRANSCODER_OUT,
    DEFAULT
};

/*
 * When a stream is created, this data is allocated and associated with that
 * stream.
 */
struct PrivateStreamData {
    uint32_t streamId;
    std::weak_ptr<IStreamEventListener> streamEventListener;
};

/*
 * Data that needs to be passed back and forth between service and backend.
 */
struct StreamHandle {
    StreamType type;
    uint32_t inTranscodeStreamId;
    uint32_t outTranscodeStreamId;
    snd_pcm_t *pcmHandle;
    snd_pcm_t *loopbackPlayHandle;
    snd_pcm_t *loopbackCaptureHandle;
    snd_pcm_uframes_t frames;
    int channels = 0;
    PrivateStreamData *privateStreamData;
    bool streamStarted = false;
    bool dtmfStarted = false;
    bool isAMR = false;
};

/*
 * Represents all user provided inputs to create a stream.
 */
struct StreamConfiguration {
    StreamConfig streamConfig;
    AmrwbpFrameFormat frameFormat;
    uint32_t bitWidth;
};

/*
 * Wraps user provided inputs with info needed to operate internally.
 */
struct StreamParams {
    StreamConfiguration config;
    StreamVolume streamVols;
    StreamMute muteStatus;
    uint32_t streamId;
    std::shared_ptr<telux::audio::IStreamEventListener> streamEventListener;
};

/*
 * Represents all user provided inputs to configure transcoder.
 */
struct TranscodingFormatInfo {
    uint32_t sampleRate;
    ChannelTypeMask mask;
    AudioFormat format;
    uint32_t bitWidth;
    AmrwbpFrameFormat frameFormat;
};

/*
 * Represents operational parameters once transcoder has been created.,
 */
struct CreatedTranscoderInfo {
    uint32_t inStreamId;
    uint32_t outStreamId;
    uint32_t readMinSize;
    uint32_t writeMinSize;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // AUDIODEFINESINTERNAL_HPP
