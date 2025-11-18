/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <future>
#include <iostream>

#include <telux/common/Version.hpp>

#include "common/utils/Utils.hpp"

#include "SubsystemApp.hpp"

SubsystemApp::SubsystemApp(std::string appName,
    std::string cursor) : ConsoleApp(appName, cursor) {
}

SubsystemApp::~SubsystemApp() {
}

/*
 *  Listener to receive state change updates.
 */
void StateChangeListener::onStateChange(telux::common::SubsystemInfo subsystemInfo,
        telux::common::OperationalStatus newOperationalStatus) {

    std::cout << "\nLocation   : " << static_cast<int>(subsystemInfo.location) << std::endl;
    std::cout << "Subsystem  : " << static_cast<int>(subsystemInfo.subsystems) << std::endl;
    std::cout << "New status : " << static_cast<int>(newOperationalStatus) << std::endl;
}

/*
 *  Register a listener to start monitoring subsystem state changes.
 */
void SubsystemApp::registerListener() {

    telux::common::ErrorCode ec;
    std::vector<telux::common::SubsystemInfo> listOfSubsystems;

    if (stateChangeListener_) {
        std::cout << "Listener exist" << std::endl;
        return;
    }

    try {
        stateChangeListener_ = std::make_shared<StateChangeListener>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate StateChangeListener" << std::endl;
        stateChangeListener_ = nullptr;
        return;
    }

    getSubsystemsToMonitor(listOfSubsystems);

    if (listOfSubsystems.empty()) {
        std::cout << "Not monitoring as no subsystem specified"<< std::endl;
        stateChangeListener_ = nullptr;
        return;
    }

    ec = subsystemMgr_->registerListener(stateChangeListener_, listOfSubsystems);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't register listener, err " << static_cast<int>(ec) << std::endl;
        stateChangeListener_ = nullptr;
        return;
    }

    std::cout << "Listener registered" << std::endl;
}

/*
 * Ask user to specify what subsystems to monitor and populate listOfSubsystems accordingly.
 */
void SubsystemApp::getSubsystemsToMonitor(
        std::vector<telux::common::SubsystemInfo> &listOfSubsystems) {

    bool yes, local;
    telux::common::SubsystemInfo subsysInfo{};

    yes = userUtils_.getYesNoFromUser("Monitor MPSS");
    if (yes) {
        subsysInfo.subsystems = telux::common::Subsystem::MPSS;
        local = userUtils_.getLocalRemoteFromUser();
        if (local) {
            subsysInfo.location = telux::common::ProcType::LOCAL_PROC;
        } else {
            subsysInfo.location = telux::common::ProcType::REMOTE_PROC;
        }
        listOfSubsystems.push_back(subsysInfo);
    }

    yes = userUtils_.getYesNoFromUser("Monitor APSS");
    if (yes) {
        subsysInfo.subsystems = telux::common::Subsystem::APSS;
        local = userUtils_.getLocalRemoteFromUser();
        if (local) {
            subsysInfo.location = telux::common::ProcType::LOCAL_PROC;
        } else {
            subsysInfo.location = telux::common::ProcType::REMOTE_PROC;
        }
        listOfSubsystems.push_back(subsysInfo);
    }
}

/*
 *  Deregister listener to stop monitoring subsystems.
 */
void SubsystemApp::deRegisterListener() {

    telux::common::ErrorCode ec;

    if (!stateChangeListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    ec = subsystemMgr_->deRegisterListener(stateChangeListener_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't deregister listener, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    stateChangeListener_ = nullptr;
    std::cout << "Listener deregistered" << std::endl;
}

/*
 *  Prepare the menu and display it on the console.
 */
void SubsystemApp::init() {

    telux::common::ServiceStatus serviceStatus;
    std::promise<telux::common::ServiceStatus> p{};

    auto &subsystemFact = telux::platform::SubsystemFactory::getInstance();

    subsystemMgr_ = subsystemFact.getSubsystemManager(
            [&p](telux::common::ServiceStatus srvStatus) {
        p.set_value(srvStatus);
    });

    if (!subsystemMgr_) {
        std::cout << "Can't get ISubsystemManager, waiting..." << std::endl;
        return;
    }

    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Subsystem manager unavailable" << std::endl;
        return;
    }

    std::shared_ptr<ConsoleAppCommand> regListener = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "Start monitoring subsystems", {},
        std::bind(&SubsystemApp::registerListener, this)));

    std::shared_ptr<ConsoleAppCommand> deregListener = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Stop monitoring subsystems", {},
        std::bind(&SubsystemApp::deRegisterListener, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainCmds = {
        regListener, deregListener };

    ConsoleApp::addCommands(mainCmds);
    ConsoleApp::displayMenu();
}

int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();

    std::string sdkReleaseName = telux::common::Version::getReleaseName();

    std::string appName = "Subsystem monitor console app - SDK v"
                            + std::to_string(sdkVersion.major) + "."
                            + std::to_string(sdkVersion.minor) + "."
                            + std::to_string(sdkVersion.patch) + "\n"
                            + "Release name: " + sdkReleaseName;

    auto sysApp = std::make_shared<SubsystemApp>(appName, "subsys> ");

    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};

    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc < 0) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    sysApp->init();

    return sysApp->mainLoop();
}
