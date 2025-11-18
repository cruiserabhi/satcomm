/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EventManager.hpp"
#include <algorithm>

EventManager *EventManager::instance = nullptr;

EventManager *EventManager::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    if (!instance) {
        instance = new EventManager();
    }
    return instance;
}

bool EventManager::init() {
    LOG(DEBUG, __FUNCTION__);
    bool initSucceed = true;
    //  Get the ConnectionFactory instances.
    auto &powerFactory = telux::power::PowerFactory::getInstance();

    std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();

    ClientInstanceConfig config;
    config.clientName = DAEMON_NAME + std::to_string(getpid());
    config.clientType = ClientType::MASTER;
    config.machineName = ALL_MACHINES;
    tcuActivityStateMgr_ = powerFactory.getTcuActivityManager(config,
                                           [&](telux::common::ServiceStatus status) {
                                               LOG(DEBUG, __FUNCTION__, " Init Callback called ");
                                               prom.set_value(status);
                                           });
    if (tcuActivityStateMgr_ == nullptr) {
        LOG(ERROR, __FUNCTION__, " ERROR - Failed to get manager instance");
        initSucceed = false;
    }

    // Wait for TCU-activity manager to be ready
    LOG(DEBUG, __FUNCTION__, "  Waiting for TCU Activity Manager to be ready ");
    telux::common::ServiceStatus serviceStatus = prom.get_future().get();
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, __FUNCTION__, " TCU-activity manager is ready");

        // considering during boot up system state will be resume
        tcuActivityStateMgr_->setActivityState(
            TcuActivityState::RESUME, ALL_MACHINES, [this](ErrorCode errorCode) {
            if (errorCode == telux::common::ErrorCode::SUCCESS) {
                LOG(DEBUG, " Setting resume in beginning Command initiated successfully " );
            } else {
                LOG(ERROR,  " Setting resume in beginning Command failed !!!"  );
            }
        });
    } else {
        LOG(ERROR, __FUNCTION__, " Failed to initialize TCU-activity manager");
        initSucceed = false;
    }
    if (initSucceed) {
        initSucceed = registerTcuActivityManager();
    }
    return initSucceed;
}

EventManager::EventManager() {
    LOG(DEBUG, __FUNCTION__);
}
EventManager::~EventManager() {
    LOG(DEBUG, __FUNCTION__);
    tcuActivityStateMgr_.reset();
    eventQueue_.clear();
}

void EventManager::notifyOnEventRejected(shared_ptr<Event> event, EventStatus status) {
    LOG(DEBUG, __FUNCTION__, " status  = ", (int)status, " event = ", event->toString());

    event->setEventStatus(status);
    // TriggerType
    for (std::vector<weak_ptr<IEventListener>>::iterator it =
             eventListeners_[TriggerType::UNKNOWN].begin();
         it != eventListeners_[TriggerType::UNKNOWN].end(); ++it) {
        if (std::shared_ptr<IEventListener> eventListener = (*it).lock()) {
            eventListener->onEventRejected(event, status);
        }
    }

    for (std::vector<weak_ptr<IEventListener>>::iterator it =
             eventListeners_[event->getTriggerType()].begin();
         it != eventListeners_[event->getTriggerType()].end(); ++it) {
        if (std::shared_ptr<IEventListener> eventListener = (*it).lock()) {
            eventListener->onEventRejected(event, status);
        }
    }
}

