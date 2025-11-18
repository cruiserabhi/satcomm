/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       OperatingModeTransitionManager.hpp
 */

#ifndef TELUX_TEL_OPERATINGMODETRANSITIONMANAGER_HPP
#define TELUX_TEL_OPERATINGMODETRANSITIONMANAGER_HPP

#include <telux/common/CommonDefines.hpp>
#include "libs/common/BaseState.hpp"
#include "libs/common/BaseStateMachine.hpp"
#include "libs/common/Event.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "protos/proto-src/event_simulation.grpc.pb.h"
#include "event/EventService.hpp"

namespace telux {
namespace tel {

class OperatingModeTransitionManager;

/**
 * Concrete state (derived from BaseState) representing the Factory Test operating mode. Device can
 * transition into this mode from other modes except OFFLINE.
 */
class FactoryTestMode : public telux::common::BaseState {
 public:
    /**
     * Constructor for FactoryTestMode
     * @param [in] name - The parent state-machine of type OperatingModeStateMachine
     */
    FactoryTestMode(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for FactoryTestMode
     * @param [in] event - The Event that needs to be handled
     * @returns true if the event was handled and false for INVALID TRANSITION
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering FactoryTestMode
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting FactoryTestMode
     */
     void onExit() override;
};

/**
 * Concrete state (derived from BaseState) representing the Online operating mode. Device can
 * transition into this mode from other modes except OFFLINE.
 */
class OnlineMode : public telux::common::BaseState {
 public:
    /**
     * Constructor for OnlineMode
     * @param [in] name - The parent state-machine of type OperatingModeStateMachine
     */
    OnlineMode(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for OnlineMode
     * @param [in] event - The Event that needs to be handled
     * @returns true if the event was handled and false for INVALID TRANSITION
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event);
    /**
     * Method invoked by state-machine framework on entering OnlineMode
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting OnlineMode
     */
     void onExit() override;
};

/**
 * Concrete state (derived from BaseState) representing the Offline operating mode. Device can
 * transition into this mode from all other modes. And transition from offline to resetting/
 * shutdown is only allowed.
 */
class OfflineMode : public telux::common::BaseState {
 public:
    /**
     * Constructor for OfflineMode
     * @param [in] name - The parent state-machine of type OperatingModeStateMachine
     */
    OfflineMode(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for OfflineMode
     * @param [in] event - The Event that needs to be handled
     * @returns true if the event was handled and false for INVALID TRANSITION
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event);
    /**
     * Method invoked by state-machine framework on entering OfflineMode
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting OfflineMode
     */
     void onExit() override;
};

/**
 * Concrete state (derived from BaseState) representing the Persistent low power operating mode.
 * Device can transition into this mode from other modes except OFFLINE.
 */
class PersistentLowPowerMode : public telux::common::BaseState {
 public:
    /**
     * Constructor for PersistentLowPowerMode
     * @param [in] name - The parent state-machine of type OperatingModeStateMachine
     */
    PersistentLowPowerMode(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for PersistentLowPowerMode
     * @param [in] event - The Event that needs to be handled
     * @returns true if the event was handled and false for INVALID TRANSITION
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering PersistentLowPowerMode
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting PersistentLowPowerMode
     */
     void onExit() override;
};

/**
 * Concrete state (derived from BaseState) representing the Airplane operating mode. Device can
 * transition into this mode from other modes except OFFLINE.
 */
class AirplaneMode : public telux::common::BaseState {
 public:
    /**
     * Constructor for AirplaneMode
     * @param [in] name - The parent state-machine of type OperatingModeStateMachine
     */
    AirplaneMode(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for AirplaneMode
     * @param [in] event - The Event that needs to be handled
     * @returns true if the event was handled and false for INVALID TRANSITION
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering AirplaneMode
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting AirplaneMode
     */
     void onExit() override;
};

/**
 * Concrete state (derived from BaseState) representing the Resetting operating mode. Device can
 * transition into this mode from all other modes.
 */
class ResettingMode : public telux::common::BaseState {
 public:
    /**
     * Constructor for ResettingMode
     * @param [in] name - The parent state-machine of type OperatingModeStateMachine
     */
    ResettingMode(std::weak_ptr<BaseStateMachine> parent);

    /**
     * Event handler for ResettingMode
     * @param [in] event - The Event that needs to be handled
     * @returns true if the event was handled and false for INVALID TRANSITION
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event);

    /**
     * Method invoked by state-machine framework on entering ResettingMode
     */
    void onEnter() override;

    /**
     * Method invoked by state-machine framework on exiting ResettingMode
     */
     void onExit() override;
};

/**
 * Concrete state (derived from BaseState) representing the Shutdown operating mode. Device can
 * transition into this mode from all other modes.
 */
class ShutdownMode : public telux::common::BaseState
{
public:
   /**
    * Constructor for ShutdownMode
    * @param [in] name - The parent state-machine of type OperatingModeStateMachine
    */
   ShutdownMode(std::weak_ptr<BaseStateMachine> parent);

   /**
    * Event handler for ShutdownMode
    * @param [in] event - The Event that needs to be handled
    * @returns true if the event was handled and false for INVALID TRANSITION
    */
   bool onEvent(std::shared_ptr<telux::common::Event> event);

   /**
    * Method invoked by state-machine framework on entering ShutdownMode
    */
   void onEnter() override;

   /**
    * Method invoked by state-machine framework on exiting ShutdownMode
    */
   void onExit() override;
};

/**
 * Generic class to send notification/events sequentially when all state change
 * (like operating mode) happens.
 */
class Notification {

 public:
    /**
     *  Constructor
     */
    Notification();

    /**
     *  Destructor
     */
    ~Notification();

    /**
     *  Notify all the events from task queue sequentially and clears the queue.
     */
    void notify();

