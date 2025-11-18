/*
* Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "CallManagerServerImpl.hpp"
#include "../../../libs/tel/TelDefinesStub.hpp"
#include "SimulationServer.hpp"
#include <telux/common/DeviceConfig.hpp>
#include <thread>
#define CALL_MANAGER "ICallManager"
#define MSD_UPDATE_EVENT "msdUpdateRequest"
#define HANGUP_CALL_EVENT "hangupCall"
#define INCOMING_CALL_EVENT "incomingCall"
#define MODIFY_CALL_REQUEST "modifyCallRequest"
#define RTT_MESSAGE_REQUEST "rttMessageRequest"

#define REST_TIMERS_ON_CALL_SETUP 2
#define JSON_PATH1 "system-state/tel/ICallManagerStateSlot1.json"
#define JSON_PATH2 "system-state/tel/ICallManagerStateSlot2.json"
#define JSON_PATH3 "api/tel/ICallManagerSlot1.json"
#define JSON_PATH4 "api/tel/ICallManagerSlot2.json"

#define SLOT_1 1
#define SLOT_2 2

#define MSD_VERSION_2 2
#define MSD_VERSION_3 3

#define MIN_REDIAL_CONFIG 1
#define MAX_CALLORIG_REDIAL_CONFIG 10
#define MAX_CALLDROP_REDIAL_CONFIG 2

#define MIN_VALUE_TIMEGAP_UNTIL_INDEX4 4
#define MIN_VALUE_TIMEGAP_AFTER_INDEX4 5

CallManagerServerImpl::CallManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    readJson();
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

CallManagerServerImpl::~CallManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if(taskQ_) {
        taskQ_ = nullptr;
    }
}

grpc::Status CallManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    if(ecallStateMachine_) {
        ecallStateMachine_ = nullptr;
    }
    calls_.clear();
    return grpc::Status::OK;
}

grpc::Status CallManagerServerImpl::readJson() {
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
    //System state response
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

void CallManagerServerImpl::getJsonForSystemData(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjSystemStateFileName_[phoneId];
    rootObj = jsonObjSystemStateSlot_[phoneId];
}

void CallManagerServerImpl::getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjApiResponseFileName_[phoneId];
    rootObj = jsonObjApiResponseSlot_[phoneId];
}

grpc::Status CallManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        rootObj = jsonObjApiResponseSlot_[SLOT_1];
        int cbDelay = rootObj[CALL_MANAGER]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus =
            rootObj[CALL_MANAGER]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);

        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::vector<std::string> filters = {TEL_CALL_FILTER};
            auto &serverEventManager = ServerEventManager::getInstance();
            serverEventManager.registerListener(shared_from_this(), filters);
        }
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::GetServiceStatus(ServerContext* context,
    const google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        rootObj = jsonObjApiResponseSlot_[SLOT_1];
        int cbDelay = rootObj[CALL_MANAGER]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus =
            rootObj[CALL_MANAGER]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::MakeCall(ServerContext* context,
    const telStub::MakeCallRequest* request, telStub::MakeCallReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    CallApi makecallApiType = static_cast<CallApi>(request->api());
    std::string apiInput = "";
    if(makecallApiType == CallApi::makeRttVoiceCall) {
        apiInput = "makeRttCall";
    } else {
        apiInput = "makeCall";
    }
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, apiInput, status,
            error, cbDelay );

        if(cbDelay == -1) {
            isCallback = false;
        }
        if(addNewCallDetails<telStub::MakeCallRequest>(request)) {
            auto f = std::async(std::launch::async,
            [this]() {
                handleCallMachine();
            }).share();
            taskQ_->add(f);
        }
        telStub::Call call_;
        call_.set_call_direction
                (static_cast<telStub::CallDirection_Direction>(callInfo_.callDirection));
        call_.set_remote_party_number(static_cast<std::string>(callInfo_.remotePartyNumber));
        call_.set_call_index(static_cast<int>(callInfo_.index));

        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        *response->mutable_call() = call_;
    }
    return readStatus;
}

void CallManagerServerImpl::handleCallMachine() {
    if(callInfo_.callDirection == CallDirection::OUTGOING) {
        if(calls_.size() == 1) {
            changeCallState(callInfo_.phoneId , "CALL_DIALING", callInfo_.index);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            changeCallState(callInfo_.phoneId ,"CALL_ALERTING", callInfo_.index);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            changeCallState(callInfo_.phoneId ,"CALL_ACTIVE", callInfo_.index);
        } else {
            changeCallState(callInfo_.phoneId ,"CALL_DIALING", callInfo_.index);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            changeCallState(callInfo_.phoneId ,"CALL_ALERTING", callInfo_.index);
            changeCallStateofActiveCalls(callInfo_);
        }
    }
}

grpc::Status CallManagerServerImpl::Answer(ServerContext* context,
    const telStub::AnswerRequest* request, telStub::AnswerReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    RttMode mode = static_cast<telux::tel::RttMode>(request->mode());
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        int callIndex = request->call_index();
        telux::common::ErrorCode error;
        telux::common::Status status;
        bool isCallback = true;
        int cbDelay;
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "answer", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
        if(info != nullptr) {
            if(info->callState == CallState::CALL_INCOMING) {
                // Update RTT mode and peer capability of the call based on user input.
                info->mode = mode;
                info->peerRttCapability = mode;
                changeCallState(info->phoneId ,"CALL_ACTIVE", info->index);
            } else if(info->callState == CallState::CALL_WAITING){
                changeCallStateofActiveCalls(*info);
            }
            response->set_status(static_cast<commonStub::Status>(status));
            response->set_iscallback(isCallback);
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            response->set_delay(cbDelay);
        }
    }
    return readStatus;
}

void CallManagerServerImpl::changeCallStateofActiveCalls(CallInfo info) {
    std::shared_ptr<CallInfo> newCall;
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if ((*callIterator)->index != info.index) {
            if((*callIterator)->callState == CallState::CALL_ACTIVE) {
                changeCallState((*callIterator)->phoneId, "CALL_HOLD",
                (*callIterator)->index);
            }
        } else {
            newCall = (*callIterator);
        }
    }
    changeCallState(newCall->phoneId, "CALL_ACTIVE", newCall->index);
}

int CallManagerServerImpl::setCallIndexForNewCall() {
    int size = calls_.size();
    int index = 1;
    LOG(DEBUG, __FUNCTION__, " Number of calls ", size);
    if(size > 0) {
        for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
            ++callIterator) {
            if ((*callIterator)->index != index) {
                LOG(DEBUG, __FUNCTION__, " new call index ", index);
                break;
            } else {
                index++;
            }
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " new call index ", index);
    }
    return index;
}

grpc::Status CallManagerServerImpl::MakeECall(ServerContext* context,
    const telStub::MakeECallRequest* request, telStub::MakeECallReply* response) {
    telux::common::ErrorCode error;
    telux::common::Status status;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    bool isCallback = true;
    int cbDelay;
    int phoneId = request->phone_id();
    CallApi makeEcallApiType = static_cast<CallApi>(request->api());
    std::string input = "";
    if(makeEcallApiType == CallApi::makeECallWithMsd) {
        input = "makeECallWithMsd";
    } else if(makeEcallApiType == CallApi::makeTpsECallOverCSWithMsd) {
        input = "makeTpsECallOverCSWithMsd";
    } else if(makeEcallApiType == CallApi::makeTpsECallOverIMS) {
        input = "makeTpsECallOverIMS";
    } else if(makeEcallApiType == CallApi::makeECallWithRawMsd) {
        input = "makeECallWithRawMsd";
    } else if(makeEcallApiType == CallApi::makeTpsECallOverCSWithRawMsd) {
        input = "makeTpsECallOverCSWithRawMsd";
    } else if(makeEcallApiType == CallApi::makeECallWithoutMsd) {
        input = "makeECallWithoutMsd";
    } else if(makeEcallApiType == CallApi::makeTpsECallOverCSWithoutMsd) {
        input = "makeTpsECallOverCSWithoutMsd";
    } else {
        input = "makeECallWithMsd";
    }
    LOG(DEBUG, __FUNCTION__, " API ", input);
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
            CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, input, status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        // add ecall data to server cache if a new call.
        if(!eCallRedialIsOngoing_) {
            if(addNewCallDetails<telStub::MakeECallRequest>(request)) {
                response->set_error(static_cast<commonStub::ErrorCode>(error));
                telStub::Call call_;
                call_.set_call_direction
                        (static_cast<telStub::CallDirection_Direction>(callInfo_.callDirection));
                call_.set_remote_party_number(static_cast<std::string>(callInfo_.remotePartyNumber));
                call_.set_call_index(static_cast<int>(callInfo_.index));
                *response->mutable_call() = call_;
                auto f = std::async(std::launch::async,
                    [this]() {
                        handleStateMachine(callInfo_.phoneId, callInfo_.index);
                    }).share();
                taskQ_->add(f);
            } else {
                // Send a negative response if eCall is already in progress.
                response->set_error(commonStub::ErrorCode::OP_IN_PROGRESS);
            }
        } else {
            response->set_error(commonStub::ErrorCode::INCOMPATIBLE_STATE);
        }

        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::SetConfig(ServerContext* context,
    const telStub::SetConfigRequest* request,  telStub::SetConfigReply* response) {
    EcallConfig config = {};
    telux::common::ErrorCode error;
    telux::common::Status status;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(SLOT_1, jsonfilename, rootObj);
        if (request->is_mute_rx_audio_valid()) {
                config.muteRxAudio = request->mute_rx_audio();
                rootObj[CALL_MANAGER]["eCallConfig"]["muteRxAudio"]
                    = config.muteRxAudio;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_num_type_valid()) {
                config.numType = static_cast<ECallNumType>(request->num_type());
                rootObj[CALL_MANAGER]["eCallConfig"]["numType"] =
                static_cast<int>(config.numType);
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
                if(config.numType == ECallNumType::OVERRIDDEN) {
                    iseCallNumTypeOverridden_ = true;
                } else {
                    iseCallNumTypeOverridden_ = false;
                }
        }
        if (request->is_overridden_num_valid()) {
                config.overriddenNum = request->overridden_num();
                rootObj[CALL_MANAGER]["eCallConfig"]["overriddenNum"]
                    = config.overriddenNum;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_use_canned_msd_valid()) {
                config.useCannedMsd = request->use_canned_msd();
                rootObj[CALL_MANAGER]["eCallConfig"]["useCannedMsd"]
                    = config.useCannedMsd;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_gnss_update_interval_valid()) {
                config.gnssUpdateInterval = request->gnss_update_interval();
                rootObj[CALL_MANAGER]["eCallConfig"]["gnssUpdateInterval"] =
                config.gnssUpdateInterval;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_t2_timer_valid()) {
                config.t2Timer = request->t2_timer();
                LOG(INFO, __FUNCTION__, " t2 timer value is : ", config.t2Timer);
                rootObj[CALL_MANAGER]["eCallConfig"]["T2Timer"] = config.t2Timer;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_t7_timer_valid()) {
                config.t7Timer = request->t7_timer();
                rootObj[CALL_MANAGER]["eCallConfig"]["T7Timer"] = config.t7Timer;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_t9_timer_valid()) {
                config.t9Timer = request->t9_timer();
                rootObj[CALL_MANAGER]["eCallConfig"]["T9Timer"] = config.t9Timer;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_msd_version_valid()) {
            if ((request->msd_version() == MSD_VERSION_2 ) ||
                (request->msd_version() == MSD_VERSION_3 )) {
                    config.msdVersion = request->msd_version();
                    rootObj[CALL_MANAGER]["eCallConfig"]["msdVersion"] = config.msdVersion;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[SLOT_1] = rootObj;
            } else {
                status = telux::common::Status::INVALIDPARAM;
                response->set_status(static_cast<commonStub::Status>(status));
                return readStatus;
            }

        }
        getJsonForApiResponseSlot(SLOT_1, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "setECallConfig", status,
            error, cbDelay );
        response->set_status(static_cast<commonStub::Status>(status));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::GetConfig(ServerContext* context,
    const google::protobuf::Empty* request, telStub::GetConfigResponse* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    telux::common::ErrorCode error;
    telux::common::Status status;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(SLOT_1, jsonfilename, rootObj);
        response->set_is_mute_rx_audio_valid(true);
        response->set_mute_rx_audio(
            rootObj[CALL_MANAGER]["eCallConfig"]["muteRxAudio"].asBool());
        response->set_is_num_type_valid(true);
        response->set_num_type(static_cast<telStub::ECallNumType>(
                rootObj[CALL_MANAGER]["eCallConfig"]["numType"].asInt()));
        response->set_is_overridden_num_valid(true);
        response->set_overridden_num(
            rootObj[CALL_MANAGER]["eCallConfig"]["overriddenNum"].asString());
        response->set_is_use_canned_msd_valid(true);
        response->set_use_canned_msd(
            rootObj[CALL_MANAGER]["eCallConfig"]["useCannedMsd"].asBool());
        response->set_is_gnss_update_interval_valid(true);
        response->set_gnss_update_interval(
            rootObj[CALL_MANAGER]["eCallConfig"]["gnssUpdateInterval"].asInt());
        response->set_is_t2_timer_valid(true);
        response->set_t2_timer(
            rootObj[CALL_MANAGER]["eCallConfig"]["T2Timer"].asInt());
        response->set_is_t7_timer_valid(true);
        response->set_t7_timer(
            rootObj[CALL_MANAGER]["eCallConfig"]["T7Timer"].asInt());
        response->set_is_t9_timer_valid(true);
        response->set_t9_timer(
            rootObj[CALL_MANAGER]["eCallConfig"]["T9Timer"].asInt());
        response->set_is_msd_version_valid(true);
        response->set_msd_version(
            rootObj[CALL_MANAGER]["eCallConfig"]["msdVersion"].asInt());
        getJsonForApiResponseSlot(SLOT_1, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "getECallConfig", status,
            error, cbDelay );
        response->set_status(static_cast<commonStub::Status>(status));
        LOG(DEBUG, __FUNCTION__, "Status is ", static_cast<int>(status));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::UpdateEcallHlapTimer(ServerContext* context,
    const telStub::UpdateEcallHlapTimerRequest* request,
    telStub::UpdateEcallHlapTimerResponse* response) {

    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    int phoneId = request->phone_id();
    ::telStub::HlapTimerType type = request->type();
    int time = request->time_duration();

    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        if(type == ::telStub::HlapTimerType::T10_TIMER) {
            rootObj[CALL_MANAGER]["eCallHlapTimer"]["t10"] = time;
            rootObj[CALL_MANAGER]["eCallConfig"]["T10Timer"] = ((time*60)*1000);
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
        }
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "updateEcallHlapTimer", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::RequestEcallHlapTimer(ServerContext* context,
    const telStub::RequestEcallHlapTimerRequest* request,
    telStub::RequestEcallHlapTimerReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    int phoneId = request->phone_id();
    ::telStub::HlapTimerType type = request->type();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "requestEcallHlapTimer", status,
            error, cbDelay );
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        if(type == ::telStub::HlapTimerType::T10_TIMER) {
            int timeDuration = rootObj[CALL_MANAGER]["eCallHlapTimer"]["t10"].asInt();
            response->set_time_duration(timeDuration);
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::RequestECallHlapTimerStatus(ServerContext* context,
    const telStub::RequestECallHlapTimerStatusRequest* request,
    telStub::RequestECallHlapTimerStatusReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        telux::common::ErrorCode error;
        telux::common::Status status;
        bool isCallback = true;
        int cbDelay;
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER,
            "requestECallHlapTimerStatus", status, error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }

        telStub::ECallHlapTimerStatus hlapTimerStatus;
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        hlapTimerStatus.set_t2(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T2Timer"].asInt())));
        hlapTimerStatus.set_t5(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T5Timer"].asInt())));
        hlapTimerStatus.set_t6(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T6Timer"].asInt())));
        hlapTimerStatus.set_t7(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T7Timer"].asInt())));
        hlapTimerStatus.set_t9(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T9Timer"].asInt())));
        hlapTimerStatus.set_t10(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T10Timer"].asInt())));
        *response->mutable_hlap_timer_status() = hlapTimerStatus;

        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::ExitEcbm(ServerContext* context,
    const telStub::RequestEcbmRequest* request,
    telStub::RequestEcbmReply* response) {
    grpc::Status readStatus = readJson();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    int phoneId = request->phone_id();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "exitEcbm", status,
            error, cbDelay );
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        ::telStub::EcbMode_Mode mode =
        static_cast<::telStub::EcbMode_Mode>(
            rootObj[CALL_MANAGER]["ecbm"]["ecbMode"].asInt());
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        if(rootObj[CALL_MANAGER]["ecbm"]["ecbMode"].asInt() == 0) {
            response->set_error(commonStub::ErrorCode::INVALID_ARGUMENTS);
        }
        response->set_ecbmode(mode);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::RequestEcbm(ServerContext* context,
    const telStub::RequestEcbmRequest* request,
    telStub::RequestEcbmReply* response) {
    grpc::Status readStatus = readJson();
    telux::common::ErrorCode error;
    telux::common::Status status;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    bool isCallback = true;
    int cbDelay;
    int phoneId = request->phone_id();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "requestEcbm", status,
            error, cbDelay );
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        ::telStub::EcbMode_Mode mode =
        static_cast<::telStub::EcbMode_Mode>(rootObj[CALL_MANAGER]["ecbm"]\
            ["ecbMode"].asInt());
        response->set_ecbmode(mode);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
    }
    return readStatus;
}

bool CallManagerServerImpl::match(std::shared_ptr<CallInfo> call, CallInfo callToCompare) {
    logCallDetails(call);
    return ((callToCompare.remotePartyNumber == call->remotePartyNumber)
        && (callToCompare.phoneId == call->phoneId));
}

void CallManagerServerImpl::logCallDetails(std::shared_ptr<CallInfo> call) {
    LOG(DEBUG, __FUNCTION__, " SlotId = ", static_cast<int>(call->phoneId),
        " Call Info: remotePartyNumber = ", call->remotePartyNumber,
        ", callIndex = ", call->index,
        ", callDirection = ", static_cast<int>(call->callDirection),
        ", isRegulatoryeCall = ", static_cast<bool>(call->isRegulatoryeCall),
        ", isMsdTransmitted = ", static_cast<bool>(call->callState),
        ", isMpty = ", static_cast<bool>(call->isMpty),
        ", isTpseCallOverIms = ", static_cast<bool>(call->isTpseCallOverIms),
        ", rttMode = ", static_cast<int>(call->mode),
        ", localRttCapability = ", static_cast<int>(call->localRttCapability),
        ", peerRttCapability = ", static_cast<int>(call->peerRttCapability),
        ", callType = ", static_cast<int>(call->callType));
}

std::shared_ptr<CallInfo> CallManagerServerImpl::findMatchingCall(int slotId, int callIndex) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<CallInfo>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);
    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallInfo> call) {
        return match(call, slotId, callIndex);
    });

    if (iter != std::end(calls_)) {
        LOG(DEBUG, __FUNCTION__, " found matched call");
        return *iter;
    } else {
        LOG(DEBUG, __FUNCTION__, " no matched call");
        return nullptr;
    }
}

bool CallManagerServerImpl::match(std::shared_ptr<CallInfo> call, int slotId, int callIndex) {
    logCallDetails(call);
    return ((call->index == callIndex) && (call->phoneId == slotId));
}

bool CallManagerServerImpl::findMatchingCall(CallInfo callToCompare) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<CallInfo>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);
    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallInfo> call) {
        return match(call, callToCompare);
    });

    if (iter != std::end(calls_)) {
        LOG(DEBUG, __FUNCTION__, " found matched call");
        return true;
    } else {
        LOG(DEBUG, __FUNCTION__, " no matched call");
        return false;
    }
}

void CallManagerServerImpl::hangupWaitingOrBackgroundCalls(int phoneId) {
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if((*callIterator)->phoneId == phoneId) {
            if(((*callIterator)->callState == CallState::CALL_ON_HOLD)
                ||((*callIterator)->callState == CallState::CALL_INCOMING)) {
                changeCallState((*callIterator)->phoneId, "CALL_ENDED", (*callIterator)->index);
            }
        }
    }
}

void CallManagerServerImpl::hangupForegroundgroundCalls(int phoneId) {
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if((*callIterator)->phoneId == phoneId) {
            if(((*callIterator)->callState == CallState::CALL_ACTIVE)
                ||((*callIterator)->callState == CallState::CALL_INCOMING)) {
            changeCallState((*callIterator)->phoneId, "CALL_ENDED",
            (*callIterator)->index);
        }
        }
    }
}

void CallManagerServerImpl::resumeBackgroundCalls(int phoneId) {
    bool foundCall = false;
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if((*callIterator)->phoneId == phoneId) {
            if((*callIterator)->callState == CallState::CALL_WAITING) {
                changeCallState((*callIterator)->phoneId, "CALL_ACTIVE",
                (*callIterator)->index);
                foundCall = true;
                break;
            }
        }
    }
    if(!foundCall) {
        for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
            ++callIterator) {
            if((*callIterator)->callState == CallState::CALL_ON_HOLD) {
                changeCallState((*callIterator)->phoneId, "CALL_ACTIVE",
                (*callIterator)->index);
                break;
            }
        }
    }

}

grpc::Status CallManagerServerImpl::HangupForegroundResumeBackground(ServerContext* context,
    const telStub::HangupForegroundResumeBackgroundRequest* request,
    telStub::HangupForegroundResumeBackgroundReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        telux::common::ErrorCode error;
        telux::common::Status status;
        bool isCallback = true;
        int cbDelay;
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER,
            "hangupForegroundResumeBackground", status, error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        hangupForegroundgroundCalls(phoneId);
        resumeBackgroundCalls(phoneId);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

int CallManagerServerImpl::getCallIndexOfActiveCall(int phoneId) {
    int index = CALL_INDEX_INVALID;
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if(((*callIterator)->phoneId == phoneId)
            && ((*callIterator)->callState == CallState::CALL_ACTIVE)) {
                index = (*callIterator)->index;
                break;
        }
    }
    LOG(DEBUG, __FUNCTION__, "Call Index of active call ", index);
    return index;
}

grpc::Status CallManagerServerImpl::Resume(ServerContext* context,
    const telStub::ResumeRequest* request,
    telStub::ResumeReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "resume", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        int callActivateIndex = getCallIndexOfActiveCall(phoneId);
        swapCalls(callIndex, phoneId, callActivateIndex);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::Hold(ServerContext* context,
    const telStub::HoldRequest* request,
    telStub::HoldReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "hold", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        holdCall(phoneId, callIndex);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

void CallManagerServerImpl::holdCall(int phoneId, int callIndex) {
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if(((*callIterator)->phoneId == phoneId) && ((*callIterator)->index )) {
            if((*callIterator)->callState == CallState::CALL_ACTIVE) {
                changeCallState((*callIterator)->phoneId, "CALL_HOLD",
                (*callIterator)->index);
                break;
            }
        }
    }
}

void CallManagerServerImpl::swapCalls(int callHoldIndex, int phoneIndex,
    int callActivateIndex) {
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if(((*callIterator)->phoneId == phoneIndex)
            && ((*callIterator)->index  == callHoldIndex)) {
            if((*callIterator)->callState == CallState::CALL_ON_HOLD) {
                changeCallState((*callIterator)->phoneId, "CALL_ACTIVE",
                (*callIterator)->index);
            }
        }
        if(((*callIterator)->phoneId == phoneIndex)
            && ((*callIterator)->index  == callActivateIndex)) {
            if((*callIterator)->callState == CallState::CALL_ACTIVE) {
                changeCallState((*callIterator)->phoneId, "CALL_HOLD",
                (*callIterator)->index);
            }
        }

    }
}

grpc::Status CallManagerServerImpl::Swap(ServerContext* context,
    const telStub::SwapRequest* request,
    telStub::SwapReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int callHoldIndex = request->call_to_hold_index();
    int phoneIndex = request->phone_id();
    int callActivateIndex = request->call_to_activate_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(SLOT_1, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "swap", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        swapCalls(callHoldIndex, phoneIndex, callActivateIndex);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::HangupWaitingOrBackground(ServerContext* context,
    const telStub::HangupWaitingOrBackgroundRequest* request,
    telStub::HangupWaitingOrBackgroundReply* response) {
    int phoneId = request->phone_id();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "hangupWaitingOrBackground",
            status, error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        hangupWaitingOrBackgroundCalls(phoneId);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::Hangup(ServerContext* context,
    const telStub::HangupRequest* request, telStub::HangupReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "hangup", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
        if(info != nullptr) {
            if((info->isRegulatoryeCall) ||
                (!(info->isRegulatoryeCall) && (!(info->isTpseCallOverIms))
                && (info->isMsdTransmitted))) {
                    // regulatory ecall or custom number ecall over CS with MSD
                LOG(DEBUG, __FUNCTION__);
                if(ecallStateMachine_ != nullptr) {
                    ecallStateMachine_->onEvent(
                    ecallStateMachine_->createTelEvent(
                    EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER,
                    "", phoneId));
                }
            } else {  // Custom number eCall over PS or voice call
                changeCallState(info->phoneId, "CALL_ENDED", info->index);
            }
            response->set_status(static_cast<commonStub::Status>(status));
            response->set_iscallback(isCallback);
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            response->set_delay(cbDelay);
        }
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::Reject(ServerContext* context,
    const telStub::RejectRequest* request, telStub::RejectReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "reject", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
        if(info != nullptr) {
            changeCallState(info->phoneId , "CALL_ENDED", info->index);
            response->set_status(static_cast<commonStub::Status>(status));
            response->set_iscallback(isCallback);
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            response->set_delay(cbDelay);
        }
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::RejectWithSMS(ServerContext* context,
    const telStub::RejectWithSMSRequest* request, telStub::RejectWithSMSReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "rejectSms", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
        if(info != nullptr) {
            changeCallState(info->phoneId , "CALL_ENDED", info->index);
            response->set_status(static_cast<commonStub::Status>(status));
            response->set_iscallback(isCallback);
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            response->set_delay(cbDelay);
        }
    }
    return readStatus;
}

void CallManagerServerImpl::handleHangupRequest(std::string eventParams) {
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    int phoneId;
    int callIndex;
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
            if (phoneId < SLOT_1 || phoneId > SLOT_2) {
                LOG(ERROR, " Invalid input for slot id");
                return;
            }
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return;
        }
    }
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(ERROR, __FUNCTION__, "CallId not passed");
        return;
    } else {
        try {
            callIndex = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return;
        }
    }
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
    std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
    if(info != nullptr) {
        if(info->isRegulatoryeCall) {
            if(ecallStateMachine_ != nullptr) {
                ecallStateMachine_->onEvent(
                    ecallStateMachine_->createTelEvent(
                    EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER, "", phoneId));
            }
            //Clear call cache in server
           std::shared_ptr<CallInfo> call =
            findCallAndUpdateCallState(callIndex, CallState::CALL_ENDED, phoneId);
        } else {
            changeCallState(info->phoneId, "CALL_ENDED", info->index);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Matching call not found ");
        return;

    }
}

grpc::Status CallManagerServerImpl::UpdateECallMsd(ServerContext* context,
    const telStub::UpdateECallMsdRequest* request, telStub::UpdateECallMsdResponse* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int updateECallMsdApiType = static_cast<int>(request->api());
    std::string input = "";
    if(updateECallMsdApiType == updateEcallMsd) {
        input = "updateECallMsd";
    } else if(updateECallMsdApiType == updateECallRawMsd) {
        input = "updateECallRawMsd";
    } else {
        input = "updateECallMsd";
    }
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, input, status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    if(ecallStateMachine_ != nullptr) {
        if(!(ecallStateMachine_->isEcallMSDUpdateInProgress())) {
            std::vector<std::string> input = parseUserInput();
            if((input[0] == "SUCCESS")
                && (ecallStateMachine_->getCurrentState() ==
                EcallStateMachine::StateID::STATE_CALL_CONVERSATION)) {
                bool isNGeCall = false;
                if(callInfo_.isRegulatoryeCall) {
                    LOG(DEBUG, __FUNCTION__, " PSAP update request for a regulatory eCall");
                    isNGeCall = getUserConfiguredeCallRat();
                } else {
                    LOG(DEBUG, __FUNCTION__, " PSAP update request for a custom number eCall");
                    isNGeCall = callInfo_.isTpseCallOverIms;
                }
                if(isNGeCall) {
                    if((callInfo_.isRegulatoryeCall) ||
                        /* For Private eCall, user sends raw MSD pdu after sending recieving
                         * MSD pull request from PSAP using ICallManager::updateECallMsd.
                         */
                        ((callInfo_.isTpseCallOverIms) &&
                         (updateECallMsdApiType == updateECallRawMsd))) {
                            ecallStateMachine_->onEvent(
                            ecallStateMachine_->createTelEvent(
                            EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP,
                            "NGeCall", phoneId));
                    }
                } else {
                    // CS eCall
                    ecallStateMachine_->onEvent(
                    ecallStateMachine_->createTelEvent(
                    EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP, "CSeCall", phoneId));
                }
            } else {
                LOG(ERROR, __FUNCTION__,
                    "Incorrect JSON configuration or ecall is not in desired state");
            }
        }
    } else {
        LOG(DEBUG, __FUNCTION__, "The state machine is not yet initialised ");
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::ModifyOrRespondToModifyCall(ServerContext* context,
    const telStub::ModifyOrRespondToModifyCallRequest* request,
    telStub::ModifyOrRespondToModifyCallReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    RttMode rttMode  = static_cast<telux::tel::RttMode>(request->rtt_mode());
    grpc::Status readStatus = readJson();
    std::string inputApi = request->api_type();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, inputApi, status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
        if(info != nullptr) {
            if(inputApi == "modify") {
                // Update rtt mode of the call based on user input. Rtt mode input validation is
                // not performed to check the current RTT mode of the call. Hence, if user sets
                // same rtt mode as current rtt mode of the call, response callback will not report
                // error.
                // API is expected to be called when call state is ACTIVE.
                info->mode = rttMode;
                changeRttModeOfCall(info->mode, info->index, info->phoneId);
            } else {
                if(rttMode == info->mode) {
                    // User requested RTT mode is same as current rtt mode of the call then
                    // there is no change in call attributes.
                } else {
                    // Update the RTT mode of the call and trigger event to clients
                    info->mode = rttMode;
                    changeRttModeOfCall(info->mode, info->index, info->phoneId);
                }
            }
            response->set_status(static_cast<commonStub::Status>(status));
            response->set_iscallback(isCallback);
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            response->set_delay(cbDelay);
        }
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::RequestNetworkDeregistration(ServerContext* context,
    const telStub::RequestNetworkDeregistrationRequest* request,
    telStub::RequestNetworkDeregistrationReply* response) {
    telux::common::ErrorCode error;
    telux::common::Status status;
    int cbDelay;
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonObjFileName = "";
    bool isCallback = true;
    std::string input = "requestNetworkDeregistration";
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        if(ecallStateMachine_ != nullptr) {
            getJsonForSystemData(phoneId, jsonObjFileName, rootObj);
            HlapTimerStatus T10Status = static_cast<HlapTimerStatus>(
                rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T10Timer"].asInt());
            // To ensure that network deregistration is requested when T10 timer is active.
            if(T10Status == HlapTimerStatus::ACTIVE) {
                ecallStateMachine_->onEvent(
                ecallStateMachine_->createTelEvent(
                EcallStateMachine::EventID::ON_NETWORK_DEREGISTRATION_REQUEST,
                "T10Timer", phoneId));
            }
        }
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, input, status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::SendRtt(ServerContext* context,
    const telStub::SendRttRequest* request, telStub::SendRttReply* response) {
    telux::common::ErrorCode error;
    telux::common::Status status;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    bool isCallback = true;
    std::string input = "sendRtt";
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, input, status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

void CallManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == TEL_CALL_FILTER) {
        onEventUpdate(message.event());
    }
}

void CallManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__,"String is ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__,"String is ", token );
    if ( MSD_UPDATE_EVENT == token) {
        handleMsdUpdateRequest(event);
    } else if(HANGUP_CALL_EVENT == token) {
         handleHangupRequest(event);
    } else if(INCOMING_CALL_EVENT == token) {
        handleIncomingCallRequest(event);
    } else if(MODIFY_CALL_REQUEST == token) {
        handleModifyCallRequest(event);
    } else if(RTT_MESSAGE_REQUEST == token) {
        handleRttMessageRequest(event);
    }else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void CallManagerServerImpl::triggerMsdPullrequestEvent(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::MsdPullRequestEvent msdPullRequestEvent;
    ::eventService::EventResponse anyResponse;

    msdPullRequestEvent.set_phone_id(phoneId);
    anyResponse.set_filter(TEL_CALL_FILTER);
    anyResponse.mutable_any()->PackFrom(msdPullRequestEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
}

void CallManagerServerImpl::triggerModifyCallRequestEvent(int phoneId, int callIndex) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::ModifyCallRequestEvent modifyCallRequestEvent;
    ::eventService::EventResponse anyResponse;

    modifyCallRequestEvent.set_phone_id(phoneId);
    modifyCallRequestEvent.set_call_index(callIndex);
    anyResponse.set_filter(TEL_CALL_FILTER);
    anyResponse.mutable_any()->PackFrom(modifyCallRequestEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
}

void CallManagerServerImpl::handleModifyCallRequest(std::string eventParams) {
    int phoneId;
    int callIndex;
    // Fetch phoneId
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The slot id is not passed! Assuming default slot id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
            if (phoneId < SLOT_1 || phoneId > SLOT_2) {
                LOG(ERROR, " Invalid input for slot id");
                return;
            }
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return;
        }
    }
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    // Fetch callId
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(ERROR, __FUNCTION__, "CallId not passed");
        return;
    } else {
        try {
            callIndex = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return;
        }
    }
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
    std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
    // Trigger event only if call state is ACTIVE and current call is a voice call.
    if(info != nullptr) {
        if((info->callState == CallState::CALL_ACTIVE) && (info->mode != RttMode::FULL)) {
            auto f = std::async(std::launch::async, [this, phoneId, callIndex]() {
                this->triggerModifyCallRequestEvent(phoneId, callIndex);
            }).share();
            taskQ_->add(f);
        }
    }
}

void CallManagerServerImpl::triggerRttMessageEvent(int phoneId, std::string message) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::RttMessageEvent event;
    ::eventService::EventResponse anyResponse;

    event.set_phone_id(phoneId);
    event.set_message(message);
    anyResponse.set_filter(TEL_CALL_FILTER);
    anyResponse.mutable_any()->PackFrom(event);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
}

