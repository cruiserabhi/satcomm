/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <chrono>
#include <iostream>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "RttMenu.hpp"

using namespace std;
#define NO_OF_SIMULTANEOUS_INCOMING_CALL 2
#define RTT_MODE_DISABLED 0
#define RTT_MODE_ENABLED 1

RttMenu::RttMenu(std::string id, std::string name)
    : CallMenu("Real Time Text Call Menu", "realTimeText> ") {
    menuOptionsAdded_ = false;
}

RttMenu::~RttMenu() {
    myDialCallCmdCb_ = nullptr;
    myHangupCb_ = nullptr;
    myModifyCb_ = nullptr;
    myrespondToModifyRequestCb_ = nullptr;
    myHoldCb_ = nullptr;
    myResumeCb_ = nullptr;
    myAnswerCb_ = nullptr;
    myRejectCb_ = nullptr;
    mySwapCb_ = nullptr;
}

bool RttMenu::init() {

    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    std::promise<ServiceStatus> callMgrprom;
    //  Get the PhoneFactory and CallManager instances.
    callManager_ = phoneFactory.getCallManager([&](ServiceStatus status) {
       callMgrprom.set_value(status);
    });
    if(!callManager_) {
       std::cout << "ERROR - Failed to get CallManager instance \n";
       return false;
    }
    std::cout << "CallManager subsystem is not ready " << ", Please wait " << std::endl;
    ServiceStatus callMgrsubSystemStatus = callMgrprom.get_future().get();

    if(callMgrsubSystemStatus == ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "CallManager subsystem is ready \n";
       myModifyCb_ = std::make_shared<MyCallCommandCallback>("Modify");
       myrespondToModifyRequestCb_ = std::make_shared<MyCallCommandCallback>("ModifyCallConfirm");
       myDialCallCmdCb_ = std::make_shared<MyDialCallback>();
       myHangupCb_ = std::make_shared<MyCallCommandCallback>("Hang");
       myHoldCb_ = std::make_shared<MyCallCommandCallback>("Hold");
       myResumeCb_ = std::make_shared<MyCallCommandCallback>("Resume");
       myAnswerCb_ = std::make_shared<MyCallCommandCallback>("Answer");
       myRejectCb_ = std::make_shared<MyCallCommandCallback>("Reject");
       mySwapCb_ = std::make_shared<MyCallCommandCallback>("Swap");
    } else {
       std::cout << "Unable to initialise CallManager subsystem " << std::endl;
       return false;
    }
    if (menuOptionsAdded_ == false) {

        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> dialRttCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Dial_RTT", {"number"},
        std::bind(&RttMenu::dialRttCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> acceptCallCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Accept_call",
            {}, std::bind(&RttMenu::acceptCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> rejectCallCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "3", "Reject_call", {},
            std::bind(&RttMenu::rejectCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> modifyCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Modify", {"callIndex"},
        std::bind(&RttMenu::modifyCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> respondToModifyRequestCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5",
            "Respond_To_Modify_Request", {"callIndex"},
        std::bind(&RttMenu::respondToModifyRequest, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> sendRttMessageCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Send_Rtt_Message", {},
        std::bind(&RttMenu::sendRttMessage, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> hangupWithCallIndexCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Hangup", {"index"},
            std::bind(&RttMenu::hangupWithCallIndex, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> hangupDialingOrAlertingCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Hangup", {},
            std::bind(&RttMenu::hangupDialingOrAlerting, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> holdCallCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Hold_call", {},
            std::bind(&RttMenu::holdCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> resumeCallCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Resume_call", {},
            std::bind(&RttMenu::resumeCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> swapCommand =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11", "Swap", {},
            std::bind(&RttMenu::swap, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getCallsCommand =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("12", "Get_InProgress_Calls", {},
        std::bind(&RttMenu::getCalls, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            dialRttCommand,
            acceptCallCommand,
            rejectCallCommand,
            modifyCommand,
            respondToModifyRequestCommand,
            sendRttMessageCommand,
            hangupWithCallIndexCommand,
            hangupDialingOrAlertingCommand,
            holdCallCommand,
            resumeCallCommand,
            swapCommand,
            getCallsCommand };
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void RttMenu::dialRttCall(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   const std::string phoneNumber = userInput[1];
   int phoneId = DEFAULT_PHONE_ID;

   if(telux::common::DeviceConfig::isMultiSimSupported()) {
      std::string slotSelection;
      char delimiter = '\n';

      std::cout << "Enter the desired Phone ID / SIM slot: ";
      std::getline(std::cin, slotSelection, delimiter);

      if (!slotSelection.empty()) {
         try {
            phoneId = std::stoi(slotSelection);
            if (phoneId < MIN_SIM_SLOT_COUNT || phoneId > MAX_SIM_SLOT_COUNT ) {
               std::cout << "ERROR: Invalid slot entered" << std::endl;
               return;
            }
         } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
               << slotSelection << std::endl;
            return;
         }
      } else {
         std::cout << "Empty input, enter the correct slot" << std::endl;
         return;
      }
   }
    static std::shared_ptr<AudioClient> audioClient = AudioClient::getInstance();
    if (audioClient->isReady()) {
        bool audioState = queryAudioState();
        std::cout << "Audio enablement status is : " << audioState << std::endl;
        if (audioState) {
            audioClient->startVoiceSession(static_cast<SlotId>(phoneId));
        }
    }
    telux::common::Status makeCallStatus
        = callManager_->makeRttCall(phoneId, phoneNumber, myDialCallCmdCb_);
    if (makeCallStatus == telux::common::Status::NOTALLOWED) {
        std::cout << "Multiple calls are already in progress." <<
           " Please hangup any one of the call or conference to initiate another call.\n";
    } else if (makeCallStatus == telux::common::Status::SUCCESS) {
        std::cout << "makeRttCall is successful.\n";
    } else {
        std::cout << "makeRttCall failed.\n";
    }
}

void RttMenu::acceptCall(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   std::string rttMode;
   char delimiter = '\n';
   int mode = 0;
   std::cout << "Enter RTT mode for the call: 0 - Disable RTT , 1 - Enable RTT ";
   std::getline(std::cin, rttMode, delimiter);
   try {
      mode = std::stoi(rttMode);
      std::cout << "RTT mode entered" << mode << std::endl;
      if(mode < RTT_MODE_DISABLED || mode > RTT_MODE_ENABLED) {
        std::cout << "ERROR: Invalid rtt mode is entered" << std::endl;
        return;
      }
   } catch (const std::exception &e) {
      std::cout << "ERROR: invalid input, please enter a valid value. INPUT: "
         << mode << std::endl;
      return;
   }
   if(telux::common::DeviceConfig::isMultiSimSupported()) {
      // Fetch the list of in progress calls from CallManager and count the
      // number of incoming/waiting calls.
      int incomingCalls = 0;
      for(auto callIterator = std::begin(inProgressCalls);
          callIterator != std::end(inProgressCalls); ++callIterator) {
         if(((*callIterator)->getCallState() == telux::tel::CallState::CALL_INCOMING)
             ||((*callIterator)->getCallState() == telux::tel::CallState::CALL_WAITING))
            ++incomingCalls;
      }
      //Incase of two simultaneous incoming calls, user to select the slotId on
      // which to accept the call
      if(incomingCalls >= NO_OF_SIMULTANEOUS_INCOMING_CALL) {
         std::string slotSelection;
         char delimiter = '\n';
         int phoneId = DEFAULT_PHONE_ID;

         std::cout << "Enter the desired Phone ID / SIM slot: ";
         std::getline(std::cin, slotSelection, delimiter);

         if (!slotSelection.empty()) {
            try {
               phoneId = std::stoi(slotSelection);
               if (phoneId < MIN_SIM_SLOT_COUNT || phoneId > MAX_SIM_SLOT_COUNT ) {
                  std::cout << "ERROR: Invalid slot entered" << std::endl;
                  return;
               }
            } catch (const std::exception &e) {
               std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
                  << slotSelection << std::endl;
               return;
            }
         } else {
            std::cout << "Empty input, enter the correct slot" << std::endl;
            return;
         }
         for(auto callIterator = std::begin(inProgressCalls);
             callIterator != std::end(inProgressCalls); ++callIterator) {
            if((*callIterator)->getPhoneId() == phoneId
               && (*callIterator)->getCallState() == telux::tel::CallState::CALL_INCOMING
                  || (*callIterator)->getCallState() == telux::tel::CallState::CALL_WAITING) {
                  spCall = *callIterator;
                  break;
            }
         }
      }
   }
   if(nullptr == spCall) {
      // Fetch the list of in progress calls from CallManager and accept the incoming/waiting call.
      for(auto callIterator = std::begin(inProgressCalls);
          callIterator != std::end(inProgressCalls); ++callIterator) {
         if (((*callIterator)->getCallState() == telux::tel::CallState::CALL_INCOMING)
            ||((*callIterator)->getCallState() == telux::tel::CallState::CALL_WAITING)) {
            spCall = *callIterator;
            break;
         }
      }
   }
    if(spCall) {
        static std::shared_ptr<AudioClient> audioClient = AudioClient::getInstance();
        if (audioClient->isReady()) {
            int phoneId = spCall->getPhoneId();
            bool audioState = queryAudioState();
            if (audioState) {
                audioClient->startVoiceSession(static_cast<SlotId>(phoneId));
            }
        }
        spCall->answer(myAnswerCb_, static_cast<telux::tel::RttMode>(mode));
    } else {
        std::cout << "No incoming/waiting call" << std::endl;
    }
}

void RttMenu::respondToModifyRequest(std::vector<std::string> userInput) {
    int callIndex;
    try {
        callIndex = std::stoi(userInput[1]);
    } catch (const std::exception &e) {
        std::cout << "Invalid index" << std::endl;
        return;
    }

    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callManager_->getInProgressCalls();
    int phoneId = getInputPhoneId();

    std::string input;
    char delimiter = '\n';
    int request = 0;
    std::cout << "Accept or Reject modify request: 0 - Reject , 1 - Accept ";
    std::getline(std::cin, input, delimiter);
    try {
        request = std::stoi(input);
        if(request < RTT_MODE_DISABLED || request > RTT_MODE_ENABLED) {
            std::cout << "ERROR: Invalid request is entered" << std::endl;
            return;
        }
        std::cout << "RTT mode entered" << request << std::endl;
    } catch (const std::exception &e) {
        std::cout << "ERROR: invalid input, please enter a valid value. INPUT: "
            << request << std::endl;
        return;
    }

    for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
        ++callIterator) {
        if((*callIterator)->getCallIndex() == callIndex
            && (*callIterator)->getPhoneId() == phoneId) {
            spCall = *callIterator;
            break;
        }
    }
    if(spCall) {
        spCall->respondToModifyRequest(static_cast<bool>(request), myrespondToModifyRequestCb_);
    } else {
        std::cout << "No call found with given index/slot" << std::endl;
    }
}

void RttMenu::modifyCall(std::vector<std::string> userInput) {
    int callIndex;
    try {
        callIndex = std::stoi(userInput[1]);
    } catch (const std::exception &e) {
        std::cout << "Invalid index" << std::endl;
        return;
    }

    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callManager_->getInProgressCalls();
    int phoneId = getInputPhoneId();
    std::string rttMode;
    char delimiter = '\n';
    int mode = 0;
    std::cout << "Enter RTT mode for the call: 0 - Disable RTT , 1 - Enable RTT ";
    std::getline(std::cin, rttMode, delimiter);
    try {
        mode = std::stoi(rttMode);
        if(mode < RTT_MODE_DISABLED || mode > RTT_MODE_ENABLED) {
            std::cout << "ERROR: Invalid rtt mode is entered" << std::endl;
            return;
        }
        std::cout << "RTT mode entered" << mode << std::endl;
    } catch (const std::exception &e) {
        std::cout << "ERROR: invalid input, please enter a valid value. INPUT: "
            << mode << std::endl;
        return;
    }

    for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
        ++callIterator) {
        if((*callIterator)->getCallIndex() == callIndex
            && (*callIterator)->getPhoneId() == phoneId) {
            spCall = *callIterator;
            break;
        }
    }
    if(spCall) {
        spCall->modify(static_cast<telux::tel::RttMode>(mode), myModifyCb_);
    } else {
        std::cout << "No call found with given index/slot" << std::endl;
    }
}

int RttMenu::getInputPhoneId() {
   int phoneId = DEFAULT_PHONE_ID;
   if(telux::common::DeviceConfig::isMultiSimSupported()) {
      std::string slotSelection;
      char delimiter = '\n';

      std::cout << "Enter the desired Phone ID / SIM slot: ";
      std::getline(std::cin, slotSelection, delimiter);

      if (!slotSelection.empty()) {
         try {
            phoneId = std::stoi(slotSelection);
            if (phoneId < MIN_SIM_SLOT_COUNT || phoneId > MAX_SIM_SLOT_COUNT ) {
               std::cout << "ERROR: Invalid slot entered" << std::endl;
               return INVALID_PHONE_ID;
            }
         } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
               << slotSelection << std::endl;
            return INVALID_PHONE_ID;
         }
      } else {
         std::cout << "Empty input, enter the correct slot" << std::endl;
         return INVALID_PHONE_ID;
      }
   }
   return phoneId;
}

void RttMenu::sendRttMessage(std::vector<std::string> userInput) {
   int phoneId = DEFAULT_PHONE_ID;
   std::string data;
   char delimiter = '\n';

   if(telux::common::DeviceConfig::isMultiSimSupported()) {
      std::string slotSelection;

      std::cout << "Enter the desired Phone ID / SIM slot: ";
      std::getline(std::cin, slotSelection, delimiter);

      if (!slotSelection.empty()) {
         try {
            phoneId = std::stoi(slotSelection);
            if (phoneId < MIN_SIM_SLOT_COUNT || phoneId > MAX_SIM_SLOT_COUNT ) {
               std::cout << "ERROR: Invalid slot entered" << std::endl;
               return;
            }
         } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
               << slotSelection << std::endl;
            return;
         }
      } else {
         std::cout << "Empty input, enter the correct slot" << std::endl;
         return;
      }
   }
   std::cout << "Enter RTT message: ";
   std::getline(std::cin, data, delimiter);
   telux::common::Status sendRttMessageStatus
        = callManager_->sendRtt(phoneId, data, MyRttMessageCallback::sendRttMessageResponse);
   if (sendRttMessageStatus == telux::common::Status::SUCCESS) {
        std::cout << "sendRtt is successful.\n";
   } else {
        std::cout << "sendRtt failed.\n";
   }
}
