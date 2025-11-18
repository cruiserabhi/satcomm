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

#include "EcallStateMachine.hpp"
#include <telux/tel/ECallDefines.hpp>
#include <thread>
namespace telux {
namespace tel {

Idle::Idle(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("CallConnect",
    EcallStateMachine::StateID::STATE_CALL_CONNECT, parent) {
    changeState(std::make_shared<CallConnect>(parent_));
}

bool Idle::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    return true;
}

CallConnect::CallConnect(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("CallConnect",
       EcallStateMachine::StateID::STATE_CALL_CONNECT, parent) {
}

bool CallConnect::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
        || event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
        "CALL_ENDED", ecallStateMachine->getCallIndex());
        (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
        changeState(std::make_shared<PSAPCallback>(parent_));
    }
    return true;
}

void CallConnect::onEnter() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    std::string config = ecallStateMachine->getECallRedialConfig();
    (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
        "CALL_DIALING", ecallStateMachine->getCallIndex());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    if(config == "CALLORIG") {
        changeState(std::make_shared<PSAPCallback>(parent_));
    } else {
        (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
            "CALL_ALERTING", ecallStateMachine->getCallIndex());
        if((config == "SUCCESS") || (config == "CALLDROP")) {
                (ecallStateMachine->getCallservice())->onECallRedial(
                    ecallStateMachine->getPhoneId(), false, telux::tel::ReasonType::CALL_CONNECTED);
        } else {
            LOG(ERROR, __FUNCTION__, " Invalid config");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        (ecallStateMachine->getCallservice())->startTimer("T2Timer");
        changeState(std::make_shared<DecodeSendMSD>(parent_));
    }
}

void CallConnect::onExit() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    std::string config = ecallStateMachine->getECallRedialConfig();
    if(!(ecallStateMachine->isNGeCall())) {
        if(ecallStateMachine->isMsdTransmitted() == true) {
            if(!(ecallStateMachine->parseVectortoString("T5FAILED")) && (config != "CALLDROP")) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->sendEvent("T5Timer", "start");
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->startTimer("T5Timer");
            }
        }
    }
}

ModemRedial::ModemRedial(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("ModemRedial",
       EcallStateMachine::StateID::STATE_MODEM_REDIAL, parent) {
}

// ECALL REDIAL FOR CALL ORIGINATION FAILURE

// Call STATE: OUTGOING
// Send T10 START event ECALL OPERATING MODE = ECALL_ONLY
// Send onEcall redial notification (ECALL WILL REDIAL DUE TO CALL ORIGINATION FAILURE)
// Send T10 STOP event ECALL OPERATING MODE = ECALL_ONLY
// Call STATE: CALL ENDED

// check if count == max value as per JSON config
// Send onEcall redial notification (ECALL WILL NOT REDIAL DUE TO MAXIMUM ATTEMPTS EXHAUSTED)
// START T10 timer event ECALL OPERATING MODE = ECALL_ONLY
// Call STATE: CALL ENDED

// ECALL REDIAL FOR CALL DROP

// Call STATE: OUTGOING
// Send onEcall redial notification (ECALL WILL REDIAL DUE TO CALL DROP FAILURE)
// Call STATE: CALL ENDED

// check if count == max value as per JSON config
// Send onEcall redial notification (ECALL WILL NOT REDIAL DUE TO MAXIMUM ATTEMPTS EXHAUSTED)
// Call STATE: CALL ENDED


void ModemRedial::onEnter() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    std::string config = ecallStateMachine->getECallRedialConfig();
    telux::tel::ECallMode mode =
        ecallStateMachine->getEcallOperatingMode(ecallStateMachine->getPhoneId());
    int totalRedialAttempts = ecallStateMachine->getConfiguredRedialAttempts(config);
    std::vector<int> timeGap = ecallStateMachine->getConfiguredRedialParameters(config);
    int dialAttempts = 1;
    for (dialAttempts = 1 ; dialAttempts <= totalRedialAttempts; dialAttempts++) {
        // timeGap between successive redial
        std::this_thread::sleep_for(std::chrono::milliseconds(timeGap[dialAttempts - 1]));
        if(dialAttempts < totalRedialAttempts) {
            if((mode == telux::tel::ECallMode::ECALL_ONLY) && (config == "CALLORIG")) {
                (ecallStateMachine->getCallservice())->sendEvent("T10Timer", "start");
            }
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
                "CALL_DIALING", ecallStateMachine->getCallIndex());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if(config == "CALLORIG") {
                (ecallStateMachine->getCallservice())->onECallRedial(
                    ecallStateMachine->getPhoneId(), true,
                    telux::tel::ReasonType::CALL_ORIG_FAILURE);
                if(mode == telux::tel::ECallMode::ECALL_ONLY) {
                    (ecallStateMachine->getCallservice())->sendEvent("T10Timer", "stop");
                }
            } else {
                (ecallStateMachine->getCallservice())->onECallRedial(
                    ecallStateMachine->getPhoneId(), true, telux::tel::ReasonType::CALL_DROP);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
                "CALL_ENDED", ecallStateMachine->getCallIndex());
        }
        if(dialAttempts == totalRedialAttempts) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
                "CALL_DIALING", ecallStateMachine->getCallIndex());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->onECallRedial(ecallStateMachine->getPhoneId(),
                false, telux::tel::ReasonType::MAX_REDIAL_ATTEMPTED);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
                "CALL_ENDED", ecallStateMachine->getCallIndex());
            if(mode == telux::tel::ECallMode::ECALL_ONLY) {
                (ecallStateMachine->getCallservice())->startTimer("T10Timer");
            }
        }
    }
}

