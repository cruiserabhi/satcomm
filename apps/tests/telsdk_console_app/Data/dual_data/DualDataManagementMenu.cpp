/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <sstream>

extern "C" {
#include "unistd.h"
}

#include "DualDataManagementMenu.hpp"
#include "Utils.hpp"
#include <telux/common/DeviceConfig.hpp>
#include "utils/Utils.hpp"
#include "../DataUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

DualDataManagementMenu::DualDataManagementMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

DualDataManagementMenu::~DualDataManagementMenu() {
}

bool DualDataManagementMenu::init() {
    bool initStatus = initDualDataManager();

    if (not initStatus) {
        return false;
    }
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> getDualDataCapability
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1",
                "get_dual_data_capability", {},
                std::bind(&DualDataManagementMenu::getDualDataCapability, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getDualDataUsageRecommendation
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2",
                "get_dual_data_usage_recommendation", {},
                std::bind(&DualDataManagementMenu::getDualDataUsageRecommendation, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestDdsSwitch
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3",
                "request_dds_switch", {},
                std::bind(&DualDataManagementMenu::requestDdsSwitch, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestCurrentDds
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4",
                "request_current_dds", {},
                std::bind(&DualDataManagementMenu::requestCurrentDds, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> configureDdsSwitchRecommendation
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5",
                "configure_dds_switch_recommendation", {},
                std::bind(&DualDataManagementMenu::configureDdsSwitchRecommendation, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getDdsSwitchRecommendation
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6",
                "get_dds_switch_recommendation", {},
                std::bind(&DualDataManagementMenu::getDdsSwitchRecommendation, this,
                std::placeholders::_1)));
        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
            = {getDualDataCapability, getDualDataUsageRecommendation, requestDdsSwitch,
            requestCurrentDds, configureDdsSwitchRecommendation, getDdsSwitchRecommendation};
        addCommands(commandsList);
    }
    return true;
}

bool DualDataManagementMenu::initDualDataManager() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;

    bool retVal = false;
    auto initCb = std::bind(&DualDataManagementMenu::onInitComplete, this, std::placeholders::_1);
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto dualDataMgr = dataFactory.getDualDataManager(initCb);

    if (dualDataMgr) {
        dualDataMgr->registerListener(shared_from_this());
        std::unique_lock<std::mutex> lck(mtx_);

        telux::common::ServiceStatus subSystemStatus = dualDataMgr->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing "
                      << " DualData Manager subsystem, Please wait \n";
            cv_.wait(lck, [this] { return this->subSystemStatusUpdated_; });
            subSystemStatus = dualDataMgr->getServiceStatus();
        }

        // At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\n"
                      << " DualData Manager is ready" << std::endl;
            retVal = true;
            dualDataManager_ = dualDataMgr;
        } else {
            std::cout << "\n"
                      << " DualData Manager is not ready" << std::endl;
        }
    }
    return retVal;
}

void DualDataManagementMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

bool DualDataManagementMenu::displayMenu() {
    bool retVal = true;
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE ==
        dualDataManager_->getServiceStatus()) {
        std::cout << "\nDual Data Manager is ready " << std::endl;
    }
    else {
        std::cout << "\nDual Data Manager is not ready " << std::endl;
        retVal = false;;
    }
    ConsoleApp::displayMenu();
    return retVal;
}

void DualDataManagementMenu::getDualDataCapability(std::vector<std::string> &inputCommand) {
    std::cout << "get dual data capability" << std::endl;

    bool capability;
    telux::common::ErrorCode errorCode = dualDataManager_->getDualDataCapability(capability);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        if (capability) {
            std::cout << " Device does supports dual data feature." << std::endl;
        } else {
            std::cout << " Device does not support dual data feature." << std::endl;
        }
    } else {
        std::cout << " failed to get dual data capability. ErrorCode: "
                  << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
    }
}

void DualDataManagementMenu::getDualDataUsageRecommendation(
    std::vector<std::string> &inputCommand) {
    std::cout << "get dual data usage recommendation" << std::endl;

    telux::data::DualDataUsageRecommendation recommendation;
    telux::common::ErrorCode errorCode =
        dualDataManager_->getDualDataUsageRecommendation(recommendation);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
            std::cout << " dual data usage is: " << convertRecommendationToString(recommendation)
                      << "." << std::endl;
    } else {
        std::cout << " failed to get dual data usage recommendation. ErrorCode: "
                  << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
    }
}

