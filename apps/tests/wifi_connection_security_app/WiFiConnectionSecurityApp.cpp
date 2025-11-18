/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cstdio>

#include <telux/common/Version.hpp>

#include "common/utils/Utils.hpp"
#include "WiFiConnectionSecurityApp.hpp"

WiFiConnectionSecurityApp::WiFiConnectionSecurityApp(
    std::string appName, std::string cursor) : ConsoleApp(appName, cursor) {
}

WiFiConnectionSecurityApp::~WiFiConnectionSecurityApp() {
}

/*
 *  Helper to get input from user.
 */
void WiFiConnectionSecurityApp::getStringFromUser(const std::string promptToDisplay,
        std::string& usrInput) {

    while(1) {
        std::cout << promptToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        return;
    }
}

/*
 *  Listener to receive ML analysis result.
 */
void WiFiSecurityReportListener::onReportAvailable(
        telux::sec::WiFiSecurityReport report) {

    std::cout << "ssid             : " << report.ssid << std::endl;
    std::cout << "bssid            : " << report.bssid << std::endl;
    std::cout << "is connected     : " << report.isConnectedToAP << std::endl;
    std::cout << "is open          : " << report.isOpenAP << std::endl;
    std::cout << "ml threat score  : " <<
        report.mlAlgorithmAnalysis.threatScore << std::endl;
    std::cout << "ml result        : " <<
        static_cast<int>(report.mlAlgorithmAnalysis.result) << std::endl;
    std::cout << "summoning result : " <<
        static_cast<int>(report.summoningAnalysis.result) << std::endl;
}

/*
 *  Listener to receive deauthentication attack info.
 */
void WiFiSecurityReportListener::onDeauthenticationAttack(
        telux::sec::DeauthenticationInfo deauthenticationInfo) {

    std::cout << "disconnect reason : " <<
        deauthenticationInfo.deauthenticationReason << std::endl;
    std::cout << "did AP initiated  : " <<
        deauthenticationInfo.didAPInitiateDisconnect << std::endl;
    std::cout << "threat score      : " <<
        deauthenticationInfo.threatScore << std::endl;
}

/*
 *  Sets the value based on selection made by user with the help of UI previously.
 */
void WiFiSecurityReportListener::isTrustedAP(telux::sec::ApInfo apInfo, bool& isTrusted) {

    std::cout << "Please press 3 to trust/distrust AP " << apInfo.ssid <<
    " with bssid " << apInfo.bssid << std::endl;

    /* Wait until user makes confirms to trust or distrust AP */
    std::unique_lock<std::mutex> lock(trustMutex_);
    promptUserForTrustingAP_ = true;
    trustCV_.wait(lock, [this]{return trustAPSelectionMade_;});

    /* Return user selection */
    isTrusted = trustGivenAP_;

    /* Reset local state */
    trustAPSelectionMade_ = false;
    trustGivenAP_ = false;
    promptUserForTrustingAP_ = false;
}

/*
 *  Save the user selection for trusting the AP.
 */
void WiFiSecurityReportListener::setTrustAPSelection(bool trust) {

    std::lock_guard<std::mutex> lock(trustMutex_);
    if (!promptUserForTrustingAP_) {
        /* User exercised option without prompting, don't do anything */
        return;
    }

    trustGivenAP_ = trust;
    trustAPSelectionMade_ = true;

    /* Pass the user selection to the waiter thread */
    trustCV_.notify_all();
}

/*
 *  Define whether to trust the AP or not when device connects to an AP and
 *  user selection listener is invoked.
 */
void WiFiConnectionSecurityApp::getTrustAPSelection() {

    std::string usrInput = "";
    size_t choiceLength;

    if (!reportListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    while(1) {
        std::cout << "do you trust this AP (yes/no): ";

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }

        choiceLength = usrInput.length();
        for (size_t x = 0; x < choiceLength; x++) {
            usrInput[x] = tolower(usrInput[x]);
        }

        if (!usrInput.compare("no")) {
            reportListener_->setTrustAPSelection(false);
            return;
        } else if (!usrInput.compare("yes")) {
            reportListener_->setTrustAPSelection(true);
            return;
        } else {
            std::cout << "invalid input " << usrInput << std::endl;
        }
    }
}

/*
 *  Register listener to start listening for security analysis reports.
 */
void WiFiConnectionSecurityApp::registerListener() {

    telux::common::ErrorCode ec;

    if (reportListener_) {
        std::cout << "Listener exist" << std::endl;
        return;
    }

    try {
        reportListener_ = std::make_shared<WiFiSecurityReportListener>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate WiFiReportListener" << std::endl;
        return;
    }

    ec = wifiConSecMgr_->registerListener(reportListener_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't register listener, err " << static_cast<int>(ec) << std::endl;
        reportListener_ = nullptr;
        return;
    }

    std::cout << "Listener registered" << std::endl;
}

/*
 *  Deregister listener to stop receiving security reports.
 */
