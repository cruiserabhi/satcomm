/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CLIENT_EVENT_MANAGER_HPP
#define CLIENT_EVENT_MANAGER_HPP

#include "EventManager.hpp"

namespace telux {
namespace common {

class ClientEventManager :
    public EventManager<::eventService::EventDispatcherService> {

public:
    static ClientEventManager &getInstance();

private:
    ClientEventManager();
    virtual ~ClientEventManager();
};

} // end of namespace common
} // end of namespace telux

#endif //CLIENT_EVENT_MANAGER_HPP