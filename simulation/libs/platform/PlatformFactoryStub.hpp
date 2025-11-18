/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       PlatformFactoryStub.hpp
 *
 * @brief      PlatformFactory creates a set of managers which provide the corresponding
 *             platform services.
 *
 */

#ifndef PLATFORM_FACTORY_STUB_HPP
#define PLATFORM_FACTORY_STUB_HPP

#include <memory>
#include <mutex>

#include "telux/platform/PlatformFactory.hpp"
#include "DeviceInfoManagerStub.hpp"
#include "AntennaManagerStub.hpp"
#include "FsManagerStub.hpp"
#include "TimeManagerStub.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/FactoryHelper.hpp"


using namespace telux::common;

namespace telux {
namespace platform {

class PlatformFactoryStub : public PlatformFactory,
                            public telux::common::FactoryHelper {
 public:
    static PlatformFactory &getInstance();

    std::shared_ptr<IFsManager> getFsManager(InitResponseCb callback = nullptr) override;
    std::shared_ptr<IDeviceInfoManager> getDeviceInfoManager(
        InitResponseCb callback = nullptr) override;
    std::shared_ptr<ITimeManager> getTimeManager(InitResponseCb callback = nullptr) override;
    std::shared_ptr<hardware::IAntennaManager> getAntennaManager(
        InitResponseCb callback = nullptr) override;

 private:
    PlatformFactoryStub();
    ~PlatformFactoryStub();

    std::mutex platformFactoryMutex_;
    std::weak_ptr<IDeviceInfoManager> deviceInfoManager_;
    std::vector<InitResponseCb> deviceInfoInitCallbacks_;
    std::weak_ptr<hardware::IAntennaManager> antennaManager_;
    std::vector<InitResponseCb> antennaInitCallbacks_;
    std::weak_ptr<IFsManager> fsManager_;
    std::vector<InitResponseCb> fsInitCallbacks_;
    std::weak_ptr<ITimeManager> timeManager_;
    std::vector<InitResponseCb> timeInitCallbacks_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};

}  // end of namespace platform

}  // end of namespace telux

#endif  // PLATFORM_FACTORY_STUB_HPP
