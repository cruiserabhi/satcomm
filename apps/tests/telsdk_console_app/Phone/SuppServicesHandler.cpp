/*
 *  Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER`
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>

#include "SuppServicesHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

using namespace telux::tel;
using namespace telux::common;

std::string SuppServicesHelper::suppServicesStatustoString(
    telux::tel::SuppServicesStatus suppSvcStatus) {
    if (suppSvcStatus == SuppServicesStatus::ENABLED) {
        return "ENABLED";
    } else if (suppSvcStatus == SuppServicesStatus::DISABLED) {
        return "DISABLED";
    } else {
        return "UNKNOWN";
    }
}

std::string SuppServicesHelper::SuppSvcProvisionStatustoString(
    telux::tel::SuppSvcProvisionStatus provisionStatus) {
    std::string status;
    switch(provisionStatus) {
        case SuppSvcProvisionStatus::PROVISIONED:
            status = "PROVISIONED";
            break;
        case SuppSvcProvisionStatus::NOT_PROVISIONED:
            status = "NOT_PROVISIONED";
            break;
        case SuppSvcProvisionStatus::PRESENTATION_RESTRICTED:
            status = "PRESENTATION_RESTRICTED";
            break;
        case SuppSvcProvisionStatus::PRESENTATION_ALLOWED:
            status = "PRESENTATION_ALLOWED";
            break;
        default:
            status = "UNKNOWN";
            break;
    }
    return status;
}

void SetSuppSvcResponseCallback::setSuppSvcResp(ErrorCode error, FailureCause failureCause) {
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << " Set Supplementary Service :"
            << Utils::getErrorCodeAsString(error) << std::endl;
    } else {
        PRINT_CB << "Set Supplementary Service Failed with, ErrorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void GetSuppSvcResponseCallback::getCallWaitingPrefResp(SuppServicesStatus suppSvcStatus,
    FailureCause failureCause, telux::common::ErrorCode error) {
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Get Call Waiting Pref : "
            << Utils::getErrorCodeAsString(error) << std::endl;
        PRINT_CB << "Call Waiting Status : " <<
            SuppServicesHelper::suppServicesStatustoString(suppSvcStatus) << std::endl;
    } else {
        PRINT_CB << " get Call waiting pref failed with ErrorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << " Failure Cause : "
            << static_cast<int>(failureCause) << std::endl;
    }
}

void GetSuppSvcResponseCallback::getForwardingPrefResp(std::vector<ForwardInfo> forwardInfoList,
    FailureCause failureCause, telux::common::ErrorCode error) {
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Get Forwarding pref : "<< Utils::getErrorCodeAsString(error) << std::endl;
        for (auto &forwardInfo : forwardInfoList) {
            PRINT_CB <<  SuppServicesHelper::suppServicesStatustoString(forwardInfo.status)
                << std::endl;
            PRINT_CB << "Number to which forwarded : " << forwardInfo.number << std::endl;
        }
    } else {
        PRINT_CB << "get forwarding pref failed with ErrorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << " Failure Cause : " <<
            static_cast<int>(failureCause) << std::endl;
    }
}

void GetSuppSvcResponseCallback::getOirStatusResp(SuppServicesStatus suppSvcStatus ,
    SuppSvcProvisionStatus provisionStatus, FailureCause failureCause,
    telux::common::ErrorCode error) {
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Get Call Identification Restriction Pref : "
            << Utils::getErrorCodeAsString(error) << std::endl;
        PRINT_CB << "Call Identification Restriction Provision Status : " <<
            SuppServicesHelper::SuppSvcProvisionStatustoString(provisionStatus) << std::endl;
        PRINT_CB << "Call Identification Restriction Status : " <<
            SuppServicesHelper::suppServicesStatustoString(suppSvcStatus) << std::endl;
    } else {
        PRINT_CB << "Get Call Identification Restriction failed with ErrorCode: "
            << static_cast<int>(error) << ", description: " << Utils::getErrorCodeAsString(error)
            << " Failure Cause : " << static_cast<int>(failureCause) << std::endl;
    }
}

// Notify SuppServicesManager subsystem status
void MySuppServicesListener::onServiceStatusChange(telux::common::ServiceStatus status) {
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
    PRINT_NOTIFICATION << " SuppServices onServiceStatusChange" << stat << "\n";
}
