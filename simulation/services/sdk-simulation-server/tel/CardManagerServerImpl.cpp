/*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*
*     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
* GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
* HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CardManagerServerImpl.hpp"
#include "libs/tel/TelDefinesStub.hpp"
#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/CardDefines.hpp>

#define JSON_PATH1 "system-state/tel/ICardManagerStateSlot1.json"
#define JSON_PATH2 "system-state/tel/ICardManagerStateSlot2.json"
#define JSON_PATH3 "api/tel/ICardManagerSlot1.json"
#define JSON_PATH4 "api/tel/ICardManagerSlot2.json"

#define CARD_EVENT "cardInfoChanged"
#define SIM_REFRESH_EVENT "simRefresh"
#define SLOT_1 1
#define SLOT_2 2
#define REFRESH_USER_ALLOW_TIMEOUT_MS (1000*10)
#define REFRESH_USER_COMPLETE_TIMEOUT_MS (1000*120)

#define GETVALUE_VIA_STR(str,value) \
do { \
    std::string strToken = EventParserUtil::getNextToken(str, DEFAULT_DELIMITER); \
    if (not strToken.empty()) { \
        try { \
            value = std::stoi(strToken); \
        } catch (exception const &ex) { \
            LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what()); \
            return false; \
        } \
    } else { \
        LOG(DEBUG, __FUNCTION__, " strToken is empty."); \
        return false; \
   } \
} while (0);

CardManagerServerImpl::CardManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    readJson();
}

CardManagerServerImpl::~CardManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    exit_ = true;
    cv_.notify_all();
}

grpc::Status CardManagerServerImpl::readJson() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObjSystemStateSlot1_, JSON_PATH1);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH1 );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    error = JsonParser::readFromJsonFile(rootObjSystemStateSlot2_, JSON_PATH2);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH2 );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    error = JsonParser::readFromJsonFile(rootObjApiResponseSlot1_, JSON_PATH3);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH3 );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    error = JsonParser::readFromJsonFile(rootObjApiResponseSlot2_, JSON_PATH4);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH4 );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    jsonObjSystemStateSlot_[SLOT_1] = rootObjSystemStateSlot1_;
    jsonObjSystemStateSlot_[SLOT_2] = rootObjSystemStateSlot2_;
    jsonObjSystemStateFileName_[SLOT_1] = JSON_PATH1;
    jsonObjSystemStateFileName_[SLOT_2] = JSON_PATH2;
    //Api response
    jsonObjApiResponseSlot_[SLOT_1] = rootObjApiResponseSlot1_;
    jsonObjApiResponseSlot_[SLOT_2] = rootObjApiResponseSlot2_;
    jsonObjApiResponseFileName_[SLOT_1] = JSON_PATH3;
    jsonObjApiResponseFileName_[SLOT_2] = JSON_PATH4;
    return grpc::Status::OK;
}

void CardManagerServerImpl::getJsonForSystemData(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjSystemStateFileName_[phoneId];
    rootObj = jsonObjSystemStateSlot_[phoneId];
}

void CardManagerServerImpl::getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjApiResponseFileName_[phoneId];
    rootObj = jsonObjApiResponseSlot_[phoneId];
}

void CardManagerServerImpl::getApiConfigureFromJson(const int slotId, const std::string apiname,
    telux::common::Status& status, telux::common::ErrorCode& ec, int& delay) {
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    if(readJson().ok()) {
        getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, rootObj);

        CommonUtils::getValues(rootObj, "ICardManager", apiname, status, ec, delay);
    }
}

bool CardManagerServerImpl::isCallbackNeeded(Json::Value rootObj, std::string apiname) {
    int value = rootObj["ICardManager"][apiname]["callbackDelay"].asInt();
    bool isCallback = true;
    if (value == -1) {
        isCallback = false;
    }
    return isCallback;
}

bool CardManagerServerImpl::findAppId(Json::Value rootObj, const char* appid, int& index) {
    int sizeofADF = rootObj["ICardManager"]["EFs"]["ADF"].size();
    LOG(DEBUG, __FUNCTION__,"Size of ADF is", sizeofADF);
    bool foundAppId = false;
    for (index =0 ; index < sizeofADF ; index++) {
        const char* tmp = (rootObj["ICardManager"]["EFs"]["ADF"]\
            [index]["AppId"].asString()).c_str();
        std::string val = rootObj["ICardManager"]["EFs"]["ADF"]\
            [index]["AppId"].asString();
        LOG(DEBUG, __FUNCTION__,"appid is orignal from json  ", val);
        if(strcmp(tmp, appid) == 0) {
            foundAppId = true;
            std::string t = rootObj["ICardManager"]["EFs"]["ADF"][index]["AppId"].asString();
            LOG(DEBUG, __FUNCTION__,"appid is ", t);
            break;
        }
    }
    return foundAppId;
}

grpc::Status CardManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request,
    commonStub::GetServiceStatusReply* response) {

    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        rootObj = jsonObjApiResponseSlot_[SLOT_1];
        int cbDelay = rootObj["ICardManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus =
            rootObj["ICardManager"]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

        if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::vector<std::string> filters = {telux::tel::TEL_CARD_FILTER};
            auto &serverEventManager = ServerEventManager::getInstance();
            serverEventManager.registerListener(shared_from_this(), filters);
        }
        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::GetServiceStatus(ServerContext* context,
    const google::protobuf::Empty* request,
    commonStub::GetServiceStatusReply* response) {

    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        rootObj = jsonObjApiResponseSlot_[SLOT_1];
        std::string srvStatus = rootObj["ICardManager"]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::IsSubsystemReady(ServerContext* context,
    const google::protobuf::Empty* request,
    commonStub::IsSubsystemReadyReply* response) {

    bool status = false;
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        rootObj = jsonObjApiResponseSlot_[SLOT_1];
        std::string IsSubsystemReady = rootObj["ICardManager"]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus servstatus = CommonUtils::mapServiceStatus(IsSubsystemReady);
        if(servstatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            status = true;
        }
        response->set_is_ready(status);
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::GetCardState(ServerContext* context,
    const ::telStub::GetCardStateRequest* request,
    telStub::GetCardStateReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    telux::tel::CardState cardState;
    std::string jsonfilename = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        int state = rootObj["ICardManager"]["getState"]["cardState"].asInt();
        cardState = static_cast<telux::tel::CardState>(state);
        // Create response
        switch(cardState) {
            case telux::tel::CardState::CARDSTATE_UNKNOWN:
                response->set_card_state(telStub::CardState::CARDSTATE_UNKNOWN);
                break;
            case telux::tel::CardState::CARDSTATE_ABSENT:
                response->set_card_state(telStub::CardState::CARDSTATE_ABSENT);
                break;
            case telux::tel::CardState::CARDSTATE_PRESENT:
                response->set_card_state(telStub::CardState::CARDSTATE_PRESENT);
                break;
            case telux::tel::CardState::CARDSTATE_ERROR:
                response->set_card_state(telStub::CardState::CARDSTATE_ERROR);
                break;
            default:
                response->set_card_state(telStub::CardState::CARDSTATE_ERROR);
                break;
        }
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::ReadEFLinearFixed(ServerContext* context,
    const ::telStub::ReadEFLinearFixedRequest* request,
    telStub::ReadEFLinearFixedReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int slotId = request->slot_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(slotId, jsonfilename, rootObj);
        //Api response
        std::string jsonObjApiResponseFileName = "";
        Json::Value jsonObjApiResponse;
        getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);

        std::string filepath = request->file_path();
        uint16_t fileId  = request->file_id();
        int recordNum = request->record_number();
        std::string aid = request->aid();
        const char* appid = aid.c_str();
        telux::tel::IccResult result;
        telux::common::ErrorCode error;
        telux::common::Status status;
        commonStub::ErrorCode tmp;
        int cbDelay;
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", "readEFLinearFixed", status,
            error, cbDelay );
        int index;
        int i =0;
        if(status == telux::common::Status::SUCCESS) {
            bool foundAppId = findAppId(rootObj, appid, index);
            if(foundAppId) {
                int size = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                    ["LinearFixedEFFiles"].size();
                LOG(DEBUG, __FUNCTION__,"LinearFixedEFfiles size ", size);
                tmp = findmatchingrecordADF<telStub::ReadEFLinearFixedReply*>(rootObj, response,
                    size, index, recordNum, fileId, i);
                if(tmp == commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
                    result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                                [i+recordNum]["sw1"].asInt();
                    result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                                [i+recordNum]["sw2"].asInt();
                    result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["LinearFixedEFFiles"][i+recordNum]["payload"].asString();
                    std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["LinearFixedEFFiles"][i+recordNum]["data"].asString();
                    result.data = CommonUtils::convertStringToVector(input);
                } else {
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            } else {
                int size = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"].size();
                tmp = findmatchingrecordDF(rootObj, size, recordNum , fileId, i);
                if (tmp == commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
                    result.sw1 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                                [i+recordNum]["sw1"].asInt();
                    result.sw2 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                                [i+recordNum]["sw2"].asInt();
                    result.payload = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                                [i+recordNum]["payload"].asString();
                    std::string input = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                                [i+recordNum]["data"].asString();
                    result.data = CommonUtils::convertStringToVector(input);
                } else {
                    LOG(DEBUG, __FUNCTION__, "Valid AppId not found");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            }
        }
        // Create response
        telStub::IccResult requestedRecord;
        std::string apiname = "readEFLinearFixed";
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        response->set_status(static_cast<commonStub::Status>(status));
        if(error == telux::common::ErrorCode::SUCCESS) {
            requestedRecord.set_sw1(result.sw1);
            requestedRecord.set_sw2(result.sw2);
            requestedRecord.set_pay_load(result.payload);
            for(auto &it : result.data) {
                requestedRecord.add_data(it);
            }
        } else {
            std::string s = "";
            requestedRecord.set_sw1(0);
            requestedRecord.set_sw2(0);
            requestedRecord.set_pay_load(s);
            for(auto &it : result.data) {
                requestedRecord.add_data(it);
            }
        }
        *response->mutable_result() = requestedRecord;
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::WriteEFLinearFixed(ServerContext* context,
    const ::telStub::WriteEFLinearFixedRequest* request,
    telStub::WriteEFLinearFixedReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->slot_id();
    std::string jsonfilename = "";
    std::string jsonObjApiResponseFileName = "";
    Json::Value rootObj;
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(slotId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
        std::string filepath = request->file_path();
        uint16_t fileId  = request->file_id();
        std::string aid = request->aid();
        const char* appid = aid.c_str();
        telux::tel::IccResult result;
        int recordsize = request->record_number();
        std::vector<uint8_t> data;
        telux::common::Status status;
        int cbDelay;
        commonStub::ErrorCode tmp;
        telux::common::ErrorCode error;
        for(int d : request->data()) {
            data.emplace_back(d);
        }
        int s = data.size();
        for (int i = 0; i < s; i++) {
            LOG(DEBUG, __FUNCTION__,"data recieved from request", static_cast<int>(data.at(i)));
        }
        std::string str1 = "";
        int i = 0;
        int index =0;
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", "writeEFLinearFixed", status,
            error, cbDelay );
        if(status == telux::common::Status::SUCCESS) {
            bool foundAppId = findAppId(rootObj, appid, index);
            if(foundAppId) {
                int size = rootObj["ICardManager"]["EFs"]["ADF"]\
                    [index]["LinearFixedEFFiles"].size();
                LOG(DEBUG, __FUNCTION__,"LinearFixedEFfiles size ", size);
                tmp = findmatchingrecordADF<telStub::WriteEFLinearFixedReply*>(rootObj,
                    response, size, index, recordsize, fileId, i);
                if(tmp == commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
                    str1 = CommonUtils::convertVectorToString(data, false);
                    LOG(DEBUG, __FUNCTION__,"String value is", str1);
                    rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i+recordsize]\
                        ["data"] = str1;
                    LOG(DEBUG, __FUNCTION__,"String is data  ", str1);
                    str1 = CommonUtils::convertVectorToString(data, true);
                    LOG(DEBUG, __FUNCTION__,"String is payload ", str1);
                    rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i+recordsize]\
                        ["payload"] = str1;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[slotId] = rootObj;
                    result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                        [i+recordsize]["sw1"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                    result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                        [i+recordsize]["sw2"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                } else {
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            } else {
                int i = 0;
                int size = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"].size();
                tmp = findmatchingrecordDF(rootObj, size, recordsize , fileId, i);
                if (tmp == commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
                    str1 = CommonUtils::convertVectorToString(data, false);
                    rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i+recordsize]\
                        ["data"] = str1;
                    LOG(DEBUG, __FUNCTION__,"String is  ", str1);
                    str1 = CommonUtils::convertVectorToString(data, true);
                    rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i+recordsize]\
                        ["payload"] = str1;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[slotId] = rootObj;
                    result.sw1 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                        [i+recordsize]["sw1"].asInt();
                    result.sw2 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                        [i+recordsize]["sw2"].asInt();
                } else {
                    LOG(DEBUG, __FUNCTION__, "Valid AppId not found");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            }
        }
        //Create response
        std::string apiname = "writeEFLinearFixed";
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        LOG(DEBUG, __FUNCTION__, "STatus is", static_cast<int>(status));
        response->set_status(static_cast<commonStub::Status>(status));

        telStub::IccResult requestedRecord;

        if(error == telux::common::ErrorCode::SUCCESS) {
            std::string s = "";
            requestedRecord.set_sw1(result.sw1);
            requestedRecord.set_sw2(result.sw2);
            requestedRecord.set_pay_load(s);
        } else {
            std::string s = "";
            requestedRecord.set_sw1(0);
            requestedRecord.set_sw2(0);
            requestedRecord.set_pay_load(s);
        }
        *response->mutable_result() = requestedRecord;
    }
    return readStatus;
    }

    grpc::Status CardManagerServerImpl::ReadEFLinearFixedAll(ServerContext* context,
    const ::telStub::ReadEFLinearFixedAllRequest* request,
    telStub::ReadEFLinearFixedAllReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int slotId = request->slot_id();
    std::string jsonfilename = "";
    std::string apiname = "readEFLinearFixedAll";
    Json::Value rootObj;
    Json::Value jsonObjApiResponse;
    std::string jsonObjApiResponseFileName = "";
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(slotId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
        std::string filepath = request->file_path();
        uint16_t fileId  = request->file_id();
        std::string aid = request->aid();
        const char* appid = aid.c_str();
        std::vector<telux::tel::IccResult> records;
        commonStub::ErrorCode tmp;
        int cbDelay;
        telux::common::Status status;
        telux::common::ErrorCode error;
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            error, cbDelay );

        int index;
        if(status == telux::common::Status::SUCCESS) {
            bool foundAppId = findAppId(rootObj, appid, index);
            if(foundAppId) {
                int i = 0;
                int size = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                    ["LinearFixedEFFiles"].size();
                LOG(DEBUG, __FUNCTION__,"LinearFixedEFfiles size ", size);
                while (i < size ) {
                    uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["LinearFixedEFFiles"][i]["fileId"].asInt();
                    if (tmpfileId == fileId) {
                        int num = rootObj["ICardManager"]["EFs"]["ADF"]\
                            [index]["LinearFixedEFFiles"][i]["numberOfRecords"].asInt();
                        LOG(DEBUG, __FUNCTION__,"NumberOfRecords ", num);
                        for(int j = 1; j <= num ; j++) {
                            telux::tel::IccResult result;
                            result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["LinearFixedEFFiles"][i+j]["sw1"].asInt();
                            LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                            result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["LinearFixedEFFiles"][i+j]["sw2"].asInt();
                            LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                            result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["LinearFixedEFFiles"][i+j]["payload"].asString();
                            LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                            std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["LinearFixedEFFiles"][i+j]["data"].asString();
                            result.data = CommonUtils::convertStringToVector(input);
                            records.emplace_back(result);
                        }
                        break;
                    } else {
                        LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
                        int num = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i]["numberOfRecords"].asInt();
                        i = i + num + 1;
                        LOG(DEBUG, __FUNCTION__,"Incremented value is ", i );
                    }
                }
                if(i == size) {
                    LOG(DEBUG, __FUNCTION__,"Valid record not found ", i );
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            } else {
                int size = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"].size();
                int i =0;
                int recordnum = 0;
                tmp = findmatchingrecordDF(rootObj, size, recordnum , fileId, i);
                if (tmp == commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
                    int num = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i]\
                        ["numberOfRecords"].asInt();
                    LOG(DEBUG, __FUNCTION__,"NumberOfRecords ", num);
                    for(int j = 1; j <= num ; j++) {
                        telux::tel::IccResult result;
                        result.sw1 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                            [j]["sw1"].asInt();
                        result.sw2 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][j]\
                            ["sw2"].asInt();
                        result.payload = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                            [j]["payload"].asString();
                        std::string input = rootObj["ICardManager"]["EFs"]\
                            ["DFLinearFixedEFRecords"][j]["data"].asString();
                        result.data = CommonUtils::convertStringToVector(input);
                        records.emplace_back(result);
                    }
                } else {
                    LOG(DEBUG, __FUNCTION__,"Valid fileId not found ");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            }
        }

        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_iscallback(iscallback);
        response->set_delay(cbDelay);
        response->set_status(static_cast<commonStub::Status>(status));

        for(auto &it : records) {
            telStub::IccResult *result = response->add_records();
            if(error == telux::common::ErrorCode::SUCCESS) {
                result->set_sw1(it.sw1);
                result->set_sw2(it.sw2);
                result->set_pay_load(it.payload);
                for (auto &r : it.data) {
                    result->add_data(r);
                }
            } else {
                std::string s = "";
                result->set_sw1(0);
                result->set_sw2(0);
                result->set_pay_load(s);
                for (auto &r : it.data) {
                    result->add_data(r);
                }
            }
        }
    }
    return readStatus;
}
grpc::Status CardManagerServerImpl::ReadEFTransparent(ServerContext* context,
    const ::telStub::ReadEFTransparentRequest* request,
    telStub::ReadEFTransparentReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->slot_id();
    std::string jsonfilename = "";
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(slotId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
        std::string filepath = request->file_path();
        uint16_t fileId  = request->file_id();
        int recordsize = request->size();
        std::string aid = request->aid();
        const char* appid = aid.c_str();
        telux::tel::IccResult result;
        telux::common::ErrorCode error;
        telux::common::Status status;
        int cbDelay;
        std::string apiname = "readEFTransparent";
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            error, cbDelay );

        int sizeofADF = rootObj["ICardManager"]["EFs"]["ADF"].size();
        LOG(DEBUG, __FUNCTION__,"Size of ADF is", sizeofADF);
        int index;
        if(status == telux::common::Status::SUCCESS) {
            bool foundAppId = findAppId(rootObj, appid, index);
            if(foundAppId) {
                int i = 0;
                int size = rootObj["ICardManager"]["EFs"]["ADF"]\
                    [index]["TransparentEFFiles"].size();
                LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size);
                while (i < size ) {
                    uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["TransparentEFFiles"][i]["fileId"].asInt();
                    if (tmpfileId == fileId) {
                        if (recordsize >= 0)  {
                            result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["TransparentEFFiles"][i]["sw1"].asInt();
                            LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                            result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["TransparentEFFiles"][i]["sw2"].asInt();
                            LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                            result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["TransparentEFFiles"][i]["payload"].asString();
                            LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                            std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                                ["TransparentEFFiles"][i]["data"].asString();
                            result.data = CommonUtils::convertStringToVector(input);
                            break;
                        } else {
                            LOG(DEBUG, __FUNCTION__,"Request failed ");
                            error = telux::common::ErrorCode::GENERIC_FAILURE;
                        }
                    }
                    i++;
                }
                if (i == size) {
                    LOG(DEBUG, __FUNCTION__,"FileId not found ");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            } else {
                int i = 0;
                int size = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"].size();
                LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size);
                while (i < size ) {
                    uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"]\
                        [i]["fileId"].asInt();
                    if (tmpfileId == fileId) {
                        if (recordsize >= 0)  {
                            result.sw1 = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"]\
                                [i]["sw1"].asInt();
                            LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                            result.sw2 = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"]\
                                [i]["sw2"].asInt();
                            LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                            result.payload = rootObj["ICardManager"]["EFs"]\
                                ["DFTransparentEFRecords"][i]["payload"].asString();
                            LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                            std::string input = rootObj["ICardManager"]["EFs"]\
                                ["DFTransparentEFRecords"][i]["data"].asString();
                            result.data = CommonUtils::convertStringToVector(input);
                            break;
                        } else {
                            LOG(DEBUG, __FUNCTION__,"Request failed ");
                            error = telux::common::ErrorCode::GENERIC_FAILURE;
                        }
                    }
                    i++;
                }
                if (i == size) {
                    LOG(DEBUG, __FUNCTION__,"FileId not found ");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            }
        }
        //Create response
        telStub::IccResult requestedRecord;
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        response->set_delay(cbDelay);
        response->set_status(static_cast<commonStub::Status>(status));
        if(error == telux::common::ErrorCode::SUCCESS) {
            requestedRecord.set_sw1(result.sw1);
            requestedRecord.set_sw2(result.sw2);
            requestedRecord.set_pay_load(result.payload);
            for(auto &it : result.data) {
                requestedRecord.add_data(it);
            }
        } else {
            std::string s = "";
            requestedRecord.set_sw1(0);
            requestedRecord.set_sw2(0);
            requestedRecord.set_pay_load(s);
            for(auto &it : result.data) {
                requestedRecord.add_data(it);
            }
        }
        *response->mutable_result() = requestedRecord;
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::WriteEFTransparent(ServerContext* context,
    const ::telStub::WriteEFTransparentRequest* request,
    telStub::WriteEFTransparentReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int slotId = request->slot_id();
    std::string jsonfilename = "";
    std::string apiname = "writeEFTransparent";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(slotId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
        std::string filepath = request->file_path();
        uint16_t fileId  = request->file_id();
        std::string aid = request->aid();
        const char* appid = aid.c_str();
        telux::tel::IccResult result;
        std::vector<uint8_t> data;
        for(int d : request->data()) {
            data.emplace_back(d);
        }
        std::string str1 = "";
        std::string str2 = "";
        int cbDelay;
        telux::common::Status status;
        telux::common::ErrorCode error;
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            error, cbDelay );
        int i = 0;

        int sizeofADF = rootObj["ICardManager"]["EFs"]["ADF"].size();
        LOG(DEBUG, __FUNCTION__,"Size of ADF is", sizeofADF);
        int index;
        if(status == telux::common::Status::SUCCESS) {
            bool foundAppId = findAppId(rootObj, appid, index);
            if(foundAppId) {
                int size = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                    ["TransparentEFFiles"].size();
                LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size );
                LOG(DEBUG, __FUNCTION__,"TransparentEFfiles index ", index );
                while (i < size ) {
                    uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["TransparentEFFiles"][i]["fileId"].asInt();
                    LOG(DEBUG, __FUNCTION__,"FileId is  ", tmpfileId);
                    if (tmpfileId == fileId) {
                        str1 = CommonUtils::convertVectorToString(data, false);
                        rootObj["ICardManager"]["EFs"]["ADF"][index]["TransparentEFFiles"][i]\
                            ["data"] = str1;
                        LOG(DEBUG, __FUNCTION__,"String is  ", str1);
                        str1 = CommonUtils::convertVectorToString(data, true);
                        rootObj["ICardManager"]["EFs"]["ADF"][index]["TransparentEFFiles"][i]\
                            ["payload"] = str1;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[slotId] = rootObj;
                        result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["TransparentEFFiles"][i]["sw1"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                        result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["TransparentEFFiles"][i]["sw2"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                        break;
                    }
                    i++;
                }
                if (i == size) {
                    LOG(DEBUG, __FUNCTION__,"FileId not found ");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            } else {
                int i = 0;
                int size = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"].size();
                LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size);
                while (i < size ) {
                    uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"]\
                        [i]["fileId"].asInt();
                    if (tmpfileId == fileId) {
                        str1 = CommonUtils::convertVectorToString(data, false);
                        rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"][i]["data"] = str1;
                        LOG(DEBUG, __FUNCTION__,"String is  ", str1);
                        str1 = CommonUtils::convertVectorToString(data, true);
                        rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"][i]["payload"]
                            = str1;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[slotId] = rootObj;
                        result.sw1 = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"][i]\
                            ["sw1"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                        result.sw2 = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"][i]\
                            ["sw2"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                        break;
                    } else {
                        LOG(DEBUG, __FUNCTION__,"Request failed ");
                        error = telux::common::ErrorCode::GENERIC_FAILURE;
                    }
                    i++;
                }
                if (i == size) {
                    LOG(DEBUG, __FUNCTION__,"FileId not found ");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            }
        }
        //Create response
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        telStub::IccResult requestedRecord;
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        response->set_delay(cbDelay);
        response->set_status(static_cast<commonStub::Status>(status));

        if(error == telux::common::ErrorCode::SUCCESS) {
            std::string s = "";
            requestedRecord.set_sw1(result.sw1);
            requestedRecord.set_sw2(result.sw2);
            requestedRecord.set_pay_load(s);
        } else {
            std::string s = "";
            requestedRecord.set_sw1(0);
            requestedRecord.set_sw2(0);
            requestedRecord.set_pay_load(s);
        }
        *response->mutable_result() = requestedRecord;
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::RequestEFAttributes(ServerContext* context,
    const ::telStub::EFAttributesRequest* request,
    ::telStub::RequestEFAttributesReply* response) {

    int slotId = request->slot_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(slotId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
        std::string filepath = request->file_path();
        ::telStub::EfType eftype = request->ef_type();
        telux::tel::EfType ef_type= static_cast<telux::tel::EfType>(eftype);
        uint16_t fileId  = request->file_id();
        std::string aid = request->aid();
        const char* appid = aid.c_str();

        telux::tel::IccResult result;
        telux::tel::FileAttributes attributes;
        commonStub::ErrorCode tmp;
        telux::common::ErrorCode error;
        std::string apiname = "requestEFAttributes";
        int cbDelay;
        int index;
        int i = 0;
        telux::common::Status status;
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            error, cbDelay );

        int sizeofADF = rootObj["ICardManager"]["EFs"]["ADF"].size();
        LOG(DEBUG, __FUNCTION__,"Size of ADF is", sizeofADF);

        if(status == telux::common::Status::SUCCESS) {
            bool foundAppId = findAppId(rootObj, appid, index);
            if(foundAppId) {
                if (ef_type == telux::tel::EfType::TRANSPARENT ) {
                    tmp = getTransparentFileAttributes(rootObj, i, fileId, attributes, index);
                    if(tmp == commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
                        std::string data = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["TransparentEFFiles"][i]["data"].asString();
                        std::vector<int> tmp = CommonUtils::convertStringToVector(data);
                        result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i]["sw1"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                        result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i]["sw2"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                        result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i]["payload"].asString();
                        LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                    } else {
                        error = telux::common::ErrorCode::GENERIC_FAILURE;
                    }
                } else if (ef_type == telux::tel::EfType::LINEAR_FIXED) {
                    tmp = getLinearfixedFileAttributes(rootObj, i, fileId, attributes, index);
                    if(tmp == commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
                        result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+1]["sw1"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                        result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+1]["sw2"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                        result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+1]["payload"].asString();
                        LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                        std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+1]["data"].asString();
                        result.data = CommonUtils::convertStringToVector(input);
                    } else {
                        error = telux::common::ErrorCode::GENERIC_FAILURE;
                    }
                } else {
                    LOG(DEBUG, __FUNCTION__,"Unknown EFType ");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            } else {
                LOG(DEBUG, __FUNCTION__, "Valid AppId not found");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }

        //Create response
        telStub::IccResult requestedRecord;

        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_delay(cbDelay);
        if(error == telux::common::ErrorCode::SUCCESS) {
            requestedRecord.set_sw1(result.sw1);
            requestedRecord.set_sw2(result.sw2);
            requestedRecord.set_pay_load(result.payload);
            for(auto &it : result.data) {
                requestedRecord.add_data(it);
            }
            *response->mutable_result() = requestedRecord;
            telStub::FileAttributes requestedAttributes;
            requestedAttributes.set_file_size(attributes.fileSize);
            requestedAttributes.set_record_size(attributes.recordSize);
            requestedAttributes.set_record_count(attributes.recordCount);
            *response->mutable_file_attributes() = requestedAttributes;
        } else {
            std::string payload = "";
            requestedRecord.set_sw1(0);
            requestedRecord.set_sw2(0);
            requestedRecord.set_pay_load(payload);
            for(auto &it : result.data) {
                requestedRecord.add_data(it);
            }
            *response->mutable_result() = requestedRecord;
            telStub::FileAttributes requestedAttributes;
            requestedAttributes.set_file_size(0);
            requestedAttributes.set_record_size(0);
            requestedAttributes.set_record_count(0);
            *response->mutable_file_attributes() = requestedAttributes;
        }
    }
    return readStatus;
}

commonStub::ErrorCode CardManagerServerImpl::getTransparentFileAttributes(Json::Value rootObj,
    int& i, uint16_t fileId, telux::tel::FileAttributes& attributes, int& index) {
    int size = rootObj["ICardManager"]["EFs"]["ADF"][index]["TransparentEFFiles"].size();
    LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size);
    commonStub::ErrorCode error = commonStub::ErrorCode::ERROR_CODE_SUCCESS;
    while (i < size ) {
        uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
            ["TransparentEFFiles"][i]["fileId"].asInt();
        if (tmpfileId == fileId) {
            std::string data = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                ["TransparentEFFiles"][i]["data"].asString();
            std::vector<int> tmp = CommonUtils::convertStringToVector(data);
            attributes.recordSize = tmp.size();
            attributes.fileSize = attributes.recordSize;
            break;
        } else {
            LOG(DEBUG, __FUNCTION__,"FileId not found ");
            i++;
        }
    }
    if(i == size) {
        LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
        error = commonStub::ErrorCode::GENERIC_FAILURE;
    }
return error;
}

commonStub::ErrorCode CardManagerServerImpl::getLinearfixedFileAttributes(Json::Value rootObj,
    int& i, uint16_t fileId, telux::tel::FileAttributes& attributes, int& index ) {
    int size = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"].size();
    LOG(DEBUG, __FUNCTION__,"LinearFixedEFfiles size ", size);
    commonStub::ErrorCode error = commonStub::ErrorCode::ERROR_CODE_SUCCESS;
    while (i < size ) {
        uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
            ["LinearFixedEFFiles"][i]["fileId"].asInt();
        if (tmpfileId == fileId) {
            attributes.recordCount = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                ["LinearFixedEFFiles"][i]["numberOfRecords"].asInt(); //
            std::string data = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                ["LinearFixedEFFiles"][i+1]["data"].asString();
            std::vector<int> tmp = CommonUtils::convertStringToVector(data);
            attributes.recordSize = tmp.size();
            attributes.fileSize = (attributes.recordCount)*(attributes.recordSize);
            error = commonStub::ErrorCode::ERROR_CODE_SUCCESS;
            break;
        } else {
            LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
            int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                [i]["numberOfRecords"].asInt();
            i = i + num + 1;
            LOG(DEBUG, __FUNCTION__,"Incremented value is ", i );
        }
    }
    if(i == size) {
        LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
        error = commonStub::ErrorCode::GENERIC_FAILURE;
    }
    return error;
}

grpc::Status CardManagerServerImpl::OpenLogicalChannel(ServerContext* context,
    const ::telStub::OpenLogicalChannelRequest* request,
    telStub::OpenLogicalChannelReply* response) {

    LOG(DEBUG, __FUNCTION__);
    int phoneid = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneid, jsonfilename, rootObj);
        getJsonForApiResponseSlot(phoneid, jsonObjApiResponseFileName, jsonObjApiResponse);
        telux::common::ErrorCode error;
        telux::common::Status status;
        int cbDelay;
        telux::tel::IccResult result;
        int channelId;

        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", "openLogicalChannel", status,
            error, cbDelay );

        if(status == telux::common::Status::SUCCESS) {
            bool isChannelOpen = rootObj["ICardManager"]["openLogicalChannel"]["isOpen"].asBool();
            if (isChannelOpen) {
                LOG(DEBUG, __FUNCTION__, "already open");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            } else {
                rootObj["ICardManager"]["openLogicalChannel"]["isOpen"] = true;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneid] = rootObj;
                result.sw1 = rootObj["ICardManager"]["transmitApduLogicalChannel"]\
                    ["onChannelResponseSw1"].asInt();
                LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                result.sw2 = rootObj["ICardManager"]["transmitApduLogicalChannel"]\
                    ["onChannelResponseSw2"].asInt();
                LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw2);
                result.payload = rootObj["ICardManager"]["transmitApduLogicalChannel"]\
                    ["onChannelResponsePayload"].asString();
                LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                std::string tmp = rootObj["ICardManager"]["transmitApduLogicalChannel"]\
                    ["onChannelResponseData"].asString();
                std::vector<int> data = CommonUtils::convertStringToVector(tmp);
                result.data = data;
                channelId = rootObj["ICardManager"]["openLogicalChannel"]\
                    ["onChannelResponseChannel"].asInt();
                LOG(DEBUG, __FUNCTION__,"channelId ", channelId);
            }
        }
        //Create response
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_delay(cbDelay);
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, "openLogicalChannel");
        response->set_iscallback(iscallback);
        telStub::IccResult requestedRecord;
        requestedRecord.set_sw1(result.sw1);
        requestedRecord.set_sw2(result.sw2);
        requestedRecord.set_pay_load(result.payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
        *response->mutable_result() = requestedRecord;
        response->set_channel_id(channelId);
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::CloseLogicalChannel(ServerContext* context,
    const ::telStub::CloseLogicalChannelRequest* request,
    telStub::CloseLogicalChannelReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        int channel = request->channel_id();
        telux::common::Status status;
        telux::common::ErrorCode error;
        int cbDelay;
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", "closeLogicalChannel", status,
            error, cbDelay );

        if(status == telux::common::Status::SUCCESS) {
            int inputchannel = rootObj["ICardManager"]["openLogicalChannel"]\
                ["onChannelResponseChannel"].asInt();
            if (inputchannel == channel) {
                bool isChannelOpen = rootObj["ICardManager"]["openLogicalChannel"]\
                    ["isOpen"].asBool();
                if (isChannelOpen) {
                    rootObj["ICardManager"]["openLogicalChannel"]["isOpen"] = false;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                } else {
                    LOG(DEBUG, __FUNCTION__, "already closed");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            } else {
                LOG(DEBUG, __FUNCTION__, "Invalid channel");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_delay(cbDelay);
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, "closeLogicalChannel");
        response->set_iscallback(iscallback);
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::TransmitAPDU(ServerContext* context,
    const ::telStub::TransmitAPDURequest* request,
    telStub::TransmitAPDUReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        std::vector<uint8_t> data;
        telux::common::Status status;
        telux::common::ErrorCode error;
        int cbDelay;
        for(int d : request->data()) {
            data.emplace_back(d);
        }
        std::string str1 = "";
        telux::tel::IccResult result;
        CommonUtils::getValues
            (jsonObjApiResponse,"ICardManager", "transmitApduLogicalChannel", status,
            error, cbDelay );

        if(status == telux::common::Status::SUCCESS) {
            str1 = CommonUtils::convertVectorToString(data, false);
            rootObj["ICardManager"]["transmitApduLogicalChannel"]["onChannelResponseData"] = str1;
            LOG(DEBUG, __FUNCTION__,"String is  ", str1);
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
            str1 = CommonUtils::convertVectorToString(data, true);
            rootObj["ICardManager"]["transmitApduLogicalChannel"]["onChannelResponsePayload"]
                = str1;
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;

            result.sw1 = rootObj["ICardManager"]["transmitApduLogicalChannel"]\
                ["onChannelResponseSw1"].asInt();
            result.sw2 = rootObj["ICardManager"]["transmitApduLogicalChannel"]\
                ["onChannelResponseSw2"].asInt();
            result.payload = rootObj["ICardManager"]["transmitApduLogicalChannel"]\
                ["onChannelResponsePayload"].asString();
            std::string tmp = rootObj["ICardManager"]["transmitApduLogicalChannel"]\
                ["onChannelResponseData"].asString();
            result.data = CommonUtils::convertStringToVector(tmp);
        }

        //Create response

        telStub::IccResult requestedRecord;
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_delay(cbDelay);
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, "transmitApduLogicalChannel");
        response->set_iscallback(iscallback);
        requestedRecord.set_sw1(result.sw1);
        requestedRecord.set_sw2(result.sw2);
        requestedRecord.set_pay_load(result.payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
        *response->mutable_result() = requestedRecord;
    }
    return readStatus;

}

grpc::Status CardManagerServerImpl::exchangeSimIO(ServerContext* context,
    const ::telStub::exchangeSimIORequest* request,
    telStub::exchangeSimIOReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        std::vector<uint8_t> data;
        for(int d : request->data()) {
            data.emplace_back(d);
        }
        std::string str1 = "";
        telux::common::Status status;
        telux::common::ErrorCode error;
        int cbDelay;
        std::string apiname = "exchangeSimIO";
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            error, cbDelay );
        telux::tel::IccResult result;

        if(status == telux::common::Status::SUCCESS) {
            str1 = CommonUtils::convertVectorToString(data, false);
            rootObj["ICardManager"]["exchangeSimIO"]["onChannelResponseData"] = str1;
            LOG(DEBUG, __FUNCTION__,"String is  ", str1);
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;

            str1 = CommonUtils::convertVectorToString(data, true);
            rootObj["ICardManager"]["exchangeSimIO"]["onChannelResponsePayload"] = str1;
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
            result.sw1 = rootObj["ICardManager"]["exchangeSimIO"]\
                ["onChannelResponseSw1"].asInt();
            result.sw2 = rootObj["ICardManager"]["exchangeSimIO"]\
                ["onChannelResponseSw2"].asInt();
            result.payload = rootObj["ICardManager"]["exchangeSimIO"]\
                ["onChannelResponsePayload"].asString();
            std::string tmp = rootObj["ICardManager"]["exchangeSimIO"]\
                ["onChannelResponseData"].asString();
            result.data = CommonUtils::convertStringToVector(tmp);
        }

        //Create response
        telStub::IccResult requestedRecord;
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_delay(cbDelay);
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        requestedRecord.set_sw1(result.sw1);
        requestedRecord.set_sw2(result.sw2);
        requestedRecord.set_pay_load(result.payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
        *response->mutable_result() = requestedRecord;
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::TransmitBasicAPDU(ServerContext* context,
    const ::telStub::TransmitBasicAPDURequest* request,
    telStub::TransmitBasicAPDUReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        std::vector<uint8_t> data;
        for(int d : request->data()) {
            data.emplace_back(d);
        }
        std::string str1 = "";
        telux::common::Status status;
        telux::common::ErrorCode error;
        int cbDelay;
        telux::tel::IccResult result;
        std::string apiname = "transmitApduBasicChannel";
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            error, cbDelay );

        if(status == telux::common::Status::SUCCESS) {
            str1 = CommonUtils::convertVectorToString(data, false);
            rootObj["ICardManager"]["transmitApduBasicChannel"]["onChannelResponseData"] = str1;
            LOG(DEBUG, __FUNCTION__,"String is  ", str1);
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;

            str1 = CommonUtils::convertVectorToString(data, true);
            rootObj["ICardManager"]["transmitApduBasicChannel"]["onChannelResponsePayload"] = str1;
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
            result.sw1 = rootObj["ICardManager"]["transmitApduBasicChannel"]\
                ["onChannelResponseSw1"].asInt();
            result.sw2 = rootObj["ICardManager"]["transmitApduBasicChannel"]\
                ["onChannelResponseSw2"].asInt();
            result.payload = rootObj["ICardManager"]["transmitApduBasicChannel"]\
                ["onChannelResponsePayload"].asString();
            std::string tmp = rootObj["ICardManager"]["transmitApduBasicChannel"]\
                ["onChannelResponseData"].asString();
            result.data = CommonUtils::convertStringToVector(tmp);
        }

        //Create response
        telStub::IccResult requestedRecord;
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_delay(cbDelay);
        response->set_iscallback(iscallback);
        requestedRecord.set_sw1(result.sw1);
        requestedRecord.set_sw2(result.sw2);
        requestedRecord.set_pay_load(result.payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
        *response->mutable_result() = requestedRecord;
    }
    return readStatus;

}

grpc::Status CardManagerServerImpl::requestEid(ServerContext* context,
    const ::telStub::requestEidRequest* request,
    telStub::requestEidReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    telux::common::Status status;
    telux::common::ErrorCode errorCodefromUser;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        std::string eid = rootObj["ICardManager"]["requestEid"]["eid"].asString();

        //Create response
        std::string apiname = "transmitApduBasicChannel";
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            errorCodefromUser, cbDelay );
        commonStub::ErrorCode error = static_cast<commonStub::ErrorCode>(errorCodefromUser);
        commonStub::Status status_value = static_cast<commonStub::Status>(status);
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_eid(eid);
        response->set_delay(cbDelay);
        response->set_iscallback(iscallback);
        response->set_error(error);
        response->set_status(status_value);
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::updateSimStatus(ServerContext* context,
    const ::telStub::updateSimStatusRequest* request,
    telStub::updateSimStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        int state = rootObj["ICardManager"]["getState"]["cardState"].asInt();
        response->set_card_state(static_cast<telStub::CardState>(state));

        int size = rootObj["ICardManager"]["getApplications"].size();
        for (int i = 0 ; i < size ; i++) {
            telStub::CardApp *apps = response->add_card_apps();
            telStub::AppType apptype = static_cast<telStub::AppType>(rootObj["ICardManager"]\
                ["getApplications"][i]["appType"].asInt());
            LOG(DEBUG, __FUNCTION__,"apptype is  ", static_cast<int>(apptype));
            apps->set_app_type(apptype);
            telStub::AppState appstate = static_cast<telStub::AppState>(rootObj["ICardManager"]\
                ["getApplications"][i]["appState"].asInt());
            LOG(DEBUG, __FUNCTION__,"appstate is  ", static_cast<int>(appstate));
            apps->set_app_state(appstate);
            std::string appid = rootObj["ICardManager"]["getApplications"][i]["appId"].asString();
            LOG(DEBUG, __FUNCTION__,"appid is  ", appid);
            apps->set_app_id(appid);
        }
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::ChangePinLock(ServerContext* context,
    const ::telStub::ChangePinLockRequest* request, telStub::ChangePinLockReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        ::telStub::CardLockType locktype = request->lock_type();
        std::string oldPwd = request->old_pin();
        std::string newPwd = request->new_pin();
        std::string appId = request->aid();
        std::string password;
        int retrycount;
        bool IsCardInfoChanged = false;
        telux::common::Status status;
        telux::common::ErrorCode error;
        int cbDelay;
        std::string apiname = "changeCardPassword";
        CommonUtils::getValues(jsonObjApiResponse, "ICardManager", apiname, status,
            error, cbDelay );

        if(status == telux::common::Status::SUCCESS) {
            if(locktype == ::telStub::CardLockType::PIN1) {
                password = rootObj["ICardManager"]["Pin1password"].asString();
                retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                    ["retryCountPin1"].asInt();
                if((oldPwd == password) && (retrycount != -1)) {
                    rootObj["ICardManager"]["Pin1password"] = newPwd;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                } else {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin1"].asInt();
                    LOG(DEBUG, __FUNCTION__, "retrycount is ", retrycount);
                    if (retrycount < 0) {
                        LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                        error = telux::common::ErrorCode::PIN_BLOCKED;
                        //Update the app state to puk for app
                        int size = rootObj["ICardManager"]["getApplications"].size();
                        for (int i = 0; i < size; i++) {
                            std::string id = rootObj["ICardManager"]["getApplications"]\
                                [i]["appId"].asString();
                            if (id == appId) {
                                rootObj["ICardManager"]["getApplications"]\
                                    [i]["appState"] = 3; //puk state
                                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                                jsonObjSystemStateSlot_[phoneId] = rootObj;
                                IsCardInfoChanged = true;
                                break;
                            } else {
                                LOG(DEBUG, __FUNCTION__,"No matching appId found");
                                error = telux::common::ErrorCode::INVALID_ARG;
                            }
                        }
                    } else {
                        if(retrycount >= -1) {
                            retrycount--;
                            LOG(DEBUG, __FUNCTION__, "retrycount is ", retrycount);
                            error = telux::common::ErrorCode::PASSWORD_INCORRECT;

                            rootObj["ICardManager"]["changeCardPassword"]\
                                ["retryCountPin1"] = retrycount;
                            JsonParser::writeToJsonFile(rootObj, jsonfilename);
                            jsonObjSystemStateSlot_[phoneId] = rootObj;
                            IsCardInfoChanged = true;
                        }
                    }
                }
            } else if (locktype == ::telStub::CardLockType::PIN2) {
                password = rootObj["ICardManager"]["Pin2password"].asString();
                retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                    ["retryCountPin2"].asInt();
                if(oldPwd == password && (retrycount != -1)) {
                    rootObj["ICardManager"]["Pin2password"] = newPwd;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin2"].asInt();
                } else {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin2"].asInt();
                    if (retrycount < 0) {
                        LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                        error = telux::common::ErrorCode::PIN_BLOCKED;
                    } else {
                        if(retrycount >= -1) {
                            retrycount--;
                            error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                        }
                        rootObj["ICardManager"]["changeCardPassword"]\
                            ["retryCountPin2"] = retrycount;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                    }
                }
            } else {
                LOG(DEBUG, __FUNCTION__,"Not Supported LockType");
                error = telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
            }
        }

        //Create response
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_delay(cbDelay);
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        response->set_retry_count(retrycount);
        response->set_iscardinfochanged(IsCardInfoChanged);
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::UnlockByPin(ServerContext* context,
    const ::telStub::UnlockByPinRequest* request,
    telStub::UnlockByPinReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        ::telStub::CardLockType locktype = request->lock_type();
        std::string pwd = request->pin();
        std::string appId = request->aid();
        std::string password;
        int retrycount;
        bool IsCardInfoChanged = false;
        int cbDelay;
        telux::common::Status status;
        telux::common::ErrorCode error;
        std::string apiname = "unlockCardByPin";
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            error, cbDelay );

        if(status == telux::common::Status::SUCCESS) {
            if(locktype == ::telStub::CardLockType::PIN1) {
                password = rootObj["ICardManager"]["Pin1password"].asString();
                retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                    ["retryCountPin1"].asInt();
                if((pwd == password) && (retrycount != -1)) {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin1"].asInt();
                } else {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin1"].asInt();
                    if (retrycount < 0) {
                        LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                        error = telux::common::ErrorCode::PIN_BLOCKED;
                        //Update the app state to puk for app
                        int size = rootObj["ICardManager"]["getApplications"].size();
                        for (int i = 0; i < size; i++) {
                            std::string id = rootObj["ICardManager"]["getApplications"]\
                                [i]["appId"].asString();
                            if (id == appId) {
                                rootObj["ICardManager"]["getApplications"][i]\
                                    ["appState"] = 3; //puk state
                                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                                jsonObjSystemStateSlot_[phoneId] = rootObj;
                                IsCardInfoChanged = true;
                                break;
                            } else {
                                LOG(DEBUG, __FUNCTION__,"No matching appId found");
                                error = telux::common::ErrorCode::INVALID_ARG;
                            }
                        }
                    } else {
                        if(retrycount >= -1) {
                            retrycount--;
                            error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                        }
                        rootObj["ICardManager"]["changeCardPassword"]\
                            ["retryCountPin1"] = retrycount;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                        IsCardInfoChanged = true;
                    }
                }
            } else if (locktype == ::telStub::CardLockType::PIN2) {
                password = rootObj["ICardManager"]["Pin2password"].asString();
                retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                    ["retryCountPin2"].asInt();
                if(pwd == password && (retrycount != -1)) {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin2"].asInt();
                } else {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin2"].asInt();
                    if (retrycount < 0) {
                    LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                    error = telux::common::ErrorCode::PIN_BLOCKED;
                    } else {
                        if(retrycount >= -1) {
                            retrycount--;
                            error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                        }
                        rootObj["ICardManager"]["changeCardPassword"]\
                            ["retryCountPin2"] = retrycount;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                    }
                }
            } else {
                LOG(DEBUG, __FUNCTION__,"Not Supported LockType");
                error = telux::common::ErrorCode::INVALID_ARG;
            }
        }
        //Create response
        response->set_retry_count(retrycount);
        response->set_iscardinfochanged(IsCardInfoChanged);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        response->set_delay(cbDelay);
        response->set_status(static_cast<commonStub::Status>(status));
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::UnlockByPuk(ServerContext* context,
    const ::telStub::UnlockByPukRequest* request,
    telStub::UnlockByPukReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        ::telStub::CardLockType locktype = request->lock_type();
        std::string pwd = request->new_pin();
        std::string appId = request->aid();
        std::string puk = request->puk();
        std::string password;
        int retrycount;
        bool IsCardInfoChanged = false;
        telux::common::ErrorCode error;
        telux::common::Status status;
        int cbDelay;
        std::string apiname = "unlockCardByPuk";
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            error, cbDelay );

        if(status == telux::common::Status::SUCCESS) {
            if(locktype == ::telStub::CardLockType::PUK1) {
                password = rootObj["ICardManager"]["Puk1password"].asString();
                retrycount = rootObj["ICardManager"]["unlockCardByPuk"]["retryCountPin1"].asInt();
                if((puk == password) && (retrycount != -1)) {
                    rootObj["ICardManager"]["Pin1password"] = pwd;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    rootObj["ICardManager"]["changeCardPassword"]["retryCountPin1"] = 3;
                    //Update the app state to puk for app
                    int size = rootObj["ICardManager"]["getApplications"].size();
                    for (int i = 0; i < size; i++) {
                        std::string id = rootObj["ICardManager"]["getApplications"][i]\
                            ["appId"].asString();
                        if (id == appId) {
                            rootObj["ICardManager"]["getApplications"][i]\
                                ["appState"] = 5; //ready state
                            JsonParser::writeToJsonFile(rootObj, jsonfilename);
                            jsonObjSystemStateSlot_[phoneId] = rootObj;
                            IsCardInfoChanged = true;
                            break;
                        } else {
                            LOG(DEBUG, __FUNCTION__,"No matching appId found");
                            error = telux::common::ErrorCode::INVALID_ARG;
                        }
                    }
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    retrycount = rootObj["ICardManager"]["unlockCardByPuk"]\
                        ["retryCountPin1"].asInt();
                } else {
                    retrycount = rootObj["ICardManager"]["unlockCardByPuk"]\
                        ["retryCountPin1"].asInt();
                    if (retrycount < 0) {
                        LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                        error = telux::common::ErrorCode::PIN_BLOCKED;
                    }
                    else {
                        if(retrycount >= -1) {
                            retrycount--;
                            error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                        }
                        rootObj["ICardManager"]["unlockCardByPuk"]["retryCountPin1"] = retrycount;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                        IsCardInfoChanged = true;
                    }
                }
            } else if (locktype == ::telStub::CardLockType::PUK2) {
                password = rootObj["ICardManager"]["Puk2password"].asString();
                retrycount = rootObj["ICardManager"]["unlockCardByPuk"]["retryCountPin2"].asInt();
                if((puk == password) && (retrycount != -1)) {
                    rootObj["ICardManager"]["Pin2password"] = pwd;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    rootObj["ICardManager"]["changeCardPassword"]["retryCountPin2"] = 3;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    retrycount = rootObj["ICardManager"]["unlockCardByPuk"]\
                        ["retryCountPin2"].asInt();
                } else {
                    retrycount = rootObj["ICardManager"]["unlockCardByPuk"]\
                        ["retryCountPin2"].asInt();
                    if (retrycount < 0) {
                        LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                        error = telux::common::ErrorCode::PIN_BLOCKED;
                    }
                    else {
                        if(retrycount >= -1) {
                            retrycount--;
                            error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                        }
                        rootObj["ICardManager"]["unlockCardByPuk"]["retryCountPin2"] = retrycount;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                        IsCardInfoChanged = true;
                    }
                }
            } else {
                LOG(DEBUG, __FUNCTION__,"Not Supported LockType");
                error = telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
            }
        }
        //Create response
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(iscallback);
        response->set_delay(cbDelay);
        response->set_retry_count(retrycount);
        response->set_iscardinfochanged(IsCardInfoChanged);
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::SetCardLock(ServerContext* context,
    const ::telStub::SetCardLockRequest* request, ::telStub::SetCardLockReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        ::telStub::CardLockType locktype = request->lock_type();
        std::string pwd = request->pwd();
        bool enable = request->enable();
        std::string appId = request->aid();
        std::string password;
        int retrycount;
        bool IsCardInfoChanged = false;
        telux::common::ErrorCode error;
        telux::common::Status status;
        int cbDelay;
        std::string apiname = "setCardLock";
        CommonUtils::getValues(jsonObjApiResponse, "ICardManager", apiname, status,
            error, cbDelay );

        if(status == telux::common::Status::SUCCESS) {
            if(locktype == ::telStub::CardLockType::PIN1) {
                password = rootObj["ICardManager"]["Pin1password"].asString();
                retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                    ["retryCountPin1"].asInt();
                if((pwd == password) && (retrycount != -1)) {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin1"].asInt();
                    rootObj["ICardManager"]["setCardLock"]["isPin1Available"] = enable;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                } else {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin1"].asInt();
                    if (retrycount < 0) {
                        LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                        error = telux::common::ErrorCode::PIN_BLOCKED;
                        //Update the app state to puk for app
                        int size = rootObj["ICardManager"]["getApplications"].size();
                        for (int i = 0; i < size; i++) {
                            std::string id = rootObj["ICardManager"]["getApplications"][i]\
                                ["appId"].asString();
                            if (id == appId) {
                                rootObj["ICardManager"]["getApplications"]\
                                    [i]["appState"] = 3; //puk state
                                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                                jsonObjSystemStateSlot_[phoneId] = rootObj;
                                IsCardInfoChanged = true;
                                break;
                            } else {
                                LOG(DEBUG, __FUNCTION__,"No matching appId found");
                                error = telux::common::ErrorCode::INVALID_ARG;
                            }
                        }
                    }
                    else {
                        if(retrycount >= -1) {
                            retrycount--;
                            error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                        }
                        rootObj["ICardManager"]["changeCardPassword"]\
                            ["retryCountPin1"] = retrycount;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                        IsCardInfoChanged = true;
                    }
                }
            } else if (locktype == ::telStub::CardLockType::FDN) {
                password = rootObj["ICardManager"]["Pin2password"].asString();
                retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                    ["retryCountPin2"].asInt();
                if(pwd == password && (retrycount != -1)) {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin2"].asInt();
                    rootObj["ICardManager"]["setCardLock"]["isPin2Available"] = enable;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                } else {
                    retrycount = rootObj["ICardManager"]["changeCardPassword"]\
                        ["retryCountPin2"].asInt();
                    if (retrycount < 0) {
                        LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                        error = telux::common::ErrorCode::PIN_BLOCKED;
                    } else {
                        if(retrycount >= -1) {
                            retrycount--;
                            error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                        }
                        rootObj["ICardManager"]["changeCardPassword"]\
                            ["retryCountPin2"] = retrycount;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                    }
                }
            } else {
                LOG(DEBUG, __FUNCTION__,"Not Supported LockType");
                error = telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
            }
        }
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_status(static_cast<commonStub::Status>(status));
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        response->set_delay(cbDelay);
        response->set_retry_count(retrycount);
        response->set_iscardinfochanged(IsCardInfoChanged);
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::QueryPin1Lock(ServerContext* context,
    const ::telStub::QueryPin1LockRequest* request, telStub::QueryPin1LockReply* response) {
    LOG(DEBUG, __FUNCTION__);
        int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        getJsonForSystemData(phoneId, jsonfilename, rootObj);

        telux::common::Status status;
        telux::common::ErrorCode error;
        int cbDelay;
        std::string apiname = "queryPin1LockState";
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            error, cbDelay );

        bool state = rootObj["ICardManager"]["setCardLock"]["isPin1Available"].asBool();
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);

        response->set_iscallback(iscallback);
        response->set_state(state);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::QueryFdnLock(ServerContext* context,
    const ::telStub::QueryFdnLockRequest* request, ::telStub::QueryFdnLockReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        telux::common::Status status;
        telux::common::ErrorCode errorCodefromUser;
        int cbDelay;

        std::string apiname = "queryFdnLockState";
        CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
            errorCodefromUser, cbDelay );
        bool state = rootObj["ICardManager"]["setCardLock"]["isPin2Available"].asBool();
        bool isAvailable = rootObj["ICardManager"]["setCardLock"]["fdnState"].asBool();
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_delay(cbDelay);
        response->set_iscallback(iscallback);
        response->set_state(state);
        response->set_is_available(isAvailable);
        response->set_error(static_cast<commonStub::ErrorCode>(errorCodefromUser));
        response->set_status(static_cast<commonStub::Status>(status));
    }
    return readStatus;
}

grpc::Status CardManagerServerImpl::CardPower(ServerContext* context,
    const ::telStub::CardPowerRequest* request,
    telStub::CardPowerResponse* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    std::string apiname;
    bool powerup = request->powerup();
    if(powerup) {
        apiname = "cardPowerUp";
    } else {
        apiname = "cardPowerDown";
    }
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        telux::common::Status status;
        telux::common::ErrorCode error;
        int cbDelay;
        CommonUtils::getValues(jsonObjApiResponse, "ICardManager", apiname, status,
            error, cbDelay );

        if(status == telux::common::Status::SUCCESS) {
            bool currentstate = rootObj["ICardManager"]["setCardPower"]["cardPowerState"].asBool();
            if (currentstate != powerup) {
                rootObj["ICardManager"]["setCardPower"]["cardPowerState"] = powerup;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
                if(powerup) {
                    rootObj["ICardManager"]["getState"]["cardState"] = 1;
                        //Update Card State to PRESENT
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                } else {
                    rootObj["ICardManager"]["getState"]["cardState"] = 0;
                        //Update Card State to ABSENT
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                }
            } else {
                error = telux::common::ErrorCode::NO_EFFECT;
            }
        }
        bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(iscallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
        response->set_status(static_cast<commonStub::Status>(status));
    }
    return readStatus;
}

commonStub::ErrorCode CardManagerServerImpl::findmatchingrecordDF (Json::Value rootObj, int& size,
    int& recordNum, uint16_t& fileId, int& i) {
    commonStub::ErrorCode error = commonStub::ErrorCode::ERROR_CODE_SUCCESS;
    while (i < size ) {
        uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
            [i]["fileId"].asInt();
        if (tmpfileId == fileId) {
            int num = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i]\
                ["numberOfRecords"].asInt();
            LOG(DEBUG, __FUNCTION__,"NumberOfRecords ", num);
            if(recordNum <= num ) {
                error = commonStub::ErrorCode::ERROR_CODE_SUCCESS;
                break;
            } else {
                error = commonStub::ErrorCode::GENERIC_FAILURE;
                LOG(DEBUG, __FUNCTION__, "Invalid Record");
                break;
            }
        } else {
            LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
            int num = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i]\
                ["numberOfRecords"].asInt();
            i = i + num + 1;
            LOG(DEBUG, __FUNCTION__,"Incremented value is ", i );
        }
    }
    if(i == size) {
        LOG(DEBUG, __FUNCTION__,"Valid record not found ", i );
        error = commonStub::ErrorCode::GENERIC_FAILURE;
    }
    return error;
}

grpc::Status CardManagerServerImpl::IsNtnProfileActive(ServerContext* context,
    const ::telStub::IsNtnProfileActiveRequest* request,
    telStub::IsNtnProfileActiveReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if (readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        bool state = rootObj["ICardManager"]["isNtnProfileActive"]["state"].asBool();
        response->set_is_ntn_profile_active(state);
    }
    return readStatus;
}

void CardManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__,"String is ", event );
    bool triggerNotification = false;
    ::eventService::EventResponse notification;
    std::string evt = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (CARD_EVENT == evt) {
        triggerNotification = handleCardInfoChanged(event, notification);
    } else if (SIM_REFRESH_EVENT == evt) {
        triggerNotification = handleSimRefreshInjector(event, notification);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    if (triggerNotification) {
        notification.set_filter(telux::tel::TEL_CARD_FILTER);
        //posting the event to EventService event queue
        EventService::getInstance().updateEventQueue(notification);
    }
}

void CardManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == telux::tel::TEL_CARD_FILTER) {
        onEventUpdate(message.event());
    }
}

bool CardManagerServerImpl::handleCardInfoChanged(std::string eventParams,
    ::eventService::EventResponse& notification) {
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    int slotId;
    std::string jsonfilename = "";
    std::string apiname = "setCardPower";
    Json::Value rootObj;
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        slotId = 1;
    } else {
        try {
            slotId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return false;
        }
    }
    if(slotId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return false;
        }
    }
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);
    // Fetch card power
    int input;
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "Card power input not passed, assuming power ON");
        input = true;
    } else {
        try {
            input = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return false;
        }
    }
    // Fetch ntn profile active status
    int isNtnProfileActive;
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, __FUNCTION__,
            "isNtnProfileActive not passed, assuming ntn profile is not active");
        isNtnProfileActive = false;
    } else {
        try {
            isNtnProfileActive = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, " isNtnProfileActive : ", isNtnProfileActive);
    getJsonForSystemData(slotId, jsonfilename, rootObj);
    bool cardpower = static_cast<bool>(input);
    LOG(DEBUG, __FUNCTION__, "The fetched card power state id is: ", cardpower);
    bool currentstate = rootObj["ICardManager"]["setCardPower"]["cardPowerState"].asBool();
    if (currentstate != cardpower) {
        rootObj["ICardManager"]["setCardPower"]["cardPowerState"] = cardpower;
        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        jsonObjSystemStateSlot_[slotId] = rootObj;
        if(cardpower) {
            rootObj["ICardManager"]["getState"]["cardState"] = 1;
                //Update Card State to PRESENT
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[slotId] = rootObj;
        } else {
            rootObj["ICardManager"]["getState"]["cardState"] = 0;
                //Update Card State to ABSENT
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[slotId] = rootObj;
        }
    } else {
         LOG(DEBUG, __FUNCTION__, "No change in card state ");
         return false;
    }
    // write ntn profile active status
    rootObj["ICardManager"]["isNtnProfileActive"]["state"] = isNtnProfileActive;
    JsonParser::writeToJsonFile(rootObj, jsonfilename);
    jsonObjSystemStateSlot_[slotId] = rootObj;

    ::telStub::cardInfoChange cardInfoChangeEvent;
    cardInfoChangeEvent.set_phone_id(slotId);
    cardInfoChangeEvent.set_card_power(cardpower);
    cardInfoChangeEvent.set_is_ntn_profile_active(isNtnProfileActive);

    notification.mutable_any()->PackFrom(cardInfoChangeEvent);
    return true;
}

bool CardManagerServerImpl::handleSimRefreshInjector(std::string eventParams,
    ::eventService::EventResponse& notification) {
    int stage = WAITING_FOR_VOTES;
    LOG(DEBUG, __FUNCTION__, " string is: ", eventParams);

    int mode;
    GETVALUE_VIA_STR(eventParams, mode);
    int fileId;
    GETVALUE_VIA_STR(eventParams, fileId);
    std::string filePath = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    int sessionId;
    GETVALUE_VIA_STR(eventParams, sessionId);
    int slotId = getSlotBySessionType(static_cast<telux::tel::SessionType>(sessionId));
    if (slotId < DEFAULT_SLOT_ID || slotId > MAX_SLOT_ID) {
        return false;
    }
    std::string aid;
    if (sessionId == static_cast<int>(telux::tel::SessionType::NONPROVISIONING_SLOT_1) ||
        sessionId == static_cast<int>(telux::tel::SessionType::NONPROVISIONING_SLOT_2)) {
        aid = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    } else {
        LOG(DEBUG, __FUNCTION__, " ignore aid as session type is ", sessionId);
    }

    RefreshEventAndPending newRefreshEvt;
    if (refreshEvtMap_.find(slotId) != refreshEvtMap_.end()) {
        CardRefreshStage cachedStage =
            static_cast<CardRefreshStage>(refreshEvtMap_[slotId].refreshEvent.stage());
        if (cachedStage == WAITING_FOR_VOTES || cachedStage == STARTING) {
            LOG(ERROR, __FUNCTION__, " 1 session in progress, invalid new user inject!");
            return false;
        }
    }

    newRefreshEvt.pendingAllow = refreshVotingClients_.size();
    newRefreshEvt.pendingComplete = refreshRegisterClients_.size();
    if (not newRefreshEvt.pendingAllow) {
        stage = STARTING;
    } else {
         updateSimRefreshStage(slotId, ENDED_WITH_FAILURE, REFRESH_USER_ALLOW_TIMEOUT_MS,
             true, false);
    }
    LOG(DEBUG, __FUNCTION__, " slotId ", slotId, ", stage ", stage,  ", mode ", mode,
        ", fileId ", fileId,  ", filePath ", filePath,", sessionId ", sessionId,
        ", aid ", aid);

    newRefreshEvt.refreshEvent.set_phone_id(slotId);
    newRefreshEvt.refreshEvent.set_stage(static_cast<::telStub::RefreshStage>(stage));
    newRefreshEvt.refreshEvent.set_mode(static_cast<::telStub::RefreshMode>(mode));

    newRefreshEvt.refreshEvent.clear_effiles();
    ::telStub::IccFile *efFilesMem = newRefreshEvt.refreshEvent.add_effiles();
    if (efFilesMem) {
        efFilesMem->set_fileid(fileId);
        efFilesMem->set_filepath(filePath);
    }

    auto ssType = static_cast<::telStub::SessionType>(sessionId);
    ::telStub::RefreshParams* refreshs = newRefreshEvt.refreshEvent.mutable_refreshs();
    if (refreshs) {
        refreshs->set_sessiontype(ssType);
        refreshs->set_aid(aid);
    }

    notification.mutable_any()->PackFrom(newRefreshEvt.refreshEvent);
    refreshEvtMap_[slotId] = newRefreshEvt;

    if (stage == STARTING) {
        /*if no pendingComplete, go to next refresh stage*/
        if (newRefreshEvt.pendingComplete == 0 ||
            not requireConfirmComplete(static_cast<CardRefreshStage>(stage),
            static_cast<telux::tel::RefreshMode>(mode),
            static_cast<telux::tel::SessionType>(sessionId))) {
            updateSimRefreshStage(slotId, ENDED_WITH_SUCCESS, DEFAULT_DELAY, false, false);
        } else {
            updateSimRefreshStage(slotId, ENDED_WITH_FAILURE, REFRESH_USER_COMPLETE_TIMEOUT_MS,
                false, true);
        }
    }
    return true;
}

