/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
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

#include <future>
#include <getopt.h>
#include <iostream>
#include <iomanip>
#include <limits>
#include <vector>
#include <condition_variable>
#include <thread>

#include <telux/sensor/SensorFactory.hpp>

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m"

#define TOTAL_BATCHES_REQUIRED 10

class SensorEventListener : public telux::sensor::ISensorEventListener {
 public:
    SensorEventListener(std::string name, std::shared_ptr<telux::sensor::ISensorClient> sensor)
       : name_(name)
       , sensorClient_(sensor)
       , totalBatches_(0) {
    }

    // [11] Receive sensor events. This notification is received every time the configured batch
    // count is available with the sensor framework
    virtual void onEvent(std::shared_ptr<std::vector<telux::sensor::SensorEvent>> events) override {

        PRINT_NOTIFICATION << "(" << name_ << "): Received " << events->size()
                           << " events from sensor: " << sensorClient_->getSensorInfo().name
                           << std::endl;

        // I/O intense operations such as below should be avoided since this thread should avoid
        // any time consuming operations
        for (telux::sensor::SensorEvent s : *(events.get())) {
            printSensorEvent(s);
        }
        ++totalBatches_;
        // [11.1] If we have received expected number of batches and want to reconfigure the sensor
        // we will spawn the request to deactivate, configure and activate on a different thread
        // since we are not allowed to invoke the sensor APIs from this thread context
        if (totalBatches_ > TOTAL_BATCHES_REQUIRED) {
            totalBatches_ = 0;
            std::thread t([&] {
                sensorClient_->deactivate();
                sensorClient_->configure(sensorClient_->getConfiguration());
                sensorClient_->activate();
            });
            // Be sure to detach the thread
            t.detach();
        }
    }

    // [9] Receive configuration updates
    virtual void onConfigurationUpdate(telux::sensor::SensorConfiguration configuration) override {
        PRINT_NOTIFICATION << "(" << name_ << "): Received configuration update from sensor: "
                           << sensorClient_->getSensorInfo().name << ": ["
                           << configuration.samplingRate << ", " << configuration.batchCount <<
                           "," << configuration.isRotated << " ]"
                           << std::endl;
    }

 private:
    bool isUncalibratedSensor(telux::sensor::SensorType type) {
        return ((type == telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED)
                || (type == telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED));
    }

    void printSensorEvent(telux::sensor::SensorEvent &s) {
        telux::sensor::SensorInfo info = sensorClient_->getSensorInfo();
        if (isUncalibratedSensor(sensorClient_->getSensorInfo().type)) {
            PRINT_NOTIFICATION << ": " << sensorClient_->getSensorInfo().name << ": " << s.timestamp
                               << ", " << s.uncalibrated.data.x << ", " << s.uncalibrated.data.y
                               << ", " << s.uncalibrated.data.z << ", " << s.uncalibrated.bias.x
                               << ", " << s.uncalibrated.bias.y << ", " << s.uncalibrated.bias.z
                               << std::endl;
        } else {
            PRINT_NOTIFICATION << ": " << sensorClient_->getSensorInfo().name << ": " << s.timestamp
                               << ", " << s.calibrated.x << ", " << s.calibrated.y << ", "
                               << s.calibrated.z << std::endl;
        }
    }

    std::string name_;
    std::shared_ptr<telux::sensor::ISensorClient> sensorClient_;
    uint32_t totalBatches_;
};

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

float getMinimumSamplingRate(telux::sensor::SensorInfo info) {
    float min = std::numeric_limits<float>::infinity();
    for (float r : info.samplingRates) {
        if (r < min) {
            min = r;
        }
    }
    return min;
}

float getMaximumSamplingRate(telux::sensor::SensorInfo info) {
    float max = 0;
    for (float r : info.samplingRates) {
        if (r > max) {
            if (r <= info.maxSamplingRate) {
                max = r;
            } else {
                break;
            }
        }
    }
    return max;
}

void printHelp(std::string programName, std::vector<telux::sensor::SensorInfo> &sensorInfo) {
    std::cout << "Usage: " << programName << " [-sh]" << std::endl
              << std::endl
              << "-s <name>    Create sensor with provided name for data acquisition" << std::endl
              << "-h           This help" << std::endl;

    std::cout << "Available sensors: ";
    for (telux::sensor::SensorInfo info : sensorInfo) {
        std::cout << info.name << ", ";
    }
    std::cout << "\b\b  " << std::endl;
}