void ModemRedial::onExit() {
}

bool ModemRedial::onEvent(std::shared_ptr<telux::common::Event> event) {

    LOG(DEBUG, "Received event ", event->name_, " while in ", name_, "with event Id", event->id_);
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
        if(event->name_ == "T9Timer") {
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            (ecallStateMachine->getCallservice())->expiryTimer("T9Timer");
        }
        if(event->name_ == "T10Timer") {
            LOG(DEBUG, "Received event T10 expiry timer");
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            (ecallStateMachine->getCallservice())->expiryTimer("T10Timer");
        }
    }
    return true;
}

DecodeSendMSD::DecodeSendMSD(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("DecodeSendMSD",
       EcallStateMachine::StateID::STATE_DECODE_SEND_MSD, parent) {
    LOG(DEBUG, __FUNCTION__);
}

bool DecodeSendMSD::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
                    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    std::string config = ecallStateMachine->getECallRedialConfig();
    if((ecallStateMachine->parseVectortoString("T5FAILED")) || (config == "CALLDROP"))
    {
        if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
            if(event->name_ == "T5Timer") {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->expiryTimer("T5Timer");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->msdTransmissionStatus(
                    "MSD_TRANSMISSION_FAILURE");
                if(config == "CALLDROP") {
                    changeState(std::make_shared<PSAPCallback>(parent_));
                } else {
                    changeState(std::make_shared<CallConversation>(parent_));
                }
            }
        }
    }
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
        || event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
        "CALL_ENDED", ecallStateMachine->getCallIndex());
        (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
        changeState(std::make_shared<PSAPCallback>(parent_));
    }
    return true;
}

