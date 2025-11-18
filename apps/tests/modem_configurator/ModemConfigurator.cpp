/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2021-2022, 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <getopt.h>
#include <iostream>
#include <dirent.h>

#include "ModemConfigurator.hpp"
#include "../../common/utils/Utils.hpp"

using namespace telux::config;

ModemConfigurator::ModemConfigurator() {
    //Setting up default parameters
    slotId_ = 1;
    configType_ = telux::config::ConfigType::SOFTWARE;
}

ModemConfigurator::~ModemConfigurator() {
}

telux::common::Status ModemConfigurator::init() {

    // Get the ConfigFactory and ModemConfigManager instances.
    auto &configFactory = telux::config::ConfigFactory::getInstance();
    std::promise<telux::common::ServiceStatus> prom;
    modemConfigManager_ = configFactory.getModemConfigManager(
        [&prom](telux::common::ServiceStatus status) {
            prom.set_value(status);
    });

    if (!modemConfigManager_) {
        std::cout << "Failed to get modem config Manager" << std::endl;
        return telux::common::Status::FAILED;
    }

    // Wait for init callback
    std::cout << "\nWaiting for modem config subsystem readiness. Please wait ..." << std::endl;
    telux::common::ServiceStatus managerStatus = prom.get_future().get();

    // Exit the application, if SDK is unable to initialize modem config subsystems
    if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
        return telux::common::Status::FAILED;
    }
    telux::common::Status status = modemConfigManager_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Reg Listener Request Failed" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

void ModemConfigurator::cleanup() {
    modemConfigManager_->deregisterListener(shared_from_this());
    modemConfigManager_ = nullptr;
}

void ModemConfigurator::requestConfigList() {
    std::promise<bool> p;
    telux::common::Status status = modemConfigManager_->requestConfigList(
        [&p, this](std::vector<telux::config::ConfigInfo> configList,
                                        telux::common::ErrorCode errCode) {
        if (errCode == telux::common::ErrorCode::SUCCESS) {
            configList_ = configList;
            p.set_value(true);
        } else {
            std::cout << "Failed to get config list" << std::endl;
            p.set_value(false);
        }
    });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Get Config List Request sent" << std::endl;
    } else {
        std::cout << "Get Config List Request failed" << std::endl;
    }

    if (p.get_future().get()) {
        std::cout << "Config List Updated !!" << std::endl;
    }
}

void ModemConfigurator::getActiveConfig() {
    std::promise<bool> p;
    telux::config::ConfigInfo activeConfig;
    telux::common::Status status = modemConfigManager_->getActiveConfig(configType_,
        [&p,&activeConfig](ConfigInfo configInfo, telux::common::ErrorCode errCode) {
            if (errCode == telux::common::ErrorCode::SUCCESS) {
                activeConfig = configInfo;
                p.set_value(true);

            } else {
                std::cout << "Failed to get active config" << std::endl;
                p.set_value(false);
            }
    }, slotId_);

    if (status == telux::common::Status::SUCCESS) {
        std::cout << "get active config request sent" << std::endl;
    } else {
        std::cout << "get active config request failed" << std::endl;
    }

    if (p.get_future().get()) {
        std::cout << "Active Config Details" << std::endl;
        std::string type;
        if (activeConfig.type == telux::config::ConfigType::HARDWARE) {
            type = "HARDWARE";
        } else if(activeConfig.type == telux::config::ConfigType::SOFTWARE) {
            type = "SOFTWARE";
        }
        std::cout << "Type        :" << type << std::endl;
        std::cout << "Size        :" << static_cast<uint32_t>(activeConfig.size) << std::endl;
        std::cout << "Version     :" << static_cast<uint32_t>(activeConfig.version) << std::endl;
        std::cout << "Description :" << activeConfig.desc << std::endl;
    }
}

void ModemConfigurator::loadConfigFile(std::string filePath) {
    std::promise<bool> p;
    FILE* file;
    DIR* directory = opendir(filePath.c_str());
    if (directory != NULL) {
        std::cout << "The path is a directory enter file path" << std::endl;
        return;
    } else {
        file = fopen(filePath.c_str(),"r");
        if(file == nullptr) {
            perror("Error ");
            return;
        }
    }
    telux::common::Status status = modemConfigManager_->loadConfigFile(filePath, configType_,
             [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to load config" << std::endl;
                p.set_value(false);
            }
        });
    if(status == telux::common::Status::SUCCESS) {
        std::cout << "Load config Request sent" << std::endl;
    } else {
        std::cout << "Load config Request failed" << std::endl;
    }

    if (p.get_future().get()) {
             std::cout << "Load config succeeded." << std::endl;
    }
}

