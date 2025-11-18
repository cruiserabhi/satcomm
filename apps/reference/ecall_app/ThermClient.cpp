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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @file    ThermClient.cpp
 *
 * @brief   ThermClient class provides methods to DISABLE the auto-shutdown mode when an eCall is
 *          initiated and then enable it when eCall is cleared down. It manages the thermal
 *          subsystem using Telematics-SDK APIs.
 */

#include <iostream>

#include "ThermClient.hpp"

#define CLIENT_NAME "ECall-Thermal-Client: "

ThermClient::ThermClient()
    : shutdownAllowed_(true)
    , thermShutdownMgr_(nullptr) {
}

ThermClient::~ThermClient() {
    setShutdownAllowedState(true);
}

// Callback invoked when thermal auto-shutdown is enabled
void ThermClient::onShutdownEnabled() {
    std::cout << CLIENT_NAME << "Thermal auto-shutdown enabled" << std::endl;
    if(!isShutdownAllowed()) {
        sendAutoShutdownModeCommand(AutoShutdownMode::DISABLE);
    }
}

// Callback invoked when thermal auto-shutdown is disabled
void ThermClient::onShutdownDisabled() {
    std::cout << CLIENT_NAME << "Thermal auto-shutdown disabled" << std::endl;
}

// Callback invoked when thermal auto-shutdown is about to be enabled
void ThermClient::onImminentShutdownEnablement(uint32_t imminentDuration) {
    std::cout << CLIENT_NAME << "Thermal auto-shutdown will be enabled in " << imminentDuration
                << " seconds" << std::endl;
    if(!isShutdownAllowed()) {
        sendAutoShutdownModeCommand(AutoShutdownMode::DISABLE);
    }
}

// Callback invoked when thermal auto-shutdown service status changes
void ThermClient::onServiceStatusChange(ServiceStatus status) {
    if(status == ServiceStatus::SERVICE_UNAVAILABLE) {
        std::cout << CLIENT_NAME << "Thermal shutdown manager service is UNAVAILABLE" << std::endl;
    } else if(status == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << CLIENT_NAME << "Thermal shutdown manager service is AVAILABLE" << std::endl;
    }
}

// Callback which provides response to setAutoShutdownMode
static void commandCallback(ErrorCode errorCode) {
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Command initiated successfully " << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Command failed!" << std::endl;
    }
}

// Callback which is invoked when Thermal Shutdown Manager initialization is processed(success or
// failure)
static void initCb(telux::common::ServiceStatus status) {
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << CLIENT_NAME << " Thermal Shutdown Manager is initialized successfully "
            << std::endl;
    } else if(status == telux::common::ServiceStatus::SERVICE_FAILED) {
        std::cout << CLIENT_NAME << " Thermal Shutdown Manager initialization failed"
            << std::endl;
    }
}

// Initialize the thermal-shutdown management subsystem
telux::common::Status ThermClient::init() {
    // Get thermal factory instance
    auto &thermalFactory = ThermalFactory::getInstance();
    // Get thermal shutdown manager object
    thermShutdownMgr_ = thermalFactory.getThermalShutdownManager(&initCb);
    if(thermShutdownMgr_ == nullptr) {
        std::cout << CLIENT_NAME << "*** ERROR - Failed to get Thermal Shutdown manager instance"
                    << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Function to enable/disable thermal auto-shutdown
telux::common::Status ThermClient::sendAutoShutdownModeCommand(AutoShutdownMode state) {
    if(!thermShutdownMgr_) {
        std::cout << CLIENT_NAME << "Invalid Thermal shutdown Manager" << std::endl;
        return telux::common::Status::FAILED;
    } else if(ServiceStatus::SERVICE_AVAILABLE != thermShutdownMgr_->getServiceStatus()) {
        std::cout << CLIENT_NAME << " Thermal shutdown Manager is not yet ready" << std::endl;
        return telux::common::Status::NOTREADY;
    }
    if(state == AutoShutdownMode::ENABLE) {
        std::cout << CLIENT_NAME << "Enabling Thermal auto-shutdown mode" << std::endl;
    } else if(state == AutoShutdownMode::DISABLE) {
        std::cout << CLIENT_NAME << "Disabling Thermal auto-shutdown mode" << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Invalid Thermal auto-shutdown mode provided" << std::endl;
        return telux::common::Status::FAILED;
    }
    telux::common::Status status = thermShutdownMgr_->setAutoShutdownMode(state, &commandCallback);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "*** ERROR - Failed to send auto-shutdown mode command"
                    << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Function to register for auto-shutdown mode updates
telux::common::Status ThermClient::registerForUpdates() {
    if(!thermShutdownMgr_) {
        std::cout << CLIENT_NAME << "Invalid Thermal Shutdown Manager" << std::endl;
        return telux::common::Status::FAILED;
    }
    // Registering a listener for auto-shutdown mode updates
    telux::common::Status status = thermShutdownMgr_->registerListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "*** ERROR - Failed to register auto-shutdown mode events"
                    << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Registered Listener for auto-shutdown mode events"
            << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

// Function to de-register for auto-shutdown mode updates
telux::common::Status ThermClient::deregisterForUpdates() {
    if(!thermShutdownMgr_) {
        std::cout << CLIENT_NAME << "Invalid Thermal Shutdown Manager" << std::endl;
        return telux::common::Status::FAILED;
    }
    // De-registering a listener for auto-shutdown mode updates
    telux::common::Status status = thermShutdownMgr_->deregisterListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "*** ERROR - Failed to de-register auto-shutdown mode events"
                    << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "De-registered listener for auto-shutdown mode events"
                    << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Indicates whether thermal auto-shutdown is allowed or not. Typically allowed when an eCall is NOT
 * in progress
 */
bool ThermClient::isShutdownAllowed() {
    std::unique_lock<std::mutex> lock(mutex_);
    return shutdownAllowed_;
}

void ThermClient::setShutdownAllowedState(bool state) {
    std::unique_lock<std::mutex> lock(mutex_);
    shutdownAllowed_ = state;
}

// Disable thermal auto-shutdown
telux::common::Status ThermClient::disableAutoShutdown() {
    if(!isShutdownAllowed()) {
        std::cout << CLIENT_NAME << "Already disabled auto-shutdown mode" << std::endl;
        return telux::common::Status::SUCCESS;
    }
    setShutdownAllowedState(false);
    registerForUpdates();
    auto status = sendAutoShutdownModeCommand(AutoShutdownMode::DISABLE);
    if(status != telux::common::Status::SUCCESS) {
        setShutdownAllowedState(true);
        deregisterForUpdates();
    }
    return status;
}

// Enable thermal auto-shutdown
telux::common::Status ThermClient::enableAutoShutdown() {
    setShutdownAllowedState(true);
    auto status = sendAutoShutdownModeCommand(AutoShutdownMode::ENABLE);
    deregisterForUpdates();
    return status;
}

