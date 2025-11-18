/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file      ApSimProfileMenu.hpp
 * @brief     The reference application to demonstrate the modem to interface with a Local Profile
 *            Assistant(LPA) running on the application processor(AP) features like retrieve
 *            profile details and enable or disable profile on the eUICC.
 */

#ifndef APSIMPROFILEMENU_HPP
#define APSIMPROFILEMENU_HPP

#include "console_app_framework/ConsoleApp.hpp"
#include "ApSimProfileClient.hpp"

using telux::common::Status;

class ApSimProfileMenu : public ConsoleApp {
 public:
     bool init();
     void cleanup();
     ApSimProfileMenu(std::string appName, std::string cursor);
     ~ApSimProfileMenu();

 private:
     void requestProfileList();
     void enableProfile();
     void disableProfile();

    std::shared_ptr<ApSimProfileClient> apSimProfileClient_ = nullptr;
};

#endif