void DecodeSendMSD::onEnter() {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    std::string config = ecallStateMachine->getECallRedialConfig();
    if(!(ecallStateMachine->isNGeCall())) {     //CS eCall
        if(ecallStateMachine->isMsdTransmitted() == true) {
            if(ecallStateMachine->eventId_ ==
                static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
                std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
                (ecallStateMachine->getCallservice())->msdTransmissionStatus("START_RECEIVED");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->msdTransmissionStatus(
                    "MSD_TRANSMISSION_STARTED");
                changeState(std::make_shared<CRCCheckonMSD>(parent_));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->msdTransmissionStatus(
                    "MSD_TRANSMISSION_STARTED");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->changeCallState(
                    ecallStateMachine->getPhoneId(), "CALL_ACTIVE",
                    ecallStateMachine->getCallIndex());
                if((!(ecallStateMachine->parseVectortoString("T5FAILED")))
                    && (config == "SUCCESS")) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    (ecallStateMachine->getCallservice())->msdTransmissionStatus("START_RECEIVED");
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    (ecallStateMachine->getCallservice())->sendEvent("T5Timer", "stop");
                    changeState(std::make_shared<CRCCheckonMSD>(parent_));
                }
            }
        } else {  //NG ecall
            if((ecallStateMachine->isCustomNumberEcall()) != true) {
                if(ecallStateMachine->eventId_ ==
                    static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
                    std::shared_ptr<EcallStateMachine> ecallStateMachine
                    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    (ecallStateMachine->getCallservice())->msdTransmissionStatus(
                        "MSD_TRANSMISSION_STARTED");
                    changeState(std::make_shared<CRCCheckonMSD>(parent_));
                }
            }
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
            "CALL_ACTIVE", ecallStateMachine->getCallIndex());
            changeState(std::make_shared<CallConversation>(parent_));
        }
    } else { //NG eCall
        if(ecallStateMachine->isMsdTransmitted() == true) {
            if((ecallStateMachine->isCustomNumberEcall()) != true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->msdTransmissionStatus(
                    "OUTBAND_MSD_TRANSMISSION_STARTED");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
            "CALL_ACTIVE", ecallStateMachine->getCallIndex());
            if(config == "CALLDROP") {
                (ecallStateMachine->getCallservice())->msdTransmissionStatus(
                    "OUTBAND_MSD_TRANSMISSION_FAILURE");
                changeState(std::make_shared<PSAPCallback>(parent_));
            } else {
                changeState(std::make_shared<DecodeMSD>(parent_));
            }

        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
            "CALL_ACTIVE", ecallStateMachine->getCallIndex());
            changeState(std::make_shared<CallConversation>(parent_));
        }
    }
}

void DecodeSendMSD::onExit() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
                    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    std::string config = ecallStateMachine->getECallRedialConfig();
    if(ecallStateMachine->isMsdTransmitted() == true) { //CS ecall
        if(ecallStateMachine->parseVectortoString("T5FAILED") || (config == "CALLDROP")) {
            //Wait in DecodeSendMSD state till timer expiry event is not recieved
        }
    }
}

CRCCheckonMSD::CRCCheckonMSD(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("CRCCheckonMSD",
       EcallStateMachine::StateID::STATE_CRC_CHECK_ON_MSD, parent) {
}

bool CRCCheckonMSD::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
        if(event->name_ == "T7Timer") {
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            (ecallStateMachine->getCallservice())->msdTransmissionStatus
                ("LL_NACK_DUE_TO_T7_EXPIRY");
            (ecallStateMachine->getCallservice())->expiryTimer("T7Timer");
            (ecallStateMachine->getCallservice())->msdTransmissionStatus
                ("MSD_TRANSMISSION_FAILURE");
            changeState(std::make_shared<CallConversation>(parent_));
        }
    }
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
        || event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
        "CALL_ENDED", ecallStateMachine->getCallIndex());
        (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
        changeState(std::make_shared<PSAPCallback>(parent_));
    }
    return true;
}

void CRCCheckonMSD::onEnter() {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(!(ecallStateMachine->parseVectortoString("T7FAILED"))) {
        if(ecallStateMachine->eventId_ ==
            static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            (ecallStateMachine->getCallservice())->sendEvent("T7Timer", "start");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            changeState(std::make_shared<DecodeMSD>(parent_));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->sendEvent("T7Timer", "start");
            changeState(std::make_shared<DecodeMSD>(parent_));
        }
    } else {
        (ecallStateMachine->getCallservice())->startTimer("T7Timer");
    }
}

void CRCCheckonMSD::onExit() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(!(ecallStateMachine->parseVectortoString("T7FAILED"))) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        (ecallStateMachine->getCallservice())->sendEvent("T7Timer", "stop");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        (ecallStateMachine->getCallservice())->msdTransmissionStatus("LL_ACK_RECEIVED");
    }
}

DecodeMSD::DecodeMSD(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("DecodeMSD",
       EcallStateMachine::StateID::STATE_DECODE_MSD, parent) {
    LOG(DEBUG, __FUNCTION__);
}

bool DecodeMSD::onEvent(std::shared_ptr<telux::common::Event> event) {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
                    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if((ecallStateMachine->isCustomNumberEcall()) != true) {
        if(!(ecallStateMachine->isNGeCall())) {     //CS eCall
            LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
            if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
                if(event->name_ == "T6Timer") {
                    (ecallStateMachine->getCallservice())->expiryTimer("T6Timer");
                    (ecallStateMachine->getCallservice())->msdTransmissionStatus
                        ("MSD_TRANSMISSION_FAILURE");
                    changeState(std::make_shared<CallConversation>(parent_));
                }
            }
        }
    }
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
        || event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
            "CALL_ENDED", ecallStateMachine->getCallIndex());
        if(
            (((ecallStateMachine->isCustomNumberEcall()) == true) // Custom number ecall over CS
            && (!(ecallStateMachine->isNGeCall()))) ||
            (((ecallStateMachine->isCustomNumberEcall()) != true) // Regulatory NGecall
            && ((ecallStateMachine->isNGeCall()))) ||
            (((ecallStateMachine->isCustomNumberEcall()) != true) // Regulatory CSecall
            && (!(ecallStateMachine->isNGeCall())))
        ) {
            (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
        }
        changeState(std::make_shared<PSAPCallback>(parent_));
    }
    return true;
}

