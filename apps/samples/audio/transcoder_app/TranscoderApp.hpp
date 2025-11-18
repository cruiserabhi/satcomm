/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include<queue>

#include <telux/audio/AudioManager.hpp>
#include <telux/audio/AudioListener.hpp>
#include <telux/audio/AudioTranscoder.hpp>

using namespace telux::audio;
using namespace telux::common;

class TranscoderApp : public ITranscodeListener,
                       public std::enable_shared_from_this<TranscoderApp> {
public:
    TranscoderApp();
    ~TranscoderApp();

    Status init();
    void transcode();
    void teardown();

    void onReadyForWrite() override;

private:
    void writeCallback(std::shared_ptr<IAudioBuffer> buffer, uint32_t bytes, ErrorCode error);
    void readCallback(std::shared_ptr<telux::audio::IAudioBuffer> buffer,
         uint32_t isLastBuffer, telux::common::ErrorCode error);
    void createTranscoder();
    void write();
    void read();
    void registerListener();
    void deRegisterListener();
    std::vector<std::thread> runningThreads_;
    std::shared_ptr<ITranscoder> transcoder_;
    std::shared_ptr<IAudioManager> audioManager_;
    std::condition_variable cv_;
    // Flag to represent if any Bitstream Buffer partial consumed and wait for event to trigger.
    FormatInfo inputConfig_;
    FormatInfo outputConfig_;
    bool pipeLineEmpty_;
    std::queue<std::shared_ptr<IAudioBuffer>> readBuffers_;
    std::queue<std::shared_ptr<IAudioBuffer>> writeBuffers_;
    std::mutex mutex_;
};
