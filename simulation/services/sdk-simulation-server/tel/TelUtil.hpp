/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       TelUtil.hpp
 *             Utility class to read from and write to JSON file.
 *
 */

#ifndef TELUX_TEL_TELUTIL_HPP
#define TELUX_TEL_TELUTIL_HPP

#include <telux/common/CommonDefines.hpp>
#include <jsoncpp/json/json.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "libs/common/CommonUtils.hpp"

namespace telux {
namespace tel {

class TelUtil {

public:
    static telux::common::ErrorCode readFromJsonFile(int phoneId, std::string subsystem,
        Json::Value &rootObj, std::string &jsonfilename);
    static telux::common::ErrorCode readJsonData(int phoneId, std::string subsystem,
        std::string method, JsonData &data);
    static telux::common::ErrorCode readJsonData(int phoneId, std::string subsystem,
        std::string method, JsonData &data, std::string &stateJsonPath);

    static JsonData readGetPhoneIdsRespFromJsonFile(telStub::GetPhoneIdsReply *response);
    static JsonData readGetPhoneIdRespFromJsonFile(telStub::GetPhoneIdReply *response);
    static JsonData readSignalStrengthRespFromJsonFile(int phoneId,
        telStub::GetSignalStrengthReply *response);
    static JsonData readVoiceServiceStateRespFromJsonFile(int phoneId,
        telStub::RequestVoiceServiceStateReply *response);
    static JsonData readCellularCapabilitiesRespFromJsonFile(
        telStub::CellularCapabilityInfoReply* response);
    static JsonData readOperatingModeRespFromJsonFile(telStub::GetOperatingModeReply *response);
    static JsonData readCellInfoListRespFromJsonFile(int phoneId,
        telStub::RequestCellInfoListReply *response);
    static JsonData readEcallOperatingModeRespFromJsonFile(int phoneId,
        telStub::GetECallOperatingModeReply* response);
    static JsonData readResetWwanRespFromJsonFile(telStub::ResetWwanReply* response);
    static JsonData readRequestOperatorInfoRespFromJsonFile(int phoneId,
        telStub::RequestOperatorInfoReply* response);

    static telux::common::ErrorCode readSignalStrengthEventFromJsonFile(int phoneId,
        telStub::SignalStrengthChangeEvent &event);
    static telux::common::ErrorCode readCellInfoListEventFromJsonFile(int phoneId,
        telStub::CellInfoListEvent &event);
    static telux::common::ErrorCode readVoiceServiceStateEventFromJsonFile(int phoneId,
        telStub::VoiceServiceStateEvent &event);
    static telux::common::ErrorCode readOperatingModeEventFromJsonFile(
        telStub::OperatingModeEvent &event);
    static telux::common::ErrorCode readServiceStateEventFromJsonFile(int phoneId,
        telStub::ServiceStateChangeEvent &event);
    static telux::common::ErrorCode readVoiceRadioTechnologyEventFromJsonFile(int phoneId,
        telStub::VoiceRadioTechnologyChangeEvent &event);
    static telux::common::ErrorCode readEcallOperatingModeEventFromJsonFile(int phoneId,
        telStub::ECallModeInfoChangeEvent &event);
    static telux::common::ErrorCode readOperatorInfoEventFromJsonFile(int phoneId,
        telStub::OperatorInfoEvent &event);

    static telux::common::ErrorCode readSignalStrengthFromJsonFile(int phoneId,
        telStub::SignalStrength &signalStrength);
    static telux::common::ErrorCode readServiceStateFromJsonFile(int phoneId,
        telStub::ServiceState &state);
    static telux::common::ErrorCode readVoiceRadioTechnologyFromJsonFile(int phoneId,
        telStub::RadioTechnology &rat);
    static telux::common::ErrorCode readSystemInfoFromJsonFile(int phoneId,
        telStub::RadioTechnology &servingRat, telStub::ServiceDomainInfo_Domain &servingDomain);
    static telux::common::ErrorCode readOperatingModeFromJsonFile(telStub::OperatingMode &mode);
    static telux::common::ErrorCode readRatPreferenceFromJsonFile(int phoneId,
        std::vector<int> &ratData);

