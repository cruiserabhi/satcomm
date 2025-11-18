/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "AudioClient.hpp"
#include <algorithm>

namespace telux {
namespace audio {

AudioClient::AudioClient(
    int clientId,
    std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher) {

    clientId_ = clientId;
    audioMsgDispatcher_ = audioMsgDispatcher;
}

AudioClient::~AudioClient() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Gives communicator client associated with this audio client.
 */
int32_t AudioClient::getClientId() {
    return clientId_;
}

/*
 * Gives message dispatcher using which this client will send responses and
 * indications to the application.
 */
std::weak_ptr<IAudioMsgDispatcher> AudioClient::getAudioMsgDispatcher() {
    return audioMsgDispatcher_;
}

/*
 * Marks that the given audio stream identified by the given streamId is owned
 * by this audio client.
 */
void AudioClient::associateStream(uint32_t streamId, StreamType type) {
    streamIdsList_[type].push_back(streamId);
}

/*
 * Disassociates the audio stream ownership from this audio client. If the given
 * stream is found to be associated with this audio client, it is deleted and true
 * is returned otherwise false.
 */
bool AudioClient::disassociateStream(uint32_t streamId, StreamType type) {

    if (type != StreamType::NONE) {
        auto itr = std::find_if(streamIdsList_[type].begin(), streamIdsList_[type].end(),
                [streamId](uint32_t id) {return (streamId == id);});
        if (itr != streamIdsList_[type].end()) {
            streamIdsList_[type].erase(itr);
            return true;
        }
    } else {
        for (auto stream : streamIdsList_) {
            auto itr = std::find_if(stream.second.begin(), stream.second.end(),
                    [streamId](uint32_t id) { return (streamId == id); });
            if (itr != stream.second.end()) {
                stream.second.erase(itr);
                return true;
            }
        }
    }

    return false;
}

/*
 * Disassociates ownership of all audio streams owned by this audio client.
 */
void AudioClient::disassociateAllStreams() {
    streamIdsList_.clear();
}

/*
 * Gives a list of audio streams currently owned by this audio client.
 */
std::map<StreamType, std::vector<uint32_t>> AudioClient::getAssociatedStreamIdList() {
    return streamIdsList_;
}

} // end namespace audio
} // end namespace telux
