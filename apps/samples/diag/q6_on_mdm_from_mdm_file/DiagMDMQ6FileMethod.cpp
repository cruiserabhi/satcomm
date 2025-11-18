/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how an application running on the MDM can collect
 * diagnostics logs from the modem DSP (Q6) on the MDM device and save them on file(s).
 * The steps are as follows:
 *
 * 1. Get a DiagnosticsFactory instance.
 * 2. Get a IDiagLogManager instance from the DiagnosticsFactory.
 * 3. Wait for the diag service to become available.
 * 4. Set configuration for the log collection.
 * 5. Start collecting the diagnostics logs.
 * 6. Execute the application specific use case.
 * 7. Finally, when the use case is over, stop the log collection.
 *
 * Usage:
 * # ./diag_mdm_q6_collect <mdm-mask-file>
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/diag/DiagLogManager.hpp>
#include <telux/platform/diag/DiagnosticsFactory.hpp>

class DiagLogCollector : public std::enable_shared_from_this<DiagLogCollector> {
 public:
    int init() {
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

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int setConfiguration(std::string mdmMaskFile) {
        telux::common::ErrorCode ec;
        telux::platform::diag::DiagConfig fileMethodCfg{};

        fileMethodCfg.method = telux::platform::diag::LogMethod::FILE;
        fileMethodCfg.srcType = telux::platform::diag::SourceType::PERIPHERAL;
        fileMethodCfg.srcInfo.peripheral =
            telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_MODEM_DSP;
        fileMethodCfg.mdmLogMaskFile = mdmMaskFile;
        fileMethodCfg.modeType = telux::platform::diag::DiagLogMode::STREAMING;

        /* Step - 4 */
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

        /* Step - 5 */
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

        /* Step - 7 */
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
    std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<DiagLogCollector> app;

    if (argc != 2) {
        std::cout << "Usage: ./diag_mdm_q6_collect <mdm-mask-file>" << std::endl;
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
        return ret;
    }

    ret = app->startCollection();
    if (ret < 0) {
        return ret;
    }

    /* Step - 6 */
    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    ret = app->stopCollection();
    if (ret < 0) {
        return ret;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Application exiting" << std::endl;
    return 0;
}