void DecodeMSD::onEnter() {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(!(ecallStateMachine->isNGeCall())) {     //CS eCall
        if(!(ecallStateMachine->parseVectortoString("T6FAILED"))) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->sendEvent("T6Timer", "start");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->msdTransmissionStatus
                ("MSD_TRANSMISSION_SUCCESS");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->sendEvent("T6Timer", "stop");
            if(ecallStateMachine->eventId_ ==
                static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
                ecallStateMachine->updateInProgress_ = false;
            } else {
                if(ecallStateMachine->getUserConfiguredALACKParameter() == true) {
                    (ecallStateMachine->getCallservice())->msdTransmissionStatus
                    ("MSD_AL_ACK_CLEARDOWN");
                }
            }
            changeState(std::make_shared<CallConversation>(parent_));
        } else {
            if((ecallStateMachine->isCustomNumberEcall()) != true) {
                (ecallStateMachine->getCallservice())->startTimer("T6Timer");
            }
        }
    } else { //NG eCall
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        (ecallStateMachine->getCallservice())->msdTransmissionStatus
            ("OUTBAND_MSD_TRANSMISSION_SUCCESS");
        if(ecallStateMachine->eventId_ ==
            static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
            ecallStateMachine->updateInProgress_ = false;
        } else {
            if(ecallStateMachine->getUserConfiguredALACKParameter() == true) {
                (ecallStateMachine->getCallservice())->msdTransmissionStatus
                ("MSD_AL_ACK_CLEARDOWN");
            }
        }
        changeState(std::make_shared<CallConversation>(parent_));
    }
}

void DecodeMSD::onExit() {
    LOG(DEBUG, __FUNCTION__);
}

PSAPCallback::PSAPCallback(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("PSAPCallback",
       EcallStateMachine::StateID::STATE_PSAP_CALLBACK, parent) {
}

bool PSAPCallback::onEvent(std::shared_ptr<telux::common::Event> event) {

    LOG(DEBUG, "Received event ", event->name_, " while in ", name_, "with event Id", event->id_);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
        if((event->name_ == "T9Timer") &&
            (ecallStateMachine->getEcallOperatingMode(ecallStateMachine->getPhoneId())
            != telux::tel::ECallMode::ECALL_ONLY)){
            (ecallStateMachine->getCallservice())->expiryTimer("T9Timer");
                ecallStateMachine->stop();
        }
        if(event->name_ == "T10Timer") {
            LOG(DEBUG, "Received event T10 expiry timer");
            (ecallStateMachine->getCallservice())->expiryTimer("T10Timer");
            if(ecallStateMachine->getEcallOperatingMode(ecallStateMachine->getPhoneId())
                == telux::tel::ECallMode::ECALL_ONLY) {
                ecallStateMachine->stop();
            }
        }
    }
    if(event->id_ == static_cast<int>(
        EcallStateMachine::EventID::ON_NETWORK_DEREGISTRATION_REQUEST)) {
        if(event->name_ == "T10Timer") {
            LOG(DEBUG, "Received event T10 stop timer");
            (ecallStateMachine->getCallservice())->sendEvent("T10Timer", "stop");
        }
    }
    return true;
}

