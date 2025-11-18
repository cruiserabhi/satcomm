/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 * @file    TelClient.cpp
 *
 * @brief   TelClient class provides methods to trigger an eCall, update MSD, answer/hangup a call.
 *          It manages the telephony subsystem using Telematics-SDK APIs.
 *          ERA-GLONASS eCall requirements:
 *          ECall dial duration - This is the connection establishment time i.e. maximum time IVS
 *          can take - starting from initial ERA-GLONASS eCall trigger - to connect the call to
 *          PSAP, including all redial attempts. If the ECALL_DIAL_DURATION expires, the
 *          expectation is to end the ERA-GLONASS eCall origination process.
 *          ECall auto answer time - This is the time interval after emergency
 *          call completion (clear-down) over which IVS stays registered to the network and
 *          automatically answers to incoming callbacks from PSAP.
 */

#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>

#include "TelClient.hpp"
#include "TelClientUtils.hpp"
#include "Utils.hpp"

#define CLIENT_NAME "ECall-Tel-Client: "

TelClient::TelClient()
   : answerCommandCallback_(nullptr)
   , hangupCommandCallback_(nullptr)
   , updateMsdCommandCallback_(nullptr)
   , callMgr_(nullptr)
   , eCall_(nullptr)
   , eCallInprogress_(false)
   , isEraglonassEnabled_(false)
   , eCallScanFailHdlrInstance_(nullptr)
   , isPrivateEcallTriggered(false)
   , isIncomingCallInProgress_(false)
   , isDialDurationTimeOut_(false)
   , stopDialTimer_(false)
   , autoAnswerDuration_(0)
   , isT9TimerActive_(false)
   , willECallRedial_(false)
   , disconnectECallInNextAttempt_(false)
   , clearECall_(false)
   , isAutoAnswerDurationTimeOut_(false) {
}

TelClient::~TelClient() {
    eCallInprogress_ = false;
    isPrivateEcallTriggered = false;
    eCallDataMap_.clear();
}

// Initialize the telephony subsystem
telux::common::Status TelClient::init() {

    answerCommandCallback_ = std::make_shared<AnswerCommandCallback>(shared_from_this());
    hangupCommandCallback_ = std::make_shared<HangupCommandCallback>();
    updateMsdCommandCallback_ = std::make_shared<UpdateMsdCommandCallback>();

    // Get PhoneFactory
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    // Get Call Manager from PhoneFactory
    std::promise<ServiceStatus> prom;
    //  Get the PhoneFactory and CallManager instances
    callMgr_ = phoneFactory.getCallManager([&](ServiceStatus status) {
       prom.set_value(status);
    });
    if (!callMgr_) {
       std::cout << CLIENT_NAME << "Failed to get Call Manager" << std::endl;
       return telux::common::Status::FAILED;
    }

    ServiceStatus callMgrsubSystemStatus = callMgr_->getServiceStatus();
    if(callMgrsubSystemStatus != ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "CallManager subsystem is not ready" << ", Please wait " << std::endl;
    }
    callMgrsubSystemStatus = prom.get_future().get();
    if(callMgrsubSystemStatus == ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "CallManager subsystem is  ready " << std::endl;
    } else {
       std::cout << "Unable to initialise CallManager subsystem " << std::endl;
       return telux::common::Status::FAILED;
    }
    auto status = callMgr_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << " Failed to register a Call listener" << std::endl;
    }
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        eCallScanFailHdlrInstance_ = std::make_shared<EcallScanFailHandler>(shared_from_this());
        if (eCallScanFailHdlrInstance_) {
            auto status = eCallScanFailHdlrInstance_->init();
            if (status != telux::common::Status::SUCCESS) {
                std::cout << CLIENT_NAME << " Failed to init ECallScanFailHandler\n";
                return telux::common::Status::FAILED;
            }
        } else {
            std::cout << " Failed to get ECallScanFailHandler instance\n";
            return telux::common::Status::FAILED;
        }
    }
    return telux::common::Status::SUCCESS;
}

// Indicates whether an eCall is in progress
bool TelClient::isECallInProgress() {
    std::unique_lock<std::mutex> lock(mutex_);
    return eCallInprogress_;
}

// Indicates whether an ERAGLONASS mode is enabled
bool TelClient::isEraGlonassEnabled() {
    return isEraglonassEnabled_;
}

// Set the ERAGLONASS mode
void TelClient::setEraGlonassEnabled(bool isEnabled) {
    isEraglonassEnabled_ = isEnabled;
}

void TelClient::setECallProgressState(bool state) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!state) {
       isPrivateEcallTriggered = false;
    }
    eCallInprogress_ = state;
}

telux::tel::CallDirection TelClient::getECallDirection() {
    if (eCall_) {
        return eCall_->getCallDirection();
    } else {
        return telux::tel::CallDirection::NONE;
    }
}

// Update locally cached MSD recieved after location update
void TelClient::setECallMsd(ECallMsdData& msdData) {
    std::lock_guard<std::mutex> lock(mutex_);
    msdData_ = msdData;
}

