/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to create a slave client and listen to power state
 * change events. The steps are as follows:
 *
 * 1. Get a PowerFactory instance.
 * 2. Get a ITcuActivityManager instance from the PowerFactory.
 * 3. Wait for the power service to become available.
 * 4. Register for power events.
 * 5. When event is received acknowledge it and execute application specific
 *    business logic.
 * 6. Finally, when the use case is over, deregister the listener.
 *
 * Usage:
 * # ./tcuactivity_slave <wait-duration>
 *
 * This will wait for <wait-duration> seconds to receive and acknowledge TcuActivity events.
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
#include <telux/power/TcuActivityListener.hpp>

class PowerEventsListener : public telux::power::ITcuActivityListener,
                            public std::enable_shared_from_this<PowerEventsListener> {
 public:
    int init() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};
        telux::power::ClientInstanceConfig config{};

        /* Step - 1 */
        auto &powerFactory = telux::power::PowerFactory::getInstance();

        /* Step - 2 */
        config.clientType = telux::power::ClientType::SLAVE;
        config.clientName = "slaveClientFoo";
        config.machineName = telux::power::LOCAL_MACHINE;

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

        /* Step - 4 */
        status = tcuActivityMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        /* Step - 6 */
        status = tcuActivityMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    /* Step - 5 */
    void onTcuActivityStateUpdate(telux::power::TcuActivityState state,
            std::string machineName) override {
        telux::common::Status status;
        std::cout << "onTcuActivityStateUpdate()" << std::endl;

        switch (state) {
            case telux::power::TcuActivityState::SUSPEND:
                std::cout << "Received suspend state for machine: " << machineName << std::endl;

                status = tcuActivityMgr_->sendActivityStateAck(
                    telux::power::StateChangeResponse::ACK, state);
                if (status == telux::common::Status::SUCCESS) {
                    std::cout << "Suspend acknowledgement sent" << std::endl;
                    break;
                }
                std::cout << "Can't acknowledgement suspend, err " <<
                    static_cast<int>(status) << std::endl;
                break;
            case telux::power::TcuActivityState::RESUME:
                std::cout << "Received resume state for machine: " << machineName << std::endl;
                /* Sending acknowledgement is not required for resume event */
                break;
            case telux::power::TcuActivityState::SHUTDOWN:
                std::cout << "Received shutdown event, machine: " << machineName << std::endl;

                status = tcuActivityMgr_->sendActivityStateAck(
                    telux::power::StateChangeResponse::ACK, state);
                if (status == telux::common::Status::SUCCESS) {
                    std::cout << "Shutdown acknowledgement sent" << std::endl;
                    break;
                }
                std::cout << "Can't acknowledgement shutdown, err " <<
                    static_cast<int>(status) << std::endl;
                break;
            default:
                std::cout << "Unexpected state " << static_cast<int>(state) <<
                    "received, machine: " << machineName << std::endl;
                break;
        }
    }

 private:
    std::shared_ptr<telux::power::ITcuActivityManager> tcuActivityMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<PowerEventsListener> app;

    if (argc < 2) {
        std::cout << "Usage: tcuactivity_slave <wait-duration>" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<PowerEventsListener>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate PowerEventsListener" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(std::stoul(argv[1])));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
