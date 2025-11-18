/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HPCMMENU_HPP
#define HPCMMENU_HPP

#include <queue>

#include "ConsoleApp.hpp"
#include "AudioClient.hpp"
#include "../../common/Audio/VoiceSession.hpp"
#include "../../common/Audio/AudioHelper.hpp"
#include <telux/audio/AudioManager.hpp>

#define NO_PLAY_BUFFER 2

class HpcmMenu : public ConsoleApp,
                 public std::enable_shared_from_this<HpcmMenu> {
 public:
    HpcmMenu(std::string appName, std::string cursor, std::shared_ptr<IAudioManager> audioManager);
    ~HpcmMenu();
    void init();
    void setSystemReady();
    void cleanup();

 private:
    Status createVoiceStream(StreamConfig &config);
    Status createHpcmRecordStream(StreamConfig &config);
    Status createHpcmPlayStream(StreamConfig &config);
    Status startVoiceStream();
    Status startHpcm();
    Status deleteHpcmRecordStream();
    Status deleteHpcmPlayStream();
    Status stopVoiceStream();
    Status deleteVoiceStream();
    void startHpcmAudio(std::vector<std::string> userInput);
    void stopHpcmAudio(std::vector<std::string> userInput);
    void deleteActiveSession(SlotId slotId);
    Status createActiveSession(SlotId slotId);
    Status setActiveSession(SlotId slotId);
    void play();
    void record();
    void writeCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error);
    void readCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
    telux::common::ErrorCode error);
    void takeUserVoicePathInput(std::vector<telux::audio::Direction> &direction);
    Status createStream(StreamConfig &streamConfig);
    void getUserSampleRateInput(uint32_t &sampleRate);
    void getUserSlotIdInput(SlotId &slotId);

    SlotId slotId_;
    std::atomic<bool> hpcmReady_;
    std::atomic<bool> exitHpcm_;
    bool readErrorOccurred_;
    bool writeErrorOccurred_;
    bool exitPlayThread_;
    bool exitRecordThread_;
    std::mutex mutex_;
    std::mutex captureMutex_;
    std::mutex bufferReadyMutex_;
    std::condition_variable captureCv_;
    std::condition_variable bufferReadyCv_;
    std::vector<std::thread> runningThreads_;
    std::shared_ptr<VoiceSession> activeSession_;
    std::map<SlotId, std::shared_ptr<VoiceSession>> voiceSessions_;
    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioVoiceStream> audioVoiceStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> audioPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> audioCaptureStream_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> freePlayBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> freeCaptureBuffers_;

};

#endif  // HPCMMENU_HPP