// Callback invoked when an incoming call is received
void TelClient::onIncomingCall(std::shared_ptr<ICall> call) {
    std::cout << CLIENT_NAME << std::endl << "Received an incoming call" << std::endl;
    std::cout
        << CLIENT_NAME << "\n Incoming CallInfo: "
        << " Call State: " << TelClientUtils::callStateToString(call->getCallState())
        << "\n Call Index: " << (int)call->getCallIndex()
        << ", Call Direction: " << TelClientUtils::callDirectionToString(call->getCallDirection())
        << ", Phone Number: " << call->getRemotePartyNumber() << std::endl;
    if(isEraGlonassEnabled()) {
        std::cout << CLIENT_NAME <<" isDialDurationTimeOut_: " << isDialDurationTimeOut_
            << ", isT9TimerActive_: " << isT9TimerActive_ << std::endl;
        // Incoming PSAP callback must be answered if T9 HLAP timer is ACTIVE or auto answer
        // dial duration has not expired.
        if((!isAutoAnswerDurationTimeOut_) && (isT9TimerActive_)) {
            answer(call->getPhoneId(), shared_from_this());
        }
    }
}

// Callback invoked when a call status changes
void TelClient::onCallInfoChange(std::shared_ptr<ICall> call) {
    std::cout
        << CLIENT_NAME << "\n CallInfoChange: "
        << " Call State: " << TelClientUtils::callStateToString(call->getCallState())
        << "\n Call Index: " << (int)call->getCallIndex()
        << ", Call Direction: " << TelClientUtils::callDirectionToString(call->getCallDirection())
        << ", Phone Number: " << call->getRemotePartyNumber() << std::endl;
    if(isEraGlonassEnabled()) {
        std::cout << CLIENT_NAME << " willECallRedial_:" << willECallRedial_<< std::endl;
    }
    // During the redial(by modem or app) scenario to setup audio session
    if (call->getCallState() == telux::tel::CallState::CALL_DIALING) {
        if((call->getCallType() == telux::tel::CallType::EMERGENCY_CALL) ||
            (call->getCallType() == telux::tel::CallType::EMERGENCY_IP_CALL)) {
            if (eCall_ == nullptr) {
                eCall_ = call;
                if(isEraGlonassEnabled()) {
                    if(!disconnectECallInNextAttempt_) {
                        setECallProgressState(true);
                        if (callListener_) {
                            callListener_->onCallConnect(call->getPhoneId());
                        }
                    } else {
                        /**
                         * During ERA-GLONASS eCall redial call states transition from
                         * OUTGOING -> CALL_ENDED. Call termination request is sent during
                         * next redial attempt if no relevant eCall was found upon expiry of
                         * dial duration.
                         */
                        std::cout << CLIENT_NAME << "Send ECall termination request" << std::endl;
                        hangup(call->getPhoneId(), call->getCallIndex());
                    }
                } else {
                    setECallProgressState(true);
                    if (callListener_) {
                        callListener_->onCallConnect(call->getPhoneId());
                    }
                }
            } else {
                std::cout << CLIENT_NAME << "eCall ptr is not null\n";
                if(isEraGlonassEnabled()) {
                    if(disconnectECallInNextAttempt_) {
                        /**
                         * During ERA-GLONASS eCall redial call states transition from
                         * OUTGOING -> CALL_ENDED. Call termination request is sent during
                         * next redial attempt if no relevant eCall was found upon expiry of
                         * dial duration.
                         */
                        std::cout << CLIENT_NAME << "Send ECall termination request" << std::endl;
                        hangup(call->getPhoneId(), call->getCallIndex());
                    }
                }
            }
        } else {
            setECallProgressState(false);
            if (callListener_) {
                callListener_->onCallConnect(call->getPhoneId());
            }
        }
    }
    if(isEraGlonassEnabled()) {
        if(call->getCallState() == telux::tel::CallState::CALL_ACTIVE) {
            // The dial timer must be stop when UE has established connection with PSAP.
            std::lock_guard<std::mutex> lock(dialDurationMtx_);
            {
                stopDialTimer_ = true;
                std::cout << " Stop dial timer " << stopDialTimer_ << std::endl;
                dialDurationCv_.notify_all();
            }
        }
    }
    if (call->getCallState() == telux::tel::CallState::CALL_ENDED) {
        if((call->getCallType() == telux::tel::CallType::EMERGENCY_CALL) ||
            (call->getCallType() == telux::tel::CallType::EMERGENCY_IP_CALL) ||
            (isIncomingCallInProgress_)) {
            std::cout << CLIENT_NAME << "  Cause of call termination: "
                << TelClientUtils::callEndCauseToString(call->getCallEndCause())
                << ((call->getSipErrorCode() > 0) ? " and Sip error code: " : "")
                << ((call->getSipErrorCode() > 0) ? std::to_string(call->getSipErrorCode()) : "")
                << std::endl;
            if (eCall_ != nullptr) {
                if (eCall_->getCallIndex() == call->getCallIndex()
                    && eCall_->getPhoneId() == call->getPhoneId()) {
                    if (callListener_) {
                        callListener_->onCallDisconnect();
                    }
                    setECallProgressState(false);
                    isIncomingCallInProgress_ = false;
                    if(isEraGlonassEnabled()) {
                        if(!willECallRedial_ || clearECall_) {
                            // When modem is redialing, eCall_ must not be cleared to ensure
                            // hangup is sent on valid eCall.
                            std::cout << CLIENT_NAME << "  clear eCall cache " << std::endl;
                            eCall_ = nullptr;
                            clearECall_ = false;
                        }
                    }
                }
            }
        } else {
           std::cout << CLIENT_NAME << "  Cause of call termination: "
                << TelClientUtils::callEndCauseToString(call->getCallEndCause())
                << ((call->getSipErrorCode() > 0) ? " and Sip error code: " : "")
                << ((call->getSipErrorCode() > 0) ? std::to_string(call->getSipErrorCode()) : "")
                << std::endl;
            if (callListener_) {
                callListener_->onCallDisconnect();
            }
        }
    }
}