::grpc::Status CardManagerServerImpl::setupRefreshConfig(ServerContext* context,
    const ::telStub::RefreshConfigReq* request,
    telStub::TelCommonReply* response) {
    if (not request || not response) {
        LOG(ERROR, __FUNCTION__, " not request || not response.");
        return ::grpc::Status::CANCELLED;
    }

    clientSimRefreshPref clientInfo;
    getClientInfoFromRpc<::telStub::RefreshConfigReq>(request, clientInfo);

    LOG(DEBUG, __FUNCTION__, " phoneId ", clientInfo.phoneId,
        " isRegister ", static_cast<int>(request->isregister()),
        ", doVoting ", static_cast<int>(request->dovoting()));

    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode error = telux::common::ErrorCode::CANCELLED;
    int cbDelay = 0;

    getApiConfigureFromJson(clientInfo.phoneId, "setupRefreshConfig", status, error, cbDelay);
    do {
        if (status != telux::common::Status::SUCCESS) {
            LOG(WARNING, __FUNCTION__, " status non-success in json config");
            break;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (refreshEvtMap_.find(clientInfo.phoneId) != refreshEvtMap_.end()) {
                if (static_cast<telux::tel::RefreshStage>(
                    refreshEvtMap_[clientInfo.phoneId].refreshEvent.stage()) ==
                    telux::tel::RefreshStage::WAITING_FOR_VOTES ||
                    static_cast<telux::tel::RefreshStage>(
                    refreshEvtMap_[clientInfo.phoneId].refreshEvent.stage()) ==
                    telux::tel::RefreshStage::STARTING) {
                    status = telux::common::Status::FAILED;
                    error = telux::common::ErrorCode::INVALID_STATE;
                    LOG(WARNING, __FUNCTION__, " reject setup config if refresh in progress");
                    break;
                }
            }
        }

        /*update voting clients vector*/
        error = updateClientSimRefresh(refreshVotingClients_, clientInfo, request->dovoting());

        /*update register clients vector*/
        error = updateClientSimRefresh(refreshRegisterClients_, clientInfo, request->isregister());
    } while (0);

    response->set_error(static_cast<commonStub::ErrorCode>(error));
    response->set_delay(cbDelay);
    response->set_status(static_cast<commonStub::Status>(status));
    return ::grpc::Status::OK;
}

