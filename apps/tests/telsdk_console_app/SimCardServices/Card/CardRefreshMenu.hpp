/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CARD_REFRESH_MENU_HPP
#define CARD_REFRESH_MENU_HPP

#include "telux/tel/CardManager.hpp"
#include "telux/tel/CardDefines.hpp"
#include "console_app_framework/ConsoleApp.hpp"

class CardRefreshMenu : public ConsoleApp {
public:
    CardRefreshMenu(std::string appName, std::string cursor);
    ~CardRefreshMenu();
    bool init();

private:
    telux::tel::RefreshParams enterRefreshParams
        (std::vector<std::string> userInput);
    void configureRefreshVote(std::vector<std::string> userInput);
    void allowRefresh(std::vector<std::string> userInput);
    void refreshComplete(std::vector<std::string> userInput);
    void requestLastEvent(std::vector<std::string> userInput);
    void selectCardSlot(std::vector<std::string> userInput);
    std::shared_ptr<telux::tel::ICardListener> cardListener_;
    std::shared_ptr<telux::tel::ICardManager> cardManager_;
    int slot_ = DEFAULT_SLOT_ID;
    std::vector<std::shared_ptr<telux::tel::ICard>> cards_;
};

class CardRefreshResponseCallback {
public:
    static void RefreshLastEventResponseCb(
        telux::tel::RefreshStage stage, telux::tel::RefreshMode mode,
        std::vector<telux::tel::IccFile> efFiles, telux::tel::RefreshParams config,
        telux::common::ErrorCode error);
    static void commandResponse(telux::common::ErrorCode error);
};

#endif  // CARD_FILE_MENU_HPP

