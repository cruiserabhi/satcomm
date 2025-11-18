/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CommandCallbackManager.hpp"

namespace telux {
namespace common {

CommandCallbackManager::CommandCallbackManager() {
   LOG(DEBUG, __FUNCTION__);
   commandId_ = 0;
}

CommandCallbackManager::~CommandCallbackManager() {
   LOG(DEBUG, __FUNCTION__);
   // Cleanup of cmdCallbackMap_
   std::lock_guard<std::mutex> m(mutex_);
   cmdCallbackMap_.clear();
   funCallbackMap_.clear();
}

int CommandCallbackManager::getNextCommandId() {
   commandId_++;
   /*
    * 1. Exclude Special Common ID number: INVALID_COMMAND_ID.
    *
    * 2. Exclude 0 as valid cmdId:
    *    Consider following code flow where an integer is typecasted to void * and
    *    sent as cookie.
    *
    *  sendRequest(callback) {
    *     intptr_t cmdId = cmdCallbackMgr_.addCallback(callback);
    *     void *cookie = (void *)cmdId;
    *     qmiClient->sendAsyncRequest(cookie);
    *  }
    *  receiveResponse(callback, void *cookie) {
    *      if (!cookie) {
    *          If we use 0 as valid cmdId, then this if-condition will become true misleading
    *          us into believing that cookie was not passed which is not true. Therefore,
    *          exclude 0 from being a valid cmdId.
    *      }
    *      callback = cmdCallbackMgr_.findAndRemoveCallback((intptr_t)cookie);
    *  }
    */
   if ((commandId_ == INVALID_COMMAND_ID) || (commandId_ == 0)) {
       commandId_++;
   }
   return commandId_;
}

int CommandCallbackManager::addCallback(std::shared_ptr<telux::common::ICommandCallback> callback) {
   std::lock_guard<std::mutex> m(mutex_);
   int cmdId = getNextCommandId();
   cmdCallbackMap_[cmdId] = callback;
   return cmdId;
}

std::shared_ptr<telux::common::ICommandCallback>
   CommandCallbackManager::findAndRemoveCallback(int cmdId) {
   std::lock_guard<std::mutex> m(mutex_);
   if(cmdId < 0) {
      LOG(DEBUG, __FUNCTION__, " invalid cmdId: ", cmdId);
      return nullptr;
   }

   auto entry = cmdCallbackMap_.find(cmdId);
   if(entry != cmdCallbackMap_.end()) {
      auto sp = cmdCallbackMap_[cmdId];
      cmdCallbackMap_.erase(entry);
      LOG(DEBUG, __FUNCTION__, " Removing callback for cmdId : ", cmdId, " cmdCallbackMap_ size: ",
          cmdCallbackMap_.size());
      return sp.lock();
   }

   auto it = funCallbackMap_.find(cmdId);
   if(it != funCallbackMap_.end()) {
      auto sp = funCallbackMap_[cmdId];
      funCallbackMap_.erase(it);
      LOG(DEBUG, __FUNCTION__, " Removing callback for cmdId : ", cmdId, " funCallbackMap_ size: ",
          funCallbackMap_.size());
      return sp;
   }
   return nullptr;
}

/*
 * Release all callbacks.
 */
void CommandCallbackManager::reset() {
   std::lock_guard<std::mutex> m(mutex_);
   cmdCallbackMap_.clear();
   funCallbackMap_.clear();
   commandId_ = 0;
}

}
}