std::string DualDataManagementMenu::convertRecommendationToString(
    telux::data::DualDataUsageRecommendation recommendation) {
    std::string recommendationStr;

    switch(recommendation) {
        case telux::data::DualDataUsageRecommendation::ALLOWED:
            recommendationStr = "ALLOWED";
        break;
        case telux::data::DualDataUsageRecommendation::NOT_ALLOWED:
            recommendationStr = "NOT_ALLOWED";
        break;
        case telux::data::DualDataUsageRecommendation::NOT_RECOMMENDED:
            recommendationStr = "NOT_RECOMMENDED";
        break;
        default:
        break;
    };

    return recommendationStr;
}

void DualDataManagementMenu::onDualDataCapabilityChange(bool isDualDataCapable) {
    std::cout << "\n\n";
    PRINT_NOTIFICATION << " ** Dual data capability has changed ** \n";
    if(isDualDataCapable) {
        std::cout << "Device does supports dual data feature.";
    } else {
        std::cout << "Device does not support dual data feature.";
    }
    std::cout << std::endl << std::endl;
}

void DualDataManagementMenu::onDualDataUsageRecommendationChange(
        telux::data::DualDataUsageRecommendation recommendation) {
    std::cout << "\n\n";
    PRINT_NOTIFICATION << " ** Dual data usage recommendation has changed ** \n";
    std::cout << "Dual data usage is: " << convertRecommendationToString(recommendation);
    std::cout << std::endl << std::endl;
}

void DualDataManagementMenu::onDdsChange(telux::data::DdsInfo currentState) {
    std::cout << "\n\n";
    PRINT_NOTIFICATION << " ** DDS sub has changed ** \n";
    std::cout <<  "DDS Info : " << "Slot_Id: " << currentState.slotId << std::endl;
        std::string type = (currentState.type == telux::data::DdsType::PERMANENT) ?
            "Permanent" : "Temporary";
        std::cout << "Switch Type: " << type << std::endl;

    std::cout << std::endl << std::endl;
}

void DualDataManagementMenu::onDdsSwitchRecommendation(
    const telux::data::DdsSwitchRecommendation ddsSwitchRecommendation) {
    std::cout << "\n\n";
    PRINT_NOTIFICATION << " ** Received DDS switch recommendation ** \n";
    printDdsSwitchRecommendation(ddsSwitchRecommendation);
}

void DualDataManagementMenu::requestDdsSwitch(std::vector<std::string> &inputCommand) {
    telux::common::Status retStat;
    int operationType;
    std::cout << "Trigger DDS Switch \n";
    if (!dualDataManager_) {
        std::cout << "Dual Data Manager is not ready" << std::endl;
        return;
    }

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    int switchType = 0;
    std::cout << "Enter switch Type (0-Perm_Switch, 1-Temp_Switch): ";
    std::cin >> switchType;
    DataUtils::validateInput(switchType, {0, 1});

    telux::data::DdsInfo requestInfo;

    requestInfo.slotId = static_cast<SlotId>(slotId);
    requestInfo.type = static_cast<telux::data::DdsType>(switchType);

    auto respCb = [](telux::common::ErrorCode error)
    {
        std::cout << std::endl;
        std::cout << "CALLBACK: "
                  << "requestDdsSwitch Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error)
                  << std::endl;
    };

    retStat = dualDataManager_->requestDdsSwitch(requestInfo, respCb);
    Utils::printStatus(retStat);
}

void DualDataManagementMenu::requestCurrentDds(std::vector<std::string> &inputCommand) {
    telux::common::Status retStat;
    int operationType;
    std::cout << "Request current DDS info \n";
    if (!dualDataManager_) {
        std::cout << "Dual Data Manager is not ready" << std::endl;
        return;
    }

    auto respCb = [](telux::data::DdsInfo currentState, telux::common::ErrorCode error)
    {
        std::cout << std::endl
                  << std::endl;

        std::cout << "CALLBACK: "
                  << "requestCurrentDds Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error)
                  << std::endl;

        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Slot_Id: " << currentState.slotId << std::endl;
            std::string type = (currentState.type == telux::data::DdsType::PERMANENT) ?
                "Permanent" : "Temporary";
            std::cout << "Switch Type: " << type << std::endl;
        }
    };

    retStat = dualDataManager_->requestCurrentDds(respCb);
    Utils::printStatus(retStat);
}

