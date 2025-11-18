/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include "JsonParser.hpp"

#include "ServerEventManager.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"

#define UNSOLICITED_COMMON_EVENT "all"
#define UPDATE_API_RESPONSE_EVENT "json_update"
#define SPACE_DELIM ' '
#define DOT_DELIM '.'

ServerEventManager::ServerEventManager() {
    LOG(DEBUG, __FUNCTION__);
}

ServerEventManager::~ServerEventManager() {
    LOG(DEBUG, __FUNCTION__);
}

ServerEventManager &ServerEventManager::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static ServerEventManager instance;
    return instance;
}

/**
* This overloaded method filters the incoming events from event_injector. Based on the
* filtering results, it either notifies that listener or ignores the notification.
*/
void ServerEventManager::handleEventNotifications(
    ::eventService::UnsolicitedEvent message) {
    LOG(DEBUG, __FUNCTION__);

    std::string filter = message.filter();
    std::lock_guard<std::mutex> lk(listenerMutex_);
    if (filter == UNSOLICITED_COMMON_EVENT) {
        LOG(DEBUG, __FUNCTION__, " passing common event::", message.event());
        //passing the unsolicited common event to all the listeners
        for (auto it = listeners_.begin(); it != listeners_.end(); ++it) {
            for (auto &listener: it->second) {
                auto sp = listener.lock();
                if (sp) {
                    sp->onEventUpdate(message);
                }
            }
        }
    } else if (filter == UPDATE_API_RESPONSE_EVENT) {
        LOG(DEBUG, __FUNCTION__, " json update event");
        updateApiResponse(message.event());
    } else {
        LOG(DEBUG, __FUNCTION__, " passing unsolicited event::", message.filter());
        //passing the unsolicited event to the listener who subscribed for it
        if(listeners_.find(filter) != listeners_.end()) {
            for (auto it = listeners_[filter].begin(); it != listeners_[filter].end();) {
                auto sp = (*it).lock();
                if (sp) {
                    sp->onEventUpdate(message);
                } else {
                    LOG(DEBUG, "erased obsolete weak pointer from EventManager listeners");
                    it = listeners_[filter].erase(it);
                    continue;
                }
                ++it;
            }
        } else {
            LOG(INFO, __FUNCTION__, " No filters registered.");
        }
    }
}

/**
* This overloaded method forwards the incoming events from server manager implementations.
* It is mainly to handle the use cases where an action performed on one manager, impacts
* the other manager. For ex: RAT preference changed by Telephony may impact data as well.
* Based on the filtering results, message is either forwarded to the listener or ignored.
*/
void ServerEventManager::sendServerEvent(::eventService::ServerEvent message) {
    LOG(DEBUG, __FUNCTION__);

    std::string filter = message.filter();
    std::lock_guard<std::mutex> lk(listenerMutex_);

    LOG(DEBUG, __FUNCTION__, " passing unsolicited event::", message.filter());
    //passing the unsolicited event to the listener who subscribed for it
    if(listeners_.find(filter) != listeners_.end()) {
        for (auto it = listeners_[filter].begin(); it != listeners_[filter].end();) {
            auto sp = (*it).lock();
            if (sp) {
                sp->onServerEvent(message.any());
            } else {
                LOG(DEBUG, "erased obsolete weak pointer from EventManager listeners");
                it = listeners_[filter].erase(it);
                continue;
            }
            ++it;
        }
    } else {
        LOG(ERROR, __FUNCTION__, " No filter for registered listeners");
    }
}

