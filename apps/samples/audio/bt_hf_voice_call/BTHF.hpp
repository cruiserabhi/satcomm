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

class BTHF {

 public:
    int init();
    int allocateBuffers();

    int createBTPlayStream();
    int deleteBTPlayStream();

    int createBTCaptureStream();
    int deleteBTCaptureStream();

    int createCodecPlayStream();
    int deleteCodecPlayStream();

    int createCodecCaptureStream();
    int deleteCodecCaptureStream();

    void readCompleteCodec(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error);
    void writeCompleteCodec(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error);

    void readCompleteBluetooth(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error);
    void writeCompleteBluetooth(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error);

    void readFromBluetoothWriteOnCodec();
    void readFromCodecWriteOnBluetooth();

    bool keepRunning_ = true;
    std::mutex btReadMutex_;
    std::mutex codecReadMutex_;
    std::condition_variable btReadWaiterCv_;
    std::condition_variable codecReadWaiterCv_;

 private:
    const int32_t BUFFER_COUNT = 2;
    uint32_t btReadSize_;
    uint32_t codecReadSize_;

    int32_t btReadDone_;
    int32_t codecReadDone_;
    int32_t codecWritePossible_;
    int32_t codecReadPossible_;
    int32_t btReadPossible_;
    int32_t btWritePossible_;

    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> readyForCodecWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> readyForBluetoothWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> btReadBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> btWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> codecReadBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> codecWriteBuffers_;

    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioPlayStream> btPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> btCaptureStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> codecPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> codecCaptureStream_;
};
