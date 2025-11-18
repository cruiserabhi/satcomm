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
 *  Copyright (c) 2021-2022, 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file        SensorClient.cpp
 *
 * @brief       This file hosts the implementation for the sensor client to configure and acquire
 *              data from the sensor framework
 */

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <future>
#include <unistd.h>

#include "SensorTestApp.hpp"
#include "SensorClient.hpp"

#include "../../common/utils/Utils.hpp"
#include <telux/common/Version.hpp>

#define print_notification(tag) std::cout << "\033[1;35m" << tag << "\033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"
#define SENSOR_DATA_RECORDING std::cerr << "###"

SensorClient::SensorClient(
    int id, std::shared_ptr<ISensorClient> sensor, SensorTestAppArguments commandLineArgs)
   : id_(id)
   , sensor_(sensor)
   , lastBatchReceivedAt_(0)
   , totalEvents_(0)
   , stop_(false)
   , activated_(false)
   , commandLineArgs_(commandLineArgs) {
    tag_ = std::string("[")
               .append(SensorUtils::getSensorType(sensor_->getSensorInfo().type))
               .append(", Sensor ID: ")
               .append(std::to_string(sensor_->getSensorInfo().id))
               .append(", Client ID: ")
               .append(std::to_string(id_))
               .append("] ");
    if (commandLineArgs_.quiet) {
        workerThread_ = std::make_shared<std::thread>([&]() {
            while (!stop_) {
                {
                    std::unique_lock<std::mutex> lock(qMutex_);
                    cv_.wait(lock, [=]() { return stop_ || activated_; });
                }
                if (activated_) {
                    sleep(commandLineArgs_.printPeriod);
                    std::lock_guard<std::mutex> lock(mtx_);
                    print_notification("Summary")
                        << tag_ << "Events since " << commandLineArgs_.printPeriod
                        << "s: " << totalEvents_ << std::endl;
                    totalEvents_ = 0;
                }
            }
        });
    }
}

void SensorClient::init() {
    sensor_->registerListener(shared_from_this());
}

void SensorClient::cleanup() {
    sensor_->deregisterListener(shared_from_this());
}

SensorClient::~SensorClient() {
    {
        std::lock_guard<std::mutex> lck(qMutex_);
        stop_ = true;
        cv_.notify_one();
    }
    if (activated_) {
        deactivate();
    }
    sensor_ = nullptr;
    if (workerThread_) {
        workerThread_->join();
        workerThread_ = nullptr;
    }
}

void SensorClient::printInfo() {
    SensorConfiguration configuration = sensor_->getConfiguration();
    std::cout << "\tClient ID: " << id_ << ", Sensor name: " << sensor_->getSensorInfo().name
              << ", Configuration: [";
    if (configuration.validityMask.test(SensorConfigParams::SAMPLING_RATE)) {
        std::cout << std::fixed << std::setprecision(2) << configuration.samplingRate << "Hz";
    } else {
        std::cout << "NA";
    }
    std::cout << ", "
              << (configuration.validityMask.test(SensorConfigParams::BATCH_COUNT)
                         ? std::to_string(configuration.batchCount)
                         : "NA")
              << ", "
              << (configuration.validityMask.test(SensorConfigParams::ROTATE)
                         ? std::to_string(configuration.isRotated)
                         : "NA")
              << "]"
              << ", Activated: " << (activated_ ? "Yes" : "No") << std::endl;
}

void SensorClient::onEvent(std::shared_ptr<std::vector<SensorEvent>> events) {
    uint64_t receivedTimeStamp = Utils::getNanosecondsSinceBoot();
    if (!commandLineArgs_.quiet) {
        float timeSinceLastBatch = 0;

        // Calculate time difference between two batches in milliseconds
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (lastBatchReceivedAt_ > 0) {
                timeSinceLastBatch = 1.0 * (receivedTimeStamp - lastBatchReceivedAt_) / 1000000;
            }
            lastBatchReceivedAt_ = receivedTimeStamp;
        }

        uint64_t eventTimeStamp = 0;
        uint32_t count = 0;
        float samplingRateAggregate = 0.0;
        for (SensorEvent s : *(events.get())) {
            float samplingRate = 0.0;
            if (eventTimeStamp > 0) {
                ++count;
                // Instantaneous sampling rate, calculated between consecutive samples
                samplingRate = 1.0 / (s.timestamp - eventTimeStamp) * 1000000000;
            }
            if (commandLineArgs_.verboseNotification) {
                SensorUtils::printSensorEvent(sensor_->getSensorInfo().type, s, samplingRate, tag_);
            }
            samplingRateAggregate += samplingRate;
            eventTimeStamp = s.timestamp;
            if(isRecordingEnabled_) {
                //Recording Data.
                std::ostringstream recordStream;
                recordStream << static_cast<uint32_t>(sensor_->getSensorInfo().type) << ","
                << sensor_->getConfiguration().isRotated << "," << s.timestamp << ","
                << s.uncalibrated.data.x << "," << s.uncalibrated.data.y << ","
                << s.uncalibrated.data.z << "," << s.uncalibrated.bias.x << ","
                << s.uncalibrated.bias.y << "," << s.uncalibrated.bias.z << std::endl;
                {
                    std::lock_guard<std::mutex> lock(mtx_);
                    SENSOR_DATA_RECORDING << recordStream.str() << std::endl;
                }
            }
        }
        print_notification("Batch")
            << tag_ << samplingRateAggregate / count << "Hz, " << receivedTimeStamp << "ns, "
            << events->size() << ", " << std::fixed << timeSinceLastBatch << "ms" << std::endl;
    } else {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            totalEvents_ += events->size();
        }
    }
}

