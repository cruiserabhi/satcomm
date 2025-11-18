/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a Data Link Manager Sample Application using Telematics SDK.
 * It is used to demonstrate API to exercise Data Link Features.
 */

#ifndef DATALINKMENU_HPP
#define DATALINKMENU_HPP

#include <telux/data/DataLinkManager.hpp>
#include "console_app_framework/ConsoleApp.hpp"

using namespace telux::data;
using namespace telux::common;

class DataLinkMenu : public ConsoleApp,
                     public std::enable_shared_from_this<DataLinkMenu> {
public:
    // initialize menu and sdk
    bool init();

    // Menu Functions
    DataLinkMenu(std::string appName, std::string cursor);

    //Initialization Callback
    void onInitCompleted(telux::common::ServiceStatus status);

    //API
    void getEthCapability(std::vector<std::string> inputCommand);
    void setPeerEthCapability(std::vector<std::string> inputCommand);
    void setLocalEthOperatingMode(std::vector<std::string> inputCommand);
    void setPeerModeChangeRequestStatus(std::vector<std::string> inputCommand);
    void setEthDataLink(std::vector<std::string> inputCommand);
    void registerListener(std::vector<std::string> inputCommand);
    void deregisterListener(std::vector<std::string> inputCommand);

    ~DataLinkMenu();
private:
    bool addMenuCmds_;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<IDataLinkManager> dataLinkManager_;
    std::shared_ptr<IDataLinkListener> dataLinkListener_;
    bool initDataLinkManagerAndListener();
};

#endif
