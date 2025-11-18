/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * Call Menu class provides dialer functionality of the SDK
 * it has menu options for dial, answer, hangup, reject, conference and swap calls
 */

#ifndef CALLMENU_HPP
#define CALLMENU_HPP

#include "console_app_framework/ConsoleApp.hpp"
#include "./../Audio/AudioClient.hpp"
#include "MyCallListener.hpp"

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2
#define MUTE 1
#define UNMUTE 0

class CallMenu : public ConsoleApp {
public:
   CallMenu(std::string appName, std::string cursor);
   virtual ~CallMenu();
   bool init();

protected:
   void dial(std::vector<std::string> userInput);
   void hangupWithCallIndex(std::vector<std::string> userInput);
   void conference(std::vector<std::string> userInput);
   bool queryMuteState(bool muteStatus);
   void rejectCall(std::vector<std::string> userInput);
   void hangupDialingOrAlerting(std::vector<std::string> userInput);
   void hangupForegroundResumeBackground(std::vector<std::string> userInput);
   void resumeCall(std::vector<std::string> userInput);
   void holdCall(std::vector<std::string> userInput);
   void swap(std::vector<std::string> userInput);
   void getCalls(std::vector<std::string> userInput);
   bool queryAudioState();

   std::shared_ptr<telux::tel::ICallListener> callListener_;
   std::shared_ptr<telux::tel::ICallManager> callManager_;
   std::shared_ptr<MyCallCommandCallback> myHoldCb_;
   std::shared_ptr<MyCallCommandCallback> myResumeCb_;
   std::shared_ptr<MyDialCallback> myDialCallCmdCb_;
   std::shared_ptr<MyCallCommandCallback> myHangupCb_;
   std::shared_ptr<MyCallCommandCallback> myAnswerCb_;
   std::shared_ptr<MyCallCommandCallback> myRejectCb_;
   std::shared_ptr<MyCallCommandCallback> mySwapCb_;

private:
   void conferenceSubMenu(std::vector<std::string> userInput);
   void acceptCall(std::vector<std::string> userInput);
   void realTimeTextSubMenu(std::vector<std::string> userInput);
   void playDtmfTone(std::vector<std::string> userInput);
   void startDtmfTone(std::vector<std::string> userInput);
   void stopDtmfTone(std::vector<std::string> userInput);
   void enableAudio(std::vector<std::string> userInput);

   std::shared_ptr<MyCallCommandCallback> myConferenceCb_;
   std::shared_ptr<MyCallCommandCallback> myPlayTonesCb_;
   std::shared_ptr<MyCallCommandCallback> myStartToneCb_;
   std::shared_ptr<MyCallCommandCallback> myStopToneCb_;
};

#endif  // CALLMENU_HPP
