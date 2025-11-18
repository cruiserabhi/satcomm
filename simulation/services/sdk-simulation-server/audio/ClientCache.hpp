/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CLIENTCACHE_HPP
#define CLIENTCACHE_HPP

#include <vector>
#include <map>

#include "libs/common/Logger.hpp"
#include "AudioClient.hpp"

namespace telux {
namespace audio {

/*
 * Maintains a list of currently connected clients to server and the streams
 * associated with them.
 */
class ClientCache {

 public:
    ClientCache();
    ~ClientCache();

    void cacheClient(int clientId, std::shared_ptr<AudioClient> audioClient);

    void unCacheClient(std::shared_ptr<AudioClient> audioClient);

    void associateStream(std::shared_ptr<AudioClient> audioClient, StreamType type,
      uint32_t streamId);

    void disassociateStream(uint32_t streamId);

    std::map<int,std::shared_ptr<AudioClient>>& getClientsList(void);

    void disassociateAllStreams(void);

    std::shared_ptr<AudioClient> getAudioClientFromClientId(int clientId);

    std::shared_ptr<AudioClient> getAudioClientByStreamId(uint32_t streamId);

 private:
    std::mutex clientListGuard_;
    std::map<int,std::shared_ptr<AudioClient>> audioClientsList_;
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // CLIENTCACHE_HPP
