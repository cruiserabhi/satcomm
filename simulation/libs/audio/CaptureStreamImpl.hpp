/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef CAPTURESTREAMIMPL_HPP
#define CAPTURESTREAMIMPL_HPP

#include "AudioStreamImpl.hpp"

namespace telux {
namespace audio {

/*
 * Represents an audio stream meant to capture audio.
 */
class CaptureStreamImpl : public IAudioCaptureStream,
                          public AudioStreamImpl,
                          public IReadCb {

 public:
    CaptureStreamImpl(uint32_t streamId, uint32_t readMinSize, uint32_t readMaxSize,
        std::shared_ptr<ICommunicator> transportClient);

    ~CaptureStreamImpl();

    std::shared_ptr<IStreamBuffer> getStreamBuffer() override;

    telux::common::Status read(std::shared_ptr<IStreamBuffer> buffer,
        uint32_t numBytesToRead, ReadResponseCb callback = nullptr) override;

    void onReadResult(telux::common::ErrorCode ec, uint32_t streamId,
        uint32_t numBytesActuallyRead, AudioUserData *audioUserData) override;

 private:
    uint32_t readMinSize_ = 0;
    uint32_t readMaxSize_ = 0;

    CaptureStreamImpl(CaptureStreamImpl const &) = delete;
    CaptureStreamImpl &operator=(CaptureStreamImpl const &) = delete;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // CAPTURESTREAMIMPL_HPP
