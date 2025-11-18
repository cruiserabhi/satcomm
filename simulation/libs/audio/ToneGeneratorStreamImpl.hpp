/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TONEGENERATORSTREAMIMPL_HPP
#define TONEGENERATORSTREAMIMPL_HPP

#include "AudioStreamImpl.hpp"

namespace telux {
namespace audio {

/*
 * Represents an audio stream used for playing tones.
 */
class ToneGeneratorStreamImpl : public IAudioToneGeneratorStream,
                                public AudioStreamImpl,
                                public IToneCb {

 public:
    ToneGeneratorStreamImpl(uint32_t streamId,
        std::shared_ptr<ICommunicator> transportClient);
    ~ToneGeneratorStreamImpl();

    telux::common::Status playTone(std::vector<uint16_t> frequency, uint16_t duration,
        uint16_t gain, telux::common::ResponseCallback callback = nullptr);

    telux::common::Status stopTone(telux::common::ResponseCallback callback = nullptr);

    void onToneStartResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;

    void onToneStopResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // TONEGENERATORSTREAMIMPL_HPP