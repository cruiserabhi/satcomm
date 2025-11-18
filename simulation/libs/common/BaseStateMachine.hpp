/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * Utility class that provides a framework to create statemachines.
 * As a framework, it comes with bare minimal functionality allowing
 * flexibility to the users.
 */

#ifndef BASESTATEMACHINE_HPP
#define BASESTATEMACHINE_HPP

#include <memory>
#include <mutex>
#include <string>
#include <sstream>

#define STATE_ID_INVALID 0
#define EVENT_ID_INVALID 0

namespace telux {

namespace common {

class BaseState;
class Event;

class BaseStateMachine {
 public:
    // Name of the statemachine for logging purpose
    const std::string name_;

    /**
     * Constructor for the base statemachine
     * @param [in] name - Name of the state machine, used for logging purpose
     */
    BaseStateMachine(std::string name);

    virtual ~BaseStateMachine();

    /**
     * Method to get the current state the statemachine is in. the statemachine
     * is traversed hierarchically to fetch the state we are in.
     *
     * @returns The id of the state the statemachine is in
     */
    virtual uint32_t getCurrentState();

    /**
     * Method used for generic handling of events - invokes the current
     * state's onEvent. Can be overridden for blocking events or additional
     * logging
     *
     * @param [in] event - The event to be handled @ref telux::commom::Event
     *
     * @returns true if the event was handled by the state or the underlying
     *          statemachines in the hierarchy
     */
    virtual bool onEvent(std::shared_ptr<Event> event);

    /**
     * Method used to start the statemachine activities like entering the
     * initial state, allowing event handling. This method should be called
     * to allow generic event handling by the framework.
     */
    virtual void start();

    /**
     * Method used to stop the statemachine activites - exits all active states
     * and disables generic event handling
     */
    virtual void stop();

    /**
     * Method to find if the state machine has been started (enabled)
     */
    bool isStarted() const;

    /**
     * A utility method to print the current schema of the statemachine
     * @param [in,out] ss - The stringstream to be populated for usage by
     *                      the caller
     */
    virtual void print(std::stringstream &ss);

    // Holds the current state this statemachine is in
    std::shared_ptr<BaseState> currentState_;

 protected:
    /**
     * Method used to request state transition. This method offers generic
     * state transition, i.e. exit the current state, enter the new state.
     * There are three possibilities for a state change
     * 1. With the current state being nullptr and state non-null to move
     * from an initial state to a valid, known state
     * 2. With the current state being non-null and state a nullptr to move
     * from a known, valid state to a final state to wind-up the state-machine
     * 3. With both current state and state being non-null to transit from
     * one valid state to another
     *
     * @param [in] state  - The new state to be entered, can be null to
     *                      move to a final state
     */
    virtual void changeState(std::shared_ptr<BaseState> state);

 private:
    friend class BaseState;
    // Flag indicating if the state machine was started
    bool started_;
};

}  // namespace common
}  // namespace telux

#endif  // BASESTATEMACHINE_HPP
