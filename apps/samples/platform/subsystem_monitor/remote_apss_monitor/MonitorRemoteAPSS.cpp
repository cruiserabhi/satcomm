/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * Steps to register listener and receive updates when APSS state is changed are:
 *
 * 1. Define listener that will receive new status updates.
 * 2. Get SubsystemFactory instance.
 * 3. Get ISubsystemManager instance from SubsystemFactory.
 * 4. Define which subsystem to monitor and register listener to receive status updates.
 * 5. Receive status updates in the registered listener.
 * 6. When the use-case is complete, deregister the listener.
 */

#include <errno.h>

#include <future>
#include <iostream>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/SubsystemFactory.hpp>
#include <telux/platform/SubsystemManager.hpp>

class RemoteAPSSStateListener : public telux::platform::ISubsystemListener {
 public:
    /* Step - 5 */
    void onStateChange(telux::common::SubsystemInfo subsystemInfo,
        telux::common::OperationalStatus newOperationalStatus) override {
        std::cout << "\nLocation   : " << static_cast<int>(subsystemInfo.location) << std::endl;
        std::cout << "Subsystem  : " << static_cast<int>(subsystemInfo.subsystems) << std::endl;
        std::cout << "New status : " << static_cast<int>(newOperationalStatus) << std::endl;
    }
};

int main(int argc, char **argv) {

    telux::common::ErrorCode ec;
    telux::common::ServiceStatus serviceStatus;
    std::promise<telux::common::ServiceStatus> p{};

    std::shared_ptr<telux::platform::ISubsystemManager> subsystemMgr;

    std::shared_ptr<RemoteAPSSStateListener> stateListener;
    telux::common::SubsystemInfo subsysInfo{};
    std::vector<telux::common::SubsystemInfo> listOfSubsystems;

    /* Step - 1 */
    try {
        stateListener = std::make_shared<RemoteAPSSStateListener>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate RemoteAPSSStateListener" << std::endl;
        return -ENOMEM;
    }

    /* Step - 2 */
    auto &subsystemFact = telux::platform::SubsystemFactory::getInstance();

    /* Step - 3 */
    subsystemMgr = subsystemFact.getSubsystemManager(
            [&p](telux::common::ServiceStatus srvStatus) {
        p.set_value(srvStatus);
    });

    if (!subsystemMgr) {
        std::cout << "Can't get ISubsystemManager" << std::endl;
        return -ENOMEM;
    }

    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "ISubsystemManager unavailable" << std::endl;
        return -EIO;
    }

    /* Step - 4 */
    subsysInfo.location = telux::common::ProcType::REMOTE_PROC;
    subsysInfo.subsystems = telux::common::Subsystem::APSS;
    listOfSubsystems.push_back(subsysInfo);
    ec = subsystemMgr->registerListener(stateListener, listOfSubsystems);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't register listener, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Waiting for state change notification" << std::endl;

    /* Add application specific business logic here */
    std::this_thread::sleep_for(std::chrono::seconds(90));

    /* Step - 6 */
    ec = subsystemMgr->deRegisterListener(stateListener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't deregister listener, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
