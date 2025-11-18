/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LOOPBACKSTREAMIMPL_HPP
#define LOOPBACKSTREAMIMPL_HPP

#include "AudioStreamImpl.hpp"

namespace telux {
namespace audio {

/*
 * Represents an audio stream used for looping back audio.
 */
class LoopbackStreamImpl : public IAudioLoopbackStream,
                           public AudioStreamImpl,
                           public IStartStreamCb,
                           public IStopStreamCb {

 public:
    LoopbackStreamImpl(uint32_t streamId,
        std::shared_ptr<ICommunicator> transportClient);

    ~LoopbackStreamImpl();

    telux::common::Status startLoopback(telux::common::ResponseCallback callback = nullptr);

    telux::common::Status stopLoopback(telux::common::ResponseCallback callback = nullptr);

    void onStreamStartResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;

    void onStreamStopResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // LOOPBACKSTREAMIMPL_HPP