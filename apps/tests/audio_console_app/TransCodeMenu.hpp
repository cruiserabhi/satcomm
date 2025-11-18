/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TRANSCODEMENU_HPP
#define TRANSCODEMENU_HPP

#include <queue>

#include "ConsoleApp.hpp"
#include "AudioClient.hpp"
#include <telux/audio/AudioManager.hpp>

class TransCodeMenu : public ConsoleApp,
                      public telux::audio::ITranscodeListener,
                      public std::enable_shared_from_this<TransCodeMenu> {
public:
    TransCodeMenu(std::string appName, std::string cursor);
    ~TransCodeMenu();
    void init();
    void cleanup();
    void setSystemReady();
    void onReadyForWrite() override;
    void tearDown(std::vector<std::string> userInput);

private:
    void startTranscoding(std::vector<std::string> userInput);
    void createTranscoder();
    void read();
    void write();
    void registerListener();
    void deRegisterListener();
    void takeFormatData(FormatInfo &info);

    void readCallback(std::shared_ptr<telux::audio::IAudioBuffer> buffer, uint32_t isLastBuffer,
                      telux::common::ErrorCode error);
    void writeCallback(std::shared_ptr<telux::audio::IAudioBuffer> buffer, uint32_t bytes,
                      telux::common::ErrorCode error);

    FormatInfo inputConfig_;
    FormatInfo outputConfig_;
    std::shared_ptr<ITranscoder> transcoder_;
    std::shared_ptr<IAudioManager> audioManager_;
    FILE * readFile_;
    FILE * writeFile_;
    std::mutex readFileM_;
    std::mutex writeFileM_;
    std::mutex writeM_;
    std::mutex readM_;
    std::mutex CreateTranscoderMutex_;
    std::condition_variable cv_;
    std::condition_variable cvRead_;
    std::condition_variable cvWrite_;
    std::string readFilePath_, writeFilePath_;
    std::vector<std::thread> runningThreads_;
    std::atomic<bool> writeStatus_;
    std::atomic<bool> readStatus_;
    std::queue<std::shared_ptr<telux::audio::IAudioBuffer>> writeBuffers_;
    std::queue<std::shared_ptr<telux::audio::IAudioBuffer>> readBuffers_;
    std::atomic<bool> pipeLineEmpty_;
    std::atomic<bool> ready_;
    std::atomic<bool> stopTranscoder_;
};

#endif // TRANSCODEMENU_HPP
