/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <sys/sysinfo.h>
#include <jsoncpp/json/json.h>
#include <telux/common/Version.hpp>

#include "CommonUtils.hpp"
#include "SimulationConfigParser.hpp"

namespace telux {

namespace common {
Status CommonUtils::mapStatus(std::string status) {
    if (status == "SUCCESS") {
        return Status::SUCCESS;
    } else if (status == "FAILED") {
        return Status::FAILED;
    } else if (status == "NOCONNECTION") {
        return Status::NOCONNECTION;
    } else if (status == "NOSUBSCRIPTION") {
        return Status::NOSUBSCRIPTION;
    } else if (status == "INVALIDPARAM") {
        return Status::INVALIDPARAM;
    } else if (status == "INVALIDSTATE") {
        return Status::INVALIDSTATE;
    } else if (status == "NOTREADY") {
        return Status::NOTREADY;
    } else if (status == "NOTALLOWED") {
        return Status::NOTALLOWED;
    } else if (status == "NOTIMPLEMENTED") {
        return Status::NOTIMPLEMENTED;
    } else if (status == "CONNECTIONLOST") {
        return Status::CONNECTIONLOST;
    } else if (status == "EXPIRED") {
        return Status::EXPIRED;
    } else if (status == "ALREADY") {
        return Status::ALREADY;
    } else if (status == "NOSUCH") {
        return Status::NOSUCH;
    } else if (status == "NOTSUPPORTED") {
        return Status::NOTSUPPORTED;
    } else if (status == "NOMEMORY") {
        return Status::NOMEMORY;
    }

    return Status::FAILED;
}

ErrorCode CommonUtils::mapErrorCode(std::string errorCode) {
    if (errorCode == "SUCCESS") {
        return ErrorCode::SUCCESS;
    } else if (errorCode == "RADIO_NOT_AVAILABLE") {
        return ErrorCode::RADIO_NOT_AVAILABLE;
    } else if (errorCode == "GENERIC_FAILURE") {
        return ErrorCode::GENERIC_FAILURE;
    } else if (errorCode == "PASSWORD_INCORRECT") {
        return ErrorCode::PASSWORD_INCORRECT;
    } else if (errorCode == "SIM_PIN2") {
        return ErrorCode::SIM_PIN2;
    } else if (errorCode == "SIM_PUK2") {
        return ErrorCode::SIM_PUK2;
    } else if (errorCode == "REQUEST_NOT_SUPPORTED") {
        return ErrorCode::REQUEST_NOT_SUPPORTED;
    } else if (errorCode == "CANCELLED") {
        return ErrorCode::CANCELLED;
    } else if (errorCode == "OP_NOT_ALLOWED_DURING_VOICE_CALL") {
        return ErrorCode::OP_NOT_ALLOWED_DURING_VOICE_CALL;
    } else if (errorCode == "OP_NOT_ALLOWED_BEFORE_REG_TO_NW") {
        return ErrorCode::OP_NOT_ALLOWED_BEFORE_REG_TO_NW;
    } else if (errorCode == "SMS_SEND_FAIL_RETRY") {
        return ErrorCode::SMS_SEND_FAIL_RETRY;
    } else if (errorCode == "SIM_ABSENT") {
        return ErrorCode::SIM_ABSENT;
    } else if (errorCode == "SUBSCRIPTION_NOT_AVAILABLE") {
        return ErrorCode::SUBSCRIPTION_NOT_AVAILABLE;
    } else if (errorCode == "MODE_NOT_SUPPORTED") {
        return ErrorCode::MODE_NOT_SUPPORTED;
    } else if (errorCode == "FDN_CHECK_FAILURE") {
        return ErrorCode::FDN_CHECK_FAILURE;
    } else if (errorCode == "ILLEGAL_SIM_OR_ME") {
        return ErrorCode::ILLEGAL_SIM_OR_ME;
    } else if (errorCode == "MISSING_RESOURCE") {
        return ErrorCode::MISSING_RESOURCE;
    } else if (errorCode == "NO_SUCH_ELEMENT") {
        return ErrorCode::NO_SUCH_ELEMENT;
    } else if (errorCode == "DIAL_MODIFIED_TO_USSD") {
        return ErrorCode::DIAL_MODIFIED_TO_USSD;
    } else if (errorCode == "DIAL_MODIFIED_TO_SS") {
        return ErrorCode::DIAL_MODIFIED_TO_SS;
    } else if (errorCode == "DIAL_MODIFIED_TO_DIAL") {
        return ErrorCode::DIAL_MODIFIED_TO_DIAL;
    } else if (errorCode == "USSD_MODIFIED_TO_DIAL") {
        return ErrorCode::USSD_MODIFIED_TO_DIAL;
    } else if (errorCode == "USSD_MODIFIED_TO_SS") {
        return ErrorCode::USSD_MODIFIED_TO_SS;
    } else if (errorCode == "USSD_MODIFIED_TO_USSD") {
        return ErrorCode::USSD_MODIFIED_TO_USSD;
    } else if (errorCode == "SS_MODIFIED_TO_DIAL") {
        return ErrorCode::SS_MODIFIED_TO_DIAL;
    } else if (errorCode == "SS_MODIFIED_TO_USSD") {
        return ErrorCode::SS_MODIFIED_TO_USSD;
    } else if (errorCode == "SUBSCRIPTION_NOT_SUPPORTED") {
        return ErrorCode::SUBSCRIPTION_NOT_SUPPORTED;
    } else if (errorCode == "SS_MODIFIED_TO_SS") {
        return ErrorCode::SS_MODIFIED_TO_SS;
    } else if (errorCode == "LCE_NOT_SUPPORTED") {
        return ErrorCode::LCE_NOT_SUPPORTED;
    } else if (errorCode == "NO_MEMORY") {
        return ErrorCode::NO_MEMORY;
    } else if (errorCode == "INTERNAL_ERR") {
        return ErrorCode::INTERNAL_ERR;
    } else if (errorCode == "SYSTEM_ERR") {
        return ErrorCode::SYSTEM_ERR;
    } else if (errorCode == "MODEM_ERR") {
        return ErrorCode::MODEM_ERR;
    } else if (errorCode == "INVALID_STATE") {
        return ErrorCode::INVALID_STATE;
    } else if (errorCode == "NO_RESOURCES") {
        return ErrorCode::NO_RESOURCES;
    } else if (errorCode == "SIM_ERR") {
        return ErrorCode::SIM_ERR;
    } else if (errorCode == "INVALID_ARGUMENTS") {
        return ErrorCode::INVALID_ARGUMENTS;
    } else if (errorCode == "INVALID_SIM_STATE") {
        return ErrorCode::INVALID_SIM_STATE;
    } else if (errorCode == "INVALID_MODEM_STATE") {
        return ErrorCode::INVALID_MODEM_STATE;
    } else if (errorCode == "INVALID_CALL_ID") {
        return ErrorCode::INVALID_CALL_ID;
    } else if (errorCode == "NO_SMS_TO_ACK") {
        return ErrorCode::NO_SMS_TO_ACK;
    } else if (errorCode == "NETWORK_ERR") {
        return ErrorCode::NETWORK_ERR;
    } else if (errorCode == "REQUEST_RATE_LIMITED") {
        return ErrorCode::REQUEST_RATE_LIMITED;
    } else if (errorCode == "SIM_BUSY") {
        return ErrorCode::SIM_BUSY;
    } else if (errorCode == "SIM_FULL") {
        return ErrorCode::SIM_FULL;
    } else if (errorCode == "NETWORK_REJECT") {
        return ErrorCode::NETWORK_REJECT;
    } else if (errorCode == "OPERATION_NOT_ALLOWED") {
        return ErrorCode::OPERATION_NOT_ALLOWED;
    } else if (errorCode == "EMPTY_RECORD") {
        return ErrorCode::EMPTY_RECORD;
    } else if (errorCode == "INVALID_SMS_FORMAT") {
        return ErrorCode::INVALID_SMS_FORMAT;
    } else if (errorCode == "ENCODING_ERR") {
        return ErrorCode::ENCODING_ERR;
    } else if (errorCode == "INVALID_SMSC_ADDRESS") {
        return ErrorCode::INVALID_SMSC_ADDRESS;
    } else if (errorCode == "NO_SUCH_ENTRY") {
        return ErrorCode::NO_SUCH_ENTRY;
    } else if (errorCode == "NETWORK_NOT_READY") {
        return ErrorCode::NETWORK_NOT_READY;
    } else if (errorCode == "NOT_PROVISIONED") {
        return ErrorCode::NOT_PROVISIONED;
    } else if (errorCode == "NO_SUBSCRIPTION") {
        return ErrorCode::NO_SUBSCRIPTION;
    } else if (errorCode == "NO_NETWORK_FOUND") {
        return ErrorCode::NO_NETWORK_FOUND;
    } else if (errorCode == "DEVICE_IN_USE") {
        return ErrorCode::DEVICE_IN_USE;
    } else if (errorCode == "ABORTED") {
        return ErrorCode::ABORTED;
    } else if (errorCode == "INCOMPATIBLE_STATE") {
        return ErrorCode::INCOMPATIBLE_STATE;
    } else if (errorCode == "NO_EFFECT") {
        return ErrorCode::NO_EFFECT;
    } else if (errorCode == "DEVICE_NOT_READY") {
        return ErrorCode::DEVICE_NOT_READY;
    } else if (errorCode == "MISSING_ARGUMENTS") {
        return ErrorCode::MISSING_ARGUMENTS;
    } else if (errorCode == "PIN_PERM_BLOCKED") {
        return ErrorCode::PIN_PERM_BLOCKED;
    } else if (errorCode == "PIN_BLOCKED") {
        return ErrorCode::PIN_BLOCKED;
    } else if (errorCode == "MALFORMED_MSG") {
        return ErrorCode::MALFORMED_MSG;
    } else if (errorCode == "INTERNAL") {
        return ErrorCode::INTERNAL;
    } else if (errorCode == "CLIENT_IDS_EXHAUSTED") {
        return ErrorCode::CLIENT_IDS_EXHAUSTED;
    } else if (errorCode == "NOTSUPPORTED"){
        return ErrorCode::NOT_SUPPORTED;
    } else if (errorCode == "INFO_UNAVAILABLE"){
        return ErrorCode::INFO_UNAVAILABLE;
    }

    return ErrorCode::INTERNAL_ERR;
}

ErrorCode CommonUtils::toErrorCode(Status status) {

    switch (status) {
        case Status::SUCCESS:
            return ErrorCode::SUCCESS;
        case Status::FAILED:
            return ErrorCode::GENERIC_FAILURE;
        case Status::NOCONNECTION:
        case Status::INVALIDSTATE:
        case Status::NOTREADY:
        case Status::CONNECTIONLOST:
            return ErrorCode::INVALID_STATE;
        case Status::NOSUBSCRIPTION:
            return ErrorCode::NO_SUBSCRIPTION;
        case Status::INVALIDPARAM:
        case Status::ALREADY:
            return ErrorCode::INVALID_ARGUMENTS;
        case Status::NOTALLOWED:
            return ErrorCode::OPERATION_NOT_ALLOWED;
        case Status::NOTIMPLEMENTED:
        case Status::NOTSUPPORTED:
            return ErrorCode::NOT_SUPPORTED;
        case Status::EXPIRED:
        case Status::NOSUCH:
            return ErrorCode::NO_SUCH_ENTRY;
        case Status::NOMEMORY:
            return ErrorCode::NO_MEMORY;
        default:
            return ErrorCode::GENERIC_FAILURE;
    }
}

std::string CommonUtils::readSystemDataValue(
    Json::Value &jsonValue, std::string defaultValue, std::vector<std::string> &path) {
    std::string value = defaultValue;
    try {
        std::string p = path.front();
        path.erase(path.begin());
        LOG(DEBUG, p, "---", jsonValue);
        if (path.size() > 0 && jsonValue.isObject()) {
            return readSystemDataValue(jsonValue[p], defaultValue, path);
        }
        if (path.size() == 0 && jsonValue.isObject()) {
            value = jsonValue[p].asString();
            if (value == "") {
                value = defaultValue;
            }
        }
    } catch (std::exception &ex) {
        value = defaultValue;
    }
    LOG(DEBUG, "Returning ", value);
    return value;
}

std::string CommonUtils::readSystemDataValue(
    std::string subsystem, std::string defaultValue, std::vector<std::string> path) {
    Json::Value jsonValue;
    std::string value;
    ErrorCode err = JsonParser::readFromJsonFile(jsonValue, "system-state/" + subsystem + ".json");
    if (err != ErrorCode::SUCCESS) {
        LOG(ERROR, "Unable to open file for ", subsystem, ". Return default value: ", defaultValue);
        value = defaultValue;
    } else {
        value = readSystemDataValue(jsonValue, defaultValue, path);
    }
    LOG(DEBUG, "Read ", value, " in ", __FUNCTION__);
    return value;
}

ErrorCode CommonUtils::readJsonData(std::string apiJsonPath, std::string stateJsonPath,
    std::string subsystem, std::string method, JsonData& data) {
    LOG(DEBUG, __FUNCTION__);
    ErrorCode err =
        JsonParser::readFromJsonFile(data.apiRootObj, apiJsonPath);
    if (err != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return err;
    }
    CommonUtils::getValues(data.apiRootObj, subsystem, method, data.status,
        data.error, data.cbDelay );

    if (data.status == telux::common::Status::SUCCESS ||
        data.error == telux::common::ErrorCode::SUCCESS) {
        err =
            JsonParser::readFromJsonFile(data.stateRootObj, stateJsonPath);
        if (err != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
            return err;
        }
    }

    return err;
}

void CommonUtils::getValues(Json::Value &values, std::string subsystem,
    std::string method, telux::common::Status &status,
    telux::common::ErrorCode &errorCode, int &cbDelay) {
    std::string statusStr = values[subsystem][method]["status"].asString();
    status = mapStatus(statusStr);

    std::string errorStr = values[subsystem][method]["error"].asString();
    errorCode = mapErrorCode(errorStr);
    cbDelay = values[subsystem][method]["callbackDelay"].asInt();
    if(cbDelay == 0) {
        cbDelay = values[subsystem]["DefaultCallbackDelay"].asInt();
    }
}

telux::common::ServiceStatus CommonUtils::mapServiceStatus(std::string status) {

    if (status == "SERVICE_FAILED") {
        return telux::common::ServiceStatus::SERVICE_FAILED;
    } else if (status == "SERVICE_UNAVAILABLE") {
        return telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (status == "SERVICE_AVAILABLE") {
        return telux::common::ServiceStatus::SERVICE_AVAILABLE;
    } else {
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }
}

std::string CommonUtils::mapServiceString(telux::common::ServiceStatus srvStatus) {
    if (srvStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        return "SERVICE_UNAVAILABLE";
    } else if (srvStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        return "SERVICE_AVAILABLE";
    } else {
        return "SERVICE_FAILED";
    }
}

std::vector<std::string> CommonUtils::splitString(const std::string &s, char delim) {
    std::vector<std::string> elements;
    std::istringstream ss(s);
    std::string token;

    while(std::getline(ss, token, delim)) {
        // remove the trailing spaces
        token.erase(std::remove(token.begin(),token.end(),' '),token.end());
        if(!token.empty()) {
            elements.push_back(token);
        }
    }
    return elements;
}

std::string CommonUtils::getCurrentTimeHHMMSS() {
    using namespace std::chrono;
    auto ms =
        std::chrono::duration_cast<milliseconds>(
            high_resolution_clock::now().time_since_epoch());
    ms %= 1000;
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);

    std::stringstream nowSs;
    nowSs << std::put_time(&tm, "%H%M%S") << '.' << ms.count()/10;

    return nowSs.str();
}

void CommonUtils::calculateBootTimeStamp(uint64_t &timestamp) {
    // Get the current time in nanoseconds
    auto now = std::chrono::system_clock::now();
    auto nowNs = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
    auto epoch = nowNs.time_since_epoch();
    auto nowNsCount = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch).count();

