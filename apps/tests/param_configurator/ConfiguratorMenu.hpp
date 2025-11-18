/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef PARAMCONFIGMENU_HPP
#define PARAMCONFIGMENU_HPP

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include <telux/config/ConfigManager.hpp>

#include "../../common/console_app_framework/ConsoleApp.hpp"

#include "ConfiguratorListener.hpp"

using namespace telux::config;
using namespace telux::common;

class ConfigMenu : public ConsoleApp {
  public:
    ConfigMenu(std::string appName, std::string cursor);
    ~ConfigMenu();

    int init();

    void getAllConfigs(std::vector<std::string> userInput);
    void setConfig(std::vector<std::string> userInput);
    void getConfig(std::vector<std::string> userInput);

    telux::common::Status initConfigManager(std::shared_ptr<IConfigManager> &ConfigManager);

  private:
    std::shared_ptr<IConfigManager> configManager_ = nullptr;
    std::shared_ptr<ConfigListener> configListener_ = nullptr;
};

#endif  // PARAMCONFIGMENU_HPP
