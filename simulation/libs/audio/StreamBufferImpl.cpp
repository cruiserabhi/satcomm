/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "StreamBufferImpl.hpp"

namespace telux {
namespace audio {

StreamBufferImpl::StreamBufferImpl(uint32_t minBufferSize, uint32_t maxBufferSize,
        uint32_t actualDataoffset, uint32_t bufferWrapperSize)
        : AudioBufferImpl(minBufferSize, maxBufferSize, actualDataoffset,
            bufferWrapperSize) {
}

StreamBufferImpl::~StreamBufferImpl() {
    LOG(DEBUG, __FUNCTION__);
}

}  // end of namespace audio
}  // end of namespace telux
