/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STREAMCACHE_HPP
#define STREAMCACHE_HPP

#include <telux/common/CommonDefines.hpp>

#include "Stream.hpp"

namespace telux {
namespace audio {

/*
 * Every audio stream is identified by a unique identifier. StreamCache associates
 * a Stream object with this identifier. When an operation is to be performed on
 * a given stream, audio client sends this identifier to audio service. The service
 * retrives corresponding audio stream and performs actual operation with the help
 * of audio backend.
 */
class StreamCache {

 public:
    StreamCache();
    ~StreamCache();

    telux::common::ErrorCode getNextAvailableStreamID(uint32_t& streamId);

    void releaseStreamId(uint32_t streamId);

    std::shared_ptr<Stream> retrieveStream(uint32_t streamId);

    void cacheStream(uint32_t streamId, std::shared_ptr<Stream> stream);

    void unCacheStream(uint32_t streamId);

    void purgeAllStreamIds(void);

 private:
    /*
     * Maximum number of audio streams that can exist concurrently. Practically,
     * total number of streams will be very less due to limited capacity of ADSP.
     */
    static const uint32_t MAX_NUM_STREAMS = 16;

    /* Used to ensure setting and getting stream id is atomic. */
    std::mutex streamIndexMutex_;

    /*
     * When creating a stream, it is assigned a unique number as unique identifier.
     * A bit, if set at a particular index, in bitset indicates that the number has been
     * assigned to a stream. Helper functions operate on this bitset in atomic fashion.
     */
    std::bitset<MAX_NUM_STREAMS> streamIdIndexes_{0};

    /*
     * When stream creation is successful, unique integer (stream id) is returned
     * to client-side library. This array holds 'streamID -> Stream object' mapping
     * at the server side so that an audio operation can be performed on the stream
     * subsequently.
     *
     * 1. Memory gain is obtained by caching pointers instead of objects.
     * 2. Performance gain is obtained by not using iterator for retrieving
     *    'Stream' object from given stream id. Also there is no dynamic
     *    insertion or deletion resulting in vector iterator invalidation
     *    or memory re-allocation to accomodate new size.
     */
    std::shared_ptr<Stream> streamsCache_[MAX_NUM_STREAMS];
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // STREAMCACHE_HPP