::grpc::Status CardManagerServerImpl::allowCardRefresh(ServerContext* context,
    const ::telStub::AllowCardRefreshReq* request,
    telStub::TelCommonReply* response) {
    if (not request || not response) {
        LOG(ERROR, __FUNCTION__, " not request || not response.");
        return ::grpc::Status::CANCELLED;
    }
    LOG(DEBUG, __FUNCTION__);

    clientSimRefreshPref clientInfo;
    getClientInfoFromRpc<::telStub::AllowCardRefreshReq>(request, clientInfo);

    bool allowRefresh = request->allowrefresh();
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode error = telux::common::ErrorCode::CANCELLED;
    int cbDelay = 0;

    getApiConfigureFromJson(clientInfo.phoneId, "allowCardRefresh", status, error, cbDelay);
    do {
        if (status != telux::common::Status::SUCCESS) {
            LOG(INFO, __FUNCTION__, " user prefer non-SUCCESS in json.");
            break;
        }
        bool foundClient = clientSimRefreshInfoPresent(refreshVotingClients_, clientInfo);
        if (not foundClient) {
            LOG(INFO, __FUNCTION__, " no such clientSimRefreshInfo found.");
            status = telux::common::Status::FAILED;
            error = telux::common::ErrorCode::NOT_PROVISIONED;
            break;
        }
        if (refreshEvtMap_.find(clientInfo.phoneId) == refreshEvtMap_.end()) {
            LOG(ERROR, __FUNCTION__, " no refresh in progress ", clientInfo.phoneId);
            status = telux::common::Status::FAILED;
            error = telux::common::ErrorCode::SUBSCRIPTION_NOT_SUPPORTED;
            break;
        }
        if (static_cast<CardRefreshStage>(refreshEvtMap_[clientInfo.phoneId].refreshEvent.stage())
            != WAITING_FOR_VOTES) {
            LOG(ERROR, __FUNCTION__, " refresh event is not in WAITING_FOR_VOTES stage");
            status = telux::common::Status::FAILED;
            error = telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
            break;
        }
        if (not allowRefresh) {
            updateSimRefreshStage(clientInfo.phoneId, ENDED_WITH_FAILURE, 0, false, false);
            break;
        }

        uint32_t pendingAllow;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (refreshEvtMap_[clientInfo.phoneId].pendingAllow > 0) {
                refreshEvtMap_[clientInfo.phoneId].pendingAllow -= 1;
            }
            pendingAllow = refreshEvtMap_[clientInfo.phoneId].pendingAllow;
            if (0 == pendingAllow) {
                cv_.notify_all();
            }
        }
        LOG(DEBUG, __FUNCTION__, " pendingAllow ", pendingAllow);
        if (not pendingAllow) {
            /*all clients allow refresh*/
            updateSimRefreshStage(clientInfo.phoneId, STARTING, DEFAULT_DELAY, false, false);
        }
    }while (0);

    response->set_error(static_cast<commonStub::ErrorCode>(error));
    response->set_status(static_cast<commonStub::Status>(status));
    response->set_delay(cbDelay);
    return ::grpc::Status::OK;
}

