/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file: ImeiTestApp.cpp
 *
 * @brief: Simple application that queries IMEI.
 */

#include <future>
#include <string>
#include <vector>
#include <condition_variable>

#include "../../common/utils/Utils.hpp"
#include <telux/platform/PlatformFactory.hpp>

using std::cout;
using std::endl;
using std::string;
using namespace telux::common;
using namespace telux::platform;


int main(int argc, char *argv[]) {
    cout << "Running IMEI test app" << endl;
    auto &platformFactory = PlatformFactory::getInstance();

    std::promise<ServiceStatus> p;
    auto initCb = [&p](ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
    };
    std::shared_ptr<IDeviceInfoManager> deviceInfoManager =
        platformFactory.getDeviceInfoManager(initCb);
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
    std::string imei = "";
    if (Status::SUCCESS == deviceInfoManager->getIMEI(imei)) {
        cout << "Request IMEI successfully: " << imei << endl;
    } else {
        cout << "Error : request for IMEI failed." << endl;
    }
    return 0;
}