void ModemConfigurator::activateConfig(int configNo) {
    std::promise<bool> p;
        std::cout << "Config index is " << configNo << std::endl;

    if (configNo < 0 || configNo > static_cast<int>(configList_.size() -1)) {
        std::cout << "Config index is2 " << configNo << std::endl;
        std::cout << "Config index is3 " << (int)(configList_.size() -1) << std::endl;
        std::cout << "Invalid config index provided" << std::endl;
        return;
    }
    ConfigId configId;
    configId = configList_[configNo].id;

    telux::common::Status status = modemConfigManager_->activateConfig(configType_, configId,
            slotId_, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to activate config" << std::endl;
                p.set_value(false);
            }
        });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Activate Request sent " << std::endl;
    } else {
        std::cout << "Activate Request failed" << std::endl;
    }

    if (p.get_future().get()) {
             std::cout << "config Activated !!" << std::endl;
    }
}

void ModemConfigurator::getSelectionMode() {
    std::promise<bool> p;
    telux::config::AutoSelectionMode selMode;
    telux::common::Status status = modemConfigManager_->getAutoSelectionMode(
            [&p, &selMode](AutoSelectionMode selectionMode, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                selMode = selectionMode;
                p.set_value(true);
            } else {
                std::cout << "Failed to get selection mode" << std::endl;
                p.set_value(false);
            }
        }, slotId_);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Get selection mode Request sent" << std::endl;
    } else {
        std::cout << "Get selection mode Request failed" << std::endl;
    }

    if (p.get_future().get()) {
        if (selMode == telux::config::AutoSelectionMode::DISABLED) {
            std::cout << "DISABLED" << std::endl;
        } else {
            std::cout << "ENABLED" << std::endl;
        }
    }
}

void ModemConfigurator::setSelectionMode(int selMode) {
    std::promise<bool> p;

    telux::config::AutoSelectionMode mode;
    if (selMode == 0) {
        mode = telux::config::AutoSelectionMode::DISABLED;
    } else if (selMode == 1) {
        mode = telux::config::AutoSelectionMode::ENABLED;
    } else {
        std::cout << "Invalid selection Mode" << std::endl;
        return;
    }

    telux::common::Status status = modemConfigManager_->setAutoSelectionMode(mode,
           slotId_, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to set selection mode" << std::endl;
                p.set_value(false);
            }
        });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "set selection mode Request sent" << std::endl;
    } else {
        std::cout << "set selection mode Request failed" << std::endl;
    }

    if (p.get_future().get()) {
             std::cout << "set selection mode succeeded." << std::endl;
    }
}

void ModemConfigurator::deactivateConfig() {
    std::promise<bool> p;

    telux::common::Status status = modemConfigManager_->deactivateConfig(configType_,
           slotId_,[&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to deactivate config" << std::endl;
                p.set_value(false);
            }
        });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Deactivate Request sent" << std::endl;
    } else {
        std::cout << "Deactivate Request failed" << std::endl;
    }

    if (p.get_future().get()) {
             std::cout << "deactivate config succeeded." << std::endl;
    }
}

void ModemConfigurator::deleteConfigFile(int configNo) {
    std::promise<bool> p;

    ConfigId configId;
    if (configNo == -1) {
        configId = "";
    } else {
        if (configNo < -1 || configNo > static_cast<int>(configList_.size() -1)) {
            std::cout << "Invalid config index provided" << std::endl;
            return;
        }
        configId = configList_[configNo].id;
    }

    telux::common::Status status = modemConfigManager_->deleteConfig(configType_,
          configId, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to delete config" << std::endl;
                p.set_value(false);
            }
        });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Delete Request sent successfully" << std::endl;
    } else {
        std::cout << "Delete Request failed" << std::endl;
    }

    if (p.get_future().get()) {
             std::cout << "Delete config succeeded." << std::endl;
    }
}

void ModemConfigurator::printHelp()
{
    std::cout << "             Modem Config App\n"
    << "---------------------------------------------------------------\n"
    << "Note : Application uses default type as SOFTWARE and default slot \n"
    << "as 1. Please change according to your requirement.\n\n"
    << "-t <type>           set config type, '-t 0' for hardware and '-t 1'\n"
    << "                    for software, (default type is software) \n\n"
    << "-s <slot_id>        set slot id -s <slot_id>, (default slot is 1)\n\n"
    << "-i                  get list of configs present in modem storage\n\n"
    << "-l <file_path>      load config to modem storage, we need to specify\n"
    << "                    type and path e,g '-t 1 -l file_path'.\n\n"
    << "-a <config_index>   activate config file, we need to specify type,\n"
    << "                    slot and config index, e.g '-s 1 -t 1 -a 2',\n"
    << "                    for config index, please get config list.\n\n"
    << "-f                  get active config details, we need to specify type\n"
    << "                    and slot, e.g '-s 1 -t 1 -f'.\n\n"
    << "-g                  get selecion mode, we need to specify slot id\n"
    << "                    e.g '-s 1 -g'.\n\n"
    << "-m <mode>           set selection mode, for config we need to specify\n"
    << "-                   slot and mode e.g '-s 1 -m 0', mode values are \n"
    << "                    0 to disable, and 1 to enable auto selection\n\n"
    << "-d                  deactivate config, we need to specify type and \n"
    << "                    slot_id, e.g '-s 1 -t 1 -d.\n\n"
    << "-r <config_index>   remove config from modem storage, we need to specify\n"
    << "                    type and config_index, for finding config index,\n"
    << "                    please get config list e.g '- t 1 -r 2' \n"
    << "                    To delete all configs of specified type, provide \n"
    << "                    config_index as -1 e.g '-t 1 -r -1'\n"
    << "-h                  help\n" << std::endl;
}

