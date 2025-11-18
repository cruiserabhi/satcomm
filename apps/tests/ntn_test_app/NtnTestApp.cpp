/*
*
*   Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
*   SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <iostream>
#include <csignal>
#include <mutex>
#include <condition_variable>
#include <string>
#include <iomanip>
#include <ios>
#include <sstream>

#include "NtnTestApp.hpp"
#include <telux/satcom/SatcomFactory.hpp>

#include "../../common/utils/Utils.hpp"
#include "../../common/utils/SignalHandler.hpp"

#define APP_NAME "ntn_test_app"

/**
 * @file: NtnTestApp.cpp
 *
 * @brief: Test application for NTN Manager
 */

static std::mutex mutex;
static std::condition_variable cv;

std::string NtnTestApp::toString(NtnState state) {
    switch (state) {
        case NtnState::DISABLED:
            return "DISABLED";
        case NtnState::OUT_OF_SERVICE:
            return "OUT_OF_SERVICE";
        case NtnState::IN_SERVICE:
            return "IN_SREVICE";
    }
    return "-";
}

std::string NtnTestApp::toString(NtnCapabilities cap) {
    return std::to_string(cap.maxDataSize);
}

std::string NtnTestApp::toString(SignalStrength ss) {
    switch (ss) {
        case SignalStrength::NONE:
            return "NONE";
        case SignalStrength::POOR:
            return "POOR";
        case SignalStrength::MODERATE:
            return "MODERATE";
        case SignalStrength::GOOD:
            return "GOOD";
        case SignalStrength::GREAT:
            return "GREAT";
    }
    return "-";
}

std::string NtnTestApp::toString(ServiceStatus status) {
    switch (status) {
        case ServiceStatus::SERVICE_AVAILABLE:
            return "SERVICE_AVAILABLE";
        case ServiceStatus::SERVICE_UNAVAILABLE:
            return "SERVICE_UNAVAILABLE";
        case ServiceStatus::SERVICE_FAILED:
            return "SERVICE_FAILED";
    }
    return "-";
}

void NtnTestApp::onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size) {
    std::cout << "**** onIncomingData *****" << std::endl;

    std::cout << "===Printing raw data===\n";
    for(uint32_t i = 0; i < size; ++i)
        std::cout << std::setfill('0') << std::hex << std::setw(2) << (uint32_t)data[i] << " ";
    std::cout << "\n===End of raw data===\n";
    std::cout << "===Printing data in ascii format (unprintable characters are printed as -)===\n";
    for(uint32_t i = 0; i < size; ++i) {
        if (!isprint((unsigned char)data[i]))
        {
            std::cout << "-";
        } else {
            std::cout << data[i];
        }
    }
    std::cout << "\n===End of data in ascii format===\n";
    std::cout << "*************************" << std::endl;
}

void NtnTestApp::onNtnStateChange(NtnState state) {
    std::cout << "**** onNtnStateChange = " << toString(state) << std::endl;
}

void NtnTestApp::onCapabilitiesChange(NtnCapabilities capabilities) {
    std::cout << "**** onCapabilitiesChange = " << toString(capabilities) << std::endl;
}

void NtnTestApp::onSignalStrengthChange(SignalStrength newStrength) {
    std::cout << "**** onSignalStrengthChange = " << toString(newStrength) << std::endl;
}

void NtnTestApp::onServiceStatusChange(ServiceStatus status) {
    std::cout << "**** onServiceStatusChange = " << toString(status) << std::endl;
}

void NtnTestApp::onDataAck(ErrorCode err, TransactionId id) {
    if (err == ErrorCode::SUCCESS) {
        std::cout << "**** = onDataAck ack received for id = " << id << std::endl;
    } else {
        std::cout << "**** = onDataAck error = " << Utils::getErrorCodeAsString(err) <<
            ", id = " << id << ")" << std::endl;
    }
}

void NtnTestApp::onCellularCoverageAvailable(bool isCellularCoverageAvailable) {
    std::cout << "onCellularCoverageAvailable = " << isCellularCoverageAvailable << std::endl;
}

void NtnTestApp::getServiceStatus(std::vector<std::string> inputCommand) {
    std::cout << "getServiceStatus = " << toString(ntnMgr_->getServiceStatus()) << std::endl;
}

void NtnTestApp::isNtnSupported(std::vector<std::string> inputCommand) {
    bool supported = false;
    auto err = ntnMgr_->isNtnSupported(supported);
    std::cout << "isNtnSupported returned error = " << Utils::getErrorCodeAsString(err) <<std::endl;
    if (supported) {
        std::cout << "<Ntn is supported>" << std::endl;
    } else {
        std::cout << "<Ntn is not supported>" << std::endl;
    }
}