// Callback to notify MSD transmission status
void TelClient::onECallMsdTransmissionStatus(int phoneId, telux::common::ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "MSD is transmitted Successfully" << std::endl;
    } else {
        std::cout << CLIENT_NAME
                  << "MSD transmission failed with error code: " << static_cast<int>(errorCode)
                  << " : " << Utils::getErrorCodeAsString(errorCode) << std::endl;
    }
}

// Callback to notify MSD transmission status
void TelClient::onECallMsdTransmissionStatus(
    int phoneId, telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus) {
    std::cout << CLIENT_NAME << "ECallMsdTransmission  Status: "
              << TelClientUtils::eCallMsdTransmissionStatusToString(msdTransmissionStatus)
              << std::endl;
    eCallDataMap_[phoneId].msdTransmissionStatus = msdTransmissionStatus;
}

// Callback to notify request from PSAP for MSD update
void TelClient::OnMsdUpdateRequest(int phoneId) {
    std::cout << CLIENT_NAME << "Request to send the MSD receieved from PSAP for SlotId "
              << phoneId
              << " for the ecall Type : "
              << (isPrivateEcallTriggered ? "Private ecall" : "Standard or NG ecall")
              << std::endl;
    if (!isPrivateEcallTriggered) {
       ECallMsdData msdData;
       if (isECallInProgress()) {
          {
             std::lock_guard<std::mutex> lock(mutex_);
             msdData = msdData_;
          }
          auto status = updateECallMSD(phoneId, msdData);
          if (status != telux::common::Status::SUCCESS) {
             std::cout << CLIENT_NAME << "Failed to update MSD " << std::endl;
             return;
          }
       }
    }
}

// Notify clients whether redial will be perfomed or not with the reason
void TelClient::onECallRedial(int phoneId, ECallRedialInfo info) {
    std::cout << CLIENT_NAME << " eCall redial will"
              << (info.willECallRedial ? " be performed " : " not be perfomed")
              << (info.willECallRedial ? " and redial reason is " : " and not redial reason is")
              << TelClientUtils::eCallRedialReasonToString(info.reason)
              << std::endl;
    if(isEraGlonassEnabled()) {
        willECallRedial_ = info.willECallRedial;
        if((!(info.willECallRedial)) &&
            ((info.reason == telux::tel::ReasonType::MAX_REDIAL_ATTEMPTED)
            || (info.reason == telux::tel::ReasonType::CALL_CONNECTED))) {
                // The dial timer must stop when hangup is sent by PSAP or
                // maximum redial attempts are exhausted.
                std::lock_guard<std::mutex> lock(dialDurationMtx_);
                {
                    stopDialTimer_ = true;
                    std::cout << " Stop dial timer " << stopDialTimer_ << std::endl;
                    dialDurationCv_.notify_all();
                }
        }
    }
}

// Callback to notify eCall HLAP timers status
void TelClient::onECallHlapTimerEvent(int phoneId, ECallHlapTimerEvents timerEvents) {
    std::string infoStr = "\n";
    std::cout << CLIENT_NAME << " eCall HLAP Timer event on phoneId: " << phoneId << std::endl;
    if ((timerEvents.t2 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t2 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T2 HLAP Timer event : "
                       + TelClientUtils::eCallHlapTimerEventToString(timerEvents.t2) + "\n");
    }
    if ((timerEvents.t5 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t5 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T5 HLAP Timer event : "
                       + TelClientUtils::eCallHlapTimerEventToString(timerEvents.t5) + "\n");
    }
    if ((timerEvents.t6 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t6 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T6 HLAP Timer event : "
                       + TelClientUtils::eCallHlapTimerEventToString(timerEvents.t6) + "\n");
    }
    if ((timerEvents.t7 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t7 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T7 HLAP Timer event : "
                       + TelClientUtils::eCallHlapTimerEventToString(timerEvents.t7) + "\n");
    }
    if ((timerEvents.t9 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t9 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T9 HLAP Timer event : "
                       + TelClientUtils::eCallHlapTimerEventToString(timerEvents.t9) + "\n");
    }
    if ((timerEvents.t10 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t10 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T10 HLAP Timer event : "
                       + TelClientUtils::eCallHlapTimerEventToString(timerEvents.t10) + "\n");
    }
    std::cout << CLIENT_NAME << infoStr << std::endl;
    if(isEraGlonassEnabled()) {
        if(timerEvents.t9 == HlapTimerEvent::STARTED) {
            isT9TimerActive_ = true;
            autoAnswerTimer_ = std::async(std::launch::async, [this] {
                std::cout << " autoAnswerDuration_: " << autoAnswerDuration_ << std::endl;
                std::unique_lock<std::mutex> lock(autoAnswerMtx_);
                {
                    auto start = std::chrono::steady_clock::now();
                    auto timeout = std::chrono::minutes(autoAnswerDuration_);
                    auto predicate = [&] {
                        if(!isT9TimerActive_) {
                            std::cout << " T9 Timer is stopped" << std::endl;
                            return true;
                        } if(std::chrono::steady_clock::now() - start >= timeout){
                            isAutoAnswerDurationTimeOut_ = true;
                            std::cout << "Auto answer Timeout" << std::endl;
                            return true;
                        }
                        return false;
                    };
                    while (!autoAnswerCv_.wait_until(lock, start + timeout, predicate)) {
                        // Loop until the predicate returns true or timeout occurs
                    }
                }
            }).share();
        } else if((timerEvents.t9 == HlapTimerEvent::STOPPED)
            || (timerEvents.t9 == HlapTimerEvent::EXPIRED)) {
                isT9TimerActive_ = false;
                autoAnswerCv_.notify_all();
        } else {
            //Nothing
        }
    }
}

