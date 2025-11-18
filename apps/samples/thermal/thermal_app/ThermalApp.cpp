/*
 *  Copyright (c) 2019 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021,2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * This application demonstrates how to get thermal zones, a thermal zone by ID,
 * cooling devices and register for trip level and colling device level updates.
 * The steps are as follows:
 *
 * 1. Get a ThermalFactory instance.
 * 2. Get a IThermalManager instance from the ThermalFactory.
 * 3. Wait for the thermal service to become available.
 * 4. Register a listener that will receive thermal event updates.
 * 5. Get information about all thermal zones.
 * 6. Get information about a thermal zone identified by thermal zone ID.
 * 7. Get information about all cooling devices.
 * 8. Finally, deregister the listener.
 *
 * Usage:
 * # ./thermal_app
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/therm/ThermalFactory.hpp>
#include <telux/therm/ThermalManager.hpp>
#include <telux/therm/ThermalListener.hpp>

class ThermalUtils {
 public:
    void printThermalZonesInfo(
        std::vector<std::shared_ptr<telux::therm::IThermalZone>> zonesInfo) {
        printThermalZoneHeader();
        for (size_t index = 0; index < zonesInfo.size(); index++) {
            printZoneInfo(zonesInfo[index]);
        }
    }

    void printSpecificThermalZoneInfo(std::shared_ptr<telux::therm::IThermalZone> zoneInfo) {
        printThermalZoneHeader();
        printZoneInfo(zoneInfo);
        printBindingInfo(zoneInfo);
    }

    void printCoolingDevicesInfo(
        std::vector<std::shared_ptr<telux::therm::ICoolingDevice>> coolingDevices) {
        printCoolingDeviceHeader();
        for (size_t index = 0; index < coolingDevices.size(); index++) {
            printDeviceInfo(coolingDevices[index]);
        }
    }

    void printThermalZoneHeader() {
        std::cout << "\n*** Thermal zones ***" << std::endl;
        std::cout << std::setw(2)
                  << "+---------------------------------------------------------------------------"
                     "--------------------+"
                  << std::endl;
        std::cout << std::setw(3) << "| Tzone Id | " << std::setw(25) << " Type  " << std::setw(5)
                  << " | Current Temp  " << std::setw(5) << "|  Passive Temp  |" << std::setw(20)
                  << " Trip Points  " << std::endl;
        std::cout << std::setw(2)
                  << "+---------------------------------------------------------------------------"
                     "--------------------+"
                  << std::endl;
    }

    void printTripPointHeader() {
        std::cout << "\n*** Trip point ***" << std::endl;
        std::cout << std::setw(2)
                  << "+---------------------------------------------------------------------------"
                     "--------------------+"
                  << std::endl;
        std::cout << std::setw(3) << "| Tzone Id | " << std::setw(10) << "Trip Id | " << std::setw(15)
                  << "  Threshold Temp  |"
                  << " " << std::setw(8) << "  Hysteresis Temp  |"
                  << " " << std::setw(8) << "  Trip Event  |"
                  << " " << std::setw(10) << "  Trip Point  |" << std::endl;
        std::cout << std::setw(2)
                  << "+---------------------------------------------------------------------------"
                     "--------------------+"
                  << std::endl;
    }

    void printCoolingDeviceHeader() {
        std::cout << "\n*** Cooling Devices ***" << std::endl;
        std::cout << std::setw(2)
                  << "+--------------------------------------------------------------------------+"
                  << std::endl;
        std::cout << std::setw(3) << " | CDev Id " << std::setw(20) << " | CDev Type " << std::setw(5)
                  << " | Max Cooling State |" << std::setw(5) << " Current Cooling State |"
                  << std::endl;
        std::cout << std::setw(2)
                  << "+--------------------------------------------------------------------------+"
                  << std::endl;
    }

    void printZoneInfo(std::shared_ptr<telux::therm::IThermalZone> &tzInfo) {
        std::string tripPoints;
        std::vector<std::shared_ptr<telux::therm::ITripPoint>> tripInfo;

        if (!tzInfo) {
            std::cout << "Invalid thermal zone" << std::endl;
            return;
        }

        tripInfo = tzInfo->getTripPoints();
        if (tripInfo.size() > 0) {
            for (size_t i = 0; i < tripInfo.size(); ++i) {
                tripPoints = tripPointToString(tripInfo[i], tripPoints);
                if (!tripPoints.size()) { return; }
            }
        }

        std::cout << std::left << std::setw(4) << " " << std::setw(3) << tzInfo->getId()
                  << std::setw(10) << " " << std::setw(25) << tzInfo->getDescription() << std::setw(7)
                  << " " << std::setw(5) << tzInfo->getCurrentTemp() << std::setw(12) << " "
                  << std::setw(5) << tzInfo->getPassiveTemp() << std::setw(5) << " " << std::setw(30)
                  << tripPoints << std::setw(20);
        std::cout << std::endl;
    }

    void printTripPointInfo(std::shared_ptr<telux::therm::ITripPoint> &tripPointInfo,
            telux::therm::TripEvent event) {
        std::string tripPoints;
        std::string trip = convertTripTypeToStr(tripPointInfo->getType());
        tripPoints += tripPointToString(tripPointInfo, trip);
        std::cout
            << std::left << std::setw(3) << " " << std::setw(2) << tripPointInfo->getTZoneId()
            << std::setw(10) << " " << std::setw(2) << tripPointInfo->getTripId() << std::setw(10)
            << " " << std::setw(6) << tripPointInfo->getThresholdTemp() << std::setw(13) << " "
            << std::setw(10) << tripPointInfo->getHysteresis() << std::setw(9) << " " << std::setw(2)
            << ((event == telux::therm::TripEvent::CROSSED_UNDER) ?
            "CROSSED_UNDER" : "CROSSED_OVER ") << std::setw(5)
            << " " << std::setw(2) << tripPoints << std::endl;
    }

    void printBindingInfo(std::shared_ptr<telux::therm::IThermalZone> &tzInfo) {
        std::vector<telux::therm::BoundCoolingDevice> boundCoolingDeviceList
            = tzInfo->getBoundCoolingDevices();
        int boundCdevSize = boundCoolingDeviceList.size();
        if (boundCdevSize > 0) {
            std::cout << std::endl;
            std::cout << "Binding Info: " << std::endl;
            std::cout << std::setw(2) << "+--------------------------------------------------+"
                      << std::endl;
            std::cout << std::setw(5) << "|" << std::setw(10) << "Cooling Dev Id  " << std::setw(10)
                      << "|" << std::setw(20) << "Trip Points" << std::setw(10) << "|" << std::endl;
            std::cout << std::setw(2) << "+--------------------------------------------------+"
                      << std::endl;
            for (auto j = 0; j < boundCdevSize; j++) {
                std::string thresholdPoints;
                int noOfBoundTripPoints = boundCoolingDeviceList[j].bindingInfo.size();
                if (noOfBoundTripPoints > 0) {
                    for (auto k = 0; k < noOfBoundTripPoints; k++) {
                        thresholdPoints = tripPointToString(
                            boundCoolingDeviceList[j].bindingInfo[k], thresholdPoints);
                        if (!thresholdPoints.size()) { return; }
                    }
                    std::cout << std::left << std::setw(7) << " " << std::setw(3)
                              << boundCoolingDeviceList[j].coolingDeviceId << std::setw(15) << " "
                              << std::setw(30) << thresholdPoints << std::setw(20) << std::endl;
                } else {
                    std::cout << "No trip points bound!" << std::endl;
                }
            }
        } else {
            std::cout << "\nNo bound cooling devices found!" << std::endl;
        }
    }

    void printDeviceInfo(std::shared_ptr<telux::therm::ICoolingDevice> &cdevInfo) {
        if (!cdevInfo) {
            std::cout << "Invalid cooling device" << std::endl;
            return;
        }
        std::cout << std::left << std::setw(5) << " " << std::setw(3) << cdevInfo->getId()
                  << std::setw(7) << " " << std::setw(20) << cdevInfo->getDescription() << std::setw(7)
                  << " " << std::setw(5) << cdevInfo->getMaxCoolingLevel() << std::setw(15) << " "
                  << std::setw(5) << cdevInfo->getCurrentCoolingLevel() << std::endl;
    }

    std::string convertTripTypeToStr(telux::therm::TripType type) {
        std::string tripType;
        switch (type) {
            case telux::therm::TripType::CRITICAL:
                tripType = "CRITICAL";
                break;
            case telux::therm::TripType::HOT:
                tripType = "HOT";
                break;
            case telux::therm::TripType::PASSIVE:
                tripType = "PASSIVE";
                break;
            case telux::therm::TripType::ACTIVE:
                tripType = "ACTIVE";
                break;
            case telux::therm::TripType::CONFIGURABLE_HIGH:
                tripType = "CONFIGURABLE_HIGH";
                break;
            case telux::therm::TripType::CONFIGURABLE_LOW:
                tripType = "CONFIGURABLE_LOW";
                break;
            default:
                tripType = "UNKNOWN";
                break;
        }
        return tripType;
    }

    std::string tripPointToString(
        std::shared_ptr<telux::therm::ITripPoint> &tripInfo, std::string &tripTempPoints) {
        if (!tripInfo) {
            std::cout << "Invalid trip point" << std::endl;
            return std::string();
        }
        std::string trip = convertTripTypeToStr(tripInfo->getType());
        if (trip == "CRITICAL")
            tripTempPoints
                += "C" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
        else if (trip == "HOT")
            tripTempPoints
                += "H" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
        else if (trip == "ACTIVE")
            tripTempPoints
                += "A" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
        else if (trip == "PASSIVE")
            tripTempPoints
                += "P" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
        else if (trip == "CONFIGURABLE_HIGH")
            tripTempPoints
                += "CH" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
        else if (trip == "CONFIGURABLE_LOW")
            tripTempPoints
                += "CL" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
        else
            tripTempPoints
                += "U" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
        return tripTempPoints;
    }
};

class ThermalInfoListener : public telux::therm::IThermalListener {
 public:
    ThermalInfoListener(std::shared_ptr<ThermalUtils> thermalUtils) {
        thermalUtils_ = thermalUtils;
    }

    void onTripEvent(std::shared_ptr<telux::therm::ITripPoint> tripPoint,
            telux::therm::TripEvent tripEvent) override {
        if (tripPoint) {
            std::cout << "\nonTripEvent()" << std::endl;
            thermalUtils_->printTripPointHeader();
            thermalUtils_->printTripPointInfo(tripPoint, tripEvent);
            return;
        }
        std::cout << "Invalid trip point" << std::endl;
    }

    void onCoolingDeviceLevelChange(
            std::shared_ptr<telux::therm::ICoolingDevice> coolingDevice) override {
        if (coolingDevice) {
            std::cout << "\nonCoolingDeviceLevelChange()" << std::endl;
            thermalUtils_->printCoolingDeviceHeader();
            thermalUtils_->printDeviceInfo(coolingDevice);
            return;
        }
        std::cout << "Invalid cooling device" << std::endl;
    }

 private:
    std::shared_ptr<ThermalUtils> thermalUtils_;
};

class Application {
 public:
    int init() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &thermalFactory = telux::therm::ThermalFactory::getInstance();

        /* Step - 2 */
        thermalMgr_ = thermalFactory.getThermalManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!thermalMgr_) {
            std::cout << "Can't get IThermalManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Thermal service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        try {
            thermalUtils_ = std::make_shared<ThermalUtils>();
        } catch (const std::exception& e) {
            std::cout << "Can't allocate ThermalUtils" << std::endl;
            return -ENOMEM;
        }

        /* Step - 4 */
        try {
            thermalInfoListener_ = std::make_shared<ThermalInfoListener>(thermalUtils_);
        } catch (const std::exception& e) {
            std::cout << "Can't allocate ThermalInfoListener" << std::endl;
            return -ENOMEM;
        }

        status = thermalMgr_->registerListener(
            thermalInfoListener_, 1 << telux::therm::TNT_TRIP_UPDATE);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        /* Step - 8 */
        status = thermalMgr_->deregisterListener(
            thermalInfoListener_, 1 << telux::therm::TNT_TRIP_UPDATE);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int getAllThermalZones() {
        std::vector<std::shared_ptr<telux::therm::IThermalZone>> allZonesInfo;

        /* Step - 5 */
        allZonesInfo = thermalMgr_->getThermalZones();
        if (!allZonesInfo.size()) {
            std::cout << "No thermal zones found!" << std::endl;
        }

        thermalUtils_->printThermalZonesInfo(allZonesInfo);
        return 0;
    }

    int getSpecificThermalZone() {
        const int THERMAL_ZONE_ID = 1;
        std::shared_ptr<telux::therm::IThermalZone> zoneInfo;

        /* Step - 6 */
        zoneInfo = thermalMgr_->getThermalZone(THERMAL_ZONE_ID);
        if (!zoneInfo) {
            std::cout << "No thermal info!" << std::endl;
        }

        thermalUtils_->printSpecificThermalZoneInfo(zoneInfo);
        return 0;
    }

    int getCoolingDevices() {
        std::vector<std::shared_ptr<telux::therm::ICoolingDevice>> coolingDevices;

        /* Step - 7 */
        coolingDevices = thermalMgr_->getCoolingDevices();
        if (!coolingDevices.size()) {
            std::cout << "No cooling devices found!" << std::endl;
        }

        thermalUtils_->printCoolingDevicesInfo(coolingDevices);
        return 0;
    }

 private:
    std::shared_ptr<ThermalUtils> thermalUtils_;
    std::shared_ptr<ThermalInfoListener> thermalInfoListener_;
    std::shared_ptr<telux::therm::IThermalManager> thermalMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<Application> app;

    try {
        app = std::make_shared<Application>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate Application" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->getAllThermalZones();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->getSpecificThermalZone();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->getCoolingDevices();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    /* Wait for receiving all asynchronous responses.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(30));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nAppliction exiting" << std::endl;
    return 0;
}