void NtnTestApp::enableNtn(std::vector<std::string> inputCommand) {

    int enable, emergency = 0;
    std::string iccid;
    std::cout << "Enter 0 to disable and 1 to enable NTN: ";
    std::cin >> enable;
    Utils::validateInput(enable, {0,1});
    if (enable) {
        std::cout << "Enter 0 for non-emergency and 1 for emergency data: ";
        std::cin >> emergency;
        Utils::validateInput(emergency, {0,1});
        std::cout << "Enter iccid: ";
        std::cin >> iccid;
        Utils::validateInput(iccid);
    }
    std::cout
        << "enableNtn errorno = " <<
        Utils::getErrorCodeAsString(ntnMgr_->enableNtn(enable, emergency, iccid)) << std::endl;
}

void NtnTestApp::sendDataString(std::vector<std::string> inputCommand) {
    int isEmergency;
    std::string str;

    std::cout << "Enter 0 for non-emergency and 1 for emergency data: ";
    std::cin >> isEmergency;
    Utils::validateInput(isEmergency, {0, 1});
    std::cout << "Enter string to be sent : ";
    std::cin >> str;
    Utils::validateInput(str);

    std::vector<uint8_t> data;
    for (char c : str) {
        data.push_back((uint8_t) c);
    }
    TransactionId tId;
    std::cout << "Sending data of size(in bytes) : " << data.size() << std::endl;
    auto err = ntnMgr_->sendData(data.data(), str.size(), isEmergency, tId);
    std::cout << "sendData status = " << static_cast<int>(err) << std::endl;
    std::cout << "sendData tId = " << tId << std::endl;
}

void NtnTestApp::sendDataRaw(std::vector<std::string> inputCommand) {
    int isEmergency;
    std::string str;

    std::cout << "Enter 0 for non-emergency and 1 for emergency data: ";
    std::cin >> isEmergency;
    Utils::validateInput(isEmergency, {0,1});
    std::cout << "Enter raw data to be sent: ";
    std::cin >> str;
    Utils::validateInput(str);

    std::vector<uint8_t> data;
    std::istringstream ss(str);
    std::string buffer;

    while((ss >> std::setw(2) >> buffer)) {
        unsigned u;
        std::istringstream ss2 (buffer);
        ss2 >> std::setbase(16) >> u;
        data.push_back((uint8_t)u);
    }

    TransactionId tId;
    std::cout << "Sending data of size(in bytes) : " << data.size() << std::endl;
    auto err = ntnMgr_->sendData(data.data(), data.size(), isEmergency, tId);
    std::cout << "sendData status = " << static_cast<int>(err) << std::endl;
    std::cout << "sendData tId = " << tId << std::endl;
}

void NtnTestApp::abortData(std::vector<std::string> inputCommand) {
    std::vector<uint8_t> data;
    for (size_t i = 0; i < 5; ++i) {
        data.push_back(i);
    }
    TransactionId tId;
    auto err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    err = ntnMgr_->sendData(data.data(), data.size(), true, tId);
    (void)err;
    std::cout << "abortData errno = " << Utils::getErrorCodeAsString(ntnMgr_->abortData())
        << std::endl;
}

void NtnTestApp::getNtnCapabilities(std::vector<std::string> inputCommand) {
    NtnCapabilities cap;
    auto err = ntnMgr_->getNtnCapabilities(cap);
    std::cout << "getNtnCapabilities errno = " << Utils::getErrorCodeAsString(err) << std::endl;
    std::cout << "getNtnCapabilities maxDataSize = " << cap.maxDataSize << std::endl;
}

void NtnTestApp::updateSystemSelectionSpecifiers(std::vector<std::string> inputCommand) {
    std::vector<SystemSelectionSpecifier> sssVec;
    size_t size;
    size_t bands, earfcns;
    uint64_t band, earfcn;

    std::cout << "Enter number of system selection params in SFL: ";
    std::cin >> size;
    Utils::validateInput(size);

    for (size_t i = 0; i < size; ++i) {
        SystemSelectionSpecifier sss;
        std::cout << "Enter mcc: ";
        std::cin >> sss.mcc;
        Utils::validateInput(sss.mcc);
        std::cout << "Enter mnc: ";
        std::cin >> sss.mnc;
        Utils::validateInput(sss.mnc);
        std::cout << "Enter number of bands: ";
        std::cin >> bands;
        Utils::validateInput(bands);
        for (; bands > 0; --bands) {
            std::cout << "Enter band: ";
            std::cin >> band;
            Utils::validateInput(band);
            sss.ntnBands.push_back(band);
        }
        std::cout << "Enter number of Earfcns: ";
        std::cin >> earfcns;
        Utils::validateInput(earfcns);
        for (; earfcns > 0; --earfcns) {
            std::cout << "Enter earfcn: ";
            std::cin >> earfcn;
            Utils::validateInput(earfcn);
            sss.ntnEarfcns.push_back(earfcn);
        }
        sssVec.push_back(sss);
    }
    auto err = ntnMgr_->updateSystemSelectionSpecifiers(sssVec);
    std::cout << "updateSFL errno = " << Utils::getErrorCodeAsString(err) << std::endl;
}