// Callback to notify Telephony subsystem restart
void TelClient::onServiceStatusChange(ServiceStatus status) {
    if (status == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        std::cout << "Telephony subsystem is UNAVAILABLE" << std::endl;
    } else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Telephony subsystem is AVAILABLE" << std::endl;
    }
}

// Callback which provides response to makeECall
void TelClient::makeCallResponse(
    telux::common::ErrorCode errorCode, std::shared_ptr<telux::tel::ICall> call) {
    std::string infoStr = "";
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        infoStr.append("Call is successful,call index - " + static_cast<int>(call->getCallIndex()));
        eCall_ = call;
    } else {
        infoStr.append("Call failed with error code: " + std::to_string(static_cast<int>(errorCode))
                       + ":" + Utils::getErrorCodeAsString(errorCode));
        if (callListener_) {
            callListener_->onCallDisconnect();
        }
        setECallProgressState(false);
    }
    std::cout << CLIENT_NAME << infoStr << std::endl;
}

// Callback which provides response to updateECallMsd command
void TelClient::UpdateMsdCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        std::string infoStr = "";
        infoStr.append(
            "Update MSD failed with error code: " + std::to_string(static_cast<int>(errorCode))
            + ":" + Utils::getErrorCodeAsString(errorCode));
        std::cout << CLIENT_NAME << infoStr << std::endl;
    }
}

TelClient::AnswerCommandCallback::AnswerCommandCallback(std::weak_ptr<TelClient> telClient) {
    eCallTelClient_ = telClient;
}

// Callback which provides response to answer command
void TelClient::AnswerCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
    std::string infoStr = "";
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        infoStr.append(" Answer Call is successful");
    } else {
        infoStr.append(
            " Answer call failed with error code: " + std::to_string(static_cast<int>(errorCode))
            + ":" + Utils::getErrorCodeAsString(errorCode));
        auto sp = eCallTelClient_.lock();
        if (sp) {
            if (sp->callListener_) {
                sp->callListener_->onCallDisconnect();
                sp->callListener_ = nullptr;
            }
            sp->setECallProgressState(false);
            sp->eCall_ = nullptr;
        } else {
            std::cout << CLIENT_NAME << "Obsolete weak pointer\n";
        }
    }
    std::cout << CLIENT_NAME << infoStr << std::endl;
}

// Callback which provides response to hangup command
void TelClient::HangupCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
    std::string infoStr = "";
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        infoStr.append(" Hangup is successful");
    } else if (errorCode == telux::common::ErrorCode::INVALID_CALL_ID) {
        infoStr.append(" Call was hung up already");
    } else {
        infoStr.append(
            " Hangup failed with error code: " + std::to_string(static_cast<int>(errorCode)) + ":"
            + Utils::getErrorCodeAsString(errorCode));
    }
    std::cout << CLIENT_NAME << infoStr << std::endl;
}

// Callback which provides response to HLAP timer status request
void TelClient::hlapTimerStatusResponse(
    telux::common::ErrorCode error, int phoneId, ECallHlapTimerStatus timersStatus) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Get HLAP timers status failed with error code: "
                  << Utils::getErrorCodeAsString(error) << std::endl;
        return;
    }
    std::string infoStr = "eCall HLAP Timers status on phoneId - "
                          + std::to_string(static_cast<int>(phoneId)) + "\n";
    infoStr.append("T2 HLAP Timer Status : "
                   + TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t2) + "\n");
    infoStr.append("T5 HLAP Timer Status : "
                   + TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t5) + "\n");
    infoStr.append("T6 HLAP Timer Status : "
                   + TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t6) + "\n");
    infoStr.append("T7 HLAP Timer Status : "
                   + TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t7) + "\n");
    infoStr.append("T9 HLAP Timer Status : "
                   + TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t9) + "\n");
    infoStr.append("T10 HLAP Timer Status : "
                   + TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t10) + "\n");
    std::cout << CLIENT_NAME << infoStr << std::endl;
}

// Callback which provides response to stop T10 HLAP timer
void TelClient::stopT10TimerResponse(telux::common::ErrorCode error) {
    if(error != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to stop T10 ECall HLAP timer with error code: "
            << Utils::getErrorCodeAsString(error) << std::endl;
        return;
    } else {
        std::cout << CLIENT_NAME << "Successfully stopped T10 ECall HLAP timer" << std::endl;
    }
}

// Callback which provides response for set post test registration timer
void TelClient::setECallPostTestRegistrationTimerResponse(telux::common::ErrorCode error) {
    if(error != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to stop POST TEST REG ECall timer with error code: "
            << Utils::getErrorCodeAsString(error) << std::endl;
        return;
    } else {
        std::cout << CLIENT_NAME << "Successfully set POST TEST REG HLAP timer" << std::endl;
    }
}


// Callback which provides response to set HLAP timer
void TelClient::setHlapTimerResponse(telux::common::ErrorCode error) {
    if(error != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to set ECall HLAP timer with error code: "
            << Utils::getErrorCodeAsString(error) << std::endl;
        return;
    } else {
        std::cout << CLIENT_NAME << "Successfully set ECall HLAP timer" << std::endl;
    }
}

