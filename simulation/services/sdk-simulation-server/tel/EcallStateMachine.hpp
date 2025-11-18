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

/**
 * @file       EcallStateMachine.hpp
 */

#ifndef ECALLSTATEMACHINE_HPP
#define ECALLSTATEMACHINE_HPP

#include <set>

#include "../../../libs/common/BaseState.hpp"
#include "../../../libs/common/BaseStateMachine.hpp"
#include "CallManagerServerImpl.hpp"
#include <telux/common/CommonDefines.hpp>
#include "../../../libs/common/Event.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

class CallManagerServerImpl;

namespace telux {
namespace tel {

class EcallStateMachine;
/**
 * TelEvent is the Event EcallStateMachine can recognize
 * and handle. This event can have IDs:
 *      HANGUP_REQUEST_FROM_USER
 *      HANGUP_REQUEST_FROM_PSAP
 *      MSD_PULL_REQUEST_FROM_PSAP
 *      PSAP_CALLBACK
 *      ON_TIMER_EXPIRY
 *      ON_NETWORK_DEREGISTERATION_REQUEST
 * The state, apart from an ID and name has a boolean to indicate
 * the state of sub-system
 */
class TelEvent : public telux::common::Event {
 public:
    TelEvent(uint32_t id, std::string name, int phoneId)
       : Event(id, name, phoneId) {
    }
};


/**
 * Concrete state (from BaseState) representing the ecall state when the
 * user triggers ecall
 */
class Idle : public telux::common::BaseState {
 public:
    /**
     * Constructor for Idle
     * @param [in] name - The parent state-machine of type EcallStateMachine
     */
    Idle(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for Idle
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event);
    /**
     * Method invoked by state-machine framework on entering Idle
     */
    // void onEnter() override;

