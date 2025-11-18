/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATACONTROLMENU_HPP
#define DATACONTROLMENU_HPP

#include "console_app_framework/ConsoleApp.hpp"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <string>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataControlManager.hpp>

class DataControlMenu : public ConsoleApp,
                        public telux::data::IDataControlListener,
                        public std::enable_shared_from_this<DataControlMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // Initialization Callback
    void onInitComplete(telux::common::ServiceStatus status);

    // DataControl Manager APIs
    void setDataStallParams(std::vector<std::string> &inputCommand);

    DataControlMenu(std::string appName, std::string cursor);
    ~DataControlMenu();

 private:
    bool initDataControlManager();

    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<telux::data::IDataControlManager> dataControlManager_;
};

#endif //DATACONTROLMENU_HPP