// Callback which provides response to get HLAP timer
void TelClient::getHlapTimerResponse(telux::common::ErrorCode error, uint32_t timeDuration) {
    if(error != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get ECall HLAP timer with error code: "
            << Utils::getErrorCodeAsString(error) << std::endl;
        return;
    } else {
        std::cout << CLIENT_NAME << "Successfully get ECall HLAP timer is " <<
            timeDuration << std::endl;
    }
}

// Callback which provides response to configure ECall redial parameters
void TelClient::configureECallRedialResponse(telux::common::ErrorCode error) {
    if(error != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME <<
            "Configuration of ECall Redial parameters failed with error code: "
            << Utils::getErrorCodeAsString(error) << std::endl;
        return;
    } else {
        std::cout << CLIENT_NAME << "Successfully configured eCall redial parameters" << std::endl;
    }
}

// Callback which provides response for restart of HLAP timer
void TelClient::restartHlapTimerResponse(telux::common::ErrorCode error) {
    if(error != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to restart eCall HLAP timer with error code: "
            << Utils::getErrorCodeAsString(error) << std::endl;
        return;
    } else {
        std::cout << CLIENT_NAME << "Successfully restarted eCall HLAP timer " << std::endl;
    }
}

// Initiate a standard eCall procedure(eg.112)
telux::common::Status TelClient::startECall(int phoneId, std::vector<uint8_t> msdPdu,
    ECallMsdData msdData, ECallCategory category, ECallVariant variant, bool transmitMsd,
    std::shared_ptr<CallStatusListener> callListener) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to initiate an eCall"
                  << std::endl;
        return telux::common::Status::FAILED;
    }
    setECallProgressState(true);
    isPrivateEcallTriggered = false;
    // Get dial duration and redial attempts from user
    char delimiter = '\n';
    std::string temp = "";
    int dialDuration = 5;
    if (isEraGlonassEnabled()) {
        std::cout << "Enter dial duration (in minutes): ";
        std::getline(std::cin, temp, delimiter);
        if(!temp.empty()) {
            try {
                dialDuration = std::stoi(temp);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values." << std::endl;
            }
        } else {
            std::cout << "No input" << std::endl;
        }

        std::cout << "Enter auto answer timer duration (in minutes): ";
        std::getline(std::cin, temp, delimiter);
        if(!temp.empty()) {
            try {
                autoAnswerDuration_ = std::stoi(temp);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values." << std::endl;
            }
        } else {
            std::cout << "No input" << std::endl;
        }
    }
    // Initiate an eCall
    telux::common::Status status = telux::common::Status::FAILED;
    if (transmitMsd) {
        if(msdPdu.empty()) {
            status = callMgr_->makeECall(phoneId, msdData, (int)category, (int)variant,
                shared_from_this());
        } else {
            status = callMgr_->makeECall(phoneId, msdPdu, (int)category, (int)variant,
                 std::bind(&TelClient::makeCallResponse, this, std::placeholders::_1,
                 std::placeholders::_2));
        }
    } else {
        status = callMgr_->makeECall(phoneId, (int)category, (int)variant,
            std::bind(
                &TelClient::makeCallResponse, this, std::placeholders::_1, std::placeholders::_2));
    }
    if (status == telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Request to make an ECall is sent successfully" << std::endl;

        ECallInfo ecallInfo = {};
        ecallInfo.transmitMsd = transmitMsd;
        ecallInfo.msdData = msdData;
        ecallInfo.msdPdu = msdPdu;
        ecallInfo.isCustomNumber = false;
        ecallInfo.category = category;
        ecallInfo.variant = variant;
        ecallInfo.triggerHighCapSwitch = false;
        ecallInfo.msdTransmissionStatus = telux::tel::ECallMsdTransmissionStatus::FAILURE;
        ecallInfo.eCallNWScanFailed = false;
        eCallDataMap_[phoneId] = ecallInfo;
        if(isEraGlonassEnabled()) {
            isDialDurationTimeOut_ = false;
            stopDialTimer_ = false;
            willECallRedial_ = false;
            disconnectECallInNextAttempt_ = false;
            isT9TimerActive_ = false;
            isAutoAnswerDurationTimeOut_ = false;
        }
    } else {
        std::cout << CLIENT_NAME << "Request to make an ECall failed!" << std::endl;
        setECallProgressState(false);
        return telux::common::Status::FAILED;
    }
    callListener_ = callListener;
    if(isEraGlonassEnabled()) {
        autoDialDurationTimer_ = std::async(std::launch::async, [this, dialDuration, phoneId] {
            this->signalForExpiryOfDialDuration(dialDuration);
            this->autoHangup(phoneId);
        }).share();
    }
    return telux::common::Status::SUCCESS;
}

void TelClient::signalForExpiryOfDialDuration(int dialDuration) {
    isDialDurationTimeOut_ = false;
    std::cout << " signalForExpiryOfDialDuration: " << dialDuration << std::endl;
    std::unique_lock<std::mutex> lock(dialDurationMtx_);
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::minutes(dialDuration);
    auto predicate = [&] {
        if(stopDialTimer_) {
            std::cout << "Timer is stopped" << std::endl;
            return true;
        }
        if(std::chrono::steady_clock::now() - start >= timeout) {
            isDialDurationTimeOut_ = true;
            std::cout << "Timeout, exiting redialing send request" << std::endl;
            return true;
        }
        return false;
    };
    while (!dialDurationCv_.wait_until(lock, start + timeout, predicate)) {
        // Loop until the predicate returns true or timeout occurs
    }
}