void CallManagerServerImpl::handleRttMessageRequest(std::string eventParams) {
    int phoneId;
    std::string message;
    // Fetch phoneId
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The slot id is not passed! assuming default slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
            if (phoneId < SLOT_1 || phoneId > SLOT_2) {
                LOG(ERROR, " Invalid input for slot id");
                return;
            }
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return;
        }
    }
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    // Fetch message
    if(eventParams == "") {
        LOG(ERROR, __FUNCTION__, "Message not passed");
        return;
    }
    message = eventParams;
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
    auto f = std::async(std::launch::async, [this, phoneId, message]() {
        this->triggerRttMessageEvent(phoneId, message);
    }).share();
    taskQ_->add(f);
}

void CallManagerServerImpl::handleIncomingCallRequest(std::string eventParams) {
    int phoneId;
    std::string dialNumber;
    RttMode mode;
    /* Fetch the slotId */
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
            if (phoneId < SLOT_1 || phoneId > SLOT_2) {
                LOG(ERROR, " Invalid input for slot id");
                return;
            }
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
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);

    /* Fetch the dialnumber */
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "MT call is considered to be originating from PSAP");
    } else {
        dialNumber = token;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched dial number is: ", dialNumber);

    /* Fetch the rttMode */
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " rttMode input is not provided");
        mode = RttMode::DISABLED;
    } else {
        mode = static_cast<RttMode>(std::stoi(token));
        if (mode < RttMode::DISABLED || mode > RttMode::FULL) {
            LOG(ERROR, " Invalid input for rtt mode");
            return;
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched rttMode is: ", static_cast<int>(mode));

    //Update call cache for new MT Voice call
    CallInfo callInfo;
    callInfo.phoneId = phoneId;
    callInfo.mode = mode;
    if(callInfo.mode == RttMode::FULL) {
        // TODO: Capability of simulation framework depends on IMS Settings
        // Currently, it is assumed to have full capability when incoming call is RTT.
        callInfo.localRttCapability = RttMode::FULL;
        // Since, remote end user makes the rtt call, peer capability is FULL.
        callInfo.peerRttCapability = RttMode::FULL;
    }
    callInfo.index = setCallIndexForNewCall();
    callInfo.callDirection = CallDirection::INCOMING;
    if(callInfo.index > 1) { //MO or MT call already exist then callState = WAITING
        callInfo.callState = CallState::CALL_WAITING;
    } else { //No MO or MT call already exist then callState = INCOMING
        callInfo.callState = CallState::CALL_INCOMING;
    }
    callInfo.remotePartyNumber = dialNumber;
    callInfo.isMsdTransmitted = false;
    callInfo.isMultiPartyCall = true;
    callInfo.isMpty = true;
    telStub::RadioTechnology rat;
    std::vector<telStub::RadioTechnology> psRatList =
        {telStub::RadioTechnology::RADIO_TECH_NR5G,
        telStub::RadioTechnology::RADIO_TECH_LTE};
    if(telux::common::ErrorCode::SUCCESS ==
        TelUtil::readVoiceRadioTechnologyFromJsonFile(callInfo.phoneId, rat)) {
        if (std::find(psRatList.begin(), psRatList.end(), rat) != psRatList.end()) {
            callInfo.callType = CallType::VOICE_IP_CALL;
        } else {
            callInfo.callType = CallType::VOICE_CALL;
        }
    } else {
        callInfo.callType = CallType::VOICE_CALL;
    }

    callInfo_ = callInfo;
    auto call = std::make_shared<CallInfo>(callInfo);
    logCallDetails(call);
    if(!findMatchingCall(callInfo)) {
        std::lock_guard<std::mutex> lock(callManagerMutex_);
        {
            calls_.emplace_back(call);
        }
    } else {
        LOG(ERROR, __FUNCTION__, "DialNumber is already in progress: ", dialNumber);
        return;
    }
    auto f = std::async(std::launch::async, [this, callInfo]() {
            this->changeCallState(callInfo.phoneId,
                Helper::getCallStateInString(callInfo.callState), callInfo.index);
        }).share();
    taskQ_->add(f);
}

void CallManagerServerImpl::handleMsdUpdateRequest(std::string eventParams) {
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    int phoneId;
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
            if (phoneId < SLOT_1 || phoneId > SLOT_2) {
                LOG(ERROR, " Invalid input for slot id");
                return;
            }
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return;
        }
    }
    if(ecallStateMachine_ != nullptr) {
        if(!(ecallStateMachine_->isEcallMSDUpdateInProgress())) {
            auto f = std::async(std::launch::async, [this, phoneId]() {
                this->triggerMsdPullrequestEvent(phoneId);
            }).share();
            taskQ_->add(f);
        }
    } else {
        LOG(DEBUG, __FUNCTION__, "The state machine is not yet initialised ");
    }
}

