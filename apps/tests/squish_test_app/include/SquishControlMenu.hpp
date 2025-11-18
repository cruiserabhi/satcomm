/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef SASQUISHCONTROLMENU_HPP
#define SASQUISHCONTROLMENU_HPP
#include "SasquishUtils.hpp"
using namespace telux;
using namespace telux::cv2x::prop;
using namespace telux::common;
class SquishControlMenu {
public:
    SquishControlMenu(
        std::string appName, std::string cursor, SasquishArguments commandLineArgs);
    ~SquishControlMenu();
    telux::common::ServiceStatus init(bool shouldInitConsole);
    void cleanup();
    void parseArgs(int argc, char **argv);
    std::shared_ptr<ICongestionControlManager> getCongCtrlManager();

private:
    shared_ptr<ICongestionControlManager> congestionControlManager_;
    std::shared_ptr<ICongestionControlListener> squishCongCtrlListener_;
};


#endif  // SASQUISHCONTROLMENU_HPP