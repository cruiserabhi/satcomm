/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TRANSPORTDEFINES_HPP
#define TRANSPORTDEFINES_HPP

/* These definitions must be exactly same in following entities:
 * Client side audio library
 * Audio server
 * IPC/RPC framework specific transport files
 *
 * Each IPC/RPC based framework will generate these defines in their format
 * therefore we can't use them as is in the source code. Therefore to ensure
 * all entities are using exact same values end to end this header file used. */

namespace telux {
namespace audio {

/* Indication message type - register/deregister for DTMF detection  */
constexpr uint32_t DETECT_DTMF = 1;

/* Maximum number of input (ex; Mic) and output (ex; Speaker) supported */
constexpr uint32_t MAX_DEVICES = 12;

/*  Left and Right channel */
constexpr uint32_t MAX_CHANNELS = 2;

/* For play and capture operations, maximum size of the buffer to use when sending
 * or receiving data from underlying audio framework */
constexpr uint32_t MAX_BUFFER_SIZE = 4096;

/* For play and capture operations, minimum size of the buffer to use when sending
 * or receiving data from underlying audio framework */
constexpr uint32_t MIN_BUFFER_SIZE = 0;

/* Maximum number of stream types */
constexpr uint32_t MAX_STREAM_TYPES = 5;

/* Maximum number of frequencies in tone */
constexpr uint32_t MAX_TONE_FREQ_SUPPORTED = 2;

/* Maximum number of voice path */
constexpr uint32_t MAX_VOICE_PATH = 3;

/* Current audio service status */
constexpr uint32_t AUDIO_SERVICE_ONLINE = 1;
constexpr uint32_t AUDIO_SERVICE_OFFLINE = 2;

}  // end of namespace audio
}  // end of namespace telux

#endif // TRANSPORTDEFINES_HPP
