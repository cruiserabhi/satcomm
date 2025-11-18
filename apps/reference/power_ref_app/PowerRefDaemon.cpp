/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <csignal>
#include <future>

extern "C"
{
#include <getopt.h>
}

#include "PowerRefDaemon.hpp"
#include <telux/common/DeviceConfig.hpp>

PowerRefDaemon &PowerRefDaemon::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static PowerRefDaemon instance;
    return instance;
}

telux::common::Status PowerRefDaemon::init() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status initStatus = telux::common::Status::SUCCESS;
    config_ = ConfigParser::getInstance();

    do {
        shared_ptr<EventManager> eventManager(EventManager::getInstance());
        if (eventManager && eventManager->init()) {
            LOG(DEBUG, __FUNCTION__, " eventManager init succeed");
            eventManager_ = eventManager;
        } else {
            LOG(ERROR, __FUNCTION__, " eventManager init failed");
            initStatus = telux::common::Status::FAILED;
            break;
        }

        if (config_->getValue("TRIGGER", "NAOIP_TRIGGER") == "ENABLE") {
            naoIpTrigger_ = make_shared<NAOIpTrigger>(eventManager);
            if (naoIpTrigger_ && naoIpTrigger_->init()) {
                LOG(DEBUG, __FUNCTION__, " naoIpTrigger init succeed");
            } else {
                LOG(ERROR, __FUNCTION__, " naoIpTrigger init failed");
                initStatus = telux::common::Status::FAILED;
                break;
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " naoIpTrigger ",
                config_->getValue("TRIGGER", "NAOIP_TRIGGER"));
        }

        if (config_->getValue("TRIGGER", "SMS_TRIGGER") == "ENABLE") {
            smsTrigger_ = make_shared<SMSTrigger>(eventManager);
            if (smsTrigger_ && smsTrigger_->init()) {
                LOG(DEBUG, __FUNCTION__, " smsTrigger init succeeded");
            } else {
                LOG(ERROR, __FUNCTION__, " smsTrigger init failed");
                initStatus = telux::common::Status::FAILED;
                break;
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " smsTrigger ", config_->getValue("TRIGGER", "SMS_TRIGGER"));
        }

        if (config_->getValue("TRIGGER", "CAN_TRIGGER") == "ENABLE") {
#ifdef CAN_TRIGGER_SUPPORTED
                canTrigger_ = CANTrigger::getInstance(eventManager);
                if (canTrigger_ && canTrigger_->init()) {
                    LOG(DEBUG, __FUNCTION__, " canTrigger init succeeded");
                } else {
                    LOG(ERROR, __FUNCTION__, " canTrigger init failed");
                    initStatus = telux::common::Status::FAILED;
                    break;
                }
#else // CAN_TRIGGER_SUPPORTED
                LOG(ERROR, " CAN trigger is not supported");
#endif // CAN_TRIGGER_SUPPORTED

        } else {
            LOG(DEBUG, __FUNCTION__, " CAN trigger ", config_->getValue("TRIGGER", "CAN_TRIGGER"));
        }
    } while (0);

    return initStatus;
}

int PowerRefDaemon::startDaemon(int argc, char **argv) {
    LOG(DEBUG, __FUNCTION__);
    if (parseArguments(argc, argv) != telux::common::Status::SUCCESS) {
        return EXIT_FAILURE;
    }

    struct sigaction sigAction;
    sigAction.sa_handler = signalHandler;
    sigaction(SIGHUP, &sigAction, NULL);
    sigaction(SIGINT, &sigAction, NULL);
    sigaction(SIGTERM, &sigAction, NULL);

    if (init() != telux::common::Status::SUCCESS) {

        if (eventManager_) {
            eventManager_ = nullptr;
        }
        if (naoIpTrigger_) {
            naoIpTrigger_ = nullptr;
        }
        if (smsTrigger_) {
            smsTrigger_ = nullptr;
        }
        return EXIT_FAILURE;
    }

    {
        // block current thread, till we get signal
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this]
                 { return exiting_; });
    }
    return EXIT_SUCCESS;
}

void PowerRefDaemon::stopDaemon() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mtx_);
    exiting_ = true;
    naoIpTrigger_.reset();
    eventManager_.reset();
    smsTrigger_.reset();
    fflush(stdout);
    cv_.notify_all();
}

void PowerRefDaemon::signalHandler(int signum) {
    LOG(DEBUG, __FUNCTION__, "Received signal = ",signum, " terminating program.");
    PowerRefDaemon::getInstance().stopDaemon();

    std::signal(signum, SIG_DFL);
    if (std::raise(signum) != 0) {
        LOG(ERROR, __FUNCTION__, "raise(): error \n");
    }
}

void PowerRefDaemon::printUsage(char **argv) {
    LOG(DEBUG, __FUNCTION__);
    LOG(DEBUG, __FUNCTION__, " Usage: ", string(argv[0]), " [options] ");
    LOG(DEBUG, __FUNCTION__, " Options: ");
    LOG(DEBUG, __FUNCTION__, " \t -h --help        Print helpful information");
    LOG(DEBUG, __FUNCTION__, " Example: ");
    LOG(DEBUG, __FUNCTION__, "    ./telux_power_refd ");
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status PowerRefDaemon::parseArguments(int argc, char **argv) {
    LOG(DEBUG, __FUNCTION__);
    int c;
    struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                    {"interface", required_argument, 0, 'i'},
                                    {0, 0, 0, 0}};
    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "dshi:", long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'h':
        default:
            printUsage(argv);
            return telux::common::Status::INVALIDPARAM;
            break;
        }
    }
    return telux::common::Status::SUCCESS;
}


using namespace std;

int main(int argc, char *argv[]) {
    // Setting required secondary groups for SDK file/diag logging
    vector<string> supplementaryGrps{"system", "diag", "radio", "logd", "dlt"};
    int rc =  Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        LOG(DEBUG, __FUNCTION__, " Adding supplementary groups failed ");
    }
    PowerRefDaemon::getInstance().startDaemon(argc, argv);

    return 0;
}
