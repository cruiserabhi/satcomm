/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * @file       LocationClient.cpp
 *
 * @brief      LocationClient class provides methods to start and stop receiving location-fix
 *             updates. It accesses the location subsystem using Telematics-SDK APIs.
 */

#include <iostream>
#include <cstring>

#include "LocationClient.hpp"

#define CLIENT_NAME "ECall-Location-Client: "
#define MILLISECONDS_IN_A_SECOND 1000
#define MILLIARCSECONDS_IN_A_DEGREE 3600000

LocationClient::LocationClient()
    : locMgr_(nullptr)
    , locListener_(nullptr) {
}

LocationClient::~LocationClient() {
}

// Callback on location-fix update from Telematics-SDK
void LocationClient::onBasicLocationUpdate(
                const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) {
    if(!locationInfo) {
        std::cout << CLIENT_NAME << "Invalid location info. received!" << std::endl;
        return;
    }
    ECallLocationInfo locInfo{};
    locInfo.latitude =
                static_cast<int32_t>(locationInfo->getLatitude()*MILLIARCSECONDS_IN_A_DEGREE);
    locInfo.longitude =
                static_cast<int32_t>(locationInfo->getLongitude()*MILLIARCSECONDS_IN_A_DEGREE);
    locInfo.direction = static_cast<uint8_t>(locationInfo->getHeading());
    locInfo.timestamp =
                static_cast<uint32_t>(locationInfo->getTimeStamp()/MILLISECONDS_IN_A_SECOND);
    if(locListener_) {
        locListener_->onLocationUpdate(locInfo);
    }
}

// Callback which provides response to startBasicReports/stopReports commands
void LocationClient::commandCallback(ErrorCode errorCode) {
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Command initiated successfully " << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Command failed!" << std::endl;
    }
}

// Callback which is invoked when LocationManager initialization is processed(success or failure)
static void initCb(telux::common::ServiceStatus status) {
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << CLIENT_NAME << " Location Manager is initialized successfully " << std::endl;
    } else if(status == telux::common::ServiceStatus::SERVICE_FAILED) {
        std::cout << CLIENT_NAME << " Location Manager initialization failed" << std::endl;
    }
}

// Initialize the location subsystem
telux::common::Status LocationClient::init() {
    // Get location manager object
    auto &locationFactory = LocationFactory::getInstance();
    locMgr_ = locationFactory.getLocationManager(&initCb);
    if(locMgr_ == nullptr) {
        std::cout << CLIENT_NAME << "*** ERROR - Failed to get Location Manager instance"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Start receiving location updates
telux::common::Status LocationClient::startLocUpdates(uint32_t interval,
                            std::shared_ptr<LocationListener> locListener) {
    if(!locMgr_) {
        std::cout << CLIENT_NAME << "Invalid Location Manager" << std::endl;
        return telux::common::Status::FAILED;
    } else if(ServiceStatus::SERVICE_AVAILABLE != locMgr_->getServiceStatus()) {
        std::cout << CLIENT_NAME << " Location Subsystem is not yet ready" << std::endl;
        return telux::common::Status::NOTREADY;
    }
    // Registering a listener for location fix updates
    telux::common::Status status = locMgr_->registerListenerEx(shared_from_this());
    if(status == telux::common::Status::ALREADY) {
        std::cout << CLIENT_NAME << "Already registered for location updates" << std::endl;
        return telux::common::Status::SUCCESS;
    } else if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to register listener for location updates" << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Registered listener for location updates" << std::endl;
    }
    status = locMgr_->startBasicReports(0, interval, std::bind(&LocationClient::commandCallback,
                                        this, std::placeholders::_1));
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "*** ERROR - Failed to start location reports" << std::endl;
        status = locMgr_->deRegisterListenerEx(shared_from_this());
        if(status != telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << " *** ERROR - Failed to de-register for location updates"
                        << std::endl;
        }
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Started location reports" << std::endl;
    }
    locListener_ = locListener;
    return telux::common::Status::SUCCESS;
}

// Stop receiving location updates
telux::common::Status LocationClient::stopLocUpdates() {
    if(!locMgr_) {
        std::cout << CLIENT_NAME << "Invalid Location Manager" << std::endl;
        return telux::common::Status::FAILED;
    } else if(ServiceStatus::SERVICE_AVAILABLE != locMgr_->getServiceStatus()) {
        std::cout << CLIENT_NAME << " Location Subsystem is not yet ready" << std::endl;
        return telux::common::Status::NOTREADY;
    }
    // De-registering a listener for location fix updates
    telux::common::Status status = locMgr_->deRegisterListenerEx(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << " *** ERROR - Failed to de-register for location updates"
                    << std::endl;
    } else {
        std::cout << CLIENT_NAME << " De-registered listener for location updates" << std::endl;
    }
    status = locMgr_->stopReports(std::bind(&LocationClient::commandCallback, this,
                                std::placeholders::_1));
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << " *** ERROR - Failed to stop location reports" << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << " Stopped location reports" << std::endl;
    }
    locListener_ = nullptr;
    return telux::common::Status::SUCCESS;
}
