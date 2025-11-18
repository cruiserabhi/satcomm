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
#include "Event.hpp"
#include "Logger.hpp"

namespace telux {

namespace common {

BaseState::BaseState(std::string name, uint32_t id, std::weak_ptr<BaseStateMachine> parent)
   : BaseStateMachine(name)
   , name_(name)
   , id_(id)
   , parent_(parent) {
}

BaseState::~BaseState() {
}

uint32_t BaseState::getCurrentState() {
    if (currentState_) {
        return currentState_->getCurrentState();
    } else {
        return id_;
    }
}

bool BaseState::onEvent(std::shared_ptr<Event> event) {
    // If the state does not want to handle the event OR
    // if the state hasn't handled the event, it reaches here.
    // We forward it to the underlying statemachine to check
    // if the event would be handled there
    return BaseStateMachine::onEvent(event);
}

void BaseState::onEnter() {
    LOG(INFO, "[enter] ", name_);
}

void BaseState::onExit() {
    // If we are a composite state, we stop the statemachine,
    // eventually exiting all the underlying states
    BaseStateMachine::stop();
    LOG(INFO, "[exit] ", name_);
}

void BaseState::changeState(std::shared_ptr<BaseState> state) {
    // Request the parent to handle state transition request
    auto parent = parent_.lock();
    if (parent) {
        parent->BaseStateMachine::changeState(state);
    }
}

void BaseState::changeSubState(std::shared_ptr<BaseState> state) {
    // Enter the requested sub-state, in this case the parent
    // would be this state (statemachine)
    BaseStateMachine::changeState(state);
}

}  // namespace common
}  // namespace telux
