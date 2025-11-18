/*
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef CARD_FILE_MENU_HPP
#define CARD_FILE_MENU_HPP

#include "telux/tel/CardManager.hpp"
#include "telux/tel/CardFileHandler.hpp"
#include "telux/tel/CardDefines.hpp"
#include "console_app_framework/ConsoleApp.hpp"

class CardFileMenu : public ConsoleApp {
public:
    CardFileMenu(std::string appName, std::string cursor);
    ~CardFileMenu();
    bool init();

private:
    void getSupportedApps(std::vector<std::string> userInput);
    void readEFLinearFixed(std::vector<std::string> userInput);
    void readEFLinearFixedAll(std::vector<std::string> userInput);
    void readEFTransparent(std::vector<std::string> userInput);
    void writeEFLinearFixed(std::vector<std::string> userInput);
    void writeEFTransparent(std::vector<std::string> userInput);
    void requestEFAttributes(std::vector<std::string> userInput);
    void selectCardSlot(std::vector<std::string> userInput);
    std::string cardStateToString(telux::tel::CardState state);
    std::string appTypeToString(telux::tel::AppType appType);
    std::string appStateToString(telux::tel::AppState appState);
    std::shared_ptr<telux::tel::ICardListener> cardListener_;
    std::shared_ptr<telux::tel::ICardManager> cardManager_;
    int slot_ = DEFAULT_SLOT_ID;
    std::vector<std::shared_ptr<telux::tel::ICard>> cards_;
};

class CardFileHandlerResponseCallback {
public:
    static void EfReadLinearFixedResponseCb(telux::common::ErrorCode error,
        telux::tel::IccResult result);
    static void EfReadAllRecordsResponseCb(telux::common::ErrorCode error,
        std::vector<telux::tel::IccResult> records);
    static void EfReadTransparentResponseCb(telux::common::ErrorCode error,
       telux::tel::IccResult result);
    static void EfWriteLinearFixedResponseCb(telux::common::ErrorCode error,
        telux::tel::IccResult result);
    static void EfWriteTransparentResponseCb(telux::common::ErrorCode error,
        telux::tel::IccResult result);
    static void EfGetFileAttributesCb(telux::common::ErrorCode error, telux::tel::IccResult result,
        telux::tel::FileAttributes attributes);
};

#endif  // CARD_FILE_MENU_HPP