void TelClient::autoHangup(int phoneId) {
    std::cout << "autoHangup" << std::endl;
    if( stopDialTimer_) {
        std::cout << " Exiting as timer is already stopped " << std::endl;
    } else if(isDialDurationTimeOut_) {
        std::cout << " Sending auto hangup request " << std::endl;
        if(eCall_) {
            hangup(phoneId, eCall_->getCallIndex());
            clearECall_ = true;
        }
        isDialDurationTimeOut_ = false;
    } else {
        // In situation when both dial duration timeout and stop dial timer are false
        // auto hangup request will not be sent.
        std::cout << " ERROR: Both stop dial timer and dial duration not expired " << std::endl;
    }
}

// Initiate a voice eCall procedure to the specified phone number
telux::common::Status TelClient::startECall(int phoneId, std::vector<uint8_t> msdPdu,
    ECallMsdData msdData, ECallCategory category, const std::string dialNumber, bool transmitMsd,
    std::shared_ptr<CallStatusListener> callListener) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to initiate an eCall"
                  << std::endl;
        return telux::common::Status::FAILED;
    }
    setECallProgressState(true);
    isPrivateEcallTriggered = false;
    // Initiate voice eCall
    telux::common::Status status = telux::common::Status::FAILED;
    if (transmitMsd) {
        if(msdPdu.empty()) {
            status = callMgr_->makeECall(phoneId, dialNumber, msdData, (int)category,
                shared_from_this());
        } else {
            status = callMgr_->makeECall(phoneId, dialNumber, msdPdu, (int)category,
                 std::bind(&TelClient::makeCallResponse, this, std::placeholders::_1,
                 std::placeholders::_2));
        }
    } else {
        status = callMgr_->makeECall(phoneId, dialNumber, (int)category,
            std::bind(
                &TelClient::makeCallResponse, this, std::placeholders::_1, std::placeholders::_2));
    }
    if (status == telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Request to make a Voice ECall is sent successfully"
                  << std::endl;
        ECallInfo ecallInfo = {};
        ecallInfo.transmitMsd = transmitMsd;
        ecallInfo.msdData = msdData;
        ecallInfo.msdPdu = msdPdu;
        ecallInfo.isCustomNumber = true;
        ecallInfo.category = category;
        ecallInfo.dialNumber = dialNumber;
        ecallInfo.triggerHighCapSwitch = false;
        ecallInfo.msdTransmissionStatus = telux::tel::ECallMsdTransmissionStatus::FAILURE;
        ecallInfo.eCallNWScanFailed = false;
        eCallDataMap_[phoneId] = ecallInfo;
    } else {
        std::cout << CLIENT_NAME << "Request to make a Voice ECall failed!" << std::endl;
        setECallProgressState(false);
        return telux::common::Status::FAILED;
    }
    callListener_ = callListener;
    return telux::common::Status::SUCCESS;
}

// Initiate a self test eCall procedure to the specified phone number.
telux::common::Status TelClient::startECall(int phoneId, const std::vector<uint8_t> rawData,
    const std::string dialNumber, std::shared_ptr<CallStatusListener> callListener) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to initiate an eCall"
                  << std::endl;
        return telux::common::Status::FAILED;
    }
    setECallProgressState(true);
    // Initiate voice eCall
    telux::common::Status status = telux::common::Status::FAILED;
    status = callMgr_->makeECall(phoneId, dialNumber, rawData,
        std::bind(
             &TelClient::makeCallResponse, this, std::placeholders::_1, std::placeholders::_2));
    if (status == telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Request to make a self test eCall is sent successfully"
                  << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Request to make a self test eCall failed!" << std::endl;
        setECallProgressState(false);
        return telux::common::Status::FAILED;
    }
    callListener_ = callListener;
    return telux::common::Status::SUCCESS;
}

// Initiate a voice eCall procedure to the specified phone number over IMS
telux::common::Status TelClient::startECall(int phoneId, const std::vector<uint8_t> rawData,
    const std::string dialNumber, std::string contentType, std::string acceptInfo,
    std::shared_ptr<CallStatusListener> callListener) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to initiate an eCall"
                  << std::endl;
        return telux::common::Status::FAILED;
    }
    setECallProgressState(true);
    isPrivateEcallTriggered = true;
    // Initiate voice eCall
    telux::common::Status status = telux::common::Status::FAILED;
    CustomSipHeader header;
    if (contentType != "") {
        header.contentType = contentType;
    } else {
        header.contentType = telux::tel::CONTENT_HEADER;
    }
    if (acceptInfo != "") {
        header.acceptInfo = acceptInfo;
    } else  {
        header.acceptInfo = "";
    }
    status = callMgr_->makeECall(phoneId, dialNumber, rawData, header,
        std::bind(
             &TelClient::makeCallResponse, this, std::placeholders::_1, std::placeholders::_2));
    if (status == telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Request to make a Voice ECall over IMS is sent successfully"
                  << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Request to make a Voice ECall failed!" << std::endl;
        setECallProgressState(false);
        return telux::common::Status::FAILED;
    }
    callListener_ = callListener;
    return telux::common::Status::SUCCESS;
}

