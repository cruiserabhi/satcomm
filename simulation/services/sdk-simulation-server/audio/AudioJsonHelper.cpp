/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "AudioJsonHelper.hpp"
#include "libs/common/JsonParser.hpp"

AudioJsonHelper::AudioJsonHelper(){
    LOG(DEBUG, __FUNCTION__);
}

AudioJsonHelper::~AudioJsonHelper(){
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status AudioJsonHelper::loadJson() {
    LOG(DEBUG, __FUNCTION__," Api Json Path: ", AUDIO_MANAGER_API_JSON);

    telux::common::ErrorCode error;

    error = JsonParser::readFromJsonFile(rootObj_, AUDIO_MANAGER_API_JSON);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return telux::common::Status::NOSUCH;
    }

    return telux::common::Status::SUCCESS;
}

telux::common::ServiceStatus AudioJsonHelper::initServiceStatus() {
    /* Reads value from json */
    std::string srvStatus = rootObj_["IAudioManager"]["IsSubsystemReady"].asString();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", srvStatus);
    std::lock_guard<std::mutex> lock(mutex_);
    serviceStatus_ = CommonUtils::mapServiceStatus(srvStatus);
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(serviceStatus_));
    return serviceStatus_;
}

int AudioJsonHelper::getSubsystemReadyDelay() {
    int subSysDelay = rootObj_["IAudioManager"]["IsSubsystemReadyDelay"].asInt();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemDelay: ", subSysDelay);
    return subSysDelay;
}

void AudioJsonHelper::getApiResponse(ApiResponse *apiResponse, std::string className,
    std::string apiname){

    CommonUtils::getValues(rootObj_, className, apiname, apiResponse->status, apiResponse->error,
        apiResponse->cbDelay);
}

telux::common::Status AudioJsonHelper::getApiRequestStatus(std::string apiname) {
    std::string statusStr = rootObj_["IAudioManager"][apiname]["status"].asString();
    return CommonUtils::mapStatus(statusStr);
}
