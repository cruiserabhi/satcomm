/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOCONSOLEAPP_HPP
#define AUDIOCONSOLEAPP_HPP

#include <telux/audio/AudioManager.hpp>
#include "ConsoleApp.hpp"
#include "AudioClient.hpp"

using namespace telux::audio;
using namespace telux::common;

class AudioConsoleApp : public ConsoleApp,
                        public telux::audio::IAudioListener,
                        public std::enable_shared_from_this<AudioConsoleApp> {
public :
    AudioConsoleApp(std::string appName, std::string cursor);
    ~AudioConsoleApp();
    void init();
    void onServiceStatusChange(telux::common::ServiceStatus status) override;

private:
    void initConsole();
    void cleanup();
    void setSystemReady();
    void voiceMenu(std::vector<std::string> userInput);
    void playMenu(std::vector<std::string> userInput);
    void captureMenu(std::vector<std::string> userInput);
    void loopbackMenu(std::vector<std::string> userInput);
    void toneMenu(std::vector<std::string> userInput);
    void transCodeMenu(std::vector<std::string> userInput);
    void hpcmMenu(std::vector<std::string> userInput);
    void repeatedPlaybackMenu(std::vector<std::string> userInput);
    void getCalStatus(std::vector<std::string> userInput);
    void getSupportedDevices(std::vector<std::string> userInput);
    void getSupportedStreams(std::vector<std::string> userInput);

    std::string getStreamName(StreamType stream);

    // Audio Client is Created by the Audio Console app and it is passed to every Menu
    std::shared_ptr<AudioClient> audioClient_;
    // Instance of all menu created are stored to maintain parallel running streams
    std::shared_ptr<VoiceMenu> voiceMenu_;
    std::shared_ptr<PlayMenu> playMenu_;
    std::shared_ptr<CaptureMenu> captureMenu_;
    std::shared_ptr<LoopbackMenu> loopbackMenu_;
    std::shared_ptr<ToneMenu> toneMenu_;
    std::shared_ptr<TransCodeMenu> transCodeMenu_;
    std::shared_ptr<HpcmMenu> hpcmMenu_;
    std::shared_ptr<RepeatedPlaybackMenu> repeatedPlaybackMenu_;
    std::shared_ptr<IAudioManager> audioManager_;
    std::atomic<bool> ready_;
};

#endif  // AUDIOCONSOLEAPP_HPP