void EventManager::notifyAndEraseEventProcessed(TriggerType triggerType,
                                                TcuActivityState triggeredState, bool succeed,
                                                EventStatus status) {
    LOG(DEBUG, __FUNCTION__);
    // notify  and erase duplicate event
    for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin();
         it != eventQueue_.end();) {
        if ((*it)->getTriggeredState() == triggeredState) {
            shared_ptr<Event> sameEventInQueue = *it;

            sameEventInQueue->setEventStatus(status);
            LOG(DEBUG, __FUNCTION__, "  removing event id = ", (int)sameEventInQueue->getId());
            for (std::vector<weak_ptr<IEventListener>>::iterator itl =
                     eventListeners_[TriggerType::UNKNOWN].begin();
                 itl != eventListeners_[TriggerType::UNKNOWN].end(); ++itl) {
                if (std::shared_ptr<IEventListener> eventListener = (*itl).lock()) {
                    eventListener->onEventProcessed(sameEventInQueue, succeed);
                }
            }
            for (std::vector<weak_ptr<IEventListener>>::iterator itl =
                     eventListeners_[triggerType].begin();
                 itl != eventListeners_[triggerType].end(); ++itl) {
                if (std::shared_ptr<IEventListener> eventListener = (*itl).lock()) {
                    eventListener->onEventProcessed(sameEventInQueue, succeed);
                }
            }
            it = eventQueue_.erase(it);
        } else {
            break;
        }
    }

    LOG(DEBUG, __FUNCTION__, " end");
}

void EventManager::updateEventStatus(shared_ptr<Event> event,
                                     bool removeFromQueue, bool succeed, EventStatus status) {
    LOG(DEBUG, __FUNCTION__, "  event = ", (int)event->getId(), " ,status = ", (int)status,
        ", remove from queue = ", (int)removeFromQueue, " succeed = ", (int)succeed);
    if (removeFromQueue) {
        notifyAndEraseEventProcessed(event->getTriggerType(), event->getTriggeredState(), succeed,
                                     status);
    } else {
        // will update failure cases
        if (!succeed) {
            notifyOnEventRejected(event, status);
        }
    }
}

void EventManager::printQueue() {
    LOG(DEBUG, __FUNCTION__);
    for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin(); it != eventQueue_.end();
         ++it) {
        LOG(DEBUG, __FUNCTION__, " event = ", (*it)->toString());
    }
}

// event management
void EventManager::pushEvent(shared_ptr<Event> event) {
    LOG(DEBUG, __FUNCTION__, " event = ", event->toString());
    std::lock_guard<std::mutex> lk(eventQueueUpdate_);
    printQueue();
    TcuActivityState newState = event->getTriggeredState();
    do {
        std::vector<std::string> machineNames;
        //check if provided valid machine name
        if (tcuActivityStateMgr_->getAllMachineNames(machineNames) ==
            telux::common::Status::SUCCESS) {
            auto it = std::find(machineNames.begin(), machineNames.end(), event->getMachineName());
            if (it == machineNames.end() && event->getMachineName() != ALL_MACHINES) {
                LOG(ERROR, __FUNCTION__, " unable to find given machine name");
                updateEventStatus(event, false, false,
                                    EventStatus::REJECTED_INVALID_MACHINE_NAME);

                break;
            }
        } else {
            LOG(ERROR, __FUNCTION__, " unable to get available machine names");
        }

        if (!eventQueue_.empty()) {
            // consider 1st event is in progress if eventQueue_ size more then 1
            TcuActivityState inProgressTrigger = eventQueue_[0]->getTriggeredState();
            // check event in progress for event overriden
            for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin();
                it != eventQueue_.end();) {
                if ((*it)->getTriggeredState() != newState &&
                    (*it)->getTriggeredState() != inProgressTrigger) {

                    shared_ptr<Event> overridenEvent = *it;
                    LOG(ERROR, __FUNCTION__,
                        " REJECTED_EVENT_OVERRIDDEN  event = ", overridenEvent->toString());

                    it = eventQueue_.erase(it);
                    updateEventStatus(overridenEvent, false, false,
                                    EventStatus::REJECTED_EVENT_OVERRIDDEN);
                } else {
                    ++it;
                }
            }
            event->setEventStatus(EventStatus::IN_QUEUE);
            eventQueue_.push_back(event);
        } else {

            if (tcuActivityStateMgr_->getServiceStatus() !=
                    telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                // tcu activity manager down
                LOG(ERROR, __FUNCTION__, " tcu activity state manager down ");
                updateEventStatus(event, false, false, EventStatus::FAILED_TCU_ACTIVITY);
                break;
            }
            // Hold the wake lock temporarily to avoid the device getting suspended automatically
            // while processing the event
            holdWakeLock();
            eventQueue_.push_back(event);
            setActivityState(event);
        }
    }while (0);
}

