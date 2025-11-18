/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOCLIENT_HPP
#define AUDIOCLIENT_HPP

#include <vector>
#include <map>

#include "libs/common/Logger.hpp"
#include "IAudioMsgDispatcher.hpp"

namespace telux {
namespace audio {

/*
 * Represents a client (indepedent of transport type) currently connected to the audio server.
 * It contains data specific to that client.
 */
class AudioClient {

 public:
    AudioClient(int clientId, std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher);
    ~AudioClient();

    int32_t getClientId();

    std::weak_ptr<IAudioMsgDispatcher> getAudioMsgDispatcher(void);

    void associateStream(uint32_t streamId, StreamType type=StreamType::NONE);

    bool disassociateStream(uint32_t streamId, StreamType type=StreamType::NONE);

    void disassociateAllStreams(void);

    std::map<StreamType,std::vector<uint32_t>> getAssociatedStreamIdList(void);

 private:
    int clientId_ = 0;

    std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher_;

    std::map<StreamType, std::vector<uint32_t>> streamIdsList_;
};

} // end namespace audio
} // end namespace telux

#endif // AUDIOCLIENT_HPP
