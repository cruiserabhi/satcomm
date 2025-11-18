/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "AudioRequest.hpp"

namespace telux {
namespace audio {

AudioRequest::AudioRequest(int cmdId, uint32_t msgId, int clientId,
    std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher) {
    msgId_ = msgId;
    cmdId_ = cmdId;
    clientId_ = clientId;
    audioMsgDispatcher_ = audioMsgDispatcher;
}

AudioRequest::~AudioRequest() {
    /* LOG(DEBUG, __FUNCTION__); */
}

uint32_t AudioRequest::getMsgId(void) {
    return msgId_;
}

int AudioRequest::getCmdId(void) {
    return cmdId_;
}

int AudioRequest::getClientId(void) {
    return clientId_;
}

std::weak_ptr<IAudioMsgDispatcher>
        AudioRequest::getAudioMsgDispatcher() {

    return audioMsgDispatcher_;
}

}  // end of namespace audio
}  // end of namespace telux