    // Get the system uptime in seconds
    struct sysinfo info;
    sysinfo(&info);
    auto uptimeNs = std::chrono::nanoseconds(info.uptime * 1000000000LL);

    // Calculate the boot time in nanoseconds since epoch
    timestamp = nowNsCount - uptimeNs.count();
}

int CommonUtils::bitwiseXOR(const std::string& str) {
    if (str.size() <= 0) {
        return 0;
    }
    int res = static_cast<int>(str[0]);

    for (size_t i = 1; i < str.size(); ++i) {
        res ^= static_cast<int>(str[i]);
    }

    return res;
}

std::string CommonUtils::convertVectorToString(std::vector<std::uint8_t> bytes, bool toHex) {
    std::stringstream ss;
    for (std::size_t i = 0; i < bytes.size(); i++)
    {
        if(toHex) {
            ss << std::hex << static_cast<int>(bytes[i]);
        } else {
            ss << static_cast<int>(bytes[i]) << " ";
        }
    }
    return ss.str();
}

std::vector<int> CommonUtils::convertStringToVector(std::string input) {
    std::stringstream iss( input );
    int parsednum;
    std::vector<int> myNumbers;
    while ( iss >> parsednum ) {
        myNumbers.push_back( parsednum );
    }
    return myNumbers;
}

std::string CommonUtils::getGrpcPort() {
    auto config = std::make_shared<SimulationConfigParser>();
    return ("localhost:" + config->getValue("RPC_PORT"));
}

std::vector<std::string> CommonUtils::splitString(std::string msg) {
    std::stringstream ss(msg);
    std::vector<std::string> message;
    while(ss.good()) {
        std::string str;
        getline(ss, str, ',');
        message.push_back(str);
    }
    return message;
}

long CommonUtils::convertHexToInt(std::string hex) {
    long value = 0;
    if (hex.rfind("0x", 0) == 0) {
        value = std::stol(hex, nullptr, 0);
    } else {
        value = std::stol(hex, nullptr, 16);
    }
    return value;
}

std::string CommonUtils::convertIntVectorToString(std::vector<int> integers) {
  std::stringstream ss;
  for (std::size_t i = 0; i < integers.size(); i++)
  {
          ss << integers[i] << " ";
  }
  return ss.str();
}

void CommonUtils::logSdkVersion() {
    auto sdkVersion            = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    LOG(INFO, "\n Telematics SDK - Release Name: ", telux::common::Version::getReleaseName(),
        "\n SDK Major Version Major: ", sdkVersion.major, ", Minor: ", sdkVersion.minor,
        ", Patch: ", sdkVersion.patch, "\nRelease name: ", sdkReleaseName);
}

}  // namespace common
}  // namespace telux
