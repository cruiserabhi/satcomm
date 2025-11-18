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
 *  Copyright (c) 2021,2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <iomanip>
#include <iostream>
#include <vector>

#include "ThermalHelper.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

std::string ThermalHelper::convertTripTypeToStr(telux::therm::TripType type) {
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

std::string ThermalHelper::tripPointToString(
    std::shared_ptr<telux::therm::ITripPoint> &tripInfo, std::string &type) {
    std::string tripTempPoints;
    if (type == "CRITICAL")
        tripTempPoints
            += "C" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (type == "HOT")
        tripTempPoints
            += "H" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (type == "ACTIVE")
        tripTempPoints
            += "A" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (type == "PASSIVE")
        tripTempPoints
            += "P" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (type == "CONFIGURABLE_HIGH")
        tripTempPoints
            += "CH" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (type == "CONFIGURABLE_LOW")
        tripTempPoints
            += "CL" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else
        tripTempPoints
            += "U" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    return tripTempPoints;
}

void ThermalHelper::printBindingInfo(std::shared_ptr<telux::therm::IThermalZone> &tzInfo) {
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
                    std::string trip
                        = convertTripTypeToStr(boundCoolingDeviceList[j].bindingInfo[k]->getType());
                    thresholdPoints
                        += tripPointToString(boundCoolingDeviceList[j].bindingInfo[k], trip);
                }
                std::cout << std::left << std::setw(7) << " " << std::setw(3)
                          << boundCoolingDeviceList[j].coolingDeviceId << std::setw(15) << " "
                          << std::setw(30) << thresholdPoints << std::setw(20) << std::endl;
            } else {
                std::cout << "No trip points bound!" << std::endl;
            }
        }
    } else {
        std::cout << "No bound cooling devices found!" << std::endl;
    }
}

void ThermalHelper::printThermalZoneHeader() {
    std::cout << "*** Thermal zones ***" << std::endl;
    std::cout << std::setw(2)
              << "+---------------------------------------------------------------------------"
                 "--------------------+"
              << std::endl;
    std::cout << std::setw(3) << "| Tzone Id | " << std::setw(10) << "Type  " << std::setw(35)
              << " | Current Temp  " << std::setw(5) << "|  Passive Temp  |" << std::setw(20)
              << " Trip Points  " << std::endl;
    std::cout << std::setw(2)
              << "+---------------------------------------------------------------------------"
                 "--------------------+"
              << std::endl;
}

void ThermalHelper::printThermalZoneInfo(std::shared_ptr<telux::therm::IThermalZone> &tzInfo) {
    std::vector<std::shared_ptr<telux::therm::ITripPoint>> tripInfo;
    tripInfo = tzInfo->getTripPoints();
    std::string tripPoints;
    if (tripInfo.size() > 0) {
        for (size_t i = 0; i < tripInfo.size(); ++i) {
            std::string trip = convertTripTypeToStr(tripInfo[i]->getType());
            tripPoints += tripPointToString(tripInfo[i], trip);
        }
    }

    std::cout << std::left << std::setw(4) << " " << std::setw(3) << tzInfo->getId()
              << std::setw(10) << " " << std::setw(25) << tzInfo->getDescription() << std::setw(7)
              << " " << std::setw(5) << tzInfo->getCurrentTemp() << std::setw(12) << " "
              << std::setw(5) << tzInfo->getPassiveTemp() << std::setw(5) << " " << std::setw(30)
              << tripPoints;
    std::cout << std::endl;
}

void ThermalHelper::printCoolingDeviceHeader() {
    std::cout << "*** Cooling Devices ***" << std::endl;
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

void ThermalHelper::printCoolingDevInfo(std::shared_ptr<telux::therm::ICoolingDevice> &cdevInfo) {
    std::cout << std::left << std::setw(5) << " " << std::setw(3) << cdevInfo->getId()
              << std::setw(7) << " " << std::setw(20) << cdevInfo->getDescription() << std::setw(7)
              << " " << std::setw(5) << cdevInfo->getMaxCoolingLevel() << std::setw(15) << " "
              << std::setw(5) << cdevInfo->getCurrentCoolingLevel() << std::endl;
}

void ThermalHelper::printTripPointHeader() {
    std::cout << "*** Trip point ***" << std::endl;
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

void ThermalHelper::printTripPointInfo(
    std::shared_ptr<telux::therm::ITripPoint> &tripPointInfo, TripEvent event) {
    std::string tripPoints;
    std::string trip = convertTripTypeToStr(tripPointInfo->getType());
    tripPoints += tripPointToString(tripPointInfo, trip);
    std::cout
        << std::left << std::setw(3) << " " << std::setw(2) << tripPointInfo->getTZoneId()
        << std::setw(10) << " " << std::setw(2) << tripPointInfo->getTripId() << std::setw(10)
        << " " << std::setw(6) << tripPointInfo->getThresholdTemp() << std::setw(13) << " "
        << std::setw(10) << tripPointInfo->getHysteresis() << std::setw(9) << " " << std::setw(2)
        << ((event == TripEvent::CROSSED_UNDER) ? "CROSSED_UNDER" : "CROSSED_OVER ") << std::setw(5)
        << " " << std::setw(2) << tripPoints << std::endl;
}
