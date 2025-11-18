/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef COMMONUTILS_HPP
#define COMMONUTILS_HPP

#include <telux/common/CommonDefines.hpp>
#include <grpcpp/grpcpp.h>
#include <vector>

#include "JsonParser.hpp"
#include "Logger.hpp"

using grpc::Channel;

#define handleApiResponseForMethod(subSystem, manager)                                       \
    telux::common::Status status = Status::FAILED;                                           \
    telux::common::ErrorCode errorCode = ErrorCode::GENERIC_FAILURE;                         \
    int cbDelay = 100;                                                                  \
    Json::Value rootNode;                                                                    \
    do {                                                                                     \
        ErrorCode err                                                                        \
            = JsonParser::readFromJsonFile(rootNode, "api/" subSystem "/" manager ".json");  \
        if (err != ErrorCode::SUCCESS) {                                                     \
            LOG(ERROR, "Unable to read file: " subSystem "/" manager);                       \
            status = Status::FAILED;                                                         \
            errorCode = ErrorCode::GENERIC_FAILURE;                                          \
            break;                                                                           \
        }                                                                                    \
        CommonUtils::getValues(rootNode, manager, __FUNCTION__, status, errorCode, cbDelay); \
    } while (0);                                                                             \
    if (status != Status::SUCCESS) {                                                         \
        LOG(ERROR, subSystem "/" manager "::", __FUNCTION__,                                 \
            " failed: ", static_cast<int>(status));                                          \
        return status;                                                                       \
    }
namespace telux {

namespace common {

struct JsonData {
    Json::Value apiRootObj;
    Json::Value stateRootObj;
    telux::common::Status status;
    telux::common::ErrorCode error;
    int cbDelay;
};

/**
 * This is a utility class to enable shared_from_this() in case when both
 * base class as well as child class wants to use shared_from_this separately
 *
 * For enabling shared_from_this use enable_inheritable_shared_from_this<BaseClass>
 * base class function    - shared_from_this().
 * Use the below function in derived class instead of shared_from_this()
 * derived class function - downcasted_shared_from_this<DerivedClass>().
 *
 */
class SharedFromThis : public std::enable_shared_from_this
        <SharedFromThis> {
public:
    virtual ~SharedFromThis() {
    }
};

template <class T>
class enable_inheritable_shared_from_this : virtual public SharedFromThis {
public:
    std::shared_ptr<T> shared_from_this () {
        return std::dynamic_pointer_cast<T>(SharedFromThis::shared_from_this());
    }

    template <class Down>
    std::shared_ptr<Down> downcasted_shared_from_this() {
        return std::dynamic_pointer_cast<Down>(SharedFromThis::shared_from_this());
    }
};

class CommonUtils {
 public:
    static telux::common::Status mapStatus(std::string status);
    static telux::common::ErrorCode mapErrorCode(std::string errorCode);
    static telux::common::ErrorCode toErrorCode(telux::common::Status status);
    static std::vector<std::string> splitString(const std::string &str, char delimiter);
    static std::string getCurrentTimeHHMMSS();
    static void calculateBootTimeStamp(uint64_t &timestamp);
    static int bitwiseXOR(const std::string& str);
    /* convert hexadecimal value to decimal */
    static long convertHexToInt(std::string hex);

    static void getValues(Json::Value &values, std::string subsystem,
        std::string method, telux::common::Status &status,
        telux::common::ErrorCode &errorCode, int &cbDelay);
    static telux::common::ServiceStatus mapServiceStatus(std::string status);
    static std::string mapServiceString(telux::common::ServiceStatus srvStatus);
    static std::string readSystemDataValue(
        std::string subsystem, std::string defaultValue, std::vector<std::string> path);
    static std::string convertVectorToString(std::vector<std::uint8_t> bytes, bool toHex);
    static std::vector<int> convertStringToVector(std::string input);
    static std::string getGrpcPort();

    /**
     * Print the SDK version in the predefined format to SDK log
     */
    static void logSdkVersion();

    template<typename T>
    static std::unique_ptr<typename T::Stub> getGrpcStub() {
        return T::NewStub(grpc::CreateChannel(CommonUtils::getGrpcPort(),
            grpc::InsecureChannelCredentials()));
    }

    template<typename T>
    static void updateJsonValue(const std::string& filePath, const std::string& subsystem,
        const std::string& method, const std::string& attribute, T val) {
        Json::Value rootObj;
        ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ");
            LOG(ERROR, __FUNCTION__, " filePath::", filePath);
            LOG(ERROR, __FUNCTION__, " subsystem::", subsystem,
                " method::", method, " attribute::", attribute, " val::", val);
        }

        rootObj[subsystem][method][attribute] = val;
        JsonParser::writeToJsonFile(rootObj, filePath);
    }

    template <typename T>
    static ErrorCode writeSystemDataValue(
        std::string subsystem, T value, std::vector<std::string> path) {
        Json::Value root;
        ErrorCode err = ErrorCode::GENERIC_FAILURE;
        JsonParser::readFromJsonFile(root, "system-state/" + subsystem + ".json");
        if (path.size() > 0) {
            std::string attr = path.front();
            path.erase(path.begin());
            writeSystemDataValue(root[attr], value, path);
            err = JsonParser::writeToJsonFile(root, "system-state/" + subsystem + ".json");
        }
        return err;
    }

    static ErrorCode readJsonData(std::string apiJsonPath, std::string stateJsonPath,
        std::string subsystem, std::string method, JsonData& data);

    static std::vector<std::string> splitString(std::string str);
    static std::string convertIntVectorToString(std::vector<int> integers);
 private:
    static std::string readSystemDataValue(
        Json::Value &jsonValue, std::string defaultValue, std::vector<std::string> &path);
    template <typename T>
    static void writeSystemDataValue(
        Json::Value &node, T value, std::vector<std::string> &path) {
        try {
            std::string p = path.front();
            path.erase(path.begin());
            if (path.size() > 0) {
                writeSystemDataValue(node[p], value, path);
            } else {
                node[p] = value;
            }
        } catch (std::exception &ex) {
            LOG(DEBUG, ex.what());
        }
    }
};

}  // namespace common
}  // namespace telux
#endif  // COMMONUTILS_HPP
