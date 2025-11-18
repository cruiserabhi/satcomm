/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * This application demonstrates how to register a listener and receive GNSS time data
 * from the Location APIs. The steps are as follows:
 *
 * 1. Get TimeManager from PlatformFactory.
 * 2. Set TimeTypeMask to GNSS_UTC_TIME.
 * 3. Register a listener to receive GNSS time data.
 * 4. Wait until user terminates the app.
 * 5. Deregister the listener upon user termination.
 *
 * Usage:
 * ./utc_info_listener_app
 *
 */

#include <errno.h>
#include <atomic>
#include <iostream>
#include <mutex>
#include <future>
#include <condition_variable>
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/TimeManager.hpp>
#include <telux/platform/TimeListener.hpp>

#include "SignalHandler.hpp"

using telux::platform::PlatformFactory;
using telux::platform::ITimeManager;
using telux::platform::ITimeListener;

// Used to get the Telux async result
std::mutex mtx;
std::condition_variable cv;

static bool gExit = false;

using namespace telux::platform;
using namespace telux::common;

class UtcInfoListener : public ITimeListener,
                        public std::enable_shared_from_this<UtcInfoListener> {
   public:
    int initTimeListener() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &platformFactory = PlatformFactory::getInstance();

        /* Step - 2 */
        timeManager_  = platformFactory.getTimeManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!timeManager_) {
            std::cout << "Can't get ITimeListenerManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Time Listener service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Time manager is ready" << std::endl;
        return 0;
    }

    int startReports() {
        mask_.set(SupportedTimeType::GNSS_UTC_TIME);

        auto status = timeManager_->registerListener(shared_from_this(), mask_);
        if (status != Status::SUCCESS) {
            std::cout << "Unable to initialize time manager" << std::endl;
            return -EINVAL;
        }

        std::cout << "Started providing fixes" << std::endl;
        return 0;
    }

    void stopReports() {
        timeManager_->deregisterListener(shared_from_this(), mask_);
    }

    void onGnssUtcTimeUpdate(const uint64_t utc) {
        // ignore invalid utc
        if (utc == 0) {
            return;
        }

        if (utc % 1000 == 0) {
            std::cout << "GNSS report with UTC = " << utc << std::endl;
        } else {
            std::cout << "GNSS report ignored with UTC = " << utc << std::endl;
        }
    }

 private:
    std::shared_ptr<telux::platform::ITimeManager> timeManager_;
    TimeTypeMask mask_;
};

int main(int argc, char **argv) {

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

    int ret;
    std::shared_ptr<UtcInfoListener> app;

    try {
        app = std::make_shared<UtcInfoListener>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate UtcInfoListener" << std::endl;
        return -ENOMEM;
    }

    /** Step - 1 */
    ret = app->initTimeListener();
    if (ret < 0) {
        return ret;
    }

    /** Step - 2 */
    ret = app->startReports();
    if (ret < 0) {
        return ret;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        while (!gExit) {
            cv.wait(lck);
        }
    }

    /** Step - 3 */
    app->stopReports();

    return 0;
}