/*
 *  Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * This sample application demonstrates how to initiate a self on a given sensor and acquire the
 * self test result.
 */

#include <future>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <vector>

#include <telux/sensor/SensorFactory.hpp>

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

std::string getSensorType(telux::sensor::SensorType type) {
    switch (type) {
        case (telux::sensor::SensorType::GYROSCOPE): {
            return "Gyroscope";
        }
        case (telux::sensor::SensorType::ACCELEROMETER): {
            return "Accelerometer";
        }
        case (telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED): {
            return "Uncalibrated Gyroscope";
        }
        case (telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED): {
            return "Uncalibrated Accelerometer";
        }
        default: {
            return "Unknown sensor type";
        }
    }
}

void printSensorInfo(telux::sensor::SensorInfo info) {
    std::cout << "\tSensor ID: " << info.id << "\n\tSensor type: " << getSensorType(info.type)
              << "\n\tSensor name: " << info.name << "\n\tVendor: " << info.vendor
              << "\n\tSampling rates: [ ";
    for (auto rate : info.samplingRates) {
        std::cout << std::fixed << std::setprecision(2) << rate << ", ";
    }
    std::cout << "\b\b ]\n\tMax sampling rate: " << std::fixed << std::setprecision(2)
              << info.maxSamplingRate << "\n\tMax batch count: " << info.maxBatchCountSupported
              << "\n\tMin batch count: " << info.minBatchCountSupported
              << "\n\tRange: " << info.range << "\n\tVersion: " << info.version
              << std::setprecision(6) << "\n\tResolution: " << info.resolution
              << "\n\tMax range: " << info.maxRange << std::endl;
}

void printHelp(std::string programName, std::vector<telux::sensor::SensorInfo> &sensorInfo) {
    std::cout << "Usage: " << programName << " [-sh]" << std::endl
              << std::endl
              << "-s <name>         Create sensor with provided name for self test" << std::endl
              << "-t <test type>    Self test type to be initiated, 0 - Positive, 1 - Negative"
              << std::endl
              << "-h                This help" << std::endl;

    std::cout << "Available sensors: ";
    for (telux::sensor::SensorInfo info : sensorInfo) {
        std::cout << info.name << ", ";
    }
    std::cout << "\b\b  " << std::endl;
}

void parseArgs(int argc, char **argv, std::string &name, telux::sensor::SelfTestType &selfTestType,
    std::vector<telux::sensor::SensorInfo> &sensorInfo) {
    int c = -1;
    static const struct option long_options[] = {{"sensor name", required_argument, 0, 's'},
        {"self test type", required_argument, 0, 't'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};
    int option_index = 0;
    c = getopt_long(argc, argv, "s:t:h", long_options, &option_index);
    if (c == -1) {
        if (sensorInfo.size() > 0) {
            name = sensorInfo[0].name;
            std::cout << "Creating sensor: " << name << std::endl;
        } else {
            std::cout << "No sensors found for self test" << std::endl;
            name = "";
        }
        return;
    }
    do {
        switch (c) {
            case 's': {
                name = optarg;
                break;
            }
            case 't': {
                int testType = std::stoi(optarg);
                if ((testType == 0) || (testType == 1)) {
                    selfTestType = static_cast<telux::sensor::SelfTestType>(testType);
                }
                break;
            }
            case 'h': {
                printHelp(argv[0], sensorInfo);
                exit(0);
            }
        }
        c = getopt_long(argc, argv, "s:t:h", long_options, &option_index);
    } while (c != -1);
}

int main(int argc, char **argv) {
    std::cout << "********* sensor self test sample app *********" << std::endl;

    std::string name;
    telux::sensor::SelfTestType selfTestType = telux::sensor::SelfTestType::POSITIVE;

    // [1] Get sensor factory instance
    auto &sensorFactory = telux::sensor::SensorFactory::getInstance();

    // [2] Prepare a callback to sensor factory which is called when the initialization of the
    // sensor sub-system is completed
    std::promise<telux::common::ServiceStatus> p;
    auto initCb = [&p](telux::common::ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
    };

    // [3] Get the sensor manager
    std::shared_ptr<telux::sensor::ISensorManager> sensorManager
        = sensorFactory.getSensorManager(initCb);
    if (sensorManager == nullptr) {
        std::cout << "sensor manager is nullptr" << std::endl;
        exit(1);
    }
    std::cout << "obtained sensor manager" << std::endl;

    // [4] Wait until initialization is complete
    p.get_future().get();
    if (sensorManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Sensor service not available" << std::endl;
        exit(1);
    }

    // [5] Get information on available sensors and their characteristics like name, supported
    // sampling rates among other information
    std::cout << "Sensor service is now available" << std::endl;
    std::vector<telux::sensor::SensorInfo> sensorInfo;
    telux::common::Status status = sensorManager->getAvailableSensorInfo(sensorInfo);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get information on available sensors" << static_cast<int>(status)
                  << std::endl;
        exit(1);
    }
    parseArgs(argc, argv, name, selfTestType, sensorInfo);
    if (name == "") {
        exit(0);
    }
    std::cout << "Received sensor information" << std::endl;
    for (auto info : sensorInfo) {
        printSensorInfo(info);
    }

    // [6] Get the desired sensor
    std::shared_ptr<telux::sensor::ISensorClient> sensorClient;
    std::cout << "Getting sensor: " << name << std::endl;
    status = sensorManager->getSensorClient(sensorClient, name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get sensor: " << name << std::endl;
        exit(1);
    }

    // [7] Invoke the self test with the required self test type and provide the callback
    status = sensorClient->selfTest(selfTestType, [](telux::common::ErrorCode result) {
        PRINT_CB << "Received self test response: " << static_cast<int>(result) << std::endl;
    });
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Self test request failed";
    } else {
        std::cout << "Self test request successful, waiting for callback" << std::endl;
    }

    std::cout << "\n\nPress ENTER to exit \n\n";
    std::cin.ignore();

    // [8] Delete the sensor client
    sensorClient = nullptr;

    // [9] When sensor manager is no longer required, delete the sensor manager object
    sensorManager = nullptr;

    return 0;
}
