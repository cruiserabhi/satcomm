/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ECallManager.cpp
 *
 * @brief      ECallManager class provides methods to initiate an eCall and answer an incoming call
 *             (typically PSAP callback). It manages various subsytems(location, audio, etc.) using
 *             Telematics-SDK, in order to handle the eCall appropriately.
 */

#include <iostream>

#include "ECallManager.hpp"
#include "../../common/utils/Utils.hpp"

#define DEFAULT_ECALL_CONFIG_FILE_PATH "/etc"
#define DEFAULT_ECALL_CONFIG_FILE_NAME "eCall.conf"
#define DEFAULT_LOCATION_FIX_INTERVAL_MS 100
#define CLIENT_NAME "ECall-Manager: "

ECallManager::ECallManager()
   : telClient_(nullptr)
   , locClient_(nullptr)
   , audioClient_(nullptr)
   , thermClient_(nullptr)
   , phoneId_(-1)
   , locUpdateIntervalMs_(DEFAULT_LOCATION_FIX_INTERVAL_MS)
   , locFixReceived_(false)
   , voiceSampleRate_(16000)
   , voiceFormat_(AudioFormat::PCM_16BIT_SIGNED)
   , voiceChannels_(ChannelType::LEFT | ChannelType::RIGHT)
   , ecnrMode_(EcnrMode::ENABLE)
   , isTpsEcallOverImsTriggered(false) {
}

ECallManager::~ECallManager() {
}

/**
 * Initialize necessary Telematics-SDK components like location, audio, etc. and get required
 * parameters from the configuration file
 */
telux::common::Status ECallManager::init() {
    telClient_ = std::make_shared<TelClient>();
    auto status = telClient_->init();
    if (status != telux::common::Status::SUCCESS) {
        return status;
    }
    locClient_ = std::make_shared<LocationClient>();
    status = locClient_->init();
    if (status != telux::common::Status::SUCCESS) {
        return status;
    }
    audioClient_ = std::make_shared<AudioClient>();
    status = audioClient_->init();
    if (status != telux::common::Status::SUCCESS) {
        return status;
    }
    thermClient_ = std::make_shared<ThermClient>();
    status = thermClient_->init();
    if (status != telux::common::Status::SUCCESS) {
        return status;
    }

    // Parse the eCall settings and fetch the static MSD data
    parseAppConfig();

    return telux::common::Status::SUCCESS;
}

/**
 * Function to trigger the standard eCall procedure(eg.112)
 */
