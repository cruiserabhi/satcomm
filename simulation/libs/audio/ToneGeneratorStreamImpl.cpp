/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "ToneGeneratorStreamImpl.hpp"

namespace telux {
namespace audio {

ToneGeneratorStreamImpl::ToneGeneratorStreamImpl(uint32_t streamId,
        std::shared_ptr<ICommunicator> transportClient)
        : AudioStreamImpl(streamId, StreamType::TONE_GENERATOR, transportClient) {
}

ToneGeneratorStreamImpl::~ToneGeneratorStreamImpl() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Generate tone for the given duration at given frequency and gain.
 */
telux::common::Status ToneGeneratorStreamImpl::playTone(std::vector<uint16_t> frequency,
        uint16_t duration, uint16_t gain, telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->playTone(streamId_, frequency, duration, gain,
            downcasted_shared_from_this<ToneGeneratorStreamImpl>(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of playTone() invocation,
 * it calls that callback method otherwise simply drops the result.
 */
void ToneGeneratorStreamImpl::onToneStartResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Stops playing the tone started with ToneGeneratorStreamImpl::playTone().
 */
telux::common::Status ToneGeneratorStreamImpl::stopTone(
        telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->stopTone(streamId_,
            downcasted_shared_from_this<ToneGeneratorStreamImpl>(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of
 * ToneGeneratorStreamImpl::stopTone() invocation, it calls that callback
 * method otherwise simply drops the result.
 */
void ToneGeneratorStreamImpl::onToneStopResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

}  // end of namespace audio
}  // end of namespace telux
