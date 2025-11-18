/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * Command callback management utility. Classes that service commands,
 * could use this utility to store the callers ICommandCallback and get an Id in return.
 * When the command /operation completes the class could retrieve the callback using the id
 * and notify the caller about the result of the operation/command
 */

#ifndef COMMANDCALLBACK_HPP
#define COMMANDCALLBACK_HPP

#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>

#include <telux/common/CommonDefines.hpp>

#include "Logger.hpp"

#define INVALID_COMMAND_ID -1

namespace telux {
namespace common {

// derived class to wrap std::function
template <typename... Args>
class FunctionalCallback : public ICommandCallback {
public:
   using fnCb = std::function<void(Args...)>;
   fnCb callback_;
   FunctionalCallback(fnCb callback)
      : callback_(callback) {
   }
};

class CommandCallbackManager {
public:
   CommandCallbackManager();
   ~CommandCallbackManager();

   void reset(void);
   int addCallback(std::shared_ptr<telux::common::ICommandCallback> callback);
   std::shared_ptr<telux::common::ICommandCallback> findAndRemoveCallback(int cmdId);

   template <typename... Args>
   std::shared_ptr<ICommandCallback> createCallback(std::function<void(Args...)> callback) {
      if(callback) {
         using functionCb = FunctionalCallback<Args...>;
         return std::make_shared<functionCb>(callback);
      }
      return nullptr;
   }

   template <typename... Args>
   int addCallback(std::function<void(Args...)> callback) {
      std::lock_guard<std::mutex> m(mutex_);
      int cmdId = getNextCommandId();
      auto cb = createCallback(callback);
      funCallbackMap_[cmdId] = cb;
      return cmdId;
   }

   // Wrapper function to execute the functional callback
   // This is a utility function so that clients do not have to deal with the
   // templatized CommandCallback object
   template <typename... Args>
   void executeCallback(std::shared_ptr<ICommandCallback> callback, Args &&... args) {
      if(callback) {
         try {
            using functionCb = FunctionalCallback<Args...>;
            std::shared_ptr<functionCb> cb = std::static_pointer_cast<functionCb>(callback);
            cb->callback_(std::forward<Args>(args)...);
         } catch(const std::bad_function_call &e) {
            LOG(DEBUG, __FUNCTION__, " Exception during executeCallback: ", e.what());
         }
      } else {
         LOG(DEBUG, __FUNCTION__, "Command Callback is null");
      }
   }

   CommandCallbackManager(const CommandCallbackManager &) = delete;
   CommandCallbackManager &operator=(const CommandCallbackManager &) = delete;

private:
   // Mutex for guarding callbackMap
   std::mutex mutex_;

   std::unordered_map<int, std::weak_ptr<telux::common::ICommandCallback>> cmdCallbackMap_;

   // map to store the functional callbacks
   std::unordered_map<int, std::shared_ptr<ICommandCallback>> funCallbackMap_;

   int commandId_;  // Unique identifier for the callback
   int getNextCommandId();
};

}  // End of namespace common
}  // End of namespace telux

#endif  // End of COMMANDCALLBACK_HPP