// Update the MSD data
telux::common::Status TelClient::updateECallMSD(int phoneId, ECallMsdData msdData) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to send MSD update request"
                  << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->updateECallMsd(phoneId, msdData, updateMsdCommandCallback_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send MSD update request!" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}
telux::common::Status TelClient::updateTpsEcallOverImsMSD(
    int phoneId, std::vector<uint8_t> msdPdurawData) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to send MSD update request"
                  << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->updateECallMsd(phoneId, msdPdurawData, &TelClient::updateEcallResponse);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send MSD update request!" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Answer an incoming call
telux::common::Status TelClient::answer(
    int phoneId, std::shared_ptr<CallStatusListener> callListener) {
    std::cout << CLIENT_NAME << "Answer " << std::endl;
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed answer call" << std::endl;
        return telux::common::Status::FAILED;
    }
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICall>> callList = callMgr_->getInProgressCalls();
    // Fetch the list of in progress calls from CallManager and accept the incoming call.
    for (auto callIterator = std::begin(callList); callIterator != std::end(callList);
         ++callIterator) {
        if ((((*callIterator)->getCallState() == telux::tel::CallState::CALL_INCOMING)
                || ((*callIterator)->getCallState() == telux::tel::CallState::CALL_WAITING))
            && (phoneId == (*callIterator)->getPhoneId())) {
            spCall = *callIterator;
            break;
        }
    }
    if (spCall) {
        eCall_ = spCall;
        std::cout << CLIENT_NAME << "Found a valid call " << std::endl;
        // Answer incoming PSAP callback
        isIncomingCallInProgress_ = true;
        setECallProgressState(true);
        telux::common::Status status = spCall->answer(answerCommandCallback_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << "Failed to accept call " << std::endl;
            isIncomingCallInProgress_ = false;
            setECallProgressState(false);
            eCall_ = nullptr;
            return telux::common::Status::FAILED;
        }
    } else {
        std::cout << CLIENT_NAME << "No incoming call found to accept " << std::endl;
        return telux::common::Status::FAILED;
    }
    callListener_ = callListener;
    return telux::common::Status::SUCCESS;
}

// Hangup an ongoing call
telux::common::Status TelClient::hangup(int phoneId, int callIndex) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to hangup call" << std::endl;
        return telux::common::Status::FAILED;
    }
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    // If callIndex is not provided, iterate through the call list in the application and hangup if
    // only one call is Active or on Hold. If callIndex is provided, hangup the corresponding call.
    std::vector<std::shared_ptr<telux::tel::ICall>> callList = callMgr_->getInProgressCalls();
    int numOfCalls = 0;
    for (auto callIterator = std::begin(callList); callIterator != std::end(callList);
         ++callIterator) {
        if (phoneId == (*callIterator)->getPhoneId()) {
            telux::tel::CallState callState = (*callIterator)->getCallState();
            if ((callState != telux::tel::CallState::CALL_ENDED)
                && ((callIndex == -1) || (callIndex == (*callIterator)->getCallIndex()))) {
                spCall = *callIterator;
                numOfCalls++;
                break;
            }
        }
    }
    if (spCall && (numOfCalls == 1)) {
        std::cout << CLIENT_NAME << "Sending hangup " << std::endl;
        telux::common::Status status = spCall->hangup(hangupCommandCallback_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << "Failed to hangup call " << std::endl;
            return telux::common::Status::FAILED;
        }
    } else {
        if (isEraGlonassEnabled()) {
            if (willECallRedial_) {
                std::cout << CLIENT_NAME <<
                    " ERA-GLONASS eCall redial is perfomed by modem" <<
                    "so call will get disconnected before next redial."
                    << std::endl;
                disconnectECallInNextAttempt_ = true;
            } else {
                disconnectECallInNextAttempt_ = false;
                std::cout << CLIENT_NAME << "No relevant call found to hangup" << std::endl;
            }
        }
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Dump the list of current calls
telux::common::Status TelClient::getCurrentCalls() {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to get current calls"
                  << std::endl;
        return telux::common::Status::FAILED;
    }
    std::vector<std::shared_ptr<telux::tel::ICall>> calls = callMgr_->getInProgressCalls();
    for (auto callIterator = std::begin(calls); callIterator != std::end(calls); ++callIterator) {
        std::cout << " Call Index: " << static_cast<int>((*callIterator)->getCallIndex())
                  << ", Phone ID: " << static_cast<int>((*callIterator)->getPhoneId())
                  << ", Call State: "
                  << TelClientUtils::callStateToString((*callIterator)->getCallState())
                  << ", Call Direction: "
                  << TelClientUtils::callDirectionToString((*callIterator)->getCallDirection())
                  << ", Phone Number: " << (*callIterator)->getRemotePartyNumber() << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

// Get eCall HLAP timers status
telux::common::Status TelClient::requestECallHlapTimerStatus(int phoneId) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to request for HLAP timers status"
                  << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->requestECallHlapTimerStatus(
        phoneId, std::bind(&TelClient::hlapTimerStatusResponse, this, std::placeholders::_1,
                     std::placeholders::_2, std::placeholders::_3));
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request for HLAP timers status" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// std::function callback for CallManager::updateECallMsd
void TelClient::updateEcallResponse(telux::common::ErrorCode error) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "updateECallMsd Request failed with errorCode: " << static_cast<int>(error);
        std::cout << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

// Stop T10 eCall High Level Application Protocol(HLAP) timer
telux::common::Status TelClient::stopT10Timer(int phoneId) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to send request to stop T10 timer"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->requestNetworkDeregistration(phoneId,
                            std::bind(&TelClient::stopT10TimerResponse, this,
                            std::placeholders::_1));
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request to stop T10 timer" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Set the value of eCall High Level Application Protocol(HLAP) timer
telux::common::Status TelClient::setHlapTimer(int phoneId, HlapTimerType type,
    uint32_t timeDuration) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to send request to set HLAP timer"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->updateEcallHlapTimer(phoneId, type, timeDuration,
                            std::bind(&TelClient::setHlapTimerResponse, this,
                            std::placeholders::_1));
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request to set HLAP timer" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Set the value of POST TEST REGISTRATION timer.
telux::common::Status TelClient::setPostTestRegistrationTimer(int phoneId, uint32_t timeDuration) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME <<
            "Invalid Call Manager, Failed to send request to set post test registration timer"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->updateECallPostTestRegistrationTimer(phoneId, timeDuration,
                            std::bind(&TelClient::setECallPostTestRegistrationTimerResponse, this,
                            std::placeholders::_1));
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME <<
            "Failed to send request to set post test registration timer" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Get the value of POST TEST REGISTRATION timer.