std::vector<std::string> CallManagerServerImpl::parseUserInput() {
    std::vector<std::string> parsedString = {};
    std::string jsonObjApiResponseFileName = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(callInfo_.phoneId, jsonObjApiResponseFileName, rootObj);
        std::string input =
            rootObj[CALL_MANAGER]["configureFailureForRegulatoryECall"].asString();
        LOG(DEBUG, __FUNCTION__, "Input is ", input);
        int size = input.size();
        int i = 1;
        while (i <= size ) {
           int j = 1;
           LOG(DEBUG, __FUNCTION__, "parsed string is", input);
           std::string out = fetchNextToken(input, " ");
           j = out.size();
           LOG(DEBUG, __FUNCTION__, "J is ", j);
           parsedString.push_back(out);
           LOG(DEBUG, __FUNCTION__, "parsed string is", out);
           i = i+j+1;
           LOG(DEBUG, __FUNCTION__, "I  is ", i);
        }
    }
    return parsedString;
}

std::string CallManagerServerImpl::fetchNextToken(std::string& inputString, std::string delimiter) {
    unsigned int position = 0;
    std::string token;
    if ((position = inputString.find(delimiter)) != std::string::npos) {
        token = inputString.substr(0, position);
        inputString.erase(0, position + delimiter.length());
    }
    return token;
}

