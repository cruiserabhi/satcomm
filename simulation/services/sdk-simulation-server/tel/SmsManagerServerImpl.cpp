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

#include "SmsManagerServerImpl.hpp"
#include "libs/tel/Helper.hpp"
#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"
#include <telux/common/DeviceConfig.hpp>

#define JSON_PATH1 "system-state/tel/ISmsManagerStateSlot1.json"
#define JSON_PATH2 "system-state/tel/ISmsManagerStateSlot2.json"

#define JSON_PATH3 "api/tel/ISmsManagerSlot1.json"
#define JSON_PATH4 "api/tel/ISmsManagerSlot2.json"

#define JSON_PATH5 "system-state/tel/ISubscriptionManagerState.json"

#define TEL_SMS_MANAGER "ISmsManager"
#define INCOMING_SMS_EVENT "incomingsms"
#define MEMORY_FULL_EVENT "memoryfull"
#define SLOT_1 1
#define SLOT_2 2

SmsManagerServerImpl::SmsManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    readJson();
}

grpc::Status SmsManagerServerImpl::readJson() {
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
    //Api system state
    jsonObjSystemStateSlot_[SLOT_1] = rootObjSystemStateSlot1_;
    jsonObjSystemStateSlot_[SLOT_2] = rootObjSystemStateSlot2_;
    jsonObjSystemStateFileName_[SLOT_1] = JSON_PATH1;
    jsonObjSystemStateFileName_[SLOT_2] = JSON_PATH2;
    //Api response
    jsonObjApiResponseSlot_[SLOT_1] = rootObjApiResponseSlot1_;
    jsonObjApiResponseSlot_[SLOT_2] = rootObjApiResponseSlot2_;
    jsonObjApiResponseFileName_[SLOT_1] = JSON_PATH3;
    jsonObjApiResponseFileName_[SLOT_2] = JSON_PATH4;
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();

    return grpc::Status::OK;
}

bool SmsManagerServerImpl::isCallbackNeeded(Json::Value rootObj, std::string apiname) {
    int value = rootObj[TEL_SMS_MANAGER][apiname]["callbackDelay"].asInt();
    bool isCallback = true;
    if (value == -1) {
        isCallback = false;
    }
    return isCallback;
}

grpc::Status SmsManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    rootObj = jsonObjApiResponseSlot_[phoneId];
    if(readStatus.ok()) {
        int cbDelay = rootObj[TEL_SMS_MANAGER]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus =
            rootObj[TEL_SMS_MANAGER]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);

        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
        if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::vector<std::string> filters = {"tel_sms"};
            auto &serverEventManager = ServerEventManager::getInstance();
            serverEventManager.registerListener(shared_from_this(), filters);
        }
        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status SmsManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {

    Json::Value rootObj;
    int phoneId = request->phone_id();
    rootObj = jsonObjApiResponseSlot_[phoneId];
    std::string srvStatus = rootObj[TEL_SMS_MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

void SmsManagerServerImpl::getJsonForSystemData(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjSystemStateFileName_[phoneId];
    rootObj = jsonObjSystemStateSlot_[phoneId];
}

void SmsManagerServerImpl::getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjApiResponseFileName_[phoneId];
    rootObj = jsonObjApiResponseSlot_[phoneId];
}

grpc::Status SmsManagerServerImpl::SetSmscAddress(ServerContext* context,
    const ::telStub::SetSmscAddressRequest* request,
    telStub::SetSmscAddressReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string smscAddress = request->number();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        telux::common::Status status;
        telux::common::ErrorCode error;
        int delay;
        std::string apiname = "setSmscAddress";
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        CommonUtils::getValues(jsonObjApiResponse, TEL_SMS_MANAGER, apiname, status, error, delay);
        if(status == telux::common::Status::SUCCESS) {
            rootObj[TEL_SMS_MANAGER]["setSmscAddress"]["smscAddress"] = smscAddress;
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
        }

        //Create response
        bool isCallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(delay);
        response->set_status(static_cast<commonStub::Status>(status));
    }
    return readStatus;
}

grpc::Status SmsManagerServerImpl::GetSmscAddress(ServerContext* context,
    const ::telStub::GetSmscAddressRequest* request,
    telStub::GetSmscAddressReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    std::string smscAddress = "";
    std::string apiname = "requestSmscAddress";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        telux::common::Status status;
        telux::common::ErrorCode error;
        int delay;

        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        CommonUtils::getValues(jsonObjApiResponse, TEL_SMS_MANAGER, apiname,
            status, error, delay );
        if(status == telux::common::Status::SUCCESS) {
            smscAddress = rootObj[TEL_SMS_MANAGER]\
                ["setSmscAddress"]["smscAddress"].asString();
        }
        //Create response
        bool isCallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_smsc_address(smscAddress);
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(delay);
        response->set_status(static_cast<commonStub::Status>(status));
    }
    return readStatus;
}

