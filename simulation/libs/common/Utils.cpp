/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <map>
#include <telux/common/Utils.hpp>

#include "Logger.hpp"

namespace telux {
namespace common {

static std::map<ErrorCode, std::string> errorCodeToStringMap_;
static bool isInitDone_ = false;

void initErrorCodeToStringMap() {
    if ( isInitDone_ ) {
        return;
    }
    isInitDone_ = true;
    errorCodeToStringMap_[ErrorCode::SUCCESS] = "SUCCESS";
    errorCodeToStringMap_[ErrorCode::RADIO_NOT_AVAILABLE] = "RADIO_NOT_AVAILABLE";
    errorCodeToStringMap_[ErrorCode::GENERIC_FAILURE] = "GENERIC_FAILURE";
    errorCodeToStringMap_[ErrorCode::PASSWORD_INCORRECT] = "PASSWORD_INCORRECT";
    errorCodeToStringMap_[ErrorCode::SIM_PIN2] = "SIM_PIN2";
    errorCodeToStringMap_[ErrorCode::SIM_PUK2] = "SIM_PUK2";
    errorCodeToStringMap_[ErrorCode::REQUEST_NOT_SUPPORTED] = "REQUEST_NOT_SUPPORTED";
    errorCodeToStringMap_[ErrorCode::CANCELLED] = "CANCELLED";
    errorCodeToStringMap_[ErrorCode::OP_NOT_ALLOWED_DURING_VOICE_CALL]
        = "OP_NOT_ALLOWED_DURING_VOICE_CALL";
    errorCodeToStringMap_[ErrorCode::OP_NOT_ALLOWED_BEFORE_REG_TO_NW]
        = "OP_NOT_ALLOWED_BEFORE_REG_TO_NW";
    errorCodeToStringMap_[ErrorCode::SMS_SEND_FAIL_RETRY] = "SMS_SEND_FAIL_RETRY";
    errorCodeToStringMap_[ErrorCode::SIM_ABSENT] = "SIM_ABSENT";
    errorCodeToStringMap_[ErrorCode::SUBSCRIPTION_NOT_AVAILABLE] = "SUBSCRIPTION_NOT_AVAILABLE";
    errorCodeToStringMap_[ErrorCode::MODE_NOT_SUPPORTED] = "MODE_NOT_SUPPORTED";
    errorCodeToStringMap_[ErrorCode::FDN_CHECK_FAILURE] = "FDN_CHECK_FAILURE";
    errorCodeToStringMap_[ErrorCode::ILLEGAL_SIM_OR_ME] = "ILLEGAL_SIM_OR_ME";
    errorCodeToStringMap_[ErrorCode::MISSING_RESOURCE] = "MISSING_RESOURCE";
    errorCodeToStringMap_[ErrorCode::NO_SUCH_ELEMENT] = "NO_SUCH_ELEMENT";
    errorCodeToStringMap_[ErrorCode::DIAL_MODIFIED_TO_USSD] = "DIAL_MODIFIED_TO_USSD";
    errorCodeToStringMap_[ErrorCode::DIAL_MODIFIED_TO_SS] = "DIAL_MODIFIED_TO_SS";
    errorCodeToStringMap_[ErrorCode::DIAL_MODIFIED_TO_DIAL] = "DIAL_MODIFIED_TO_DIAL";
    errorCodeToStringMap_[ErrorCode::USSD_MODIFIED_TO_DIAL] = "USSD_MODIFIED_TO_DIAL";
    errorCodeToStringMap_[ErrorCode::USSD_MODIFIED_TO_SS] = "USSD_MODIFIED_TO_SS";
    errorCodeToStringMap_[ErrorCode::USSD_MODIFIED_TO_USSD] = "USSD_MODIFIED_TO_USSD";
    errorCodeToStringMap_[ErrorCode::SS_MODIFIED_TO_DIAL] = "SS_MODIFIED_TO_DIAL";
    errorCodeToStringMap_[ErrorCode::SS_MODIFIED_TO_USSD] = "SS_MODIFIED_TO_USSD";
    errorCodeToStringMap_[ErrorCode::SUBSCRIPTION_NOT_SUPPORTED] = "SUBSCRIPTION_NOT_SUPPORTED";
    errorCodeToStringMap_[ErrorCode::SS_MODIFIED_TO_SS] = "SS_MODIFIED_TO_SS";
    errorCodeToStringMap_[ErrorCode::LCE_NOT_SUPPORTED] = "LCE_NOT_SUPPORTED";
    errorCodeToStringMap_[ErrorCode::NO_MEMORY] = "NO_MEMORY";
    errorCodeToStringMap_[ErrorCode::INTERNAL_ERR] = "INTERNAL_ERR";
    errorCodeToStringMap_[ErrorCode::SYSTEM_ERR] = "SYSTEM_ERR";
    errorCodeToStringMap_[ErrorCode::MODEM_ERR] = "MODEM_ERR";
    errorCodeToStringMap_[ErrorCode::INVALID_STATE] = "INVALID_STATE";
    errorCodeToStringMap_[ErrorCode::NO_RESOURCES] = "NO_RESOURCES";
    errorCodeToStringMap_[ErrorCode::SIM_ERR] = "SIM_ERR";
    errorCodeToStringMap_[ErrorCode::INVALID_ARGUMENTS] = "INVALID_ARGUMENTS";
    errorCodeToStringMap_[ErrorCode::INVALID_SIM_STATE] = "INVALID_SIM_STATE";
    errorCodeToStringMap_[ErrorCode::INVALID_MODEM_STATE] = "INVALID_MODEM_STATE";
    errorCodeToStringMap_[ErrorCode::INVALID_CALL_ID] = "INVALID_CALL_ID";
    errorCodeToStringMap_[ErrorCode::NO_SMS_TO_ACK] = "NO_SMS_TO_ACK";
    errorCodeToStringMap_[ErrorCode::NETWORK_ERR] = "NETWORK_ERR";
    errorCodeToStringMap_[ErrorCode::REQUEST_RATE_LIMITED] = "REQUEST_RATE_LIMITED";
    errorCodeToStringMap_[ErrorCode::SIM_BUSY] = "SIM_BUSY";
    errorCodeToStringMap_[ErrorCode::SIM_FULL] = "SIM_FULL";
    errorCodeToStringMap_[ErrorCode::NETWORK_REJECT] = "NETWORK_REJECT";
    errorCodeToStringMap_[ErrorCode::OPERATION_NOT_ALLOWED] = "OPERATION_NOT_ALLOWED";
    errorCodeToStringMap_[ErrorCode::EMPTY_RECORD] = "EMPTY_RECORD";
    errorCodeToStringMap_[ErrorCode::INVALID_SMS_FORMAT] = "INVALID_SMS_FORMAT";
    errorCodeToStringMap_[ErrorCode::ENCODING_ERR] = "ENCODING_ERR";
    errorCodeToStringMap_[ErrorCode::INVALID_SMSC_ADDRESS] = "INVALID_SMSC_ADDRESS";
    errorCodeToStringMap_[ErrorCode::NO_SUCH_ENTRY] = "NO_SUCH_ENTRY";
    errorCodeToStringMap_[ErrorCode::NETWORK_NOT_READY] = "NETWORK_NOT_READY";
    errorCodeToStringMap_[ErrorCode::NOT_PROVISIONED] = "NOT_PROVISIONED";
    errorCodeToStringMap_[ErrorCode::NO_SUBSCRIPTION] = "NO_SUBSCRIPTION";
    errorCodeToStringMap_[ErrorCode::NO_NETWORK_FOUND] = "NO_NETWORK_FOUND";
    errorCodeToStringMap_[ErrorCode::DEVICE_IN_USE] = "DEVICE_IN_USE";
    errorCodeToStringMap_[ErrorCode::ABORTED] = "ABORTED";
    errorCodeToStringMap_[ErrorCode::NO_EFFECT] = "NO_EFFECT";
    errorCodeToStringMap_[ErrorCode::DEVICE_NOT_READY] = "DEVICE_NOT_READY";
    errorCodeToStringMap_[ErrorCode::MISSING_ARGUMENTS] = "MISSING_ARGUMENTS";
    errorCodeToStringMap_[ErrorCode::MALFORMED_MSG] = "MALFORMED_MSG";
    errorCodeToStringMap_[ErrorCode::INTERNAL] = "INTERNAL";
    errorCodeToStringMap_[ErrorCode::CLIENT_IDS_EXHAUSTED] = "CLIENT_IDS_EXHAUSTED";
    errorCodeToStringMap_[ErrorCode::UNABORTABLE_TRANSACTION] = "UNABORTABLE_TRANSACTION";
    errorCodeToStringMap_[ErrorCode::INVALID_CLIENT_ID] = "INVALID_CLIENT_ID";
    errorCodeToStringMap_[ErrorCode::NO_THRESHOLDS] = "NO_THRESHOLDS";
    errorCodeToStringMap_[ErrorCode::INVALID_HANDLE] = "INVALID_HANDLE";
    errorCodeToStringMap_[ErrorCode::INVALID_PROFILE] = "INVALID_PROFILE";
    errorCodeToStringMap_[ErrorCode::INVALID_PINID] = "INVALID_PINID";
    errorCodeToStringMap_[ErrorCode::INCORRECT_PIN] = "INCORRECT_PIN";
    errorCodeToStringMap_[ErrorCode::CALL_FAILED] = "CALL_FAILED";
    errorCodeToStringMap_[ErrorCode::OUT_OF_CALL] = "OUT_OF_CALL";
    errorCodeToStringMap_[ErrorCode::MISSING_ARG] = "MISSING_ARG";
    errorCodeToStringMap_[ErrorCode::ARG_TOO_LONG] = "ARG_TOO_LONG";
    errorCodeToStringMap_[ErrorCode::INVALID_TX_ID] = "INVALID_TX_ID";
    errorCodeToStringMap_[ErrorCode::OP_NETWORK_UNSUPPORTED] = "OP_NETWORK_UNSUPPORTED";
    errorCodeToStringMap_[ErrorCode::OP_DEVICE_UNSUPPORTED] = "OP_DEVICE_UNSUPPORTED";
    errorCodeToStringMap_[ErrorCode::NO_FREE_PROFILE] = "NO_FREE_PROFILE";
    errorCodeToStringMap_[ErrorCode::INVALID_PDP_TYPE] = "INVALID_PDP_TYPE";
    errorCodeToStringMap_[ErrorCode::INVALID_TECH_PREF] = "INVALID_TECH_PREF";
    errorCodeToStringMap_[ErrorCode::INVALID_PROFILE_TYPE] = "INVALID_PROFILE_TYPE";
    errorCodeToStringMap_[ErrorCode::INVALID_SERVICE_TYPE] = "INVALID_SERVICE_TYPE";
    errorCodeToStringMap_[ErrorCode::INVALID_REGISTER_ACTION] = "INVALID_REGISTER_ACTION";
    errorCodeToStringMap_[ErrorCode::INVALID_PS_ATTACH_ACTION] = "INVALID_PS_ATTACH_ACTION";
    errorCodeToStringMap_[ErrorCode::AUTHENTICATION_FAILED] = "AUTHENTICATION_FAILED";
    errorCodeToStringMap_[ErrorCode::PIN_BLOCKED] = "PIN_BLOCKED";
    errorCodeToStringMap_[ErrorCode::PIN_PERM_BLOCKED] = "PIN_PERM_BLOCKED";
    errorCodeToStringMap_[ErrorCode::SIM_NOT_INITIALIZED] = "SIM_NOT_INITIALIZED";
    errorCodeToStringMap_[ErrorCode::MAX_QOS_REQUESTS_IN_USE] = "MAX_QOS_REQUESTS_IN_USE";
    errorCodeToStringMap_[ErrorCode::INCORRECT_FLOW_FILTER] = "INCORRECT_FLOW_FILTER";
    errorCodeToStringMap_[ErrorCode::NETWORK_QOS_UNAWARE] = "NETWORK_QOS_UNAWARE";
    errorCodeToStringMap_[ErrorCode::INVALID_ID] = "INVALID_ID";
    errorCodeToStringMap_[ErrorCode::REQUESTED_NUM_UNSUPPORTED] = "REQUESTED_NUM_UNSUPPORTED";
    errorCodeToStringMap_[ErrorCode::INTERFACE_NOT_FOUND] = "INTERFACE_NOT_FOUND";
    errorCodeToStringMap_[ErrorCode::FLOW_SUSPENDED] = "FLOW_SUSPENDED";
    errorCodeToStringMap_[ErrorCode::INVALID_DATA_FORMAT] = "INVALID_DATA_FORMAT";
    errorCodeToStringMap_[ErrorCode::GENERAL] = "GENERAL";
    errorCodeToStringMap_[ErrorCode::UNKNOWN] = "UNKNOWN";
    errorCodeToStringMap_[ErrorCode::INVALID_ARG] = "INVALID_ARG";
    errorCodeToStringMap_[ErrorCode::INVALID_INDEX] = "INVALID_INDEX";
    errorCodeToStringMap_[ErrorCode::NO_ENTRY] = "NO_ENTRY";
    errorCodeToStringMap_[ErrorCode::DEVICE_STORAGE_FULL] = "DEVICE_STORAGE_FULL";
    errorCodeToStringMap_[ErrorCode::CAUSE_CODE] = "CAUSE_CODE";
    errorCodeToStringMap_[ErrorCode::MESSAGE_NOT_SENT] = "MESSAGE_NOT_SENT";
    errorCodeToStringMap_[ErrorCode::MESSAGE_DELIVERY_FAILURE] = "MESSAGE_DELIVERY_FAILURE";
    errorCodeToStringMap_[ErrorCode::INVALID_MESSAGE_ID] = "INVALID_MESSAGE_ID";
    errorCodeToStringMap_[ErrorCode::ENCODING] = "ENCODING";
    errorCodeToStringMap_[ErrorCode::AUTHENTICATION_LOCK] = "AUTHENTICATION_LOCK";
    errorCodeToStringMap_[ErrorCode::INVALID_TRANSITION] = "INVALID_TRANSITION";
    errorCodeToStringMap_[ErrorCode::NOT_A_MCAST_IFACE] = "NOT_A_MCAST_IFACE";
    errorCodeToStringMap_[ErrorCode::MAX_MCAST_REQUESTS_IN_USE] = "MAX_MCAST_REQUESTS_IN_USE";
    errorCodeToStringMap_[ErrorCode::INVALID_MCAST_HANDLE] = "INVALID_MCAST_HANDLE";
    errorCodeToStringMap_[ErrorCode::INVALID_IP_FAMILY_PREF] = "INVALID_IP_FAMILY_PREF";
    errorCodeToStringMap_[ErrorCode::SESSION_INACTIVE] = "SESSION_INACTIVE";
    errorCodeToStringMap_[ErrorCode::SESSION_INVALID] = "SESSION_INVALID";
    errorCodeToStringMap_[ErrorCode::SESSION_OWNERSHIP] = "SESSION_OWNERSHIP";
    errorCodeToStringMap_[ErrorCode::INSUFFICIENT_RESOURCES] = "INSUFFICIENT_RESOURCES";
    errorCodeToStringMap_[ErrorCode::DISABLED] = "DISABLED";
    errorCodeToStringMap_[ErrorCode::INVALID_OPERATION] = "INVALID_OPERATION";
    errorCodeToStringMap_[ErrorCode::INVALID_QMI_CMD] = "INVALID_QMI_CMD";
    errorCodeToStringMap_[ErrorCode::TPDU_TYPE] = "TPDU_TYPE";
    errorCodeToStringMap_[ErrorCode::SMSC_ADDR] = "SMSC_ADDR";
    errorCodeToStringMap_[ErrorCode::INFO_UNAVAILABLE] = "INFO_UNAVAILABLE";
    errorCodeToStringMap_[ErrorCode::SEGMENT_TOO_LONG] = "SEGMENT_TOO_LONG";
    errorCodeToStringMap_[ErrorCode::SEGMENT_ORDER] = "SEGMENT_ORDER";
    errorCodeToStringMap_[ErrorCode::BUNDLING_NOT_SUPPORTED] = "BUNDLING_NOT_SUPPORTED";
    errorCodeToStringMap_[ErrorCode::OP_PARTIAL_FAILURE] = "OP_PARTIAL_FAILURE";
    errorCodeToStringMap_[ErrorCode::POLICY_MISMATCH] = "POLICY_MISMATCH";
    errorCodeToStringMap_[ErrorCode::SIM_FILE_NOT_FOUND] = "SIM_FILE_NOT_FOUND";
    errorCodeToStringMap_[ErrorCode::EXTENDED_INTERNAL] = "EXTENDED_INTERNAL";
    errorCodeToStringMap_[ErrorCode::ACCESS_DENIED] = "ACCESS_DENIED";
    errorCodeToStringMap_[ErrorCode::HARDWARE_RESTRICTED] = "HARDWARE_RESTRICTED";
    errorCodeToStringMap_[ErrorCode::ACK_NOT_SENT] = "ACK_NOT_SENT";
    errorCodeToStringMap_[ErrorCode::INJECT_TIMEOUT] = "INJECT_TIMEOUT";
    errorCodeToStringMap_[ErrorCode::FDN_RESTRICT] = "FDN_RESTRICT";
    errorCodeToStringMap_[ErrorCode::SUPS_FAILURE_CAUSE] = "SUPS_FAILURE_CAUSE";
    errorCodeToStringMap_[ErrorCode::NO_RADIO] = "NO_RADIO";
    errorCodeToStringMap_[ErrorCode::NOT_SUPPORTED] = "NOT_SUPPORTED";
    errorCodeToStringMap_[ErrorCode::CARD_CALL_CONTROL_FAILED] = "CARD_CALL_CONTROL_FAILED";
    errorCodeToStringMap_[ErrorCode::NETWORK_ABORTED] = "NETWORK_ABORTED";
    errorCodeToStringMap_[ErrorCode::MSG_BLOCKED] = "MSG_BLOCKED";
    errorCodeToStringMap_[ErrorCode::INVALID_SESSION_TYPE] = "INVALID_SESSION_TYPE";
    errorCodeToStringMap_[ErrorCode::INVALID_PB_TYPE] = "INVALID_PB_TYPE";
    errorCodeToStringMap_[ErrorCode::NO_SIM] = "NO_SIM";
    errorCodeToStringMap_[ErrorCode::PB_NOT_READY] = "PB_NOT_READY";
    errorCodeToStringMap_[ErrorCode::PIN_RESTRICTION] = "PIN_RESTRICTION";
    errorCodeToStringMap_[ErrorCode::PIN2_RESTRICTION] = "PIN2_RESTRICTION";
    errorCodeToStringMap_[ErrorCode::PUK_RESTRICTION] = "PUK_RESTRICTION";
    errorCodeToStringMap_[ErrorCode::PUK2_RESTRICTION] = "PUK2_RESTRICTION";
    errorCodeToStringMap_[ErrorCode::PB_ACCESS_RESTRICTED] = "PB_ACCESS_RESTRICTED";
    errorCodeToStringMap_[ErrorCode::PB_DELETE_IN_PROG] = "PB_DELETE_IN_PROG";
    errorCodeToStringMap_[ErrorCode::PB_TEXT_TOO_LONG] = "PB_TEXT_TOO_LONG";
    errorCodeToStringMap_[ErrorCode::PB_NUMBER_TOO_LONG] = "PB_NUMBER_TOO_LONG";
    errorCodeToStringMap_[ErrorCode::PB_HIDDEN_KEY_RESTRICTION] = "PB_HIDDEN_KEY_RESTRICTION";
    errorCodeToStringMap_[ErrorCode::PB_NOT_AVAILABLE] = "PB_NOT_AVAILABLE";
    errorCodeToStringMap_[ErrorCode::DEVICE_MEMORY_ERROR] = "DEVICE_MEMORY_ERROR";
    errorCodeToStringMap_[ErrorCode::NO_PERMISSION] = "NO_PERMISSION";
    errorCodeToStringMap_[ErrorCode::TOO_SOON] = "TOO_SOON";
    errorCodeToStringMap_[ErrorCode::TIME_NOT_ACQUIRED] = "TIME_NOT_ACQUIRED";
    errorCodeToStringMap_[ErrorCode::OP_IN_PROGRESS] = "OP_IN_PROGRESS";
    errorCodeToStringMap_[ErrorCode::INTERNAL_ERROR] = "INTERNAL_ERROR";
    errorCodeToStringMap_[ErrorCode::SERVICE_ERROR] = "SERVICE_ERROR";
    errorCodeToStringMap_[ErrorCode::TIMEOUT_ERROR] = "TIMEOUT_ERROR";
    errorCodeToStringMap_[ErrorCode::EXTENDED_ERROR] = "EXTENDED_ERROR";
    errorCodeToStringMap_[ErrorCode::PORT_NOT_OPEN_ERROR] = "PORT_NOT_OPEN_ERROR";
    errorCodeToStringMap_[ErrorCode::MEMCOPY_ERROR] = "MEMCOPY_ERROR";
    errorCodeToStringMap_[ErrorCode::INVALID_TRANSACTION] = "INVALID_TRANSACTION";
    errorCodeToStringMap_[ErrorCode::ALLOCATION_FAILURE] = "ALLOCATION_FAILURE";
    errorCodeToStringMap_[ErrorCode::TRANSPORT_ERROR] = "TRANSPORT_ERROR";
    errorCodeToStringMap_[ErrorCode::PARAM_ERROR] = "PARAM_ERROR";
    errorCodeToStringMap_[ErrorCode::INVALID_CLIENT] = "INVALID_CLIENT";
    errorCodeToStringMap_[ErrorCode::FRAMEWORK_NOT_READY] = "FRAMEWORK_NOT_READY";
    errorCodeToStringMap_[ErrorCode::INVALID_SIGNAL] = "INVALID_SIGNAL";
    errorCodeToStringMap_[ErrorCode::TRANSPORT_BUSY_ERROR] = "TRANSPORT_BUSY_ERROR";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_FAIL] = "DS_PROFILE_REG_RESULT_FAIL";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_HNDL]
        = "DS_PROFILE_REG_RESULT_ERR_INVAL_HNDL";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_OP]
        = "DS_PROFILE_REG_RESULT_ERR_INVAL_OP";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_TYPE]
        = "DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_TYPE";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_NUM]
        = "DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_NUM";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_IDENT]
        = "DS_PROFILE_REG_RESULT_ERR_INVAL_IDENT";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL]
        = "DS_PROFILE_REG_RESULT_ERR_INVAL";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_LIB_NOT_INITED]
        = "DS_PROFILE_REG_RESULT_ERR_LIB_NOT_INITED";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_LEN_INVALID]
        = "DS_PROFILE_REG_RESULT_ERR_LEN_INVALID";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_LIST_END]
        = "DS_PROFILE_REG_RESULT_LIST_END";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_SUBS_ID]
        = "DS_PROFILE_REG_RESULT_ERR_INVAL_SUBS_ID";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_INVAL_PROFILE_FAMILY]
        = "DS_PROFILE_REG_INVAL_PROFILE_FAMILY";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_PROFILE_VERSION_MISMATCH]
        = "DS_PROFILE_REG_PROFILE_VERSION_MISMATCH";
    errorCodeToStringMap_[ErrorCode::REG_RESULT_ERR_OUT_OF_MEMORY] = "REG_RESULT_ERR_OUT_OF_MEMORY";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_FILE_ACCESS]
        = "DS_PROFILE_REG_RESULT_ERR_FILE_ACCESS";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_REG_RESULT_ERR_EOF]
        = "DS_PROFILE_REG_RESULT_ERR_EOF";
    errorCodeToStringMap_[ErrorCode::REG_RESULT_ERR_VALID_FLAG_NOT_SET]
        = "REG_RESULT_ERR_VALID_FLAG_NOT_SET";
    errorCodeToStringMap_[ErrorCode::REG_RESULT_ERR_OUT_OF_PROFILES]
        = "REG_RESULT_ERR_OUT_OF_PROFILES";
    errorCodeToStringMap_[ErrorCode::REG_RESULT_NO_EMERGENCY_PDN_SUPPORT]
        = "REG_RESULT_NO_EMERGENCY_PDN_SUPPORT";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_3GPP_INVAL_PROFILE_FAMILY]
        = "DS_PROFILE_3GPP_INVAL_PROFILE_FAMILY";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_3GPP_ACCESS_ERR] = "DS_PROFILE_3GPP_ACCESS_ERR";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_3GPP_CONTEXT_NOT_DEFINED]
        = "DS_PROFILE_3GPP_CONTEXT_NOT_DEFINED";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_3GPP_VALID_FLAG_NOT_SET]
        = "DS_PROFILE_3GPP_VALID_FLAG_NOT_SET";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_3GPP_READ_ONLY_FLAG_SET]
        = "DS_PROFILE_3GPP_READ_ONLY_FLAG_SET";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_3GPP_ERR_OUT_OF_PROFILES]
        = "DS_PROFILE_3GPP_ERR_OUT_OF_PROFILES";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_3GPP2_ERR_INVALID_IDENT_FOR_PROFILE]
        = "DS_PROFILE_3GPP2_ERR_INVALID_IDENT_FOR_PROFILE";
    errorCodeToStringMap_[ErrorCode::DS_PROFILE_3GPP2_ERR_OUT_OF_PROFILE]
        = "DS_PROFILE_3GPP2_ERR_OUT_OF_PROFILE";
    errorCodeToStringMap_[ErrorCode::V2X_ERR_EXCEED_MAX] = "V2X_ERR_EXCEED_MAX";
    errorCodeToStringMap_[ErrorCode::V2X_ERR_V2X_DISABLED] = "V2X_ERR_V2X_DISABLED";
    errorCodeToStringMap_[ErrorCode::V2X_ERR_UNKNOWN_SERVICE_ID] = "V2X_ERR_UNKNOWN_SERVICE_ID";
    errorCodeToStringMap_[ErrorCode::V2X_ERR_SRV_ID_L2_ADDRS_NOT_COMPATIBLE]
        = "V2X_ERR_SRV_ID_L2_ADDRS_NOT_COMPATIBLE";
    errorCodeToStringMap_[ErrorCode::V2X_ERR_PORT_UNAVAIL] = "V2X_ERR_PORT_UNAVAIL";
    errorCodeToStringMap_[ErrorCode::INCOMPATIBLE_STATE] = "INCOMPATIBLE_STATE";
    errorCodeToStringMap_[ErrorCode::SUBSYSTEM_UNAVAILABLE] = "SUBSYSTEM_UNAVAILABLE";
    errorCodeToStringMap_[ErrorCode::OPERATION_TIMEOUT] = "OPERATION_TIMEOUT";
    errorCodeToStringMap_[ErrorCode::ROLLBACK_FAILED] = "ROLLBACK_FAILED";
}

std::string Utils::getErrorCodeAsString(ErrorCode error) {
    initErrorCodeToStringMap();
    if(errorCodeToStringMap_.find(error) != std::end(errorCodeToStringMap_)) {
        return errorCodeToStringMap_[error];
    }
    LOG(DEBUG, __FUNCTION__, "Error: ", static_cast<int>(error), " not found");
    return "UNKNOWN_ERROR";
}

std::string Utils::getErrorCodeAsString(int error) {
    initErrorCodeToStringMap();
    return getErrorCodeAsString(static_cast<ErrorCode>(error));
}

} //namespace common
} //namespace telux