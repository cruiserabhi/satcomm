/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef EVENT_SERVICE_HPP
#define EVENT_SERVICE_HPP

#include <iostream>

#include "EventServiceHelper.hpp"

class EventService final:
    public EventServiceHelper<eventService::EventDispatcherService> {
    /**
    * @brief This class acts as the central event service for the framework
    *        on the server side & is responsible for receiving events from
    *        the event_injector & forwarding to the eventManager locally.
    *        It is also responsible for forwarding the events to EventManager
    *        on the client side via writing to the stream.
    */
public:
    static EventService &getInstance();

private:
    EventService();
    ~EventService();
};

#endif // EVENT_SERVER_HPP