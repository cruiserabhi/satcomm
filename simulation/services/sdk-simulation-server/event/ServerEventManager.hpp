/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SERVER_EVENT_MANAGER_HPP
#define SERVER_EVENT_MANAGER_HPP

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <unordered_map>
#include <telux/common/CommonDefines.hpp>
#include "protos/proto-src/event_simulation.grpc.pb.h"

#define MODEM_FILTER "modem_filter"

class IServerEventListener {
public:
    /**
     * @brief This API is to receive the events, broadcasted by ServerEventManager
     * locally to all the managers on server side.
     * The events triggered from event_injector are in string format &
     * has to converted to google::protobuf::Any type by vertical specific server Impl.
     *
     * @param event - A string depicting the event.
     */
    virtual void onEventUpdate(::eventService::UnsolicitedEvent event) {}

    /**
     * @brief This API is to receive the events, broadcasted by ManagerServerImpl
     * locally to all the managers on server side.
     * It is mainly to handle the use cases where an action performed on one manager, impacts
     * the other manager. For ex: RAT preference changed by Telephony may impact data as well.
     *
     * @param event - google::protobuf::Any message depicting the event
     */
    virtual void onServerEvent(google::protobuf::Any event) {}

    virtual ~IServerEventListener() {}
};

class ServerEventManager {
    /**
    * @brief This class acts as the event manager on the server side.
    * It is responsible for broadcasting the event locally to vertical specific
    * services.
    */
public:
    static ServerEventManager &getInstance();

    telux::common::Status registerListener(
        std::weak_ptr<IServerEventListener> listener,
        std::vector<std::string> filter);

    telux::common::Status deregisterListener(
        std::weak_ptr<IServerEventListener> listener,
        std::vector<std::string> filter);

    telux::common::Status registerListener(
        std::weak_ptr<IServerEventListener> listener, std::string filter);

    telux::common::Status deregisterListener(
        std::weak_ptr<IServerEventListener> listener, std::string filter);

    void handleEventNotifications(::eventService::UnsolicitedEvent message);
    void sendServerEvent(::eventService::ServerEvent message);

private:
    ServerEventManager();
    ~ServerEventManager();

    void updateApiResponse(std::string message);

    std::unordered_map<std::string,
        std::set<std::weak_ptr<IServerEventListener>,
        std::owner_less<std::weak_ptr<IServerEventListener>>>> listeners_;

    std::mutex listenerMutex_;
};

#endif // SERVER_EVENT_MANAGER_HPP
