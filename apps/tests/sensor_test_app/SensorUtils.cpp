/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       SensorUtils.cpp
 *
 * @brief      Sensor Utility class
 */

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#include "SensorClient.hpp"
#include "SensorUtils.hpp"
#include "../../common/utils/Utils.hpp"
#include <telux/sensor/SensorDefines.hpp>

#define print_notification(tag) std::cout << "\033[1;35m" << tag << "\033[0m"

std::string SensorUtils::getSensorType(SensorType type) {
    switch (type) {
        case (SensorType::GYROSCOPE): {
            return "Gyroscope";
        }
        case (SensorType::ACCELEROMETER): {
            return "Accelerometer";
        }
        case (SensorType::GYROSCOPE_UNCALIBRATED): {
            return "Uncalibrated Gyroscope";
        }
        case (SensorType::ACCELEROMETER_UNCALIBRATED): {
            return "Uncalibrated Accelerometer";
        }
        default: {
            return "Unknown sensor type";
        }
    }
}

bool SensorUtils::isUncalibratedSensor(SensorType type) {
    return ((type == SensorType::GYROSCOPE_UNCALIBRATED)
            || (type == SensorType::ACCELEROMETER_UNCALIBRATED));
}

void SensorUtils::printSensorInfo(SensorInfo info, bool more, std::ostream &os) {
    os << "\tSensor ID: " << info.id << "\n\tSensor type: " << getSensorType(info.type)
       << "\n\tSensor name: " << info.name << "\n\tVendor: " << info.vendor
       << "\n\tSampling rates: [ ";
    for (auto rate : info.samplingRates) {
        os << std::fixed << std::setprecision(2) << rate << ", ";
    }
    os << "\b\b ]\n\tMax sampling rate: " << std::fixed << std::setprecision(2)
       << info.maxSamplingRate << "\n\tMax batch count: " << info.maxBatchCountSupported
       << "\n\tMin batch count: " << info.minBatchCountSupported << "\n\tRange: " << info.range
       << "\n\tVersion: " << info.version << std::setprecision(6)
       << "\n\tResolution: " << info.resolution << "\n\tMax range: " << info.maxRange;
    if (!more) {
        os << std::endl << std::endl;
    }
}

std::string SensorUtils::getSupportedRates(SensorInfo info) {
    std::string supportedRates = "[ ";
    for (float f : info.samplingRates) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << f;
        supportedRates.append(ss.str()).append(", ");
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << info.maxSamplingRate;
    supportedRates.append("\b\b ], <= ").append(ss.str());
    return supportedRates;
}

std::string SensorUtils::getBatchCountLimits(SensorInfo info) {
    std::string batchCountLimits = "[ ";
    batchCountLimits.append(std::to_string(info.minBatchCountSupported) + ", "
                            + std::to_string(info.maxBatchCountSupported) + " ]");
    return batchCountLimits;
}

SensorConfiguration SensorUtils::getSensorConfig(std::shared_ptr<SensorClient> s) {
    // If sensor type == GYRO | ACCELERO, get sampling rate and batch count
    std::shared_ptr<ISensorClient> sensor = s->getSensorClient();
    SensorType type = sensor->getSensorInfo().type;
    if ((type == SensorType::GYROSCOPE) || (type == SensorType::ACCELEROMETER)
        || ((type == SensorType::GYROSCOPE_UNCALIBRATED)
            || (type == SensorType::ACCELEROMETER_UNCALIBRATED))) {
        float samplingRate = 0;
        uint32_t batchCount = 0;
        bool isRotated = true;
        std::string supportedRates = getSupportedRates(sensor->getSensorInfo());
        std::string batchCountLimits = getBatchCountLimits(sensor->getSensorInfo());
        SensorUtils::getInput("Enter sampling rate " + supportedRates + ": ", samplingRate);
        SensorUtils::getInput("Enter batch count " + batchCountLimits + ": ", batchCount);
        SensorUtils::getInput("Enter isRotated: ", isRotated);

        // Set the sensor configuration
        SensorConfiguration s;
        s.samplingRate = samplingRate;
        s.batchCount = batchCount;
        s.isRotated = isRotated;
        s.validityMask.set(SensorConfigParams::SAMPLING_RATE);
        s.validityMask.set(SensorConfigParams::BATCH_COUNT);
        s.validityMask.set(SensorConfigParams::ROTATE);
        return s;
    }
    return SensorConfiguration();
}

