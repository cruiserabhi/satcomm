/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ClientCache.hpp"
#include <algorithm>

namespace telux {
namespace audio {

ClientCache::ClientCache() {
}

ClientCache::~ClientCache() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Locally saves given audio client.
 */
void ClientCache::cacheClient(int clientId, std::shared_ptr<AudioClient> audioClient) {

    /* Protect against clients connecting/disconnecting concurrently */
    std::lock_guard<std::mutex> lock(clientListGuard_);
    audioClientsList_[clientId] = audioClient;
}

/*
 * Removes given audio client saved locally.
 */
void ClientCache::unCacheClient(std::shared_ptr<AudioClient> audioClient) {

    /* Protect against clients connecting/disconnecting concurrently */
    std::lock_guard<std::mutex> lock(clientListGuard_);

    audioClientsList_.erase(audioClient->getClientId());
}

/*
 * Associates the given audio stream identifier with the given audio client.
 * Locking is not needed as creating and deleting stream happens on same thread
 * hence serialized.
 */
void ClientCache::associateStream(std::shared_ptr<AudioClient> audioClient,
        StreamType type, uint32_t streamId) {

    audioClient->associateStream(streamId, type);
}

/*
 * Disassociates the given audio stream identifier from an audio client.
 * Locking is not needed as creating and deleting stream happens on same
 * thread hence serialized.
 */
void ClientCache::disassociateStream(uint32_t streamId) {

    bool deleted = false;

    for (auto it = audioClientsList_.begin(); it != audioClientsList_.end(); it++) {
        deleted = it->second->disassociateStream(streamId);
        if (deleted) {
            return;
        }
    }
}

/*
 * Gives list of all audio clients currently connected with the audio server.
 * Locking to protect against concurrent create stream and SSR handling is not
 * needed because both of them executes on same thread hence serialized.
 */
std::map<int,std::shared_ptr<AudioClient>>& ClientCache::getClientsList() {

    return audioClientsList_;
}

/*
 * Iterate over all clients and empty the stream id vector. Locking not required
 * as it is called during SSR handling only, during which regular audio operations
 * are not executed.
 */
void ClientCache::disassociateAllStreams(void) {

    for (auto it = audioClientsList_.begin(); it != audioClientsList_.end(); it++) {
        it->second->disassociateAllStreams();
    }
}

/*
 * Gives an audio client corresponding to the communicator client.
 */
std::shared_ptr<AudioClient> ClientCache::getAudioClientFromClientId(int clientId) {

    /* Protect against create stream for a client and the same client disconnecting */
    std::lock_guard<std::mutex> lock(clientListGuard_);

    if(audioClientsList_.find(clientId)!= audioClientsList_.end()){
        return audioClientsList_[clientId];
    }

    return nullptr;
}

/*
 * Gives an audio client corresponding to the audio stream identifier.
 */
std::shared_ptr<AudioClient> ClientCache::getAudioClientByStreamId(uint32_t streamId) {

    std::map<StreamType, std::vector<uint32_t>> streamIdList;

    /* Protect against delivering stream event to a client and the same client disconnecting */
    std::lock_guard<std::mutex> lock(clientListGuard_);

    for (auto it = audioClientsList_.begin(); it != audioClientsList_.end(); it++) {
        streamIdList = it->second->getAssociatedStreamIdList();
        for (auto stream : streamIdList) {
            auto itr = std::find_if(stream.second.begin(), stream.second.end(),
                    [streamId](uint32_t id) { return(id == streamId); });
            if (itr != stream.second.end()) {
                return it->second;
            }
        }
    }

    return nullptr;
}

}  // end of namespace audio
}  // end of namespace telux