void NtnTestApp::getNtnState(std::vector<std::string> inputCommand) {
    auto state = ntnMgr_->getNtnState();
    std::cout << "getNtnState: " << toString(state) << std::endl;
}

void NtnTestApp::enableCellularScan(std::vector<std::string> inputCommand) {
    int flag = 0;
    std::cout << "Enter 1 to enable or 0 to disable cellular scan: \n";
    std::cin >> flag;
    Utils::validateInput(flag, {0,1});
    auto err = ntnMgr_->enableCellularScan(flag);
    std::cout << "enableCellularScan errno = " << Utils::getErrorCodeAsString(err) << std::endl;
}

void NtnTestApp::consoleInit() {
    std::shared_ptr<ConsoleAppCommand> isNtnSupportedCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "isNtnSupported", {},
            std::bind(&NtnTestApp::isNtnSupported, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> enableNtnCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "2", "enableNtn", {}, std::bind(&NtnTestApp::enableNtn, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getNtnStateCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "getNtnState", {},
            std::bind(&NtnTestApp::getNtnState, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getNtnCapabilitiesCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "getNtnCapabilities", {},
            std::bind(&NtnTestApp::getNtnCapabilities, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> updateSflCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "updateSFL", {},
            std::bind(&NtnTestApp::updateSystemSelectionSpecifiers, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> sendDataStringCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "6", "sendData(string)", {}, std::bind(&NtnTestApp::sendDataString,
            this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> sendDataRawCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "7", "sendData(raw)", {}, std::bind(&NtnTestApp::sendDataRaw,
            this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> abortDataCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "8", "abortData", {}, std::bind(&NtnTestApp::abortData, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> enableCellularScanCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "9", "enableCellularScan", {}, std::bind(&NtnTestApp::enableCellularScan,
            this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsNtn
        = {isNtnSupportedCmd, enableNtnCmd, getNtnStateCmd,
            getNtnCapabilitiesCmd, updateSflCmd, sendDataStringCmd, sendDataRawCmd, abortDataCmd,
            enableCellularScanCmd};
    ConsoleApp::addCommands(commandsNtn);
    ConsoleApp::displayMenu();
}

void NtnTestApp::registerForUpdates() {
    Status status = ntnMgr_->registerListener(shared_from_this());
    if (status != Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to register for ntn notification" << std::endl;
    } else {
        std::cout << APP_NAME << " Registered Listener for ntn notification" << std::endl;
    }
}

void NtnTestApp::deregisterForUpdates() {
    Status status = ntnMgr_->deregisterListener(shared_from_this());
    if (status != Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to de-register for ntn notification" << std::endl;
    } else {
        std::cout << APP_NAME << " De-registered listener" << std::endl;
    }
}

std::shared_ptr<NtnTestApp> init() {
    std::shared_ptr<NtnTestApp> ntnTestApp = std::make_shared<NtnTestApp>();
    if (!ntnTestApp) {
        std::cout << "Failed to instantiate NtnTestApp" << std::endl;
        return nullptr;
    }

    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    auto rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    std::mutex initMtx;
    std::condition_variable initCv;
    ServiceStatus status;
    auto initCb = [&](telux::common::ServiceStatus sparam) {
        std::lock_guard<std::mutex> lock(initMtx);
        status = sparam;
        initCv.notify_all();
        };

    auto &satcomFactory = telux::satcom::SatcomFactory::getInstance();
    std::unique_lock<std::mutex> lck(initMtx);
    ntnTestApp->ntnMgr_ = satcomFactory.getNtnManager(initCb);
    initCv.wait(lck);

    if (ntnTestApp->ntnMgr_ == nullptr)
    {
        std::cout << "satcomFactory.getNtnManager returned nullptr" << std::endl;
        return ntnTestApp;
    }
    if (ntnTestApp->ntnMgr_->getServiceStatus() == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "*** Ntn subsystem is ready ***" << std::endl;
    } else {
        std::cout << "*** Ntn subsystem is not ready ***" << std::endl;
        return nullptr;
    }

    return ntnTestApp;
}

NtnTestApp::NtnTestApp()
   : ConsoleApp("Ntn Test Menu", "ntn-test> ")
   , ntnMgr_(nullptr) {
}

NtnTestApp::~NtnTestApp() {
}

/**
 * Main routine
 */
int main(int argc, char **argv) {

    std::cout << "\n#################################################\n"
              << "  Ntn test app\n"
              << "#################################################\n"
              << std::endl;
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGHUP);
    SignalHandlerCb cb = [](int sig) {
        // We can call exit() here if no cleanups needed,
        // or maybe just set a flag, and let the main thread to decide
        // when to exit.
        exit(sig);
    };
    SignalHandler::registerSignalHandler(sigset, cb);

    std::shared_ptr<NtnTestApp> myNtnTestApp = init();
    myNtnTestApp->registerForUpdates();

    myNtnTestApp->consoleInit();
    myNtnTestApp->mainLoop();
    myNtnTestApp->deregisterForUpdates();

    std::cout << "Exiting application..." << std::endl;
    return 0;
}