grpc::Status SmsManagerServerImpl::RequestSmsMessageList(ServerContext* context,
    const ::telStub::RequestSmsMessageListRequest* request,
    telStub::RequestSmsMessageListReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    std::string apiname = "requestSmsMessageList";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        std::string jsonObjApiResponseFileName = "";
        Json::Value jsonObjApiResponse;
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        telStub::SmsTagType::TagType tag = request->tag_type();
        telux::tel::SmsTagType type = static_cast<telux::tel::SmsTagType>(tag);
        std::vector<telux::tel::SmsMetaInfo> infos;
        telux::common::Status status;
        telux::common::ErrorCode error;
        int delay;
        CommonUtils::getValues(jsonObjApiResponse, TEL_SMS_MANAGER, apiname, status, error, delay);

        if(status == telux::common::Status::SUCCESS) {
            int size = getSMSStorage(phoneId);
            for (int i = 0; i < size; i++) {
                telux::tel::SmsTagType typeInput = type;
                telux::tel::SmsMetaInfo Info;
                if (type == telux::tel::SmsTagType::UNKNOWN) {
                    Info.msgIndex =
                    rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][i]\
                        ["smsMetaInfo_msgIndex"].asInt();
                    std::string tagType =
                    rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][i]\
                        ["smsMetaInfo_tagType"].asString();
                    Info.tagType = Helper::getTagType(tagType);
                    infos.push_back(Info);
                } else if ((type == telux::tel::SmsTagType::MT_READ) ||
                            (type == telux::tel::SmsTagType::MT_NOT_READ)) {
                    std::string tag =
                    rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][i]\
                        ["smsMetaInfo_tagType"].asString();
                    telux::tel::SmsTagType tagType = Helper::getTagType(tag);
                    if(typeInput == tagType ) {
                        Info.msgIndex =
                        rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][i]\
                            ["smsMetaInfo_msgIndex"].asInt();
                        Info.tagType =  tagType;
                        infos.push_back(Info);
                    }
                } else {
                    status = telux::common::Status::NOTSUPPORTED;
                }
            }
        }
        //Create response

        bool isCallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(isCallback);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(delay);

        for(auto &it : infos) {
            telStub::SmsMetaInfo *result = response->add_meta_info();
            if(error == telux::common::ErrorCode::SUCCESS) {
                result->set_msg_index(it.msgIndex);
                result->set_tag_type(static_cast<telStub::SmsTagType_TagType>(it.tagType));
                LOG(DEBUG, __FUNCTION__, "Sms meta index : ", static_cast<int>(it.msgIndex),
                "Sms meta tag : ", static_cast<int>(it.tagType));
            } else {
                result->set_msg_index(0);
                result->set_tag_type(telStub::SmsTagType_TagType_UNKNOWN);
            }
        }
    }
    return readStatus;
    }

int SmsManagerServerImpl::getSMSStorage(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    Json::Value rootObj;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    int size = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"].size();
    LOG(DEBUG, __FUNCTION__, "size of SmsDatabaseStorage is", size);
    return size;
}

grpc::Status SmsManagerServerImpl::ReadMessage(ServerContext *context,
    const telStub::ReadMessageRequest *request, telStub::ReadMessageReply *response) {

    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    std::string apiname = "readMessage";
    Json::Value rootObj;
    int phoneId = request->phone_id();
    uint32_t messageIndex = request->msg_index();
    LOG(ERROR, __FUNCTION__, " MsgIndex ", messageIndex );
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        std::string jsonObjApiResponseFileName = "";
        Json::Value jsonObjApiResponse;
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        uint32_t size = getSMSStorage(phoneId);
        uint32_t index_for_db;
        SmsMsg msg;
        telux::common::Status status;
        telux::common::ErrorCode error;
        int delay;
        bool found = false;
        CommonUtils::getValues(jsonObjApiResponse, TEL_SMS_MANAGER, apiname, status, error, delay);

        if(status == telux::common::Status::SUCCESS) {
            for (index_for_db = 0; index_for_db < size; index_for_db++) {
                uint32_t Id = -1;
                Id = std::stoi(rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index_for_db]\
                    ["smsMetaInfo_msgIndex"].asString());
                LOG(DEBUG, __FUNCTION__, " MsgIndex ", Id);
                if(Id == messageIndex ) {
                    LOG(DEBUG, __FUNCTION__, " Fetching data at MsgIndex ", index_for_db);
                    parseMessageAtIndex(phoneId, index_for_db, msg);
                    found = true;
                    break;
                }
            }
            if (!found) {
                LOG(ERROR, __FUNCTION__, " MsgIndex ", messageIndex ," not found");
                error = telux::common::ErrorCode::INVALID_INDEX;
            }
        }
        bool isCallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(isCallback);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_delay(delay);
        //Create response
        telStub::SmsMessage msg_;
        response->set_error((static_cast<commonStub::ErrorCode>(error)));
        if(error == telux::common::ErrorCode::SUCCESS) {
            msg_.set_text(msg.text);
            msg_.set_sender(msg.sender);
            msg_.set_receiver(msg.receiver);
            msg_.set_encoding(static_cast<telStub::MessageAttributes::SmsEncoding>(msg.encoding));
            msg_.set_pdu(msg.pdu);
            msg_.set_pdu_buff(msg.pduBuffer);
            msg_.set_messageinforef_no( msg.messageInfoRefNumber);
            msg_.set_messageinfono_of_segments(msg.messageInfoSegments);
            msg_.set_messageinfosegment_no(msg.messageInfoSegmentNumber);
            msg_.set_ismetainfo_valid(msg.isMetaInfoValid);
            msg_.set_msg_index(msg.msgIndex);
            msg_.set_tag_type(static_cast<telStub::SmsTagType_TagType>(msg.tagType));
            *response->mutable_sms_message() = msg_;
        } else {
            msg_.set_text("");
            msg_.set_sender("");
            msg_.set_receiver("");
            msg_.set_encoding(static_cast<telStub::MessageAttributes::SmsEncoding>
                (telux::tel::SmsEncoding::UNKNOWN));
            msg_.set_pdu("");
            msg_.set_pdu_buff("");
            msg_.set_messageinforef_no(0);
            msg_.set_messageinfono_of_segments(0);
            msg_.set_messageinfosegment_no(0);
            msg_.set_ismetainfo_valid(false);
            msg_.set_msg_index(0);
            msg_.set_tag_type
                (static_cast<telStub::SmsTagType_TagType>(telux::tel::SmsTagType::UNKNOWN));
            *response->mutable_sms_message() = msg_;
        }
    }
    return readStatus;
}