telux::common::Status ECallManager::triggerECall(
    int phoneId, ECallCategory category, ECallVariant variant, bool transmitMsd,
    std::vector<uint8_t> msdPdu) {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    if (telClient_->isECallInProgress()) {
        std::cout << CLIENT_NAME << "An ECall is in progress already " << std::endl;
        return telux::common::Status::FAILED;
    }
    msdPdu_.clear();
    if(!msdPdu.empty()) {
        msdPdu_ = msdPdu;
    }
    setup(phoneId);
    if (transmitMsd && msdPdu_.empty() && !isLocationReceived()) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        if (std::cv_status::timeout
            == locUpdateCV_.wait_for(lock, std::chrono::milliseconds(locUpdateIntervalMs_))) {
            std::cout << CLIENT_NAME << "Error: Location fetch timeout! " << std::endl;
        }
    }
    auto status = telClient_->startECall(
        phoneId, msdPdu_, msdData_, category, variant, transmitMsd, shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to initiate eCall " << std::endl;
        cleanup();
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "ECall initiated " << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Function to trigger the self test ERA-GLONASS eCall to a specified number.
 */
telux::common::Status ECallManager::triggerECall(
    int phoneId, const std::string dialNumber,
    std::vector<uint8_t> msdPdu) {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    if (telClient_->isECallInProgress()) {
        std::cout << CLIENT_NAME << "An ECall is in progress already " << std::endl;
        return telux::common::Status::FAILED;
    }
    msdPdu_.clear();
    if(!msdPdu.empty()) {
        msdPdu_ = msdPdu;
    }
    setup(phoneId);
    if (msdPdu_.empty() && !isLocationReceived()) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        if (std::cv_status::timeout
            == locUpdateCV_.wait_for(lock, std::chrono::milliseconds(locUpdateIntervalMs_))) {
            std::cout << CLIENT_NAME << "Error: Location fetch timeout! " << std::endl;
        }
    }
    auto status = telClient_->startECall(
        phoneId, msdPdu_, dialNumber, shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to initiate self test eCall " << std::endl;
        cleanup();
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Self test eCall initiated " << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Function to trigger a voice eCall procedure to the specified phone number
 */
telux::common::Status ECallManager::triggerECall(
    int phoneId, ECallCategory category, const std::string dialNumber, bool transmitMsd,
    std::vector<uint8_t> msdPdu) {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    if (telClient_->isECallInProgress()) {
        std::cout << CLIENT_NAME << "An ECall is in progress already " << std::endl;
        return telux::common::Status::FAILED;
    }
    msdPdu_.clear();
    if(!msdPdu.empty()) {
        msdPdu_ = msdPdu;
    }
    setup(phoneId);
    if (transmitMsd && msdPdu_.empty() && !isLocationReceived()) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        if (std::cv_status::timeout
            == locUpdateCV_.wait_for(lock, std::chrono::milliseconds(locUpdateIntervalMs_))) {
            std::cout << CLIENT_NAME << "Error: Location fetch timeout! " << std::endl;
        }
    }
    auto status = telClient_->startECall(
        phoneId, msdPdu_, msdData_, category, dialNumber, transmitMsd, shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to initiate Voice eCall " << std::endl;
        cleanup();
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Voice ECall initiated " << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Function to trigger a voice eCall procedure to the specified phone number over IMS
 *
 */
telux::common::Status ECallManager::triggerECall(
    int phoneId, const std::string dialNumber, std::string contentType, std::string acceptInfo) {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    if (telClient_->isECallInProgress()) {
        std::cout << CLIENT_NAME << "An ECall is in progress already " << std::endl;
        return telux::common::Status::FAILED;
    }
    char delimiter = '\n';
    std::string msdData;
    std::cout << "Enter MSD PDU: ";
    std::getline(std::cin, msdData, delimiter);
    std::vector<uint8_t> rawData;
    if (!msdData.empty()) {
        rawData = Utils::convertHexToBytes(msdData);
    } else {
        rawData = {2, 41, 68, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0, 128, 4, 52, 10, 140,
            65, 89, 164, 56, 119, 207, 131, 54, 210, 63, 65, 104, 16, 24, 8, 32, 19, 198, 68, 0, 0,
            8, 20};
    }
    isTpsEcallOverImsTriggered = true;
    setup(phoneId);
    auto status = telClient_->startECall(
        phoneId, rawData, dialNumber, contentType, acceptInfo, shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to initiate Voice eCall over IMS " << std::endl;
        cleanup();
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Voice ECall initiated over IMS " << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Send MSD for Tps eCall over IMS
 *
 */
telux::common::Status ECallManager::updateEcallMSD() {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    char delimiter = '\n';
    std::string msdData;
    std::cout << "Enter MSD PDU: ";
    std::getline(std::cin, msdData, delimiter);
    std::vector<uint8_t> rawData;
    if (!msdData.empty()) {
        rawData = Utils::convertHexToBytes(msdData);
    } else {
        rawData = {2, 41, 68, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0, 128, 4, 52, 10, 140,
            65, 89, 164, 56, 119, 207, 131, 54, 210, 63, 65, 104, 16, 24, 8, 32, 19, 198, 68, 0, 0,
            48, 20};
    }
    auto status = telClient_->updateTpsEcallOverImsMSD(phoneId_, rawData);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to update MSD for Voice eCall over IMS " << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Update MSD for Voice ECall over IMS initiated " << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Answer an incoming Call
 */
telux::common::Status ECallManager::answerCall(int phoneId) {
    if (!telClient_) {
        std::cout << CLIENT_NAME << " Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    if (telClient_->isECallInProgress()) {
        // If the existing/in-progress call is an MT call on the same phoneId, allow the app to
        // answer the WAITING call
        if (telClient_->getECallDirection() == telux::tel::CallDirection::INCOMING) {
            if (phoneId_ == phoneId) {
                std::cout << CLIENT_NAME << " Accepting the WAITING call" << std::endl;
            } else {
                std::cout << CLIENT_NAME << " Operation not supported by the application"
                          << std::endl;
                return telux::common::Status::FAILED;
            }
        } else {
            std::cout << CLIENT_NAME << " An ECall is in progress already " << std::endl;
            return telux::common::Status::FAILED;
        }
    }
    setup(phoneId);
    auto status = telClient_->answer(phoneId_, shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to answer call " << std::endl;
        cleanup();
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Incoming call answered" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Hang-up an ongoing Call
 */
telux::common::Status ECallManager::hangupCall(int phoneId, int callIndex) {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->hangup(phoneId, callIndex);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to hangup the call" << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Call hang-up successful" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Dump the list of calls in progress
 */
telux::common::Status ECallManager::getCalls() {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->getCurrentCalls();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get current calls" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Request status of various eCall HLAP timers
 */
telux::common::Status ECallManager::requestHlapTimerStatus(int phoneId) {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->requestECallHlapTimerStatus(phoneId);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request for HLAP timers status" << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Sent request for HLAP timers status" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status ECallManager::getECallConfig() {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->getECallConfig();
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get eCall configuration" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Request to get the value of POST TEST REGISTRATION timer.
 */
telux::common::ErrorCode ECallManager::getECallPostTestRegistrationTimer(int phoneId) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::ErrorCode::INVALID_STATE;
    }
    auto errorCode = telClient_->getECallPostTestRegistrationTimer(phoneId);
    if(errorCode != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get post test registration timer" << std::endl;
    }
    return errorCode;
}

/**
 * Request to set the value of POST TEST REGISTRATION timer.
 */
telux::common::Status ECallManager::setPostTestRegistrationTimer(int phoneId,
    uint32_t timeDuration) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->setPostTestRegistrationTimer(phoneId, timeDuration);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to set post test registration timer" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Request to get eCall redial parameters for call origination failure and call drop.
 */
telux::common::ErrorCode ECallManager::getECallRedialConfig() {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::ErrorCode::INVALID_STATE;
    }
    auto errorCode = telClient_->getECallRedialConfig();
    if(errorCode != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get eCall redial configuration" << std::endl;
    }
    return errorCode;
}

telux::common::Status ECallManager::setECallConfig(EcallConfig config) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->setECallConfig(config);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to set eCall configuration" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status ECallManager::restartECallHlapTimer(int phoneId, EcallHlapTimerId id,
    int duration) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->restartECallHlapTimer(phoneId, id, duration);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME
            << "Failed to send request to restart eCall HLAP timer" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status ECallManager::getEncodedOptionalAdditionalDataContent() {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    std::vector<uint8_t> data;
    auto status = telClient_->getEncodedOptionalAdditionalDataContent(
        optionalAdditionalDataContent_, data);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get encoded optional additional data content"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::ErrorCode ECallManager::getECallMsdPayload() {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }
    std::vector<uint8_t> msdPdu = {};
    auto errCode = telClient_->getECallMsdPayload(msdData_, msdPdu);
    if (errCode != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get eCall MSD payload" << std::endl;
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }
    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Request to stop T10 eCall High Level Application Protocol(HLAP) timer
 */
telux::common::Status ECallManager::stopT10Timer(int phoneId) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->stopT10Timer(phoneId);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request to stop T10 HLAP timer" << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Sent request to stop T10 HLAP timer" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Request to set the value of eCall High Level Application Protocol(HLAP) timer
 */
telux::common::Status ECallManager::setHlapTimer(int phoneId, HlapTimerType type,
    uint32_t timeDuration) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->setHlapTimer(phoneId, type, timeDuration);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request to set HLAP timer" << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Sent request to set HLAP timer" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Request to get the value of eCall High Level Application Protocol(HLAP) timer
 */
telux::common::Status ECallManager::getHlapTimer(int phoneId, HlapTimerType type) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->getHlapTimer(phoneId, type);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request to get HLAP timer" << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Sent request to get HLAP timer" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Function to enable necessary functionalities in various subsystems(location, audio, etc.),
 * that are required for an eCall
 */
void ECallManager::setup(int phoneId) {
    phoneId_ = phoneId;
    // Start voice session
    if (!audioClient_) {
        std::cout << CLIENT_NAME << "Invalid Audio Client, cannot establish voice conversation"
                  << std::endl;
    } else {
        audioClient_->startVoiceSession(
            phoneId, audioDevices_, voiceSampleRate_, voiceFormat_, voiceChannels_, ecnrMode_);
    }
    // Get the location updates. This application doesn't update the MSD automatically when a TPS
    // eCall over IMS is triggered or when user provides MSD in raw PDU format(contains location
    // info). Hence location reports are not enabled in these scenarios.
    if(!isTpsEcallOverImsTriggered && msdPdu_.empty()) {
        setLocationReceived(false);
        if (!locClient_) {
            std::cout << CLIENT_NAME << "Invalid Location Client, cannot provide current location"
                      << std::endl;
        } else {
            locClient_->startLocUpdates(locUpdateIntervalMs_, shared_from_this());
        }
    }
    // Disable Thermal auto-shutdown
    if (!thermClient_) {
        std::cout << CLIENT_NAME << "Invalid Thermal Client, cannot disable thermal auto-shutdown"
                  << std::endl;
    } else {
        thermClient_->disableAutoShutdown();
    }
}

/**
 * Function to disable the functionalities in various subsystems(location, audio, etc.). Typically
 * performed when an eCall ends.
 */
void ECallManager::cleanup() {
    // Stop voice session
    if (!audioClient_) {
        std::cout << CLIENT_NAME << "Invalid Audio Client, cannot disable voice conversation"
                  << std::endl;
    } else {
        audioClient_->stopVoiceSession();
    }
    // Get the location updates
    if (!locClient_) {
        std::cout << CLIENT_NAME << "Invalid Location Client, cannot stop location updates"
                  << std::endl;
    } else {
        locClient_->stopLocUpdates();
    }
    // Enable Thermal auto-shutdown
    if (!thermClient_) {
        std::cout << CLIENT_NAME << "Invalid Thermal Client, cannot enable thermal auto-shutdown"
                  << std::endl;
    } else {
        thermClient_->enableAutoShutdown();
    }
    phoneId_ = -1;
    isTpsEcallOverImsTriggered = false;
}

/**
 * Function to update the cached MSD data stored in Modem
 */
telux::common::Status ECallManager::updateMSD(int phoneId) {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->updateECallMSD(phoneId, msdData_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to update MSD " << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}
/**
 * Function to indicate if atleast one location fix is received after the eCall is triggered.
 */
bool ECallManager::isLocationReceived() {
    std::unique_lock<std::mutex> lock(mutex_);
    return locFixReceived_;
}

void ECallManager::setLocationReceived(bool state) {
    std::unique_lock<std::mutex> lock(mutex_);
    locFixReceived_ = state;
}

/**
 * This function will be invoked whenever a new location-fix is received from the location client.
 */
void ECallManager::onLocationUpdate(ECallLocationInfo locInfo) {
    msdData_.control.positionCanBeTrusted = true;
    msdData_.vehicleLocation.positionLatitude = locInfo.latitude;
    msdData_.vehicleLocation.positionLongitude = locInfo.longitude;
    msdData_.timestamp = locInfo.timestamp;
    msdData_.vehicleDirection = locInfo.direction;
    if (telClient_->isECallInProgress()) {
        if(!isTpsEcallOverImsTriggered) {
            updateMSD(phoneId_);
            telClient_->setECallMsd(msdData_);
        }
    } else {
        setLocationReceived(true);
        locUpdateCV_.notify_all();
    }
}

/**
 * Function to parse the settings from the eCall configuration file and fetch the static MSD Data
 */
void ECallManager::parseAppConfig() {
    std::shared_ptr<ConfigParser> appSettings = std::make_shared<ConfigParser>(
        DEFAULT_ECALL_CONFIG_FILE_NAME, DEFAULT_ECALL_CONFIG_FILE_PATH);
    // Get the location of MSD data file and fetch the static MSD data
    std::string param = appSettings->getValue("MSD_FILE_NAME");
    if (!param.empty()) {
        MsdProvider msdSettings;
        std::string filePath = appSettings->getValue("MSD_FILE_PATH");
        optionalAdditionalDataContent_ = msdSettings.readEuroNcapOptionalAdditionalDataContent(
            param, filePath);
        std::vector<uint8_t> data;
        if (telClient_) {
            auto status = telClient_->getEncodedOptionalAdditionalDataContent(
                optionalAdditionalDataContent_, data);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << CLIENT_NAME << "Optional additional data content encoding failed"
                    << std::endl;
            }
            msdSettings.setOptionalAdditionalDataContent(data);
        }
        msdSettings.init(param, filePath);
        msdData_ = msdSettings.getMsd();
        if (telClient_) {
           telClient_->setECallMsd(msdData_);
        }
    } else {
        std::cout << CLIENT_NAME << "MSD data file not found! " << std::endl;
    }
    // Get the periodic interval for which location updates needs to be received
    param = appSettings->getValue("LOCATION_UPDATE_INTERVAL_MS");
    if (!param.empty()) {
        locUpdateIntervalMs_ = atol(param.c_str());
    } else {
        std::cout << CLIENT_NAME
                  << "Using default location update interval(in ms): " << locUpdateIntervalMs_
                  << std::endl;
    }
    // Get the configured output audio device
    param = appSettings->getValue("AUDIO_OUTPUT_DEVICE_TYPE");
    if (!param.empty()) {
        std::stringstream ss(param);
        int i = -1;
        audioDevices_.clear();
        std::cout << CLIENT_NAME << "Using audio devices: ";
        while(ss >> i) {
            audioDevices_.emplace_back(static_cast<DeviceType>(i));
            std::cout << i << "  ";
            if(ss.peek() == ',') {
                ss.ignore();
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Using default audio devices" << std::endl;
    }
    // Get the configured audio sample rate
    param = appSettings->getValue("VOICE_SAMPLE_RATE");
    if (!param.empty()) {
        voiceSampleRate_ = atol(param.c_str());
    } else {
        std::cout << CLIENT_NAME << "Using default audio sample rate: " << voiceSampleRate_
                  << std::endl;
    }
    // Get the configured audio channels
    param = appSettings->getValue("VOICE_CHANNEL_TYPE");
    if (param.compare("LEFT") == 0) {
        voiceChannels_ = ChannelType::LEFT;
    } else if (param.compare("RIGHT") == 0) {
        voiceChannels_ = ChannelType::RIGHT;
    } else if (param.compare("STEREO") == 0) {
        voiceChannels_ = ChannelType::LEFT | ChannelType::RIGHT;
    } else {
        std::cout << CLIENT_NAME << "Using default audio channels: " << voiceChannels_ << std::endl;
    }
    // Get the configured audio sream format
    param = appSettings->getValue("VOICE_STREAM_FORMAT");
    if (param.compare("PCM_16BIT_SIGNED") == 0) {
        voiceFormat_ = AudioFormat::PCM_16BIT_SIGNED;
    } else {
        std::cout << CLIENT_NAME << "Using default audio stream format" << std::endl;
    }
    // Get the ecnr mode status
    param = appSettings->getValue("ECNR_MODE");
    if (param.compare("DISABLE") == 0) {
        ecnrMode_ = EcnrMode::DISABLE;
    } else if (param.compare("ENABLE") == 0) {
        ecnrMode_ = EcnrMode::ENABLE;
    } else {
        std::cout << CLIENT_NAME << "Enabling ecnr mode by default" << std::endl;
    }
    // Get the ERA-GLONASS mode
    param = appSettings->getValue("ERAGLONASS_ECALL");
    if (param.compare("DISABLE") == 0) {
        telClient_->setEraGlonassEnabled(false);
    } else if (param.compare("ENABLE") == 0) {
        telClient_->setEraGlonassEnabled(true);
    } else {
        std::cout << CLIENT_NAME << "Disabling ERAGLONASS_ECALL mode by default" << std::endl;
        telClient_->setEraGlonassEnabled(false);
    }
}

telux::common::Status ECallManager::configureECallRedial(int config, std::vector<int> &timeGap) {
    if (!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->configureECallRedial(config, timeGap);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to configure eCall redial " << std::endl;
        return status;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * This function will be invoked when an eCall is failed to establish or an eCall is disconnected
 */
void ECallManager::onCallDisconnect() {
    cleanup();
}

/** This function is called when the eCall connection is in progress i.e, during redial from
 * application or modem
 */
void ECallManager::onCallConnect(int phoneId) {
    setup(phoneId);
}
