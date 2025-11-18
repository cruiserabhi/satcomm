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

#include "BaseState.hpp"
#include "BaseStateMachine.hpp"
#include "Logger.hpp"

namespace telux {

namespace common {

BaseStateMachine::BaseStateMachine(std::string name)
   : name_(name)
   , currentState_(nullptr)
   , started_(false) {
}

uint32_t BaseStateMachine::getCurrentState() {
    // We are not in any state
    if (currentState_ == nullptr) {
        return STATE_ID_INVALID;
    }

    // Ask the current state to get the ID of the underlying active state
    return currentState_->getCurrentState();
}

bool BaseStateMachine::onEvent(std::shared_ptr<Event> event) {
    if (!currentState_) {
        return false;
    } else {
        return currentState_->onEvent(event);
    }
}

void BaseStateMachine::changeState(std::shared_ptr<BaseState> state) {
    if (!started_) {
        LOG(WARNING, "[request] ", name_, ": rejected since state machine is not started");
        return;
    }
    LOG(INFO, "[request] ", name_, ": ", (currentState_ ? currentState_->name_ : "null"), " -> ",
        (state ? state->name_ : "null"));

    // Some basic checks to ensure an actual state transition is requested

    if (currentState_ == state) {
        return;
    }
    if (currentState_) {
        if (state) {
            if (currentState_->id_ == state->id_) {
                return;
            }
        }
        // Exit the current state
        currentState_->onExit();
    }
    currentState_ = state;

    // Enter the new current state
    if (currentState_) {
        currentState_->onEnter();
    }
}

void BaseStateMachine::start() {
    started_ = true;
}

void BaseStateMachine::stop() {
    // Exit the current state, this eventually is propagated
    // to the underlying states (composite and simple) to
    // ensure all states are exited
    LOG(DEBUG, __FUNCTION__);
    if (currentState_) {
        LOG(DEBUG, __FUNCTION__, "Current state is ",currentState_->name_);
        currentState_->onExit();
    }
    currentState_ = nullptr;
    started_ = false;
}

bool BaseStateMachine::isStarted() const {
    return started_;
}

void BaseStateMachine::print(std::stringstream &ss) {
    // Put out our name
    ss << name_;

    // Ask the underlying states/statemachines to populate the stream
    if (currentState_) {
        ss << " --> ";
        currentState_->print(ss);
    } else {
        ss << " --> null";
    }
}

BaseStateMachine::~BaseStateMachine() {
    // Stop the statemachine in case we are being deleted before
    // having been stopped
    stop();
}

}  // namespace common
}  // namespace telux
