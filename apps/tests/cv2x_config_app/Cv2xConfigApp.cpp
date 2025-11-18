/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xConfigApp.cpp
 *
 * @brief: Simple application that demonstrates Cv2x configuration relevant operations.
 */

#include <iostream>
#include <future>
#include <string>
#include <mutex>
#include <memory>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <stdlib.h>
#include <atomic>

#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include "../../common/utils/Utils.hpp"
#include "Cv2xConfigApp.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::cin;
using std::getline;
using std::promise;
using std::string;
using std::mutex;
using std::make_shared;
using std::shared_ptr;
using std::ifstream;
using std::ofstream;
using std::time;
using std::setw;
using std::lock_guard;

using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ConfigEventInfo;
using telux::cv2x::ConfigEvent;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;

static const string CONFIG_FILE_PATH("/var/tmp/");
static const string CONFIG_FILE("/var/tmp/v2x.xml");
static const string EXPIRY_FILE("/var/tmp/expiry.xml");

class ConfigListener : public ICv2xConfigListener {
public:
    void waitForConfigChangeEvent(ConfigEvent event) {
        //initialize the promise to ignore indications received before calling this API
        promiseSet_ = false;
        configPromise_ = promise<ConfigEvent>();

        while (event != configPromise_.get_future().get()) {
            // the recevied indication is not as expected, wait for the next indication
            configPromise_ = promise<ConfigEvent>();
            promiseSet_ = false;
        }
    }

    void onConfigChanged(const ConfigEventInfo & info) override {
        if (not promiseSet_) {
            promiseSet_ = true;
            configPromise_.set_value(info.event);
        }
    }

private:
    promise<ConfigEvent> configPromise_;
    std::atomic<bool> promiseSet_{false};
};

Cv2xConfigApp::Cv2xConfigApp()
    : ConsoleApp("Cv2x Config Menu", "config> ") {
}

Cv2xConfigApp::~Cv2xConfigApp() {
   if(cv2xConfig_ and configListener_) {
      cv2xConfig_->deregisterListener(configListener_);
   }
}

int Cv2xConfigApp::initialize() {
    if (EXIT_SUCCESS != cv2xInit()) {
        return EXIT_FAILURE;
    }

    consoleInit();

    return EXIT_SUCCESS;
}

int Cv2xConfigApp::cv2xInit() {
    // get handle of cv2x config
    auto & cv2xFactory = Cv2xFactory::getInstance();
    bool cv2xConfigStatusUpdated = false;
    telux::common::ServiceStatus cv2xConfigStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xConfigStatusUpdated = true;
        cv2xConfigStatus = status;
        cv.notify_all();
    };

    cv2xConfig_ = cv2xFactory.getCv2xConfig(statusCb);
    if (!cv2xConfig_) {
        cout << "Failed to get Cv2xConfig" << endl;;
        return EXIT_FAILURE;
    }
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xConfigStatusUpdated; });
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
        cv2xConfigStatus ||
        telux::common::ServiceStatus::SERVICE_AVAILABLE !=
        cv2xConfig_->getServiceStatus()) {
        cout << "Failed to initialize Cv2xConfig" << endl;
        return EXIT_FAILURE;
    }

    // register listener for config change indications
    configListener_ = make_shared<ConfigListener>();
    if (Status::SUCCESS != cv2xConfig_->registerListener(configListener_)) {
        cout << "Error : register Cv2x config listener failed!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Cv2xConfigApp::consoleInit() {
    shared_ptr<ConsoleAppCommand> retrieveCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "1", "Retrieve_Config", {},
        std::bind(&Cv2xConfigApp::retrieveConfigCommand, this)));

    shared_ptr<ConsoleAppCommand> updateCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "2", "Update_Config", {},
        std::bind(&Cv2xConfigApp::updateConfigCommand, this)));

    shared_ptr<ConsoleAppCommand> enforceExpirationCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "3", "Enforce_Config_Expiration", {},
        std::bind(&Cv2xConfigApp::enforceConfigExpirationCommand, this)));

    std::vector<shared_ptr<ConsoleAppCommand>> commandsList
        = {retrieveCmd, updateCmd, enforceExpirationCmd};
    ConsoleApp::addCommands(commandsList);
    ConsoleApp::displayMenu();
}