::grpc::Status CardManagerServerImpl::confirmRefreshHandlingCompleted(ServerContext* context,
    const ::telStub::ConfirmRefreshHandlingCompleteReq* request,
    telStub::TelCommonReply* response) {
    LOG(DEBUG, __FUNCTION__);
    if (not request || not response) {
        LOG(ERROR, __FUNCTION__, " not request || not response.");
        return ::grpc::Status::CANCELLED;
    }

    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode error = telux::common::ErrorCode::CANCELLED;
    int cbDelay = 0;
    clientSimRefreshPref clientInfo;

    getClientInfoFromRpc<::telStub::ConfirmRefreshHandlingCompleteReq>(request, clientInfo);
    getApiConfigureFromJson(clientInfo.phoneId, "confirmRefreshHandlingCompleted", status, error, cbDelay);
    do {
        if (status != telux::common::Status::SUCCESS) {
            LOG(INFO, __FUNCTION__, " user prefer non-SUCCESS in json.");
            break;
        }
        bool isCompleted = request->iscompleted();
        bool foundClient = clientSimRefreshInfoPresent(refreshRegisterClients_, clientInfo);
        if (not foundClient) {
            LOG(INFO, __FUNCTION__, " no such clientSimRefreshInfo found.");
            status = telux::common::Status::FAILED;
            error = telux::common::ErrorCode::NOT_PROVISIONED;
            break;
        }
        if (refreshEvtMap_.find(clientInfo.phoneId) == refreshEvtMap_.end()) {
            LOG(ERROR, __FUNCTION__, " no refresh in progress ", clientInfo.phoneId);
            status = telux::common::Status::FAILED;
            error = telux::common::ErrorCode::SUBSCRIPTION_NOT_SUPPORTED;
            break;
        }

        auto refresh = refreshEvtMap_[clientInfo.phoneId].refreshEvent;
        if (not requireConfirmComplete(static_cast<CardRefreshStage>(refresh.stage()),
            static_cast<telux::tel::RefreshMode>(refresh.mode()),
            static_cast<telux::tel::SessionType>(refresh.refreshs().sessiontype()))) {
            LOG(ERROR, __FUNCTION__, " does not require user confirm.");
            status = telux::common::Status::FAILED;
            error = telux::common::ErrorCode::MODE_NOT_SUPPORTED;
            break;
        }

        if (isCompleted) {
            uint32_t pendComplete;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (refreshEvtMap_[clientInfo.phoneId].pendingComplete > 0) {
                    refreshEvtMap_[clientInfo.phoneId].pendingComplete -= 1;
                }
                pendComplete = refreshEvtMap_[clientInfo.phoneId].pendingComplete;
                if (0 == pendComplete) {
                    cv_.notify_all();
                }
            }
            LOG(DEBUG, __FUNCTION__, " pendComplete ", pendComplete);
            if (pendComplete == 0) {
                updateSimRefreshStage(clientInfo.phoneId, ENDED_WITH_SUCCESS,
                    DEFAULT_DELAY, false, false);
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " user confirm not complete, waiting.");
        }
    } while (0);

    response->set_error(static_cast<commonStub::ErrorCode>(error));
    response->set_delay(cbDelay);
    response->set_status(static_cast<commonStub::Status>(status));
    return ::grpc::Status::OK;
}