void SmsManagerServerImpl::parseMessageAtIndex(int phoneId, int index, SmsMsg& msg ) {
    LOG(DEBUG, __FUNCTION__,"Index", index );
    std::string jsonfilename = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        msg.text = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index]["text"].asString();
        LOG(DEBUG, __FUNCTION__,"text_", msg.text );
        msg.sender = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index]["sender"].asString();
        msg.receiver = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index]\
            ["receiver"].asString();
        std::string encoding = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index]\
            ["encoding"].asString();
        msg.encoding = Helper::getencodingMethod(encoding);
        msg.pdu = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index]["pdu"].asString();
        msg.msgIndex = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index]\
            ["smsMetaInfo_msgIndex"].asInt();
        msg.messageInfoRefNumber = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"]\
            [index]["messagePartInfo_refNumber"].asInt();
        msg.messageInfoSegments = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"]\
            [index]["messagePartInfo_numberOfSegments"].asInt();
        msg.messageInfoSegmentNumber = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"]\
            [index]["messagePartInfo_segmentNumber"].asInt();
        LOG(DEBUG, __FUNCTION__,"messagePartInfo_refNumber", msg.messageInfoRefNumber );
        LOG(DEBUG, __FUNCTION__,"messagePartInfo_numberOfSegments", msg.messageInfoSegments );
        LOG(DEBUG, __FUNCTION__,"messagePartInfo_segmentNumber", msg.messageInfoSegmentNumber );
        std::string tagType = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"]\
            [index]["smsMetaInfo_tagType"].asString();
        msg.tagType = Helper::getTagType(tagType );
        msg.isMetaInfoValid = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"]\
            [index]["isMetaInfoValid"].asBool();
        msg.pduBuffer = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index]["rawPdu"].asString();
    }
}

grpc::Status SmsManagerServerImpl::DeleteMessage(ServerContext *context,
    const telStub::DeleteMessageRequest *request, telStub::DeleteMessageRequestReply *response) {

    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    std::string apiname = "deleteMessage";
    Json::Value rootObj;
    telux::common::ErrorCode error;
    telux::common::Status status;
    int delay;
    int phoneId = request->phone_id();
    uint32_t messageIndex = request->msg_index();
    telStub::SmsTagType::TagType tag = request->tag_type();
    telux::tel::SmsTagType tagType = static_cast<telux::tel::SmsTagType>(tag);
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        std::string jsonObjApiResponseFileName = "";
        Json::Value jsonObjApiResponse;
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        telStub::DelType::DeleteType deleteType = request->del_type();
        telux::tel::DeleteType deltype = static_cast<telux::tel::DeleteType>(deleteType);
        CommonUtils::getValues(jsonObjApiResponse, TEL_SMS_MANAGER, apiname, status, error, delay);

        if(status == telux::common::Status::SUCCESS) {
            uint32_t size = getSMSStorage(phoneId);
            uint32_t index_for_db;
            int adjust_index = 0;
            bool delAtIndex = true;
            std::vector<int> indexstodelete;
            for (index_for_db = 0; index_for_db < size; index_for_db++) {
                if(deltype == telux::tel::DeleteType::DELETE_MESSAGES_BY_TAG) {
                    std::string tag =
                    rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index_for_db]\
                        ["smsMetaInfo_tagType"].asString().c_str();
                    if (Helper::getTagType(tag) == tagType) {
                        int tmp = index_for_db - adjust_index;
                        indexstodelete.push_back(tmp);
                        adjust_index++;
                    }
                } else if (deltype == telux::tel::DeleteType::DELETE_MSG_AT_INDEX) {
                    uint32_t Id =
                    rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index_for_db]\
                        ["smsMetaInfo_msgIndex"].asInt();
                    if(Id == messageIndex) {
                        indexstodelete.push_back(index_for_db);
                        break;
                    }
                } else if (deltype == telux::tel::DeleteType::DELETE_ALL) {
                    Json::Value newRoot;
                    rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"] = newRoot["SmsDatabaseStorage"];
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    delAtIndex = false;
                } else {
                    status = telux::common::Status::NOTSUPPORTED;
                }
            }
            if(delAtIndex) {
                error = deletedSmsatIndex(phoneId, indexstodelete);
            }
        }
        //Create respone

        bool isCallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(isCallback);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_delay(delay);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
    }
    return readStatus;
}

