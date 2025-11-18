/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xMacCloneAttackTest.cpp
 *
 * @brief: Simple application that demonstrates how to register and deregister
 * a listener for MAC cloning attack indication.
 */

#include <iostream>
#include <future>
#include <mutex>
#include <string>
#include <signal.h>

#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xRadioListener.hpp>

#include "../../common/utils/Utils.hpp"
#include "../../common/utils/SignalHandler.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::make_shared;
using std::shared_ptr;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xRadioListener;

static bool gExit = false;
static std::mutex mtx;
static std::condition_variable cv;
static shared_ptr<ICv2xRadio> gCv2xRadio = nullptr;
static shared_ptr<ICv2xRadioListener> gCv2xListener = nullptr;

class MacCloneAttackListener : public ICv2xRadioListener {
public:
    void onMacAddressCloneAttack(const bool detected) override {
        cout << "------sys time:" << Utils::getCurrentTimeString();
        cout << "------" << endl;
        cout << "mac cloning attack detect:" << detected << endl;
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

static int initCv2x() {
    bool statusUpdate = false;
    telux::common::ServiceStatus cv2xStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        statusUpdate = true;
        cv2xStatus = status;
        cv.notify_all();
    };

    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!cv2xRadioMgr) {
        cerr << "Failed to get cv2x radio manager" << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return (statusUpdate); });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE != cv2xStatus) {
            cerr << "CV2X radio Manager initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }
    if (gExit) {
         cerr << "gExit==true, aborting" << endl;
         return EXIT_FAILURE;;
    }

    // init cv2x radio
    statusUpdate = false;
    cv2xStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    gCv2xRadio = cv2xRadioMgr->getCv2xRadio(telux::cv2x::TrafficCategory::SAFETY_TYPE, statusCb);
    if (!gCv2xRadio) {
        cerr << "Failed to get cv2x radio" << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return (statusUpdate); });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE != cv2xStatus) {
            cerr << "CV2X radio initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }
    if (gExit) {
         cerr << "Exiting, abort" << endl;
         return EXIT_FAILURE;;
    }

    // register listener for mac cloning attack indications
    try {
        gCv2xListener = std::make_shared<MacCloneAttackListener>();
    } catch (std::bad_alloc& e) {
        cerr << "Error cv2x listener allocation" << endl;
        return EXIT_FAILURE;
    }

    if (Status::SUCCESS != gCv2xRadio->registerListener(gCv2xListener)) {
        cerr << "Failed to register cv2x listener" << endl;
        gCv2xListener = nullptr;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    cout << "Running CV2X Mac Clone Attack Test APP" << endl;

    std::vector<std::string> groups{"system", "diag", "radio", "logd", "dlt"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << endl;
    }

    installSignalHandler();

    if (initCv2x()) {
        return EXIT_FAILURE;
    }

    cout << "Start listening to mac cloning attack indications, press CTRL+C to exit." << endl;

    // wait for exit
    std::unique_lock<std::mutex> lck(mtx);
    while (!gExit) {
        cv.wait(lck);
    }

    if (gCv2xRadio) {
        if (gCv2xListener) {
            gCv2xRadio->deregisterListener(gCv2xListener);
        }
        gCv2xRadio = nullptr;
    }

    return EXIT_SUCCESS;
}