    /**
     * Add events to the task queue.
     */
    void addEvent(eventService::EventResponse eventResponse);

private:
    std::vector<eventService::EventResponse> events_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::mutex notificationMutex_;
    /**
     * Send event to event service to process further.
     */
    void triggerChangeEvent(eventService::EventResponse eventResponse);
};

/**
 * Base class to building the notifications.
 */
class NotificationBuilder {

 public:
    void init() {
        LOG(DEBUG, __FUNCTION__);
        reset();
    }

    void reset() {
        LOG(DEBUG, __FUNCTION__);
        notification_ = nullptr;
        notification_ = std::make_shared<telux::tel::Notification>();
    }

    virtual std::shared_ptr<Notification> build() = 0;

    virtual ~NotificationBuilder(){}

protected:
    std::shared_ptr<Notification> notification_;
};

/**
 * Telephony notification builder class to build notifications such as operating mode, voice service
 * state change etc. And add notification to task queue of Notification class.
 */
class TelephonyNotificationBuilder: public NotificationBuilder,
                              public std::enable_shared_from_this<TelephonyNotificationBuilder> {

 public:
    TelephonyNotificationBuilder();
    ~TelephonyNotificationBuilder();
    void addVoiceServiceStateChangeEvent(int phoneId, telStub::VoiceServiceStateEvent &event);
    void addOperatingModeChangeEvent(telStub::OperatingModeEvent &event);
    void addSignalStrengthChangeEvent(int phoneId, telStub::SignalStrengthChangeEvent &event);
    void addServiceStateChangeEvent(int phoneId, telStub::ServiceStateChangeEvent &event);
    void addVoiceRadioTechnologyChangeEvent(int phoneId,
        telStub::VoiceRadioTechnologyChangeEvent &event);
    std::shared_ptr<Notification> build();
 private:
    std::vector<eventService::EventResponse> events_;
    std::mutex notificationBuilderMutex_;
};

class PhoneEvent : public telux::common::Event {
 public:
    PhoneEvent(uint32_t id, std::string name, int phoneId)
       : Event(id, name, phoneId) {
    }

    void setOperatingMode(telStub::OperatingMode operatingMode) {
        operatingMode_ = operatingMode;
    }

    telStub::OperatingMode getOperatingMode() {
        return operatingMode_;
    }

 private:
    telStub::OperatingMode operatingMode_;
};

/**
 * Concrete state-machine (from BaseStateMachine) representing the operating mode state machine
 *
 */
class OperatingModeTransitionManager : public telux::common::BaseStateMachine,
                              public std::enable_shared_from_this<OperatingModeTransitionManager> {

 public:
    /**
     * Constructor for OperatingModeTransitionManager
     */
    OperatingModeTransitionManager();

    /**
     * Overridden start method
     */
    void start() override;

    /**
     * Overridden stop method
     */
    void stop() override;

    /**
     * Top-level event handler for the state-machine
     * Handles the incoming events, identifies the event and passes on further
     * to the current state for further handling
     * @param [in] event - The TelEvent that needs to be handled
     * @returns true if the event was handled
     */
    bool onEvent(std::shared_ptr<telux::common::Event> event) override;

    enum StateID {
        STATE_INVALID = -1,
        STATE_ONLINE,
        STATE_AIRPLANE,
        STATE_FACTORY_TEST,
        STATE_OFFLINE,
        STATE_RESETTING,
        STATE_SHUTDOWN,
        STATE_PERSISTENT_LOW_POWER,
    };

    enum EventID {
        NONE = EVENT_ID_INVALID,
        UPDATE_OPERATING_MODE
    };

    std::shared_ptr<telux::common::Event> createEvent(EventID eventId, std::string name,
        int phoneId) {
        return std::make_shared<PhoneEvent>(eventId, name, phoneId);
    }

    /**
     * Holds the previous state this statemachine is in.
     */
    std::shared_ptr<BaseState> prevState_;

    grpc::Status init();
    telux::common::ErrorCode getOperatingMode(telStub::OperatingMode &operatingMode);
    std::shared_ptr<BaseState> getPrevState();
    std::shared_ptr<TelephonyNotificationBuilder> getBuilder();
    telStub::SignalStrength& getCachedSS(int slotId);
    telStub::RadioTechnology& getCachedServingRat(int slotId);
    telux::common::ErrorCode updateOperatingMode(::telStub::OperatingMode opMode);
    telux::common::ErrorCode updateCachedSignalStrength(int slotId);
    telux::common::ErrorCode updateCachedServingRat(int slotId);
    void notifyAll(telStub::OperatingMode mode);
    void notifyOperatingMode(telStub::OperatingMode mode);

 private:
    std::vector<std::string> result_;
    std::shared_ptr<TelephonyNotificationBuilder> notificationBuilder_;
    std::map<int, telStub::SignalStrength> cachedSS_;
    std::map<int, telStub::RadioTechnology> cachedServingRat_;

    //Read only API JSON Data
    telux::common::ServiceStatus readSubsystemStatus(int slotId);
    telux::common::ServiceStatus readSubsystemStatus(int slotId, int &cbDelay);
    //Read both API and System State JSON Data
    telux::common::ErrorCode readJsonData(int slotId, std::string method, JsonData &data);
    telux::common::ErrorCode readJsonData(int slotId, std::string method,
        JsonData &data, std::string &stateJsonPath);
    telux::common::ErrorCode initOperatingMode();
    telux::common::ErrorCode initSignalStrength();
    telux::common::ErrorCode initServingRat();
    telux::common::ErrorCode initServiceState();

};

}  // namespace tel
}  // namespace telux

#endif  // TELUX_TEL_OPERATINGMODETRANSITIONMANAGER_HPP