telux::common::Status CallManagerServerImpl::handleStateMachine(int phoneId, int callIndex) {
    LOG(DEBUG, __FUNCTION__);
    if(callInfo_.isRegulatoryeCall != true) {
        // User configurable failures for Ecall HLAP timers are not applicable for a
        // custom number eCall
        std::vector<std::string> input = {"SUCCESS"};
        ecallStateMachine_ = std::make_shared<EcallStateMachine>(shared_from_this(),
            input, callInfo_.isMsdTransmitted, callInfo_.isTpseCallOverIms, false, phoneId,
            callIndex, true, "SUCCESS", false);
    } else {
        // Regulatory eCall
        std::vector<std::string> input = parseUserInput();
        bool isNGeCall = getUserConfiguredeCallRat();
        bool isALACKConfigEnabled = getUserConfiguredALACKParameter();
        // Redial config from user is applicable only during regulatory eCalls.
        std::string eCallRedialConfig = getUserConfiguredECallRedialConfig();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        ecallStateMachine_ = std::make_shared<EcallStateMachine>(shared_from_this(),
            input, callInfo_.isMsdTransmitted, isNGeCall, isALACKConfigEnabled, phoneId, callIndex,
            false, eCallRedialConfig, false);
    }
    if(!ecallStateMachine_) {
        return telux::common::Status::NOMEMORY;
    } else {
        ecallStateMachine_->start();
    }
    return telux::common::Status::SUCCESS;
}

