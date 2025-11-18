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

#ifndef NAOIPTRIGGER_HPP
#define NAOIPTRIGGER_HPP

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <map>
#include <memory>
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <string>
#include <chrono>

#include <telux/power/TcuActivityDefines.hpp>
#include <telux/common/Log.hpp>

#include "filter/DataFilterController.hpp"
#include "../../common/RefAppUtils.hpp"
#include "../../common/define.hpp"
#include "../../common/ConfigParser.hpp"
#include "../../Event.hpp"
#include "../../EventManager.hpp"
#include "../../IEventListener.hpp"

#define MAX_CLIENT_CONNECT  10
#define DEFAULT_PORT        8080
#define BUFFER_SIZE         1024
#define RETRY_INIT_SDK      3

typedef struct ClientSocketInfo{
  int socketFd;                             /** file descriptor for the client sockets */
  std::future<void> clientDisconnected;     /** future to clean the thread on connection close */
  std::thread runningOnThread;              /** thread used to read socket message */
} ClientSocketInfo;


/**
 * @brief NAOIpTrigger class watches for TCU state change events in IP packets. controls data
 * filtering according to triggered status.
 */

class NAOIpTrigger :    public IEventListener ,
                        public enable_shared_from_this<NAOIpTrigger> {
private:
    bool isUDP_ = false;
    bool isServerRunning_ = false;  /** server status listening client socket */
    std::mutex serverUpdate_ ;      /** required lock to update the server status from different
                                        threads */
    std::map<string, TcuActivityState> triggerText_;        /** map which stores trigger text with
                                                                respect to TcuActivityState */

    ConfigParser * config_;     /** config parser to fetch data from config file */
    std::shared_ptr<EventManager> eventManager_;            /** event management */
    std::shared_ptr<DataFilterController> dataController_;        /** controller for data call and data
                                                                filters */

    int serverSocket_ = 0;      /** file descriptor for the sever socket */
    std::vector<ClientSocketInfo> clientsSocketInfo_;
                    /** client listener threads connected to server */
    std::thread server_;        /** server thread accepting new clients */

    void startServer();
    void startTCPSever();
    void startUDPSever();
    void stopServer();
    void listenNewTriggerClient(int triggerSocket, bool closeSocket);
    void cleanOldDisconnectedClientThreads();
    bool validateTrigger(char* buffer, int length, TcuActivityState& tcuActivityState, std::string& machineName);
    bool loadConfig();
    void triggerEvent(TcuActivityState event, std::string machineName);
    bool enableFilter();
    bool disableFilter();

public:
    NAOIpTrigger(std::shared_ptr<EventManager> eventManager);
    bool init();
    void onEventRejected(shared_ptr<Event> event,EventStatus reason) override;
    void onEventProcessed(shared_ptr<Event> event,bool success) override;
    ~NAOIpTrigger();
};


#endif