telux::common::ErrorCode SmsManagerServerImpl::deletedSmsatIndex(int phoneId,
    std::vector<int> index) {
    std::string jsonfilename = "";
    Json::Value rootObj;
    ErrorCode error;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        LOG(DEBUG, __FUNCTION__, " No of entries to be deleted are  ", index.size());
        for (auto itr = index.begin(); itr !=index.end(); itr++) {
            Json::Value newRoot;
            error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
            if (error != ErrorCode::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " Reading JSON File failed!");
            }
            int numprofiles = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"].size();
            LOG(DEBUG, __FUNCTION__, numprofiles);

            int newCount = 0;
            for (int i=0; i < numprofiles; i++) {
                if(i != *itr) {
                    newRoot["SmsDatabaseStorage"][newCount] =
                        rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][i];
                    newCount++;
                } else {
                    LOG(DEBUG, __FUNCTION__, " The index deleting currently is  ", *itr);
                }
            }
            rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"] = newRoot["SmsDatabaseStorage"];
            error = JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
        }
    }
    return error;
}
grpc::Status SmsManagerServerImpl::SetPreferredStorage(ServerContext *context,
    const telStub::SetPreferredStorageRequest *request,
    telStub::SetPreferredStorageReply *response) {

    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    std::string apiname = "setPreferredStorage";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    telux::common::ErrorCode error;
    telux::common::Status status;
    int delay;
    telStub::StorageType::Type storageType = request->storage_type();
    telux::tel::StorageType type = static_cast<telux::tel::StorageType>(storageType);
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        CommonUtils::getValues(jsonObjApiResponse, TEL_SMS_MANAGER, apiname, status, error, delay);

        if(status == telux::common::Status::SUCCESS) {
            std::string storage = Helper::storageTypeToString(type);
            rootObj[TEL_SMS_MANAGER]["setPreferredStorage"]["storageType"] = storage;
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
        }

        //Create response
        bool isCallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(delay);
        response->set_status(static_cast<commonStub::Status>(status));
    }
    return readStatus;
}

grpc::Status SmsManagerServerImpl::RequestPreferredStorage(ServerContext *context,
    const telStub::RequestPreferredStorageRequest *request,
    telStub::RequestPreferredStorageReply *response) {

    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    telux::common::ErrorCode error;
    telux::common::Status status;
    std::string apiname = "requestPreferredStorage";
    int delay;
    telux::tel::StorageType type = telux::tel::StorageType::UNKNOWN;
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        CommonUtils::getValues(jsonObjApiResponse, TEL_SMS_MANAGER, apiname, status, error, delay);

        if(status == telux::common::Status::SUCCESS) {
            std::string storage = rootObj[TEL_SMS_MANAGER]["setPreferredStorage"]\
                ["storageType"].asString();
            type = Helper::getstorageType(storage);
        }

        //Create response
        telStub::StorageType::Type storageType = static_cast<telStub::StorageType::Type>(type);
        response->set_storage_type(storageType);
        bool isCallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(delay);
        response->set_status(static_cast<commonStub::Status>(status));
    }

    return readStatus;
}

grpc::Status SmsManagerServerImpl::SetTag(ServerContext *context,
    const telStub::SetTagRequest *request, telStub::SetTagReply *response) {

    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    std::string apiname = "setTag";
    Json::Value rootObj;
    telux::common::ErrorCode error;
    telux::common::Status status;
    int delay;
    bool foundMsgIndex = true;
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        std::string jsonObjApiResponseFileName = "";
        Json::Value jsonObjApiResponse;
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        uint32_t messageIndex = request->msg_index();
        telStub::SmsTagType::TagType tag = request->tag_type();
        telux::tel::SmsTagType tagType = static_cast<telux::tel::SmsTagType>(tag);
        CommonUtils::getValues(jsonObjApiResponse, TEL_SMS_MANAGER, apiname, status, error, delay);

        if(status == telux::common::Status::SUCCESS) {
            uint32_t size = getSMSStorage(phoneId);
            uint32_t index_for_db;
            if (messageIndex > size) {
                LOG(ERROR, __FUNCTION__, " MsgIndex ", messageIndex ," not found");
                status = telux::common::Status::INVALIDPARAM;
                foundMsgIndex = false;
            }
            if (foundMsgIndex) {
                for (index_for_db = 0; index_for_db < size; index_for_db++) {
                    uint32_t Id = -1;
                    Id = std::stoi(rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"]\
                        [index_for_db]["smsMetaInfo_msgIndex"].asString());
                    LOG(DEBUG, __FUNCTION__, " messageIndex ", Id);
                    if(Id == messageIndex ) {
                        rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index_for_db]\
                            ["smsMetaInfo_tagType"] = Helper::tagTypeToString(tagType);
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                    }
                }
            }
        }
        //Create response
        bool isCallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(delay);
        response->set_status(static_cast<commonStub::Status>(status));
    }
    return readStatus;
}

