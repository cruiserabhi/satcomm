/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xSlssUtcTest.cpp
 *
 * @brief: Simple application that demonstrates how to inject coarse UTC when UE is
 * sync to remote UE through SLSS and get precise UTC in turn.
 */

#include <iostream>
#include <future>
#include <mutex>
#include <string>
#include <signal.h>

#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/TimeManager.hpp>
#include <telux/platform/TimeListener.hpp>

#include "../../common/utils/Utils.hpp"
#include "../../common/utils/SignalHandler.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::promise;
using std::string;
using std::make_shared;
using std::shared_ptr;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xRadioManager;
using telux::platform::PlatformFactory;
using telux::platform::ITimeManager;
using telux::platform::ITimeListener;
using telux::platform::SupportedTimeType;
using telux::platform::TimeTypeMask;

static bool gExit = false;
static std::mutex mtx;
static std::condition_variable cv;
static uint64_t gInjectUtc = 0;
static bool gEnableUtcReport = false;
static shared_ptr<ICv2xRadioManager> gCv2xRadioMgr = nullptr;
static shared_ptr<ITimeManager> gTimeMgr = nullptr;
static shared_ptr<ITimeListener> gTimeListener = nullptr;

class UtcListener : public ITimeListener {
public:
    void onCv2xUtcTimeUpdate(const uint64_t utcInMs) override {
        cout << "------sys time(ms):" << Utils::getCurrentTimestamp()/1000;
        cout << "------" << endl;
        cout << "utcTime:" << utcInMs << endl;
    }
};

static void installSignalHandler() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGHUP);
    SignalHandlerCb cb = [](int sig) {
        std::unique_lock<std::mutex> lck(mtx);
        gExit = true;
        cv.notify_all();
    };

    SignalHandler::registerSignalHandler(sigset, cb);
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << endl;
    cout << " -i <utc> - Inject coarse UTC in units of millisecond" << endl;
    cout << " -l - Listen to UTC reports until exit using CTRL+C" << endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int c;
    while ((c = getopt(argc, argv, "?hi:l")) != -1) {
        switch (c) {
        case 'i':
            if (optarg) {
                gInjectUtc = atoll(optarg);
            }
            break;
        case 'l':
            gEnableUtcReport = true;
            break;
        case 'h':
        case '?':
        default:
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int initCv2x() {
    bool statusUpdate = false;
    telux::common::ServiceStatus cv2xRadioMgrStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        statusUpdate = true;
        cv2xRadioMgrStatus = status;
        cv.notify_all();
    };

    auto & cv2xFactory = Cv2xFactory::getInstance();
    gCv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!gCv2xRadioMgr) {
        cerr << "Failed to get Cv2xRadioManager" << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return statusUpdate; });
        if (gExit || telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioMgrStatus) {
            cerr << "CV2X Radio Manager initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int injectUtc() {
    bool getResponse = false;
    auto response = ErrorCode::GENERIC_FAILURE;
    auto injectUtcCb = [&](telux::common::ErrorCode errorCode) {
        std::lock_guard<std::mutex> lock(mtx);
        getResponse = true;
        response = errorCode;
        cv.notify_all();
    };
    if (Status::SUCCESS == gCv2xRadioMgr->injectCoarseUtcTime(gInjectUtc, injectUtcCb)) {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return getResponse; });
    }

    if (gExit || ErrorCode::SUCCESS != response) {
        cerr << "Failed to inject UTC" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int registerUtcReport() {
    try {
        gTimeListener = std::make_shared<UtcListener>();
    } catch (std::bad_alloc& e) {
        cerr << "Error CV2X UTC Listener allocation" << endl;
        return EXIT_FAILURE;
    }

    bool statusUpdated = false;
    auto servicStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    TimeTypeMask capabilities;
    auto statusCb = [&statusUpdated, &servicStatus](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        statusUpdated = true;
        servicStatus = status;
        cv.notify_all();
    };
    gTimeMgr = PlatformFactory::getInstance().getTimeManager(statusCb);
    if (gTimeMgr) {
        // wait for utc manager to be ready
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&statusUpdated] { return statusUpdated; });
    }

    if (gExit) {
        return EXIT_FAILURE;
    }

    if (servicStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        cout << "Time manager is ready" << endl;
    } else {
        cerr << "Unable to initialize time manager" << endl;
        return EXIT_FAILURE;
    }

    TimeTypeMask mask;
    mask.set(SupportedTimeType::CV2X_UTC_TIME);
    if (Status::SUCCESS != gTimeMgr->registerListener(gTimeListener, mask)) {
        cerr << "Failed to register time listener" << endl;
        gTimeListener = nullptr;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int deregisterUtcReport() {
    TimeTypeMask mask;
    mask.set(SupportedTimeType::CV2X_UTC_TIME);
    if (gTimeListener and
        Status::SUCCESS != gTimeMgr->deregisterListener(gTimeListener, mask)) {
        cerr << "Failed to deregister CV2X UTC listener" << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    cout << "Running CV2X SLSS UTC Test APP" << endl;

    std::vector<std::string> groups{"system", "diag", "radio", "locclient", "logd", "dlt"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << endl;
    }

    installSignalHandler();

    if (parseOpts(argc, argv)){
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;
    if (gInjectUtc > 0) {
        if (initCv2x() or injectUtc()) {
            ret = EXIT_FAILURE;
        }
    }

    if (gEnableUtcReport) {
        if (registerUtcReport()) {
            return EXIT_FAILURE;
        }

        cout << "Start listening to CV2X UTC reports, press CTRL+C to exit." << endl;

        // wait for exit
        std::unique_lock<std::mutex> lck(mtx);
        while (!gExit) {
            cv.wait(lck);
        }

        ret = deregisterUtcReport();
    }

    return ret;
}