int Cv2xConfigApp::retrieveConfigFile(string path) {
    cout << "Retrieving config file..." << endl;

    promise<ErrorCode> prom;
    if (Status::SUCCESS != cv2xConfig_->retrieveConfiguration(path,
                                                              [&prom](ErrorCode code)
                                                              {
                                                                  prom.set_value(code);
                                                              })) {
        cout << "Error : Retrieve config file failed!" << endl;
        return EXIT_FAILURE;
    }

    auto res = prom.get_future().get();
    if (ErrorCode::SUCCESS != res) {
        cout << "Error : Retrieve config file failed with code: "
            << static_cast<int>(res) << "!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Cv2xConfigApp::retrieveConfigCommand() {
    string configFilePath;
    string configFileName;
    cout << "CV2X config file will be stored in " << CONFIG_FILE_PATH << endl;
    cout << "Enter the XML file name(e.g., v2x.xml): ";
    getline(cin, configFileName);
    configFilePath = CONFIG_FILE_PATH + configFileName;

    if (EXIT_SUCCESS == retrieveConfigFile(configFilePath)) {
        cout << "Config file saved to " << configFilePath << " with success." << endl;
    } else {
        cout << "Fail to retrieve config file" << endl;
    }

    displayMenu();
}

int Cv2xConfigApp::updateConfigFile(string path) {
    cout << "Updating config file..." << endl;

    promise<ErrorCode> prom;
    if (Status::SUCCESS != cv2xConfig_->updateConfiguration(path,
                                                            [&prom](ErrorCode code)
                                                            {
                                                                prom.set_value(code);
                                                            })) {
        cout << "Error : Update config file failed!" << endl;
        return EXIT_FAILURE;
    }

    auto res = prom.get_future().get();
    if (ErrorCode::SUCCESS != res) {
        cout << "Error : Update config file failed with code: "
            << static_cast<int>(res) << "!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Cv2xConfigApp::updateConfigCommand() {
    string configFilePath;
    string configFileName;
    cout << "Put the v2x configuration XML file under " << CONFIG_FILE_PATH << endl;
    cout << "Then enter the file name(e.g., v2x.xml): ";
    getline(cin, configFileName);
    configFilePath = CONFIG_FILE_PATH + configFileName;

    if (EXIT_SUCCESS == updateConfigFile(configFilePath)) {
        cout << "Update config file successfully." <<  endl;
    }

    displayMenu();
}

int Cv2xConfigApp::generateExpiryConfigFile(string configFilePath, string expiryFilePath) {
    cout << "Generating expiry config file..." << endl;

    // print current timestamp
    std::cout << "Current timestamp:" << time(NULL) << endl;

    // add expiry item to config file
    string timestamp;
    cout << "Enter config expiry timestamp: ";
    getline(cin, timestamp);

    ifstream input(configFilePath);
    if (!input) {
        cout << "Error : open original config file failed!" << endl;
        return EXIT_FAILURE;
    }

    ofstream output(expiryFilePath);
    if (!output) {
        cout << "Error : open expiry config file failed!" << endl;
        return EXIT_FAILURE;
    }

    string line;
    input.unsetf(ifstream::skipws);

    while(!input.eof()) {
        getline(input, line);

        // not copy the line including Expiration tag
        if (string::npos != line.find("<Expiration>")) {
            continue;
        }
        output << line << '\n';

        if (string::npos != line.find("<V2XoverPC5>")) {
            // insert expiry item to the next line
            int len = sizeof("<Expiration>") + 3; // add whitespaces
            output << setw(len) << "<Expiration>" << timestamp << "</Expiration>" << '\n';
        }
    }

    cout << "Current timestamp:" << time(NULL) << endl;

    return EXIT_SUCCESS;
}

int Cv2xConfigApp::enforceConfigExpiration() {
    int ret = EXIT_SUCCESS;

    // generate expiry config file based on the retrieved config file
    // and then update the exipry config file
    if (EXIT_SUCCESS == retrieveConfigFile(CONFIG_FILE) and
        EXIT_SUCCESS == generateExpiryConfigFile(CONFIG_FILE, EXPIRY_FILE) and
        EXIT_SUCCESS == updateConfigFile(EXPIRY_FILE)) {
        auto sp = std::dynamic_pointer_cast<ConfigListener>(configListener_);
        if (sp) {
            // wait until receiving config expiry indication
            cout << "Waiting for config expiry indication..." << endl;
            sp->waitForConfigChangeEvent(ConfigEvent::EXPIRED);

            // wait until receiving config changed indication
            cout << "Waiting for config changed indication..." << endl;
            sp->waitForConfigChangeEvent(ConfigEvent::CHANGED);
        } else {
            ret = EXIT_FAILURE;
        }
    } else {
        ret = EXIT_FAILURE;
    }

    return ret;
}

void Cv2xConfigApp::enforceConfigExpirationCommand() {
    if (EXIT_SUCCESS == enforceConfigExpiration()) {
        cout << "Enforce expiration of Cv2x config successfully." << endl;
    }

    displayMenu();
}

int main(int argc, char *argv[]) {
    std::vector<std::string> groups{"system", "diag", "radio", "logd", "dlt"};
    if (-1 == Utils::setSupplementaryGroups(groups)) {
        cout << "Adding supplementary group failed!" << std::endl;
    }
    shared_ptr<Cv2xConfigApp> cv2xConfig = nullptr;
    try {
        cv2xConfig = make_shared<Cv2xConfigApp>();
    } catch (std::bad_alloc & e) {
        cout << "Error: Create cv2xConfig failed!" << endl;
        return EXIT_FAILURE;
    }

    if (EXIT_SUCCESS != cv2xConfig->initialize()){
        cout << "Error: Initialization failed!" << endl;
        return EXIT_FAILURE;
    }

    // continuously read and execute commands
    return cv2xConfig->mainLoop();
}
