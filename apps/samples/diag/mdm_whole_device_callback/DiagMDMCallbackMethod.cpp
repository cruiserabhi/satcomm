/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how an application running on the MDM can collect
 * diagnostics logs from all the peripherals on the MDM device in a callback function.
 * The steps are as follows:
 *
 * 1. Get a DiagnosticsFactory instance.
 * 2. Get a IDiagLogManager instance from the DiagnosticsFactory.
 * 3. Wait for the diag service to become available.
 * 4. Define listener that will receive logs and register it.
 * 5. Set configuration for the log collection.
 * 6. Start collecting diagnostics logs.
 * 7. Execute the application specific use case.
 * 8. Stop the log collection when the use case is over.
 * 9. Finally, deregister the listener.
 *
 * Usage:
 * # ./diag_mdm_dev_callback <mdm-mask-file>
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <cstdio>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/diag/DiagLogManager.hpp>
#include <telux/platform/diag/DiagnosticsFactory.hpp>

class LogsReceiver : public telux::platform::diag::IDiagListener {
 public:
    void onAvailableLogs(uint8_t *data, int length) {
        std::cout << "onAvailableLogs: length " << length << std::endl;

        /* Print data in hexadecimal format (N row * 32 columns) */
        for (int x = 0; x < length; x++) {
            printf("%02x ", data[x] & 0xffU);
            if (x && !((x+1) % 32) && (x != (length - 1))) {
                printf("\n");
            }
        }
        printf("\n");
    };
};

class DiagLogCollector : public std::enable_shared_from_this<DiagLogCollector> {
 public:
    int init() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &diagFactory = telux::platform::diag::DiagnosticsFactory::getInstance();

        /* Step - 2 */
        diagMgr_ = diagFactory.getDiagLogManager(
                [&p](telux::common::ServiceStatus srvStatus) {
            p.set_value(srvStatus);
        });

        if (!diagMgr_) {
            std::cout << "Can't get IDiagLogManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Diag service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
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

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        /* Step - 9 */
        status = diagMgr_->deregisterListener(logsReceiver_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Deregistered listener" << std::endl;
        return 0;
    }

    int setConfiguration(std::string mdmMaskFile) {
        telux::common::ErrorCode ec;
        telux::platform::diag::DiagConfig fileMethodCfg{};

        fileMethodCfg.method = telux::platform::diag::LogMethod::CALLBACK;
        fileMethodCfg.srcType = telux::platform::diag::SourceType::DEVICE;
        fileMethodCfg.srcInfo.device = telux::platform::diag::DeviceType::DIAG_DEVICE_MDM;
        fileMethodCfg.mdmLogMaskFile = mdmMaskFile;
        fileMethodCfg.modeType = telux::platform::diag::DiagLogMode::STREAMING;

        /* Step - 5 */
        ec = diagMgr_->setConfig(fileMethodCfg);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't config, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        std::cout << "Config set" << std::endl;
        return 0;
    }

    int startCollection() {
        telux::common::ErrorCode ec;

        /* Step - 6 */
        ec = diagMgr_->startLogCollection();
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't start collection, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        std::cout << "Collection started" << std::endl;
        return 0;
    }

    int stopCollection() {
        telux::common::ErrorCode ec;

        /* Step - 8 */
        ec = diagMgr_->stopLogCollection();
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't stop collection, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        std::cout << "Collection stopped" << std::endl;
        return 0;
    }

 private:
    std::shared_ptr<LogsReceiver> logsReceiver_;
    std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<DiagLogCollector> app;

    if (argc != 2) {
        std::cout << "Usage: ./diag_mdm_dev_callback <mdm-mask-file>" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<DiagLogCollector>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate DiagLogCollector" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->setConfiguration(argv[1]);
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->startCollection();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    /* Step - 6 */
    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(8));

    ret = app->stopCollection();
    if (ret < 0) {
        return ret;
    }

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
