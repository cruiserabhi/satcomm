/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * ListenerManager class to register and deregister listeners of specific events
 */
#ifndef LISTENERMANAGER_HPP
#define LISTENERMANAGER_HPP
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <bitset>
#include <set>
#include <map>

#include <telux/common/CommonDefines.hpp>
#include "Logger.hpp"

namespace telux {

namespace common {

template <typename T, typename U = std::bitset<32>>
class ListenerManager {
public:


ListenerManager() {
    LOG(DEBUG, __FUNCTION__);
}

~ListenerManager() {
    LOG(DEBUG, __FUNCTION__);
    cleanup();
}

telux::common::Status registerListener(std::weak_ptr<T> listener) {
   auto sp = listener.lock();

   if(sp == nullptr) {
      LOG(ERROR, "Null listener");
      return telux::common::Status::INVALIDPARAM;
   }

   std::lock_guard<std::mutex> lock(listenerMutex_);
   // Check whether the listener existed ...
   auto itr = std::find_if(
      std::begin(listeners_), std::end(listeners_),
      [=](std::weak_ptr<T> listenerExisted) { return (listenerExisted.lock() == sp); });
   if(itr != std::end(listeners_)) {
      LOG(DEBUG, "registerListener() - listener already exists");
      return telux::common::Status::ALREADY;
   }

   LOG(DEBUG, "registerListener() - creates a new listener entry");
   listeners_.emplace_back(listener);  // store listener

   return telux::common::Status::SUCCESS;
}

telux::common::Status deRegisterListener(std::weak_ptr<T> listener) {
   bool listenerExisted = false;
   std::lock_guard<std::mutex> lock(listenerMutex_);
   for(auto it = listeners_.begin(); it != listeners_.end();) {
      auto sp = (*it).lock();
      if(!sp) {
         LOG(DEBUG, "Erasing obsolete weak pointer from Listener");
         it = listeners_.erase(it);
      } else if(sp == listener.lock()) {
         it = listeners_.erase(it);
         LOG(DEBUG, "removeListener success");
         listenerExisted = true;
      } else {
         ++it;
      }
   }
   if(listenerExisted) {
      return telux::common::Status::SUCCESS;
   } else {
      LOG(WARNING, "QmiClient removeListener: listener not found");
      return telux::common::Status::NOSUCH;
   }
}

void getAvailableListeners(
   std::vector<std::weak_ptr<T>> &availableListeners) {
   // Entering critical section, copy lockable shared_ptr from global listener
   std::lock_guard<std::mutex> lock(listenerMutex_);
   for(auto it = listeners_.begin(); it != listeners_.end();) {
      auto sp = (*it).lock();
      if(sp) {
         availableListeners.emplace_back(sp);
         ++it;
      } else {
         // if we unable to lock the listener, we should remove it from
         // listenerList
         LOG(DEBUG, "erased obsolete weak pointer from listeners");
         it = listeners_.erase(it);
      }
   }
}

/** Map the listener for selected indications as present in the bitset.*/
telux::common::Status registerListener(std::weak_ptr<T> listener, U indications,
    U &firstRegistration) {
    auto sp = listener.lock();
    // Flag to determine if there is atleast one new listener for any indication
    bool foundNewListener = false;
    if(sp == nullptr) {
        LOG(ERROR, __FUNCTION__, " Null listener");
        return telux::common::Status::INVALIDPARAM;
    }
    // When no indications are provided, simply return success without storing the listener for any
    // indication
    if(indications.none()) {
        LOG(WARNING, __FUNCTION__, " No indications provided");
        return telux::common::Status::SUCCESS;
    }
    std::lock_guard<std::mutex> lock(listenerMutex_);
    for(size_t itr = 0; itr < indications.size(); itr++) {
        if(indications.test(itr)) {
            // Check if the indication is being registered for the first time.
            if(registrationMap_[itr].empty()) {
                foundNewListener = true;
                firstRegistration.set(itr);
                registrationMap_[itr].insert(sp);
            } else {
                // Check whether the listener is already registered
                auto temp = std::find_if(
                    std::begin(registrationMap_[itr]), std::end(registrationMap_[itr]),
                    [=](std::weak_ptr<T> listenerExisted) { return (listenerExisted.lock() == sp);
                    });
                if(temp != std::end(registrationMap_[itr])) {
                    LOG(DEBUG, __FUNCTION__, " listener already exists for ",
                        static_cast<int>(itr));
                } else {
                    foundNewListener = true;
                    registrationMap_[itr].insert(sp);
                }
            }
        }
    }
    if(!foundNewListener) {
        return telux::common::Status::ALREADY;
    }
    return telux::common::Status::SUCCESS;
}

/** Erase the listener for selected indications as present in the bitset. */
telux::common::Status deRegisterListener(std::weak_ptr<T> listener, U indications,
    U &lastDeregistration) {
    auto sp = listener.lock();
    if(sp == nullptr) {
        LOG(ERROR, __FUNCTION__, " Null listener");
        return telux::common::Status::INVALIDPARAM;
    }
    // When no indications are provided, simply return success without removing any listener for
    // any indication
    if(indications.none()) {
        LOG(WARNING, __FUNCTION__, " No indications provided");
        return telux::common::Status::SUCCESS;
    }
    bool listenerExisted = false;
    std::lock_guard<std::mutex> lock(listenerMutex_);
    for(size_t itr = 0; itr < indications.size(); itr++) {
        if(indications.test(itr)) {
            if(registrationMap_.find(itr) != registrationMap_.end()) {
                if(registrationMap_[itr].erase(sp)) {
                    listenerExisted = true;
                    // Check if the last listener was deregistered from the indication.
                    if(registrationMap_[itr].empty()) {
                        lastDeregistration.set(itr);
                    }
                }
            }
        }
    }
    if(listenerExisted) {
        return telux::common::Status::SUCCESS;
    } else {
        LOG(ERROR, __FUNCTION__, " Listener not found");
        return telux::common::Status::NOSUCH;
    }
}

/** If the indication is present in the map, return the corresponding list of listeners. */
void getAvailableListeners(uint32_t indication, std::vector<std::weak_ptr<T>> &vec) {
    std::lock_guard<std::mutex> lock(listenerMutex_);
    auto keyPair = registrationMap_.find(indication);
    if(keyPair == registrationMap_.end()) {
        return;
    }
    auto listener = (*keyPair).second.begin();
    auto listenersEnd = (*keyPair).second.end();
    for( ;listener != listenersEnd; ) {
        auto sp = (*listener).lock();
        if(sp) {
            vec.emplace_back(sp);
            ++listener;
        } else {
            // if we unable to lock the listener, we should remove it from
            // listenerList
            LOG(DEBUG, "erased obsolete weak pointer listeners");
            listener = (*keyPair).second.erase(listener);
        }
    }
}

/** Provide the bitset i.e bitmask of the active indications with listeners */
void getActiveIndications(U &activeIndications) {
    std::lock_guard<std::mutex> lock(listenerMutex_);
    for(size_t itr = 0; itr < activeIndications.size(); itr++) {
        if(registrationMap_.find(itr) != registrationMap_.end()) {
            if(!registrationMap_[itr].empty()) {
                activeIndications.set(itr);
            }
        }
    }
}

/** Query whether a listener has enabled indications*/
bool isEnableAnyIndication(std::weak_ptr<T> listener) {
    std::lock_guard<std::mutex> lock(listenerMutex_);
    for (auto it = registrationMap_.begin(); it != registrationMap_.end(); ++it) {
        if (it->second.find(listener) != it->second.end()) {
            return true;
        }
    }
    LOG(DEBUG, __FUNCTION__, " false");
    return false;
}

/** Remove all the references to listeners */
void cleanup() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(listenerMutex_);
    registrationMap_.clear();
}

private:

std::mutex listenerMutex_;
std::vector<std::weak_ptr<T>> listeners_;

/** std::weak_ptr doesn't support relational operators. Need to use a binary predicate. */
struct SetPredicate {
    bool operator() (const std::weak_ptr<T> &lhs, const std::weak_ptr<T> &rhs)const {
        auto lptr = lhs.lock();
        auto rptr = rhs.lock();
        if(!rptr) {
            //RHS is a nullptr. Eg: Any address (address < 0) is false.
            return false;
        }
        if(!lptr) {
            //LHS is a nullptr. Any address (0 < address) is true.
            return true;
        }
        //Both Lhs and Rhs are legal addresses. So we compare and return.
        return lptr < rptr;
    }
};

/** We maintain a mapping between an indication and all the corresponding listeners registered. */
std::map<uint32_t, std::set<std::weak_ptr<T>, SetPredicate> > registrationMap_;


};
}// common
} // telux
#endif