grpc::Status SmsManagerServerImpl::RequestStorageDetails(ServerContext *context,
    const telStub::RequestStorageDetailsRequest *request,
    telStub::RequestStorageDetailsReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiname = "requestStorageDetails";
    telux::common::ErrorCode error;
    telux::common::Status status;
    int delay;
    uint32_t availableCount = 0;
    uint32_t maxCount = 0;
    int size = 0;
    int phoneId = request->phone_id();
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        bool isCallback = isCallbackNeeded(jsonObjApiResponse, apiname);
        CommonUtils::getValues(jsonObjApiResponse, TEL_SMS_MANAGER, apiname, status, error, delay);

        if(status == telux::common::Status::SUCCESS) {
            size = getSMSStorage(phoneId);
            maxCount =
            jsonObjApiResponse[TEL_SMS_MANAGER]["requestStorageDetails"]\
                ["requestStorageDetailsCbMaxCount"].asInt();
            availableCount = maxCount - size;
        }

        //Create response
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(delay);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_max_count(maxCount);
        response->set_available_count(availableCount);
    }
    return readStatus;
}

grpc::Status SmsManagerServerImpl::GetMessageAttributes(ServerContext *context,
    const telStub::GetMessageAttributesRequest *request,
    telStub::GetMessageAttributesReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    Json::Value rootObj;
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        std::string val =
            rootObj[TEL_SMS_MANAGER]["calculateMessageAttributes"]\
            ["messageAttributes_encoding"].asString();
        telux::tel::SmsEncoding encoding = Helper::getencodingMethod(val);
        int numberOfSegments =
        rootObj[TEL_SMS_MANAGER]["calculateMessageAttributes"]\
            ["messageAttributes_numberOfSegments"].asInt();
        int segmentSize =
        rootObj[TEL_SMS_MANAGER]["calculateMessageAttributes"]\
            ["messageAttributes_segmentSize"].asInt();
        int numberOfCharsLeftInLastSegment =
        rootObj[TEL_SMS_MANAGER]["calculateMessageAttributes"]\
            ["messageAttributes_numberOfCharsLeftInLastSegment"].asInt();

        //Create response
        telStub::MessageAttributes msgAttr;
        msgAttr.set_encoding(static_cast<telStub::MessageAttributes::SmsEncoding>(encoding));
        msgAttr.set_number_of_segments(numberOfSegments);
        msgAttr.set_segment_size(segmentSize);
        msgAttr.set_number_of_chars_left_in_last_segment(numberOfCharsLeftInLastSegment);
        *response->mutable_message_attribute() = msgAttr;
    }
    return readStatus;
}

grpc::Status SmsManagerServerImpl::SendSmsWithoutSmsc(ServerContext *context,
    const telStub::SendSmsWithoutSmscRequest *request,
    telStub::SendSmsWithoutSmscReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    Json::Value rootObj;
    int phoneId = request->phone_id();
    std::string tmp;
    telux::common::Status status;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonfilename, rootObj);

        int noOfSegments = rootObj[TEL_SMS_MANAGER]["sendSmsDeprecated"][0]\
            ["numberOfSegments"].asInt();

        tmp = rootObj[TEL_SMS_MANAGER]["sendSmsDeprecated"][0]["status"].asString();
        status = CommonUtils::mapStatus(tmp);

        tmp = rootObj[TEL_SMS_MANAGER]["sendSmsDeprecated"][1]\
            ["sentCallbackErrorCode"].asString();
        telux::common::ErrorCode sentCallbackErrorCode = CommonUtils::mapErrorCode(tmp);
        int sentCallbackDelay = rootObj[TEL_SMS_MANAGER]["sendSmsDeprecated"][1]\
            ["sentCallbackDelay"].asInt();
        std::string refs = rootObj[TEL_SMS_MANAGER]["sendSmsDeprecated"][1]\
            ["sentCallbackMsgRefs"].asString();
        tmp = rootObj[TEL_SMS_MANAGER]["sendSmsDeprecated"][2]\
            ["deliveryCallbackErrorCode"].asString();
        telux::common::ErrorCode deliveryCallbackErrorCode = CommonUtils::mapErrorCode(tmp);
        int deliveryCallbackDelay = rootObj[TEL_SMS_MANAGER]["sendSmsDeprecated"][2]\
            ["deliveryCallbackDelay"].asInt();

        //Create response
        response->set_noofsegments(noOfSegments);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_sentcallback_errorcode
            (static_cast<commonStub::ErrorCode>(sentCallbackErrorCode));
        response->set_sentcallback_callbackdelay(sentCallbackDelay);
        response->set_sentcallback_msgrefs(refs);
        response->set_deliverycallback_errorcode
            (static_cast<commonStub::ErrorCode>(deliveryCallbackErrorCode));
        response->set_deliverycallback_callbackdelay(deliveryCallbackDelay);
    }
    return readStatus;
}

