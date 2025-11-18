/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to get domain preferences. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a IServingSystemManager instance from the PhoneFactory.
 * 3. Wait for the serving system service to become available.
 * 4. Request service domain preference.
 * 5. Receive the service domain preferences.
 *
 * Usage:
 * # ./serving_system_app
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/ServingSystemManager.hpp>

class ServingSystemInfo : public std::enable_shared_from_this<ServingSystemInfo> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        auto servingSystemMgr_ = phoneFactory.getServingSystemManager(DEFAULT_SLOT_ID,
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!servingSystemMgr_) {
            std::cout << "Can't get IServingSystemManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Serving system service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int getServingSystemInfo() {
        telux::common::Status status;

        auto responseCb = std::bind(&ServingSystemInfo::serviceDomainResponse,
            this, std::placeholders::_1, std::placeholders::_2);

        /* Step - 4 */
        status = servingSystemMgr_->requestServiceDomainPreference(responseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request preference, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    /* Step - 5 */
    void serviceDomainResponse(telux::tel::ServiceDomainPreference preference,
        telux::common::ErrorCode errorCode) {

        if (errorCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to get preference" << std::endl;
            return;
        }

        std::cout << "Preference: " << getServiceDomain(preference) << std::endl;
    }

 private:
    std::string getServiceDomain(telux::tel::ServiceDomainPreference preference) {
        switch(preference) {
            case telux::tel::ServiceDomainPreference::CS_ONLY:
                return " Circuit Switched(CS) only";
            case telux::tel::ServiceDomainPreference::PS_ONLY:
                return " Packet Switched(PS) only";
            case telux::tel::ServiceDomainPreference::CS_PS:
                return " Circuit Switched and Packet Switched ";
            default:
                return " Unknown";
        }
    }

    std::shared_ptr<telux::tel::IServingSystemManager> servingSystemMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<ServingSystemInfo> app;

    try {
        app = std::make_shared<ServingSystemInfo>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate ServingSystemInfo" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->getServingSystemInfo();
    if (ret < 0) {
        return ret;
    }

    /* Wait for the response for serving system info, application specific logic goes here */
    /* This wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(30));

    std::cout << "\nServing system app exiting" << std::endl;
    return 0;
}
