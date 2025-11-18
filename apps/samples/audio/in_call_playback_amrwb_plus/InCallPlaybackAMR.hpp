/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <queue>
#include <condition_variable>

#include <telux/audio/AudioManager.hpp>
#include <telux/audio/AudioListener.hpp>

class InCallPlaybackAMR : public telux::audio::IPlayListener,
                        public std::enable_shared_from_this<InCallPlaybackAMR> {

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

    void onReadyForWrite() override;
    void onPlayStopped() override;

    char *fileToPlayPath_;

 private:
    const int32_t TIME_10_SECONDS = 10;
    const int32_t BUFFER_POOL_SIZE = 2;
    bool errorOccurred_;
    bool frameworkReadyForNextWrite_;
    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioVoiceStream> audioVoiceStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> audioPlayStream_;
    FILE *fileToPlay_;
    std::mutex playMutex_;
    std::mutex playStopMutex_;
    std::condition_variable writeWaitCv_;
    std::condition_variable playStopCv_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> bufferPool_;
};