void ModemConfigurator::changeConfigType(int type) {
    if (type == 0) {
        configType_ = telux::config::ConfigType::HARDWARE;
    } else if (type == 1) {
        configType_ = telux::config::ConfigType::SOFTWARE;
    } else {
        std::cout << "Unknown config type" << std::endl;
        return;
    }
    std::cout << "config Type changed !!" << std::endl;
}

void ModemConfigurator::changeSlotId(int slotId) {
    slotId_= slotId;
    std::cout << "Slot Id changed to : "<< slotId_ << std::endl;
}

void ModemConfigurator::onConfigUpdateStatus(ConfigUpdateStatus status, int slotId) {
    std::string state;
    if(status == ConfigUpdateStatus::START) {
        state = " Started.";
    } else {
        state = " Completed.";
    }
    std::cout << "Config update on slot id: "<< slotId << state <<std::endl;
}

void ModemConfigurator::onServiceStatusChange(telux::common::ServiceStatus status) {
    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Modem Config service AVAILABLE" << std::endl;
    }
    if (status == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        std::cout << "Modem Config Service NOT AVAILABLE" << std::endl;
    }
}

void ModemConfigurator::printConfigList() {
    std::cout << "Total Configs in Modem " << configList_.size() << std::endl;
    int count = 0;
    for (auto &config : configList_) {
        std::cout << "Config No  : " << count << std::endl;
        std::string type;
        if (config.type == telux::config::ConfigType::HARDWARE) {
            type = "HARDWARE";
        } else if(config.type == telux::config::ConfigType::SOFTWARE) {
            type = "SOFTWARE";
        }
        std::cout << "Type        : " << type << std::endl;
        std::cout << "Size        : " << static_cast<uint32_t>(config.size) << std::endl;
        std::cout << "Version     : " << static_cast<uint32_t>(config.version) << std::endl;
        std::cout << "Description : " << (config.desc) << std::endl << std::endl;
        count++;
    }
}

telux::common::Status ModemConfigurator::parseArguments(int argc, char **argv) {
    int c;
        static struct option long_options[] = {
            {"change config type",        required_argument, 0, 't'},
            {"change slot id",            required_argument, 0, 's'},
            {"get config list",           no_argument, 0, 'i'},
            {"load config",               required_argument, 0, 'l'},
            {"activate config",           required_argument, 0, 'a'},
            {"get active config",         no_argument, 0, 'f'},
            {"get selection mode",        no_argument, 0, 'g'},
            {"set selection mode",        required_argument, 0, 'm'},
            {"deactivate config",         no_argument, 0, 'd'},
            {"delete config",             required_argument, 0, 'r'},
            {"help",                      no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        c = getopt_long(argc, argv, "t:s:il:a:fgm:dr:h", long_options, &option_index);
        if (c == -1) {
            // if no option is entered help is printed.
            c = 'h';
        }
    do {
        switch (c) {
            case 't':
                changeConfigType(atoi(optarg));
                break;
            case 's':
                changeSlotId(atoi(optarg));
                break;
            case 'i':
                requestConfigList();
                printConfigList();
                break;
            case 'l':
                loadConfigFile(optarg);
                break;
            case 'a':
                requestConfigList();
                activateConfig(atoi(optarg));
                break;
            case 'f':
                getActiveConfig();
                break;
            case 'g':
                getSelectionMode();
                break;
            case 'm':
                setSelectionMode(atoi(optarg));
                break;
            case 'd':
                deactivateConfig();
                break;
            case 'r':
                requestConfigList();
                deleteConfigFile(atoi(optarg));
                break;
            case 'h':
                printHelp();
        }
        c = getopt_long(argc, argv, "t:s:il:a:fgm:dr:h", long_options, &option_index);

    } while (c != -1);
    return telux::common::Status::SUCCESS;
}

int main(int argc, char **argv) {
    std::shared_ptr<ModemConfigurator> modemConfigurator = std::make_shared<ModemConfigurator>();
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    if (modemConfigurator) {
        status = modemConfigurator->init();
    }

    if (status == telux::common::Status::SUCCESS) {
        status = modemConfigurator->parseArguments(argc, argv);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Unable to parse" << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        std::cout << "Init Failure. Exiting App" << std::endl;
        return EXIT_FAILURE;
    }
    if (modemConfigurator) {
        modemConfigurator->cleanup();
    }
    return EXIT_SUCCESS;
}
