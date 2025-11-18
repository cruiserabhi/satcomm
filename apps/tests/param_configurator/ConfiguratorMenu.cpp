/*
 *  Copyright (c) 2023, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <sstream>
#include <cstring>

#include <telux/config/ConfigFactory.hpp>
#include <telux/common/Version.hpp>
#include "../../common/utils/Utils.hpp"
#include "ConfiguratorMenu.hpp"

ConfigMenu::ConfigMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

ConfigMenu::~ConfigMenu() {
    if(configManager_) {
        if(configListener_) {
            configManager_->deregisterListener(configListener_);
        }
        configManager_ = nullptr;
    }
}

telux::common::Status ConfigMenu::initConfigManager(std::shared_ptr<IConfigManager>
    &configManager) {
    if(configManager == nullptr) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        auto &configFactory = ConfigFactory::getInstance();
        configManager = configFactory.getConfigManager([&](ServiceStatus status) {
            if (status == ServiceStatus::SERVICE_AVAILABLE) {
                    prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                    prom.set_value(ServiceStatus::SERVICE_FAILED);
                }
            });
        std::chrono::time_point<std::chrono::steady_clock> startTime, endTime;
        startTime = std::chrono::steady_clock::now();
        ServiceStatus configMgrStatus = configManager->getServiceStatus();
        if(configMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Apps Config subsystem is not ready, Please wait" << std::endl;
        }
        configMgrStatus = prom.get_future().get();
        if(configMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count()
                << "s\n" << std::endl;
        } else {
            std::cout << "ERROR - Unable to initialize Apps Config subsystem" << std::endl;
            return telux::common::Status::FAILED;
        }
    } else {
        std::cout<< "Apps Config manager already initialized" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

int ConfigMenu::init() {
    std::shared_ptr<ConsoleAppCommand> getAllConfigs
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "1", "Get All Configs", {},
            std::bind(&ConfigMenu::getAllConfigs, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setConfig
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "2", "Set Config", {},
            std::bind(&ConfigMenu::setConfig, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getConfig
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "3", "Get Config", {},
            std::bind(&ConfigMenu::getConfig, this, std::placeholders::_1)));


    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListConfigSubMenu
        = {getAllConfigs, setConfig, getConfig};

    addCommands(commandsListConfigSubMenu);
    ConsoleApp::displayMenu();

    telux::common::Status status = telux::common::Status::FAILED;
    int rc = 0;
    status = initConfigManager(configManager_);
    if (status != telux::common::Status::SUCCESS) {
        rc = -1;
    } else {
        if(configManager_) {
            configListener_ = std::make_shared<ConfigListener>();
            telux::common::Status status = configManager_->registerListener(configListener_);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "Reg Listener Request Failed" << std::endl;
            }
        }
    }

    return rc;
}

void ConfigMenu::getAllConfigs(std::vector<std::string> userInput) {
    if(configManager_) {
        auto configMap = configManager_->getAllConfigs();
        std::cout << "Current config List - \n";
        for(auto itr: configMap) {
            std::cout << itr.first << " : " << itr.second << "\n";
        }
    }
}

void ConfigMenu::getConfig(std::vector<std::string> userInput) {
    if(configManager_) {
        char delimiter = '\n';
        std::string key;
        std::cout << "Enter the Key for retrieving the value : ";
        std::getline(std::cin, key, delimiter);

        std::string value = configManager_->getConfig(key);
        std::cout << "Corresponding Value: " << value << "\n";
    }
}

void ConfigMenu::setConfig(std::vector<std::string> userInput) {
    if(configManager_) {
        char delimiter = '\n';
        std::string key;
        std::cout << "Enter the Key to be updated : ";
        std::getline(std::cin, key, delimiter);

        std::cin.get();
        std::string value;
        std::cout << "Enter the new Value : ";
        std::getline(std::cin, value, delimiter);

        auto ret = configManager_->setConfig(key, value);
        if(ret == Status::SUCCESS) {
            std::cout << "Success in setting config \n";
        } else {
            std::cout << "Failed to set config \n";
        }
    }
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::string appName = "Configurator Menu - SDK v" + std::to_string(sdkVersion.major) + "."
        + std::to_string(sdkVersion.minor) + "." + std::to_string(sdkVersion.patch) +"\n" +
        "Release name: " + sdkReleaseName;
    ConfigMenu configMenu(appName, "config> ");
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    if(argc > 1) {
        if(strcmp(argv[1],"get")==0) {
            if((argc == 3) && argv[2]) {
                std::string key = argv[2];
                std::shared_ptr<IConfigManager> configManager = nullptr;
                Status status = configMenu.initConfigManager(configManager);
                if(status == Status::SUCCESS && configManager) {
                    std::string value = configManager->getConfig(key);
                    std::cout << "Key: " << key <<" Value: " << value << "\n";
                } else {
                    std::cout << "Failed to initialize config manager \n";
                    return -1;
                }
                if(configManager) {
                    configManager = nullptr;
                }
                return 0;
            } else {
                std::cout << "Invalid cmd line args \n";
                return -1;
            }
        } else if(strcmp(argv[1],"set")==0) {
            if((argc == 4) && argv[2] && argv[3]) {
                std::string key = argv[2];
                std::string value = argv[3];
                std::shared_ptr<IConfigManager> configManager = nullptr;
                Status status = configMenu.initConfigManager(configManager);
                if(status == Status::SUCCESS && configManager) {
                    auto ret = configManager->setConfig(key, value);
                    if(ret == Status::SUCCESS) {
                        std::cout << "Success in setting config \n";
                    } else {
                        std::cout << "Failed to set config \n";
                        return -1;
                    }
                } else {
                    std::cout << "Failed to initialize config manager \n";
                    return -1;
                }
                if(configManager) {
                    configManager = nullptr;
                }
                return 0;
            } else {
                std::cout << "Invalid cmd line args \n";
                return -1;
            }
        }
    }
    if( configMenu.init() == -1) {
        std::cout << "ERROR - Subsystem not ready, Exiting !!!" << std::endl;
        return -1;
    }
    configMenu.mainLoop();
    return 0;
}
