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

#include <queue>
#include <atomic>
#include <condition_variable>

#include <telux/audio/AudioManager.hpp>

class Hpcm {

 public:
    int init();

    int createVoiceStream();
    int deleteVoiceStream();
    int startVoiceStream();
    int stopVoiceStream();

    int allocateBuffers();

    int createTXPlayStream();
    int deleteTXPlayStream();

    int createTXCaptureStream();
    int deleteTXCaptureStream();

    int createRXPlayStream();
    int deleteRXPlayStream();

    int createRXCaptureStream();
    int deleteRXCaptureStream();

    void readCompleteTX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error);
    void writeCompleteTX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error);

    void readCompleteRX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error);
    void writeCompleteRX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error);

    void readFromTXwriteOnTX();
    void readFromRXwriteOnRX();

    bool keepRunning_ = true;
    std::mutex txReadMutex_;
    std::mutex rxReadMutex_;
    std::condition_variable txReadWaiterCv_;
    std::condition_variable rxReadWaiterCv_;

 private:
    const int32_t BUFFER_COUNT = 2;
    uint32_t txReadSize_;
    uint32_t rxReadSize_;

    int32_t txReadDone_;
    int32_t rxReadDone_;
    int32_t txWritePossible_;
    int32_t txReadPossible_;
    int32_t rxReadPossible_;
    int32_t rxWritePossible_;

    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> readyForTxWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> readyForRxWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> txReadBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> txWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> rxReadBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> rxWriteBuffers_;

    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioVoiceStream> audioVoiceStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> txPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> txCaptureStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> rxPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> rxCaptureStream_;
};
