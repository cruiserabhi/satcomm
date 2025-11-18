/*
 *  Copyright (c) 2020 The Linux Foundation. All rights reserved.
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

#include "iostream"
#include "MyMultiSimHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MyMultiSimCallback::requestHighCapabilityResponse(int slotId, telux::common::ErrorCode error) {
    if(error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Slot with high capability: " << slotId << std::endl;
    } else {
        PRINT_CB << "Request high capability request failed, errorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void MyMultiSimCallback::setHighCapabilityResponse(telux::common::ErrorCode error) {
    if(error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Set high capability request executed successfully" << std::endl;
    } else {
        PRINT_CB << "Set high capability request failed, errorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void MyMultiSimCallback::setActiveSlotResponse(telux::common::ErrorCode error) {
    if(error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Set active slot request executed successfully" << std::endl;
    } else {
        PRINT_CB << "Set active slot request failed, errorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void MyMultiSimCallback::requestsSlotsStatusResponse(std::map<SlotId,
        telux::tel::SlotStatus> slotStatus, telux::common::ErrorCode error) {
    if(error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Slots status request executed successfully" << std::endl;
        for(auto it = slotStatus.begin(); it != slotStatus.end(); ++it) {
            auto slotId = it->first;
            auto slotStatus = it->second;
            PRINT_CB << " SlotId: " << static_cast<int>(slotId)
                     << ", SlotState: " << MyMultiSimHelper::slotStateToString(slotStatus.slotState)
                     << ", CardState: " << MyMultiSimHelper::cardStateToString(slotStatus.cardState)
                     << ", CardError: " << MyMultiSimHelper::cardErrorToString(slotStatus.cardError)
                     << std::endl;
        }
    } else {
        PRINT_CB << "Slots status request failed, errorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

std::string MyMultiSimHelper::slotStateToString(telux::tel::SlotState slotState) {
   std::string slotStateString = "UNKNOWN";
   switch(slotState) {
    case telux::tel::SlotState::ACTIVE:
        slotStateString = "ACTIVE";
        break;
    case telux::tel::SlotState::INACTIVE:
        slotStateString = "INACTIVE";
        break;
    default:
        slotStateString = "UNKNOWN";
        break;
   }
   return slotStateString;
}

std::string MyMultiSimHelper::cardStateToString(telux::tel::CardState cardState) {
   std::string cardStateString = "UNKNOWN";
   switch(cardState) {
    case telux::tel::CardState::CARDSTATE_ABSENT:
        cardStateString = "ABSENT";
        break;
    case telux::tel::CardState::CARDSTATE_PRESENT:
        cardStateString = "PRESENT";
        break;
    case telux::tel::CardState::CARDSTATE_ERROR:
        cardStateString = "ERROR";
        break;
    case telux::tel::CardState::CARDSTATE_UNKNOWN:
        cardStateString = "UNKNOWN";
        break;
    default:
        cardStateString = "UNKNOWN";
        break;
   }
   return cardStateString;
}

std::string MyMultiSimHelper::cardErrorToString(telux::tel::CardError cardError) {
   std::string cardErrorString = "UNKNOWN";
   switch(cardError) {
    case telux::tel::CardError::POWER_DOWN:
        cardErrorString = "POWER_DOWN";
        break;
    case telux::tel::CardError::POLL_ERROR:
        cardErrorString = "POLL_ERROR";
        break;
    case telux::tel::CardError::NO_ATR_RECEIVED:
        cardErrorString = "NO_ATR_RECEIVED";
        break;
    case telux::tel::CardError::VOLT_MISMATCH:
        cardErrorString = "VOLT_MISMATCH";
        break;
    case telux::tel::CardError::PARITY_ERROR:
        cardErrorString = "PARITY_ERROR";
        break;
    case telux::tel::CardError::POSSIBLY_REMOVED:
        cardErrorString = "POSSIBLY_REMOVED";
        break;
    case telux::tel::CardError::TECHNICAL_PROBLEMS:
        cardErrorString = "TECHNICAL_PROBLEMS";
        break;
    case telux::tel::CardError::NULL_BYTES:
        cardErrorString = "NULL_BYTES";
        break;
    case telux::tel::CardError::SAP_CONNECTED:
        cardErrorString = "SAP_CONNECTED";
        break;
    case telux::tel::CardError::CMD_TIMEOUT:
        cardErrorString = "CMD_TIMEOUT";
        break;
    case telux::tel::CardError::UNKNOWN:
        cardErrorString = "UNKNOWN";
        break;
    default:
        cardErrorString = "UNKNOWN";
        break;
   }
   return cardErrorString;
}
