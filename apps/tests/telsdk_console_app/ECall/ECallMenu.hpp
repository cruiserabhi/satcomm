/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ECALLMENU_HPP
#define ECALLMENU_HPP

#include <cctype>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

extern "C" {
#include <sys/time.h>
}

#include <telux/tel/Call.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Phone.hpp>
#include <telux/tel/CallListener.hpp>
#include <telux/tel/CallManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"
#include "MsdSettings.hpp"
#include "./../Audio/AudioClient.hpp"

#define CATEGORY_AUTO 1
#define CATEGORY_MANUAL 2
#define VARIANT_TEST 1
#define VARIANT_EMERGENCY 2
#define VARIANT_EMERGENCY_CUSTOM_NUMBER 3

const std::string ECALL_CATEGORY_AUTO = "auto";
const std::string ECALL_CATEGORY_MANUAL = "manual";
const std::string ECALL_VARIANT_TEST = "test";
const std::string ECALL_VARIANT_EMERGENCY = "emergency";

class ECallMenu : public ConsoleApp {
public:
   /**
    * Initialize commands and SDK
    */
   bool init();

   ECallMenu(std::string appName, std::string cursor);
   ~ECallMenu();

private:
   int phoneId_;
   std::vector<int> phoneIds_;
   std::shared_ptr<telux::tel::IPhoneManager> phoneManager_;
   std::shared_ptr<telux::tel::ICallManager> callManager_;

   bool initalizeSDK();

   /**
    * Sample dial operation
    */
   void makeCall(std::vector<std::string> inputCommand);
   /**
    * Sample hangup operation
    */
   void hangup(std::vector<std::string> inputCommand);

   /**
    * Sample eCall operation
    */
   void makeECall(std::vector<std::string> inputCommand);

   /**
    * Sample voice eCall operation
    */
   void makeCustomNumberECall(std::vector<std::string> inputCommand);

   /**
    * Sample eCall operation with raw PDU
    */
   void eCallWithPdu(std::vector<std::string> inputCommand);

   /**
    * Sample eCall SOS operation
    */
   void eCallSos(std::vector<std::string> inputCommand);

   // answer incoming call
   void answerCall(std::vector<std::string> inputCommand);

   /**
    * Sample Update eCall MSD operation
    */
   void updateECallMSD(std::vector<std::string> inputCommand);

   /**
    * Sample Update eCall MSD operation with raw pdu
    */
   void updateEcallMsdWithPdu(std::vector<std::string> inputCommand);

   /**
    * Changes the Phone ID to use for operations
    */
   void selectPhoneId(std::vector<std::string> inputCommand);

   /**
    * Sample get in progress calls operations
    */
   void getCalls(std::vector<std::string> inputCommand);

   std::string getCallDescription(std::shared_ptr<telux::tel::ICall> call);

   /**
    * Register a listener
    */
   void registerCallListener(std::shared_ptr<telux::tel::ICallListener> listener);

   /**
    * Remove a registered listener
    */
   void removeCallListener(std::shared_ptr<telux::tel::ICallListener> listener);

   /**
    * This method is useful to trim the spaces in options and converting them into LOWERCASE
    */
   std::string toLowerCase(std::string inputOption);

   /**
    * This method is used to initialize audio subsystem for the ecall.
    */
   void enableAudio(std::vector<std::string> userInput);

   /**
    * This method is used to query whether audio to be enabled for a particular call.
    */
   bool queryAudioState();

   /* This method is used to get emergency callback mode */
   void requestEcbm(std::vector<std::string> userInput);

   /* This method is used to exit emergency callback mode */
   void exitEcbm(std::vector<std::string> userInput);

   /* To get the encoded optional additional data content for Euro NCAP */
   void getEncodedOptionalAdditionalDataContent(std::vector<std::string> userInput);

   /* To retrieve the encoded eCall MSD payload */
   void getECallMsdPayload(std::vector<std::string> userInput);

   /* Updates static optional additional data content to main MSD. */
   void updateOptionalAdditionalDataContent(MsdSettings &msdSettings);

   // Member variable to keep the Listener object alive till application ends.
   std::shared_ptr<telux::tel::ICallListener> callListener_;

   class CallCommandCallback : public telux::tel::IMakeCallCallback {
   public:
      void makeCallResponse(telux::common::ErrorCode error,
                            std::shared_ptr<telux::tel::ICall>) override;
   };
   std::shared_ptr<CallCommandCallback> callCommandCallback_;

   class UpdateMsdCommandCallback : public telux::common::ICommandResponseCallback {
   public:
      void commandResponse(telux::common::ErrorCode error) override;
   };
   std::shared_ptr<UpdateMsdCommandCallback> updateMsdCommandCallback_;

   class HangupCommandCallback : public telux::common::ICommandResponseCallback {
   public:
      void commandResponse(telux::common::ErrorCode error) override;
   };
   std::shared_ptr<HangupCommandCallback> hangupCommandCallback_;

   class AnswerCommandCallback : public telux::common::ICommandResponseCallback {
   public:
      void commandResponse(telux::common::ErrorCode error) override;
   };
   std::shared_ptr<AnswerCommandCallback> answerCommandCallback_;
};

#endif  // ECallMenu_HPP
