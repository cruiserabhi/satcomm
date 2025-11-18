/*
 *  Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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

#include <iostream>
#include <memory>
#include <chrono>
#include <cmath>
#include <sstream>
#include <bitset>

extern "C" {
#include <sys/time.h>
}

#include "MySmsListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

const std::string keyword = "location";
const std::string keyword2 = "Location";

const std::string GREEN = "\033[0;32m";
const std::string RED = "\033[0;31m";
const std::string BOLD_RED = "\033[1;31m";
const std::string DONE = "\033[0m";  // No color

void MySmsListener::onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> smsMsg) {
   PRINT_NOTIFICATION << std::endl
                      << "Received SMS from " << smsMsg->getSender()
                      << ", Message: " << smsMsg->getText() << std::endl;
   std::string senderNumber = smsMsg->getSender();

   std::string::size_type kwPos = smsMsg->toString().find(keyword);
   std::string::size_type kwPos2 = smsMsg->toString().find(keyword2);
   std::string::size_type kwToken = smsMsg->toString().find(std::to_string(token_));
   if(kwPos != std::string::npos || kwPos2 != std::string::npos) {
      if(kwToken != std::string::npos) {
         std::cout << std::endl << "SMS Token matched" << std::endl;
         std::cout << "Sending updated location to the sender " << std::endl;
         myLocationListener_->setRequestReceived(true, senderNumber);
         setSecureToken();
      } else {
         std::cout << std::endl
                   << "SMS Token mismatch, Token to be sent is: " << token_ << std::endl;
      }
   }
}

MySmsListener::MySmsListener() {
   setSecureToken();
}

void MySmsListener::setLocationListener(std::shared_ptr<MyLocationListener> myLocationListener) {
   myLocationListener_ = myLocationListener;
}

void MySmsListener::setSecureToken() {
   srand((unsigned)time(0));
   token_ = (rand() % 9000) + 1000;  // Generate 4 Digit random number
   std::cout << std::endl << "New SMS Token: " << GREEN << token_ << DONE << std::endl << std::endl;
}

void SmsCallback::commandResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << "onSmsSent successfully" << std::endl;
   } else {
      std::cout << "onSmsSent failed" << std::endl;
   }
   std::cout << "onSmsSent error = " << static_cast<int>(error) << std::endl;
}
