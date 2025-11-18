/*
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <iostream>
#include <string>

#include "RspListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void RspListener::onDownloadStatus(SlotId slotId, telux::tel::DownloadStatus status,
    telux::tel::DownloadErrorCause cause) {

    PRINT_NOTIFICATION << " Profile Download Status: " << profileDownloadStatusToString(status)
                       << std::endl;
    PRINT_NOTIFICATION << " Slot Id: " << static_cast<int>(slotId) << std::endl;
    PRINT_NOTIFICATION << " Profile Download Error Cause: "
        << profileDownloadErrorCauseToString(cause) << std::endl;
}

void RspListener::onUserDisplayInfo(SlotId slotId, bool userConsentRequired,
    telux::tel::PolicyRuleMask mask) {

    PRINT_NOTIFICATION << " Is User Consent Required: " << userConsentRequired
                       << std::endl;
    PRINT_NOTIFICATION << " Slot Id: " << static_cast<int>(slotId) << std::endl;
    std::string policyRule = pprMaskToString(mask);
    PRINT_NOTIFICATION << " Profile Policy Rule: " << policyRule << std::endl;
}

void RspListener::onConfirmationCodeRequired(SlotId slotId, std::string profileName) {

    PRINT_NOTIFICATION << " Confirmation Code Required" << std::endl;
    PRINT_NOTIFICATION << " Slot Id: " << static_cast<int>(slotId) << std::endl;
    PRINT_NOTIFICATION << " Profile Name: " << profileName
                       << std::endl;
}

std::string RspListener::profileDownloadStatusToString(telux::tel::DownloadStatus status) {
    std::string downloadStatus;
    switch (status) {
        case telux::tel::DownloadStatus::DOWNLOAD_ERROR:
            downloadStatus = "DOWNLOAD ERROR";
            break;
        case telux::tel::DownloadStatus::DOWNLOAD_INSTALLATION_COMPLETE:
            downloadStatus = "DOWNLOAD INSTALLATION COMPLETE";
            break;
        default:
            downloadStatus = "UNKNOWN";
            break;
    }
    return downloadStatus;
}

std::string RspListener::profileDownloadErrorCauseToString(telux::tel::DownloadErrorCause cause) {
    std::string errorCause;
    switch (cause) {
        case telux::tel::DownloadErrorCause::GENERIC:
            errorCause = "GENERIC";
            break;
        case telux::tel::DownloadErrorCause::SIM:
            errorCause = "SIM";
            break;
        case telux::tel::DownloadErrorCause::NETWORK:
            errorCause = "NETWORK";
            break;
        case telux::tel::DownloadErrorCause::MEMORY:
            errorCause = "MEMORY";
            break;
        case telux::tel::DownloadErrorCause::UNSUPPORTED_PROFILE_CLASS:
            errorCause = "UNSUPPORTED PROFILE CLASS";
            break;
        case telux::tel::DownloadErrorCause::PPR_NOT_ALLOWED:
            errorCause = "PPR NOT ALLOWED";
            break;
        case telux::tel::DownloadErrorCause::END_USER_REJECTION:
            errorCause = "END USER REJECTION";
            break;
        case telux::tel::DownloadErrorCause::END_USER_POSTPONED:
            errorCause = "END USER POSTPONED";
            break;
        default:
            errorCause = "UNKNOWN";
            break;
    }
    return errorCause;
}

std::string RspListener::pprMaskToString(telux::tel::PolicyRuleMask mask) {
    std::string ppr = "";
    bool pprAvailable = false;
    if (mask[static_cast<int>(
        telux::tel::PolicyRuleType::PROFILE_DISABLE_NOT_ALLOWED)]) {
        ppr = "Profile disable not allowed. ";
        pprAvailable = true;
    }

    if (mask[static_cast<int>(
        telux::tel::PolicyRuleType::PROFILE_DELETE_NOT_ALLOWED)]) {
        ppr = ppr + "Profile delete not allowed. ";
        pprAvailable = true;
    }

    if (mask[static_cast<int>(
        telux::tel::PolicyRuleType::PROFILE_DELETE_ON_DISABLE)]) {
        ppr = ppr + "Profile delete on disable. ";
        pprAvailable = true;
    }

    if (!pprAvailable) {
        ppr = "UNKNOWN";
    }
    return ppr;
}