::grpc::Status CardManagerServerImpl::requestLastRefreshEvent(ServerContext* context,
    const ::telStub::RequestLastRefreshEventReq* request,
    telStub::RequestLastRefreshEventResp* response) {
    LOG(DEBUG, __FUNCTION__);
    if (not request || not response) {
        LOG(ERROR, __FUNCTION__, " not request || not response.");
        return ::grpc::Status::CANCELLED;
    }

    clientSimRefreshPref clientInfo;
    getClientInfoFromRpc<::telStub::RequestLastRefreshEventReq>(request, clientInfo);

    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode error = telux::common::ErrorCode::CANCELLED;
    int cbDelay = 0;
    getApiConfigureFromJson(clientInfo.phoneId, "confirmRefreshHandlingCompleted", status, error, cbDelay);
    do {
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " user prefer settings is non-success.");
            break;
        }
        if (refreshEvtMap_.find(clientInfo.phoneId) == refreshEvtMap_.end()) {
            LOG(ERROR, __FUNCTION__, " no refresh in progress ", clientInfo.phoneId);
            status = telux::common::Status::FAILED;
            error = telux::common::ErrorCode::SUBSCRIPTION_NOT_SUPPORTED;
            break;
        }

        auto lastRefreshEvt = refreshEvtMap_[clientInfo.phoneId].refreshEvent;
        response->set_stage(lastRefreshEvt.stage());
        response->set_mode(lastRefreshEvt.mode());

        for (int i = 0; i< lastRefreshEvt.effiles_size(); ++i) {
            ::telStub::IccFile* efFile = response->add_effiles();
            if (efFile) {
                efFile->set_fileid(lastRefreshEvt.effiles(i).fileid());
                efFile->set_filepath(lastRefreshEvt.effiles(i).filepath());
            } else {
                status = telux::common::Status::FAILED;
                error = telux::common::ErrorCode::NO_MEMORY;
                break;
            }
        }

        ::telStub::RefreshParams* refreshs = response->mutable_refreshs();
        refreshs->set_sessiontype(request->refreshs().sessiontype());
        refreshs->set_aid(request->refreshs().aid());
    } while (0);

    response->set_error(static_cast<commonStub::ErrorCode>(error));
    response->set_delay(cbDelay);
    response->set_status(static_cast<commonStub::Status>(status));
    return ::grpc::Status::OK;
}

