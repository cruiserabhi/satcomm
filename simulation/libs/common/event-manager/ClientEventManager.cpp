/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #include "ClientEventManager.hpp"

namespace telux {
namespace common {

ClientEventManager::ClientEventManager()
: EventManager<::eventService::EventDispatcherService>() {
    LOG(DEBUG, __FUNCTION__);
}

ClientEventManager::~ClientEventManager() {
    LOG(DEBUG, __FUNCTION__);
}

ClientEventManager &ClientEventManager::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static ClientEventManager instance;
    return instance;
}

} // end of namespace common
} // end of namespace telux