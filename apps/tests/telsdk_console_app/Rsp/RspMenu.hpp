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
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021, 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file      RspMenu.hpp
 * @brief     The reference application to demonstrate Remote SIM Provisioning features
 *            like addProfile, deleteProfile, setProfile, requestProfileList, update nickname,
 *            provide user consent, request Eid.
 */

#ifndef RSPMENU_HPP
#define RSPMENU_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SimProfileManager.hpp>
#include <telux/tel/CardManager.hpp>

#include "RspListener.hpp"
#include "console_app_framework/ConsoleApp.hpp"

using telux::common::Status;

class RemoteSimProfileMenu : public ConsoleApp {
 public:
    RemoteSimProfileMenu(std::string appName, std::string cursor);
    ~RemoteSimProfileMenu();
    bool init();

 private:
    std::shared_ptr<telux::tel::ISimProfileManager> simProfileManager_ = nullptr;
    std::shared_ptr<RspListener> rspListener_ = nullptr;
    std::shared_ptr<telux::tel::ICardManager> cardManager_ = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICard>> cards_;

    // Wrapper function for request Profile list, add, delete, enable or update profile
    void requestProfileList(std::vector<std::string> userInput);
    void addProfile(std::vector<std::string> userInput);
    void deleteProfile(std::vector<std::string> userInput);
    void setProfile(std::vector<std::string> userInput);
    void updateNickName(std::vector<std::string> userInput);
    void requestEid(std::vector<std::string> userInput);
    void provideUserConsent(std::vector<std::string> userInput);
    void setServerAddress(std::vector<std::string> userInput);
    void requestServerAddress(std::vector<std::string> userInput);
    void provideConfirmationCode(std::vector<std::string> userInput);
    void memoryReset(std::vector<std::string> userInput);
    void onResponseCallback(telux::common::ErrorCode error);

    SlotId getSlotIdInput();
};

#endif