void parseArgs(
    int argc, char **argv, std::string &name, std::vector<telux::sensor::SensorInfo> &sensorInfo) {
    int c = -1;
    static const struct option long_options[]
        = {{"sensor name", required_argument, 0, 's'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};
    int option_index = 0;
    c = getopt_long(argc, argv, "s:h", long_options, &option_index);
    if (c == -1) {
        if (sensorInfo.size() > 0) {
            name = sensorInfo[0].name;
            std::cout << "Creating sensor: " << name << std::endl;
        } else {
            std::cout << "No sensors found for data acquisition" << std::endl;
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
            case 'h': {
                printHelp(argv[0], sensorInfo);
                exit(0);
            }
        }
        c = getopt_long(argc, argv, "s:h", long_options, &option_index);
    } while (c != -1);
}

int main(int argc, char **argv) {
    std::cout << "********* sensor sample app *********" << std::endl;

    std::string name;

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
    parseArgs(argc, argv, name, sensorInfo);
    if (name == "") {
        exit(0);
    }
    std::cout << "Received sensor information" << std::endl;
    for (auto info : sensorInfo) {
        printSensorInfo(info);
    }

    // [6] Get the desired sensor
    std::shared_ptr<telux::sensor::ISensorClient> lowRateSensorClient;
    std::cout << "Getting sensor: " << name << std::endl;
    status = sensorManager->getSensorClient(lowRateSensorClient, name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get sensor: " << name << std::endl;
        exit(1);
    }

    // [7] Create a dedicated listener per sensor and register the listener to get notifications
    // about sensor configuration updates, sensor events
    std::shared_ptr<SensorEventListener> lowRateSensorClientEventListener
        = std::make_shared<SensorEventListener>("Low-rate", lowRateSensorClient);
    lowRateSensorClient->registerListener(lowRateSensorClientEventListener);

    // [8] Configure the sensor with the desired configuration, with the required validityMask set
    telux::sensor::SensorConfiguration lowRateConfig;
    lowRateConfig.samplingRate = getMinimumSamplingRate(lowRateSensorClient->getSensorInfo());
    lowRateConfig.batchCount = lowRateSensorClient->getSensorInfo().maxBatchCountSupported;
    lowRateConfig.isRotated = false;
    std::cout << "Configuring sensor with samplingRate, batchCount [" << lowRateConfig.samplingRate
              << ", " << lowRateConfig.batchCount << ", " << lowRateConfig.isRotated << "]" <<
    std::endl;
    lowRateConfig.validityMask.set(telux::sensor::SensorConfigParams::SAMPLING_RATE);
    lowRateConfig.validityMask.set(telux::sensor::SensorConfigParams::BATCH_COUNT);
    lowRateConfig.validityMask.set(telux::sensor::SensorConfigParams::ROTATE);
    status = lowRateSensorClient->configure(lowRateConfig);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to configure sensor: " << name << std::endl;
        exit(1);
    }

    // [10] Activate the sensor
    status = lowRateSensorClient->activate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to activate sensor: " << name << std::endl;
        exit(1);
    }

    // [12] Create another sensor client for the same sensor and it's corresponding listener
    std::shared_ptr<telux::sensor::ISensorClient> highRateSensorClient;
    std::cout << "Getting sensor: " << name << std::endl;
    status = sensorManager->getSensorClient(highRateSensorClient, name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get sensor: " << name << std::endl;
        exit(1);
    }
    std::shared_ptr<SensorEventListener> highRateSensorEventListener
        = std::make_shared<SensorEventListener>("High-rate", highRateSensorClient);
    highRateSensorClient->registerListener(highRateSensorEventListener);

    // [13] Configure this sensor client with a different configuration, as necessary
    telux::sensor::SensorConfiguration highRateConfig;
    highRateConfig.samplingRate = getMaximumSamplingRate(highRateSensorClient->getSensorInfo());
    highRateConfig.batchCount = highRateSensorClient->getSensorInfo().maxBatchCountSupported;
    highRateConfig.batchCount = true;
    std::cout << "Configuring sensor with samplingRate, batchCount [" << highRateConfig.samplingRate
              << ", " << highRateConfig.batchCount << ", " << lowRateConfig.isRotated << "]" <<
              std::endl;
    highRateConfig.validityMask.set(telux::sensor::SensorConfigParams::SAMPLING_RATE);
    highRateConfig.validityMask.set(telux::sensor::SensorConfigParams::BATCH_COUNT);
    highRateConfig.validityMask.set(telux::sensor::SensorConfigParams::ROTATE);
    status = highRateSensorClient->configure(highRateConfig);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to configure sensor: " << name << std::endl;
        exit(1);
    }

    // [14] Activate this sensor as well
    status = highRateSensorClient->activate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to activate sensor: " << name << std::endl;
        exit(1);
    }

    std::cout << "\n\nWait to receive further notifications OR press ENTER to exit \n\n";
    std::cin.ignore();

    // [15] Deactivate the sensors
    status = lowRateSensorClient->deactivate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to deactivate sensor: " << name << std::endl;
        exit(1);
    }
    status = highRateSensorClient->deactivate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to deactivate sensor: " << name << std::endl;
        exit(1);
    }

    // [16] Delete the sensor objects
    lowRateSensorClient = nullptr;
    highRateSensorClient = nullptr;

    // [17] When sensor manager is no longer required, delete the sensor manager object
    sensorManager = nullptr;

    return 0;
}
