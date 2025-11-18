/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOBUFFERIMPL_HPP
#define AUDIOBUFFERIMPL_HPP

#include <telux/audio/AudioManager.hpp>

namespace telux {
namespace audio {

/*
 * Represents generic buffer used during read and write operations.
 *
 * Audio clients (applications) uses IAudioBuffer.*() APIs to exchange
 * playback/capture data with this library.
 */
class AudioBufferImpl : virtual public IAudioBuffer {

 public:
    AudioBufferImpl(uint32_t minBufferSize, uint32_t maxBufferSize,
        uint32_t actualDataoffset, uint32_t bufferWrapperSize);

    ~AudioBufferImpl();

    size_t getMinSize() override;

    size_t getMaxSize() override;

    uint8_t *getRawBuffer() override;

    uint32_t getDataSize() override;

    void setDataSize(uint32_t size) override;

    telux::common::Status reset() override;

    uint8_t *getTransportBuffer(void);

 private:
    uint32_t dataSize_ = 0;
    uint32_t minBufferSize_ = 0;
    uint32_t maxBufferSize_ = 0;
    uint32_t bufferWrapperSize_ = 0;
    uint32_t actualDataoffset_ = 0;
    uint8_t *bufferWrapper_ = nullptr;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // AUDIOBUFFERIMPL_HPP
