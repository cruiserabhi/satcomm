/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef PARAMCONFIGLISTENER_HPP
#define PARAMCONFIGLISTENER_HPP

#include <iostream>
#include <string>

#include <telux/config/ConfigManager.hpp>

using namespace telux::config;
using namespace telux::common;

class ConfigListener : public IConfigListener {
  public:
    void onConfigUpdate(std::string key, std::string value) override;
    void onServiceStatusChange(ServiceStatus status) override;
    ~ConfigListener() {}
};

#endif  //PARAMCONFIGLISTENER_HPP
