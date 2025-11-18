/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: TeluxVersionApp.cpp
 *
 * @brief: Simple application that queries platform versions.
 */

#include <future>
#include <string>
#include <vector>
#include <condition_variable>

#include <telux/common/Version.hpp>
#include "../../common/utils/Utils.hpp"
#include <telux/platform/PlatformFactory.hpp>

using std::cout;
using std::endl;
using std::string;
using namespace telux::common;
using namespace telux::platform;


int main(int argc, char *argv[]) {
    cout << "Running telux version app" << endl;
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt", "firmware"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc < 0) {
       std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    auto &platformFactory = PlatformFactory::getInstance();

    std::promise<ServiceStatus> p;
    auto initCb = [&p](ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
    };
    std::shared_ptr<IDeviceInfoManager> deviceInfoManager = platformFactory.getDeviceInfoManager(initCb);
    if (deviceInfoManager == nullptr) {
        std::cout << "DeviceInfo manager is nullptr" << std::endl;
        exit(1);
    }
    std::cout << "Obtained deviceInfo manager" << std::endl;
    p.get_future().get();
    if (deviceInfoManager->getServiceStatus() != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "DeviceInfo service not available" << std::endl;
        exit(1);
    }
    PlatformVersion version;
    if (Status::SUCCESS == deviceInfoManager->getPlatformVersion(version)) {
        auto sdkVersion = telux::common::Version::getSdkVersion();
        cout << "Request telux version success" << endl << "modem: " << version.modem
            << endl << "meta: " << version.meta << endl << "externalApp: "
            << version.externalApp << endl << "integratedApp: " << version.integratedApp
            << endl << "SDK: " << std::to_string(sdkVersion.major) << "."
            << std::to_string(sdkVersion.minor) << "." << std::to_string(sdkVersion.patch) << endl;
    } else {
        cout << "Error : request for telux version failed." << endl;
    }
    deviceInfoManager = nullptr;
    return 0;
}
