/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOREQUEST_HPP
#define AUDIOREQUEST_HPP

#include<memory>

namespace telux {
namespace audio {

class IAudioMsgDispatcher;

/*
 * Represents a request from a client, indepedent of the transport type.
 */
class AudioRequest {

 public:
    AudioRequest(int cmdId, uint32_t msgId, int clientId,
        std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher);

    ~AudioRequest();

    std::weak_ptr<IAudioMsgDispatcher> getAudioMsgDispatcher(void);

    uint32_t getMsgId(void);

    int getCmdId(void);

    int getClientId(void);

 private:
    uint32_t msgId_ = 0;
    int cmdId_ = 0;
    int clientId_ = 0;
    std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher_;

};

}  // end of namespace audio
}  // end of namespace telux

#endif // AUDIOREQUEST_HPP