void WiFiConnectionSecurityApp::deregisterListener() {

    telux::common::ErrorCode ec;

    {
        std::lock_guard<std::mutex> lock(reportListener_->trustMutex_);
        reportListener_->trustGivenAP_ = false;
        reportListener_->trustAPSelectionMade_ = true;
        /* Unblock callback thread by sending false before deregistration.*/
        reportListener_->trustCV_.notify_all();
    }

    if (!reportListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    ec = wifiConSecMgr_->deregisterListener(reportListener_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't deregister listener, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    reportListener_ = nullptr;
    std::cout << "Listener deregistered" << std::endl;
}

/*
 *  List trusted APs.
 */
void WiFiConnectionSecurityApp::getTrustedApList() {

    telux::common::ErrorCode ec;
    std::vector<telux::sec::ApInfo> trustedAPList{};

    if (!reportListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    ec = wifiConSecMgr_->getTrustedApList(trustedAPList);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't list APs, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    for (auto ap : trustedAPList) {
        std::cout << "ssid: " << ap.ssid << ", bssid: " << ap.bssid << std::endl;
    }
}

/*
 *  Remove trusted APs.
 */
void WiFiConnectionSecurityApp::removeApFromTrustedList() {

    telux::sec::ApInfo apInfo{};
    telux::common::ErrorCode ec;

    if (!reportListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    getStringFromUser("Enter SSID of AP  : ", apInfo.ssid);
    getStringFromUser("Enter BSSID of AP : ", apInfo.bssid);

    ec = wifiConSecMgr_->removeApFromTrustedList(apInfo);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't distrust AP, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << apInfo.ssid << " AP distrusted" << std::endl;
}

/*
 *  Prepare the menu and display it on the console.
 */
void WiFiConnectionSecurityApp::init() {

    ServiceStatus serviceStatus;
    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();

    //  Get the ConnectionSecurityFactory and WiFiSecurityManager instances.
    auto &wifiConSecFact = telux::sec::ConnectionSecurityFactory::getInstance();

    wifiConSecMgr_ = wifiConSecFact.getWiFiSecurityManager([&](telux::common::ServiceStatus
        srvStatus) {
            prom.set_value(srvStatus);
        });

    if (!wifiConSecMgr_) {
        std::cout << "Failed to get IWiFiSecurityManager " << std::endl;
        return;
    }

    // Wait for the subsystem to be available.
    serviceStatus = prom.get_future().get();
    //  Exit the application, if SDK is unable to initialize security subsystems.
    if(serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Security Subsystems ready" << std::endl;
    } else {
        std::cout << "Unable to initialize security subsystem, err: " << static_cast<int>(
            serviceStatus) << std::endl;
        return;
    }

    // Register for service status events
    auto ec = wifiConSecMgr_->registerListener(shared_from_this());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Security listener registeration failed, err: " << static_cast<int>(
            ec) << std::endl;
    }

    initConsole();
}

void WiFiConnectionSecurityApp::onServiceStatusChange(telux::common::ServiceStatus status) {
    if (status == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        std::cout << "Security service UNAVAILABLE" << std::endl;
    }
    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Security service AVAILABLE" << std::endl;
    }
}

void WiFiConnectionSecurityApp::initConsole() {
    std::shared_ptr<ConsoleAppCommand> regListener = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "Start listening to security reports", {},
        std::bind(&WiFiConnectionSecurityApp::registerListener, this)));

    std::shared_ptr<ConsoleAppCommand> deregListener = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Stop listening to security reports", {},
        std::bind(&WiFiConnectionSecurityApp::deregisterListener, this)));

    std::shared_ptr<ConsoleAppCommand> getTrustAPSelection = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("3", "Trust the AP (yes/no)", {},
        std::bind(&WiFiConnectionSecurityApp::getTrustAPSelection, this)));

    std::shared_ptr<ConsoleAppCommand> getTrustedApList = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("4", "List trusted APs", {},
        std::bind(&WiFiConnectionSecurityApp::getTrustedApList, this)));

    std::shared_ptr<ConsoleAppCommand> removeApFromTrustedList = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("5", "Remove trusted AP", {},
        std::bind(&WiFiConnectionSecurityApp::removeApFromTrustedList, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainCmds = {
        regListener, deregListener, getTrustAPSelection,
        getTrustedApList, removeApFromTrustedList };

    ConsoleApp::addCommands(mainCmds);
    ConsoleApp::displayMenu();
}

int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();

    std::string sdkReleaseName = telux::common::Version::getReleaseName();

    std::string appName = "WiFi connection security console app - SDK v"
                            + std::to_string(sdkVersion.major) + "."
                            + std::to_string(sdkVersion.minor) + "."
                            + std::to_string(sdkVersion.patch) + "\n"
                            + "Release name: " + sdkReleaseName;

    auto wcsApp = std::make_shared<WiFiConnectionSecurityApp>(appName, "wificonsec> ");

    std::vector<std::string> supplementaryGrps{"system", "diag", "gps", "logd", "dlt"};

    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc < 0) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    wcsApp->init();

    return wcsApp->mainLoop();
}