    static telux::common::ErrorCode writeSignalStrengthToJsonFile(int phoneId,
        telStub::SignalStrengthChangeEvent &event);
    static telux::common::ErrorCode writeVoiceServiceStateToJsonFile(int phoneId,
        telStub::VoiceServiceStateEvent &event);
    static telux::common::ErrorCode writeOperatingModeToJsonFile(
        telStub::OperatingModeEvent &event);
    static telux::common::ErrorCode writeServiceStateToJsonFile(int phoneId,
        telStub::ServiceStateChangeEvent &event);
    static telux::common::ErrorCode writeVoiceRadioTechnologyToJsonFile(int phoneId,
        telStub::VoiceRadioTechnologyChangeEvent &event);
    static telux::common::ErrorCode writeSystemInfoToJsonFile(int phoneId,
        telStub::RadioTechnology &servingRat, telStub::ServiceDomainInfo_Domain &servingDomain);

    static JsonData writeOperatingModeToJsonFileAndReply(telStub::OperatingMode mode,
        telStub::SetOperatingModeReply* response);
    static telux::common::ErrorCode writeSetCellInfoListRateToJsonFileAndReply(int cellInfoListRate,
        telStub::SetCellInfoListRateReply *response);
    static telux::common::ErrorCode writeSetRadioPowerToJsonFileAndReply(int phoneId, int enable,
        telStub::SetRadioPowerReply *response);
    static telux::common::ErrorCode writeEcallOperatingModeToJsonFileAndReply(int phoneId,
        telStub::ECallMode mode, telStub::ECallModeReason_Reason reason,
        telStub::SetECallOperatingModeReply *response);
    static telux::common::ErrorCode writeConfigureSignalStrengthToJsonFileAndReply(int phoneId,
        std::vector<telStub::ConfigureSignalStrength> configs,
        telStub::ConfigureSignalStrengthReply* response);
    static telux::common::ErrorCode writeConfigureSignalStrengthExToJsonFileAndReply(int phoneId,
        std::vector<telStub::ConfigureSignalStrengthEx> signalStrengthConfigEx,
        telStub::ConfigureSignalStrengthExReply* response, uint16_t hysTimer);

    static telux::common::ErrorCode writeSignalStrengthToJsonFile(std::vector<std::string> params,
        int &phoneId, bool &notify);
    static telux::common::ErrorCode writeCellInfoListToJsonFile(std::vector<std::string> params,
        int &phoneId);
    static telux::common::ErrorCode writeVoiceServiceStateToJsonFile(std::string params,
        int &phoneId);
    static telux::common::ErrorCode writeOperatingModeToJsonFile(std::string params,
        int &phoneId);
    static telux::common::ErrorCode writeEcallOperatingModeToJsonFile(std::string params,
        int &phoneId);
    static telux::common::ErrorCode writeOperatorInfoToJsonFile(std::string params,
        int &phoneId);

    static telStub::SignalStrengthChangeEvent createSignalStrengthEvent(int phoneId,
        telStub::SignalStrength &strength);
    static telStub::SignalStrengthChangeEvent createSignalStrengthWithDefaultValues(int phoneId);
    static telStub::VoiceServiceStateEvent createVoiceServiceStateEvent(int phoneId,
        telStub::VoiceServiceStateInfo voiceServiceStateInfo);
    static telStub::VoiceServiceStateEvent createVoiceServiceStateEvent(int phoneId,
        int voiceServiceState, int voiceServiceDenialCause, int radioTech);
    static telStub::OperatingModeEvent createOperatingModeEvent(telStub::OperatingMode mode);
    static telStub::ServiceStateChangeEvent createServiceStateEvent(int phoneId,
        telStub::ServiceState serviceState);
    static telStub::VoiceRadioTechnologyChangeEvent createVoiceRadioTechnologyChangeEvent(
        int phoneId, telStub::RadioTechnology rat);
    static int checkSignalStrengthCriteriaAndNotify(int phoneId, int rat, int sigMeasType,
        int oldValue, int newValue);

    //Utilities
    static telStub::RATCapability convertRATCapStringToEnum(std::string radioCap);
    static telStub::VoiceServiceTechnology convertVoiceTechStringToEnum(std::string voiceTech);

    template<typename T>
    static void updateResponse(T* response, JsonData &data) {
       if(data.cbDelay != -1) {
        response->set_iscallback(true);
        } else {
            response->set_iscallback(false);
        }
        response->set_error(static_cast<commonStub::ErrorCode>(data.error));
        response->set_delay(data.cbDelay);
        response->set_status(static_cast<commonStub::Status>(data.status));
        LOG(DEBUG, __FUNCTION__, " error: ", response->error(), " status: ", response->status());
    }

};

}  // namespace tel
}  // namespace telux

#endif // TELUX_TEL_TELUTIL_HPP
