/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "StreamCache.hpp"

namespace telux {
namespace audio {

StreamCache::StreamCache() {
    for (uint32_t x = 0; x < MAX_NUM_STREAMS; x++) {
        streamsCache_[x] = nullptr;
    }
}

StreamCache::~StreamCache() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Provides an unused unique identifier in an atomic test_and_set() fashion.
 */
telux::common::ErrorCode StreamCache::getNextAvailableStreamID(uint32_t& streamId) {

    std::lock_guard<std::mutex> lock(streamIndexMutex_);

    for(uint32_t x=0; x < MAX_NUM_STREAMS; x++) {
        if (!streamIdIndexes_.test(x)) {
            streamIdIndexes_.set(x);
            streamId = x;
            return telux::common::ErrorCode::SUCCESS;
        }
    }

    return telux::common::ErrorCode::NO_RESOURCES;
}

/*
 * Marks the given identifier as unused.
 */
void StreamCache::releaseStreamId(uint32_t streamId) {
    std::lock_guard<std::mutex> lock(streamIndexMutex_);
    streamIdIndexes_.reset(streamId);
}

/*
 * Gives an audio stream associated with the audio stream identifier.
 */
std::shared_ptr<Stream> StreamCache::retrieveStream(uint32_t streamId) {
    return streamsCache_[streamId];
}

/*
 * Associates the given audio stream identifier with the audio stream.
 * The audio stream reference is saved locally.
 */
void StreamCache::cacheStream(uint32_t streamId, std::shared_ptr<Stream> stream) {
    streamsCache_[streamId] = stream;
}

/*
 * Disassociates the audio stream from its audio stream identifier.
 * It also releases reference to the stream saved locally.
 */
void StreamCache::unCacheStream(uint32_t streamId) {
    streamsCache_[streamId] = nullptr;
}

/*
 * Marks all the audio stream identifiers as available for use.
 */
void StreamCache::purgeAllStreamIds() {
    std::lock_guard<std::mutex> lock(streamIndexMutex_);
    streamIdIndexes_.reset();
}

}  // end of namespace audio
}  // end of namespace telux
