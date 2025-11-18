/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xSlssRxTest.cpp
 *
 * @brief: Simple application that demonstrates how to get Cv2x SLSS Rx infromation.
 */

#include <iostream>
#include <future>
#include <map>
#include <mutex>
#include <string>
#include <signal.h>

#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>

#include "../../common/utils/Utils.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::map;
using std::promise;
using std::string;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::SlssRxInfo;
using telux::cv2x::SlssSyncPattern;
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xRadioManager;

static int gTerminate = 0;
static int gTerminatePipe[2];
static bool gListenMode = false;
static promise<ErrorCode> gCallbackPromise;

static map<SlssSyncPattern, string> gCv2xSlssPatternToString = {
    { SlssSyncPattern::OFFSET_IND_1, "OFFSET_IND_1" },
    { SlssSyncPattern::OFFSET_IND_2, "OFFSET_IND_2" },
    { SlssSyncPattern::OFFSET_IND_3, "OFFSET_IND_3" },
    { SlssSyncPattern::ODD_RESERVED, "ODD_RESERVED" },
    { SlssSyncPattern::EVEN_RESERVED, "EVEN_RESERVED" },
    { SlssSyncPattern::UNKNOWN, "UNKNOWN" },
};

static void printSlssRxInfo(const SlssRxInfo& info) {
    cout << "Number of syncRefUE:" << info.ueInfo.size() << endl;
    for (auto i = 0u; i < info.ueInfo.size(); ++i) {
        cout << " UE[" << i << "]:" << endl;
        cout << "  slssID:" << info.ueInfo[i].slssId;
        cout << ", inCoverage:" << std::boolalpha << info.ueInfo[i].inCoverage;
        cout << ", pattern:" << gCv2xSlssPatternToString[info.ueInfo[i].pattern];
        cout << ", rsrp:" << static_cast<uint32_t>(info.ueInfo[i].rsrp);
        cout << ", selected:" << std::boolalpha << info.ueInfo[i].selected << endl;
    }
}

class SlssListener : public ICv2xListener {
public:
    void onSlssRxInfoChanged(const SlssRxInfo& slssInfo) override {
        printSlssRxInfo(slssInfo);
    }
};

// Callback function for Cv2xRadioManager->getSlssRxInfo(Cv2xStatusEx)
static void getSlssRxInfoCallback(const SlssRxInfo& info, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        printSlssRxInfo(info);
    }
    gCallbackPromise.set_value(error);
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << endl;
    cout << " none option - Get V2X SLSS Rx info for one time" << endl;
    cout << " option -l - Listen to V2X SLSS Rx info updates until exit using CTRL+C" << endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int rc = 0;
    int c;
    while ((c = getopt(argc, argv, "?hl")) != -1) {
        switch (c) {
        case 'l':
            gListenMode = true;
            break;
        case 'h':
        case '?':
        default:
            rc = -1;
            printUsage(argv[0]);
            return rc;
        }
    }

    return rc;
}


static void terminationHandler(int signum)
{
    gTerminate = 1;
    write(gTerminatePipe[1], &gTerminate, sizeof(int));
}

static void installSignalHandler()
{
    struct sigaction sig_action;

    sig_action.sa_handler = terminationHandler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    sigaction(SIGINT, &sig_action, NULL);
    sigaction(SIGHUP, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
}

int main(int argc, char *argv[]) {
    cout << "Running CV2X SLSS Rx Info APP" << endl;

    std::vector<std::string> groups{"system", "diag", "radio", "logd", "dlt"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << std::endl;
    }

    // Parse parameters, set V2X status type
    if (parseOpts(argc, argv)){
        return EXIT_FAILURE;
    }

    if (gListenMode) {
        if (pipe(gTerminatePipe) == -1) {
            cerr << "Pipe error" << endl;
            return EXIT_FAILURE;
        }
        installSignalHandler();
    }

    int ret = EXIT_SUCCESS;
    std::shared_ptr<ICv2xListener> slssListener;
    std::shared_ptr<ICv2xRadioManager> cv2xRadioManager;
    do {
        bool cv2xRadioManagerStatusUpdated = false;
        telux::common::ServiceStatus cv2xRadioManagerStatus =
            telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
        std::condition_variable cv;
        std::mutex mtx;
        auto statusCb = [&](telux::common::ServiceStatus status) {
            std::lock_guard<std::mutex> lock(mtx);
            cv2xRadioManagerStatusUpdated = true;
            cv2xRadioManagerStatus = status;
            cv.notify_all();
        };
        // Get handle to Cv2xRadioManager
        auto & cv2xFactory = Cv2xFactory::getInstance();
        cv2xRadioManager = cv2xFactory.getCv2xRadioManager(statusCb);
        if (!cv2xRadioManager) {
            cout << "Error: failed to get Cv2xRadioManager." << endl;
            ret = EXIT_FAILURE;
            break;
        }
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioManagerStatus) {
            cout << "Error: failed to initialize Cv2xRadioManager." << endl;
            ret = EXIT_FAILURE;
            break;
        }

        if (gListenMode) {
            slssListener = std::make_shared<SlssListener>();
            if (Status::SUCCESS != cv2xRadioManager->registerListener(slssListener)) {
                cerr << "Register CV2X SLSS Rx listener failed!"<< endl;
                slssListener = nullptr;
                ret = EXIT_FAILURE;
                break;
            }
        }

        // Get CV2X SLSS Rx info
        if (Status::SUCCESS != cv2xRadioManager->getSlssRxInfo(getSlssRxInfoCallback)
            or ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Error: failed to get CV2X SLSS Rx info." << endl;
            // for listening mode, listen to indications even if the first query failed
            if (not gListenMode) {
                return EXIT_FAILURE;
            }
        }
    } while(0);

    if (gListenMode) {
        if (EXIT_SUCCESS == ret) {
            cout << "Enter listening mode, press CTRL+C to exit." << endl;

            int terminate = 0;
            read(gTerminatePipe[0], &terminate, sizeof(int));
            cout << "Termination!" << endl;
        }

        if (slssListener and
            cv2xRadioManager and
            Status::SUCCESS != cv2xRadioManager->deregisterListener(slssListener)) {
            cerr << "Deregister CV2X SLSS Rx listener failed!"<< endl;
            ret = EXIT_FAILURE;
        }

        close(gTerminatePipe[0]);
        close(gTerminatePipe[1]);
    }

    return ret;
}