telux::tel::ECallMode EcallStateMachine::getEcallOperatingMode(int phoneId) {
    JsonData data;
    std::string apiJsonPath = (phoneId == SLOT_ID_1) ? "api/tel/IPhoneManagerSlot1.json" :
                              "api/tel/IPhoneManagerSlot2.json";
    std::string stateJsonPath = (phoneId == SLOT_ID_1) ?
                               "system-state/tel/IPhoneManagerStateSlot1.json" :
                               "system-state/tel/IPhoneManagerStateSlot2.json";
    std::string method = "requestECallOperatingMode";
    std::string subsystem = "IPhoneManager";
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);
    int ecallMode = data.stateRootObj[subsystem]["eCallOperatingMode"]["ecallMode"].asInt();
    return static_cast<telux::tel::ECallMode>(ecallMode);
}

void PSAPCallback::onEnter() {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    (ecallStateMachine->getCallservice())->startTimer("T9Timer");
    std::string config = ecallStateMachine->getECallRedialConfig();
    if(config == "SUCCESS") {
        if(ecallStateMachine->getEcallOperatingMode(ecallStateMachine->getPhoneId())
            == telux::tel::ECallMode::ECALL_ONLY) {
            (ecallStateMachine->getCallservice())->startTimer("T10Timer");
        }
    }
    if(config == "CALLORIG") {
        // call on ecall redial notification based on call drop or call orig
        (ecallStateMachine->getCallservice())->onECallRedial(ecallStateMachine->getPhoneId(),
            true, telux::tel::ReasonType::CALL_ORIG_FAILURE);
    } else {
        // call drop
        (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
        if(config == "CALLDROP") {
            (ecallStateMachine->getCallservice())->onECallRedial(ecallStateMachine->getPhoneId(),
                true, telux::tel::ReasonType::CALL_DROP);
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
        "CALL_ENDED", ecallStateMachine->getCallIndex());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    if(config != "SUCCESS") {
        changeState(std::make_shared<ModemRedial>(parent_));
    }
}

void PSAPCallback::onExit() {
    LOG(DEBUG, __FUNCTION__);
}

CallConversation::CallConversation(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("CallConversation",
       EcallStateMachine::StateID::STATE_CALL_CONVERSATION, parent) {
}

bool CallConversation::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
        if(event->name_ == "T2Timer") {
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            (ecallStateMachine->getCallservice())->expiryTimer("T2Timer");
            (ecallStateMachine->getCallservice())->changeCallState
            (ecallStateMachine->getPhoneId(), "CALL_ENDED",
                ecallStateMachine->getCallIndex());
            changeState(std::make_shared<PSAPCallback>(parent_));
        }
    } else if (event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
        || event->id_ ==
        static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        changeState(std::make_shared<PSAPCallback>(parent_));
    } else if(event->id_ ==
        static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        ecallStateMachine->eventId_ =  event->id_;
        if(event->name_ == "CSeCall") {
            ecallStateMachine->updateInProgress_ = true;
            changeState(std::make_shared<DecodeSendMSD>(parent_));
        } else {
            ecallStateMachine->updateInProgress_ = true;
            if((ecallStateMachine->isCustomNumberEcall()) != true) {
                (ecallStateMachine->getCallservice())->msdTransmissionStatus
                    ("OUTBAND_MSD_TRANSMISSION_STARTED");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            (ecallStateMachine->getCallservice())->msdTransmissionStatus
                ("OUTBAND_MSD_TRANSMISSION_SUCCESS");
            ecallStateMachine->updateInProgress_ = false;
        }
    }
    return true;
}

void CallConversation::onEnter() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(ecallStateMachine->getUserConfiguredALACKParameter() == true) {
        changeState(std::make_shared<PSAPCallback>(parent_));
    }
}

void CallConversation::onExit() {
    LOG(DEBUG, __FUNCTION__);
}


std::shared_ptr<CallManagerServerImpl> EcallStateMachine::getCallservice() const {
    return callservice_.lock();
}

EcallStateMachine::EcallStateMachine(std::shared_ptr<CallManagerServerImpl> callservice,
    std::vector<std::string> result, bool isMsdTransmitted, bool isNGeCall,
    bool isALACKConfigEnabled, int phoneId, int callIndex, bool isCustomNumbereCall,
    std::string eCallRedialConfig, bool updateInProgress)
   : BaseStateMachine("CallSubSystemStateMachine")
   , callservice_(callservice)
   , result_(result)
   , isMsdTransmitted_(isMsdTransmitted)
   , isNGeCall_(isNGeCall)
   , isALACKConfigEnabled_(isALACKConfigEnabled)
   , phoneId_(phoneId)
   , callIndex_(callIndex)
   , isCustomNumbereCall_(isCustomNumbereCall)
   , eCallRedialConfig_(eCallRedialConfig)
   , updateInProgress_(updateInProgress) {
}

bool EcallStateMachine::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event: ", event->name_);
    currentState_->onEvent(event);
    return true;
}

