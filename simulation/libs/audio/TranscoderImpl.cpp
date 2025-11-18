/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "TranscoderImpl.hpp"

namespace telux {
namespace audio {

TranscoderImpl::TranscoderImpl(CreatedTranscoderInfo transcoderInfo,
        std::shared_ptr<ICommunicator> transportClient) {

    transportClient_ = transportClient;

    inStreamId_ = transcoderInfo.inStreamId;
    outStreamId_ = transcoderInfo.outStreamId;
    readMinSize_ = transcoderInfo.readMinSize;
    readMaxSize_ = transcoderInfo.readMaxSize;
    writeMinSize_ = transcoderInfo.writeMinSize;
    writeMaxSize_ = transcoderInfo.writeMaxSize;
    isLastBuffer_ = 0;
}

TranscoderImpl::~TranscoderImpl() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Setup the ListenerManager for invoking application provided callbacks.
 * Also register callbacks for drain done and write ready events with
 * audio qmi client.
 */
telux::common::Status TranscoderImpl::init() {

    /* Used to pass events on playback-stream like drain done and write ready
     * to the registered client (application) */
    try {
        eventListenerMgr_ = std::make_shared<
            telux::common::ListenerManager<ITranscodeListener>>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create ListenerManager");
        return telux::common::Status::FAILED;
    }

    /* Register to get drain done and write ready events */
    return transportClient_->registerForPlayStreamEvents(shared_from_this());
}

/*
 * Contains data to transcode - input to transcoder.
 */
std::shared_ptr<IAudioBuffer> TranscoderImpl::getWriteBuffer() {

    try {
        return std::make_shared<AudioBufferImpl>(writeMinSize_, writeMaxSize_,
            0, writeMaxSize_);
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create AudioBufferImpl");
    }

    return nullptr;
}

/*
 * Contains transcoded data - output from transcoder.
 */
std::shared_ptr<IAudioBuffer> TranscoderImpl::getReadBuffer() {

    try {
        return std::make_shared<AudioBufferImpl>(readMinSize_, readMaxSize_,
            0, readMaxSize_);
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create AudioBufferImpl");
    }

    return nullptr;
}

/*
 * Send data for transcoding.
 *
 * All steps are same as for PCM playback except that, the next buffer should
 * be sent only after 'write ready indication' has been received.
 */
telux::common::Status TranscoderImpl::write(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t isLastBuffer, TranscoderWriteResponseCb callback) {

    uint32_t numBytesToWrite = 0;
    telux::common::Status status;
    AudioUserData *audioUserData = nullptr;

    /* represents full qmi write request message */
    uint8_t *transportBuffer;

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

    audioUserData->audioBuffer = std::dynamic_pointer_cast<AudioBufferImpl>(buffer);
    if (!audioUserData->audioBuffer) {
        LOG(ERROR, __FUNCTION__, " invalid IAudioBuffer");
        if (callback) {
            cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
        }
        delete audioUserData;
        return telux::common::Status::INVALIDPARAM;
    }

    numBytesToWrite = audioUserData->audioBuffer->getDataSize();

    if ((numBytesToWrite > MAX_BUFFER_SIZE) || (numBytesToWrite <= 0)) {
        LOG(ERROR, __FUNCTION__, " invalid data length ", numBytesToWrite);
        if (callback) {
            cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
        }
        delete audioUserData;
        return telux::common::Status::INVALIDPARAM;
    }

    transportBuffer = audioUserData->audioBuffer->getTransportBuffer();

    status = transportClient_->write(inStreamId_, transportBuffer, isLastBuffer,
                shared_from_this(), audioUserData, numBytesToWrite);
    if (status != telux::common::Status::SUCCESS) {
        if (callback) {
            cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
        }
        delete audioUserData;
    }

    return status;
}

/*
 * Indicates data has been sent for transcoding.
 */
void TranscoderImpl::onWriteResult(telux::common::ErrorCode ec, uint32_t streamId,
        uint32_t bytesWritten, AudioUserData *audioUserData) {

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

    cmdCallbackMgr_.executeCallback(resultListener, audioUserData->audioBuffer,
        bytesWritten, ec);

    delete audioUserData;
}

/*
 * Read request to get transcoded data.
 */
telux::common::Status TranscoderImpl::read(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t numBytesToRead, TranscoderReadResponseCb callback) {

    telux::common::Status status;
    AudioUserData *audioUserData = nullptr;

    /* represents full qmi read response message */
    uint8_t *transportBuffer;

    if ((numBytesToRead > MAX_BUFFER_SIZE) || (numBytesToRead <= 0)) {
        LOG(ERROR, __FUNCTION__, " invalid numBytesToRead ", numBytesToRead);
        return telux::common::Status::INVALIDPARAM;
    }

    if (!buffer) {
        LOG(ERROR, __FUNCTION__, " invalid IAudioBuffer");
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

    audioUserData->audioBuffer = std::dynamic_pointer_cast<AudioBufferImpl>(buffer);
    if (!audioUserData->audioBuffer) {
        LOG(ERROR, __FUNCTION__, " invalid IAudioBuffer");
        if (callback) {
            cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
        }
        delete audioUserData;
        return telux::common::Status::INVALIDPARAM;
    }

    transportBuffer = audioUserData->audioBuffer->getTransportBuffer();

    status = transportClient_->read(outStreamId_, numBytesToRead, transportBuffer,
                shared_from_this(), audioUserData);
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
 * Read transcoded data. TranscoderImpl::onReadResult and TranscoderImpl::onDrainDone
 * are executed from the same thread hence serialized. Therefore no lock needed to
 * protect isLastBuffer_.
 */
void TranscoderImpl::onReadResult(telux::common::ErrorCode ec, uint32_t streamId,
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

    audioUserData->audioBuffer->setDataSize(numBytesActuallyRead);

    cmdCallbackMgr_.executeCallback(resultListener, audioUserData->audioBuffer,
        isLastBuffer_, ec);

    delete audioUserData;
}

/*
 * Delete the streams and release resources allocated for transcoding.
 */
telux::common::Status TranscoderImpl::tearDown(
        telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->deleteTranscoder(inStreamId_, outStreamId_,
            shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * Result of transcoder deletion.
 */
void TranscoderImpl::onDeleteTranscoderResult(telux::common::ErrorCode ec,
        uint32_t inStreamId_, uint32_t outStreamId_, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Register application listener for write ready event.
 */
telux::common::Status TranscoderImpl::registerListener(
        std::weak_ptr<ITranscodeListener> listener) {

    std::vector<std::weak_ptr<ITranscodeListener>> transcodeListeners;

    eventListenerMgr_->getAvailableListeners(transcodeListeners);

    return eventListenerMgr_->registerListener(listener);
}

/*
 * De-register application listener for write ready event.
 */
telux::common::Status TranscoderImpl::deRegisterListener(
        std::weak_ptr<ITranscodeListener> listener) {

    return eventListenerMgr_->deRegisterListener(listener);
}

/*
 * Send write ready event to the application.
 */
void TranscoderImpl::onWriteReady(uint32_t streamId) {

    LOG(DEBUG, __FUNCTION__);

    std::vector<std::weak_ptr<ITranscodeListener>> listeners;

    eventListenerMgr_->getAvailableListeners(listeners);

    if (listeners.size() == 0) {
        return;
    }

    for (auto &wp : listeners) {
        if (auto sp = wp.lock()) {
            LOG(DEBUG, __FUNCTION__, " calling ready");
            sp->onReadyForWrite();
        }
    }
}

/*
 * Indicates, that the last buffer was submitted for transcoding, it is safe
 * to stop further read operations now.
 */
void TranscoderImpl::onDrainDone(uint32_t streamId) {

    /* Last buffer for transcoding has been processed,
     * application can prepare to stop reading. */
    isLastBuffer_ = 1;
}

/*
 * Receives audio SSR updates.
 */
void TranscoderImpl::onServiceStatusChange() {

    cmdCallbackMgr_.reset();
}

}  // end of namespace audio
}  // end of namespace telux
