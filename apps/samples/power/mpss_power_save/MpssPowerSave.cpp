/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to enable/disable certain power saving feature on
 * the modem peripheral subsystem (MPSS). The steps are as follows:
 *
 * 1. Get a PowerFactory instance.
 * 2. Get a ITcuActivityManager instance from the PowerFactory.
 * 3. Wait for the power service to become available.
 * 4. Enable MPSS power saving.
 * 5. Execute application specific business logic.
 * 6. Finally, when the use case is over, disable MPSS power saving.
 *
 * Usage:
 * # ./mpss_power_saver
 *
 * This will enable MPSS power saving, wait for some time and then disables
 * MPSS extra power savings.
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>

#include <telux/common/CommonDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityManager.hpp>

class MPSSPowerSaver : public std::enable_shared_from_this<MPSSPowerSaver> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};
        telux::power::ClientInstanceConfig config{};

        /* Step - 1 */
        auto &powerFactory = telux::power::PowerFactory::getInstance();

        /* Step - 2 */
        config.clientType = telux::power::ClientType::MASTER;
        config.clientName = "masterClientFoo";

        tcuActivityMgr_ = powerFactory.getTcuActivityManager(
            config, [&p](telux::common::ServiceStatus srvStatus) {
            p.set_value(srvStatus);
        });

        if (!tcuActivityMgr_) {
            std::cout << "Can't get ITcuActivityManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Power service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int enableModemPowerSaving() {
        telux::common::Status status;

        /* Step - 4 */
        status = tcuActivityMgr_->setModemActivityState(
            telux::power::TcuActivityState::SUSPEND);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't enable modem power saving, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Power saving enabled" << std::endl;
        return 0;
    }

    int disableModemPowerSaving() {
        telux::common::Status status;

        /* Step - 6 */
        status = tcuActivityMgr_->setModemActivityState(
            telux::power::TcuActivityState::RESUME);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't disable modem power saving, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Power saving disabled" << std::endl;
        return 0;
    }

 private:
    std::shared_ptr<telux::power::ITcuActivityManager> tcuActivityMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<MPSSPowerSaver> app;

    try {
        app = std::make_shared<MPSSPowerSaver>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate MPSSPowerSaver" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->enableModemPowerSaving();
    if (ret < 0) {
        return ret;
    }

    /* Step - 5 */
    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    ret = app->disableModemPowerSaving();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
