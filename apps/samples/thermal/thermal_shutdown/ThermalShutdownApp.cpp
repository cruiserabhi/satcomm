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
 *  Copyright (c) 2021,2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * This application demonstrates how to get status of the automatic thermal
 * shutdown and register for getting notifications when automatic thermal
 * shutdown mode is enabled, disabled or will be enabled imminently. The
 * steps are as follows:
 *
 * 1. Get a ThermalFactory instance.
 * 2. Get a IThermalShutdownManager instance from the ThermalFactory.
 * 3. Wait for the thermal service to become available.
 * 4. Register a listener that will receive shutdown events.
 * 5. Get information about current shutdown mode.
 * 6. Finally, deregister the listener when the use case is complete.
 *
 * Usage:
 * # ./thermal_shutdown_app
 */

#include <errno.h>

#include <iostream>
#include <chrono>
#include <thread>

#include <telux/therm/ThermalDefines.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/therm/ThermalFactory.hpp>

#include "ThermalShutdownApp.hpp"

void ThermalShutdownListener::onShutdownEnabled() {
    std::cout << "onShutdownEnabled()" << std::endl;
}

void ThermalShutdownListener::onShutdownDisabled() {
    std::cout << "onShutdownDisabled()" << std::endl;
}

void ThermalShutdownListener::onImminentShutdownEnablement(uint32_t imminentDuration) {
    std::cout << "onImminentShutdownEnablement()" << std::endl;
    std::cout << "Auto shutdown will be enabled in " <<
        imminentDuration << " seconds" << std::endl;
}

int Application::init() {
    telux::common::Status status;
    telux::common::ServiceStatus serviceStatus;
    std::promise<telux::common::ServiceStatus> p{};

    /* Step - 1 */
    auto &thermalFactory = telux::therm::ThermalFactory::getInstance();

    /* Step - 2 */
    thermShutdownMgr_ = thermalFactory.getThermalShutdownManager(
        [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
    });

    if (!thermShutdownMgr_) {
        std::cout << "Can't get IThermalManager" << std::endl;
        return -ENOMEM;
    }

    /* Step - 3 */
    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Thermal service unavailable, status " <<
            static_cast<int>(serviceStatus) << std::endl;
        return -EIO;
    }

    /* Step - 4 */
    try {
        thermShutdownListener_ = std::make_shared<ThermalShutdownListener>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate ThermalShutdownListener" << std::endl;
        return -ENOMEM;
    }

    status = thermShutdownMgr_->registerListener(thermShutdownListener_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Can't register listener, err " <<
            static_cast<int>(status) << std::endl;
        return -EIO;
    }

    std::cout << "Initialization complete" << std::endl;
    return 0;
}

int Application::deinit() {
    telux::common::Status status;

    /* Step - 6 */
    status = thermShutdownMgr_->deregisterListener(thermShutdownListener_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Can't deregister listener, err " <<
            static_cast<int>(status) << std::endl;
        return -EIO;
    }

    return 0;
}

int Application::getAutoShutdownMode() {
    telux::common::Status status;
    telux::therm::AutoShutdownMode shutoDownMode;
    std::promise<telux::therm::AutoShutdownMode> p{};

    /* Step - 5 */
    status = thermShutdownMgr_->getAutoShutdownMode(
        [&p](telux::therm::AutoShutdownMode mode) {
        p.set_value(mode);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Can't get shutdown mode, err " <<
            static_cast<int>(status) << std::endl;
        return -EIO;
    }

    std::cout << "\nRequested for shutdown mode" << std::endl;

    shutoDownMode = p.get_future().get();

    if (shutoDownMode == telux::therm::AutoShutdownMode::ENABLE) {
        std::cout << "\nShutdown mode is enabled" << std::endl;
    } else if (shutoDownMode == telux::therm::AutoShutdownMode::DISABLE) {
        std::cout << "\nShutdown mode is disabled" << std::endl;
    } else {
        std::cout << "\nShutdown mode is unknown" << std::endl;
    }

    return 0;
}

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<Application> app;

    try {
        app = std::make_shared<Application>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate Application" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->getAutoShutdownMode();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    /* Wait for receiving all asynchronous responses.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(3));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nApplication exiting" << std::endl;
    return 0;
}