telux::common::ErrorCode TelClient::getECallPostTestRegistrationTimer(int phoneId) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Ecall Manager, Failed to get Ecall configuration"
            << std::endl;
        return telux::common::ErrorCode::INVALID_STATE;
    }
    uint32_t timer = 0;
    auto errorCode = callMgr_->getECallPostTestRegistrationTimer(phoneId, timer);
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << " ECall post test registration timer: " << timer << std::endl;
    } else {
        std::cout << CLIENT_NAME
            << "Failed to get eCall post test registration timer with errorCode "
            << static_cast<int>(errorCode) << std::endl;
    }
    return errorCode;
}

telux::common::Status TelClient::getECallConfig() {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Ecall Manager, Failed to get Ecall configuration"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    telux::tel::EcallConfig config = {};
    auto status = callMgr_->getECallConfig(config);
    if(status == telux::common::Status::SUCCESS) {
        TelClientUtils::printEcallConfig(config);
    } else {
        std::cout << CLIENT_NAME << "Failed to get eCall configuration" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Get eCall redial parameters for call origination failure and call drop.
telux::common::ErrorCode TelClient::getECallRedialConfig() {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Ecall Manager" << std::endl;
        return telux::common::ErrorCode::INVALID_STATE;
    }
    std::vector<int> callOrigTimeGap = {};
    std::vector<int> callDropTimeGap = {};
    auto errorCode = callMgr_->getECallRedialConfig(callOrigTimeGap, callDropTimeGap);
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Call origination failure redial config values ";
        for (int value : callOrigTimeGap) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
        std::cout << " Call drop failure redial config values ";
        for (int value : callDropTimeGap) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Failed to get eCall redial configuration"
            << static_cast<int>(errorCode) << std::endl;
    }
    return errorCode;
}

// Get the value of eCall High Level Application Protocol(HLAP) timer
telux::common::Status TelClient::getHlapTimer(int phoneId, HlapTimerType type) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to send request to get HLAP timer"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->requestEcallHlapTimer(phoneId, type,
                            std::bind(&TelClient::getHlapTimerResponse, this,
                            std::placeholders::_1, std::placeholders::_2));
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request to get HLAP timer" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status TelClient::setECallConfig(EcallConfig config) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Ecall Manager, Failed to set Ecall configuration"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->setECallConfig(config);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to set eCall configuration" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status TelClient::restartECallHlapTimer(int phoneId, EcallHlapTimerId id,
    int duration) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Ecall Manager, Failed to restart eCall HLAP timer"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->restartECallHlapTimer(phoneId, id, duration,
        std::bind(&TelClient::restartHlapTimerResponse, this, std::placeholders::_1));
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to restart eCall HLAP timer" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status TelClient::getEncodedOptionalAdditionalDataContent(
    ECallOptionalEuroNcapData optionalEuroNcapData, std::vector<uint8_t> &data) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid ECall Manager, Failed to get encoded optional"
            << " additional data content" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->encodeEuroNcapOptionalAdditionalData(optionalEuroNcapData, data);
    std::vector<uint8_t> optionalAdditionalDataContent = data;
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get encoded optional additional data content"
            << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::string encodedString(optionalAdditionalDataContent.begin(),
            optionalAdditionalDataContent.end());
        TelClientUtils::printEncodedOptionalAdditionalDataContent(encodedString);
    }
    return telux::common::Status::SUCCESS;
}

telux::common::ErrorCode TelClient::getECallMsdPayload(ECallMsdData eCallMsd,
    std::vector<uint8_t> &msdPdu) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to get encoded eCall"
            << " MSD payload" << std::endl;
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }
    auto errCode = callMgr_->encodeECallMsd(eCallMsd, msdPdu);
    std::vector<uint8_t> msdPayload = msdPdu;
    if (errCode != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get encoded eCall MSD payload" << std::endl;
        return telux::common::ErrorCode::GENERIC_FAILURE;
    } else {
        std::stringstream ss;
        for (auto i : msdPdu) {
            ss << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << (int)i;
        }
        TelClientUtils::printECallMsdPayload(ss.str());
    }
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::Status TelClient::configureECallRedial(int config, std::vector<int> &timeGap) {
    if (!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager,  Failed to configure eCall redial"
            << " configuration " << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->configureECallRedial(static_cast<RedialConfigType>(config), timeGap,
        std::bind(&TelClient::configureECallRedialResponse, this, std::placeholders::_1));
    if (status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to configure eCall redial configuration" << std::endl;
        return status;
    }
    return telux::common::Status::SUCCESS;
}
