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
 *  Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef SENSORUTILS_HPP
#define SENSORUTILS_HPP

#include <memory>
#include <limits>
#include <string>
#include <sstream>
#include <telux/sensor/SensorDefines.hpp>
#include <telux/power/TcuActivityDefines.hpp>
#include <telux/sensor/Sensor.hpp>

using namespace telux::sensor;

class SensorClient;

class SensorUtils {
 public:
    static std::string getSensorType(SensorType type);
    static void printSensorInfo(SensorInfo info, bool more = false, std::ostream &os = std::cout);
    static std::string getSupportedRates(SensorInfo info);
    static std::string getBatchCountLimits(SensorInfo info);
    static SensorConfiguration getSensorConfig(std::shared_ptr<SensorClient> s);
    static telux::sensor::EulerAngleConfig getEulerAngleConfig();
    static std::shared_ptr<SensorClient> getSensorClient(
        int cid, std::vector<std::shared_ptr<SensorClient>> &sensors);
    template <typename T>
    static void getInput(std::string prompt, T &input) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        std::stringstream ss(line);
        ss >> input;
        bool valid = false;
        do {
            if (!ss.bad() && ss.eof() && !ss.fail()) {
                valid = true;
            } else {
                // If an error occurs then an error flag is set and future attempts to get
                // input will fail. Clear the error flag on cin.
                std::cin.clear();
                // Clear the string stream's states and buffer
                ss.clear();
                ss.str("");
                std::cout << "Invalid input, please re-enter" << std::endl;
                std::cout << prompt;
                std::getline(std::cin, line);
                ss << line;
                ss >> input;
            }
        } while (!valid);
    }
    static void printSensorEvent(
        SensorType type, SensorEvent &s, float samplingRate, std::string &tag);
    static std::string sensorResultTypeToString(SensorResultType sensorResultType);
    static void printSensorFeatureBufferedEvent(SensorEvent &s);
    static bool isUncalibratedSensor(SensorType type);
    static void printSensorFeatureInfo(SensorFeature feature);
    static void printSensorFeatureEvent(SensorFeatureEvent event);
    static void printTcuActivityState(telux::power::TcuActivityState state);
};

struct SensorTestAppArguments {
    /**
     * To enable detailed notifications upon receiving sensor events
     */
    bool verboseNotification;
    /**
     * To reduce verbosity of the sensor events. If quiet is enabled, sensor client will print
     * a summmary every printPeriod seconds
     */
    bool quiet;

    /**
     * The duration between two summary ouputs in quiet mode
     */
    uint32_t printPeriod;
};

#endif  // SENSORUTILS_HPP
