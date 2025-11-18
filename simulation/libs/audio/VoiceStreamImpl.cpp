/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "VoiceStreamImpl.hpp"

namespace telux {
namespace audio {

VoiceStreamImpl::VoiceStreamImpl(uint32_t streamId,
        std::shared_ptr<ICommunicator> transportClient)
            : AudioStreamImpl(streamId, StreamType::VOICE_CALL, transportClient) {
}

VoiceStreamImpl::~VoiceStreamImpl() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status VoiceStreamImpl::init() {

    /* Used to pass events on voice-stream like dtmf detection to the registered clients */
    try {
        eventListenerMgr_ = std::make_shared<telux::common::ListenerManager<IVoiceListener>>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create ListenerManager");
        return telux::common::Status::FAILED;
    }

    /* Register to get dtmf detected event */
    return transportClient_->registerForVoiceStreamEvents(streamId_,
                downcasted_shared_from_this<VoiceStreamImpl>());

}

/*
 * Starts a voice-call stream to send and recieve audio samples.
 *
 *   -------------------------------------------------------
 *  |  Stream type   | Start/Stop                           |
 *   -------------------------------------------------------
 *  | Voice call     | Y                                    |
 *  | Playback       | N/A                                  |
 *  | Capture        | N/A                                  |
 *  | Loopback       | Y                                    |
 *  | Tone generator | N/A                                  |
 *   -------------------------------------------------------
 */
telux::common::Status VoiceStreamImpl::startAudio(telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->startStream(streamId_,
            downcasted_shared_from_this<VoiceStreamImpl>(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of VoiceStreamImpl::startAudio
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void VoiceStreamImpl::onStreamStartResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Stops a voice-call stream started with VoiceStreamImpl::startAudio.
 */
telux::common::Status VoiceStreamImpl::stopAudio(telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->stopStream(streamId_,
            downcasted_shared_from_this<VoiceStreamImpl>(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of VoiceStreamImpl::stopAudio
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void VoiceStreamImpl::onStreamStopResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Generates DTMF tone on the given voice-call stream with user supplied parameters.
 *
 *   ---------------------------------------
 *  |  Stream type   | DTMF generate/detect |
 *   ---------------------------------------
 *  | Voice call     | Y - direction RX/TX  |
 *  | Playback       | N/A                  |
 *  | Capture        | N/A                  |
 *  | Loopback       | N/A                  |
 *  | Tone generator | N/A                  |
 *   ---------------------------------------
 *
 * (a) On a voice call, playDtmfTone() generates DTMF tone on local speaker. This
 *     same signal is also sent to far-end device connected to cellular network.
 * (b) On a voice call, registerListener() registers with HAL/PAL for DTMF signal
 *     detection. When it detects DTMF, an event is sent to the application.
 * (c) Telephony also has an API to generate DTMF signal. When invoked, it sends
 *     character to cellular network which in turn actually generates corresponding
 *     DTMF tone.
 *
 * To generate a DTMF tone corresponding to a given key, a particular pair of the
 * low and high frequency is used, as shown in the table below.
 *
 *   -----------------------------------------------
 *  |                   |    High frequencies       |
 *  |                   | 1209 | 1336 | 1477 | 1633 |
 *   -----------------------------------------------
 *  | Low          697  |  1   |  2   |  3   |  A   |
 *  | frequencies  770  |  4   |  5   |  6   |  B   |
 *  |              852  |  7   |  8   |  9   |  C   |
 *  |              941  |  *   |  0   |  #   |  D   |
 *   -----------------------------------------------
 */
telux::common::Status VoiceStreamImpl::playDtmfTone(DtmfTone dtmfTone, uint16_t duration,
        uint16_t gain, telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->playDtmfTone(dtmfTone, duration, gain, streamId_,
            downcasted_shared_from_this<VoiceStreamImpl>(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of VoiceStreamImpl::playDtmfTone
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void VoiceStreamImpl::onPlayDtmfResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Stops the DTMF tone that was generated using VoiceStreamImpl::playDtmfTone.
 */
telux::common::Status VoiceStreamImpl::stopDtmfTone(StreamDirection direction,
        telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->stopDtmfTone(direction, streamId_,
            downcasted_shared_from_this<VoiceStreamImpl>(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of VoiceStreamImpl::stopDtmfTone
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void VoiceStreamImpl::onStopDtmfResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Telsdk audio server registers with HAL/PAL to receive events on a
 * voice-call stream on behalf of the audio clients. This registration is
 * needed only once.
 *
 * Currently only DTMF is used. On SA515M, when using DSDA, session based
 * detection is enabled. This means, same linux process can have detection
 * using two SIMs concurrently.
 *
 * Case 2: Audio server has not registered with HAL/PAL. An application now
 * registers for voice-call events. In this case, we go all the way to the
 * audio server and request it to register with HAL/PAL for voice-call events.
 *
 * Case 1: Audio server has already registered with HAL/PAL. In this case, we
 * simply append given listener to the list of existing listeners as all of
 * them all are subscribing for events on the same voice-call stream.
 */
telux::common::Status VoiceStreamImpl::registerListener(std::weak_ptr<IVoiceListener> listener,
        telux::common::ResponseCallback callback) {

    telux::common::Status status;
    std::shared_future<void> future;
    std::vector<std::weak_ptr<IVoiceListener>> voiceStreamEventListeners;

    status = eventListenerMgr_->registerListener(listener);
    if (status != telux::common::Status::SUCCESS) {
        return status;
    }

    if (callback) {
        future = std::async(std::launch::async,
            [callback]() {
                callback(telux::common::ErrorCode::SUCCESS);
            }).share();

        asyncTaskQueue_.add(future);
    }

    return status;
}

/*
 * Unregisteres the listener registered with VoiceStreamImpl::registerListener.
 */
telux::common::Status VoiceStreamImpl::deRegisterListener(
        std::weak_ptr<IVoiceListener> listener) {

    telux::common::Status status;
    std::vector<std::weak_ptr<IVoiceListener>> voiceStreamEventListeners;

    status = eventListenerMgr_->deRegisterListener(listener);
    return status;
}

/*
 * Whenever a DTMF signal is detected, this method invokes all the listeners
 * to pass them information about this DTMF signal.
 */
void VoiceStreamImpl::onDtmfToneDetected(DtmfTone dtmfTone) {

    std::vector<std::weak_ptr<IVoiceListener>> voiceStreamEventListeners;

    eventListenerMgr_->getAvailableListeners(voiceStreamEventListeners);

    for (auto &wp : voiceStreamEventListeners) {
        if (auto sp = wp.lock()) {
            sp->onDtmfToneDetection(dtmfTone);
        }
    }
}



}  // end of namespace audio
}  // end of namespace telux
