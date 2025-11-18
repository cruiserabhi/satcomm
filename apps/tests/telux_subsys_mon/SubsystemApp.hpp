/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SUBSYSTEMAPP_HPP
#define SUBSYSTEMAPP_HPP

#include <telux/platform/SubsystemFactory.hpp>
#include <telux/platform/SubsystemManager.hpp>

#include "common/console_app_framework/ConsoleApp.hpp"

#include "UserUtils.hpp"

class StateChangeListener : public telux::platform::ISubsystemListener {

 public:
    void onStateChange(telux::common::SubsystemInfo subsystemInfo,
        telux::common::OperationalStatus newOperationalStatus) override;
};

class SubsystemApp : public ConsoleApp {

 public:
    SubsystemApp(std::string appName, std::string cursor);
    ~SubsystemApp();

    void init(void);
    void registerListener(void);
    void deRegisterListener(void);

 private:
    UserUtils userUtils_;
    std::shared_ptr<StateChangeListener> stateChangeListener_;
    std::shared_ptr<telux::platform::ISubsystemManager> subsystemMgr_;

    void getSubsystemsToMonitor(std::vector<telux::common::SubsystemInfo> &listOfSubsystems);
};

#endif // SUBSYSTEMAPP_HPP