grpc::Status SmsManagerServerImpl::SendSms(ServerContext *context,
    const telStub::SendSmsRequest *request, telStub::SendSmsReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    Json::Value rootObj;
    int phoneId = request->phone_id();
    std::string tmp;
    telStub::Deliverycallback result;
    telux::common::Status status;
    std::vector<SmsDeliveryInfo> infos;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonfilename, rootObj);

        int noOfSegments = rootObj[TEL_SMS_MANAGER]["sendSms"][0]["numberOfSegments"].asInt();

        tmp = rootObj[TEL_SMS_MANAGER]["sendSms"][0]["status"].asString();
        status = CommonUtils::mapStatus(tmp);

        tmp = rootObj[TEL_SMS_MANAGER]["sendSms"][1]["smsResponseCbErrorCode"].asString();
        telux::common::ErrorCode sentCallbackErrorCode = CommonUtils::mapErrorCode(tmp);
        int smsResponseCbDelay = rootObj[TEL_SMS_MANAGER]["sendSms"][1]\
            ["smsResponseCbDelay"].asInt();
        std::string refs = rootObj[TEL_SMS_MANAGER]["sendSms"][1]\
            ["smsResponseCbMsgRefs"].asString();

        for (int i = 1; i <= noOfSegments ;i++) {
            SmsDeliveryInfo records;
            std::string error = rootObj[TEL_SMS_MANAGER]["sendSms"][i+1]\
                ["onDeliveryReportErrorCode"].asString();
            records.errorCode  = CommonUtils::mapErrorCode(error);
            records.msgRef = rootObj[TEL_SMS_MANAGER]["sendSms"][i+1]\
                ["onDeliveryReportMsgRef"].asInt();
            records.cbDelay = rootObj[TEL_SMS_MANAGER]["sendSms"][i+1]\
                ["onDeliveryReportCallbackDelay"].asInt();
            infos.emplace_back(records);
        }
        for(auto &it : infos) {
            telStub::Deliverycallback *result = response->add_records();
            result->set_ondeliveryreport_errorcode
                (static_cast<commonStub::ErrorCode>(it.errorCode));
            result->set_ondeliveryreportmsgref(it.msgRef);
            result->set_deliverycallbackdelay(it.cbDelay);
        }
        response->set_noofsegments(noOfSegments);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_smsresponsecb_errorcode
            (static_cast<commonStub::ErrorCode>(sentCallbackErrorCode));
        response->set_smsresponsecb_callbackdelay(smsResponseCbDelay);
        response->set_sentcallback_msgrefs(refs);
    }
    return readStatus;
}

grpc::Status SmsManagerServerImpl::SendRawSms(ServerContext *context,
    const telStub::SendRawSmsRequest *request, telStub::SendRawSmsReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    Json::Value rootObj;
    telux::common::Status status;
    int phoneId = request->phone_id();
    int size = request->size();
    std::string tmp;
    telStub::Deliverycallback result;
    std::vector<SmsDeliveryInfo> infos;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonfilename, rootObj);

        std::string recieverAddress = rootObj[TEL_SMS_MANAGER]["sendRawSms"][0]\
            ["receiverAddress"].asString();

        tmp = rootObj[TEL_SMS_MANAGER]["sendRawSms"][0]["status"].asString();
        status = CommonUtils::mapStatus(tmp);

        tmp = rootObj[TEL_SMS_MANAGER]["sendRawSms"][1]["smsResponseCbErrorCode"].asString();
        telux::common::ErrorCode sentCallbackErrorCode = CommonUtils::mapErrorCode(tmp);
        int smsResponseCbDelay = rootObj[TEL_SMS_MANAGER]["sendRawSms"][1]\
            ["smsResponseCbDelay"].asInt();
        std::string refs = rootObj[TEL_SMS_MANAGER]["sendRawSms"][1]\
            ["smsResponseCbMsgRefs"].asString();

        for (int i = 1; i <= size ;i++) {
            SmsDeliveryInfo records;
            std::string error = rootObj[TEL_SMS_MANAGER]["sendRawSms"][i+1]\
                ["onDeliveryReportErrorCode"].asString();
            records.errorCode  = CommonUtils::mapErrorCode(error);
            records.msgRef = rootObj[TEL_SMS_MANAGER]["sendRawSms"][i+1]\
                ["onDeliveryReportMsgRef"].asInt();
            records.cbDelay = rootObj[TEL_SMS_MANAGER]["sendRawSms"][i+1]\
                ["onDeliveryReportCallbackDelay"].asInt();
            infos.emplace_back(records);
        }

        for(auto &it : infos) {
            telStub::Deliverycallback *result = response->add_records();
            result->set_ondeliveryreport_errorcode
            (static_cast<commonStub::ErrorCode>(it.errorCode));
            result->set_ondeliveryreportmsgref(it.msgRef);
            result->set_deliverycallbackdelay(it.cbDelay);
        }
        response->set_reciever_address(recieverAddress);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_smsresponsecb_errorcode
            (static_cast<commonStub::ErrorCode>(sentCallbackErrorCode));
        response->set_smsresponsecb_callbackdelay(smsResponseCbDelay);
        response->set_sentcallback_msgrefs(refs);
    }
    return readStatus;
}

void SmsManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__,"String is ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__,"Token is ", token );
    if ( INCOMING_SMS_EVENT == token) {
        handleIncomingSms(event);
    } else if( MEMORY_FULL_EVENT == token) {
        handleMemoryFullEvent(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void SmsManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == "tel_sms") {
        onEventUpdate(message.event());
    }
}

void SmsManagerServerImpl::handleMemoryFullEvent(std::string eventParams) {
    int slotId;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        slotId = 1;
    } else {
        try {
            slotId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    if(slotId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", slotId);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);
    // Fetch storage type
    std::string input;
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    input =  token;
    if(token == "") {
        LOG(INFO, __FUNCTION__, "Storage type not passed, assuming UNKNOWN");
        input = "UNKNOWN";
    }

    ::telStub::memoryFullEvent memoryFullEvent;
    ::eventService::EventResponse anyResponse;

    telux::tel::StorageType type = Helper::getstorageType(input);
    memoryFullEvent.set_phone_id(slotId);
    memoryFullEvent.set_storage_type(static_cast<telStub::StorageType::Type>(type));
    anyResponse.set_filter("tel_sms");
    anyResponse.mutable_any()->PackFrom(memoryFullEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);

}

int SmsManagerServerImpl::getNewSmsIndex(int phoneId) {
    std::string jsonfilename = "";
    Json::Value rootObj;
    int currentIndex;
    int nextIndex;
    bool flag = false;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    /* If sms message of index 0 is not present return index 0
     * as it's the first missing element of database.
     */
    if(rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][0]["smsMetaInfo_msgIndex"].asInt() == 0) {
        int size = getSMSStorage(phoneId);
        for (int i = 0; i < size - 1; i++) {
            currentIndex = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][i]\
                ["smsMetaInfo_msgIndex"].asInt();
            nextIndex = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][i+1]\
                ["smsMetaInfo_msgIndex"].asInt();
            if(nextIndex != currentIndex + 1) {
                flag = true;
                break;
            }
        }
        if(flag != true) {
            LOG(DEBUG, __FUNCTION__, "Return Current index is ", size);
            return size;
        } else {
            LOG(DEBUG, __FUNCTION__, "Current index is ", currentIndex + 1 );
            return currentIndex + 1;
        }
    } else {
        return 0;
    }
}

void SmsManagerServerImpl::handleIncomingSms(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    int numberOfSegments;
    int refNumber;
    int segmentNumber;
    int msgIndex;
    std::string tagType;
    std::string encoding;
    bool isMetaInfoValid;
    std::string pdu;
    std::string rawPdu;
    std::string receiver;
    std::string sender;
    std::string text;

    /* Fetch the slotId */

    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);

    /* Fetch the numberOfSegments */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The numberOfSegments is not passed!");
    } else {
        try {
            numberOfSegments = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched numberofsegments is: ", numberOfSegments);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);
    /* Fetch refNumber */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The refNumber not passed!");
    } else {
        try {
            refNumber = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched refNumber is: ", refNumber);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);
    /* Fetch segmentNumber */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The segmentNumber is not passed!");
    } else {
        try {
            segmentNumber = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched segmentNumber is: ", segmentNumber);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);

    /* Update msgIndex */

    msgIndex = getNewSmsIndex(phoneId);
    LOG(DEBUG, __FUNCTION__, "The fetched msgIndex is: ", msgIndex);

    /* Update tagType */

    tagType = "MT_NOT_READ";
    LOG(DEBUG, __FUNCTION__, "The fetched tagType is: ", tagType );

    /* Fetch encoding */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The encoding is not passed!");
    } else {
        encoding = token;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched encoding is: ", encoding );
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);

    /* Fetch isMetaInfoValid */

   token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The isMetaInfoValid is not passed!");
    } else {
        try {
            int input = std::stoi(token);
            if (input == 1) {
                isMetaInfoValid = true ;
            } else {
                isMetaInfoValid = false ;
            }
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }

    LOG(DEBUG, __FUNCTION__, "The fetched isMetaInfoValid is: ", isMetaInfoValid );
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);

    /* Fetch pdu */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The pdu is not passed!");
    } else {
        pdu = token;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched pdu is: ", pdu );
        LOG(DEBUG, __FUNCTION__, "The leftover string is: ", token);

    /* Fetch receiver */
    Json::Value rootObjSubscription;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObjSubscription, JSON_PATH5);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
    }
    receiver =
        rootObjSubscription["ISubscriptionManager"]["Subscription"][phoneId - 1]\
            ["phoneNumber"].asString();
    LOG(DEBUG, __FUNCTION__, "The Receiver is", receiver);

   /* Fetch sender */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The sender is not passed!");
    } else {
        sender = token;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched sender is: ", sender );
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);

    /* Fetch text */
    LOG(DEBUG, __FUNCTION__, "The text is: ", eventParams);
    if (token == "") {
        LOG(INFO, "The text is not passed!");
    } else {
        text = eventParams;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched text is: ", text );

    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    getJsonForSystemData(phoneId, jsonfilename, rootObj);

    std::string storage = rootObj[TEL_SMS_MANAGER]["setPreferredStorage"]\
        ["storageType"].asString();
    telux::tel::StorageType type = Helper::getstorageType(storage);
    if(type == telux::tel::StorageType::SIM) {
        int size = getSMSStorage(phoneId);
        uint32_t maxCount =
        jsonObjApiResponse[TEL_SMS_MANAGER]["requestStorageDetails"]\
            ["requestStorageDetailsCbMaxCount"].asInt();
        uint32_t availableCount = maxCount - size;
        if(availableCount > 0) {
            Json::Value newSms;
            int currentSMSCount = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"].size();
            LOG(DEBUG, __FUNCTION__,"Current SMS  Count is : ", currentSMSCount);
            newSms["text"] = text;
            newSms["sender"] = sender;
            newSms["receiver"] = receiver;
            newSms["encoding"] = encoding;
            newSms["rawPdu"] = pdu;
            newSms["pdu"] = pdu;
            newSms["messagePartInfo_refNumber"] = refNumber;
            newSms["messagePartInfo_segmentNumber"] = segmentNumber;
            newSms["messagePartInfo_numberOfSegments"] = numberOfSegments;
            newSms["isMetaInfoValid"] = isMetaInfoValid;
            newSms["smsMetaInfo_msgIndex"] =  msgIndex;
            newSms["smsMetaInfo_tagType"] = tagType;
            rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][currentSMSCount] = newSms;
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
            sortDatabase(phoneId, newSms, msgIndex);
        } else {
            LOG(DEBUG, __FUNCTION__, "Memory Full ");
        }
    }
    auto f = std::async(std::launch::async, [this, phoneId, numberOfSegments, refNumber,
        segmentNumber, msgIndex, tagType, encoding, isMetaInfoValid, pdu, receiver,
        sender, text ]() {
            this->triggerIncomingSmsEvent(phoneId, numberOfSegments, refNumber, segmentNumber,
            msgIndex, tagType, encoding, isMetaInfoValid, pdu, receiver, sender, text);
        }).share();
        taskQ_->add(f);
}