std::string CallManagerServerImpl::getRemotePartyNumber(int phoneId) {
    LOG(DEBUG, __FUNCTION__, " PhoneId ", phoneId);
    std::string jsonObjFileName = "";
    std::string input = "";
    std::string defaultEcallNumber = "112";
    Json::Value rootObj;
    readJson();
    getJsonForSystemData(phoneId, jsonObjFileName, rootObj);
    if(iseCallNumTypeOverridden_ != true) {
        input = defaultEcallNumber;
    } else {
        input = rootObj[CALL_MANAGER]["eCallConfig"]["overriddenNum"].asString();
    }
    LOG(DEBUG, __FUNCTION__, " Remote party number is ", input);
    return input;
}

bool CallManagerServerImpl::getUserConfiguredeCallRat() {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonObjFileName = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(callInfo_.phoneId, jsonObjFileName, rootObj);
        std::string input =
                rootObj[CALL_MANAGER]["eCallType"].asString();
        if(input == "NGeCall") {
            LOG(DEBUG, __FUNCTION__, "NG ecall is configured");
            return true;
        }
    }
    LOG(DEBUG, __FUNCTION__, "CS ecall is configured");
    return false;
}

bool CallManagerServerImpl::getUserConfiguredALACKParameter() {
    LOG(DEBUG, __FUNCTION__, " phoneId ", callInfo_.phoneId);
    std::string jsonObjFileName = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    bool input = false;
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(callInfo_.phoneId, jsonObjFileName, rootObj);
        input = rootObj[CALL_MANAGER]["enableALACKWithClearDown"].asBool();
    }
    LOG(DEBUG, __FUNCTION__, " input " , input);
    return input;
}

