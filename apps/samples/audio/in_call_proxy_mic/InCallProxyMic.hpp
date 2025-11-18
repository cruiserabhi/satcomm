/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cstdio>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>

#include <telux/audio/AudioManager.hpp>

class InCallProxyMic {

 public:
    int init();
    int createVoiceStream();
    int deleteVoiceStream();
    int startVoiceStream();
    int stopVoiceStream();
    int createIncallPlayStream();
    int deleteIncallPlayStream();
    void play();
    void writeComplete(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error);

    char *fileToPlayPath_;

  private:
    const int32_t TIME_10_SECONDS = 10;
    const int32_t BUFFER_POOL_SIZE = 2;
    bool errorOccurred_;
    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioVoiceStream> audioVoiceStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> audioPlayStream_;
    FILE *fileToPlay_;
    std::mutex playMutex_;
    std::condition_variable cv_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> bufferPool_;
};
