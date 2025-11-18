/*
 *  Copyright (c) 2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "NAOIpTrigger.hpp"

NAOIpTrigger::NAOIpTrigger(std::shared_ptr<EventManager> eventManager) {
    LOG(DEBUG, __FUNCTION__);
    eventManager_ = eventManager;
}

NAOIpTrigger::~NAOIpTrigger() {
    LOG(DEBUG, __FUNCTION__);
    stopServer();
}

bool NAOIpTrigger::init() {
    LOG(DEBUG, __FUNCTION__);
    config_ = ConfigParser::getInstance();

    bool returnValue = false;

    do {
        if (!loadConfig()) {
            break;
        }
        weak_ptr<NAOIpTrigger> weakFromThis = shared_from_this();
        if(!eventManager_) {
            LOG(ERROR, __FUNCTION__, "  event manager is not available ");
            break;
        }
        dataController_ = std::make_shared<DataFilterController>();
        if (dataController_ ) {
            isUDP_ = dataController_->isUDP();
            for (size_t i = 0; i < RETRY_INIT_SDK; i++) {
                returnValue = dataController_->initializeSDK(
                    // callback to actually start naoip trigger after default data call is available
                    [&](bool isDefaultDataCall) {
                        LOG(INFO, __FUNCTION__, "isDefaultDataCall = ",(int)isDefaultDataCall,
                            " , isServerRunning = ", (int)isServerRunning_);
                        //Note: Don't hold the callback thread for long.
                        if (isDefaultDataCall) {
                            if (!isServerRunning_) {
                                std::thread server = std::thread([this] { startServer(); });
                                {
                                    std::lock_guard<std::mutex> serverUpdate(serverUpdate_);
                                    server_ = std::move(server);
                                }
                            }
                        } else {
                            std::thread stopServerTh([this] { stopServer(); });
                            stopServerTh.detach();
                        }
                    }
                );

                if (returnValue) {
                    //Listen to all triggers to be able to add and remove data filters.
                    eventManager_->registerListener(weakFromThis, TriggerType::UNKNOWN);
                    break;
                } else {
                    //telsdk initialisation failed wait for some time and retry
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                }
            }
        } else {
            LOG(ERROR, __FUNCTION__, "  Unable to instantiate data controller ");
        }

    } while (0);

    return returnValue;
}

bool NAOIpTrigger::enableFilter() {
    LOG(DEBUG, __FUNCTION__);
    if (dataController_) {
        if (dataController_->addFilter()) {
            DataRestrictMode mode;
            // Note: If filter auto exit is enabled, it will disable the filter if any packets pass
            //       through a whitelisted filter, even if it is an unexpected packet.
            //       @ref DataRestrictMode
            // ex. mode.filterAutoExit = DataRestrictModeType::ENABLE;
            mode.filterAutoExit = DataRestrictModeType::DISABLE;
            mode.filterMode = DataRestrictModeType::ENABLE;

            if (dataController_->sendSetDataRestrictMode(mode)) {
                return true;
            }
            LOG(ERROR, __FUNCTION__, " sendSetDataRestrictMode failed");
        } else {
            LOG(ERROR, __FUNCTION__, " addFilter failed");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " dataFilterController is not ready");
    }
    return false;
}

bool NAOIpTrigger::disableFilter() {
    LOG(DEBUG, __FUNCTION__);
    if (dataController_) {
        DataRestrictMode mode;
        mode.filterMode = DataRestrictModeType::DISABLE;
        if (!dataController_->sendSetDataRestrictMode(mode)) {
            LOG(ERROR, __FUNCTION__, " sendSetDataRestrictMode is failed");
        }
        return true;
    } else {
        LOG(ERROR, __FUNCTION__, " dataFilterController is not ready");
    }
    return false;
}

void NAOIpTrigger::onEventRejected(shared_ptr<Event> event, EventStatus reason) {
    LOG(DEBUG, __FUNCTION__, " reason = ", (int)reason);
    if (event->getTriggeredState() == TcuActivityState::SUSPEND
        && reason == EventStatus::REJECTED_INVALID_STATE_TRANSITION) {
        enableFilter();
    }
    if (event->getTriggeredState() == TcuActivityState::RESUME
        && reason == EventStatus::REJECTED_INVALID_STATE_TRANSITION) {
        disableFilter();
    }
}

void NAOIpTrigger::onEventProcessed(shared_ptr<Event> event, bool success) {
    LOG(DEBUG, __FUNCTION__);

    if (success) {
        if (event->getTriggeredState() == TcuActivityState::SUSPEND) {
            enableFilter();
        } else if (event->getTriggeredState() == TcuActivityState::RESUME) {
            disableFilter();
        }
    }
}

void NAOIpTrigger::triggerEvent(TcuActivityState eventState, std::string machineName) {
    LOG(DEBUG, __FUNCTION__);

    std::shared_ptr<Event> event = std::make_shared<Event>(eventState, machineName,
        TriggerType::NAOIP_TRIGGER);
    if ( event ) {
        if(eventManager_) {
            eventManager_->pushEvent(event);
        } else {
            LOG(ERROR, __FUNCTION__, "  event manager is not available ");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " unable to create event");
    }

}

bool NAOIpTrigger::validateTrigger(char *buffer, int length,
    TcuActivityState& tcuActivityState, std::string& machineName) {
    LOG(DEBUG, __FUNCTION__);
    string text(buffer, length);
    // to avoid \n in a string which might lead to not matching trigger text
    text.erase(std::remove(text.begin(), text.end(), '\n'), text.cend());
    LOG(DEBUG, __FUNCTION__, text);
    size_t deliminatorPosition = 0;
    if(( deliminatorPosition = text.find(MACHINE_NAME_DELIMINATOR)) != std::string::npos ) {
        machineName = text.substr(deliminatorPosition + sizeof(MACHINE_NAME_DELIMINATOR),
            text.length());
        text = text.substr(0, deliminatorPosition);
    }
    if (triggerText_.find(text) == triggerText_.end()) {
        LOG(ERROR, __FUNCTION__, " invalid trigger text, text = ", text);
    } else {
        LOG(INFO, __FUNCTION__, " valid trigger text, text = ", text);
        tcuActivityState = triggerText_[text];
        return true;
    }
    return false;
}

void NAOIpTrigger::listenNewTriggerClient(int triggerSocket, bool closeSocket) {
    LOG(DEBUG, __FUNCTION__);
    char buffer[BUFFER_SIZE] = {0};
    try{
        do {
            int length = read(triggerSocket, buffer, BUFFER_SIZE);
            LOG(DEBUG, __FUNCTION__, " buffer = ", buffer, "\nlength = ", length);
            if (length <= 0) {
                LOG(ERROR, __FUNCTION__, " trigger connection interrupted ");
                break;
            }
            TcuActivityState triggerState = TcuActivityState::UNKNOWN;
            std::string machineName = ALL_MACHINES;
            if (validateTrigger(buffer, length, triggerState, machineName)) {
                triggerEvent(triggerState, machineName);
            } else {
                LOG(ERROR, __FUNCTION__, " trigger not match ");
            }
            memset(buffer, 0, BUFFER_SIZE * (sizeof buffer[0]));
        } while (true);

        if (closeSocket) {
            if (shutdown(triggerSocket, SHUT_RDWR) == -1) {
                std::string logTmp = "shutdown failed errno = " + string(strerror(errno));
                LOG(ERROR, __FUNCTION__, logTmp);
            }
            if (close(triggerSocket) == -1) {
                std::string logTmp = "close failed errno = " + string(strerror(errno));
                LOG(ERROR, __FUNCTION__, logTmp);
            }
        }
    } catch(const std::exception& e) {
        LOG(ERROR, __FUNCTION__, "  exception ", string(e.what()));
    }

    LOG(DEBUG, __FUNCTION__, " exit ");
}

void NAOIpTrigger::startServer() {
    LOG(DEBUG, __FUNCTION__);

    if (isUDP_) {
        startUDPSever();
    } else {
        startTCPSever();
    }

    LOG(DEBUG, __FUNCTION__, " exit");
}

void NAOIpTrigger::startTCPSever() {
    LOG(DEBUG, __FUNCTION__);
    struct sockaddr_in address = {0};
    int opt = 1;
    int addrlen = sizeof(address);
    int port = DEFAULT_PORT;

    try {
        do {
            {
                std::lock_guard<std::mutex> serverUpdate(serverUpdate_);
                if (isServerRunning_) {
                    LOG(ERROR, __FUNCTION__, " server already running ");
                    break;
                }

                isServerRunning_ = true;
                if ((serverSocket_ = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
                    LOG(ERROR, __FUNCTION__, " socket failed return value = ", serverSocket_);
                    throw std::runtime_error("Error: socket failed");
                }

                int returnValue = 0;
                if ( (returnValue = setsockopt(serverSocket_, SOL_SOCKET,
                            SO_REUSEADDR | SO_REUSEPORT, &opt,
                            sizeof(opt)))) {
                    LOG(ERROR, __FUNCTION__, " setsockopt failed return value = ", returnValue);
                    throw std::runtime_error("Error: setsockopt failed");
                }
                address.sin_family = AF_INET;
                address.sin_addr.s_addr = INADDR_ANY;

                string portString = config_->getValue("NAOIP_TRIGGER", "NAOIP_FILTER_SERVER_PORT");
                if (portString != "") {
                    port = stoi(portString);
                }
                address.sin_port = htons(port);

                if ((returnValue = bind(serverSocket_, (struct sockaddr *)&address,
                        sizeof(address))) < 0) {
                    LOG(ERROR, __FUNCTION__, " bind failed return value = ", returnValue);
                    throw std::runtime_error("Error: bind failed");
                }
                if ((returnValue = listen(serverSocket_, 3)) < 0) {
                    LOG(ERROR, __FUNCTION__, " listen failed return value = ", returnValue);
                    throw std::runtime_error("Error: listen failed");
                }

            }

            do {
                int clientSocket = 0;
                LOG(INFO, __FUNCTION__, " server is accepting clients on port = ", port);
                if ((clientSocket = accept(serverSocket_, (struct sockaddr *)&address,
                                        (socklen_t *)&addrlen)) < 0) {
                    LOG(ERROR, __FUNCTION__, " accept failed return value = ", clientSocket);
                } else {
                    cleanOldDisconnectedClientThreads();
                    if (clientsSocketInfo_.size() <= MAX_CLIENT_CONNECT) {
                        std::promise<void> clientDisconnectedPromise;
                        ClientSocketInfo newClient = {0};
                        newClient.socketFd = clientSocket;
                        newClient.runningOnThread = std::thread(
                            [this, clientSocket, &clientDisconnectedPromise]{
                                listenNewTriggerClient(clientSocket, true);

                                clientDisconnectedPromise.set_value();
                            }
                        );
                        newClient.clientDisconnected = clientDisconnectedPromise.get_future();
                        clientsSocketInfo_.push_back(std::move(newClient));
                    } else {
                        if (close(clientSocket) == -1) {
                            LOG(ERROR, __FUNCTION__,
                                "close failed errno = ", string(strerror(errno)));
                        }
                        LOG(ERROR, __FUNCTION__, " max client limit reached ");
                    }
                }
            } while (isServerRunning_);
        } while (0);

    } catch(const std::exception& e) {
        LOG(ERROR, __FUNCTION__, "  exception ", string(e.what()));
        stopServer();
    }
    LOG(DEBUG, __FUNCTION__, " exit");
}

void NAOIpTrigger::startUDPSever() {
    LOG(DEBUG, __FUNCTION__);

    int ret;
    int port = DEFAULT_PORT;
    std::string portString;
    struct sockaddr_in serverAddr;

    {
        std::lock_guard<std::mutex> serverUpdate(serverUpdate_);
        if (isServerRunning_) {
            LOG(ERROR, __FUNCTION__, " server already running ");
            return;
        }
        isServerRunning_ = true;

        ret = socket(AF_INET, SOCK_DGRAM, 0);
        if (ret < 0) {
            LOG(ERROR, __FUNCTION__, " can't create socket, lnx err ", errno);
            stopServer();
        }

        serverSocket_ = ret;

        memset(&serverAddr, 0, sizeof(serverAddr));

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        portString = config_->getValue("NAOIP_TRIGGER", "NAOIP_FILTER_SERVER_PORT");
        if (portString != "") {
            port = std::stoi(portString);
        }
        serverAddr.sin_port = htons(port);

        ret = bind(serverSocket_, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (ret < 0) {
            LOG(ERROR, __FUNCTION__, " can't bind socket, lnx err ", errno);
            stopServer();
        }
    }

    listenNewTriggerClient(serverSocket_, false);
}

void NAOIpTrigger::cleanOldDisconnectedClientThreads() {
    LOG(DEBUG, __FUNCTION__);
    for (auto it = clientsSocketInfo_.begin(); it != clientsSocketInfo_.end(); it++) {
        if ((*it).clientDisconnected.wait_for(std::chrono::milliseconds(0)) ==
            std::future_status::ready && (*it).runningOnThread.joinable()) {
            (*it).runningOnThread.join();
            clientsSocketInfo_.erase(it--);
        }
    }
    LOG(DEBUG, __FUNCTION__, " exit");
}

void NAOIpTrigger::stopServer() {
    LOG(DEBUG, __FUNCTION__);
    try {
        std::lock_guard<std::mutex> serverUpdate(serverUpdate_);
        disableFilter();
        if (dataController_) {
            dataController_->removeAllFilter();
        }
        if (!isServerRunning_) {
            LOG(ERROR, __FUNCTION__, " server already stoped ");
            return;
        }
        for (auto it = clientsSocketInfo_.begin(); it != clientsSocketInfo_.end();it++) {
            if (shutdown((*it).socketFd, SHUT_RDWR) == -1) {
                std::string logTmp = "shutdown failed errno = " + string(strerror(errno));
                LOG(ERROR, __FUNCTION__, logTmp);
            }
            if (close((*it).socketFd) == -1) {
                std::string logTmp = "close failed errno = " + string(strerror(errno));
                LOG(ERROR, __FUNCTION__, logTmp);
            }
        }
        LOG(DEBUG, __FUNCTION__, " clients closed ");
        isServerRunning_ = false;
        if (shutdown(serverSocket_, SHUT_RD) == -1) {
            std::string logTmp = "shutdown failed errno = " + string(strerror(errno));
            LOG(ERROR, __FUNCTION__, logTmp);
        }
        if (close(serverSocket_) == -1) {
            std::string logTmp = "close failed errno = " + string(strerror(errno));
            LOG(ERROR, __FUNCTION__, logTmp);
        }

        serverSocket_ = 0;
        LOG(DEBUG, __FUNCTION__, " server closed ");

        for (auto it = clientsSocketInfo_.begin(); it != clientsSocketInfo_.end();) {
            // even after socket shutdown and close if client is connected it take some time to get
            // out of read operation
            if ((*it).clientDisconnected.wait_for(std::chrono::milliseconds(
                    1000)) == std::future_status::ready &&
                (*it).runningOnThread.joinable()) {
                (*it).runningOnThread.join();
            } else {
                LOG(ERROR, __FUNCTION__, " unable to join client thread ");
            }
            clientsSocketInfo_.erase(it);
        }
        LOG(DEBUG, __FUNCTION__, " clients joined ");
        if(server_.joinable()) {
            server_.join();
            LOG(DEBUG, __FUNCTION__, " server joined ");
        } else {
            LOG(ERROR, __FUNCTION__, " unable to join server thread ");
        }
    } catch(const std::exception& e) {
        LOG(ERROR, __FUNCTION__, "  exception ", string(e.what()));
    }
    LOG(DEBUG, __FUNCTION__," exit");
}

bool NAOIpTrigger::loadConfig() {
    LOG(DEBUG, __FUNCTION__);
    std::map<std::string, TcuActivityState> expectedTrigger{
        {TRIGGER_SUSPEND, TcuActivityState::SUSPEND},
        {TRIGGER_RESUME, TcuActivityState::RESUME},
        {TRIGGER_SHUTDOWN, TcuActivityState::SHUTDOWN}};
    try {
        std::string configTriggerText = "";
        for (auto itr = expectedTrigger.begin(); itr != expectedTrigger.end(); ++itr) {
            configTriggerText = config_->getValue("NAOIP_TRIGGER", itr->first);
            if (!configTriggerText.empty()) {
                if (triggerText_.find(configTriggerText) != triggerText_.end()) {
                    LOG(ERROR, __FUNCTION__, " Error : same trigger for multiple state");
                    return false;
                }
                triggerText_.insert({configTriggerText, itr->second});
            }
        }
    } catch (const std::invalid_argument& ia) {
        LOG(ERROR, __FUNCTION__, " Error : invalid argument");
        return false;
    }
    return true;
}