std::string CallManagerServerImpl::getUserConfiguredECallRedialConfig() {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonObjFileName = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(callInfo_.phoneId, jsonObjFileName, rootObj);
        std::string input =rootObj[CALL_MANAGER]["configureECallRedialFailure"].asString();
        LOG(DEBUG, __FUNCTION__, " ECallRedial config is", input);
        return input;
    } else {
        return "";
    }
}

void CallManagerServerImpl::updateEcallHlapTimer(std::string timer,
    HlapTimerStatus status) {
    LOG(DEBUG, __FUNCTION__," Timer ", timer, " Timer status ", static_cast<int>(status));
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    grpc::Status readStatus = readJson();
    if (!readStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed!");
        return;
    }
    getJsonForSystemData(callInfo_.phoneId, jsonObjApiResponseFileName, rootObj);
    rootObj[CALL_MANAGER]["ecallHlapTimerStatus"][timer] = static_cast<int>(status);
    JsonParser::writeToJsonFile(rootObj, jsonObjApiResponseFileName);
    jsonObjSystemStateSlot_[callInfo_.phoneId] = rootObj;
}

void CallManagerServerImpl::startTimer(std::string timer) {
    LOG(DEBUG, __FUNCTION__,"Start timer ", timer);
    updateEcallHlapTimer(timer, HlapTimerStatus::ACTIVE);
    if((timer == "T2Timer") || (timer == "T5Timer") || (timer == "T6Timer")
        || (timer == "T7Timer") || (timer == "T9Timer") || (timer == "T10Timer")) {
        startTimers(timer);
        auto f = std::async(std::launch::async, [this, timer]() {
            this->triggerECallInfoChangeEvent(timer, HlapTimerEvent::STARTED);
        }).share();
        taskQ_->add(f);
    } else {
       LOG(ERROR, __FUNCTION__,"Invalid timer ", timer);
    }
}

void CallManagerServerImpl::msdTransmissionStatus(std::string msdtransmision ) {
    auto f = std::async(std::launch::async, [this, msdtransmision ]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            this->triggerECallInfoChangeEvent(msdtransmision, HlapTimerEvent::UNCHANGED);
        }).share();
    taskQ_->add(f);
}

void CallManagerServerImpl::startTimers(std::string timer) {
    Json::Value rootObj;
    std::string jsonfilename = "";
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        updateEcallHlapTimer(timer, HlapTimerStatus::ACTIVE);
        getJsonForSystemData(callInfo_.phoneId, jsonfilename, rootObj);
        LOG(DEBUG, __FUNCTION__,"Timer is ", timer, " Phone id is ", callInfo_.phoneId);
        int delay;
        if((timer == "T5Timer") || (timer == "T6Timer")) {
            delay = 5000; //timer expiry is set as per eCall specification to 5 secs
        } else {
            delay = rootObj[CALL_MANAGER]["eCallConfig"][timer].asInt();
        }
        auto f = std::async(std::launch::async, [this, delay, timer]() {
            LOG(DEBUG, __FUNCTION__,"Delay is", delay);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            Json::Value obj;
            std::string filename = "";
            getJsonForSystemData(callInfo_.phoneId, filename, obj);
            HlapTimerStatus status = static_cast<HlapTimerStatus>(
                obj[CALL_MANAGER]["ecallHlapTimerStatus"][timer].asInt());
            if(status == HlapTimerStatus::ACTIVE) {
                LOG(DEBUG, __FUNCTION__," Timer is active", timer);
                this->triggerTimerExpiry(timer, callInfo_.phoneId);
            }
        }).share();
        taskQ_->add(f);
        /* Reset T9 timer and T10 timer when a new eCall is triggered
         * (and T2 starts with call setup) before T9 expiry.
         */
        if(timer == "T2Timer") {
            std::string timer[REST_TIMERS_ON_CALL_SETUP] = { "T9Timer", "T10Timer"};
            for(int i = 0 ; i <= REST_TIMERS_ON_CALL_SETUP - 1; i++) {
                HlapTimerStatus status = static_cast<HlapTimerStatus>(
                    rootObj[CALL_MANAGER]["ecallHlapTimerStatus"][timer[i]].asInt());
                if(status == HlapTimerStatus::ACTIVE) {
                    LOG(DEBUG, __FUNCTION__, "Timer", timer[i], "is active");
                    updateEcallHlapTimer(timer[i], HlapTimerStatus::INACTIVE);
                    auto f = std::async(std::launch::async, [this, timer, i, status]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        this->triggerECallInfoChangeEvent(timer[i],
                            HlapTimerEvent::STOPPED);
                    }).share();
                    taskQ_->add(f);
                }
            }
        }
    }
}