bool EcallStateMachine::isMsdTransmitted() {
    return isMsdTransmitted_;
}

bool EcallStateMachine::isNGeCall() {
    return isNGeCall_;
}

int EcallStateMachine::getPhoneId() {
    return phoneId_;
}

bool EcallStateMachine::isEcallMSDUpdateInProgress() {
    return updateInProgress_;
}

int EcallStateMachine::getCallIndex() {
    return callIndex_;
}

bool EcallStateMachine::isCustomNumberEcall() {
    return isCustomNumbereCall_;
}

std::string EcallStateMachine::getECallRedialConfig() {
    return eCallRedialConfig_;
}

bool EcallStateMachine::getUserConfiguredALACKParameter() {
    return isALACKConfigEnabled_;
}

int EcallStateMachine::getConfiguredRedialAttempts(std::string config) {
    LOG(DEBUG, __FUNCTION__, " User redial config ", config);
    JsonData data;
    std::string apiJsonPath = "api/tel/ICallManagerSlot1.json";
    std::string stateJsonPath = "system-state/tel/ICallManagerStateSlot1.json";
    std::string method = "configureECallRedial";
    std::string subsystem = "ICallManager";
    int sizeOfTimeGap = -1;
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);
    if(config == "CALLORIG") {
        std::string input = data.stateRootObj[subsystem]["eCallRedialTimeGap"]\
            ["callOrigFailure"].asString();
        LOG(DEBUG, __FUNCTION__, " timeGap ", input);
        std::vector<int> timeGap = CommonUtils::convertStringToVector(input);
        sizeOfTimeGap = timeGap.size();
    } else if(config == "CALLDROP") {
        std::string input = data.stateRootObj[subsystem]["eCallRedialTimeGap"]\
            ["callDrop"].asString();
        std::vector<int> timeGap = CommonUtils::convertStringToVector(input);
        sizeOfTimeGap = timeGap.size();
    } else {
        LOG(ERROR, __FUNCTION__, " Invalid config");
    }
    LOG(DEBUG, __FUNCTION__, " TimeGap size ", sizeOfTimeGap);
    return sizeOfTimeGap;
}

std::vector<int> EcallStateMachine::getConfiguredRedialParameters(std::string config){
    LOG(DEBUG, __FUNCTION__, " User redial config", config);
    JsonData data;
    std::string apiJsonPath = "api/tel/ICallManagerSlot1.json";
    std::string stateJsonPath = "system-state/tel/ICallManagerStateSlot1.json";
    std::string method = "configureECallRedial";
    std::string subsystem = "ICallManager";
    std::vector<int> timeGap;
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);
    if(config == "CALLORIG") {
        std::string input = data.stateRootObj[subsystem]["eCallRedialTimeGap"]\
            ["callOrigFailure"].asString();
        timeGap = CommonUtils::convertStringToVector(input);
    } else if(config == "CALLDROP") {
        std::string input = data.stateRootObj[subsystem]["eCallRedialTimeGap"]\
            ["callDrop"].asString();
        timeGap = CommonUtils::convertStringToVector(input);
    } else {
       LOG(ERROR, __FUNCTION__, " Invalid config");
    }
    return timeGap;
}

void EcallStateMachine::start() {
    // Call the base class method to get it running
    LOG(DEBUG, __FUNCTION__);
    BaseStateMachine::start();
    // Move to CallConnect
    changeState(std::make_shared<CallConnect>(shared_from_this()));
}

void EcallStateMachine::stop() {
    BaseStateMachine::stop();
}

bool EcallStateMachine::parseVectortoString(std::string compareTimer) {
    int size = result_.size();
    int i = 0;
    while(i < size) {
        if (compareTimer == result_[i]) {
            return true;
        }
        i++;
    }
    return false;
}

std::shared_ptr<telux::common::Event> EcallStateMachine::createTelEvent(
    EventID id, std::string timer, int phoneId) {
    return std::make_shared<TelEvent>(id, timer, phoneId);
}

}  // namespace tel
}  // namespace telux
