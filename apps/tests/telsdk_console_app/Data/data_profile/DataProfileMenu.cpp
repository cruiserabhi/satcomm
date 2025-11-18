/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021,2023,2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>
#include <sstream>

#include <telux/data/DataFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "DataProfileMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

DataProfileMenu::DataProfileMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   subSystemStatusUpdated_ = false;
}

DataProfileMenu::~DataProfileMenu() {
    myDataProfileListCb_.clear();
    myDataProfileListCbForQuery_.clear();
    myDataCreateProfileCb_.clear();
    myDataProfileCb_.clear();
    myDeleteProfileCb_.clear();
    myModifyProfileCb_.clear();
    myDataProfileCbForGetProfileById_.clear();

    for (auto& profMgr : dataProfileManagerMap_) {
        profMgr.second->deregisterListener(profileListeners_[profMgr.first]);
    }
    dataProfileManagerMap_.clear();
    profileListeners_.clear();
}

bool DataProfileMenu::init() {
    bool dpmSubSystemStatus = initDataProfileManagerAndListener(DEFAULT_SLOT_ID);
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        dpmSubSystemStatus |= initDataProfileManagerAndListener(SLOT_ID_2);
    }

    std::shared_ptr<ConsoleAppCommand> reqProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "request_profile_list", {},
            std::bind(&DataProfileMenu::requestProfileList, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> createProfileMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "create_profile", {},
            std::bind(&DataProfileMenu::createProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> deleteProfileMenu = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("3", "delete_profile",
            {"slotId (1-Primary, 2-Secondary)", "profileId", "techPref (0-3GPP, 1-3GPP2)"},
            std::bind(&DataProfileMenu::deleteProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> modifyProfileMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "modify_profile", {},
            std::bind(&DataProfileMenu::modifyProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> queryProfileMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "query_profile", {},
            std::bind(&DataProfileMenu::queryProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> requestProfileByIdMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "request_profile_by_id",
            {"slotId (1-Primary, 2-Secondary)", "profileId", "techPref (0-3GPP, 1-3GPP2)"},
            std::bind(&DataProfileMenu::requestProfileById, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {reqProfile, createProfileMenu,
        deleteProfileMenu, modifyProfileMenu, queryProfileMenu, requestProfileByIdMenu};

    addCommands(commandsList);

    return dpmSubSystemStatus;
}

bool DataProfileMenu::displayMenu() {
    bool retVal = true;
    if ((dataProfileManagerMap_.find(DEFAULT_SLOT_ID) != dataProfileManagerMap_.end()) &&
        (telux::common::ServiceStatus::SERVICE_AVAILABLE ==
        dataProfileManagerMap_[DEFAULT_SLOT_ID]->getServiceStatus())) {
            std::cout << "\nData Profile Manager on slot "<< DEFAULT_SLOT_ID <<
            " is ready" << std::endl;
    }
    else {
        std::cout << "\nData Profile Manager on slot "<< DEFAULT_SLOT_ID <<
        " is not ready" << std::endl;
        retVal = false;
    }
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        if ((dataProfileManagerMap_.find(SLOT_ID_2) != dataProfileManagerMap_.end()) &&
            (telux::common::ServiceStatus::SERVICE_AVAILABLE ==
            dataProfileManagerMap_[SLOT_ID_2]->getServiceStatus())) {
            std::cout << "\nData Profile Manager on slot "<< SLOT_ID_2 << " is ready" << std::endl;
            retVal = true;
        }
        else {
            std::cout << "\nData Profile Manager on slot "<< SLOT_ID_2 <<
            " is not ready" << std::endl;
            //Intentionally did not set retVal = false to not overwrite slot 1 value
        }
    }
    ConsoleApp::displayMenu();
    return retVal;
}

bool DataProfileMenu::initDataProfileManagerAndListener(SlotId slotId) {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool retValue = false;
    subSystemStatusUpdated_ = false;
    auto initCb = std::bind(&DataProfileMenu::onInitCompleted, this, std::placeholders::_1);
    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto profMgr = dataFactory.getDataProfileManager(slotId, initCb);

    if (profMgr) {
        //  Initialize data profile manager
        std::cout << "\n\nInitializing Data profile manager subsystem on slot " <<
            slotId << ", Please wait ..." << endl;
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
        subSystemStatus = profMgr->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nData Profile Manager on slot "<< slotId << " is ready" << std::endl;
            retValue = true;
        }
        else {
            std::cout << "\nData Profile Manager on slot "<< slotId << " is not ready" << std::endl;
            return false;
        }

        //If this is newly created Manager
        if (dataProfileManagerMap_.find(slotId) == dataProfileManagerMap_.end()) {
            dataProfileManagerMap_.emplace(slotId, profMgr);
            myDataProfileListCb_.emplace(slotId, std::make_shared<MyDataProfilesCallback>());
            myDataProfileListCbForQuery_.emplace(slotId,
                                                 std::make_shared<MyDataProfilesCallback>());
            myDataCreateProfileCb_.emplace(slotId, std::make_shared<MyDataCreateProfileCallback>());
            myDataProfileCb_.emplace(slotId, std::make_shared<MyDataProfileCallback>());
            myDeleteProfileCb_.emplace(slotId, std::make_shared<MyDeleteProfileCallback>());
            myModifyProfileCb_.emplace(slotId, std::make_shared<MyModifyProfileCallback>());
            myDataProfileCbForGetProfileById_.emplace(slotId,
                                                      std::make_shared<MyDataProfileCallback>());
            profileListeners_.emplace(slotId, std::make_shared<MyProfileListener>(slotId));

            telux::common::Status status =
                dataProfileManagerMap_[slotId]->registerListener(profileListeners_[slotId]);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "Unable to register data profile manager listener on slot " <<
                slotId << std::endl;
            }
        }
    } else {
        std::cout << "Data Profile Manager failed to initialize" << std::endl;
    }
    return retValue;
}

void DataProfileMenu::onInitCompleted(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void DataProfileMenu::getProfileParamsFromUser() {
    char delimiter = '\n';
    int techPref;
    std::cout << "Enter Tech Preference (0-3GPP, 1-3GPP2): ";
    std::cin >> techPref;
    Utils::validateInput(techPref, {static_cast<int>(telux::data::TechPreference::TP_3GPP),
        static_cast<int>(telux::data::TechPreference::TP_3GPP2)});

    std::string profileName;
    std::cout << "Enter profileName : ";
    std::getline(std::cin, profileName, delimiter);

    std::string apnName;
    std::cout << "Enter APN : ";
    std::getline(std::cin, apnName, delimiter);

    ApnTypes mask = getApnMask();

    std::string username;
    std::cout << "Enter userName : ";
    std::getline(std::cin, username, delimiter);

    std::string password;
    std::cout << "Enter password : ";
    std::getline(std::cin, password, delimiter);

    int authType;
    std::cout << "Enter Authentication Protocol Type : \n0-None \n1-PAP \n2-CHAP"
                 "\n3-PAP_CHAP\n";
    std::cin >> authType;
    Utils::validateInput(authType, {static_cast<int>(telux::data::AuthProtocolType::AUTH_NONE),
        static_cast<int>(telux::data::AuthProtocolType::AUTH_PAP),
        static_cast<int>(telux::data::AuthProtocolType::AUTH_CHAP),
        static_cast<int>(telux::data::AuthProtocolType::AUTH_PAP_CHAP)});

    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType, {static_cast<int>(telux::data::IpFamilyType::IPV4),
        static_cast<int>(telux::data::IpFamilyType::IPV6),
        static_cast<int>(telux::data::IpFamilyType::IPV4V6)});

    int emergencyAllowed;
    std::cout << "Enter Emergency Enabled (0-UNSPECIFIED, 1-ALLOWED, 2-NOT ALLOWED): ";
    std::cin >>  emergencyAllowed;
    Utils::validateInput(emergencyAllowed,
        {static_cast<int>(telux::data::EmergencyCapability::UNSPECIFIED),
        static_cast<int>(telux::data::EmergencyCapability::ALLOWED),
        static_cast<int>(telux::data::EmergencyCapability::NOT_ALLOWED)});

    params_.profileName = profileName;
    params_.techPref = static_cast<telux::data::TechPreference>(techPref);
    params_.authType = static_cast<telux::data::AuthProtocolType>(authType);
    params_.ipFamilyType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    params_.apn = apnName;
    params_.apnTypes = mask;
    params_.userName = username;
    params_.password = password;
    params_.emergencyAllowed = static_cast<telux::data::EmergencyCapability>(emergencyAllowed);
}

ApnTypes DataProfileMenu::getApnMask() {
    char delimiter = '\n';
    std::string apnMask;
    ApnTypes mask;
    std::vector<int> options;
    std::cout << "Enter the apn type mask to be enabled : \n"
                "0 - DEFAULT, 1 - IMS, 2 - MMS, 3 - DUN, \n"
                "4 - SUPL, 5 - HIPRI , 6 - FOTA, 7 - CBS \n"
                "8 - IA, 9 - EMERGENCY, 10 - UT, 11 - MCX \n"
                "(Example: enter 0,1,3 to enable DEFAULT, IMS and DUN):\n";
    std::getline(std::cin,apnMask,delimiter);
    std::stringstream ss(apnMask);
    int i = -1;
    while(ss >> i) {
    options.push_back(i);
    if(ss.peek() == ',' || ss.peek() == ' ')
        ss.ignore();
    }
    for(auto &opt : options) {
        if(opt >=0 || opt<= 11) {
            try {
                mask.set(opt);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << opt
                    << std::endl;
            }
        } else {
            std::cout << "Apn type mask should not be out of range" << std::endl;
        }
    }
    return mask;
}

void DataProfileMenu::requestProfileList(std::vector<std::string> inputCommand) {
    std::cout << "\nRequest Profile List" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataProfileManagerMap_.find(static_cast<SlotId>(slotId)) == dataProfileManagerMap_.end()) {
        std::cout << "\nData Profile Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->requestProfileList(
            myDataProfileListCb_[static_cast<SlotId>(slotId)]);

    Utils::printStatus(status);
}

void DataProfileMenu::createProfile(std::vector<std::string> inputCommand) {
    std::cout << "\nCreate Profile Request" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataProfileManagerMap_.find(static_cast<SlotId>(slotId)) == dataProfileManagerMap_.end()) {
        std::cout << "\nData Profile Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    getProfileParamsFromUser();

    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->createProfile(
        params_, myDataCreateProfileCb_[static_cast<SlotId>(slotId)]);

    Utils::printStatus(status);
}

void DataProfileMenu::deleteProfile(std::vector<std::string> inputCommand) {
    int slotId, profileId, techPrefId;
    try {
        slotId = std::stoi(inputCommand[1]);
        profileId = std::stoi(inputCommand[2]);
        techPrefId = std::stoi(inputCommand[3]);
    } catch (const std::exception &e) {
        std::cout << "ERROR: Invalid input, please enter numerical values " << std::endl;
        return;
    }
    if (slotId != SLOT_ID_1 && slotId != SLOT_ID_2) {
        std::cout << "Invalid slot id"  << std::endl;
        std::cin.get();
        return;
    }
    if (dataProfileManagerMap_.find(static_cast<SlotId>(slotId)) == dataProfileManagerMap_.end()) {
        std::cout << "\nData Profile Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    if (isDefaultProfile(static_cast<SlotId>(slotId), profileId)) {
        std::cout << "\nCannot delete default profile "
            << profileId << " on slotId " << slotId << std::endl;
        return;
    }

    std::cout << "\nDeleting Profile " << profileId << " on slotId " << slotId << std::endl;
    telux::data::TechPreference tp = telux::data::TechPreference::UNKNOWN;
    if (techPrefId == 0) {
        tp = telux::data::TechPreference::TP_3GPP;
    } else if (techPrefId == 1) {
        tp = telux::data::TechPreference::TP_3GPP2;
    }
    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->deleteProfile(
        profileId, tp, myDeleteProfileCb_[static_cast<SlotId>(slotId)]);
    Utils::printStatus(status);
}

void DataProfileMenu::modifyProfile(std::vector<std::string> inputCommand) {
    std::cout << "\nModify Profile Request" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataProfileManagerMap_.find(static_cast<SlotId>(slotId)) == dataProfileManagerMap_.end()) {
        std::cout << "\nData Profile Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    int profileId;
    std::cout << "Enter profile Id to Modify : ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    getProfileParamsFromUser();

    telux::common::Status status
        = dataProfileManagerMap_[static_cast<SlotId>(slotId)]->modifyProfile(
            profileId, params_, myModifyProfileCb_[static_cast<SlotId>(slotId)]);
    Utils::printStatus(status);
}

void DataProfileMenu::queryProfile(std::vector<std::string> inputCommand) {
    std::cout << "\nQuery Profile Request" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataProfileManagerMap_.find(static_cast<SlotId>(slotId)) == dataProfileManagerMap_.end()) {
        std::cout << "\nData Profile Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    char delimiter = '\n';
    int techPref;
    std::cout << "Enter Tech Preference (0-3GPP, 1-3GPP2): ";
    std::cin >> techPref;
    Utils::validateInput(techPref, {static_cast<int>(telux::data::TechPreference::TP_3GPP),
        static_cast<int>(telux::data::TechPreference::TP_3GPP2)});

    std::string profileName;
    std::cout << "Enter profileName: ";
    std::getline(std::cin, profileName, delimiter);

    std::string apnName;
    std::cout << "Enter APN: ";
    std::getline(std::cin, apnName, delimiter);

    std::string username;
    std::cout << "Enter username: ";
    std::getline(std::cin, username, delimiter);

    std::string password;
    std::cout << "Enter password: ";
    std::getline(std::cin, password, delimiter);

    int authType;
    std::cout << "Enter Authentication Protocol Type : \n0-None \n1-PAP"
                 "\n2-CHAP \n3-PAP_CHAP\n";
    std::cin >> authType;
    Utils::validateInput(authType, {static_cast<int>(telux::data::AuthProtocolType::AUTH_NONE),
        static_cast<int>(telux::data::AuthProtocolType::AUTH_PAP),
        static_cast<int>(telux::data::AuthProtocolType::AUTH_CHAP),
        static_cast<int>(telux::data::AuthProtocolType::AUTH_PAP_CHAP)});


    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPV6, 10-IPV4V6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType, {static_cast<int>(telux::data::IpFamilyType::IPV4),
        static_cast<int>(telux::data::IpFamilyType::IPV6),
        static_cast<int>(telux::data::IpFamilyType::IPV4V6)});

    int emergencyAllowed;
    std::cout << "Enter Emergency Enabled (0-UNSPECIFIED, 1-ALLOWED, 2-NOT ALLOWED): ";
    std::cin >>  emergencyAllowed;
    Utils::validateInput(emergencyAllowed,
        {static_cast<int>(telux::data::EmergencyCapability::UNSPECIFIED),
        static_cast<int>(telux::data::EmergencyCapability::ALLOWED),
        static_cast<int>(telux::data::EmergencyCapability::NOT_ALLOWED)});

    params_.profileName = profileName;
    params_.techPref = static_cast<telux::data::TechPreference>(techPref);
    params_.authType = static_cast<telux::data::AuthProtocolType>(authType);
    params_.ipFamilyType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    params_.apn = apnName;
    params_.userName = username;
    params_.password = password;
    params_.emergencyAllowed = static_cast<telux::data::EmergencyCapability>(emergencyAllowed);

    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->queryProfile(
        params_, myDataProfileListCbForQuery_[static_cast<SlotId>(slotId)]);
    Utils::printStatus(status);
}

void DataProfileMenu::requestProfileById(std::vector<std::string> inputCommand) {
    int slotId, profileId, techPrefId;
    try {
        slotId = std::stoi(inputCommand[1]);
        profileId = std::stoi(inputCommand[2]);
        techPrefId = std::stoi(inputCommand[3]);
    } catch (const std::exception &e) {
        std::cout << "ERROR: Invalid input, please enter numerical values " << std::endl;
        return;
    }
    if (slotId != SLOT_ID_1 && slotId != SLOT_ID_2) {
        std::cout << "Invalid slot id"  << std::endl;
        std::cin.get();
        return;
    }
    if (dataProfileManagerMap_.find(static_cast<SlotId>(slotId)) == dataProfileManagerMap_.end()) {
        std::cout << "\nData Profile Manager on slot "<< slotId << " is not ready" << std::endl;
        return;
    }

    std::cout << "\nRequest Profile By Id " << profileId << " on slotId " << slotId << std::endl;
    telux::data::TechPreference tp = telux::data::TechPreference::UNKNOWN;
    if (techPrefId == 0) {
        tp = telux::data::TechPreference::TP_3GPP;
    } else if (techPrefId == 1) {
        tp = telux::data::TechPreference::TP_3GPP2;
    }
    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->requestProfile(
        profileId, tp, myDataProfileCbForGetProfileById_[static_cast<SlotId>(slotId)]);
    Utils::printStatus(status);
}


bool DataProfileMenu::isDefaultProfile(SlotId slotId, int profileId) {

    // in case of error we should be preventing this profile from getting deleted.
    // return true.
    if (!initalizeDCM(slotId)) {
        return true;
    }

    int localProfileId = getDefaultProfile(slotId, telux::data::OperationType::DATA_LOCAL);
    int remoteProfileId = getDefaultProfile(slotId, telux::data::OperationType::DATA_REMOTE);

    if (((localProfileId != -1) && (profileId == localProfileId)) ||
            ((remoteProfileId != -1) && (profileId == remoteProfileId))) {
        dataConnectionManagerMap_.clear();
        return true;
    }
    dataConnectionManagerMap_.clear();
    return false;
}

int DataProfileMenu::getDefaultProfile(SlotId slotId, telux::data::OperationType opr) {

    std::promise<telux::common::ErrorCode> prom{};
    int profileId = -1;

    auto defaultProfileCb =
    [&prom, &profileId](int pId, SlotId slotId, telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            profileId = pId;
        }
        prom.set_value(error);
    };

    if (dataConnectionManagerMap_.find(slotId) == dataConnectionManagerMap_.end()) {
        return -1;
    }

    telux::common::Status status =
        dataConnectionManagerMap_[slotId]->getDefaultProfile(opr, defaultProfileCb);

    if (status == telux::common::Status::SUCCESS) {
        telux::common::ErrorCode errCode = prom.get_future().get();
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            return -1;
        }
    }
    return profileId;
}

bool DataProfileMenu::initalizeDCM(SlotId slotId) {

    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool retValue = false;
    std::promise<telux::common::ServiceStatus> prom{};

    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto conMgr = dataFactory.getDataConnectionManager(slotId,
        [&prom](telux::common::ServiceStatus status) { prom.set_value(status); });

    if (conMgr) {
        //  Initialize data connection manager
        std::cout << "\n\nInitializing Data connection manager subsystem on slot " <<
            slotId << ", Please wait ..." << endl;
        subSystemStatus = prom.get_future().get();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nData Connection Manager on slot "<< slotId << " is ready" << std::endl;
            retValue = true;
        } else {
            std::cout << "\nData Connection Manager on slot "<< slotId
                << " is not ready" << std::endl;
            return false;
        }

        //If this is newly created Manager
        if (dataConnectionManagerMap_.find(slotId) == dataConnectionManagerMap_.end()) {
            dataConnectionManagerMap_.emplace(slotId, conMgr);
        }
    } else {
        std::cout << "Data Connection Manager failed to initialize" << std::endl;
    }
    return retValue;
}
