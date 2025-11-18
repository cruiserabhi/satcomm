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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021,2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This application demonstrates how to get location report. The steps are as follows:
 *
 * 1. Get a LocationFactory instance.
 * 2. Get a ILocationManager instance from LocationFactory.
 * 3. Wait for the location service to become available.
 * 4. Register the listener which will receive location report.
 * 5. Start collecting location details.
 * 6. Execute application specific business logic.
 * 7. Finally, stop the reports and deregister the listener.
 *
 * Usage:
 * # ./loc_app
 */

#include <errno.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include "telux/loc/LocationFactory.hpp"
#include "telux/loc/LocationManager.hpp"
#include "telux/loc/LocationListener.hpp"

class LocationListener : public telux::loc::ILocationListener,
                         public std::enable_shared_from_this<LocationListener> {
 public:
    int init();
    int getBasicReports();
    int deinit();
    void onBasicLocationUpdate(
        const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) override;

 private:
    std::shared_ptr<telux::loc::ILocationManager> locMgr_;
};

int LocationListener::init() {
    telux::common::Status status;
    telux::common::ServiceStatus serviceStatus;
    std::promise<telux::common::ServiceStatus> p{};

    /* Step - 1 */
    auto &locationFactory = telux::loc::LocationFactory::getInstance();

    /* Step - 2 */
    locMgr_ = locationFactory.getLocationManager(
            [&p](telux::common::ServiceStatus status) {
        p.set_value(status);
    });

    if (!locMgr_) {
        std::cout << "Can't get ILocationManager" << std::endl;
        return -ENOMEM;
    }

    /* Step - 3 */
    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Location service unavailable, status " <<
            static_cast<int>(serviceStatus) << std::endl;
        return -EIO;
    }

    /* Step - 4 */
    status = locMgr_->registerListenerEx(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Can't register listener, err " <<
            static_cast<int>(status) << std::endl;
        return -EIO;
    }

    std::cout << "Initialization complete" << std::endl;
    return 0;
}

int LocationListener::getBasicReports() {
    telux::common::Status status;

    /* Step - 5 */
    status = locMgr_->startBasicReports(1000, nullptr);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Can't start location gathering, err " <<
            static_cast<int>(status) << std::endl;
        return -EIO;
    }

    std::cout << "Request for basic reports placed" << std::endl;
    return 0;
}

int LocationListener::deinit() {
    /* Step - 7 */
    locMgr_->stopReports(nullptr);
    telux::common::Status status = locMgr_->deRegisterListenerEx(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Can't deregister listener, err " <<
            static_cast<int>(status) << std::endl;
        return -EIO;
    }
    return 0;
}

void LocationListener::onBasicLocationUpdate(
    const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) {
    std::cout << "***************** Basic Location Report ***************" << std::endl;
    if(locationInfo->getTimeStamp() != telux::loc::UNKNOWN_TIMESTAMP) {
        time_t realtime;
        realtime = (time_t)((locationInfo->getTimeStamp() / 1000));
        std::cout << "Time stamp: " << locationInfo->getTimeStamp() << " mSec" << std::endl;
        std::cout << "GMT Time stamp: " << ctime(&realtime);
    }
    std::cout << "Latitude: " << std::setprecision(15) << locationInfo->getLatitude() << std::endl
        << "Longitude: " << std::setprecision(15) << locationInfo->getLongitude() << std::endl
        << "Altitude: " << std::setprecision(15) << locationInfo->getAltitude() << std::endl
        << "Speed: " << locationInfo->getSpeed() << std::endl
        << "Heading: " << locationInfo->getHeading() << std::endl
        << "Horizontal uncertainty: " << locationInfo->getHorizontalUncertainty() << std::endl
        << "Vertical uncertainty: " << locationInfo->getVerticalUncertainty() << std::endl
        << "Speed uncertainty: " << locationInfo->getSpeedUncertainty() << std::endl
        << "Heading uncertainty: " << locationInfo->getHeadingUncertainty() << std::endl
        << "Elapsed real time: " << locationInfo->getElapsedRealTime() << std::endl
        << "Elapsed real time uncertainty: " << locationInfo->getElapsedRealTimeUncertainty()
        << std::endl
        << "Time uncertainty: " << locationInfo->getTimeUncMs() << std::endl
        << "gPTP time: " << locationInfo->getElapsedGptpTime() << std::endl
        << "gPTP time uncertainty: " << locationInfo->getElapsedGptpTimeUnc() << std::endl;
    std::cout << "*************************************************************" << std::endl;
}

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<LocationListener> app;

    try {
        app = std::make_shared<LocationListener>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate LocationListener" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->getBasicReports();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    /* Step - 6 */
    /* Wait for receiving all asynchronous responses.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::minutes(2));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Location app exiting" << std::endl;
    return 0;
}
