/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "Event.hpp"

uint64_t Event::nextId;

/**
 * @brief Construct a new Event:: Event object
 *
 * @param event          Triggered tcu activity state
 * @param triggerType    Trigger type to identify who initiated it
 */
Event::Event(TcuActivityState triggeredState, std::string machineName,
             TriggerType triggerType) : id_(++nextId), triggeredState_(triggeredState),
                                        machineName_(machineName), triggerType_(triggerType),
                                        status_(EventStatus::INITIALIZED)
{
    LOG(DEBUG, __FUNCTION__, toString());
    timeStamps_.insert({EventStatus::INITIALIZED,
                        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())});
}

Event::~Event() {
    LOG(DEBUG, __FUNCTION__, toString());
}

// getter setter
uint64_t Event::getId() {
    return id_;
}

TcuActivityState Event::getTriggeredState() {
    return triggeredState_;
}

std::string Event::getMachineName() {
    return machineName_;
}

TriggerType Event::getTriggerType() {
    return triggerType_;
}

string Event::toString() {
    string eventString = "trigger id = " + to_string(id_) +
                         "  triggered by " + RefAppUtils::triggerTypeToString(triggerType_) +
                         "  trigger status = " + RefAppUtils::eventStatusToString(status_) +
                         "  machine Name = " + machineName_ +
                         "  TCU activity triggered state = " +
                         RefAppUtils::tcuActivityStateToString(triggeredState_);
    return eventString;
}

EventStatus Event::getEventStatus() {
    return status_;
}

void Event::setEventStatus(EventStatus status) {
    LOG(DEBUG, __FUNCTION__);
    status_ = status;
    timeStamps_.insert({status,
                        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())});
}