void CardManagerServerImpl::updateSimRefreshStage(int slotId, CardRefreshStage newStage,
    uint32_t delayMs, bool checkPendingUserAllow, bool checkPendingUserComplete) {
    LOG(DEBUG, __FUNCTION__, " slotId ", slotId, ", newStage ", static_cast<int>(newStage),
        ", delayMs ", delayMs, ", checkPendingUserAllow", +checkPendingUserAllow,
            ", checkPendingUserComplete ", checkPendingUserComplete);
    auto f = std::async(std::launch::async, [this, slotId, newStage, delayMs,
        checkPendingUserAllow, checkPendingUserComplete]() {
        if (delayMs > 0) {
            std::unique_lock<std::mutex> lck(mutex_);
            cv_.wait_for(lck, std::chrono::milliseconds(delayMs));
        }
        if (exit_) {
            LOG(INFO, " Abort updateSimRefreshStage due to exiting");
            return;
        }
        if ((checkPendingUserAllow && (refreshEvtMap_[slotId].pendingAllow == 0)) ||
            (checkPendingUserComplete && (refreshEvtMap_[slotId].pendingComplete == 0))) {
            LOG(INFO, " Cancel. pendingAllow ", refreshEvtMap_[slotId].pendingAllow,
                " Cancel. pendingComplete ", refreshEvtMap_[slotId].pendingComplete);
            return;
        }

        ::telStub::RefreshEvent refreshEvt;
        {
            std::lock_guard<std::mutex> lck(mutex_);
            if (static_cast<int>(refreshEvtMap_[slotId].refreshEvent.stage()) >=
                static_cast<int>(ENDED_WITH_SUCCESS)) {
                LOG(DEBUG, "simrefresh ignore setting newStage ", static_cast<int>(newStage));
               return;
            }
            if (ENDED_WITH_FAILURE == newStage) {
                refreshEvtMap_[slotId].pendingAllow = 0;
                refreshEvtMap_[slotId].pendingComplete = 0;
                cv_.notify_all();
            }
            refreshEvtMap_[slotId].refreshEvent.set_stage(
                static_cast<::telStub::RefreshStage>(newStage));
            refreshEvt = refreshEvtMap_[slotId].refreshEvent;
            LOG(DEBUG, "simrefresh slotId ", slotId, ", newStage ", static_cast<int>(newStage));
        }

        ::eventService::EventResponse evt;
        evt.mutable_any()->PackFrom(refreshEvt);
        evt.set_filter(telux::tel::TEL_CARD_FILTER);
        //posting the event to EventService event queue
        EventService::getInstance().updateEventQueue(evt);

        if (newStage == STARTING) {
            if (refreshEvtMap_[slotId].pendingComplete > 0 &&
                requireConfirmComplete(newStage,
                static_cast<telux::tel::RefreshMode>(refreshEvtMap_[slotId].refreshEvent.mode()),
                static_cast<telux::tel::SessionType>(
                    refreshEvtMap_[slotId].refreshEvent.refreshs().sessiontype()))) {
                /*require client confirm complete,  fail if timeout.*/
                updateSimRefreshStage(slotId, ENDED_WITH_FAILURE,
                    REFRESH_USER_COMPLETE_TIMEOUT_MS, false, true);
            } else {
                /*do not require client confirm complete, go ahead change to next stage.*/
                updateSimRefreshStage(slotId, ENDED_WITH_SUCCESS, DEFAULT_DELAY, false, false);
            }
        }
    }).share();
    taskQ_.add(f);
}

