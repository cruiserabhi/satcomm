/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <errno.h>

#include <cstdio>
#include <iostream>

#include "CallbackMethod.hpp"

/*
 * Callback - receives logs when using callback method.
 */
void LogsReceiver::onAvailableLogs(uint8_t *data, int length) {
    std::cout << "onAvailableLogs: length " << length << std::endl;

    /* Print data in hexadecimal format (N row * 32 columns) */
    for (int x = 0; x < length; x++) {
        printf("%02x ", data[x] & 0xffU);
        if (x && !((x+1) % 32) && (x != (length - 1))) {
            printf("\n");
        }
    }
    printf("\n");
}

/*
 * Constructor.
 */
CallbackMethod::CallbackMethod(std::string menuTitle, std::string cursor,
    std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr)
    : CollectionMethod(menuTitle, cursor, diagMgr) {
}

/*
 * Destructor.
 */
CallbackMethod::~CallbackMethod() {
    diagMgr_->deregisterListener(logsReceiver_);
}

/*
 * Drain logs from peripheral's local buffer to Apps buffer.
 */
void CallbackMethod::drainPeripheralBuffer() {
    telux::platform::diag::Peripherals peripherals{};
    telux::common::ErrorCode ec{};

    usrInput.takePeripheralsForDraining(peripherals);

    ec = diagMgr_->drainPeripheralBuffers(peripherals);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't drain, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Peripherals drained" << std::endl;
}

/*
 * Setup callback method relevant resources.
 */
int CallbackMethod::initCallbackMethod() {
    telux::common::Status status{};

    try {
        logsReceiver_ = std::make_shared<LogsReceiver>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate LogsReceiver" << std::endl;
        return -ENOMEM;
    }

    status = diagMgr_->registerListener(logsReceiver_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't register listener, err " <<
            static_cast<int>(status) << std::endl;
        return -EIO;
    }

    return 0;
}

/*
 * Prepare options applicable for callback method and display them.
 */
void CallbackMethod::showCallbackMenu() {
    std::shared_ptr<ConsoleAppCommand> setConfig = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "Set configuration", {},
        std::bind(&CallbackMethod::setConfig, this,
        telux::platform::diag::LogMethod::CALLBACK)));

    std::shared_ptr<ConsoleAppCommand> getConfig = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Get configuration", {},
        std::bind(&CallbackMethod::getConfig, this)));

    std::shared_ptr<ConsoleAppCommand> startCollection = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("3", "Start log collection", {},
        std::bind(&CallbackMethod::startCollection, this)));

    std::shared_ptr<ConsoleAppCommand> stopCollection = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("4", "Stop log collection", {},
        std::bind(&CallbackMethod::stopCollection, this)));

    std::shared_ptr<ConsoleAppCommand> drainBuffer = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("5", "Drain peripheral's buffer", {},
        std::bind(&CallbackMethod::drainPeripheralBuffer, this)));

    std::shared_ptr<ConsoleAppCommand> getSrvStatus = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("6", "Get service status", {},
        std::bind(&CallbackMethod::getServiceStatus, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> cbCmds = {setConfig,
        getConfig, startCollection, stopCollection, drainBuffer, getSrvStatus};

    ConsoleApp::addCommands(cbCmds);
    ConsoleApp::displayMenu();
    shared_from_this()->mainLoop();
}
