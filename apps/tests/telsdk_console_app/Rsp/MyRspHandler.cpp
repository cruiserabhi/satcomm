/*
 *  Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
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
#include "MyRspHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MyRspCallback::onProfileListResponse(
        const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
        telux::common::ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        if (profiles.size() == 0) {
            PRINT_CB << "Profile List is empty  \n";
            return;
        } else {
            PRINT_CB << "Profile List: \n";
        }

        for (size_t index = 0; index < profiles.size(); index++) {
            std::cout << profiles[index]->toString() << std::endl;
            std::vector<uint8_t> icon = profiles[index]->getIcon();
            if (icon.size() > 0) {
                std::cout << "Icon data is present \n";
            } else {
                std::cout << "Icon data is not present \n";
            }
        }
    } else {
        PRINT_CB << "\n Request profile list failed, ErrorCode: " <<static_cast<int>(errorCode)
                 << " Description : " << Utils::getErrorCodeAsString(errorCode)<< std::endl;
    }
}

void MyRspCallback::onEidResponse(std::string eid, telux::common::ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Eid : " << eid <<std::endl;;
    } else {
        PRINT_CB << "Request Eid failed, ErrorCode: " <<static_cast<int>(errorCode)
                 << " Description : " << Utils::getErrorCodeAsString(errorCode)<< std::endl;
    }
}

void MyRspCallback::onResponseCallback(telux::common::ErrorCode error) {
    std::cout << std::endl;
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request failed with errorCode: " << static_cast<int>(error)
                 << " Description : " << Utils::getErrorCodeAsString(error)<< std::endl;
    } else {
        PRINT_CB << "Request processed successfully \n";
    }
}

void MyRspCallback::onServerAddressResponse(std::string smdpAddress,
        std::string smdsAddress, telux::common::ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "SM-DP+ Address : " << smdpAddress <<std::endl;
        PRINT_CB << "SMDS Address : " << smdsAddress <<std::endl;;
    } else {
        PRINT_CB << "Request Server Address failed, ErrorCode: " <<static_cast<int>(errorCode)
                 << " Description : " << Utils::getErrorCodeAsString(errorCode)<< std::endl;
    }
}

