/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DIAGODLAPP_HPP
#define DIAGODLAPP_HPP

#include <telux/platform/diag/DiagLogManager.hpp>

#include "common/console_app_framework/ConsoleApp.hpp"

class DiagODLApp : public ConsoleApp {
 public:
    DiagODLApp(std::string appName, std::string cursor);
    ~DiagODLApp();

    int init();
    void showMainMenu();

 private:
    std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr_;
    void fileMethodMenu();
    void callbackMethodMenu();

};

#endif // DIAGODLAPP_HPP