void CallManagerServerImpl::triggerTimerExpiry(std::string timer, int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    if((timer == "T2Timer") || (timer == "T5Timer") || (timer == "T6Timer") || (timer == "T7Timer")
        || (timer == "T10Timer") || (timer == "T9Timer")) {
        updateEcallHlapTimer(timer, HlapTimerStatus::INACTIVE);
        if(ecallStateMachine_) {
            ecallStateMachine_->onEvent(ecallStateMachine_->createTelEvent(
                EcallStateMachine::EventID::ON_TIMER_EXPIRY, timer, phoneId));
        }
    } else {
      LOG(ERROR, __FUNCTION__, "Invalid timer");
    }
}

void CallManagerServerImpl::expiryTimer(std::string timer) {
    LOG(DEBUG, __FUNCTION__,"timer is ", timer);
    updateEcallHlapTimer(timer, HlapTimerStatus::INACTIVE);
    auto f = std::async(std::launch::async, [this, timer]() {
            this->triggerECallInfoChangeEvent(timer, HlapTimerEvent::EXPIRED);
    }).share();
    taskQ_->add(f);
}

void CallManagerServerImpl::sendEvent(std::string timer, std::string status ) {
    LOG(DEBUG, __FUNCTION__, "Timer event for ", timer, "timer status is", status);
    std::string value = timer + status;
    if(status == "start") {
        updateEcallHlapTimer(timer, HlapTimerStatus::ACTIVE);
        auto f = std::async(std::launch::async, [this, timer, status]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            this->triggerECallInfoChangeEvent(timer, HlapTimerEvent::STARTED);
        }).share();
        taskQ_->add(f);
    } else if (status == "stop" ) {
        updateEcallHlapTimer(timer, HlapTimerStatus::INACTIVE);
        auto f = std::async(std::launch::async, [this, timer, status]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            this->triggerECallInfoChangeEvent(timer, HlapTimerEvent::STOPPED);
        }).share();
        taskQ_->add(f);
    } else {
        LOG(ERROR, __FUNCTION__, "Invalid event");
    }
}

void CallManagerServerImpl::triggerECallInfoChangeEvent(std::string timer,
    HlapTimerEvent action ) {
    int slotId = callInfo_.phoneId;
    LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
    ::telStub::ECallInfoEvent eCallInfoEvent;
    ::eventService::EventResponse anyResponse;
    eCallInfoEvent.set_timer(timer);
    eCallInfoEvent.set_action(static_cast<telStub::HlapTimerEvent>(action));
    eCallInfoEvent.set_phone_id(slotId);
    anyResponse.set_filter(TEL_CALL_FILTER);
    anyResponse.mutable_any()->PackFrom(eCallInfoEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void CallManagerServerImpl::triggerCallInfoChangeEvent(int phoneId,
    std::shared_ptr<CallInfo> call) {
    int callIndex = call->index;
    triggerCallInfoChange(phoneId);
    if(call->callState == CallState::CALL_ENDED ) {
        //Clear call cache in server
        auto f = std::async(std::launch::async, [this, callIndex, phoneId]() {
         std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            bool isCallRemoved = findAndRemoveMatchingCall(callIndex);
            if(isCallRemoved) {
                // Event to update the call cache for clients.
                triggerCallListAfterCallEnd(phoneId);
            }
        }).share();
        taskQ_->add(f);
    }
}

void CallManagerServerImpl::onECallRedial(int phoneId, bool willECallRedial,
    telux::tel::ReasonType reason) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId, " willECallRedial ", willECallRedial,
        "Redial reason", static_cast<int>(reason));
    ::telStub::ECallRedialInfoEvent eCallInfoEvent;
    ::eventService::EventResponse anyResponse;
    eCallInfoEvent.set_will_ecall_redial(willECallRedial);
    eCallInfoEvent.set_reason(static_cast<telStub::ReasonType>(reason));
    eCallInfoEvent.set_phone_id(phoneId);
    anyResponse.set_filter(TEL_CALL_FILTER);
    anyResponse.mutable_any()->PackFrom(eCallInfoEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
    if((willECallRedial)  &&
        ((reason == telux::tel::ReasonType::CALL_ORIG_FAILURE)
            || (reason == telux::tel::ReasonType::CALL_DROP))) {
        eCallRedialIsOngoing_ = true;
        LOG(DEBUG, __FUNCTION__, " Ecall will redial");
    } else {
        LOG(DEBUG, __FUNCTION__, " Ecall will not redial ");
        eCallRedialIsOngoing_ = false;
    }
}

grpc::Status CallManagerServerImpl::ConfigureECallRedial(ServerContext* context,
    const telStub::ConfigureECallRedialRequest* request,
    telStub::ConfigureECallRedialResponse* response) {
    telux::common::ErrorCode error;
    telux::common::Status status;
    int cbDelay;
    bool isCallback = true;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::vector<int> data;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(SLOT_1, jsonfilename, rootObj);
        for(int d : request->time_gap()) {
            data.emplace_back(d);
        }
        int size = data.size();
        std::string timeGapInString = "";
        bool isTimeGapDataAsper3GPP = true;
        std::vector<int> timeGapAsPer3GPP = {5000, 60000, 60000, 60000, 180000};

        getJsonForApiResponseSlot(SLOT_1, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "configureECallRedial", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
        if(request->config() == telStub::RedialConfigType::REDIAL_CONFIG_CALL_ORIG) {
            if((size < MIN_REDIAL_CONFIG) || (size >= MAX_CALLORIG_REDIAL_CONFIG)) {
                response->set_error(commonStub::ErrorCode::REQUEST_NOT_SUPPORTED);
                return grpc::Status::OK;
            }
        } else if(request->config() == telStub::RedialConfigType::REDIAL_CONFIG_CALL_DROP) {
            if((size < MIN_REDIAL_CONFIG) || (size > MAX_CALLDROP_REDIAL_CONFIG)) {
                response->set_error(commonStub::ErrorCode::REQUEST_NOT_SUPPORTED);
                return grpc::Status::OK;
            }
        } else {
            return grpc::Status(grpc::StatusCode::INTERNAL, " Incorrect redial config");
        }
        for (int i = 0; i < size; i++) {
            LOG(DEBUG, __FUNCTION__," data recieved from request", static_cast<int>(data.at(i)));
            if(i <= MIN_VALUE_TIMEGAP_UNTIL_INDEX4) {
                if(data[i] < timeGapAsPer3GPP[i]) {
                    isTimeGapDataAsper3GPP = false;
                    break;
                }
            }
            if(i >= MIN_VALUE_TIMEGAP_AFTER_INDEX4) {
                if(data[i] < timeGapAsPer3GPP[MIN_VALUE_TIMEGAP_UNTIL_INDEX4]) {
                    isTimeGapDataAsper3GPP = false;
                    break;
                }
            }
        }
        if(isTimeGapDataAsper3GPP) {
            timeGapInString = CommonUtils::convertIntVectorToString(data);
            LOG(DEBUG, __FUNCTION__," String value is ", timeGapInString);
            if(request->config() == telStub::RedialConfigType::REDIAL_CONFIG_CALL_ORIG) {
                rootObj["ICallManager"]["eCallRedialTimeGap"]["callOrigFailure"] = timeGapInString;
            } else if(request->config() == telStub::RedialConfigType::REDIAL_CONFIG_CALL_DROP) {
                rootObj["ICallManager"]["eCallRedialTimeGap"]["callDrop"] = timeGapInString;
            } else {
                return grpc::Status(grpc::StatusCode::INTERNAL, "Incorrect redial config");
            }
            LOG(DEBUG, __FUNCTION__," String is data  ", timeGapInString);
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[SLOT_1] = rootObj;
            response->set_error(static_cast<commonStub::ErrorCode>(error));
        } else {
            response->set_error(commonStub::ErrorCode::REQUEST_NOT_SUPPORTED);
        }
        LOG(DEBUG, __FUNCTION__, "Error is ", static_cast<int>(error));
    }
    return readStatus;
}

std::vector<std::shared_ptr<CallInfo>>
    CallManagerServerImpl::fetchSlotIdCalls(int phoneId) {
    std::vector<std::shared_ptr<CallInfo>> calls;
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if((*callIterator)->phoneId == phoneId) {
            calls.emplace_back(*callIterator);
            logCallDetails(*callIterator);
        }
    }
    return calls;
}