telux::common::Status ServerEventManager::registerListener(
    std::weak_ptr<IServerEventListener> listener, std::string filter) {
    LOG(DEBUG, __FUNCTION__, " filter:", filter);
    std::lock_guard<std::mutex> listenerLock(listenerMutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        listeners_[filter].insert(listener);
        LOG(DEBUG, "Registering Listener");
    }
    else {
        LOG(ERROR, "Failed to register");
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status ServerEventManager::deregisterListener(
        std::weak_ptr<IServerEventListener> listener, std::string filter) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(listenerMutex_);
    if (listeners_.find(filter) == listeners_.end()) {
        LOG(INFO, __FUNCTION__, " Filter not found: ", filter);
        return telux::common::Status::NOSUCH;
    }
    auto spt = listener.lock();

    if (spt != nullptr) {
        auto eventItr = listeners_[filter].find(listener);
        if (eventItr != listeners_[filter].end()) {
            listeners_[filter].erase(eventItr);
            if (listeners_[filter].size() == 0) {
                LOG(INFO, __FUNCTION__, " Filter erased: ", filter);
                listeners_.erase(filter);
            }
        }

        LOG(DEBUG, "In deRegister removed listener");
        retVal = telux::common::Status::SUCCESS;
    }
    return retVal;
}

telux::common::Status ServerEventManager::registerListener(
    std::weak_ptr<IServerEventListener> listener,
    std::vector<std::string> filters) {

    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::SUCCESS;
    for (auto &x: filters) {
        status = registerListener(listener, x);

        if (status != telux::common::Status::SUCCESS)
            break;
    }

    return status;
}

telux::common::Status ServerEventManager::deregisterListener(
    std::weak_ptr<IServerEventListener> listener,
    std::vector<std::string> filters) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::SUCCESS;
    for (auto &x: filters) {
        status = deregisterListener(listener, x);

        if (status != telux::common::Status::SUCCESS)
            break;
    }

    return status;
}

/**
* @brief This API is to handle dynamic json updates on server side.
*/
void ServerEventManager::updateApiResponse(std::string message) {
    LOG(DEBUG, __FUNCTION__, message);
    std::stringstream stream(message);
    std::vector<std::string> attributeList;

    //skipping modify action, would be probably used
    //once we add more features to our json utility.
    std::string action = "";
    std::getline(stream, action, SPACE_DELIM);

    //reading path
    std::string path = "";
    std::getline(stream, path, SPACE_DELIM);

    //reading attributes
    std::string attributes = "";
    std::getline(stream, attributes, SPACE_DELIM);

    //reading value
    std::string value = "";
    std::getline(stream, value, SPACE_DELIM);
    LOG(INFO, __FUNCTION__, " for attribute::", attributes, " value::", value);

    /**
     * preparing nested attribute list, we are able to handle update
     * of multiple nested level of attributes.
     * For ex:
     * - ISmsManager.deleteMessage.error - is to update error value.
     * - ISmsManager.sendSms.0.numberOfSegments - is to update numberOfSegments
     *                                            at 0th index in sendSms array.
    */
    std::stringstream ss(attributes);
    std::string currAttribute;
    while (getline(ss, currAttribute, DOT_DELIM)) {
        currAttribute.erase(remove_if(currAttribute.begin(), currAttribute.end(),
            ::isspace), currAttribute.end());
        attributeList.push_back(currAttribute);
    }

    //updating the attribute value
    Json::Value rootObj;
    Json::Value *currObj;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, path);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ");
        LOG(ERROR, __FUNCTION__, " filePath::", path);
        return;
    }
    currObj = &rootObj;
    for (const auto & key: attributeList) {
        if (std::all_of(key.begin(), key.end(), ::isdigit)) {
            //since for update of array we would be taking index value
            currObj = &(*currObj)[std::stoi(key)];
            continue;
        }
        currObj = &(*currObj)[key];
    }

    if((*currObj).isInt()) {
        *currObj = std::stoi(value);
    } else if ((*currObj).isBool()) {
        *currObj = (value == "true")? true : false;
    } else if ((*currObj).isString()) {
        *currObj = value;
    } else {
        LOG(ERROR, __FUNCTION__, " invalid type");
    }

    JsonParser::writeToJsonFile(rootObj, path);
}