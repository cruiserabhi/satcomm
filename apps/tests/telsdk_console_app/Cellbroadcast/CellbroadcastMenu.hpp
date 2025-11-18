/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

/*  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file      CellbroadcastMenu.hpp
 * @brief     The reference application to demonstrate Cellbroadcast features
 *            like update message filters, request message filters, set and get
 *            activation status and receive cell broadcast message
 */

#ifndef CELLBROADCASTMENU_HPP
#define CELLBROADCASTMENU_HPP

#include <memory>
#include <string>
#include <vector>

#include <telux/tel/CellBroadcastManager.hpp>
#include "CellbroadcastListener.hpp"
#include "console_app_framework/ConsoleApp.hpp"

class CellbroadcastMenu : public ConsoleApp {
public:
    CellbroadcastMenu(std::string appName, std::string cursor);
    ~CellbroadcastMenu();
    bool init();

private:
    std::vector<std::shared_ptr<telux::tel::ICellBroadcastManager>> cbManagers_;
    std::shared_ptr<CellbroadcastListener> cbListener_ = nullptr;
    void updateMessageFilters(std::vector<std::string> userInput);
    void setActivationStatus(std::vector<std::string> userInput);
    void requestMessageFilters(std::vector<std::string> userInput);
    void requestActivationStatus(std::vector<std::string> userInput);
    void selectSimSlot(std::vector<std::string> userInput);
    int slot_ = DEFAULT_SLOT_ID;

    void onRequestMsgFilterResponse(std::vector<telux::tel::CellBroadcastFilter> filters,
        bool isActivated, telux::common::ErrorCode errorCode);
    void onResponseCallback(telux::common::ErrorCode error);

};

#endif  // CELLBROADCASTMENU_HPP

