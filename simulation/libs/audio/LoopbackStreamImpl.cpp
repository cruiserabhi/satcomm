/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "LoopbackStreamImpl.hpp"

namespace telux {
namespace audio {

LoopbackStreamImpl::LoopbackStreamImpl(uint32_t streamId,
        std::shared_ptr<ICommunicator> transportClient)
            : AudioStreamImpl(streamId, StreamType::LOOPBACK, transportClient) {
}

LoopbackStreamImpl::~LoopbackStreamImpl() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * The loopback-type stream was created using createStream() API. This method
 * starts actual loopback operation at physical level.
 */
telux::common::Status LoopbackStreamImpl::startLoopback(
        telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->startStream(streamId_,
            downcasted_shared_from_this<LoopbackStreamImpl>(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of
 * LoopbackStreamImpl::startLoopback() invocation, it calls that callback
 * method otherwise simply drops the result.
 */
void LoopbackStreamImpl::onStreamStartResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Stops looping-back audio started with LoopbackStreamImpl::startLoopback().
 */
telux::common::Status LoopbackStreamImpl::stopLoopback(
        telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->stopStream(streamId_,
            downcasted_shared_from_this<LoopbackStreamImpl>(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of
 * LoopbackStreamImpl::stopLoopback() invocation, it calls that callback
 * method otherwise simply drops the result.
 */
void LoopbackStreamImpl::onStreamStopResult(telux::common::ErrorCode ec,
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
