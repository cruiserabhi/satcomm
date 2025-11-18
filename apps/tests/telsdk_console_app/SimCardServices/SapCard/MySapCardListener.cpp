/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <iostream>

#include "MySapCardListener.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

/**
 *  Implementation of MySapCommandCallback
 */
void MySapCommandResponseCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " commandResponse successful." << std::endl;
   } else {
      PRINT_CB << "commandResponse failed\n error: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

/**
 *  Implementation of MyCardReaderCallback
 */
void MyCardReaderCallback::cardReaderResponse(telux::tel::CardReaderStatus readerStatus,
                                              telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;

   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " CardReaderStatus, id = " << readerStatus.id
               << "\n isRemovable = " << readerStatus.isRemovable
               << "\n isPresent = " << readerStatus.isPresent
               << "\n isID1size = " << readerStatus.isID1size
               << "\n isCardPresent = " << readerStatus.isCardPresent
               << "\n isCardPoweredOn = " << readerStatus.isCardPoweredOn << "\n";
   } else {
      PRINT_CB << "cardReaderResponse failed\n error: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

/**
 *  Implementation of MySapTransmitApduResponseCallback
 */
void MySapTransmitApduResponseCallback::onResponse(telux::tel::IccResult result,
                                                   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "onResponse successful" << std::endl;
      std::cout << " sw1: 0x" << std::hex << result.sw1 << "\n";
      std::cout << " sw2: 0x" << std::hex << result.sw2 << "\n";
      std::cout << " payload in Hex: " << result.payload << "\n\n";
      std::cout << " data: ";
      for(auto &i : result.data) {
         std::cout << std::dec << i << " ";
      }
      std::cout << std::endl << std::endl;
   } else {
      PRINT_CB << "onResponse failed\n error: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

/**
 *  Implementation of MyAtrResponseCallback
 */
void MyAtrResponseCallback::atrResponse(std::vector<int> responseAtr,
                                        telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      for(int val : responseAtr) {
         std::cout << " " << val;
      }
   } else {
      PRINT_CB << "atrResponse failed\n error: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
   std::cout << std::endl;
}

void MySapStateCallback::logSapState(telux::tel::SapState sapState) {
   if(sapState == telux::tel::SapState::SAP_STATE_NOT_ENABLED) {
      std::cout << "Sap state not enabled \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_CONNECTING) {
      std::cout << "Sap state connecting \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_CONNECTED_SUCCESSFULLY) {
      std::cout << "Sap state connected successfully \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_CONNECTION_ERROR) {
      std::cout << "Sap state connection error \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_DISCONNECTING) {
      std::cout << "Sap state disconnecting \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_DISCONNECTED_SUCCESSFULLY) {
      std::cout << "Sap state disconnected successfully \n";
   }
}

void MySapStateCallback::sapStateResponse(telux::tel::SapState sapState,
                                          telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "SAP state: " << (int)sapState << std::endl << std::endl;
      logSapState(sapState);
   } else {
      PRINT_CB << "sapConnectionStatusResponse failed\n error: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
   std::cout << std::endl;
}

// Notify SapCardManager subsystem status
void MySapCardListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::string stat = "";
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }
    PRINT_NOTIFICATION << " Sap Card onServiceStatusChange" << stat << "\n";
}
