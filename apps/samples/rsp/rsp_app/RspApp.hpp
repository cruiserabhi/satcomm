/*
 *  Copyright (c) 2021,2024 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * @file      RspApp.hpp
 * @brief     The reference application to demonstrate how to use the Remote SIM Provisioning API
 *            for performing SIM profile management operations on the eUICC such as add profile,
 *            enable/disable profile, delete profile, query profile list, configure server address
 *            and perform memory reset.
 */

#ifndef RSPAPP_HPP
#define RSPAPP_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SimProfileManager.hpp>
#include <telux/tel/CardManager.hpp>

#include "RspListener.hpp"

using telux::common::Status;

class RemoteSimProfile {
 public:
    static RemoteSimProfile &getInstance();
    Status parseArguments(int arg, char *argv[]);
    bool init();
    void cleanup();

 private:
    SlotId slotId_;
    int profileId_;
    bool enableProfile_;
    bool userConsent_;
    int reason_;
    int resetOption_;
    // Profile activation code
    std::string activationCode_;
    std::string confirmationCode_;
    std::string nickname_;
    std::string smdpAddress_;
    std::shared_ptr<telux::tel::ISimProfileManager> simProfileManager_ = nullptr;
    std::shared_ptr<RspListener> rspListener_ = nullptr;
    std::shared_ptr<telux::tel::ICardManager> cardManager_= nullptr;
    std::vector<std::shared_ptr<telux::tel::ICard>> cards_;

    RemoteSimProfile();
    ~RemoteSimProfile();
    void printUsage(char **argv);

    // Wrapper function for request Profile list, add, delete, enable or update profile
    void requestProfileList();
    void addProfile(const std::string &actCode, const std::string &confCode,
                    bool isUserConsentRequired);
    void deleteProfile(int profileId);
    void setProfile(int profileId, bool enable);
    void updateNickName(int profileId, const std::string &nickname);
    void provideUserConsent(bool isUserConsentRequired, int reason);
    void provideUserConfirmation(std::string confirmationCode);
    void setServerAddress(std::string serverAddress);
    void getServerAddress();
    void memoryReset(int resetOption);
    void requestEid();

    // Response callbacks
    void onProfileListResponse(const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
        telux::common::ErrorCode errorCode);
    void onEidResponse(std::string eid, telux::common::ErrorCode errorCode);
    void serverAddressResponse(std::string smdpAddress,
        std::string smdsAddress, telux::common::ErrorCode error);
    void onResponseCallback(telux::common::ErrorCode error);
};

#endif
