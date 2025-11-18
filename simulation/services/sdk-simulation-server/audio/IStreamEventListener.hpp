/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ISTREAMEVENTLISTENER_HPP
#define ISTREAMEVENTLISTENER_HPP

#include <telux/audio/AudioDefines.hpp>

namespace telux {
namespace audio {

/*
 * Playback and voicecall type stream uses this to get drain done,
 * write ready and dtmf detected events from HAL/PAL.
 */
class IStreamEventListener {

 public:
    virtual void onDTMFDetectedEvent(uint32_t streamId, uint32_t lowFreq,
        uint32_t highFreq, StreamDirection streamDirection) = 0;

    virtual void onDrainDoneEvent(uint32_t streamId) = 0;

    virtual void onWriteReadyEvent(uint32_t streamId) = 0;
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // ISTREAMEVENTLISTENER_HPP