void EventManager::setActivityState(shared_ptr<Event> event) {
    LOG(DEBUG, __FUNCTION__);
    tcuActivityStateMgr_->setActivityState(event->getTriggeredState(), event->getMachineName(),
        [event, this](ErrorCode errorCode) {
        if (errorCode != telux::common::ErrorCode::SUCCESS ) {
            LOG(ERROR, __FUNCTION__,  " Command failed !!!"  );
            processedEventHandler(EventStatus::FAILED_TCU_ACTIVITY);
        } else {
            LOG(DEBUG, __FUNCTION__,  " Command initiated successfully " );
            if (event->getTriggeredState() == TcuActivityState::RESUME) {
                //Acknowledgment message (onSlaveAckStatusUpdate) is not expected for resume.
                processedEventHandler(EventStatus::SUCCEED);
                releaseWakeLock();
            } else {
                event->setEventStatus(EventStatus::IN_PROGRESS_TCU_ACTIVITY);
            }
        }
    });
}

// Event listener
void EventManager::registerListener(weak_ptr<IEventListener> eventListener,
                                    TriggerType triggerType) {
    LOG(DEBUG, __FUNCTION__, " TriggerType = ", (int)triggerType);
    if (eventListeners_.find(triggerType) == eventListeners_.end()) {
        std::vector<weak_ptr<IEventListener>> temp;
        temp.push_back(eventListener);
        eventListeners_.insert({triggerType, temp});
    } else {
        eventListeners_[triggerType].push_back(eventListener);
    }
}

void EventManager::writeToSystemNode(char *nodepath, char *value, int length) {
    LOG(DEBUG, __FUNCTION__);
    int fd = -1;
    LOG(DEBUG, __FUNCTION__, " About to open node ", string(nodepath));
    string logTmp;
    fd = open(nodepath, O_WRONLY | O_APPEND | O_NONBLOCK);
    if (fd < 0) {
        logTmp = " Opening of  node failed!!! err " + string(nodepath) +
                 " errno = " + string(strerror(errno));
        LOG(ERROR, __FUNCTION__, logTmp);
    } else {
        LOG(DEBUG, __FUNCTION__, " Opening of  node success ", string(nodepath));
        int returnValueWrite = write(fd, value, length);
        if (returnValueWrite == -1) {
            logTmp = " Writing of  to  node failed err "+ string(value) +
                     " "+string(nodepath) + " errno = " + string(strerror(errno));
            LOG(ERROR, __FUNCTION__, logTmp);
        } else {
            logTmp = " Writing of  to  node success " + string(value) +
                     " " + string(nodepath);
            LOG(DEBUG, __FUNCTION__, logTmp);
        }
    }
    close(fd);
}

void EventManager::holdWakeLock() {
    LOG(DEBUG, __FUNCTION__);
    writeToSystemNode((char *)WAKELOCK_PATH, (char *)WAKE_LOCK,
        strlen(WAKE_LOCK));
}

void EventManager::releaseWakeLock() {
    LOG(DEBUG, __FUNCTION__);
    writeToSystemNode((char *)WAKEUNLOCK_PATH, (char *)WAKE_LOCK,
        strlen(WAKE_LOCK));
}

