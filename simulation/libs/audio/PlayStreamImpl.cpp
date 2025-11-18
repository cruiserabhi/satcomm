/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "common/Logger.hpp"

#include "PlayStreamImpl.hpp"
#include "AudioDefinesLibInternal.hpp"


namespace telux {
namespace audio {

PlayStreamImpl::PlayStreamImpl(uint32_t streamId, uint32_t writeMinSize,
        uint32_t writeMaxSize, std::shared_ptr<ICommunicator> transportClient)
        : AudioStreamImpl(streamId, StreamType::PLAY, transportClient) {

    writeMinSize_ = writeMinSize;
    writeMaxSize_ = writeMaxSize;
}

PlayStreamImpl::~PlayStreamImpl() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status PlayStreamImpl::init() {
    /* Used to pass events on playback-stream like drain done and write ready
     * to the registered client (application) */
    try {
        eventListenerMgr_ = std::make_shared<telux::common::ListenerManager<IPlayListener>>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create ListenerManager");
        return telux::common::Status::FAILED;
    }

    /* Register to get drain done and write ready events */
    return transportClient_->registerForPlayStreamEvents(
                downcasted_shared_from_this<PlayStreamImpl>());
}

/*
 * Gives audio buffer used to exchange data between application and this library.
 * Note; every time this method is called, a new buffer is allocated and returned
 * to caller. This allows application to prepare next buffer while the current
 * buffer is getting played. This helps in smooth continuous playback hence better
 * user experience.
 */
std::shared_ptr<IStreamBuffer> PlayStreamImpl::getStreamBuffer() {

    try {
        return std::make_shared<StreamBufferImpl>(writeMinSize_, writeMaxSize_,
            0, writeMaxSize_);
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create StreamBufferImpl");
    }

    return nullptr;
}

/*
 * Sends audio data to the audio device associated with this stream.
 *
 * PCM format write flow:
 *
 * In a nutshell just keep sending buffers back-to-back until all
 * of them are not played. Fill the next buffer while the previous
 * one is getting played. Write complete in flow below refers to the
 * async qmi response received as response to the previous async qmi
 * write request.
 *
 * 1. Create a playback audio stream.
 * 2. Get minimum and maximum buffer size for this stream.
 * 3. Decide actual size of buffer to use. If minimum size is 0,
 *    use maximum otherwise use minimum size.
 * 4. Allocate two buffers to operate in ping-pong fashion.
 * 5. Get raw buffer and copy data to be played into 1st buffer.
 * 6. Call write() method to send this buffer to HAL/PAL.
 * 7. Fill 2nd buffer and call write() method to send this buffer
 *    to HAL/PAL.
 * 8. Write complete response callback will be invoked as a response to write
 *    complete for 1st buffer. In this callback fill the 1st buffer again
 *    and send it for playing by calling write().
 * 9. When write complete happens for 2nd buffer, fill it again and send
 *    for playback. Step 5 to 9 are repeated until all buffers are played.
 * 10. Delete the audio playback stream.
 *
 * AMR* format write flow:
 *
 * All steps are same as for PCM playback except when application should call write.
 * a. If the "number of bytes actually written == 0" OR "number of bytes actually written < number
 * of bytes to write" application should wait for write ready indication. Once received it should
 * send next buffer to play.
 * b. If the number of to write and number of bytes written are exactly same, application should
 * just send next buffer to write and should not wait for write ready indication.
 * c. If the write() returns and error, it should be treated as error and handled as per
 * application's business logic.
 */
telux::common::Status PlayStreamImpl::write(std::shared_ptr<IStreamBuffer> buffer,
        WriteResponseCb callback) {

    uint32_t numBytesToWrite = 0;
    telux::common::Status status;
    AudioUserData *audioUserData = nullptr;

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

    audioUserData->streamBuffer = std::dynamic_pointer_cast<StreamBufferImpl>(buffer);
    if (!audioUserData->streamBuffer) {
        LOG(ERROR, __FUNCTION__, " invalid IStreamBuffer");
        if (callback) {
            cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
        }
        delete audioUserData;
        return telux::common::Status::INVALIDPARAM;
    }

    numBytesToWrite = audioUserData->streamBuffer->getDataSize();
    if ((numBytesToWrite > MAX_BUFFER_SIZE) || (numBytesToWrite <= 0)) {
        LOG(ERROR, __FUNCTION__, " invalid data length ", numBytesToWrite);
        if (callback) {
            cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
        }
        delete audioUserData;
        return telux::common::Status::INVALIDPARAM;
    }

    transportBuffer = audioUserData->streamBuffer->getTransportBuffer();

    /* 0 - isLastBuffer is not applicable for regular playback stream */
    status = transportClient_->write(streamId_, transportBuffer, 0,
                downcasted_shared_from_this<PlayStreamImpl>(), audioUserData,
                numBytesToWrite);
    if (status != telux::common::Status::SUCCESS) {
        if (callback) {
            cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
        }
        delete audioUserData;
    }

    return status;
}

/*
 * If application provided a callback to receive the result of PlayStreamImpl::write
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void PlayStreamImpl::onWriteResult(telux::common::ErrorCode ec, uint32_t streamId,
        uint32_t bytesWritten, AudioUserData *audioUserData) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(audioUserData->cmdCallbackId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", audioUserData->cmdCallbackId);
        delete audioUserData;
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, audioUserData->streamBuffer,
        bytesWritten, ec);
}

/*
 * Applicable only for AMR* format playback.
 *
 * Indicates that there is no more data to be played and stream is about to be
 * closed.
 */
telux::common::Status PlayStreamImpl::stopAudio(StopType stopType,
        telux::common::ResponseCallback callback) {
    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    switch (stopType) {
        case StopType::FORCE_STOP:
            status = transportClient_->flush(streamId_, downcasted_shared_from_this<PlayStreamImpl>(
                ), cmdId);
            break;
        case StopType::STOP_AFTER_PLAY:
            status = transportClient_->drain(streamId_, downcasted_shared_from_this<PlayStreamImpl>(
                ), cmdId);
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid stop type ", static_cast<int>(stopType));
            if (callback) {
                cmdCallbackMgr_.findAndRemoveCallback(cmdId);
            }
            return telux::common::Status::INVALIDPARAM;
    }

    if(status != telux::common::Status::SUCCESS && callback){
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;;
}

/*
 * If application provided a callback to receive the result of PlayStreamImpl::stopAudio
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void PlayStreamImpl::onFlushResult(telux::common::ErrorCode ec, uint32_t streamId,
        int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * If application provided a callback to receive the result of PlayStreamImpl::stopAudio
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void PlayStreamImpl::onDrainResult(telux::common::ErrorCode ec, uint32_t streamId,
        int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}


/*
 * When AMR* format audio is played, these listeners, listen for drain, flush
 * and write complete messages (indications) from HAL/PAL. Currently, flush
 * is not required hence skipped.
 *
 * PlayStreamImpl::write()         - async request
 * PlayStreamImpl::onWriteResult() - async response
 * PlayStreamImpl::onWriteReady()  - async indication <-- register for this
 *
 * PlayStreamImpl::drain()         - async request
 * PlayStreamImpl::onDrainResult() - async response
 * PlayStreamImpl::onDrainDone()   - async indication <-- register for this
 *
 * Note: even when indication is used, async response to previously sent async
 * request will come. This flow confirms that a request has been submitted to
 * the HAL/PAL. When indication comes, it confirms that the HAL/PAL has actually
 * completed requested operation or can accept the next buffer.
 */
telux::common::Status PlayStreamImpl::registerListener(
        std::weak_ptr<IPlayListener> listener) {

    std::vector<std::weak_ptr<IPlayListener>> playListener;

    eventListenerMgr_->getAvailableListeners(playListener);

    return eventListenerMgr_->registerListener(listener);
}

telux::common::Status PlayStreamImpl::deRegisterListener(
        std::weak_ptr<IPlayListener> listener) {

    return eventListenerMgr_->deRegisterListener(listener);
}

/*
 * Indicates that the last buffer sent has been successfully played.
 * Let us stop now sending more buffers.
 */
void PlayStreamImpl::onDrainDone(uint32_t streamId) {

    std::vector<std::weak_ptr<IPlayListener>> listeners;

    eventListenerMgr_->getAvailableListeners(listeners);

    if (listeners.size() == 0) {
        /* There is no listener or it unregistered just before this
         * indication came, drop the indication */
        return;
    }

    for (auto &wp : listeners) {
        if (auto sp = std::dynamic_pointer_cast<IPlayListener>(wp.lock())) {
            sp->onPlayStopped();
        }
    }
}

/*
 * Indicates that ALSA is ready to accept next buffer to play.
 */
void PlayStreamImpl::onWriteReady(uint32_t streamId) {

    std::vector<std::weak_ptr<IPlayListener>> listeners;

    eventListenerMgr_->getAvailableListeners(listeners);

    if (listeners.size() == 0) {
        return;
    }

    for (auto &wp : listeners) {
        if (auto sp = std::dynamic_pointer_cast<IPlayListener>(wp.lock())) {
            sp->onReadyForWrite();
        }
    }
}

}  // end of namespace audio
}  // end of namespace telux