telux::sensor::EulerAngleConfig SensorUtils::getEulerAngleConfig() {
        telux::sensor::EulerAngleConfig EulerAngleConfig = {0, 0, 0};
        SensorUtils::getInput("Enter roll angle: ", EulerAngleConfig.roll);
        SensorUtils::getInput("Enter pitch angle: ", EulerAngleConfig.pitch);
        SensorUtils::getInput("Enter yaw angle: ", EulerAngleConfig.yaw);

        // Set the sensor Euler angle configuration
        telux::sensor::EulerAngleConfig e;
        e.roll = EulerAngleConfig.roll;
        e.pitch = EulerAngleConfig.pitch;
        e.yaw = EulerAngleConfig.yaw;
        return e;
}

std::shared_ptr<SensorClient> SensorUtils::getSensorClient(
    int cid, std::vector<std::shared_ptr<SensorClient>> &sensors) {

    std::shared_ptr<SensorClient> sensor = nullptr;
    for (auto s : sensors) {
        if (s->id_ == cid) {
            sensor = s;
            break;
        }
    }
    if (sensor == nullptr) {
        std::cout << "Sensor with client ID " << cid << " not available" << std::endl;
    }
    return sensor;
}

void SensorUtils::printSensorEvent(
    SensorType type, SensorEvent &s, float samplingRate, std::string &tag) {
    if (isUncalibratedSensor(type)) {
        print_notification("Events")
            << tag << samplingRate << "Hz, " << s.timestamp << "ns, " << s.uncalibrated.data.x
            << ", " << s.uncalibrated.data.y << ", " << s.uncalibrated.data.z << ", "
            << s.uncalibrated.bias.x << ", " << s.uncalibrated.bias.y << ", "
            << s.uncalibrated.bias.z << std::endl;
    } else {
        print_notification("Events")
            << tag << samplingRate << " Hz, " << s.timestamp << ", " << s.calibrated.x << ", "
            << s.calibrated.y << ", " << s.calibrated.z << std::endl;
    }
}

void SensorUtils::printSensorFeatureBufferedEvent(SensorEvent &s) {
    print_notification("Buffered Events: ")
        << s.timestamp << "ns, " << s.uncalibrated.data.x << ", " << s.uncalibrated.data.y << ", "
        << s.uncalibrated.data.z << ", " << s.uncalibrated.bias.x << ", " << s.uncalibrated.bias.y
        << ", " << s.uncalibrated.bias.z << std::endl;
}

void SensorUtils::printSensorFeatureInfo(SensorFeature feature) {
    std::cout << "\t" << feature.name << std::endl;
}

void SensorUtils::printSensorFeatureEvent(SensorFeatureEvent event) {
    print_notification("SensorFeatureEvent: ")
        << event.id << " from feature " << event.name << " @ " << event.timestamp << std::endl;
}

void SensorUtils::printTcuActivityState(telux::power::TcuActivityState state) {

    if (state == telux::power::TcuActivityState::SUSPEND) {
        print_notification("TCU-activity State : SUSPEND") << std::endl;
    } else if (state == telux::power::TcuActivityState::RESUME) {
        print_notification("TCU-activity State : RESUME") << std::endl;
    } else if (state == telux::power::TcuActivityState::SHUTDOWN) {
        print_notification(" TCU-activity State : SHUTDOWN") << std::endl;
    } else if (state == telux::power::TcuActivityState::UNKNOWN) {
        print_notification(" TCU-activity State : UNKNOWN") << std::endl;
    } else {
        std::cout << " ERROR: Invalid TCU-activity state notified" << std::endl;
    }
}

std::string SensorUtils::sensorResultTypeToString(SensorResultType sensorResultType) {
    switch(sensorResultType) {
        case SensorResultType::HISTORICAL : return "HISTORICAL";
        case SensorResultType::CURRENT    : return "CURRENT";
        default : return "UNKNOWN";
    }
}