grpc::Status CallManagerServerImpl::updateCalls(ServerContext* context,
    const telStub::UpdateCurrentCallsRequest* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    triggerCallInfoChange(phoneId);
    return grpc::Status::OK;
}

void CallManagerServerImpl::triggerCallInfoChange(int phoneId) {
    LOG(DEBUG, __FUNCTION__, "PhoneId ", phoneId);
    ::telStub::CallStateChangeEvent callStateChangeEvent;
    ::eventService::EventResponse anyResponse;
    std::vector<std::shared_ptr<CallInfo>> calls = fetchSlotIdCalls(phoneId);
    for(auto &it : calls) {
        telStub::Call *result = callStateChangeEvent.add_calls();
        result->set_call_state(static_cast<telStub::CallState>(it->callState));
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"CallState is ", static_cast<int>(it->callState));
        result->set_call_index(it->index);
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"CallIndex is ", static_cast<int>(it->index));
        result->set_call_direction
        (static_cast<telStub::CallDirection_Direction>(it->callDirection));
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"Calldirection is ",
        static_cast<int>(it->callDirection));
        result->set_remote_party_number(it->remotePartyNumber);
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"remotePartyNumber is ",
        static_cast<std::string>(it->remotePartyNumber));
        result->set_call_end_cause(static_cast<telStub::CallEndCause_Cause>(it->callEndCause));
        result->set_sip_error_code(it->sipErrorCode);
        result->set_is_multi_party_call(it->isMultiPartyCall);
        result->set_is_mpty(it->isMpty);
        LOG(DEBUG, __FUNCTION__," Rtt mode: ", static_cast<int>(it->mode),
            " Local capability: ", static_cast<int>(it->localRttCapability),
            " Peer capability: ", static_cast<int>(it->peerRttCapability),
            " Call type: ", static_cast<int>(it->callType));
        result->set_mode(static_cast<telStub::RttMode>(it->mode));
        result->set_local_rtt_capability(static_cast<telStub::RttMode>(it->localRttCapability));
        result->set_peer_rtt_capability(static_cast<telStub::RttMode>(it->peerRttCapability));
        result->set_call_type(static_cast<telStub::CallType>(it->callType));
    }
    callStateChangeEvent.set_phone_id(phoneId);
    anyResponse.set_filter(TEL_CALL_FILTER);
    anyResponse.mutable_any()->PackFrom(callStateChangeEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void CallManagerServerImpl::changeRttModeOfCall(RttMode mode, int index, int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<CallInfo> call = findCallAndUpdateRttMode(index, mode, phoneId);
    triggerCallInfoChangeEvent(phoneId, call);
}

void CallManagerServerImpl::changeCallState(int phoneId, std::string action,
    int index) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId);
    CallState state = Helper::getCallState(action);
    if((eCallRedialIsOngoing_) && (state == CallState::CALL_DIALING)) {
        std::unique_lock<std::mutex> lock(mtx);
        {
            while(!callEndOperationCompleted_) {
                cv.wait(lock);
                if(callEndOperationCompleted_) {
                    break;
                }
            }
            if(callEndOperationCompleted_) {
                calls_.emplace_back(redialECallCache_);
                LOG(DEBUG, __FUNCTION__, " Redial eCall cache is added to call list" );
            }
            callEndOperationCompleted_ = false;
        }
        std::shared_ptr<CallInfo> call = findCallAndUpdateCallState(index, state, phoneId);
        triggerCallInfoChangeEvent(phoneId, call);
    } else {
        LOG(DEBUG, __FUNCTION__, " Ecall is not redialing or call state is not dialing" );
        std::shared_ptr<CallInfo> call = findCallAndUpdateCallState(index, state, phoneId);
        triggerCallInfoChangeEvent(phoneId, call);
    }

}

void CallManagerServerImpl::triggerCallListAfterCallEnd(int phoneId) {
    LOG(DEBUG, __FUNCTION__, " PhoneId ", phoneId);
    ::telStub::CallStateChangeEvent callStateChangeEvent;
    ::eventService::EventResponse anyResponse;
    std::vector<std::shared_ptr<CallInfo>> calls = calls_;
    for(auto &it : calls) {
        telStub::Call *result = callStateChangeEvent.add_calls();
        result->set_call_state(static_cast<telStub::CallState>(it->callState));
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"CallState is ", static_cast<int>(it->callState));
        result->set_call_index(it->index);
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"CallIndex is ", static_cast<int>(it->index));
        result->set_call_direction
        (static_cast<telStub::CallDirection_Direction>(it->callDirection));
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"Calldirection is ",
        static_cast<int>(it->callDirection));
        result->set_remote_party_number(it->remotePartyNumber);
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"remotePartyNumber is ",
        static_cast<std::string>(it->remotePartyNumber));
        result->set_call_end_cause(static_cast<telStub::CallEndCause_Cause>(it->callEndCause));
        result->set_sip_error_code(it->sipErrorCode);
        result->set_is_multi_party_call(it->isMultiPartyCall);
        result->set_is_mpty(it->isMpty);
    }
    callStateChangeEvent.set_phone_id(phoneId);
    anyResponse.set_filter(TEL_CALL_FILTER);
    anyResponse.mutable_any()->PackFrom(callStateChangeEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
    std::lock_guard<std::mutex> lock(mtx);
    {
        callEndOperationCompleted_ = true;
        cv.notify_all();
    }
}

std::shared_ptr<CallInfo> CallManagerServerImpl::findCallAndUpdateCallState(
    int index, CallState action, int phoneId) {
    LOG(DEBUG, __FUNCTION__," callIndex ", index, " callState is ", static_cast<int>(action),
    "phoneId ", phoneId );
    std::vector<std::shared_ptr<CallInfo>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);

    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallInfo> call) {
        return find(call, index, phoneId);
    });

    if (iter != std::end(calls_)) {
        LOG(DEBUG, __FUNCTION__, " found matched call");
        (*iter)->callState = action;
        return *iter;
    } else {
        return nullptr;
    }
}

std::shared_ptr<CallInfo> CallManagerServerImpl::findCallAndUpdateRttMode(
    int index, RttMode mode, int phoneId) {
    LOG(DEBUG, __FUNCTION__," Call Index ", index, " Rtt mode ", static_cast<int>(mode) );
    std::vector<std::shared_ptr<CallInfo>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);

    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallInfo> call) {
        return find(call, index, phoneId);
    });

    if (iter != std::end(calls_)) {
        LOG(DEBUG, __FUNCTION__, " found matched call");
        (*iter)->mode = mode;
        (*iter)->peerRttCapability = mode;
        return *iter;
    } else {
        return nullptr;
    }
}

bool CallManagerServerImpl::find(std::shared_ptr<CallInfo> call, int index, int phoneId) {
    if((call->index == index) && (call->phoneId == phoneId)) {
        return true;
    } else {
        return false;
    }
}

bool CallManagerServerImpl::findAndRemoveMatchingCall(int callIndex) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<CallInfo>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);

    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallInfo> call) {
        if(call->index == callIndex ) {
            if(eCallRedialIsOngoing_)
            {
                // Save eCall
                LOG(DEBUG, __FUNCTION__, " Saving ecall cache for next redial");
                redialECallCache_ = call;
            }
            return true;
        } else {
            return false;
        }
    });
    if (iter != std::end(calls_)) {
        calls_.erase(iter);
        LOG(DEBUG, __FUNCTION__, " found matched call");
        return true;
    } else {
        return false;
    }
}
