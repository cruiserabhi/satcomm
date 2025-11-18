/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DgnssManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/JsonParser.hpp"
#include "common/CommonUtils.hpp"
#include <future>
#include <thread>
namespace telux {

namespace loc {

DgnssManagerStub::DgnssManagerStub(DgnssDataFormat dataFormat) {
    LOG(DEBUG, __FUNCTION__);
    dataFormat_ = dataFormat;
}

std::future<bool> DgnssManagerStub::onSubsystemReady() {
  LOG(DEBUG, __FUNCTION__);
  auto f = std::async(std::launch::async, [&] {
    return waitForInitialization();
  });
  return f;
}

bool DgnssManagerStub::waitForInitialization() {
  LOG(DEBUG, __FUNCTION__);
  std::unique_lock<std::mutex> cvLock(mutex_);
  cv_.wait(cvLock);
  return isSubsystemReady();
}

bool DgnssManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::ServiceStatus DgnssManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::Status DgnssManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async,
    [this, callback]() {
        this->initSync(callback);
        }).share();
        taskQ_.add(f);
    return telux::common::Status::SUCCESS;
}

void DgnssManagerStub::initSync(telux::common::InitResponseCb callback) {
    int cbDelay = 100;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;

    {
        ErrorCode errorCode = JsonParser::readFromJsonFile(rootNode, "api/loc/IDgnssManager.json");
        if(errorCode == ErrorCode::SUCCESS) {
            cbDelay = rootNode["IDgnssManager"]["SubSystemReadinessDelay"].asInt();
            serviceStatus = rootNode["IDgnssManager"]["SubSystemInit"].asBool() == true ? ServiceStatus::SERVICE_AVAILABLE : ServiceStatus::SERVICE_FAILED;
        } else {
            LOG(ERROR, "Unable to read DgnssManager JSON");
        }
    }

    LOG(DEBUG, "Delay: ", cbDelay, " ServiceStatus: ", static_cast<int>(serviceStatus));

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    callback(serviceStatus);
    cv_.notify_all();
}

telux::common::Status DgnssManagerStub::registerListener(
    std::weak_ptr<IDgnssStatusListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if (statusListener_.lock()) {
        LOG(ERROR, __FUNCTION__, " Listener Already Registered");
        return telux::common::Status::INVALIDSTATE;
    }
    if (listener.lock()!=nullptr) {
        LOG(INFO, __FUNCTION__, " Listener Registered");
        statusListener_ = listener;
        return telux::common::Status::SUCCESS;
    } else {
        LOG(ERROR, __FUNCTION__, " Listener Parameter Invalid");
        return telux::common::Status::INVALIDPARAM;
    }
}

telux::common::Status DgnssManagerStub::deRegisterListener(void) {
    LOG(DEBUG, __FUNCTION__);
    if (statusListener_.lock()) {
        statusListener_.reset();
        LOG(INFO, __FUNCTION__, " Listener Deregistered");
        return telux::common::Status::SUCCESS;
    } else {
        LOG(ERROR, __FUNCTION__, " No Listener Registered");
        return telux::common::Status::NOSUBSCRIPTION;
    }
}

telux::common::Status DgnssManagerStub::createSource(DgnssDataFormat dataFormat) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, "api/loc/IDgnssManager.json");
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "IDgnssManager", __FUNCTION__, status, errorCode, cbDelay);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    return status;
}

telux::common::Status DgnssManagerStub::releaseSource(void) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, "api/loc/IDgnssManager.json");
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "IDgnssManager", __FUNCTION__, status, errorCode, cbDelay);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    return status;
}

telux::common::Status DgnssManagerStub::injectCorrectionData(const uint8_t* buffer,
        uint32_t bufferSize) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, "api/loc/IDgnssManager.json");
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "IDgnssManager", __FUNCTION__, status, errorCode, cbDelay);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    return status;
}

DgnssManagerStub::~DgnssManagerStub() {}

}

}
