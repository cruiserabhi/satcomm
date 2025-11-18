/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef COLLECTIONMETHOD_HPP
#define COLLECTIONMETHOD_HPP

#include <telux/platform/diag/DiagLogManager.hpp>

#include "common/console_app_framework/ConsoleApp.hpp"

#include "UserInput.hpp"

class CollectionMethod : public ConsoleApp {
 public:
    CollectionMethod(std::string menuTitle, std::string cursor,
        std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr);
    ~CollectionMethod();

 protected:
    UserInput usrInput;
    std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr_;

    void setConfig(telux::platform::diag::LogMethod collectionMethod);
    void getConfig();
    void startCollection();
    void stopCollection();
    void getServiceStatus();
};

#endif // COLLECTIONMETHOD_HPP
