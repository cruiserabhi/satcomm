/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <telux/common/Log.hpp>
#include "common/CommonUtils.hpp"
#include "PlatformFactoryStub.hpp"
#include <thread>
#include <exception>
#include <memory>

#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace platform {

PlatformFactory::PlatformFactory() {
    LOG(DEBUG, __FUNCTION__);
}

PlatformFactory &PlatformFactory::getInstance() {
    return PlatformFactoryStub::getInstance();
}

PlatformFactory::~PlatformFactory() {
    LOG(DEBUG, __FUNCTION__);
}

PlatformFactoryStub::PlatformFactoryStub() {
    LOG(DEBUG, __FUNCTION__);

    telux::common::CommonUtils::logSdkVersion();

    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

PlatformFactoryStub::~PlatformFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
}

PlatformFactory &PlatformFactoryStub::getInstance() {
    static PlatformFactoryStub instance;
    return instance;
}

std::shared_ptr<IDeviceInfoManager> PlatformFactoryStub::getDeviceInfoManager(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::function<std::shared_ptr<IDeviceInfoManager>(InitResponseCb)> createAndInit
        = [this](telux::common::InitResponseCb initCb) -> std::shared_ptr<IDeviceInfoManager> {
        std::shared_ptr<DeviceInfoManagerStub> manager = std::make_shared<DeviceInfoManagerStub>();
        if (telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };

    auto type = std::string("DeviceInfo manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
        " , callback = ", &deviceInfoInitCallbacks_);

    auto manager = getManager<telux::platform::IDeviceInfoManager>(type, deviceInfoManager_,
        deviceInfoInitCallbacks_, callback, createAndInit);

    return manager;
}

std::shared_ptr<hardware::IAntennaManager> PlatformFactoryStub::getAntennaManager(
    InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::function<std::shared_ptr<hardware::IAntennaManager>(InitResponseCb)> createAndInit
        = [this](telux::common::InitResponseCb initCb) ->
        std::shared_ptr<hardware::IAntennaManager> {
        std::shared_ptr<hardware::AntennaManagerStub> manager =
        std::make_shared<hardware::AntennaManagerStub>();
        if (telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Antenna manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
        " , callback = ", &antennaInitCallbacks_);
    auto manager = getManager<telux::platform::hardware::IAntennaManager>(type, antennaManager_,
        antennaInitCallbacks_, callback, createAndInit);
    return manager;
}

std::shared_ptr<IFsManager> PlatformFactoryStub::getFsManager(InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::function<std::shared_ptr<IFsManager>(InitResponseCb)> createAndInit
        = [this](telux::common::InitResponseCb initCb) -> std::shared_ptr<IFsManager> {
        std::shared_ptr<FsManagerStub> manager = std::make_shared<FsManagerStub>();
        if (telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Fs manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
        " , callback = ", &fsInitCallbacks_);
    auto manager = getManager<telux::platform::IFsManager>(type, fsManager_,
        fsInitCallbacks_, callback, createAndInit);
    return manager;
}

std::shared_ptr<ITimeManager> PlatformFactoryStub::getTimeManager(InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::function<std::shared_ptr<ITimeManager>(InitResponseCb)> createAndInit
        = [this](telux::common::InitResponseCb initCb) -> std::shared_ptr<ITimeManager> {
        std::shared_ptr<TimeManagerStub> manager = std::make_shared<TimeManagerStub>();
        if (telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Time manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
        " , callback = ", &timeInitCallbacks_);
    auto manager = getManager<telux::platform::ITimeManager>(type, timeManager_,
        timeInitCallbacks_, callback, createAndInit);
    return manager;
}



}  // end namespace platform
}  // end namespace telux