void DualDataManagementMenu::configureDdsSwitchRecommendation(
    std::vector<std::string> &inputCommand) {
    std::cout << "Configure DDS switch recommendation \n";
    if (!dualDataManager_) {
        std::cout << "Dual Data Manager is not ready" << std::endl;
        return;
    }
    telux::data::DdsSwitchRecommendationConfig ddsSwitchRecommendationConfig{};

    //Temporary switch recommendation
    int tempRecommendation = 0;
    std::cout << "Temporary switch recommendation (0-Disable, 1-Enable): ";
    std::cin >> tempRecommendation;
    DataUtils::validateInput(tempRecommendation, {0, 1});
    if(tempRecommendation) {
        ddsSwitchRecommendationConfig.enableTemporaryRecommendations = true;
    } else {
        ddsSwitchRecommendationConfig.enableTemporaryRecommendations = false;
    }

    //Permanent switch recommendation
    int permRecommendation = 0;
    std::cout << "Permanent switch recommendation (0-Disable, 1-Enable): ";
    std::cin >> permRecommendation;
    DataUtils::validateInput(permRecommendation, {0, 1});
    if(permRecommendation) {
        ddsSwitchRecommendationConfig.enablePermanentRecommendations = true;
    } else {
        ddsSwitchRecommendationConfig.enablePermanentRecommendations = false;
    }

    if(tempRecommendation | permRecommendation) {
        int type = 0;
        //DDS recommendation basis
        std::cout << "DDS recommendation based on (1-Throughput, 2-Latency): ";
        std::cin >> type;
        DataUtils::validateInput(type, {1, 2});
        ddsSwitchRecommendationConfig.recommBasis =
            static_cast<telux::data::DDSRecommendationBasis>(type);
    }

    telux::common::ErrorCode retStat =
        dualDataManager_->configureDdsSwitchRecommendation(ddsSwitchRecommendationConfig);
    if (retStat == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Successfully Configured DDS switch recommendation " << std::endl;
    } else {
        std::cout << " Configure dds switch recommendation returned with ErrorCode: "
            << static_cast<int>(retStat)
            << ", description: " << Utils::getErrorCodeAsString(retStat)
            << std::endl;
    }
}

void DualDataManagementMenu::getDdsSwitchRecommendation(std::vector<std::string> &inputCommand) {
    std::cout << "Get DDS switch recommendation \n";
    if (!dualDataManager_) {
        std::cout << "Dual Data Manager is not ready" << std::endl;
        return;
    }
    telux::data::DdsSwitchRecommendation ddsSwitchRec{};
    telux::common::ErrorCode retStat =
        dualDataManager_->getDdsSwitchRecommendation(ddsSwitchRec);
    if (retStat == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Getting DDS switch recommendation is successful" << std::endl;
        printDdsSwitchRecommendation(ddsSwitchRec);
    } else {
        std::cout << " Get dds switch recommendation returned with ErrorCode: "
            << static_cast<int>(retStat)
            << ", description: " << Utils::getErrorCodeAsString(retStat)
            << std::endl;
    }
}