    // /**
    //  * Method invoked by state-machine framework on exiting Idle
    //  */
    // void onExit() override;
};

/**
 * Concrete state (from BaseState) representing the system when the
 * ecall state is CallConnect.
 */
class CallConnect : public telux::common::BaseState {
 public:
    /**
     * Constructor for CallConnect
     * @param [in] name - The parent state-machine of type EcallStateMachine
     */
    CallConnect(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for CallConnect
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event);
    /**
     * Method invoked by state-machine framework on entering CallConnect
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting CallConnect
     */
    void onExit() override;
};

class ModemRedial : public telux::common::BaseState {
 public:
    /**
     * Constructor for ModemRedial
     * @param [in] name - The parent state-machine of type EcallStateMachine
     */
    ModemRedial(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for ModemRedial
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */

    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering ModemRedial
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting ModemRedial
     */
    void onExit() override;
};

class DecodeSendMSD : public telux::common::BaseState {
 public:
    /**
     * Constructor for DecodeSendMSD
     * @param [in] name - The parent state-machine of type EcallStateMachine
     */
    DecodeSendMSD(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for DecodeSendMSD
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */

    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering DecodeSendMSD
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting DecodeSendMSD
     */
    void onExit() override;
};

class CallConversation : public telux::common::BaseState {
 public:
    /**
     * Constructor for CallConversation
     * @param [in] name - The parent state-machine of type EcallStateMachine
     */
    CallConversation(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for CallConversation
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */

    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering CallConversation
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting CallConversation
     */
    void onExit() override;
};

class CRCCheckonMSD : public telux::common::BaseState {
 public:
    /**
     * Constructor for CRCCheckonMSD
     * @param [in] name - The parent state-machine of type EcallStateMachine
     */
    CRCCheckonMSD(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for CRCCheckonMSD
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */

    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering CRCCheckonMSD
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting CRCCheckonMSD
     */
    void onExit() override;
};

class DecodeMSD : public telux::common::BaseState {
 public:
    /**
     * Constructor for DecodeMSD
     * @param [in] name - The parent state-machine of type EcallStateMachine
     */
    DecodeMSD(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for DecodeMSD
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */

    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering DecodeMSD
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting DecodeMSD
     */
    void onExit() override;
};

class PSAPCallback : public telux::common::BaseState {
 public:
    /**
     * Constructor for PSAPCallback
     * @param [in] name - The parent state-machine of type EcallStateMachine
     */
    PSAPCallback(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for PSAPCallback
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */

    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering PSAPCallback
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting PSAPCallback
     */
    void onExit() override;
};

/**
 * Concrete state-machine (from BaseStateMachine) representing the
 * eCall state-machine handling
 * Does basic (common) handling of the sub-system events (TelEvent)
 * and passes on the event to the current state
 */
class EcallStateMachine : public telux::common::BaseStateMachine,
                                  public std::enable_shared_from_this<EcallStateMachine> {
 private:

    const std::weak_ptr<CallManagerServerImpl> callservice_;
    std::vector<std::string> result_;
    bool isMsdTransmitted_;
    bool isNGeCall_;
    bool isALACKConfigEnabled_;
    int phoneId_;
    int callIndex_;
    bool isCustomNumbereCall_;
    std::string eCallRedialConfig_;
 public:
    /**
     * Constructor for EcallStateMachine
     * @param [in] name - The service, should be type CallManagerServerImpl
     */
    EcallStateMachine(std::shared_ptr<CallManagerServerImpl> callservice,
        std::vector<std::string>, bool isMsdTransmitted, bool isNGeCall,
        bool isALACKConfigEnabled, int phoneId, int callIndex, bool isCustomNumberEcall,
        std::string eCallRedialConfig, bool updateInProgress);

    /**
     * Overridden start method, would move the state machine to CallIdle
     */
    void start() override;

    /**
     * Overridden stop method, clears all internal states and variables
     */
    void stop() override;

    /**
     * Method to acquire the service in underlying states
     * @returns the pointer to the CallManagerServerImpl service
     */
    std::shared_ptr<CallManagerServerImpl> getCallservice() const;

    /**
     * Top-level event handler for the state-machine
     * Handles the incoming events, identifies the event and passes on further
     * to the current state for further handling
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event) override;

    enum EventID {
        NONE = EVENT_ID_INVALID,
        HANGUP_REQUEST_FROM_USER,
        HANGUP_REQUEST_FROM_PSAP,
        MSD_PULL_REQUEST_FROM_PSAP,
        ON_TIMER_EXPIRY,
        ON_NETWORK_DEREGISTRATION_REQUEST
    };

    uint32_t eventId_ = telux::tel::EcallStateMachine::EventID::NONE;

    /**
     * Utility method to create TelEvent
     * @param [in] id - The event ID identifying the sub-system
     * @param [in] timer - Provides the details for timer name for a timer expiry event.
     * @param [in] phoneId - PhoneId of the event
     * @returns the TelEvent created from id and state
     */
    std::shared_ptr<telux::common::Event> createTelEvent(EventID id, std::string timer,
        int phoneId);

    /**
     * Utility method to fetch user input for statemachine
     *
     */
    bool parseVectortoString(std::string compareTimer);

    /**
     * To check if MSD is transmitted on eCall
     *
     */

    bool isMsdTransmitted();

    bool isNGeCall();

    bool isEcallMSDUpdateInProgress();

    bool isCustomNumberEcall();

    bool getUserConfiguredALACKParameter();

    int getPhoneId();

    int getCallIndex();

    std::string getECallRedialConfig();

    int getConfiguredRedialAttempts(std::string config);

    std::vector<int> getConfiguredRedialParameters(std::string config);

    telux::tel::ECallMode getEcallOperatingMode(int phoneId);

    bool updateInProgress_;

    enum StateID {
        STATE_IDLE = STATE_ID_INVALID,
        STATE_MODEM_REDIAL,
        STATE_CALL_CONNECT,
        STATE_DECODE_SEND_MSD,
        STATE_CRC_CHECK_ON_MSD,
        STATE_DECODE_MSD,
        STATE_CALL_CONVERSATION,
        STATE_PSAP_CALLBACK
    };
};

}  // namespace tel
}  // namespace telux

#endif  // ECALLSTATEMACHINE_HPP
