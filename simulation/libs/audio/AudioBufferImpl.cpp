/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "AudioBufferImpl.hpp"

namespace telux {
namespace audio {

AudioBufferImpl::AudioBufferImpl(uint32_t minBufferSize, uint32_t maxBufferSize,
        uint32_t actualDataoffset, uint32_t bufferWrapperSize) {

    minBufferSize_ = minBufferSize;
    maxBufferSize_ = maxBufferSize;
    actualDataoffset_ = actualDataoffset;

    /* If exception happens, let it propagate to the caller */
    bufferWrapper_ = new uint8_t[bufferWrapperSize]{};
    bufferWrapperSize_ = bufferWrapperSize;
}

AudioBufferImpl::~AudioBufferImpl() {
    LOG(DEBUG, __FUNCTION__);

    delete[] bufferWrapper_;
}

/*
 * Gives minimum possible size of the buffer.
 */
size_t AudioBufferImpl::getMinSize() {

    return minBufferSize_;
}

/*
 * Gives maximum possible size of the buffer.
 */
size_t AudioBufferImpl::getMaxSize() {

    return maxBufferSize_;
}

/*
 * Gives pointer to the underlying raw buffer.
 */
uint8_t *AudioBufferImpl::getRawBuffer() {

    return bufferWrapper_ + actualDataoffset_;
}

/*
 * Gives actual size of the buffer.
 */
uint32_t AudioBufferImpl::getDataSize() {

    return dataSize_;
}

/*
 * Sets actual number of bytes used in read/write operation.
 */
void AudioBufferImpl::setDataSize(uint32_t newSize) {

    if (newSize > maxBufferSize_) {
        LOG(ERROR, __FUNCTION__, " invalid size, greater than maximum");
        return;
    }

    dataSize_ = newSize;
}

/*
 * Clears content of the transport buffer.
 */
telux::common::Status AudioBufferImpl::reset() {

    memset(bufferWrapper_, 0, bufferWrapperSize_);

    return telux::common::Status::SUCCESS;
}

/*
 * Gives transport buffer.
 */
uint8_t *AudioBufferImpl::getTransportBuffer() {

    return bufferWrapper_;
}

}  // end of namespace audio
}  // end of namespace telux
