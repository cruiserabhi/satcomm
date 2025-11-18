/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include <telux/common/CommonDefines.hpp>

#include "CollectionMethod.hpp"

/*
 * Constructor.
 */
CollectionMethod::CollectionMethod(std::string menuTitle, std::string cursor,
    std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr)
    : ConsoleApp(menuTitle, cursor) {

    diagMgr_ = diagMgr;
}

/*
 * Destructor.
 */
CollectionMethod::~CollectionMethod() {
}

/*
 * Set configuration.
 */
void CollectionMethod::setConfig(telux::platform::diag::LogMethod collectionMethod) {
    telux::common::ErrorCode ec{};
    telux::platform::diag::DiagConfig cfg{};

    cfg.method = collectionMethod;
    usrInput.takeConfiguration(cfg);

    ec = diagMgr_->setConfig(cfg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't config, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Configuration set successfully" << std::endl;
}

/*
 * Get currently set configuration.
 */
void CollectionMethod::getConfig() {
    std::string conf;
    telux::platform::diag::DiagConfig config{};

    config = diagMgr_->getConfig();

    std::cout << "Current configuration:" << std::endl;

    conf += "source type : " + std::to_string(static_cast<int>(config.srcType));

    if(config.srcType == telux::platform::diag::SourceType::DEVICE) {
        conf += "\nsource info device : " +
            std::to_string(static_cast<int>(config.srcInfo.device));
    } else {
        conf += "\nsource info peripheral : " +
            std::to_string(static_cast<int>(config.srcInfo.peripheral));
    }

    conf += "\nmdm mask path : " + config.mdmLogMaskFile;
    /* conf += "\neap mask path : " + config.eapLogMaskFile; unsupported */
    conf += "\nmode type : " + std::to_string(static_cast<int>(config.modeType));
    conf += "\nlog method : " + std::to_string(static_cast<int>(config.method));
    conf += "\nmax file size : " + std::to_string(config.methodConfig.fileConfig.maxSize);
    conf += "\nmax file count : " + std::to_string(config.methodConfig.fileConfig.maxNumber);

    std::cout << conf << std::endl;
}

/*
 * Start log collection.
 */
void CollectionMethod::startCollection() {
    telux::common::ErrorCode ec{};

    ec = diagMgr_->startLogCollection();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't start collection, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Collection started" << std::endl;
}

/*
 * Stop log collection.
 */
void CollectionMethod::stopCollection() {
    telux::common::ErrorCode ec{};

    ec = diagMgr_->stopLogCollection();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't stop collection, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Collection stopped" << std::endl;
}

/*
 * Get's IDiagLogManager service status.
 */
void CollectionMethod::getServiceStatus() {
    telux::common::ServiceStatus srvStatus{};

    srvStatus = diagMgr_->getServiceStatus();

    switch(srvStatus) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            std::cout << "Service status : available" << std::endl;
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            std::cout << "Service status : unavailable" << std::endl;
            break;
        case telux::common::ServiceStatus::SERVICE_FAILED:
            std::cout << "Service status : failed" << std::endl;
            break;
        default:
            std::cout << "Service status : unknown" << std::endl;
    }
}