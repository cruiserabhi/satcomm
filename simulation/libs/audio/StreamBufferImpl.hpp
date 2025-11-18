/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STREAMBUFFERIMPL_HPP
#define STREAMBUFFERIMPL_HPP

#include "AudioBufferImpl.hpp"

namespace telux {
namespace audio {

/*
 * AudioBufferImpl represents a buffer used during read and write operations.
 * It adds audio stream perspective to the generic AudioBufferImpl.
 */
class StreamBufferImpl : public IStreamBuffer,
                         public AudioBufferImpl {

 public:
    StreamBufferImpl(uint32_t minBufferSize, uint32_t maxBufferSize,
        uint32_t actualDataoffset, uint32_t bufferWrapperSize);

    ~StreamBufferImpl();
};

}  // end of namespace audio
}  // end of namespace telux

#endif // STREAMBUFFERIMPL_HPP
