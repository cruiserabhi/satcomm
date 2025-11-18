/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cstdio>

#include <telux/common/Version.hpp>

#include "common/utils/Utils.hpp"

#include "CellularConnectionSecurityApp.hpp"

CellularConnectionSecurityApp::CellularConnectionSecurityApp(
        std::string appName, std::string cursor) : ConsoleApp(appName, cursor) {
}

CellularConnectionSecurityApp::~CellularConnectionSecurityApp() {
}

/*
 *  Listener to receive reports.
 */
void CellSecurityReportListener::onScanReportAvailable(
        telux::sec::CellularSecurityReport report, telux::sec::EnvironmentInfo envInfo) {

    std::cout << "Threat score: " << static_cast<uint32_t>(report.threatScore) << std::endl;
    std::cout << "Cell ID     : " << static_cast<uint32_t>(report.cellId) << std::endl;
    std::cout << "PID         : " << static_cast<uint32_t>(report.pid) << std::endl;
    std::cout << "MCC         : " << report.mcc << std::endl;
    std::cout << "MNC         : " << report.mnc << std::endl;
    std::cout << "Action type : " << static_cast<uint32_t>(report.actionType) << std::endl;
    std::cout << "RAT         : " << static_cast<uint32_t>(report.rat) << std::endl;

    for (size_t x = 0; x < report.threats.size(); x++) {
        std::cout << "Threat type : " <<
            static_cast<uint32_t>(report.threats[x]) << std::endl;
    }

    std::cout << "Environment : " <<
        static_cast<uint32_t>(envInfo.environmentState) << std::endl;
}

/*
 *  Listener to receive SSR events.
 */
void CellSecurityReportListener::onServiceStatusChange(
        telux::common::ServiceStatus newStatus) {

    std::cout << "Service status: " << static_cast<uint32_t>(newStatus) << std::endl;
}

void CellularConnectionSecurityApp::registerListener() {

    telux::common::ErrorCode ec;

    if (reportListener_) {
        std::cout << "Listener exist" << std::endl;
        return;
    }

    try {
        reportListener_ = std::make_shared<CellSecurityReportListener>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate CellSecurityReportListener" << std::endl;
    }

    ec = cellConSecMgr_->registerListener(reportListener_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't register listener, err " << static_cast<int>(ec) << std::endl;
        reportListener_ = nullptr;
        return;
    }

    std::cout << "Listener registered" << std::endl;
}

void CellularConnectionSecurityApp::deregisterListener() {

    telux::common::ErrorCode ec;

    if (!reportListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    ec = cellConSecMgr_->deRegisterListener(reportListener_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't register listener, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    reportListener_ = nullptr;
    std::cout << "Listener deregistered" << std::endl;
}

/*
 *  Since the time listener was registered till now, get overall stats.
 */
void CellularConnectionSecurityApp::getSessionStats() {

    telux::common::ErrorCode ec;
    telux::sec::SessionStats stats{};

    if (!reportListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    ec = cellConSecMgr_->getCurrentSessionStats(stats);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't get stat, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Report count            : " <<
        static_cast<uint32_t>(stats.reportsCount) << std::endl;
    std::cout << "Threshold crossed count : " <<
        static_cast<uint32_t>(stats.thresholdCrossedCount) << std::endl;
    std::cout << "Average threat score    : " <<
        static_cast<uint32_t>(stats.averageThreatScore) << std::endl;
    std::cout << "Last action             : " <<
        static_cast<uint32_t>(stats.lastAction) << std::endl;
    std::cout << "Any action taken        : " <<
        static_cast<uint32_t>(stats.anyActionTaken) << std::endl;

    for (size_t x = 0; x < stats.threats.size(); x++) {
        std::cout << "Threat type             : " <<
            static_cast<uint32_t>(stats.threats[x]) << std::endl;
    }
}

/*
 *  Prepare menu and display on console.
 */
void CellularConnectionSecurityApp::init() {

    telux::common::ErrorCode ec;

    auto &cellConSecFact = telux::sec::ConnectionSecurityFactory::getInstance();

    cellConSecMgr_ = cellConSecFact.getCellularSecurityManager(ec);
    if (!cellConSecMgr_) {
        std::cout <<
         "can't get ICellularSecurityManager, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::shared_ptr<ConsoleAppCommand> regListener = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "Start listening to security reports", {},
        std::bind(&CellularConnectionSecurityApp::registerListener, this)));

    std::shared_ptr<ConsoleAppCommand> deregListener = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Stop listening to security reports", {},
        std::bind(&CellularConnectionSecurityApp::deregisterListener, this)));

    std::shared_ptr<ConsoleAppCommand> getStats = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("3", "Get session stats", {},
        std::bind(&CellularConnectionSecurityApp::getSessionStats, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainCmds = {
        regListener, deregListener, getStats
    };

    ConsoleApp::addCommands(mainCmds);
    ConsoleApp::displayMenu();
}

int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::string appName = "Cellular connection security console app - SDK v"
                            + std::to_string(sdkVersion.major) + "."
                            + std::to_string(sdkVersion.minor) + "."
                            + std::to_string(sdkVersion.patch) + "\n"
                            + "Release name: " + sdkReleaseName;

    auto ccsApp = std::make_shared<CellularConnectionSecurityApp>(appName, "cellconsec> ");

    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};

    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc < 0) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    ccsApp->init();

    return ccsApp->mainLoop();
}