void DualDataManagementMenu::printDdsSwitchRecommendation(
        const telux::data::DdsSwitchRecommendation ddsSwitchRec) {
    std::cout << "Recommended DDS Slot_Id: " << ddsSwitchRec.recommendedDdsInfo.slotId << std::endl;
    if(ddsSwitchRec.recommendedDdsInfo.type == telux::data::DdsType::TEMPORARY) {
        std::cout << "Recommendation type : TEMPORARY" << std::endl;
        std::cout << "Temporary recommendation type : " ;
        switch (ddsSwitchRec.recommendationDetails.tempType) {
            case telux::data::TemporaryRecommendationType::REVOKE:
                std::cout << " REVOKE " << std::endl;
                break;
            case telux::data::TemporaryRecommendationType::LOW:
                std::cout << " LOW " << std::endl;;
                break;
            case telux::data::TemporaryRecommendationType::HIGH:
                std::cout << " HIGH " << std::endl;;
                break;
            default:
                std::cout << " UNKNOWN " << std::endl;;
                break;
        }

        std::cout << "Cause: " << ddsSwitchRec.recommendationDetails.tempCause;

        if(ddsSwitchRec.recommendationDetails.tempCause &
            telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_DSDA_IMPOSSIBLE) {
            std::cout << " TEMP_CAUSE_CODE_DSDA_IMPOSSIBLE ";
        }
        if(ddsSwitchRec.recommendationDetails.tempCause &
            telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_DDS_INTERNET_UNAVAIL) {
            std::cout << " TEMP_CAUSE_CODE_DDS_INTERNET_UNAVAIL ";
        }
        if(ddsSwitchRec.recommendationDetails.tempCause &
            telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_TX_SHARING) {
            std::cout << " TEMP_CAUSE_CODE_TX_SHARING ";
        }
        if(ddsSwitchRec.recommendationDetails.tempCause &
            telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_CALL_STATUS_CHANGED) {
            std::cout << " TEMP_CAUSE_CODE_CALL_STATUS_CHANGED ";
        }
        if(ddsSwitchRec.recommendationDetails.tempCause &
            telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_ACTIVE_CALL_ON_DDS) {
            std::cout << " TEMP_CAUSE_CODE_ACTIVE_CALL_ON_DDS ";
        }
        if(ddsSwitchRec.recommendationDetails.tempCause &
            telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_TEMP_REC_DISABLED) {
            std::cout << " TEMP_CAUSE_CODE_TEMP_REC_DISABLED ";
        }
        if(ddsSwitchRec.recommendationDetails.tempCause &
            telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_NON_DDS_INTERNET_UNAVAIL)
        {
            std::cout << " TEMP_CAUSE_CODE_NON_DDS_INTERNET_UNAVAIL ";
        }
        if(ddsSwitchRec.recommendationDetails.tempCause &
        telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_DATA_OFF) {
            std::cout << " TEMP_CAUSE_CODE_DATA_OFF ";
        }
        if(ddsSwitchRec.recommendationDetails.tempCause &
            telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_EMERGENCY_CALL_ON_GOING)
        {
            std::cout << " TEMP_CAUSE_CODE_EMERGENCY_CALL_ON_GOING ";
        }
        if(ddsSwitchRec.recommendationDetails.tempCause &
            telux::data::TemporaryRecommendationCauseCode::TEMP_CAUSE_CODE_DDS_SIM_REMOVED) {
            std::cout << " TEMP_CAUSE_CODE_DDS_SIM_REMOVED ";
        }
        std::cout << std::endl;
    } else {
        std::cout << " Recommendation type : PERMANENT" << std::endl;
        std::cout << " Cause:" << ddsSwitchRec.recommendationDetails.permCause;

        if(ddsSwitchRec.recommendationDetails.permCause &
            telux::data::PermanentRecommendationCauseCode::PERM_CAUSE_CODE_TEMP_CLEAN_UP) {
            std::cout << " PERM_CAUSE_CODE_TEMP_CLEAN_UP ";
        }
        if(ddsSwitchRec.recommendationDetails.permCause &
            telux::data::PermanentRecommendationCauseCode::PERM_CAUSE_CODE_DATA_SETTING_OFF) {
            std::cout << " PERM_CAUSE_CODE_DATA_SETTING_OFF ";
        }
        if(ddsSwitchRec.recommendationDetails.permCause &
            telux::data::PermanentRecommendationCauseCode::PERM_CAUSE_CODE_PS_INVALID) {
            std::cout << " PERM_CAUSE_CODE_PS_INVALID ";
        }
        if(ddsSwitchRec.recommendationDetails.permCause &
            telux::data::PermanentRecommendationCauseCode::PERM_CAUSE_CODE_INTERNET_NOT_AVAIL) {
            std::cout << " PERM_CAUSE_CODE_INTERNET_NOT_AVAIL ";
        }
        std::cout << std::endl;
    }
}