void SmsManagerServerImpl::sortDatabase(int phoneId, Json::Value newSms, int index) {
    std::string jsonfilename = "";
    Json::Value rootObj;
    LOG(DEBUG, __FUNCTION__,"Index is : ", index);

    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    int currentSMSCount = rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"].size();
    LOG(DEBUG, __FUNCTION__,"Current SMS  Count is : ", currentSMSCount);
    if(index > currentSMSCount - 1) {
        return;
    } else {
        for (int i = currentSMSCount - 1; i > index ; --i ) {
           rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][i] =
            rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][i - 1];
        }
        rootObj[TEL_SMS_MANAGER]["SmsDatabaseStorage"][index] = newSms;
    }
    JsonParser::writeToJsonFile(rootObj, jsonfilename);
    jsonObjSystemStateSlot_[phoneId] = rootObj;

}

void SmsManagerServerImpl::triggerIncomingSmsEvent(int phoneId, int numberOfSegments,
    int refNumber, int segmentNumber, int msgIndex, std::string tagType, std::string encoding,
    bool isMetaInfoValid, std::string pdu, std::string receiver, std::string sender,
    std::string text) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::SmsMessage SmsMessageEvent;
    ::eventService::EventResponse anyResponse;

    telux::tel::SmsEncoding data = Helper::getencodingMethod(encoding);
    telux::tel::SmsTagType tag = Helper::getTagType(tagType);

    SmsMessageEvent.set_phone_id(phoneId);
    SmsMessageEvent.set_text(text);
    SmsMessageEvent.set_sender(sender);
    SmsMessageEvent.set_receiver(receiver);
    SmsMessageEvent.set_encoding(static_cast<telStub::MessageAttributes::SmsEncoding>(data));
    SmsMessageEvent.set_pdu(pdu);
    SmsMessageEvent.set_messageinforef_no(refNumber);
    SmsMessageEvent.set_messageinfono_of_segments(numberOfSegments);
    SmsMessageEvent.set_messageinfosegment_no(segmentNumber);
    SmsMessageEvent.set_ismetainfo_valid(isMetaInfoValid);
    SmsMessageEvent.set_msg_index(msgIndex);
    SmsMessageEvent.set_tag_type(static_cast<telStub::SmsTagType::TagType>(tag));
    anyResponse.set_filter("tel_sms");
    anyResponse.mutable_any()->PackFrom(SmsMessageEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

grpc::Status SmsManagerServerImpl::IsMemoryFull(ServerContext *context,
    const telStub::IsMemoryFullRequest *request, telStub::IsMemoryFullReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonfilename = "";
    Json::Value rootObj;
    int phoneId = request->phone_id();
    getJsonForApiResponseSlot(phoneId, jsonfilename, rootObj);

    bool isMemoryFull = false;
    int size = getSMSStorage(phoneId);
    uint32_t maxCount =
    rootObj[TEL_SMS_MANAGER]["requestStorageDetails"]
        ["requestStorageDetailsCbMaxCount"].asInt();
    uint32_t availableCount = maxCount - size;
    if(availableCount == 0) {
        isMemoryFull = true;
    }
    //Create response
    response->set_ismemoryfull(isMemoryFull);
    return grpc::Status::OK;
}