/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "common/Logger.hpp"

#include "AudioDefinesLibInternal.hpp"
#include "CaptureStreamImpl.hpp"

namespace telux {
namespace audio {

CaptureStreamImpl::CaptureStreamImpl(uint32_t streamId, uint32_t readMinSize,
        uint32_t readMaxSize, std::shared_ptr<ICommunicator> transportClient)
        : AudioStreamImpl(streamId, StreamType::CAPTURE, transportClient) {

    readMinSize_ = readMinSize;
    readMaxSize_ = readMaxSize;
}

CaptureStreamImpl::~CaptureStreamImpl() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Gives the audio buffer used to exchange audio data between audio client and this
 * library.
 */
std::shared_ptr<IStreamBuffer> CaptureStreamImpl::getStreamBuffer() {

    try {
        return std::make_shared<StreamBufferImpl>(readMinSize_, readMaxSize_,
            0, readMinSize_);
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create StreamBufferImpl");
    }

    return nullptr;
}

/*
 * Reads audio data from the audio device associated with this stream.
 */
telux::common::Status CaptureStreamImpl::read(std::shared_ptr<IStreamBuffer> buffer,
        uint32_t numBytesToRead, ReadResponseCb callback) {

    telux::common::Status status;
    AudioUserData *audioUserData = nullptr;

    /* represents full qmi read response message */
    uint8_t *transportBuffer;

    if ((numBytesToRead > MAX_BUFFER_SIZE) || (numBytesToRead <= 0)) {
        LOG(ERROR, __FUNCTION__, " invalid numBytesToRead ", numBytesToRead);
        return telux::common::Status::INVALIDPARAM;
    }

    if (!buffer) {
        LOG(ERROR, __FUNCTION__, " invalid IStreamBuffer");
        return telux::common::Status::INVALIDPARAM;
    }

    audioUserData = new (std::nothrow) AudioUserData;
    if (!audioUserData) {
        LOG(ERROR, __FUNCTION__, " can't allocate AudioUserData");
        return telux::common::Status::NOMEMORY;
    }

    audioUserData->cmdCallbackId = INVALID_COMMAND_ID;
    if (callback) {
        audioUserData->cmdCallbackId = cmdCallbackMgr_.addCallback(callback);
    }

    audioUserData->streamBuffer = std::dynamic_pointer_cast<StreamBufferImpl>(buffer);
    if (!audioUserData->streamBuffer) {
        LOG(ERROR, __FUNCTION__, " invalid StreamBufferImpl");
        if (callback) {
            cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
        }
        delete audioUserData;
        return telux::common::Status::INVALIDPARAM;
    }

    transportBuffer = audioUserData->streamBuffer->getTransportBuffer();

    status = transportClient_->read(streamId_, numBytesToRead, transportBuffer,
                downcasted_shared_from_this<CaptureStreamImpl>(), audioUserData);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "can't read stream, err ", static_cast<int>(status));
        if (callback) {
            cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
        }
        delete audioUserData;
    }

    return status;
}

/*
 * If application provided a callback to receive the result of CaptureStreamImpl::read
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void CaptureStreamImpl::onReadResult(telux::common::ErrorCode ec, uint32_t streamId,
        uint32_t numBytesActuallyRead, AudioUserData *audioUserData) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    if (!audioUserData) {
        LOG(ERROR, __FUNCTION__, " invalid AudioUserData");
        return;
    }

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", audioUserData->cmdCallbackId);
        delete audioUserData;
        return;
    }

    audioUserData->streamBuffer->setDataSize(numBytesActuallyRead);

    cmdCallbackMgr_.executeCallback(resultListener, audioUserData->streamBuffer, ec);

    delete audioUserData;
}

}  // end of namespace audio
}  // end of namespace telux