int CardManagerServerImpl::getSlotBySessionType(telux::tel::SessionType st) {
    switch (st) {
        case telux::tel::SessionType::PRIMARY:
        case telux::tel::SessionType::NONPROVISIONING_SLOT_1:
        case telux::tel::SessionType::CARD_ON_SLOT_1:
            return SLOT_1;
        case telux::tel::SessionType::SECONDARY:
        case telux::tel::SessionType::NONPROVISIONING_SLOT_2:
        case telux::tel::SessionType::CARD_ON_SLOT_2:
            return SLOT_2;
        default:
            LOG(ERROR, __FUNCTION__, " invalid sessionType ", static_cast<int>(st));
    }
    return INVALID_SLOT_ID;
}

bool CardManagerServerImpl::requireConfirmComplete(const CardRefreshStage stage,
    const telux::tel::RefreshMode mode, const telux::tel::SessionType st) {
    if (stage != STARTING) {
        return false;
    }

    /*FCN / Init + FCN refresh mode for GW session type, and
      non RESET refresh mode for noprovisioning session type requires client confirm*/
    if (((mode == telux::tel::RefreshMode::INIT_FCN || mode == telux::tel::RefreshMode::FCN) &&
        (st == telux::tel::SessionType::PRIMARY || st == telux::tel::SessionType::SECONDARY))
        ||
        (mode != telux::tel::RefreshMode::RESET &&
            (telux::tel::SessionType::NONPROVISIONING_SLOT_1 == st ||
            telux::tel::SessionType::NONPROVISIONING_SLOT_2 == st))) {
        /*In a state require client confirm complete*/
        LOG(DEBUG, __FUNCTION__, " yes, mode ", static_cast<int>(mode),
            ", st ", static_cast<int>(st));
        return true;
    }

    LOG(DEBUG, __FUNCTION__, " no, mode ", static_cast<int>(mode), ", st ", static_cast<int>(st));
    return false;
}

