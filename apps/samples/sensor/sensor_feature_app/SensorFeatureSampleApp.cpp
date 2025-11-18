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

#include <future>
#include <getopt.h>
#include <iostream>
#include <vector>
#include <condition_variable>

#include <telux/sensor/SensorFactory.hpp>

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m"

class SensorFeatureEventListener : public telux::sensor::ISensorFeatureEventListener {
 public:
    SensorFeatureEventListener() {
    }

    // [8] Receive sensor feature events. This notification is received every time there is an
    // event generated on enabled features
    virtual void onEvent(telux::sensor::SensorFeatureEvent event) override {
        printSensorFeatureEvent(event);
    }

 private:
    void printSensorFeatureEvent(telux::sensor::SensorFeatureEvent &event) {
        PRINT_NOTIFICATION << ": name " << event.name << ": " << event.timestamp << ", " << event.id
                           << std::endl;
    }
};

void printSensorFeatureInfo(telux::sensor::SensorFeature feature) {
    std::cout << "Name: " << feature.name << std::endl;
}

void printHelp(std::string programName, std::vector<telux::sensor::SensorFeature> &sensorFeatures) {
    std::cout << "Usage: " << programName << " [-fh]" << std::endl
              << std::endl
              << "-f <name>    Name of the feature to be enabled" << std::endl
              << "-h           This help" << std::endl;

    std::cout << "Available features: ";
    for (telux::sensor::SensorFeature feature : sensorFeatures) {
        std::cout << feature.name << ", ";
    }
    std::cout << "\b\b  " << std::endl;
}

void parseArgs(int argc, char **argv, std::string &name,
    std::vector<telux::sensor::SensorFeature> &sensorFeatures) {
    int c = -1;
    static const struct option long_options[] = {{"sensor feature name", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};
    int option_index = 0;
    c = getopt_long(argc, argv, "f:h", long_options, &option_index);
    if (c == -1) {
        if (sensorFeatures.size() > 0) {
            name = sensorFeatures[0].name;
            std::cout << "Enabling feature: " << name << std::endl;
        } else {
            std::cout << "No sensors features found" << std::endl;
            name = "";
        }
        return;
    }
    do {
        switch (c) {
            case 'f': {
                name = optarg;
                break;
            }
            case 'h': {
                printHelp(argv[0], sensorFeatures);
                exit(0);
            }
        }
        c = getopt_long(argc, argv, "f:h", long_options, &option_index);
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

    // [3] Get the sensor feature manager
    std::shared_ptr<telux::sensor::ISensorFeatureManager> sensorFeatureManager
        = sensorFactory.getSensorFeatureManager(initCb);
    if (sensorFeatureManager == nullptr) {
        std::cout << "sensor feature manager is nullptr" << std::endl;
        exit(1);
    }
    std::cout << "obtained sensor feature manager" << std::endl;

    // [4] Wait until initialization is complete
    p.get_future().get();
    if (sensorFeatureManager->getServiceStatus()
        != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Sensor feature service not available" << std::endl;
        exit(1);
    }

    // [5] Get information on available sensor features
    std::cout << "Sensor feature service is now available" << std::endl;
    std::vector<telux::sensor::SensorFeature> sensorFeatures;
    telux::common::Status status = sensorFeatureManager->getAvailableFeatures(sensorFeatures);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get information on available features" << static_cast<int>(status)
                  << std::endl;
        exit(1);
    }
    parseArgs(argc, argv, name, sensorFeatures);
    if (name == "") {
        exit(0);
    }
    std::cout << "Received sensor features" << std::endl;
    for (auto feature : sensorFeatures) {
        printSensorFeatureInfo(feature);
    }

    // [6] Create a listener for the sensor feature events
    std::shared_ptr<SensorFeatureEventListener> sensorFeatureEventListener
        = std::make_shared<SensorFeatureEventListener>();
    sensorFeatureManager->registerListener(sensorFeatureEventListener);

    // [7] Enable the desired features
    // Note: Enabling a sensor feature when the system is active would additionally require
    // enabling the corresponding sensor which is used by the sensor feature.
    // If the sensor feature only needs to be enabled during suspend mode, just enabling the sensor
    // feature using this method would be sufficient. The underlying framework would take care
    // to enable the required sensor when the system is about to enter suspend state.

    status = sensorFeatureManager->enableFeature(name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to enable feature: " << name << std::endl;
        exit(1);
    }

    std::cout << "\n\nWait to receive further notifications OR press ENTER to exit \n\n";
    std::cin.ignore();

    // [9] Disable the sensor features
    status = sensorFeatureManager->disableFeature(name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to disable feature: " << name << std::endl;
        exit(1);
    }

    // [10] When sensor feature manager is no longer required, delete the sensor feature manager
    // object
    sensorFeatureManager = nullptr;

    return 0;
}