void EventManager::processedEventHandler(EventStatus status) {
    LOG(DEBUG, __FUNCTION__, " status = ", RefAppUtils::eventStatusToString(status));
    std::lock_guard<std::mutex> lk(eventQueueUpdate_);
    printQueue();

    // Note: even in case of timeout or other error, the master is proceeding with state change
    bool isEventExecutionSucceed = true;
    if (!eventQueue_.empty()) {
        // check event in progress
        shared_ptr<Event> processedEvent =
            *((std::deque<shared_ptr<Event>>::iterator)eventQueue_.begin());

        //check for the latest event
        if (eventQueue_.back()->getTriggeredState() != processedEvent->getTriggeredState()) {
            LOG(ERROR, __FUNCTION__, " found conflict with latest event");
            updateEventStatus(processedEvent, true, false, EventStatus::REJECTED_EVENT_OVERRIDDEN);
            isEventExecutionSucceed = false;
        } else {
            updateEventStatus(processedEvent, true, isEventExecutionSucceed, status);
        }

        // keep processing the next event
        LOG(DEBUG, __FUNCTION__, " check next event ");
        if (!eventQueue_.empty()) {
            shared_ptr<Event> nextEvent =
                *((std::deque<shared_ptr<Event>>::iterator)eventQueue_.begin());
            LOG(DEBUG, __FUNCTION__, " execute next event. event = ", nextEvent->toString());
            setActivityState(nextEvent);
        } else {
            //after processing all event in queue remove temporary wake lock
            releaseWakeLock();
        }
    } else {
        LOG(ERROR, __FUNCTION__, "  eventQueue is empty");
    }
}

void EventManager::onSlaveAckStatusUpdate(const telux::common::Status status,
    const std::string machineName, const std::vector<ClientInfo> unresponsiveClients,
    const std::vector<ClientInfo> nackResponseClients) {
    LOG(DEBUG, __FUNCTION__);
    EventStatus eventStatus = EventStatus::FAILED_TCU_ACTIVITY;
    if (status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " Slave applications successfully acknowledged the state",
            " transition");
        eventStatus = EventStatus::SUCCEED;
    } else if (status == telux::common::Status::EXPIRED) {
        LOG(ERROR, __FUNCTION__, " Timeout occured while waiting for acknowledgements from slave",
            " applications");
        eventStatus = EventStatus::FAILED_TCU_ACTIVITY_TIMEOUT;
    } else {
        LOG(ERROR, __FUNCTION__, " Failed to receive acknowledgements from slave applications");
        eventStatus = EventStatus::FAILED_TCU_ACTIVITY;
    }

    if(unresponsiveClients.size() > 0) {
        LOG(ERROR, __FUNCTION__, " Number of unresponsive clients : ", unresponsiveClients.size());
        for (size_t i = 0; i < unresponsiveClients.size(); i++) {
            LOG(ERROR, __FUNCTION__, " client name : ", unresponsiveClients[i].first
                , " , machine name : ", unresponsiveClients[i].second);
        }
    }

    if(nackResponseClients.size() > 0) {
        LOG(ERROR, __FUNCTION__, " Number of clients responded with nack : ",
         nackResponseClients.size());
        for (size_t i = 0; i < nackResponseClients.size(); i++) {
            LOG(ERROR, __FUNCTION__, " client name : ", nackResponseClients[i].first
                    ," , machine name : ", nackResponseClients[i].second);
        }
    }

    processedEventHandler(eventStatus);
}

void EventManager::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status : ", RefAppUtils::serviceStatusToString(status));
}

bool EventManager::registerTcuActivityManager() {
    LOG(DEBUG, __FUNCTION__);
    // Registering a listener for TCU-activity state updates
    // weak_ptr<ITcuActivityListener> ptr(instance);
    telux::common::Status status = tcuActivityStateMgr_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " ERROR - Failed to register for TCU-activity state updates");
        return false;
    } else {
        LOG(DEBUG, __FUNCTION__, " Registered Listener for TCU-activity state updates");
    }
    // Registering a listener for TCU-activity management service status updates
    status = tcuActivityStateMgr_->registerServiceStateListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " ERROR - Failed to register for Service status updates");
        return false;
    }
    return true;
}

void EventManager::onTcuActivityStateUpdate(TcuActivityState state, std::string machineName) {
    // The master is not expected to get this indication, as the master is the one who triggers the
    // state change. Other concerned slave clients will get this indication, and it is expected that
    // the slave acknowledges this indication via (sendActivityStateAck).
    LOG(ERROR, __FUNCTION__, " ", RefAppUtils::tcuActivityStateToString(state));
}