bool CardManagerServerImpl::clientSimRefreshInfoPresent(
    std::vector <clientSimRefreshPref>& vector, const clientSimRefreshPref&  entry) {

    for (std::vector<clientSimRefreshPref>::iterator it = vector.begin();
        it != vector.end();) {
        if (it->clientId == entry.clientId && it->phoneId == entry.phoneId &&
            it->sessionAid.sessionType == entry.sessionAid.sessionType &&
            it->sessionAid.aid == entry.sessionAid.aid) {
            return true;
        }
        ++it;
    }
    LOG(DEBUG, __FUNCTION__, " not found. phoneId ", entry.phoneId, ", sessionType",
        static_cast<int>(entry.sessionAid.sessionType), " aid ", entry.sessionAid.aid);
    return false;
}

telux::common::ErrorCode CardManagerServerImpl::updateClientSimRefresh(
    std::vector <clientSimRefreshPref>& vector, const clientSimRefreshPref& usrPref, bool enable) {
    telux::common::ErrorCode res = telux::common::ErrorCode::ALREADY;
    bool found = false;
    for (std::vector<clientSimRefreshPref>::iterator it = vector.begin();
        it != vector.end();) {
        if (it->clientId == usrPref.clientId && it->phoneId == usrPref.phoneId &&
            it->sessionAid.sessionType == usrPref.sessionAid.sessionType &&
            it->sessionAid.aid == usrPref.sessionAid.aid) {
            found = true;
            if (not enable) {
                vector.erase(it);
                LOG(DEBUG, __FUNCTION__, " erase entry from vector.");
                res = telux::common::ErrorCode::SUCCESS;
            } else {
                LOG(ERROR, __FUNCTION__, " ALREADY enabled.");
            }
            break;
        }
        ++it;
    }
    if (not found) {
        if (enable) {
            LOG(DEBUG, __FUNCTION__, " push back entry to vector.");
            vector.push_back(usrPref);
            res = telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(ERROR, __FUNCTION__, " ALREADY disabled.");
        }
    }
    return res;
}

template <typename T>
void CardManagerServerImpl::getClientInfoFromRpc(const T* rpcMsg, clientSimRefreshPref& client) {
    client.clientId = rpcMsg->identifier();
    client.phoneId  = rpcMsg->phone_id();
    client.sessionAid.sessionType = static_cast<telux::tel::SessionType>(
        rpcMsg->refreshs().sessiontype());
    client.sessionAid.aid =  rpcMsg->refreshs().aid();

    LOG(DEBUG, __FUNCTION__, " phoneId ", client.phoneId, ", sessionType ",
        static_cast<uint32_t>(client.sessionAid.sessionType),
        ", aid ", client.sessionAid.aid);
}
