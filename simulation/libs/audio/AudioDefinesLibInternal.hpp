/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIODEFINESLIBINTERNAL_HPP
#define AUDIODEFINESLIBINTERNAL_HPP

#include <telux/audio/AudioDefines.hpp>

#include "StreamBufferImpl.hpp"

/* Msg Id used for sending audio request to the server. */
#define GET_SUPPORTED_DEVICES_REQ 1
#define GET_SUPPORTED_DEVICES_RESP 1
#define GET_SUPPORTED_STREAMS_REQ 2
#define GET_SUPPORTED_STREAMS_RESP 2
#define CREATE_STREAM_REQ 3
#define CREATE_STREAM_RESP 3
#define DELETE_STREAM_REQ 4
#define DELETE_STREAM_RESP 4
#define STREAM_START_REQ 5
#define STREAM_START_RESP 5
#define STREAM_STOP_REQ 6
#define STREAM_STOP_RESP 6
#define STREAM_SET_DEVICE_REQ 7
#define STREAM_SET_DEVICE_RESP 7
#define STREAM_GET_DEVICE_REQ 8
#define STREAM_GET_DEVICE_RESP 8
#define STREAM_SET_VOLUME_REQ 9
#define STREAM_SET_VOLUME_RESP 9
#define STREAM_GET_VOLUME_REQ 10
#define STREAM_GET_VOLUME_RESP 10
#define STREAM_SET_MUTE_STATE_REQ 11
#define STREAM_SET_MUTE_STATE_RESP 11
#define STREAM_GET_MUTE_STATE_REQ 12
#define STREAM_GET_MUTE_STATE_RESP 12
#define STREAM_READ_REQ 13
#define STREAM_READ_RESP 13
#define STREAM_WRITE_REQ 14
#define STREAM_WRITE_RESP 14
#define STREAM_DTMF_START_REQ 15
#define STREAM_DTMF_START_RESP 15
#define STREAM_DTMF_STOP_REQ 16
#define STREAM_DTMF_STOP_RESP 16
#define GET_CAL_INIT_STATUS_REQ 17
#define GET_CAL_INIT_STATUS_RESP 17
#define STREAM_TONE_START_REQ 18
#define STREAM_TONE_START_RESP 18
#define STREAM_TONE_STOP_REQ 19
#define STREAM_TONE_STOP_RESP 19
#define DELETE_TRANSCODER_REQ 20
#define DELETE_TRANSCODER_RESP 20
#define CREATE_TRANSCODER_REQ  21
#define CREATE_TRANSCODER_RESP  21
#define STREAM_FLUSH_REQ 22
#define STREAM_FLUSH_RESP 22
#define STREAM_DRAIN_REQ 23
#define STREAM_DRAIN_RESP 23
#define STREAM_DTMF_DETECTED_IND 26
#define AUDIO_STATUS_IND 27
#define STREAM_WRITE_IND 28
#define STREAM_DRAIN_IND 29

#define MAX_DEVICES 12
#define MAX_VOICE_PATH 3
/*TODO: Update the correct max buffer size based on alsa version supported.*/
#define MAX_BUFFER_SIZE 4096


namespace telux {
namespace audio {

enum IndicationType {
    Indication_TYPE_DTMF_DETECT
};

struct AudioUserData {
    int cmdCallbackId;
    std::shared_ptr<StreamBufferImpl> streamBuffer;
    std::shared_ptr<AudioBufferImpl> audioBuffer;
};

struct CreatedStreamInfo {
    StreamType streamType;
    uint32_t streamId;
    uint32_t readMinSize;
    uint32_t readMaxSize;
    uint32_t writeMinSize;
    uint32_t writeMaxSize;
};

struct CreatedTranscoderInfo {
    uint32_t inStreamId;
    uint32_t outStreamId;
    uint32_t readMinSize;
    uint32_t readMaxSize;
    uint32_t writeMinSize;
    uint32_t writeMaxSize;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // AUDIODEFINESLIBINTERNAL_HPP
