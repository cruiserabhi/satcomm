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

#ifndef BASESTATE_HPP
#define BASESTATE_HPP

#include <memory>

#include "BaseStateMachine.hpp"

class Event;

namespace telux {

namespace common {

class BaseState : public BaseStateMachine {
 public:
    // The name of the state
    const std::string name_;
    // Unique ID of the state within the state machine
    const uint32_t id_;

    /**
     * Constructor to create a state
     *
     * @param [in] name   - Name of the state - used for logging
     * @param [in] id     - Unique ID of the state within the statemachine
     *                      Used for checking during state transitions.
     *                      Zero (0 = STATE_ID_INVALID) is reserved.
     * @param [in] parent - The parent state machine (or state) that should
     *                      be used to request state transitions
     */
    BaseState(std::string name, uint32_t id, std::weak_ptr<BaseStateMachine> parent);

    virtual ~BaseState();

    /**
     * Method to get the current state the statemachine is in. The statemachine
     * is traversed hierarchically to fetch the state we are in.
     *
     * @returns The id of the state the statemachine is in
     */
    virtual uint32_t getCurrentState() override;

    /**
     * This method should be overridden in the state to receive an event.
     * In case of a composite state, the base statemachine's onEvent can
     * be invoked to handle the event in the sub-states. The order of
     * invocation should be carefully considered. If the base state's
     * onEvent is invoked, the event handling would be in the deepest
     * state.
     *
     * @param [in] event - The event to be handled @ref telux::commom::Event
     *
     * @returns true if the event was handled by the state or the underlying
     *          statemachines in the hierarchy
     */
    virtual bool onEvent(std::shared_ptr<Event> event) = 0;

 protected:
    /**
     * Helper method for requesting state transition to the current
     * state machine. This request is passed on to the parent statemachine
     * for further handling.
     * @param [in] state  - The new state to be entered
     */
    virtual void changeState(std::shared_ptr<BaseState> state) override;

    /**
     * Helper method for requesting state transition to a sub-state. A call
     * to this method increases the depth of the statemachine by 1.
     *
     * @param [in] state  - The new sub-state to be entered
     */
    virtual void changeSubState(std::shared_ptr<BaseState> state);

    /**
     * Method with generic logging up on entering the state. This method
     * should be overridden by the implementation state for specific actions
     * up on entering the state
     */
    virtual void onEnter();

    /**
     * Method with generic logging just before exiting the state. This method
     * should be overridden by the implementation state for specific actions
     * to be taken care up exiting the state
     */
    virtual void onExit();

    // The parent statemachine that would be used to request
    // state transitions
    const std::weak_ptr<BaseStateMachine> parent_;

 private:
    friend class BaseStateMachine;
};

}  // namespace common
}  // namespace telux

#endif  // BASESTATE_HPP
