/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "TelUtil.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "protos/proto-src/event_simulation.grpc.pb.h"

#define PHONE_JSON_API_PATH1 "api/tel/IPhoneManagerSlot1.json"
#define PHONE_JSON_API_PATH2 "api/tel/IPhoneManagerSlot2.json"
#define PHONE_JSON_STATE_PATH1 "system-state/tel/IPhoneManagerStateSlot1.json"
#define PHONE_JSON_STATE_PATH2 "system-state/tel/IPhoneManagerStateSlot2.json"
#define SERVING_JSON_API_PATH1 "api/tel/IServingSystemManagerSlot1.json"
#define SERVING_JSON_API_PATH2 "api/tel/IServingSystemManagerSlot2.json"
#define SERVING_JSON_STATE_PATH1 "system-state/tel/IServingSystemManagerStateSlot1.json"
#define SERVING_JSON_STATE_PATH2 "system-state/tel/IServingSystemManagerStateSlot2.json"
#define DEFAULT_DELIMITER " "

#define TEL_PHONE_MANAGER                       "IPhoneManager"
#define TEL_SERVING_MANAGER                     "IServingSystemManager"
#define INVALID_SIGNAL_STRENGTH_VALUE 0x7FFFFFFF

#define SLOT_1 1
#define SLOT_2 2
#define DEFAULT_SLOT_ID SLOT_1
#define MAX_THRESHOLD_LIST 10

namespace telux {
namespace tel {

telux::common::ErrorCode TelUtil::readFromJsonFile(int phoneId, std::string subsystem,
    Json::Value &rootObj, std::string &jsonfilename) {
    if (subsystem == TEL_PHONE_MANAGER) {
        jsonfilename = (phoneId == SLOT_1) ? PHONE_JSON_STATE_PATH1 : PHONE_JSON_STATE_PATH2;
    } else if(subsystem == TEL_SERVING_MANAGER) {
        jsonfilename = (phoneId == SLOT_1) ? SERVING_JSON_STATE_PATH1 : SERVING_JSON_STATE_PATH2;
    }
    return JsonParser::readFromJsonFile(rootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::readJsonData(int phoneId, std::string subsystem,
    std::string method, JsonData &data) {
    std::string stateJsonPath = "";
    return TelUtil::readJsonData(phoneId, subsystem, method, data, stateJsonPath);
}

telux::common::ErrorCode TelUtil::readJsonData(int phoneId, std::string subsystem,
    std::string method, JsonData &data, std::string &stateJsonPath) {
    std::string apiJsonPath = "";
    if (subsystem == TEL_PHONE_MANAGER) {
        apiJsonPath = (phoneId == SLOT_1)? PHONE_JSON_API_PATH1 : PHONE_JSON_API_PATH2;
        stateJsonPath = (phoneId == SLOT_1)? PHONE_JSON_STATE_PATH1 : PHONE_JSON_STATE_PATH2;
    } else if(subsystem == TEL_SERVING_MANAGER) {
        apiJsonPath = (phoneId == SLOT_1)? SERVING_JSON_API_PATH1 : SERVING_JSON_API_PATH2;
        stateJsonPath = (phoneId == SLOT_1)? SERVING_JSON_STATE_PATH1 : SERVING_JSON_STATE_PATH2;
    }
    return CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);
}

JsonData TelUtil::readGetPhoneIdsRespFromJsonFile(telStub::GetPhoneIdsReply *response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(DEFAULT_SLOT_ID, TEL_PHONE_MANAGER,
        "getPhoneIds", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return data;
    }
    response->set_status(static_cast<commonStub::Status>(data.status));
    return data;
}

JsonData TelUtil::readGetPhoneIdRespFromJsonFile(telStub::GetPhoneIdReply *response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    telux::common::ErrorCode error = readJsonData(DEFAULT_SLOT_ID, TEL_PHONE_MANAGER,
        "getPhoneId", data);
    if (ErrorCode::SUCCESS != error) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return data;
    }
    response->set_status(static_cast<commonStub::Status>(data.status));
    return data;
}

JsonData TelUtil::readSignalStrengthRespFromJsonFile(int phoneId,
    telStub::GetSignalStrengthReply *response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    telux::common::ErrorCode error = readJsonData(phoneId, TEL_PHONE_MANAGER,
        "requestSignalStrength", data);
    if (ErrorCode::SUCCESS != error) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return data;
    }
    if (data.status == telux::common::Status::SUCCESS) {
        // Create response
        telStub::RadioTechnology servingRat;
        telStub::ServiceDomainInfo_Domain servingDomain;
        error = TelUtil::readSystemInfoFromJsonFile(phoneId, servingRat, servingDomain);
        data.error = error;
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading System Info failed" );
            data.status = telux::common::Status::FAILED;
            return data;
        }
        telStub::OperatingModeEvent event;
        error = TelUtil::readOperatingModeEventFromJsonFile(event);

        // gsm signal strength
        if (servingRat == telStub::RadioTechnology::RADIO_TECH_GSM &&
                event.operating_mode() ==  telStub::OperatingMode::ONLINE) {
            response->mutable_signal_strength()->mutable_gsm_signal_strength_info()->
                set_gsm_signal_strength(data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]
                ["gsmSignalStrengthInfo"]["gsmSignalStrength"].asInt());
            response->mutable_signal_strength()->mutable_gsm_signal_strength_info()->
                set_gsm_bit_error_rate(data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                    ["gsmSignalStrengthInfo"]["gsmBitErrorRate"].asInt());
        } else {
            response->mutable_signal_strength()->mutable_gsm_signal_strength_info()->\
                set_gsm_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_gsm_signal_strength_info()->\
                set_gsm_bit_error_rate(INVALID_SIGNAL_STRENGTH_VALUE);
        }
        LOG(DEBUG, __FUNCTION__, " gsmSignalStrength: ", response->mutable_signal_strength()->\
            mutable_gsm_signal_strength_info()->gsm_signal_strength(),
            " gsmBitErrorRate: ", response->mutable_signal_strength()->\
            mutable_gsm_signal_strength_info()->gsm_bit_error_rate());

