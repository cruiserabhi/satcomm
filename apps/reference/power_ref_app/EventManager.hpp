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

#ifndef EVENTMANAGER_HPP
#define EVENTMANAGER_HPP

#include <memory>
#include <iostream>
#include <deque>
#include <vector>
#include <map>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <telux/power/TcuActivityDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityManager.hpp>
#include <telux/power/TcuActivityListener.hpp>
#include <telux/common/Log.hpp>

#include "common/define.hpp"
#include "Event.hpp"
#include "IEventListener.hpp"

using namespace telux::power;
using namespace telux::common;
using namespace std;

/**
 * @brief The EventManager class controls the execution sequence of events. The Event Manager
 * executes events with the help of TcuActivityManager (telsdk) and the controlling node dealing
 * with power state.
 *
 */

class EventManager : public ITcuActivityListener,
                     public IServiceStatusListener,
                     public enable_shared_from_this<EventManager>
{

   static EventManager *instance;

private:
   deque<shared_ptr<Event>> eventQueue_;
   mutex eventQueueUpdate_;

   shared_ptr<ITcuActivityManager> tcuActivityStateMgr_;
   map<TriggerType, vector<weak_ptr<IEventListener>>> eventListeners_;

   EventManager();
   void eventSchedule(shared_ptr<Event> event);
   bool registerTcuActivityManager();
   void printTcuActivityState(TcuActivityState state);
   void printQueue();

   void setActivityState(shared_ptr<Event> event);
   void notifyOnEventRejected(shared_ptr<Event> event, EventStatus status);
   void notifyAndEraseEventProcessed(TriggerType triggerType, TcuActivityState triggeredState,
                                     bool success, EventStatus status);

   // wake lock node control
   void writeToSystemNode(char *nodepath, char *value, int length);
   void holdWakeLock();
   void releaseWakeLock();

public:
   ~EventManager();
   static EventManager *getInstance();
   bool init();

   // interact with TcuActivityManager
   void onTcuActivityStateUpdate(TcuActivityState state, std::string machineName) override;
   void onSlaveAckStatusUpdate(const telux::common::Status status,
      const std::string machineName, const std::vector<ClientInfo> unresponsiveClients,
      const std::vector<ClientInfo> nackResponseClients) override;
   void onServiceStatusChange(telux::common::ServiceStatus status) override;

   // event management
   void pushEvent(shared_ptr<Event> event);
   // remove and notify 0th event and another event in queue triggered for the same TCU state
   void processedEventHandler(EventStatus status);

   // Event listener
   void registerListener(weak_ptr<IEventListener> eventListener,
                         TriggerType triggerType = TriggerType::UNKNOWN);
   void updateEventStatus(shared_ptr<Event> event, bool processed, bool succeed,
                          EventStatus status);
};

#endif