void SensorClient::onConfigurationUpdate(SensorConfiguration configuration) {
    print_notification("ConfigUpdate")
        << tag_ << "Received configuration update: [" << configuration.samplingRate << ", "
        << configuration.batchCount << "," << configuration.isRotated << "]" << std::endl;
}

telux::common::Status SensorClient::configure(SensorConfiguration config) {
    telux::common::Status status = sensor_->configure(config);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "sensor configuration failed: ";
        Utils::printStatus(status);
    } else {
        std::cout << tag_ << "Sensor configuration successful" << std::endl;
    }
    return status;
}

telux::common::Status SensorClient::activate() {
    telux::common::Status status = sensor_->activate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "sensor activation failed: ";
        Utils::printStatus(status);
    } else {
        {
            std::lock_guard<std::mutex> lck(qMutex_);
            activated_ = true;
            cv_.notify_one();
        }
        std::cout << tag_ << "Sensor activation successful" << std::endl;
    }
    return status;
}

telux::common::Status SensorClient::deactivate() {
    telux::common::Status status = sensor_->deactivate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "sensor deactivation failed: ";
        Utils::printStatus(status);
    } else {
        activated_ = false;
        std::cout << tag_ << "Sensor deactivation successful" << std::endl;
    }
    return status;
}

void SensorClient::enableLowPowerMode() {
    telux::common::Status status = sensor_->enableLowPowerMode();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "low power mode enable request failed: ";
        Utils::printStatus(status);
        return;
    }
    std::cout << tag_ << "Low power mode enable request successful" << std::endl;
}

void SensorClient::disableLowPowerMode() {
    telux::common::Status status = sensor_->disableLowPowerMode();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "low power mode disable request failed: ";
        Utils::printStatus(status);
        return;
    }
    std::cout << tag_ << "Low power mode disable request successful" << std::endl;
}

telux::common::Status SensorClient::selfTest(SelfTestType selfTestType) {
    static uint64_t requestID = 0;
    ++requestID;
    uint64_t thisRequestID = requestID;
    uint64_t requestTimeStamp = Utils::getNanosecondsSinceBoot();
    telux::common::Status status = sensor_->selfTest(selfTestType, [thisRequestID, requestTimeStamp,
                                                                       this](ErrorCode result) {
        if(result != telux::common::ErrorCode::INFO_UNAVAILABLE) {
            uint64_t responseTimeStamp = Utils::getNanosecondsSinceBoot();
            PRINT_CB << tag_ << "Received self test response: "
                    << Utils::getErrorCodeAsString(result)
                    << " for requestID = " << thisRequestID << " after "
                    << (responseTimeStamp - requestTimeStamp) * 1.0 / 1000000 << "ms" << std::endl;
        } else {
            PRINT_CB << tag_ << " Received self test response: " <<
                Utils::getErrorCodeAsString(result);
        }
    });
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "self test request with ID " << requestID << " failed: ";
        Utils::printStatus(status);
    } else {
        std::cout << tag_ << "Self test request with requestID " << requestID
            << " successful, waiting for callback" << std::endl;
    }
    return status;
}

telux::common::Status SensorClient::selfTestEx(SelfTestType selfTestType) {
    telux::common::Status status = sensor_->selfTest(selfTestType,
    [this](ErrorCode result, SelfTestResultParams selfTestResultParams) {
        if(result != telux::common::ErrorCode::INFO_UNAVAILABLE) {
            PRINT_CB << tag_ << " Received self test response: " <<
                Utils::getErrorCodeAsString(result)
                << " for Sensor result type: "
                << SensorUtils::sensorResultTypeToString(selfTestResultParams.sensorResultType_)
                << " performed at: " << selfTestResultParams.timestamp_ << " ns\n";
        } else {
            PRINT_CB << tag_ << " Received self test response: " <<
                Utils::getErrorCodeAsString(result);
        }
    });
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "self test request failed: ";
        Utils::printStatus(status);
    } else {
        std::cout << tag_ << "Self test request successful, waiting for callback" << std::endl;
    }
    return status;
}

void SensorClient::onSelfTestFailed() {
    PRINT_CB << tag_ << " Self Test triggered by Sensor service Failed at "
        << Utils::getNanosecondsSinceBoot() << " ns\n";
}

void SensorClient::setRecordingFlag(bool enable) {
   isRecordingEnabled_ = enable;
}
