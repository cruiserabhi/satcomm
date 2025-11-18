/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    SimConnection.hpp
 * @brief   This file contains declarations for the SimConnection class, which
 *          is used by the Remote SIM Daemon and Sap Card Provider. This includes
 *          connection establishment/teardown and sending/receiving messages between
 *          both ends.
 */

#ifndef SIMCONNECTION_HPP
#define SIMCONNECTION_HPP

#include <ctime>
#include <vector>

extern "C" {
    #include <netinet/in.h>
    #include <poll.h>
    #include <sys/socket.h>
}

#include <telux/tel/PhoneFactory.hpp>

#define BUF_LEN 1024

#define APDU_TRANSFER_MSG 0
#define CARD_CONNECT_MSG 1
#define CARD_DISCONNECT_MSG 2
#define CARD_POWER_UP_MSG 3
#define CARD_POWER_DOWN_MSG 4
#define CARD_RESET_MSG 5
#define CARD_INSERTED_MSG 6
#define CARD_REMOVED_MSG 7
#define CONN_KEEPALIVE_MSG 8

class SimConnection {
public:
    static SimConnection &getInstance();

    telux::common::Status setupDaemonSocket();

    telux::common::Status setupClientConnection(const std::string &daemonIp);

    void acceptClientConnection();

    bool handleClientConnection(std::time_t &lastReceivedMsgTime, uint8_t *buf, int &bytes);

    bool handleDaemonConnection(std::time_t &lastReceivedMsgTime, uint8_t *buf, int &bytes);

    bool isClientConnected();

    bool isDaemonConnected();

    void tearDownClientConnection();

    void tearDownDaemonConnection();

    void sendApduRequest(const unsigned int id, const std::vector<uint8_t> &apdu);

    void sendCardConnectRequest();

    void sendCardDisconnectRequest();

    void sendCardPowerUpRequest();

    void sendCardPowerDownRequest();

    void sendCardResetRequest();

    void sendApduResponse(const std::vector<int> &data, uint8_t apduId);

    void sendCardResetResponse(const std::vector<int> &responseAtr);

    void sendCardInsertedMessage(const std::vector<int> &responseAtr);

    void sendCardRemovedMessage();

    void sendConnKeepaliveResponse();

private:
    void sendConnKeepaliveRequest();

    void sendRequest(const uint8_t *buf, const int msgLen);

    void sendResponse(const uint8_t *buf, const int msgLen);

    telux::common::Status connectToDaemon();

    void updateDaemonFds();

    void updateClientFds();

    struct sockaddr_in daemonAddr_;
    struct sockaddr_in clientAddr_;
    struct pollfd daemonFds_;
    struct pollfd clientFds_;
    int serverSocketFd_ = -1;
    int clientConnectionFd_ = -1;
    int daemonConnectionFd_ = -1;
    bool clientConnected_ = false;
    bool daemonConnected_ = false;
    bool pendingKeepalive_ = false;
};

#endif // SIMCONNECTION_HPP
