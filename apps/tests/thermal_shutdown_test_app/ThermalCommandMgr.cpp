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
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <sstream>

#include "ThermalCommandMgr.hpp"
#include "Utils.hpp"
#include "ThermalShutdownMgrTestApp.hpp"

ThermalCommandMgr::ThermalCommandMgr() {
}

ThermalCommandMgr::~ThermalCommandMgr() {
    thermShutdownMgr_ = nullptr;
}

int ThermalCommandMgr::init() {
    // Get thermal factory instance
    auto &thermalFactory = ThermalFactory::getInstance();
    // Get thermal shutdown manager object
    std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
    thermShutdownMgr_ = thermalFactory.getThermalShutdownManager(
                            [&](telux::common::ServiceStatus status) {
                                 prom.set_value(status);
                            });
    if (thermShutdownMgr_ == NULL) {
        std::cout << APP_NAME << " *** ERROR - Failed to get manager instance" << std::endl;
        return -1;
    }
    // wait for thermal shutdown manager to be ready
    std::cout << " Waiting for Thermal Shutdown Manager to be ready " << std::endl;
    telux::common::ServiceStatus serviceStatus = prom.get_future().get();
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << APP_NAME << " Thermal-Shutdown management services are ready !" << std::endl;
    } else {
        std::cout << APP_NAME << " *** ERROR - Unable to initialize Thermal-Shutdown management "
                                 "services" << std::endl;
        return -1;
    }
    myThermListener_ = std::make_shared<ThermalListener>(shared_from_this());
    setAutoDisableFlag(false);
    return 0;
}

void ThermalCommandMgr::waitForAsyncTaskToComplete() {
    {
        std::unique_lock<std::mutex> lock(myThermListener_->listenerMtx_);
        myThermListener_->listenerCv_.wait(lock, [this]() {
                return myThermListener_->taskCompleted_; });
    }
    std::lock_guard<std::mutex> lock(mgrMtx_);
    taskCompleted_ = true;
    std::cout << " AsyncTask is completed " << std::endl;
    mgrCv_.notify_all();
}

void ThermalCommandMgr::registerForUpdates() {
    // Registering a listener for auto-shutdown mode updates
    telux::common::Status status = thermShutdownMgr_->registerListener(myThermListener_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " *** ERROR - Failed to register for auto-shutdown mode events"
                << std::endl;
    } else {
        std::cout << APP_NAME << " Registered Listener for auto-shutdown mode events" << std::endl;
    }
}

void ThermalCommandMgr::deregisterForUpdates() {
    // De-registering a listener for auto-shutdown mode updates
    telux::common::Status status = thermShutdownMgr_->deregisterListener(myThermListener_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " *** ERROR - Failed to de-register for auto-shutdown mode events"
                << std::endl;
    } else {
        std::cout << APP_NAME << " De-registered listener" << std::endl;
    }
}

void ThermalCommandMgr::sendAutoShutdownModeCommand(AutoShutdownMode state) {
    std::string cmd, userInput;
    int timeout = DEFAULT_TIMEOUT;
    bool defaultTimeout = false;
    telux::common::Status status;
    if (state == AutoShutdownMode::ENABLE) {
        cmd = "Enable Command: ";
        setAutoDisableFlag(false);
        std::cout << APP_NAME << " Sending ENABLE command" << std::endl;
    } else if (state == AutoShutdownMode::DISABLE) {
        cmd = "Disable Command: ";
        if (getAutoDisableFlag()) {
            defaultTimeout = true;
        } else {
            std::cout << "Enter petting timeout (in seconds) or -1 for default timeout( "
                    << DEFAULT_TIMEOUT <<" seconds) ";
            if (std::getline(std::cin, userInput)) {
                std::stringstream inputStream(userInput);
                if (!(inputStream >> timeout)) {
                   std::cout << "Invalid input!" << std::endl;
                   return;
                }
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        }
    std::cout << APP_NAME << " Sending DISABLE command" << std::endl;
    }
    defaultTimeout = (timeout < 0) ? true : false ;
    cmdRspCb_= std::make_shared<ThermalCommandCallback>(cmd);
    auto RspCb = std::bind(&ThermalCommandCallback::commandResponse, cmdRspCb_,
            std::placeholders::_1);
    if (defaultTimeout) {
        status = thermShutdownMgr_->setAutoShutdownMode(state, RspCb);
    } else {
        status = thermShutdownMgr_->setAutoShutdownMode(state, RspCb, timeout);
    }
    if (status == telux::common::Status::NOTREADY) {
        std::cout << APP_NAME << "Thermal Shutdown Mangement service is UNAVAILABLE" << std::endl;
    } else if (status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME <<
            " *** ERROR - Failed to send set auto-shutdown mode command" << std::endl;
    }
}

void ThermalCommandMgr::sendRecurringDisableCommand() {
    setAutoDisableFlag(true);
    sendAutoShutdownModeCommand(AutoShutdownMode::DISABLE);
}

std::future<bool> ThermalCommandMgr::getAutoShutdownModeCommand() {
    cmdRspCb_= std::make_shared<ThermalCommandCallback>("Get status: ");
    telux::common::Status status =
       thermShutdownMgr_->getAutoShutdownMode(
          std::bind(&ThermalCommandCallback::getCmdResponse, cmdRspCb_, std::placeholders::_1));

    if (status == telux::common::Status::NOTREADY) {
        std::cout << APP_NAME << "Thermal Shutdown Mangement service is UNAVAILABLE" << std::endl;
    } else if (status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME <<
            " *** ERROR - Failed to send get auto-shutdown mode command" << std::endl;
    }
    return cmdRspCb_->getPromValue();
}

void ThermalCommandMgr::setAutoDisableFlag(bool disable) {
   autoDisable_ = disable;
}

bool ThermalCommandMgr::getAutoDisableFlag() {
  return autoDisable_;
}
