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

#include "CellbroadcastHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void CellbroadcastCallbackHandler::requestMsgFilterResponse(
    std::vector<telux::tel::CellBroadcastFilter> filters,
    telux::common::ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request for get message filters executed successfully" << std::endl;
        for (int index = 0; index < filters.size(); index++) {
            PRINT_CB << "Filter: " << index + 1 << ", StartMsgId: " <<
            filters[index].startMessageId << ", EndMsgId: " <<
            filters[index].endMessageId << std::endl;
        }
    } else {
        PRINT_CB << "Request for message filters failed, ErrorCode: " <<static_cast<int>(errorCode)
                 << " description : " << Utils::getErrorCodeAsString(errorCode)<< std::endl;
    }
}

void CellbroadcastCallbackHandler::updateMsgFilterResponse(telux::common::ErrorCode error) {
    if(error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Update message filter request executed successfully" << std::endl;
    } else {
        PRINT_CB << "Update message filter request failed, errorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void CellbroadcastCallbackHandler::setActivationStatusResponse(telux::common::ErrorCode error) {
    if(error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Set Activation status request executed successfully" << std::endl;
    } else {
        PRINT_CB << "Set Activation status request failed, errorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void CellbroadcastCallbackHandler::requestActivationStatusResponse(bool isActivated,
    telux::common::ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request for get activation status executed successfully" << std::endl;
        PRINT_CB << "isActivated : " << isActivated <<std::endl;
    } else {
        PRINT_CB << "Request for activation status failed, ErrorCode: " <<static_cast<int>(errorCode)
                 << " description : " << Utils::getErrorCodeAsString(errorCode)<< std::endl;
    }
}