        // lte signal strength
        if (servingRat == telStub::RadioTechnology::RADIO_TECH_LTE &&
                event.operating_mode() ==  telStub::OperatingMode::ONLINE) {
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->
                set_lte_signal_strength(data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                    ["lteSignalStrengthInfo"]["lteSignalStrength"].asInt());
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rsrp(
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                ["lteRsrp"].asInt());
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rsrq(
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                ["lteRsrq"].asInt());
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rssnr(
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                ["lteRssnr"].asInt());
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_cqi(
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                ["lteCqi"].asInt());
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->\
                set_timing_advance(data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["lteSignalStrengthInfo"]["timingAdvance"].asInt());
        } else {
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->
                set_lte_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rsrp(
                INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rsrq(
                INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rssnr(
                INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_cqi(
                INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->\
                set_timing_advance(INVALID_SIGNAL_STRENGTH_VALUE);
        }
        LOG(DEBUG, __FUNCTION__, " lteSignalStrength: ",
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->\
                lte_signal_strength(), " lteRsrp: ",
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->\
                lte_rsrp(), " lteRsrq: ",
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->\
                lte_rsrq(),  " lteRssnr: ",
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->\
                lte_rssnr(), " lteCqi: ",
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->\
                lte_cqi(), " timingAdvance: ",
            response->mutable_signal_strength()->mutable_lte_signal_strength_info()->\
                timing_advance());

        // wcdma signal strength
        if (servingRat == telStub::RadioTechnology::RADIO_TECH_UMTS &&
                event.operating_mode() ==  telStub::OperatingMode::ONLINE) {
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
            set_signal_strength(data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["wcdmaSignalStrengthInfo"]["signalStrength"].asInt());
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                set_bit_error_rate(data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                    ["wcdmaSignalStrengthInfo"]["bitErrorRate"].asInt());
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                set_ecio(data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                    ["wcdmaSignalStrengthInfo"]["ecio"].asInt());
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                set_rscp(data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                    ["wcdmaSignalStrengthInfo"]["rscp"].asInt());
        } else {
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                set_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                set_bit_error_rate(INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                set_ecio(INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                set_rscp(INVALID_SIGNAL_STRENGTH_VALUE);
        }
        LOG(DEBUG, __FUNCTION__, " wcdmaSignalStrength: ",
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->\
            signal_strength(), " bitErrorRate: ",
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->\
            bit_error_rate(), " ecio: ",
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->\
            ecio(), " rscp: ",
            response->mutable_signal_strength()->mutable_wcdma_signal_strength_info()->\
            rscp());

        // nr5g signal strength
        if (servingRat == telStub::RadioTechnology::RADIO_TECH_NR5G &&
                event.operating_mode() ==  telStub::OperatingMode::ONLINE) {
            response->mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rsrp(
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrp"].asInt());
            response->mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rsrq(
                data.stateRootObj[TEL_PHONE_MANAGER]\
                ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrq"].asInt());
            response->mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rssnr(
                data.stateRootObj[TEL_PHONE_MANAGER]\
                ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rssnr"].asInt());
        } else {
            response->mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rsrp(
                INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rsrq(
                INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rssnr(
                INVALID_SIGNAL_STRENGTH_VALUE);
        }
        LOG(DEBUG, __FUNCTION__, " nr5gRsrp: ",
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrp"].asInt(), " nr5gRsrq: ",
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrq"].asInt(), " nr5gRssnr : ",
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rssnr"].asInt());
        // NB1 NTN signal strength
        if (servingRat == telStub::RadioTechnology::RADIO_TECH_NB1_NTN) {
            response->mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
                set_signal_strength(data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["nb1NtnSignalStrengthInfo"]["signalStrength"].asInt());
            response->mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->set_rsrp(
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["nb1NtnSignalStrengthInfo"]["rsrp"].asInt());
            response->mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->set_rsrq(
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["nb1NtnSignalStrengthInfo"]["rsrq"].asInt());
            response->mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->set_rssnr(
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["nb1NtnSignalStrengthInfo"]["rssnr"].asInt());
        } else {
            response->mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
                set_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()
                ->set_rsrp(INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()
                ->set_rsrq(INVALID_SIGNAL_STRENGTH_VALUE);
            response->mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()
                ->set_rssnr(INVALID_SIGNAL_STRENGTH_VALUE);
       }
       LOG(DEBUG, __FUNCTION__, " nb1NtnSignalStrength: ",
           data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
           ["signalstrength"].asInt(), " nb1NtnRsrp: ",  data.stateRootObj[TEL_PHONE_MANAGER]\
           ["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]["rsrp"].asInt(), " nb1NtnRsrq: ",
           data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
           ["rsrq"].asInt(), " nb1NtnRssnr: ",  data.stateRootObj[TEL_PHONE_MANAGER]\
           ["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]["rssnr"].asInt());
        response->set_phone_id(phoneId);
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to fetch signal strength");
    }
    updateResponse(response, data);
    return data;
}

JsonData TelUtil::readVoiceServiceStateRespFromJsonFile(int phoneId,
        telStub::RequestVoiceServiceStateReply *response) {
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(phoneId, TEL_PHONE_MANAGER, "requestVoiceServiceState",
        data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return data;
    }
    if (data.status == telux::common::Status::SUCCESS) {
        telStub::RadioTechnology servingRat;
        telStub::ServiceDomainInfo_Domain servingDomain;
        telux::common::ErrorCode error = TelUtil::readSystemInfoFromJsonFile(phoneId, servingRat,
            servingDomain);
        data.error = error;
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading System Info failed" );
            data.status = telux::common::Status::FAILED;
            return data;
        }
        telStub::OperatingModeEvent event;
        error = TelUtil::readOperatingModeEventFromJsonFile(event);

        if (event.operating_mode() ==  telStub::OperatingMode::ONLINE) {
            telStub::VoiceServiceState voiceServiceState = static_cast<telStub::VoiceServiceState>(
                data.stateRootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]\
                ["voiceServiceState"].asInt());
            LOG(DEBUG, __FUNCTION__," VoiceServiceState is :", static_cast<int>(voiceServiceState));
            response->mutable_voice_service_state_info()->set_voice_service_state(
                voiceServiceState);
            telStub::VoiceServiceDenialCause voiceServiceDenialCause =
                static_cast<telStub::VoiceServiceDenialCause>(data.stateRootObj[TEL_PHONE_MANAGER]\
                ["voiceServiceStateInfo"]["voiceServiceDenialCause"].asInt());
            LOG(DEBUG, __FUNCTION__," VoiceServiceDenialCause is :",
                    static_cast<int>(voiceServiceDenialCause));
            response->mutable_voice_service_state_info()->set_voice_service_denial_cause(
                voiceServiceDenialCause);
            telStub::RadioTechnology radioTech = static_cast<telStub::RadioTechnology>(
                data.stateRootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]["radioTech"].asInt());
            LOG(DEBUG, __FUNCTION__," RadioTech is :", static_cast<int>(radioTech));
            response->mutable_voice_service_state_info()->set_radio_technology(radioTech);
        } else {
            LOG(DEBUG, __FUNCTION__, " Operating Mode other than online" );
            response->mutable_voice_service_state_info()->set_voice_service_state(
                telStub::VoiceServiceState::NOT_REG_AND_SEARCHING);
            response->mutable_voice_service_state_info()->set_voice_service_denial_cause(
                telStub::VoiceServiceDenialCause::GENERAL);
            response->mutable_voice_service_state_info()->set_radio_technology(
                telStub::RadioTechnology::RADIO_TECH_UNKNOWN);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to fetch voice service state");
    }
    updateResponse(response, data);
    return data;
}

JsonData TelUtil::readCellularCapabilitiesRespFromJsonFile(
    telStub::CellularCapabilityInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(DEFAULT_SLOT_ID, TEL_PHONE_MANAGER,
        "requestCellularCapabilityInfo", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return data;
    }
    if (data.status == telux::common::Status::SUCCESS) {
        int size = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
            ["voiceTech"].size();

        for (auto index = 0; index < size; index++) {
            response->mutable_capability_info()->add_voice_service_techs(
                convertVoiceTechStringToEnum(data.stateRootObj[TEL_PHONE_MANAGER]\
                ["cellularCapabilityInfo"]["voiceTech"][index].asString()));
        }
        response->mutable_capability_info()->set_sim_count(data.stateRootObj[TEL_PHONE_MANAGER]\
            ["cellularCapabilityInfo"]["simCount"].asInt());
        response->mutable_capability_info()->set_max_active_sims(
            data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
            ["maxActiveSims"].asInt());
        int simRATCapSize = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
            ["SimRATCapabilities"].size();
        for (auto i = 0; i < simRATCapSize; i++) {
            telStub::SimRatCapability *simCaps = response->mutable_capability_info()->
                add_sim_rat_capabilities();
            simCaps->set_phone_id(data.stateRootObj[TEL_PHONE_MANAGER]\
                ["cellularCapabilityInfo"]["SimRATCapabilities"][i]["slotId"].asInt());
            int ratCapSize = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
                ["SimRATCapabilities"][i]["capabilities"].size();
            for (auto j = 0; j < ratCapSize; j++) {
                simCaps->add_capabilities(
                convertRATCapStringToEnum(data.stateRootObj[TEL_PHONE_MANAGER]\
                    ["cellularCapabilityInfo"]["SimRATCapabilities"][i]["capabilities"]\
                    [j].asString()));
            }
        }
        int deviceRATCapSize = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
            ["DeviceRATCapabilities"].size();
        for (auto i = 0; i < deviceRATCapSize; i++) {
            telStub::SimRatCapability *deviceCaps =
                response->mutable_capability_info()->add_device_rat_capability();
            deviceCaps->set_phone_id(data.stateRootObj[TEL_PHONE_MANAGER]\
                ["cellularCapabilityInfo"]["DeviceRATCapabilities"][i]["slotId"].asInt());
            int sizeDeviceCap = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
                ["DeviceRATCapabilities"][i]["capabilities"].size();
            for (auto j = 0; j < sizeDeviceCap; j++) {
                deviceCaps->add_capabilities(
                convertRATCapStringToEnum(data.stateRootObj[TEL_PHONE_MANAGER]\
                    ["cellularCapabilityInfo"]\
                    ["DeviceRATCapabilities"][i]["capabilities"][j].asString()));
            }
        }
    }
    //Update response
    updateResponse(response, data);
    return data;
}

JsonData TelUtil::readOperatingModeRespFromJsonFile(telStub::GetOperatingModeReply *response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    telux::common::ErrorCode error = readJsonData(DEFAULT_SLOT_ID, TEL_PHONE_MANAGER,
        "requestOperatingMode", data);
    if (ErrorCode::SUCCESS != error) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return data;
    }
    if (data.status == telux::common::Status::SUCCESS) {
        int opMode = data.stateRootObj[TEL_PHONE_MANAGER]["operatingModeInfo"]\
            ["operatingMode"].asInt();
        telStub::OperatingMode operatingMode = static_cast<telStub::OperatingMode>(opMode);
        response->set_operating_mode(operatingMode);
        LOG(DEBUG, __FUNCTION__," Operating Mode is :", static_cast<int>(operatingMode));
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to fetch operating mode setting");
    }
    updateResponse(response, data);
    return data;
}

JsonData TelUtil::readCellInfoListRespFromJsonFile(int phoneId,
    telStub::RequestCellInfoListReply *response) {
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(phoneId, TEL_PHONE_MANAGER, "requestCellInfo", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return data;
    }

    if (data.status == telux::common::Status::SUCCESS) {
        telStub::RadioTechnology servingRat;
        telStub::ServiceDomainInfo_Domain servingDomain;
        telux::common::ErrorCode error = TelUtil::readSystemInfoFromJsonFile(phoneId, servingRat,
            servingDomain);
        data.error = error;
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading System Info failed" );
            data.status = telux::common::Status::FAILED;
            return data;
        }

        telStub::OperatingModeEvent event;
        error = TelUtil::readOperatingModeEventFromJsonFile(event);

        if((error == ErrorCode::SUCCESS) &&
            (event.operating_mode() == telStub::OperatingMode::ONLINE)) {
        // Create response
            int newCellCount = data.stateRootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"].size();
            LOG(DEBUG, __FUNCTION__, " newCellCount: ", newCellCount);
            for (int i = 0 ; i < newCellCount; i++) {
                telStub::CellInfoList *cellInfo = response->add_cell_info_list();
                Json::Value requestedCell =
                    data.stateRootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"][i];
                // whether any cell registered
                int isRegistered = requestedCell["registered"].asInt();
                cellInfo->mutable_cell_type()->set_registered(isRegistered);
                telStub::CellInfo_CellType cellType =
                    static_cast<telStub::CellInfo_CellType>(requestedCell["cellType"].asInt());
                cellInfo->mutable_cell_type()->set_cell_type(
                    static_cast<telStub::CellInfo_CellType>(cellType));
                switch(cellType) {
                    case telStub::CellInfo_CellType_GSM: {
                        // cell identity
                        cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_mcc(
                            requestedCell["gsmCellInfo"]["gsmCellIdentity"]["mcc"].asString());
                        cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_mnc(
                            requestedCell["gsmCellInfo"]["gsmCellIdentity"]["mnc"].asString());
                        cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_lac(
                            requestedCell["gsmCellInfo"]["gsmCellIdentity"]["lac"].asInt());
                        cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_cid(
                            requestedCell["gsmCellInfo"]["gsmCellIdentity"]["cid"].asInt());
                        cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_arfcn(
                            requestedCell["gsmCellInfo"]["gsmCellIdentity"]["arfcn"].asInt());
                        cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_bsic(
                            requestedCell["gsmCellInfo"]["gsmCellIdentity"]["bsic"].asInt());
                        // signal strength
                        cellInfo->mutable_gsm_cell_info()->mutable_gsm_signal_strength_info()->
                            set_gsm_signal_strength(requestedCell["gsmCellInfo"]\
                            ["gsmSignalStrengthInfo"]["gsmSignalStrength"].asInt());
                        cellInfo->mutable_gsm_cell_info()->mutable_gsm_signal_strength_info()->
                            set_gsm_bit_error_rate(requestedCell["gsmCellInfo"]\
                            ["gsmSignalStrengthInfo"]["gsmBitErrorRate"].asInt());
                        break;
                    }
                    case telStub::CellInfo_CellType_LTE: {
                        // cell identity
                        cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_mcc(
                            requestedCell["lteCellInfo"]["lteCellIdentity"]["mcc"].asString());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_mnc(
                            requestedCell["lteCellInfo"]["lteCellIdentity"]["mnc"].asString());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_ci(
                            requestedCell["lteCellInfo"]["lteCellIdentity"]["ci"].asInt());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_pci(
                            requestedCell["lteCellInfo"]["lteCellIdentity"]["pci"].asInt());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_tac(
                            requestedCell["lteCellInfo"]["lteCellIdentity"]["tac"].asInt());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_earfcn(
                            requestedCell["lteCellInfo"]["lteCellIdentity"]["earfcn"].asInt());
                        // signal strength
                        cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                            set_lte_signal_strength(requestedCell["lteCellInfo"]\
                            ["lteSignalStrengthInfo"]["lteSignalStrength"].asInt());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                            set_lte_rsrp(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                            ["lteRsrp"].asInt());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                            set_lte_rsrq(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                            ["lteRsrq"].asInt());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                            set_lte_rssnr(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                            ["lteRssnr"].asInt());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                            set_lte_cqi(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                            ["lteCqi"].asInt());
                        cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                            set_timing_advance(requestedCell["lteCellInfo"]\
                            ["lteSignalStrengthInfo"]["timingAdvance"].asInt());
                        break;
                    }
                    case telStub::CellInfo_CellType_WCDMA: {
                        // cell identity
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_mcc(
                            requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["mcc"].asString());
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_mnc(
                            requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["mnc"].asString());
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_lac(
                            requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["lac"].asInt());
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_cid(
                            requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["cid"].asInt());
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_psc(
                            requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["psc"].asInt());
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->\
                            set_uarfcn(requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]\
                            ["uarfcn"].asInt());
                        // signal strength
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                            set_signal_strength(requestedCell["wcdmaCellInfo"]\
                            ["wcdmaSignalStrengthInfo"]["signalStrength"].asInt());
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                            set_bit_error_rate(requestedCell["wcdmaCellInfo"]\
                            ["wcdmaSignalStrengthInfo"]["bitErrorRate"].asInt());
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                            set_ecio(requestedCell["wcdmaCellInfo"]\
                            ["wcdmaSignalStrengthInfo"]["ecio"].asInt());
                        cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                            set_rscp(requestedCell["wcdmaCellInfo"]\
                            ["wcdmaSignalStrengthInfo"]["rscp"].asInt());
                        break;
                    }
                    case telStub::CellInfo_CellType_NR5G: {
                        // cell identity
                        cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_mcc(
                            requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["mcc"].asString());
                        cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_mnc(
                            requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["mnc"].asString());
                        cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_ci(
                            requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["ci"].asInt());
                        cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_pci(
                            requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["pci"].asInt());
                        cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_tac(
                            requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["tac"].asInt());
                        cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_arfcn(
                            requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["arfcn"].asInt());
                        // signal strength
                        cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                            set_rsrp(requestedCell["nr5gCellInfo"]["nr5gSignalStrengthInfo"]\
                            ["rsrp"].asInt());
                        cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                            set_rsrq(requestedCell["nr5gCellInfo"]["nr5gSignalStrengthInfo"]\
                            ["rsrq"].asInt());
                        cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                            set_rssnr(requestedCell["nr5gCellInfo"]["nr5gSignalStrengthInfo"]\
                            ["rssnr"].asInt());
                        break;
                    }
                    case telStub::CellInfo_CellType_NB1_NTN: {
                        // cell identity
                        cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->
                            set_mcc(requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]\
                            ["mcc"].asString());
                        cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->
                            set_mnc(requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]\
                            ["mnc"].asString());
                        cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->
                            set_ci(requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]\
                            ["ci"].asInt());
                        cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->
                            set_tac(requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]\
                            ["tac"].asInt());
                        cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->
                            set_earfcn(requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]\
                            ["earfcn"].asInt());
                        // signal strength
                        cellInfo->mutable_nb1_ntn_cell_info()->
                            mutable_nb1_ntn_signal_strength_info()->
                            set_signal_strength(requestedCell["nb1NtnCellInfo"]\
                            ["nb1NtnSignalStrengthInfo"]["signalStrength"].asInt());
                        cellInfo->mutable_nb1_ntn_cell_info()->
                            mutable_nb1_ntn_signal_strength_info()->
                            set_rsrp(requestedCell["nb1NtnCellInfo"]["nb1NtnSignalStrengthInfo"]\
                            ["rsrp"].asInt());
                        cellInfo->mutable_nb1_ntn_cell_info()->
                            mutable_nb1_ntn_signal_strength_info()->
                            set_rsrq(requestedCell["nb1NtnCellInfo"]["nb1NtnSignalStrengthInfo"]\
                            ["rsrq"].asInt());
                        cellInfo->mutable_nb1_ntn_cell_info()->
                            mutable_nb1_ntn_signal_strength_info()->
                            set_rssnr(requestedCell["nb1NtnCellInfo"]["nb1NtnSignalStrengthInfo"]\
                            ["rssnr"].asInt());
                        break;
                    }
                    case telStub::CellInfo_CellType_CDMA:
                    case telStub::CellInfo_CellType_TDSCDMA:
                    default:
                        LOG(DEBUG, " Deprecated or Invalid type");
                        break;
                } // end switch
            } // end for loop
        } else {
            data.error = telux::common::ErrorCode::SYSTEM_ERR;
        }
    }
    //Update response
    response->set_phone_id(phoneId);
    updateResponse(response, data);
    return data;
}

JsonData TelUtil::readEcallOperatingModeRespFromJsonFile(int phoneId,
    telStub::GetECallOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(phoneId, TEL_PHONE_MANAGER, "requestECallOperatingMode",
        data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return data;
    }
    if (data.status == telux::common::Status::SUCCESS) {
        int ecallMode = data.stateRootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]\
        ["ecallMode"].asInt();
        response->set_ecall_mode(static_cast<telStub::ECallMode>(ecallMode));
    }
    //Update response
    updateResponse(response, data);
    return data;
}

JsonData TelUtil::readResetWwanRespFromJsonFile(telStub::ResetWwanReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telStub::OperatingModeEvent event;
    telux::common::ErrorCode error = TelUtil::readOperatingModeEventFromJsonFile(event);
    JsonData data;
    if (error == ErrorCode::SUCCESS && event.operating_mode() == telStub::OperatingMode::ONLINE) {
        if (ErrorCode::SUCCESS != readJsonData(DEFAULT_SLOT_ID, TEL_PHONE_MANAGER,
            "resetWwan", data)) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return data;
        }
        if (data.status == telux::common::Status::SUCCESS) {
            LOG(DEBUG, __FUNCTION__, " Data Status is Success");
        }
    } else {
        data.error = telux::common::ErrorCode::INVALID_MODEM_STATE;
    }
    //Update response
    updateResponse(response, data);
    return data;
}

JsonData TelUtil::readRequestOperatorInfoRespFromJsonFile(int phoneId,
    telStub::RequestOperatorInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);

    telStub::OperatingModeEvent event;
    telux::common::ErrorCode error = TelUtil::readOperatingModeEventFromJsonFile(event);
    JsonData data;
    if (error == ErrorCode::SUCCESS && event.operating_mode() == telStub::OperatingMode::ONLINE) {
        if (ErrorCode::SUCCESS != readJsonData(phoneId, TEL_PHONE_MANAGER, "requestOperatorInfo",
            data)) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return data;
        }
        if (data.status == telux::common::Status::SUCCESS) {
            std::string operatorLongName = data.stateRootObj[TEL_PHONE_MANAGER]\
                ["operatorNameInfo"]["longName"].asString();
            std::string operatorShortName = data.stateRootObj[TEL_PHONE_MANAGER]\
                ["operatorNameInfo"]["shortName"].asString();
            std::string plmn = data.stateRootObj[TEL_PHONE_MANAGER]\
                ["operatorNameInfo"]["plmn"].asString();
            bool home = data.stateRootObj[TEL_PHONE_MANAGER]\
                ["operatorNameInfo"]["home"].asBool();
            response->mutable_plmn_info()->set_long_name(operatorLongName);
            response->mutable_plmn_info()->set_short_name(operatorShortName);
            response->mutable_plmn_info()->set_plmn(plmn);
            response->mutable_plmn_info()->set_ishome(home);
        }
    } else {
        response->mutable_plmn_info()->set_long_name("");
        response->mutable_plmn_info()->set_short_name("");
        response->mutable_plmn_info()->set_plmn("");
        response->mutable_plmn_info()->set_ishome(false);
    }
    //Update response
    updateResponse(response, data);
    return data;
}

telux::common::ErrorCode TelUtil::readSignalStrengthEventFromJsonFile(int phoneId,
    telStub::SignalStrengthChangeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER,
        stateRootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    telStub::RadioTechnology servingRat;
    telStub::ServiceDomainInfo_Domain servingDomain;
    error = TelUtil::readSystemInfoFromJsonFile(phoneId, servingRat, servingDomain);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading System Info failed" );
        return error;
    }
    // gsm signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_GSM) {
        event.mutable_signal_strength()->mutable_gsm_signal_strength_info()->
            set_gsm_signal_strength(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]
            ["gsmSignalStrengthInfo"]["gsmSignalStrength"].asInt());
        event.mutable_signal_strength()->mutable_gsm_signal_strength_info()->
            set_gsm_bit_error_rate(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["gsmSignalStrengthInfo"]["gsmBitErrorRate"].asInt());
    } else {
        event.mutable_signal_strength()->mutable_gsm_signal_strength_info()->\
            set_gsm_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_gsm_signal_strength_info()->\
            set_gsm_bit_error_rate(INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " gsmSignalStrength:", event.mutable_signal_strength()->\
        mutable_gsm_signal_strength_info()->gsm_signal_strength(),
        " gsmBitErrorRate", event.mutable_signal_strength()->\
        mutable_gsm_signal_strength_info()->gsm_bit_error_rate());

    // lte signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_LTE) {
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->
            set_lte_signal_strength(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["lteSignalStrengthInfo"]["lteSignalStrength"].asInt());
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rsrp(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRsrp"].asInt());
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rsrq(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRsrq"].asInt());
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rssnr(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRssnr"].asInt());
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_cqi(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteCqi"].asInt());
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_timing_advance(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["timingAdvance"].asInt());
    } else {
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->
            set_lte_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rsrp(
            INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rsrq(
            INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_rssnr(
            INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_lte_cqi(
            INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->set_timing_advance(
            INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " lteSignalStrength: ",
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->\
            lte_signal_strength(), " lteRsrp: ",
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->\
            lte_rsrp(), " lteRssnr: ",
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->\
            lte_rssnr(), " lteRsrq: ",
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->\
            lte_rsrq(), " lteCqi: ",
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->\
            lte_cqi(), " timingAdvance: ",
        event.mutable_signal_strength()->mutable_lte_signal_strength_info()->\
            timing_advance());

    // wcdma signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_UMTS) {
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
        set_signal_strength(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
            ["wcdmaSignalStrengthInfo"]["signalStrength"].asInt());
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
            set_bit_error_rate(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["wcdmaSignalStrengthInfo"]["bitErrorRate"].asInt());
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
            set_ecio(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["wcdmaSignalStrengthInfo"]["ecio"].asInt());
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
            set_rscp(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["wcdmaSignalStrengthInfo"]["rscp"].asInt());
    } else {
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
            set_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
            set_bit_error_rate(INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
            set_ecio(INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
            set_rscp(INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " wcdmaSignalStrength: ",
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->\
        signal_strength(), " bitErrorRate: ",
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->\
        bit_error_rate(), " ecio: ",
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->\
        ecio(), " rscp: ",
        event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->\
        rscp());

    // nr5g signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_NR5G) {
        event.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rsrp(
        stateRootObj[TEL_PHONE_MANAGER]\
        ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrp"].asInt());
        event.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rsrq(
            stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrq"].asInt());
        event.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rssnr(
            stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rssnr"].asInt());
    } else {
        event.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rsrp(
            INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rsrq(
            INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->set_rssnr(
            INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " nr5gRsrp: ",
        stateRootObj[TEL_PHONE_MANAGER]\
        ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrp"].asInt(), " nr5gRsrq: ",
        stateRootObj[TEL_PHONE_MANAGER]\
        ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrq"].asInt(), " nr5gRssnr: ",
        stateRootObj[TEL_PHONE_MANAGER]\
        ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rssnr"].asInt());
    // nb1 ntn signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_NB1_NTN) {
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
            set_signal_strength(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["nb1NtnSignalStrengthInfo"]["signalStrength"].asInt());
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->set_rsrp(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
            ["rsrp"].asInt());
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->set_rsrq(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
            ["rsrq"].asInt());
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->set_rssnr(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
            ["rssnr"].asInt());
    } else {
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
            set_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->set_rsrp(
            INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->set_rsrq(
            INVALID_SIGNAL_STRENGTH_VALUE);
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->set_rssnr(
            INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " nb1NtnSignalStrength: ",
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->\
            signal_strength(), " rsrp: ",
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->\
            rsrp(), " rssnr: ",
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->\
            rssnr(), " rsrq: ",
        event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->\
            rsrq());
    event.set_phone_id(phoneId);
    return error;
}

telux::common::ErrorCode TelUtil::readCellInfoListEventFromJsonFile(int phoneId,
    telStub::CellInfoListEvent &cellInfoListEvent) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER,
        stateRootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    telStub::RadioTechnology servingRat;
    telStub::ServiceDomainInfo_Domain servingDomain;
    error = TelUtil::readSystemInfoFromJsonFile(phoneId, servingRat, servingDomain);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading System Info failed" );
        return error;
    }
    telStub::OperatingModeEvent event;
    error = TelUtil::readOperatingModeEventFromJsonFile(event);
    if (error == ErrorCode::SUCCESS && event.operating_mode() == telStub::OperatingMode::ONLINE) {
    // Create response
        int newCellCount = stateRootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"].size();
        LOG(DEBUG, __FUNCTION__, " newCellCount: ", newCellCount);
        for (int i = 0 ; i < newCellCount; i++) {
            telStub::CellInfoList *cellInfo = cellInfoListEvent.add_cell_info_list();
            Json::Value requestedCell =
                stateRootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"][i];
            // whether any cell registered
            int isRegistered = requestedCell["registered"].asInt();
            cellInfo->mutable_cell_type()->set_registered(isRegistered);
            telStub::CellInfo_CellType cellType =
                static_cast<telStub::CellInfo_CellType>(requestedCell["cellType"].asInt());
            cellInfo->mutable_cell_type()->set_cell_type(
                static_cast<telStub::CellInfo_CellType>(cellType));
            switch(cellType) {
                case telStub::CellInfo_CellType_GSM: {
                    // cell identity
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_mcc(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["mcc"].asString());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_mnc(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["mnc"].asString());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_lac(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["lac"].asInt());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_cid(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["cid"].asInt());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_arfcn(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["arfcn"].asInt());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_bsic(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["bsic"].asInt());
                    // signal strength
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_signal_strength_info()->
                        set_gsm_signal_strength(requestedCell["gsmCellInfo"]\
                        ["gsmSignalStrengthInfo"]["gsmSignalStrength"].asInt());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_signal_strength_info()->
                        set_gsm_bit_error_rate(requestedCell["gsmCellInfo"]\
                        ["gsmSignalStrengthInfo"]["gsmBitErrorRate"].asInt());
                    break;
                }
                case telStub::CellInfo_CellType_LTE: {
                    // cell identity
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_mcc(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["mcc"].asString());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_mnc(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["mnc"].asString());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_ci(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["ci"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_pci(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["pci"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_tac(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["tac"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_earfcn(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["earfcn"].asInt());
                    // signal strength
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_signal_strength(requestedCell["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteSignalStrength"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_rsrp(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                        ["lteRsrp"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_rsrq(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                        ["lteRsrq"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_rssnr(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                        ["lteRssnr"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_cqi(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                        ["lteCqi"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_timing_advance(requestedCell["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["timingAdvance"].asInt());
                    break;
                }
                case telStub::CellInfo_CellType_WCDMA: {
                    // cell identity
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_mcc(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["mcc"].asString());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_mnc(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["mnc"].asString());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_lac(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["lac"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_cid(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["cid"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_psc(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["psc"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->\
                        set_uarfcn(requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]\
                        ["uarfcn"].asInt());
                    // signal strength
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_signal_strength(requestedCell["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["signalStrength"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_bit_error_rate(requestedCell["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["bitErrorRate"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_ecio(requestedCell["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["ecio"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_rscp(requestedCell["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["rscp"].asInt());
                    break;
                }
                case telStub::CellInfo_CellType_NR5G: {
                    // cell identity
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_mcc(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["mcc"].asString());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_mnc(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["mnc"].asString());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_ci(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["ci"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_pci(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["pci"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_tac(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["tac"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_arfcn(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["arfcn"].asInt());
                    // signal strength
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                        set_rsrp(requestedCell["nr5gCellInfo"]["nr5gSignalStrengthInfo"]\
                        ["rsrp"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                        set_rsrq(requestedCell["nr5gCellInfo"]["nr5gSignalStrengthInfo"]\
                        ["rsrq"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                        set_rssnr(requestedCell["nr5gCellInfo"]["nr5gSignalStrengthInfo"]\
                        ["rssnr"].asInt());
                    break;
                }
                case telStub::CellInfo_CellType_NB1_NTN: {
                    // cell identity
                    cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->set_mcc(
                        requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]["mcc"].asString());
                    cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->set_mnc(
                        requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]["mnc"].asString());
                    cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->set_ci(
                        requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]["ci"].asInt());
                    cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->set_tac(
                        requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]["tac"].asInt());
                    cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_cell_identity()->
                        set_earfcn(
                        requestedCell["nb1NtnCellInfo"]["nb1NtnCellIdentity"]["earfcn"].asInt());
                    // signal strength
                    cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_signal_strength_info()->
                        set_signal_strength(requestedCell["nb1NtnCellInfo"]\
                        ["nb1NtnSignalStrengthInfo"]["signalStrength"].asInt());
                    cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_signal_strength_info()->
                        set_rsrp(requestedCell["nb1NtnCellInfo"]["nb1NtnSignalStrengthInfo"]\
                        ["rsrp"].asInt());
                    cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_signal_strength_info()->
                        set_rsrq(requestedCell["nb1NtnCellInfo"]["nb1NtnSignalStrengthInfo"]\
                        ["rsrq"].asInt());
                    cellInfo->mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_signal_strength_info()->
                        set_rssnr(requestedCell["nb1NtnCellInfo"]["nb1NtnSignalStrengthInfo"]\
                        ["rssnr"].asInt());
                    break;
                }
                case telStub::CellInfo_CellType_CDMA:
                case telStub::CellInfo_CellType_TDSCDMA:
                default:
                    LOG(DEBUG, " Deprecated or Invalid type");
                    break;
            } // end switch
        } // end for loop
    } else {
        return telux::common::ErrorCode::SYSTEM_ERR;
    }
    cellInfoListEvent.set_phone_id(phoneId);
    return error;
}

telux::common::ErrorCode TelUtil::readVoiceServiceStateEventFromJsonFile(int phoneId,
    telStub::VoiceServiceStateEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    telStub::VoiceServiceState voiceServiceState = static_cast<telStub::VoiceServiceState>(
        stateRootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]\
        ["voiceServiceState"].asInt());
    LOG(DEBUG, __FUNCTION__," VoiceServiceState is :", static_cast<int>(voiceServiceState));
    event.mutable_voice_service_state_info()->set_voice_service_state(voiceServiceState);
    telStub::VoiceServiceDenialCause voiceServiceDenialCause =
        static_cast<telStub::VoiceServiceDenialCause>(stateRootObj[TEL_PHONE_MANAGER]\
        ["voiceServiceStateInfo"]["voiceServiceDenialCause"].asInt());
    LOG(DEBUG, __FUNCTION__," VoiceServiceDenialCause is :",
            static_cast<int>(voiceServiceDenialCause));
    event.mutable_voice_service_state_info()->set_voice_service_denial_cause(
        voiceServiceDenialCause);
    telStub::RadioTechnology radioTech = static_cast<telStub::RadioTechnology>(
        stateRootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]["radioTech"].asInt());
    LOG(DEBUG, __FUNCTION__," RadioTech is :", static_cast<int>(radioTech));
    event.mutable_voice_service_state_info()->set_radio_technology(radioTech);
    event.set_phone_id(phoneId);
    return error;
}

telux::common::ErrorCode TelUtil::readOperatingModeEventFromJsonFile(
    telStub::OperatingModeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(DEFAULT_SLOT_ID, TEL_PHONE_MANAGER,
        stateRootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    int opMode = stateRootObj[TEL_PHONE_MANAGER]["operatingModeInfo"]\
        ["operatingMode"].asInt();
    telStub::OperatingMode operatingMode = static_cast<telStub::OperatingMode>(opMode);
    event.set_operating_mode(operatingMode);
    LOG(DEBUG, __FUNCTION__," Operating Mode is :", static_cast<int>(operatingMode));
    return error;
}

telux::common::ErrorCode TelUtil::readServiceStateEventFromJsonFile(int phoneId,
    telStub::ServiceStateChangeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    int servState = stateRootObj[TEL_PHONE_MANAGER]["serviceStateInfo"]["serviceState"].asInt();
    telStub::ServiceState serviceState = static_cast<telStub::ServiceState>(servState);
    event.set_service_state(serviceState);
    LOG(DEBUG, __FUNCTION__," Service State is :", static_cast<int>(serviceState));
    return error;
}

telux::common::ErrorCode TelUtil::readVoiceRadioTechnologyEventFromJsonFile(int phoneId,
    telStub::VoiceRadioTechnologyChangeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }

    telStub::RadioTechnology servingRat;
    telStub::ServiceDomainInfo_Domain servingDomain;
    error = TelUtil::readSystemInfoFromJsonFile(phoneId, servingRat, servingDomain);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading System Info failed" );
        return error;
    }

    event.set_radio_technology(servingRat);
    LOG(DEBUG, __FUNCTION__," RAT is :", static_cast<int>(servingRat));
    return error;
}

telux::common::ErrorCode TelUtil::readEcallOperatingModeEventFromJsonFile(int phoneId,
    telStub::ECallModeInfoChangeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    int ecallMode = stateRootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]["ecallMode"].asInt();
    int ecallModeReason =
        stateRootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]["ecallModeReason"].asInt();
    event.set_phone_id(phoneId);
    event.set_ecall_mode(static_cast<telStub::ECallMode>(ecallMode));
    event.set_ecall_mode_reason(static_cast<telStub::ECallModeReason::Reason>(ecallModeReason));
    LOG(DEBUG, __FUNCTION__," ecallMode:", ecallMode, " ecallModeReason:", ecallModeReason);
    return error;
}

telux::common::ErrorCode TelUtil::readOperatorInfoEventFromJsonFile(int phoneId,
    telStub::OperatorInfoEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    std::string longName = stateRootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]\
        ["longName"].asString();
    std::string shortName = stateRootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]\
        ["shortName"].asString();
    std::string plmn = stateRootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["plmn"].asString();
    bool isHome = stateRootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["home"].asBool();

    event.set_phone_id(phoneId);
    event.mutable_plmn_info()->set_long_name(longName);
    event.mutable_plmn_info()->set_short_name(shortName);
    event.mutable_plmn_info()->set_plmn(plmn);
    event.mutable_plmn_info()->set_ishome(isHome);
    LOG(DEBUG, __FUNCTION__," longName:", longName, " shortName:", shortName, " plmn:", plmn,
        " isHome:", static_cast<int>(isHome));
    return error;
}

telux::common::ErrorCode TelUtil::readSignalStrengthFromJsonFile(int phoneId,
    telStub::SignalStrength &signalStrength) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER,
        stateRootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }

    telStub::RadioTechnology servingRat;
    telStub::ServiceDomainInfo_Domain servingDomain;
    error = TelUtil::readSystemInfoFromJsonFile(phoneId, servingRat, servingDomain);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading System Info failed" );
        return error;
    }

    // gsm signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_GSM) {
        signalStrength.mutable_gsm_signal_strength_info()->
        set_gsm_signal_strength(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]
            ["gsmSignalStrengthInfo"]["gsmSignalStrength"].asInt());
        signalStrength.mutable_gsm_signal_strength_info()->
            set_gsm_bit_error_rate(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["gsmSignalStrengthInfo"]["gsmBitErrorRate"].asInt());
    } else {
        signalStrength.mutable_gsm_signal_strength_info()->set_gsm_signal_strength(
            INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_gsm_signal_strength_info()->set_gsm_bit_error_rate(
            INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " gsmSignalStrength: ",
        signalStrength.mutable_gsm_signal_strength_info()->gsm_signal_strength(),
        " gsmBitErrorRate: ",
        signalStrength.mutable_gsm_signal_strength_info()->gsm_bit_error_rate());

    // lte signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_LTE) {
        signalStrength.mutable_lte_signal_strength_info()->
            set_lte_signal_strength(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["lteSignalStrengthInfo"]["lteSignalStrength"].asInt());
        signalStrength.mutable_lte_signal_strength_info()->set_lte_rsrp(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRsrp"].asInt());
        signalStrength.mutable_lte_signal_strength_info()->set_lte_rsrq(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRsrq"].asInt());
        signalStrength.mutable_lte_signal_strength_info()->set_lte_rssnr(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRssnr"].asInt());
        signalStrength.mutable_lte_signal_strength_info()->set_lte_cqi(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteCqi"].asInt());
        signalStrength.mutable_lte_signal_strength_info()->set_timing_advance(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["timingAdvance"].asInt());
    } else {
        signalStrength.mutable_lte_signal_strength_info()->
            set_lte_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_lte_signal_strength_info()->set_lte_rsrp(
            INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_lte_signal_strength_info()->set_lte_rsrq(
            INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_lte_signal_strength_info()->set_lte_rssnr(
            INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_lte_signal_strength_info()->set_lte_cqi(
            INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_lte_signal_strength_info()->set_timing_advance(
            INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " lteSignalStrength: ",
        signalStrength.mutable_lte_signal_strength_info()->lte_signal_strength(), " lteRsrp: ",
        signalStrength.mutable_lte_signal_strength_info()->lte_rsrp(), " lteRsrq: ",
        signalStrength.mutable_lte_signal_strength_info()->lte_rsrq(), " lteRssnr: ",
        signalStrength.mutable_lte_signal_strength_info()->lte_rssnr(), " lteCqi: ",
        signalStrength.mutable_lte_signal_strength_info()->lte_cqi(), " timingAdvance: ",
        signalStrength.mutable_lte_signal_strength_info()->timing_advance());

    // wcdma signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_UMTS) {
        signalStrength.mutable_wcdma_signal_strength_info()->
        set_signal_strength(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
            ["wcdmaSignalStrengthInfo"]["signalStrength"].asInt());
        signalStrength.mutable_wcdma_signal_strength_info()->
            set_bit_error_rate(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["wcdmaSignalStrengthInfo"]["bitErrorRate"].asInt());
        signalStrength.mutable_wcdma_signal_strength_info()->
            set_ecio(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["wcdmaSignalStrengthInfo"]["ecio"].asInt());
        signalStrength.mutable_wcdma_signal_strength_info()->
            set_rscp(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["wcdmaSignalStrengthInfo"]["rscp"].asInt());
    } else {
        signalStrength.mutable_wcdma_signal_strength_info()->
            set_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_wcdma_signal_strength_info()->
            set_bit_error_rate(INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_wcdma_signal_strength_info()->
            set_ecio(INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_wcdma_signal_strength_info()->
            set_rscp(INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " wcdmaSignalStrength: ",
        signalStrength.mutable_wcdma_signal_strength_info()->signal_strength(),
        " bitErrorRate: ",
        signalStrength.mutable_wcdma_signal_strength_info()->bit_error_rate(),
        " ecio: ",
        signalStrength.mutable_wcdma_signal_strength_info()->ecio(),
        " rscp: ",
        signalStrength.mutable_wcdma_signal_strength_info()->rscp());

    // nr5g signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_NR5G) {
        signalStrength.mutable_nr5g_signal_strength_info()->set_rsrp(
        stateRootObj[TEL_PHONE_MANAGER]\
        ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrp"].asInt());
        signalStrength.mutable_nr5g_signal_strength_info()->set_rsrq(
            stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrq"].asInt());
        signalStrength.mutable_nr5g_signal_strength_info()->set_rssnr(
            stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rssnr"].asInt());
    } else {
        signalStrength.mutable_nr5g_signal_strength_info()->set_rsrp(INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_nr5g_signal_strength_info()->set_rsrq(INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_nr5g_signal_strength_info()->set_rssnr(
            INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " nr5gRsrp: ",
        signalStrength.mutable_nr5g_signal_strength_info()->rsrp(), " nr5gRsrq: ",
        signalStrength.mutable_nr5g_signal_strength_info()->rsrq(), " nr5gRssnr: ",
        signalStrength.mutable_nr5g_signal_strength_info()->rssnr());
    // NB1 NTN signal strength
    if (servingRat == telStub::RadioTechnology::RADIO_TECH_NB1_NTN) {
        signalStrength.mutable_nb1_ntn_signal_strength_info()->
            set_signal_strength(stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                ["nb1NtnSignalStrengthInfo"]["signalStrength"].asInt());
        signalStrength.mutable_nb1_ntn_signal_strength_info()->set_rsrp(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
            ["rsrp"].asInt());
        signalStrength.mutable_nb1_ntn_signal_strength_info()->set_rsrq(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
            ["rsrq"].asInt());
        signalStrength.mutable_nb1_ntn_signal_strength_info()->set_rssnr(
            stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
            ["rssnr"].asInt());
    } else {
        signalStrength.mutable_nb1_ntn_signal_strength_info()->
            set_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_nb1_ntn_signal_strength_info()->set_rsrp(
            INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_nb1_ntn_signal_strength_info()->set_rsrq(
            INVALID_SIGNAL_STRENGTH_VALUE);
        signalStrength.mutable_nb1_ntn_signal_strength_info()->set_rssnr(
            INVALID_SIGNAL_STRENGTH_VALUE);
    }
    LOG(DEBUG, __FUNCTION__, " nb1NtnSignalStrength: ",
        signalStrength.mutable_nb1_ntn_signal_strength_info()->signal_strength(), " nb1NtnRsrp: ",
        signalStrength.mutable_nb1_ntn_signal_strength_info()->rsrp(), " nb1NtnRsrq: ",
        signalStrength.mutable_nb1_ntn_signal_strength_info()->rsrq(), " nb1NtnRssnr: ",
        signalStrength.mutable_nb1_ntn_signal_strength_info()->rssnr());
    return error;
}

telux::common::ErrorCode TelUtil::readServiceStateFromJsonFile(int phoneId,
    telStub::ServiceState &state) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    int servState = stateRootObj[TEL_PHONE_MANAGER]["serviceStateInfo"]["serviceState"].asInt();
    state = static_cast<telStub::ServiceState>(servState);
    LOG(DEBUG, __FUNCTION__," Service State is :", static_cast<int>(state));
    return error;
}

telux::common::ErrorCode TelUtil::readVoiceRadioTechnologyFromJsonFile(int phoneId,
    telStub::RadioTechnology &rat) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    telStub::ServiceDomainInfo_Domain servingDomain;
    error = TelUtil::readSystemInfoFromJsonFile(phoneId, rat, servingDomain);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading System Info failed" );
        return error;
    }
    LOG(DEBUG, __FUNCTION__," RAT is :", static_cast<int>(rat));
    return error;
}

telux::common::ErrorCode TelUtil::readSystemInfoFromJsonFile(int phoneId,
    telStub::RadioTechnology &servingRat, telStub::ServiceDomainInfo_Domain &servingDomain) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_SERVING_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    int rat = stateRootObj[TEL_SERVING_MANAGER]["ServingSystemInfo"]["rat"].asInt();
    servingRat = static_cast<telStub::RadioTechnology>(rat);
    int domain = stateRootObj[TEL_SERVING_MANAGER]["ServingSystemInfo"]["domain"].asInt();
    servingDomain = static_cast<telStub::ServiceDomainInfo_Domain>(domain);
    LOG(DEBUG, __FUNCTION__," RAT: ", rat, " Domain: ", domain);
    return error;
}

telux::common::ErrorCode TelUtil::readOperatingModeFromJsonFile(telStub::OperatingMode &mode) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(DEFAULT_SLOT_ID, TEL_SERVING_MANAGER,
        stateRootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    int operatingMode = stateRootObj[TEL_PHONE_MANAGER]["operatingModeInfo"]\
        ["operatingMode"].asInt();
    mode = static_cast<telStub::OperatingMode>(operatingMode);
    LOG(DEBUG, __FUNCTION__," OperatingMode: ", operatingMode);
    return error;
}

telux::common::ErrorCode TelUtil::readRatPreferenceFromJsonFile(int phoneId,
    std::vector<int> &ratData) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_SERVING_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    std::string ratPref = stateRootObj[TEL_SERVING_MANAGER]["RATPreference"].asString();
    ratData = CommonUtils::convertStringToVector(ratPref);
    LOG(DEBUG, __FUNCTION__," RAT preference: ", ratPref);
    return error;
}

telux::common::ErrorCode TelUtil::writeSignalStrengthToJsonFile(int phoneId,
    telStub::SignalStrengthChangeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, rootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]["gsmSignalStrength"] =
        event.signal_strength().gsm_signal_strength_info().gsm_signal_strength();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]["gsmBitErrorRate"] =
        event.signal_strength().gsm_signal_strength_info().gsm_bit_error_rate();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]["signalStrength"] =
        event.signal_strength().wcdma_signal_strength_info().signal_strength();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]["bitErrorRate"] =
        event.signal_strength().wcdma_signal_strength_info().bit_error_rate();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]["ecio"] =
        event.signal_strength().wcdma_signal_strength_info().ecio();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]["rscp"] =
        event.signal_strength().wcdma_signal_strength_info().rscp();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]["lteSignalStrength"] =
        event.signal_strength().lte_signal_strength_info().lte_signal_strength();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]["lteRsrp"] =
        event.signal_strength().lte_signal_strength_info().lte_rsrp();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]["lteRsrq"] =
        event.signal_strength().lte_signal_strength_info().lte_rsrp();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]["lteRssnr"] =
        event.signal_strength().lte_signal_strength_info().lte_rssnr();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]["lteCqi"] =
        event.signal_strength().lte_signal_strength_info().lte_cqi();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]["timingAdvance"] =
        event.signal_strength().lte_signal_strength_info().timing_advance();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrp"] =
        event.signal_strength().nr5g_signal_strength_info().rsrp();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrq"] =
        event.signal_strength().nr5g_signal_strength_info().rsrq();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rssnr"] =
        event.signal_strength().nr5g_signal_strength_info().rssnr();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]["signalStrength"] =
        event.signal_strength().nb1_ntn_signal_strength_info().signal_strength();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]["rsrp"] =
        event.signal_strength().nb1_ntn_signal_strength_info().rsrp();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]["rsrq"] =
        event.signal_strength().nb1_ntn_signal_strength_info().rsrq();
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]["rssnr"] =
        event.signal_strength().nb1_ntn_signal_strength_info().rssnr();
    return JsonParser::writeToJsonFile(rootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::writeVoiceServiceStateToJsonFile(int phoneId,
    telStub::VoiceServiceStateEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    stateRootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]\
        ["voiceServiceState"] = event.voice_service_state_info().voice_service_state();
    stateRootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]\
        ["voiceServiceDenialCause"] = event.voice_service_state_info().voice_service_denial_cause();
    stateRootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]["radioTech"] =
        event.voice_service_state_info().radio_technology();

    LOG(DEBUG, __FUNCTION__, "Writing VoiceServiceState:",
        event.voice_service_state_info().voice_service_state(),
        " VoiceServiceDenialCause:", event.voice_service_state_info().voice_service_denial_cause(),
        " RadioTech:", event.voice_service_state_info().radio_technology());
    return JsonParser::writeToJsonFile(stateRootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::writeOperatingModeToJsonFile(telStub::OperatingModeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(DEFAULT_SLOT_ID, TEL_PHONE_MANAGER,
        stateRootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    stateRootObj[TEL_PHONE_MANAGER]["operatingModeInfo"]["operatingMode"] =
        event.operating_mode();
    LOG(DEBUG, __FUNCTION__," Operating Mode:", static_cast<int>(event.operating_mode()));
    return JsonParser::writeToJsonFile(stateRootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::writeServiceStateToJsonFile(int phoneId,
    telStub::ServiceStateChangeEvent &event) {
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    stateRootObj[TEL_PHONE_MANAGER]["serviceStateInfo"]["serviceState"] =
        event.service_state();
    LOG(DEBUG, __FUNCTION__," Service State:", static_cast<int>(event.service_state()));
    return JsonParser::writeToJsonFile(stateRootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::writeVoiceRadioTechnologyToJsonFile(int phoneId,
    telStub::VoiceRadioTechnologyChangeEvent &event) {
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_PHONE_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    stateRootObj[TEL_PHONE_MANAGER]["voiceRatInfo"]["rat"] =
        event.radio_technology();
    LOG(DEBUG, __FUNCTION__," RAT:", static_cast<int>(event.radio_technology()));
    return JsonParser::writeToJsonFile(stateRootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::writeSystemInfoToJsonFile(int phoneId,
        telStub::RadioTechnology &servingRat, telStub::ServiceDomainInfo_Domain &servingDomain) {
    Json::Value stateRootObj;
    std::string jsonfilename;
    telux::common::ErrorCode error = readFromJsonFile(phoneId, TEL_SERVING_MANAGER, stateRootObj,
        jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return error;
    }
    stateRootObj[TEL_SERVING_MANAGER]["ServingSystemInfo"]["rat"] = servingRat;
    stateRootObj[TEL_SERVING_MANAGER]["ServingSystemInfo"]["domain"] = servingDomain;
    LOG(DEBUG, __FUNCTION__," RAT:", static_cast<int>(servingRat), " Domain: ",
        static_cast<int>(servingDomain));
    return JsonParser::writeToJsonFile(stateRootObj, jsonfilename);
}

JsonData TelUtil::writeOperatingModeToJsonFileAndReply(telStub::OperatingMode mode,
    telStub::SetOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    std::string stateJsonPath = "";
    telux::common::ErrorCode error = readJsonData(DEFAULT_SLOT_ID, TEL_PHONE_MANAGER,
        "setOperatingMode", data, stateJsonPath);
    do {
        if (error == ErrorCode::SUCCESS) {
            if (data.status == telux::common::Status::SUCCESS) {
                int opMode = static_cast<int>(mode);
                if (opMode < (static_cast<int>(telStub::OperatingMode::ONLINE)) ||
                    opMode > (static_cast<int>(telStub::OperatingMode::PERSISTENT_LOW_POWER))) {
                    LOG(ERROR, __FUNCTION__, " Invalid operating mode");
                    data.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
                    break;
                }
                data.stateRootObj[TEL_PHONE_MANAGER]["operatingModeInfo"]["operatingMode"] = mode;
                LOG(DEBUG, __FUNCTION__, " OperatingMode:", mode);
                data.error = JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
                break;
            }
        }
    } while(0);

    if (data.error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Unable to write operating mode setting to JSON");
    }
    updateResponse(response, data);
    return data;
}

telux::common::ErrorCode TelUtil::writeSetCellInfoListRateToJsonFileAndReply(int cellInfoListRate,
        telStub::SetCellInfoListRateReply *response) {

    JsonData data;
    std::string stateJsonPath = "";
    telux::common::ErrorCode error = readJsonData(DEFAULT_SLOT_ID, TEL_PHONE_MANAGER,
        "setCellInfoListRate", data, stateJsonPath);

    if (error == ErrorCode::SUCCESS) {
        if (data.status == telux::common::Status::SUCCESS) {
            LOG(DEBUG, __FUNCTION__, " cellInfoListRate:", cellInfoListRate);
            data.stateRootObj[TEL_PHONE_MANAGER]["cellInfoListRate"]["timeInterval"] =
                cellInfoListRate;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
            data.error = JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read cell info list rate from JSON");
        return error;
    }

    if (data.error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Unable to write cell info list rate to JSON");
    }
    updateResponse(response, data);
    return data.error;
}

telux::common::ErrorCode TelUtil::writeSetRadioPowerToJsonFileAndReply(int phoneId, int enable,
    telStub::SetRadioPowerReply *response) {
    JsonData data;
    std::string stateJsonPath = "";
    telux::common::ErrorCode error = readJsonData(phoneId, TEL_PHONE_MANAGER, "setRadioPower", data,
        stateJsonPath);
    if (error == ErrorCode::SUCCESS) {
        if (data.status == telux::common::Status::SUCCESS) {
            LOG(DEBUG, __FUNCTION__, " enable:", enable);
            data.stateRootObj[TEL_PHONE_MANAGER]["radioPowerState"]["enable"] = enable;
            data.error = JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read radio power from JSON");
        return error;
    }

    if (data.error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Unable to write radio power value to JSON");
    }
    // TODO resetting voice service state, signal strength values if radio is OFF
    //Update response
    updateResponse(response, data);
    return data.error;
}

telux::common::ErrorCode TelUtil::writeEcallOperatingModeToJsonFileAndReply(int phoneId,
    telStub::ECallMode ecallMode, telStub::ECallModeReason_Reason reason,
    telStub::SetECallOperatingModeReply *response) {
    JsonData data;
    std::string stateJsonPath = "";
    telStub::OperatingModeEvent event;
    telux::common::ErrorCode error = TelUtil::readOperatingModeEventFromJsonFile(event);
    if((error == ErrorCode::SUCCESS) &&
        (event.operating_mode() == telStub::OperatingMode::ONLINE)) {
        error = readJsonData(phoneId, TEL_PHONE_MANAGER,
            "setECallOperatingMode", data, stateJsonPath);
        if (error == ErrorCode::SUCCESS) {
            if (data.status == telux::common::Status::SUCCESS) {
                LOG(DEBUG, __FUNCTION__, " eCallMode:", static_cast<int>(ecallMode));
                if (ecallMode < (static_cast<int>(telStub::ECallMode::NORMAL)) ||
                    ecallMode > (static_cast<int>(telStub::ECallMode::ECALL_ONLY))) {
                    LOG(ERROR, __FUNCTION__, " Invalid eCall operating mode");
                    response->set_error(static_cast<commonStub::ErrorCode>(
                        commonStub::ErrorCode::NOT_SUPPORTED));
                } else {
                    data.stateRootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]["ecallMode"] =
                        static_cast<int>(ecallMode);
                    data.stateRootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]["ecallModeReason"] =
                        static_cast<int>(reason);
                    data.error = JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
                    LOG(DEBUG, __FUNCTION__, " ecallMode: ", ecallMode, " ecallModeReason: ",
                        reason);
                    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
                }
            }
        } else {
            LOG(ERROR, __FUNCTION__, " Unable to read eCall operating mode from JSON");
            return error;
        }
        if (data.error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Unable to write eCall operating mode to JSON");
        }
    } else {
        data.error = telux::common::ErrorCode::DEVICE_IN_USE;
    }
    //Update response
    updateResponse(response, data);
    return data.error;
}

telux::common::ErrorCode TelUtil::writeConfigureSignalStrengthToJsonFileAndReply(int phoneId,
    std::vector<telStub::ConfigureSignalStrength> signalStrengthConfig,
    telStub::ConfigureSignalStrengthReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    std::string stateJsonPath = "";

    telux::common::ErrorCode error = readJsonData(phoneId, TEL_PHONE_MANAGER,
        "configureSignalStrength", data, stateJsonPath);
    if (error == ErrorCode::SUCCESS) {
        if (data.status == telux::common::Status::SUCCESS) {
            int currentCount = 0;
            for (unsigned int i = 0; i < signalStrengthConfig.size(); i++) {
                Json::Value newconfig;
                currentCount = data.stateRootObj[TEL_PHONE_MANAGER]\
                    ["configureSignalStrengthInfo"].size();
                LOG(DEBUG, __FUNCTION__," current configcount is : ", currentCount);
                newconfig["radioSignalType"] = signalStrengthConfig[i].rat_sig_type();
                newconfig["configType"] = signalStrengthConfig[i].config_type();
                switch(signalStrengthConfig[i].config_type()) {
                     case telStub::SignalStrengthConfigType::DELTA:
                         newconfig["delta"] = signalStrengthConfig[i].delta();
                         break;
                     case telStub::SignalStrengthConfigType::THRESHOLD:
                         newconfig["lowerThreshold"] = signalStrengthConfig[i].mutable_threshold()->
                             lower_range_threshold();
                         newconfig["upperThreshold"] = signalStrengthConfig[i].mutable_threshold()->
                             upper_range_threshold();
                         break;
                    default:
                        break;
                }

                bool ratFound = false;
                for (int j = 0; j < currentCount; j++) {
                    // Check the RAT stored and user requested RAT and update it.
                    if ((data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthInfo"][j]\
                        ["radioSignalType"]) == newconfig["radioSignalType"]) {
                            LOG(DEBUG, __FUNCTION__, " Matched RAT");
                            data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthInfo"]\
                                [j] = newconfig;
                        ratFound = true;
                        break;
                    }
                }

                if (ratFound) {
                    LOG(DEBUG, __FUNCTION__, " Matching RAT found");
                    continue;
                }
                data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthInfo"]\
                    [currentCount++] = newconfig;
            }
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }

    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read from JSON");
        return error;
    }

    if (data.error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Unable to configure signal strength to JSON");
    }
    //Update response
    updateResponse(response, data);
    return data.error;
}

telux::common::ErrorCode TelUtil::writeConfigureSignalStrengthExToJsonFileAndReply(int phoneId,
    std::vector<telStub::ConfigureSignalStrengthEx> signalStrengthConfigEx,
    telStub::ConfigureSignalStrengthExReply* response, uint16_t hysTimer) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    std::string stateJsonPath = "";

    telux::common::ErrorCode error = readJsonData(phoneId, TEL_PHONE_MANAGER,
        "configureSignalStrength", data, stateJsonPath);
    if (error == ErrorCode::SUCCESS) {
        if (data.status == telux::common::Status::SUCCESS) {
            int currentCount = 0;
            LOG(DEBUG, __FUNCTION__, " signal strength config size = ",
                signalStrengthConfigEx.size());
            for (unsigned int idx = 0; idx < signalStrengthConfigEx.size(); idx++) {
                Json::Value newconfig;
                currentCount = data.stateRootObj[TEL_PHONE_MANAGER]\
                    ["configureSignalStrengthExInfo"]["configureSignalStrengthExInfoList"].size();
                LOG(DEBUG, __FUNCTION__," current config count is : ", currentCount);

                newconfig["radioTech"] = signalStrengthConfigEx[idx].radio_tech();
                LOG(DEBUG, __FUNCTION__, " signal strength config type size = ",
                    signalStrengthConfigEx[idx].config_types().size());
                for (int cfIdx = 0; cfIdx < signalStrengthConfigEx[idx].config_types().size();
                    cfIdx++) {
                    if (signalStrengthConfigEx[idx].config_types(cfIdx)) {
                        newconfig["configExType"][cfIdx] =
                            static_cast<int>(signalStrengthConfigEx[idx].config_types(cfIdx));
                    }
                }
                LOG(DEBUG, __FUNCTION__, " signal strength config data size = ",
                    signalStrengthConfigEx[idx].sig_config_data().size());
                for (int cdIdx = 0;
                    cdIdx < signalStrengthConfigEx[idx].sig_config_data().size(); cdIdx++) {
                    newconfig["sigMeasType"] =
                        signalStrengthConfigEx[idx].mutable_sig_config_data(cdIdx)->sig_meas_type();
                    for (int cfIdx = 0;
                        cfIdx < signalStrengthConfigEx[idx].config_types().size(); cfIdx++) {
                        if (signalStrengthConfigEx[idx].config_types(cfIdx) ==
                            telStub::SignalStrengthConfigExType::EX_DELTA) {
                            newconfig["delta"] =
                                signalStrengthConfigEx[idx].mutable_sig_config_data(cdIdx)->delta();
                        }
                        if (signalStrengthConfigEx[idx].config_types(cfIdx) ==
                            telStub::SignalStrengthConfigExType::EX_THRESHOLD) {
                            for (int arrIdx = 0;
                                arrIdx < signalStrengthConfigEx[idx].mutable_sig_config_data(cdIdx)
                                ->mutable_elements()->threshold_list().size(); arrIdx++) {
                                if (signalStrengthConfigEx[idx].mutable_sig_config_data(cdIdx)
                                    ->mutable_elements()->threshold_list(arrIdx)) {
                                    newconfig["thresholdList"][arrIdx] =
                                        signalStrengthConfigEx[idx].mutable_sig_config_data(cdIdx)
                                        ->mutable_elements()->threshold_list(arrIdx);
                                }
                            }
                        }
                        if (signalStrengthConfigEx[idx].config_types(cfIdx) ==
                            telStub::SignalStrengthConfigExType::EX_HYSTERESIS_DB) {
                            newconfig["hysteresisDb"] =
                                signalStrengthConfigEx[idx].mutable_sig_config_data(cdIdx)
                                ->mutable_elements()->hysteresis_db();
                        }
                    }

                    bool ratFound = false;
                    for (int j = 0; j < currentCount; j++) {
                        // check the RAT + signal measurement type stored and user requested RAT +
                        // signal measurement type and update it, otherwise add a new entry.
                        if ((data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthExInfo"]\
                            ["configureSignalStrengthExInfoList"][j]\
                                ["radioTech"]) == newconfig["radioTech"] &&
                            (data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthExInfo"]\
                                ["configureSignalStrengthExInfoList"][j]["sigMeasType"])
                                    == newconfig["sigMeasType"]) {
                            LOG(DEBUG, __FUNCTION__, " Matched RAT");
                            data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthExInfo"]\
                                ["configureSignalStrengthExInfoList"][j] = newconfig;
                            ratFound = true;
                            break;
                        }
                    }

                    if (ratFound) {
                        LOG(DEBUG, __FUNCTION__, " Matching RAT found");
                        continue;
                    }
                    data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthExInfo"]\
                        ["configureSignalStrengthExInfoList"][currentCount++] = newconfig;
                 }
             }
             data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthExInfo"]\
                 ["hysteresisMs"] = hysTimer;
             JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read from JSON");
        return error;
    }

    if (data.error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Unable to configure signal strength to JSON");
    }
    // Update response
    updateResponse(response, data);
    return data.error;
}

telux::common::ErrorCode TelUtil::writeSignalStrengthToJsonFile(std::vector<std::string> params,
    int &phoneId, bool &notify) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::INTERNAL_ERR;
    try {
        // Read string to get slotId
        std::string token = telux::common::EventParserUtil::getNextToken(params[0],
            DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return errorCode;
        }
        Json::Value rootObj;
        std::string jsonfilename = (phoneId == SLOT_1)? PHONE_JSON_STATE_PATH1 :
            PHONE_JSON_STATE_PATH2;
        errorCode = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (errorCode != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return errorCode;
        }
        // retrieve serving rat
        std::string stateJsonPath = (phoneId == SLOT_1)?
            "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";
        int servingRat = std::stoi(CommonUtils::readSystemDataValue(stateJsonPath, "",
            {TEL_SERVING_MANAGER, "ServingSystemInfo", "rat"}));
        LOG(DEBUG, __FUNCTION__, " Serving RAT is: ", servingRat);

        int signalStrengthInfoCount = params.size() - 1;
        LOG(DEBUG, __FUNCTION__, " signalStrengthInfoCount : ", signalStrengthInfoCount);
        for (int index = 1; index <= signalStrengthInfoCount; index++) {
            std::string rat = telux::common::EventParserUtil::getNextToken(params[index],
                DEFAULT_DELIMITER);
            LOG(DEBUG, __FUNCTION__, " RAT Type is: ", rat);
            if (rat == "GSM") {
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int signalStrength = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int bitErrorRate = std::stoi(token);
                LOG(DEBUG, __FUNCTION__," signalStrength: ", signalStrength," bitErrorRate: ",
                    bitErrorRate);

                if (servingRat == static_cast<int>(telStub::RadioTechnology::RADIO_TECH_GSM)) {
                    int oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                        ["gsmSignalStrengthInfo"]["gsmSignalStrength"].asInt();
                    notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                        telStub::RadioTechnology::RADIO_TECH_GSM,
                        telStub::SignalStrengthMeasurementType::RSSI, oldValue, signalStrength);
                }
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]\
                    ["gsmSignalStrength"] = signalStrength;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]\
                    ["gsmBitErrorRate"] = bitErrorRate;
            } else if(rat == "WCDMA") {
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int signalStrength = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int bitErrorRate = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int ecio = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rscp = std::stoi(token);
                LOG(DEBUG, __FUNCTION__, " signalStrength: ", signalStrength, " bitErrorRate: ",
                    bitErrorRate, " ecio: ", ecio, " rscp: ", rscp);

                if (servingRat == static_cast<int>(telStub::RadioTechnology::RADIO_TECH_UMTS)) {
                    int oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                        ["wcdmaSignalStrengthInfo"]["signalStrength"].asInt();
                    notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                        telStub::RadioTechnology::RADIO_TECH_UMTS,
                        telStub::SignalStrengthMeasurementType::RSSI, oldValue, signalStrength);

                    if (!notify) { // if any one field changes, need to notify,
                                  // skip next field check
                        oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                            ["wcdmaSignalStrengthInfo"]["ecio"].asInt();
                        notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                            telStub::RadioTechnology::RADIO_TECH_UMTS,
                            telStub::SignalStrengthMeasurementType::ECIO, oldValue, ecio);
                    }
                    if (!notify) { // if any one field changes,need to notify,
                                   // skip next field check
                        oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                            ["wcdmaSignalStrengthInfo"]["rscp"].asInt();
                        notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                            telStub::RadioTechnology::RADIO_TECH_UMTS,
                            telStub::SignalStrengthMeasurementType::RSCP, oldValue, rscp);
                    }
                }
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
                    ["signalStrength"] = signalStrength;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
                    ["bitErrorRate"] = bitErrorRate;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
                    ["ecio"] = ecio;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
                    ["rscp"] = rscp;
            } else if(rat == "LTE") {
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int signalStrength = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rsrp = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rsrq = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rssnr = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int cqi = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int timingAdvance = std::stoi(token);
                LOG(DEBUG, __FUNCTION__," signalStrength: ", signalStrength," rsrp: ",
                    rsrp, " rsrq: ", rsrq," rssnr: ", rssnr, " cqi: ", cqi," timingAdvance: ",
                    timingAdvance);

                if (servingRat == static_cast<int>(telStub::RadioTechnology::RADIO_TECH_LTE)) {
                    int oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                        ["lteSignalStrengthInfo"]["lteSignalStrength"].asInt();
                    notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                        telStub::RadioTechnology::RADIO_TECH_LTE,
                        telStub::SignalStrengthMeasurementType::RSSI, oldValue, signalStrength);
                    if (!notify) { // if any one field changes, need to notify
                                   // skip next field check
                        oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                            ["lteSignalStrengthInfo"]["lteRsrp"].asInt();
                        notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                            telStub::RadioTechnology::RADIO_TECH_LTE,
                            telStub::SignalStrengthMeasurementType::RSRP, oldValue, rsrp);
                    }
                    if (!notify) { // if any one field changes, need to notify
                                   // skip next field check
                        oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                            ["lteSignalStrengthInfo"]["lteRsrq"].asInt();
                        notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                            telStub::RadioTechnology::RADIO_TECH_LTE,
                            telStub::SignalStrengthMeasurementType::RSRQ, oldValue, rsrq);
                    }
                    if (!notify) { // if any one field changes, need to notify
                                   // skip next field check
                        oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                            ["lteSignalStrengthInfo"]["lteRssnr"].asInt();
                        notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                            telStub::RadioTechnology::RADIO_TECH_LTE,
                            telStub::SignalStrengthMeasurementType::SNR, oldValue, rssnr);
                    }
                }
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteSignalStrength"] = signalStrength;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteRsrp"] = rsrp;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteRsrq"] = rsrq;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteRssnr"] = rssnr;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteCqi"] = cqi;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["timingAdvance"] = timingAdvance;
            } else if(rat == "NR5G") {
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rsrp = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rsrq = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rssnr = std::stoi(token);
                LOG(DEBUG, __FUNCTION__, " rsrp: ", rsrp, " rsrq: ", rsrq, " rssnr: ", rssnr);

                if (servingRat == static_cast<int>(telStub::RadioTechnology::RADIO_TECH_NR5G)) {
                    int oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                        ["nr5gSignalStrengthInfo"]["rsrp"].asInt();
                    notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                        telStub::RadioTechnology::RADIO_TECH_NR5G,
                        telStub::SignalStrengthMeasurementType::RSRP, oldValue, rsrp);
                    if (!notify) { // if any one field changes for particular RAT, need to notify
                                  // skip next field check
                        oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                            ["nr5gSignalStrengthInfo"]["rsrq"].asInt();
                        notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                            telStub::RadioTechnology::RADIO_TECH_NR5G,
                            telStub::SignalStrengthMeasurementType::RSRQ, oldValue, rsrq);
                    }
                    if (!notify) { // if any one field changes for particular RAT, need to notify
                                  // skip next field check
                        oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                            ["nr5gSignalStrengthInfo"]["rssnr"].asInt();
                        notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                            telStub::RadioTechnology::RADIO_TECH_NR5G,
                            telStub::SignalStrengthMeasurementType::SNR, oldValue, rssnr);
                    }
                }
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]\
                    ["rsrp"] = rsrp;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]\
                    ["rsrq"] = rsrq;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]\
                    ["rssnr"] = rssnr;
            } else if(rat == "NB1_NTN") {
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int signalStrength = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rsrp = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rsrq = std::stoi(token);
                token = telux::common::EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rssnr = std::stoi(token);
                LOG(DEBUG, __FUNCTION__, " signalStrength ", signalStrength, " rsrp: ", rsrp,
                    " rsrq: ", rsrq, " rssnr: ", rssnr);

                if (servingRat == static_cast<int>(telStub::RadioTechnology::RADIO_TECH_NB1_NTN)) {
                    int oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                        ["nb1NtnSignalStrengthInfo"]["signalStrength"].asInt();
                    notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                        telStub::RadioTechnology::RADIO_TECH_NB1_NTN,
                        telStub::SignalStrengthMeasurementType::RSSI, oldValue, signalStrength);
                    if (!notify) { // if any one field changes, need to notify
                                   // skip next field check
                        oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                            ["nb1NtnSignalStrengthInfo"]["rsrp"].asInt();
                        notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                            telStub::RadioTechnology::RADIO_TECH_NB1_NTN,
                            telStub::SignalStrengthMeasurementType::RSRP, oldValue, rsrp);
                        if (!notify) { // if any one field changes for particular RAT, need to
                                  // notify, skip next field check
                            oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                                ["nb1NtnSignalStrengthInfo"]["rsrq"].asInt();
                            notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                                telStub::RadioTechnology::RADIO_TECH_NB1_NTN,
                                telStub::SignalStrengthMeasurementType::RSRQ, oldValue, rsrq);
                        }
                        if (!notify) { // if any one field changes for particular RAT, need to
                                  // notify, skip next field check
                            oldValue = rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]\
                                ["nb1NtnSignalStrengthInfo"]["rssnr"].asInt();
                            notify = checkSignalStrengthCriteriaAndNotify(phoneId,
                                telStub::RadioTechnology::RADIO_TECH_NB1_NTN,
                                telStub::SignalStrengthMeasurementType::SNR, oldValue, rssnr);
                        }
                   }
                }
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
                    ["signalStrength"] = signalStrength;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
                    ["rsrp"] = rsrp;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
                    ["rsrq"] = rsrq;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nb1NtnSignalStrengthInfo"]\
                    ["rssnr"] = rssnr;
            } else {
                LOG(ERROR, " Invalid or deprecated RAT");
            }
        }
        LOG(DEBUG, __FUNCTION__, " need to notify : ", notify);
        // last notification time (this will be used if hystereis timer criteria is set)
        auto current = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(current);
        auto tm = std::localtime(&now_c);
        char buffer[32];
        std::strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", tm);
        rootObj[TEL_PHONE_MANAGER]["lastNotificationInfo"]["ssNotificationTimeStamp"] =
            std::string(buffer);
        errorCode = JsonParser::writeToJsonFile(rootObj, jsonfilename);
    }  catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        errorCode = telux::common::ErrorCode::INTERNAL_ERR;
    }
    return errorCode;
}

telux::common::ErrorCode TelUtil::writeCellInfoListToJsonFile(std::vector<std::string> params,
    int &phoneId) {
    Json::Value rootObj;
    std::string jsonfilename = "";
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(params[0], DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " PhoneId : ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for phone id");
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }

        jsonfilename = (phoneId == SLOT_1)? PHONE_JSON_STATE_PATH1 : PHONE_JSON_STATE_PATH2;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }

        rootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"].clear();
        int jsonCellCount = rootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"].size();
        int newCellCount = params.size() - 1;
        LOG(DEBUG, " jsonCellCount ", jsonCellCount , " newCellCount ", newCellCount);

        for (int i = 1; i <= newCellCount; i++) {
            LOG(DEBUG, " Parsing Params:" , params[i]);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int cell = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Cell Type is: ", cell);
            ::telStub::CellInfo_CellType cellType = static_cast<::telStub::CellInfo_CellType>(cell);
            if (cell < (static_cast<int>(::telStub::CellInfo_CellType::CellInfo_CellType_GSM)) ||
                cell > (static_cast<int>(::telStub::CellInfo_CellType::CellInfo_CellType_NB1_NTN)))
            {
                LOG(ERROR, __FUNCTION__, " Invalid input for cell type");
                return telux::common::ErrorCode::INVALID_ARGUMENTS;
            }

            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int registered = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Is registered cell: ", registered);

            rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["cellType"] = cell;
            rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["registered"] = registered;

            switch(cellType) {
                case ::telStub::CellInfo_CellType::CellInfo_CellType_GSM:
                {
                    std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lac = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int cid = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int arfcn = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int bsic = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int signalStrength = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int bitErrorRate = std::stoi(token);

                    LOG(DEBUG, __FUNCTION__," mcc:", mcc," mnc:", mnc, " lac:", lac, " cid:",
                        cid, " arfcn:", arfcn, " bsic:", bsic, " signalStrength:", signalStrength
                        , " bitErrorRate:", bitErrorRate);

                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["mcc"] = mcc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["mnc"] = mnc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["lac"] = lac;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["cid"] = cid;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["arfcn"] = arfcn;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["bsic"] = bsic;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmSignalStrengthInfo"]["gsmSignalStrength"] = signalStrength;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmSignalStrengthInfo"]["gsmBitErrorRate"] = bitErrorRate;

                    break;
                }
                case ::telStub::CellInfo_CellType::CellInfo_CellType_WCDMA:
                {
                    std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lac = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int cid = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int psc = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int uarfcn = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int signalStrength = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int bitErrorRate = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int ecio = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rscp = std::stoi(token);

                    LOG(DEBUG, __FUNCTION__," mcc:", mcc," mnc:", mnc, " lac:", lac, " cid:",
                        cid, " psc:", psc, " uarfcn:", uarfcn, " signalStrength:", signalStrength
                        , " bitErrorRate:", bitErrorRate, " ecio:", ecio, "rscp:", rscp);

                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["mcc"] = mcc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["mnc"] = mnc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["lac"] = lac;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["cid"] = cid;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["psc"] = psc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["uarfcn"] = uarfcn;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["signalStrength"] = signalStrength;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["bitErrorRate"] = bitErrorRate;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["ecio"] = ecio;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["rscp"] = rscp;

                    break;
                }
                case ::telStub::CellInfo_CellType::CellInfo_CellType_LTE:
                {
                    std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int ci = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int pci = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int tac = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int earfcn = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int signalStrength = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lteRsrp = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lteRsrq = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lteRssnr = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lteCqi = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int timingAdvance = std::stoi(token);

                    LOG(DEBUG, __FUNCTION__," mcc:", mcc," mnc:", mnc, " ci:", ci, " pci:",
                        pci, " tac:", tac, " earfcn:", earfcn, " signalStrength:", signalStrength
                        , "lteRsrp:", lteRsrp, "lteRsrq:", lteRsrq, "lteRssnr:", lteRssnr
                        , "lteCqi:", lteCqi, "timingAdvance:", timingAdvance);

                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["mcc"] = mcc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["mnc"] = mnc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["ci"] = ci;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["pci"] = pci;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["tac"] = tac;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["earfcn"] = earfcn;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteSignalStrength"] = signalStrength;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteRsrp"] = lteRsrp;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteRsrq"] = lteRsrq;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteRssnr"] = lteRssnr;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteCqi"] = lteCqi;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["timingAdvance"] = timingAdvance;

                    break;
                }
                case ::telStub::CellInfo_CellType::CellInfo_CellType_NR5G:
                {
                    std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int ci = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int pci = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int tac = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int arfcn = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rsrp = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rsrq = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rssnr = std::stoi(token);

                    LOG(DEBUG, __FUNCTION__," mcc:", mcc," mnc:", mnc, " ci:", ci, " pci:",
                        pci, " tac:", tac, " arfcn:", arfcn, " rsrp:", rsrp, "rsrq:", rsrq, "rssnr:"
                        , rssnr);

                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["mcc"] = mcc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["mnc"] = mnc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["ci"] = ci;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["pci"] = pci;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["tac"] = tac;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["arfcn"] = arfcn;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gSignalStrengthInfo"]["rsrp"] = rsrp;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gSignalStrengthInfo"]["rsrq"] = rsrq;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gSignalStrengthInfo"]["rssnr"] = rssnr;

                    break;
                }
                case ::telStub::CellInfo_CellType::CellInfo_CellType_NB1_NTN:
                {
                    std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int ci = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int tac = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int earfcn = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int signalStrength = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rsrp = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rsrq = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rssnr = std::stoi(token);

                    LOG(DEBUG, __FUNCTION__," mcc:", mcc," mnc:", mnc, " ci:", ci, " tac:", tac,
                        " earfcn:", earfcn, " signalStrength:", signalStrength
                        , "rsrp:", rsrp, "rsrq:", rsrq, "rssnr:", rssnr);

                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["nb1NtnCellInfo"]\
                        ["nb1NtnCellIdentity"]["mcc"] = mcc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["nb1NtnCellInfo"]\
                        ["nb1NtnCellIdentity"]["mnc"] = mnc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["nb1NtnCellInfo"]\
                        ["nb1NtnCellIdentity"]["ci"] = ci;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["nb1NtnCellInfo"]\
                        ["nb1NtnCellIdentity"]["tac"] = tac;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["nb1NtnCellInfo"]\
                        ["nb1NtnCellIdentity"]["earfcn"] = earfcn;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["nb1NtnCellInfo"]\
                        ["nb1NtnSignalStrengthInfo"]["signalStrength"] = signalStrength;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["nb1NtnCellInfo"]\
                        ["nb1NtnSignalStrengthInfo"]["rsrp"] = rsrp;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["nb1NtnCellInfo"]\
                        ["nb1NtnSignalStrengthInfo"]["rsrq"] = rsrq;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["nb1NtnCellInfo"]\
                        ["nb1NtnSignalStrengthInfo"]["rssnr"] = rssnr;
                    break;
                 }
                case ::telStub::CellInfo_CellType::CellInfo_CellType_CDMA:
                case ::telStub::CellInfo_CellType::CellInfo_CellType_TDSCDMA:
                default:
                {
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["registered"] = 0; // none of the cell is registered
                    LOG(ERROR, " Invalid or deprecated cell type");
                    break;
                }
            }
        }
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }
    return JsonParser::writeToJsonFile(rootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::writeVoiceServiceStateToJsonFile(std::string eventParams,
    int &phoneId) {
    Json::Value rootObj;
    std::string jsonfilename = "";
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }

        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int voiceServiceState = std::stoi(token);
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int voiceServiceDenialCause = std::stoi(token);
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int radioTech = std::stoi(token);
        Json::Value rootObj;
        jsonfilename = (phoneId == SLOT_1)? PHONE_JSON_STATE_PATH1 : PHONE_JSON_STATE_PATH2;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
        rootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]\
            ["voiceServiceState"] = voiceServiceState;
        rootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]\
            ["voiceServiceDenialCause"] = voiceServiceDenialCause;
        rootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]["radioTech"] = radioTech;

        LOG(DEBUG, __FUNCTION__, " VoiceServiceState:", voiceServiceState,
            " VoiceServiceDenialCause:", voiceServiceDenialCause, " RadioTech:",
            radioTech );
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }
    return JsonParser::writeToJsonFile(rootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::writeOperatingModeToJsonFile(std::string eventParams,
    int &phoneId) {
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    Json::Value rootObj;
    std::string jsonfilename = PHONE_JSON_STATE_PATH1;
    try {
        int operatingMode = std::stoi(token);
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
        rootObj[TEL_PHONE_MANAGER]["operatingModeInfo"]["operatingMode"] = operatingMode;
        LOG(DEBUG, __FUNCTION__, " OperatingMode:", operatingMode);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }
    return JsonParser::writeToJsonFile(rootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::writeEcallOperatingModeToJsonFile(std::string eventParams,
    int &phoneId) {

    Json::Value rootObj;
    std::string jsonfilename = "";
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
        jsonfilename = (phoneId == SLOT_1)? PHONE_JSON_STATE_PATH1 : PHONE_JSON_STATE_PATH2;
        // Read string to get eCall mode
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int ecallMode = std::stoi(token);
        if (ecallMode < (static_cast<int>(telStub::ECallMode::NORMAL)) ||
            ecallMode > (static_cast<int>(telStub::ECallMode::NONE))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for eCall mode");
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
        // Read string to get eCall mode reason
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int ecallModeReason = std::stoi(token);
        if (ecallModeReason < (static_cast<int>(telStub::ECallModeReason::NORMAL)) ||
            ecallModeReason > (static_cast<int>(telStub::ECallModeReason::ERA_GLONASS))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for eCall mode reason");
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }

        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
        rootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]["ecallMode"] = ecallMode;
        rootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]["ecallModeReason"] = ecallModeReason;
        LOG(DEBUG, __FUNCTION__, " ecallMode: ", ecallMode, " ecallModeReason: ", ecallModeReason);

    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }
    return JsonParser::writeToJsonFile(rootObj, jsonfilename);
}

telux::common::ErrorCode TelUtil::writeOperatorInfoToJsonFile(std::string eventParams,
    int &phoneId) {
    Json::Value rootObj;
    std::string jsonfilename = "";
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        jsonfilename = (phoneId == SLOT_1)? PHONE_JSON_STATE_PATH1 :
            PHONE_JSON_STATE_PATH2;
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
        ::telStub::PlmnInfo info;
        // Read string to get long name
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        string longName = token;
        info.set_long_name(token);

        // Read string to get short name
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        string shortName = token;
        info.set_short_name(token);

        // Read string to get plmn name
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        string plmn = token;
        info.set_plmn(token);

        // Read string to get home
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int home = std::stoi(token);
        info.set_ishome(false);
        if (home == 1 || home == 0) {
           if (home == 1) {
                info.set_ishome(true);
           }
        } else {
            LOG(ERROR, " Invalid input for home");
            return telux::common::ErrorCode::GENERIC_FAILURE;
        }
        rootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["longName"] = longName;
        rootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["shortName"] = shortName;
        rootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["plmn"] = plmn;
        rootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["home"] = static_cast<bool>(home);
        LOG(DEBUG, __FUNCTION__, " longName:", longName, " shortName:", shortName,
            " plmn:", plmn, " ishome:", home);

    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }
    return JsonParser::writeToJsonFile(rootObj, jsonfilename);
}

telStub::SignalStrengthChangeEvent TelUtil::createSignalStrengthEvent(int phoneId,
    telStub::SignalStrength &strength) {
    LOG(DEBUG, __FUNCTION__);
    telStub::SignalStrengthChangeEvent signalStrengthChangeEvent;
    signalStrengthChangeEvent.set_phone_id(phoneId);
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_gsm_signal_strength_info()->set_gsm_signal_strength(
            strength.gsm_signal_strength_info().gsm_signal_strength());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_gsm_signal_strength_info()->set_gsm_bit_error_rate(
            strength.gsm_signal_strength_info().gsm_bit_error_rate());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_wcdma_signal_strength_info()->set_signal_strength(
            strength.wcdma_signal_strength_info().signal_strength());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_wcdma_signal_strength_info()->set_bit_error_rate(
            strength.wcdma_signal_strength_info().bit_error_rate());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_wcdma_signal_strength_info()->set_ecio(
            strength.wcdma_signal_strength_info().ecio());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_wcdma_signal_strength_info()->set_rscp(
            strength.wcdma_signal_strength_info().rscp());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_lte_signal_strength_info()->set_lte_signal_strength(
            strength.lte_signal_strength_info().lte_signal_strength());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_lte_signal_strength_info()->set_lte_rsrp(
            strength.lte_signal_strength_info().lte_rsrp());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_lte_signal_strength_info()->set_lte_rsrq(
            strength.lte_signal_strength_info().lte_rsrq());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_lte_signal_strength_info()->set_lte_rssnr(
            strength.lte_signal_strength_info().lte_rssnr());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_lte_signal_strength_info()->set_lte_cqi(
            strength.lte_signal_strength_info().lte_cqi());
     signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_lte_signal_strength_info()->set_timing_advance(
            strength.lte_signal_strength_info().timing_advance());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_nr5g_signal_strength_info()->set_rsrp(
            strength.nr5g_signal_strength_info().rsrp());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_nr5g_signal_strength_info()->set_rsrq(
            strength.nr5g_signal_strength_info().rsrq());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_nr5g_signal_strength_info()->set_rssnr(
            strength.nr5g_signal_strength_info().rssnr());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_nb1_ntn_signal_strength_info()->set_signal_strength(
            strength.nb1_ntn_signal_strength_info().signal_strength());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_nb1_ntn_signal_strength_info()->set_rsrp(
            strength.nb1_ntn_signal_strength_info().rsrp());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_nb1_ntn_signal_strength_info()->set_rsrq(
            strength.nb1_ntn_signal_strength_info().rsrq());
    signalStrengthChangeEvent.mutable_signal_strength()->
        mutable_nb1_ntn_signal_strength_info()->set_rssnr(
            strength.nb1_ntn_signal_strength_info().rssnr());
    return signalStrengthChangeEvent;
}

telStub::SignalStrengthChangeEvent TelUtil::createSignalStrengthWithDefaultValues(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    telStub::SignalStrengthChangeEvent signalStrengthChangeEvent;
    signalStrengthChangeEvent.set_phone_id(phoneId);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_gsm_signal_strength_info()->
        set_gsm_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_gsm_signal_strength_info()->
        set_gsm_bit_error_rate(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
        set_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
        set_bit_error_rate(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
        set_ecio(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
        set_rscp(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_lte_signal_strength_info()->
        set_lte_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_lte_signal_strength_info()->
        set_lte_rsrp(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_lte_signal_strength_info()->
        set_lte_rsrq(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_lte_signal_strength_info()->
        set_lte_rssnr(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_lte_signal_strength_info()->
        set_lte_cqi(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_lte_signal_strength_info()->
        set_timing_advance(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->
        set_rsrp(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->
        set_rsrq(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->
        set_rssnr(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
        set_signal_strength(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
        set_rsrp(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
        set_rsrq(INVALID_SIGNAL_STRENGTH_VALUE);
    signalStrengthChangeEvent.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
        set_rssnr(INVALID_SIGNAL_STRENGTH_VALUE);
    return signalStrengthChangeEvent;
}

telStub::VoiceServiceStateEvent TelUtil::createVoiceServiceStateEvent(
    int phoneId, telStub::VoiceServiceStateInfo voiceServiceStateInfo) {
    telStub::VoiceServiceStateEvent voiceServiceStateChangeEvent;
    ::eventService::EventResponse anyResponse;
    voiceServiceStateChangeEvent.set_phone_id(phoneId);
    voiceServiceStateChangeEvent.mutable_voice_service_state_info()->set_voice_service_state(
        voiceServiceStateInfo.voice_service_state());
    voiceServiceStateChangeEvent.mutable_voice_service_state_info()->set_voice_service_denial_cause(
        voiceServiceStateInfo.voice_service_denial_cause());
    voiceServiceStateChangeEvent.mutable_voice_service_state_info()->set_radio_technology(
        voiceServiceStateInfo.radio_technology());
    return voiceServiceStateChangeEvent;
}

telStub::VoiceServiceStateEvent TelUtil::createVoiceServiceStateEvent(
    int phoneId, int voiceServiceState, int voiceServiceDenialCause, int radioTech) {
    LOG(DEBUG, __FUNCTION__);
    telStub::VoiceServiceStateEvent voiceServiceStateChangeEvent;
    voiceServiceStateChangeEvent.set_phone_id(phoneId);
    voiceServiceStateChangeEvent.mutable_voice_service_state_info()->set_voice_service_state
        (static_cast<telStub::VoiceServiceState>(voiceServiceState));
    voiceServiceStateChangeEvent.mutable_voice_service_state_info()->set_voice_service_denial_cause
        (static_cast<telStub::VoiceServiceDenialCause>(voiceServiceDenialCause));
    voiceServiceStateChangeEvent.mutable_voice_service_state_info()->set_radio_technology
        (static_cast<telStub::RadioTechnology>(radioTech));
    return voiceServiceStateChangeEvent;
}

telStub::OperatingModeEvent TelUtil::createOperatingModeEvent(telStub::OperatingMode mode) {
    LOG(DEBUG, __FUNCTION__);
    telStub::OperatingModeEvent opModeEvent;
    opModeEvent.set_operating_mode(mode);
    return opModeEvent;
}

telStub::ServiceStateChangeEvent TelUtil::createServiceStateEvent(int phoneId,
    telStub::ServiceState serviceState) {
    telStub::ServiceStateChangeEvent serviceStateChangeEvent;
    serviceStateChangeEvent.set_phone_id(phoneId);
    serviceStateChangeEvent.set_service_state(serviceState);
    return serviceStateChangeEvent;
}

telStub::VoiceRadioTechnologyChangeEvent TelUtil::createVoiceRadioTechnologyChangeEvent(
    int phoneId, telStub::RadioTechnology rat) {
    telStub::VoiceRadioTechnologyChangeEvent voiceRadioTechnologyChangeEvent;
    voiceRadioTechnologyChangeEvent.set_phone_id(phoneId);
    voiceRadioTechnologyChangeEvent.set_radio_technology(rat);
    return voiceRadioTechnologyChangeEvent;
}

telStub::RATCapability TelUtil::convertRATCapStringToEnum(std::string radioCap) {
    LOG(DEBUG, __FUNCTION__, " RadioCap : ", radioCap);
    if (radioCap == "AMPS") {
        return telStub::RATCapability::AMPS;
    } else if (radioCap == "CDMA") {
        return telStub::RATCapability::CDMA;
    } else if (radioCap == "HDR") {
        return telStub::RATCapability::HDR;
    } else if (radioCap == "GSM") {
        return telStub::RATCapability::GSM;
    } else if (radioCap == "WCDMA") {
        return telStub::RATCapability::WCDMA;
    } else if (radioCap == "LTE") {
        return telStub::RATCapability::LTE;
    } else if (radioCap == "NR5G") {
        return telStub::RATCapability::NR5G;
    } else if (radioCap == "NR5GSA") {
        return telStub::RATCapability::NR5GSA;
    } else if (radioCap == "NB1_NTN") {
        return telStub::RATCapability::NB1_NTN;
    } else {
        LOG(ERROR, " Invalid radio capability");
    }
    return telStub::RATCapability::RAT_CAP_INVALID;
}

telStub::VoiceServiceTechnology TelUtil::convertVoiceTechStringToEnum(std::string voiceTech) {
    LOG(DEBUG, __FUNCTION__, " VoiceTech : ", voiceTech);
    if (voiceTech == "GW_CSFB") {
        return ::telStub::VoiceServiceTechnology::VOICE_TECH_GW_CSFB;
    } else if (voiceTech == "1x_CSFB") {
        return ::telStub::VoiceServiceTechnology::VOICE_TECH_1x_CSFB;
    } else if (voiceTech == "VOLTE") {
        return ::telStub::VoiceServiceTechnology::VOICE_TECH_VOLTE;
    } else {
        LOG(ERROR, " Invalid VoiceTech");
    }
    return ::telStub::VoiceServiceTechnology::VOICE_TECH_INVALID;
}

/*
 * This function is to check signal strength criteria and control notification
 * based on the criteria set on subscription.
 * HysteresisMs is set, it is the highest priority and other criteria is ignored.
 * Delta or thresholdList can be set as criteria on particular RAT(rat+sigMeasType),
 * hysteresisDb can be applied only on top of thresholsList.If none of the criteria
 * is specified, as per on-target behaviour default values(specified in the RIL) are
 * set.
 * For example: RSSI(50), ECIO(10), SNR(40), RSRQ(20), RSRP(60), RSCP(40)
 */
int TelUtil::checkSignalStrengthCriteriaAndNotify(int phoneId, int rat, int sigMeasType,
    int oldValue, int newValue) {
    LOG(DEBUG, __FUNCTION__);
    bool notify = false;
    JsonData data;
    std::string stateJsonPath = "";

    telux::common::ErrorCode error = readJsonData(phoneId, TEL_PHONE_MANAGER,
        "configureSignalStrength", data, stateJsonPath);
    if (error == ErrorCode::SUCCESS) {
        if (data.status == telux::common::Status::SUCCESS) {
            uint16_t hysteresisMs = data.stateRootObj[TEL_PHONE_MANAGER]\
                ["configureSignalStrengthExInfo"]["hysteresisMs"].asInt();
            LOG(DEBUG, __FUNCTION__, " hysteresis timer : ", static_cast<int>(hysteresisMs));
            // if hysteresis timer is set, skip other criteria check
            if (hysteresisMs > 0) {
                std::string lastNotificationString = data.stateRootObj[TEL_PHONE_MANAGER]\
                    ["lastNotificationInfo"]["ssNotificationTimeStamp"].asString();
                LOG(DEBUG, __FUNCTION__, " lastNotification time : ", lastNotificationString);
                std::tm timeDate = {};
                std::istringstream ss(lastNotificationString);
                ss >> std::get_time(&timeDate, "%Y-%m-%d %H:%M:%S");
                auto lastNotification = std::chrono::system_clock::from_time_t(mktime(&timeDate));
                auto current = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsed_seconds = current - lastNotification;
                LOG(DEBUG, __FUNCTION__, " signal strength last notification in milliseconds : ",
                    elapsed_seconds.count()*1000);
                if ((elapsed_seconds.count()*1000) > hysteresisMs) {
                    // hysteresis timer crossed, notify here
                    LOG(DEBUG, __FUNCTION__, " Criteria: hysteresis timer satisfied");
                    notify = true;
                }
            } else {
                int diff = 0;
                uint16_t defaultDelta = 0;
                int diffSoFar = 0;
                int thresholdValue = 0;
                bool ratMatched = false;
                LOG(DEBUG, __FUNCTION__," oldValue : ", oldValue, " newValue : ", newValue);
                int currentCount = data.stateRootObj[TEL_PHONE_MANAGER]\
                    ["configureSignalStrengthExInfo"]["configureSignalStrengthExInfoList"].size();
                LOG(DEBUG, __FUNCTION__," current config count is : ", currentCount);
                int configType = static_cast<int>(telStub::SignalStrengthConfigExType::EX_DELTA);
                for (int j = 0; j < currentCount; j++) {
                    // check config type for RAT and signal measurement type provided from event
                    if (rat == (data.stateRootObj[TEL_PHONE_MANAGER]\
                        ["configureSignalStrengthExInfo"]["configureSignalStrengthExInfoList"][j]\
                        ["radioTech"].asInt()) &&
                        sigMeasType == (data.stateRootObj[TEL_PHONE_MANAGER]\
                            ["configureSignalStrengthExInfo"]["configureSignalStrengthExInfoList"]\
                            [j]["sigMeasType"].asInt())) {
                        LOG(DEBUG, __FUNCTION__, " matched RAT");
                        ratMatched = true;
                        int ctCount = data.stateRootObj[TEL_PHONE_MANAGER]\
                            ["configureSignalStrengthExInfo"]["configureSignalStrengthExInfoList"]\
                            [j]["configExType"].size();
                        LOG(DEBUG, __FUNCTION__," configType+++ count : ", ctCount);
                        for (int ct = 0; ct < ctCount; ct++) {
                            configType = data.stateRootObj[TEL_PHONE_MANAGER]\
                                ["configureSignalStrengthExInfo"]\
                                ["configureSignalStrengthExInfoList"][j]["configExType"][ct].asInt();
                            LOG(DEBUG, __FUNCTION__," configType : ", configType);
                            if (configType ==
                                static_cast<int>(telStub::SignalStrengthConfigExType::EX_DELTA)) {
                                diff = newValue - oldValue;
                                diff = (diff < 0) ? (-1*diff) : diff;
                                uint16_t delta = data.stateRootObj[TEL_PHONE_MANAGER]\
                                    ["configureSignalStrengthExInfo"]\
                                    ["configureSignalStrengthExInfoList"][j]["delta"].asInt();
                                LOG(DEBUG, " delta : ", delta);
                                if (diff >= (delta/10)) {
                                    // send notification
                                    LOG(DEBUG, __FUNCTION__, " Criteria: delta satisfied");
                                    notify = true;
                                }
                            } else if (configType == static_cast<int>
                                (telStub::SignalStrengthConfigExType::EX_THRESHOLD)) {
                                int thresholdListLen = data.stateRootObj[TEL_PHONE_MANAGER]\
                                ["configureSignalStrengthExInfo"]\
                                ["configureSignalStrengthExInfoList"][j]["thresholdList"].size();
                                LOG(DEBUG, "  threshold list size :", thresholdListLen);
                                if (thresholdListLen > 0 && thresholdListLen <= MAX_THRESHOLD_LIST)
                                {
                                    // signal strength(RSSI) and SNR is passed as positive integer,
                                    // convert to negative to compare with threshold.
                                    if (sigMeasType == static_cast<int>
                                        (telStub::SignalStrengthMeasurementType::RSSI)
                                        || sigMeasType == static_cast<int>
                                        (telStub::SignalStrengthMeasurementType::SNR))
                                    {
                                        // SNR can be negative or positive(range -200 to 300)
                                        if (oldValue > 0 && newValue > 0) {
                                            oldValue = oldValue * -1;
                                            newValue = newValue * -1;
                                        }
                                    }
                                    for (int thIdx = 0; thIdx < thresholdListLen; thIdx++) {
                                       int threshold = ((data.stateRootObj[TEL_PHONE_MANAGER]\
                                           ["configureSignalStrengthExInfo"]
                                           ["configureSignalStrengthExInfoList"][j]\
                                           ["thresholdList"][thIdx].asInt())/10);
                                        LOG(DEBUG, __FUNCTION__, " threshold: ", threshold * 10);
                                       // if newValue < threshold <= oldValue
                                       if ((newValue < threshold) && (threshold <= oldValue)) {
                                           LOG(DEBUG, __FUNCTION__,
                                               " Criteria: threshold satisfied");
                                           notify = true;
                                           diff = newValue - threshold;
                                           diff = (diff < 0) ? (-1*diff) : diff;
                                           if (diff >= diffSoFar) {
                                               diffSoFar = diff;
                                               thresholdValue = threshold;
                                           }
                                       }
                                       // if oldValue < threshold <= newValue
                                       if ((oldValue < threshold) && (threshold  <= newValue))
                                       {
                                           LOG(DEBUG, __FUNCTION__,
                                               " Criteria: threshold satisfied");
                                           notify = true;
                                           diff = newValue - threshold;
                                           diff = (diff < 0) ? (-1*diff) : diff;
                                           if (diff >= diffSoFar)
                                           {
                                               diffSoFar = diff;
                                               thresholdValue = threshold;
                                           }
                                       }
                                    }
                                }
                            }
                            if (configType == static_cast<int>
                                (telStub::SignalStrengthConfigExType::EX_HYSTERESIS_DB)){
                                uint16_t hysteresisDb = data.stateRootObj[TEL_PHONE_MANAGER]\
                                    ["configureSignalStrengthExInfo"]\
                                    ["configureSignalStrengthExInfoList"][j]["hysteresisDb"].asInt();
                                LOG(DEBUG, " hysteresisDb : ", hysteresisDb);
                                if (notify && hysteresisDb > 0) {
                                    diff = 0; // reset diff, threshold criteria is satisfied
                                    diff = newValue - thresholdValue;
                                    diff = (diff < 0) ? (-1*diff) : diff;
                                }
                                if (diff > (hysteresisDb/10)) {
                                    // send notification
                                    LOG(DEBUG, __FUNCTION__,
                                        " Criteria: threshold + hysteresis delta satisfied");
                                    notify = true;
                                }
                            }
                        }
                    }
                }
                if (!ratMatched) {
                    LOG(DEBUG, __FUNCTION__, " Criteria is not set, checking with default");
                    diff = newValue - oldValue;
                    diff = (diff < 0) ? (-1*diff) : diff;
                    switch (sigMeasType) {
                        case telStub::SignalStrengthMeasurementType::RSSI:
                            defaultDelta = 50;
                            break;
                        case telStub::SignalStrengthMeasurementType::ECIO:
                            defaultDelta = 10;
                            break;
                        case telStub::SignalStrengthMeasurementType::SNR:
                        case telStub::SignalStrengthMeasurementType::RSCP:
                            defaultDelta = 40;
                            break;
                        case telStub::SignalStrengthMeasurementType::RSRP:
                            defaultDelta = 60;
                            break;
                        case telStub::SignalStrengthMeasurementType::RSRQ:
                            defaultDelta = 20;
                            break;
                        default:
                            LOG(DEBUG, __FUNCTION__, " not supported signal type");
                            break;
                    }
                    // compare with default delta
                    if (diff >= (defaultDelta/10)) {
                        // send notification
                        LOG(DEBUG, __FUNCTION__, " Criteria: default delta satisfied");
                        notify = true;
                    }
                }
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read from JSON");
        // notify if no signal strength config criteria is set
        notify = true;
    }
    LOG(DEBUG, __FUNCTION__, " notify : ", notify);
    return notify;
}

}  // End of namespace tel
}  // End of namespace telux
