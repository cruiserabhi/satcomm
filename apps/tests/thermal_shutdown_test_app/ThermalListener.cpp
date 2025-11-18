/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <memory>

#include "ThermalListener.hpp"
#include "ThermalShutdownMgrTestApp.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

ThermalListener::ThermalListener(std::weak_ptr<ThermalCommandMgr> MyThermCmdMgr) {
        myThermCmdMgr_ = MyThermCmdMgr;
}

ThermalListener::~ThermalListener() {
}

void ThermalListener::printAutoShutdownMode(AutoShutdownMode mode) {

    if(mode == AutoShutdownMode::ENABLE) {
        PRINT_NOTIFICATION << "Auto shutdown mode : ENABLE" << std::endl;
    } else if(mode == AutoShutdownMode::DISABLE) {
        PRINT_NOTIFICATION << "Auto shutdown mode : DISABLE" << std::endl;
    } else if(mode == AutoShutdownMode::UNKNOWN) {
        PRINT_NOTIFICATION << "Auto shutdown mode : UNKNOWN" << std::endl;
    } else {
        std::cout << APP_NAME << " ERROR: Invalid Auto shutdown mode notified" << std::endl;
    }
}

void ThermalListener::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if(status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : UNAVAILABLE" << std::endl;
    } else if(status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : AVAILABLE" << std::endl;
        std::lock_guard<std::mutex> lock(listenerMtx_);
        taskCompleted_ = true;
        std::cout << " Notifying To CommandManager" << std::endl;
        listenerCv_.notify_all();
    }
}

void ThermalListener::onShutdownEnabled() {
    std::cout << std::endl;
    printAutoShutdownMode(AutoShutdownMode::ENABLE);
    std::lock_guard<std::mutex> lock(listenerMtx_);
    taskCompleted_ = true;
    std::cout << " Notifying To CommandManager" << std::endl;
    listenerCv_.notify_all();
}

void ThermalListener::onShutdownDisabled() {
    std::cout << std::endl;
    printAutoShutdownMode(AutoShutdownMode::DISABLE);
    std::lock_guard<std::mutex> lock(listenerMtx_);
    taskCompleted_ = true;
    std::cout << " Notifying To CommandManager" << std::endl;
    listenerCv_.notify_all();
}

void ThermalListener::onImminentShutdownEnablement(uint32_t imminentDuration) {
    std::cout << std::endl;
    PRINT_NOTIFICATION << "Auto shutdown will be enabled in " <<
        imminentDuration << " seconds" << std::endl;
    auto sp = myThermCmdMgr_.lock();
    if(sp && sp->getAutoDisableFlag()) {
        sp->sendAutoShutdownModeCommand(telux::therm::AutoShutdownMode::DISABLE);
    }
}
