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
 * @file    SimConnection.cpp
 * @brief   This file contains the implementation for SIM Connection functions.
 */

#include <cstring>

extern "C" {
    #include <arpa/inet.h>
    #include <unistd.h>
}

#include "RemoteSimDaemon.hpp"
#include "RemoteSimUtils.hpp"
#include "SimConnection.hpp"

#define APDU_REQUEST_MSG_HEADER_LENGTH 2
#define APDU_RESPONSE_MSG_HEADER_LENGTH 3

#define PORT 8080
#define POLL_TIMEOUT_MS 1000

#define KEEPALIVE_MSG_INTERVAL_SEC 10
#define KEEPALIVE_MSG_TIMEOUT_SEC 13

using namespace telux::common;
using namespace telux::tel;

SimConnection & SimConnection::getInstance()
{
    static SimConnection instance;
    return instance;
}

Status SimConnection::setupDaemonSocket()
{
    if ((serverSocketFd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOGE("Failed to create socket! %s\n", strerror(errno));
        return Status::FAILED;
    }

    int optVal = 1;
    if (setsockopt(serverSocketFd_, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int)) < 0) {
        LOGE("Failed to set socket options! %s\n", strerror(errno));
        return Status::FAILED;
    }

    daemonAddr_.sin_family = AF_INET;
    daemonAddr_.sin_port = htons(PORT);
    daemonAddr_.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocketFd_, (struct sockaddr *)&daemonAddr_, sizeof(daemonAddr_)) < 0) {
        LOGE("Failed to bind socket! %s\n", strerror(errno));
        return Status::FAILED;
    }

    if (listen(serverSocketFd_, 1) < 0) {
        LOGE("Failed to listen on socket! %s\n", strerror(errno));
        close(serverSocketFd_);
        return Status::FAILED;
    }

    return Status::SUCCESS;
}

Status SimConnection::setupClientConnection(const std::string &daemonIp)
{
    clientAddr_.sin_family = AF_INET;
    clientAddr_.sin_port = htons(PORT);

    if (inet_aton(daemonIp.c_str(), (struct in_addr *)&(clientAddr_.sin_addr.s_addr)) == 0) {
        LOGE("Failed to read daemon IP address! Tried %s\n", daemonIp.c_str());
        return Status::FAILED;
    }

    while (connectToDaemon() != Status::SUCCESS) {
        LOGE("Connecting to daemon failed, attempting again...\n");
        sleep(5);
    }

    return Status::SUCCESS;
}

void SimConnection::acceptClientConnection()
{
    int addrLen = sizeof(daemonAddr_);

    while ((clientConnectionFd_ = accept(serverSocketFd_, (struct sockaddr *)&daemonAddr_,
                                         (socklen_t *)&addrLen)) < 0) {
        LOGE("Failed to accept a client connection! %s\n", strerror(errno));
    }

    clientConnected_ = true;
    updateDaemonFds();
    LOGD("Accepted client connection.\n");
}

bool SimConnection::handleClientConnection(std::time_t &lastReceivedMsgTime, uint8_t *buf,
                                           int &bytes)
{
    bool msgReceived = false;

    // If client is connected, poll connection file descriptor for activity.
    // If not, accept new client connection (blocking).
    if (clientConnected_) {
        if (poll(&daemonFds_, 1, POLL_TIMEOUT_MS) < 0) {
            LOGE("Poll() failed! %s\n", strerror(errno));
        }

        // Check for incoming data to read.
        if (daemonFds_.revents & POLLIN) {
            std::memset(buf, 0, BUF_LEN);
            if ((bytes = read(clientConnectionFd_, buf, BUF_LEN)) <= 0) {
                LOGE("Connection closed or failed to read from client! %s\n", strerror(errno));
                clientConnected_ = false;
            } else {
                msgReceived = true;
                if (pendingKeepalive_) {
                    pendingKeepalive_ = false;
                }
                lastReceivedMsgTime = std::time(nullptr);
            }
        }

        // Check for connection errors.
        if (clientConnected_ && daemonFds_.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            LOGE("Error reading from connection file descriptor, revents = %d.\n",
                 daemonFds_.revents);
            clientConnected_ = false;
        }

        // Send connection keepalive message if no message has been received from client for
        // KEEPALIVE_MSG_INTERVAL_SEC seconds.
        std::time_t currentTime = std::time(nullptr);
        if (clientConnected_ &&
            std::difftime(currentTime, lastReceivedMsgTime) > KEEPALIVE_MSG_INTERVAL_SEC) {
            if (!pendingKeepalive_) {
                pendingKeepalive_ = true;
                sendConnKeepaliveRequest();
            } else if (std::difftime(currentTime, lastReceivedMsgTime) >
                       KEEPALIVE_MSG_TIMEOUT_SEC) {
                LOGE("Client did not respond to keepalive message; disconnected.\n");
                clientConnected_ = false;
                pendingKeepalive_ = false;
            }
        }

        // Check if client has disconnected.
        if (!clientConnected_) {
            close(clientConnectionFd_);
            clientConnectionFd_ = -1;
            updateDaemonFds();
        }
    } else {
        acceptClientConnection();
        lastReceivedMsgTime = std::time(nullptr);
    }

    return msgReceived;
}

bool SimConnection::handleDaemonConnection(std::time_t &lastReceivedMsgTime, uint8_t *buf,
                                           int &bytes)
{
    bool msgReceived = false;

    // If daemon is connected, poll connection file descriptor for activity.
    // If not, attempt to connect to daemon until successful.
    if (daemonConnected_) {
        if (poll(&clientFds_, 1, POLL_TIMEOUT_MS) < 0) {
            LOGE("Poll() failed! %s\n", strerror(errno));
        }

        // Check for incoming data to read.
        if (clientFds_.revents & POLLIN) {
            std::memset(buf, 0, BUF_LEN);
            if ((bytes = read(daemonConnectionFd_, buf, BUF_LEN)) <= 0) {
                LOGE("Connection closed or failed to read from daemon! %s\n", strerror(errno));
                daemonConnected_ = false;
            } else {
                msgReceived = true;
                lastReceivedMsgTime = std::time(nullptr);
            }
        }

        // Check for connection errors.
        if (daemonConnected_ && clientFds_.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            LOGE("Error reading from connection file descriptor, revents = %d.\n",
                 clientFds_.revents);
            daemonConnected_ = false;
        }

        // Check if connection has timed out.
        std::time_t currentTime = std::time(nullptr);
        if (daemonConnected_ &&
            std::difftime(currentTime, lastReceivedMsgTime) > KEEPALIVE_MSG_TIMEOUT_SEC) {
            daemonConnected_ = false;
        }

        // Check if client has disconnected.
        if (!daemonConnected_) {
            close(daemonConnectionFd_);
            daemonConnectionFd_ = -1;
            updateClientFds();
        }
    } else {
        while (connectToDaemon() != Status::SUCCESS) {
            LOGE("Connecting to daemon failed, attempting again...\n");
            sleep(5);
        }
        lastReceivedMsgTime = std::time(nullptr);
    }

    return msgReceived;
}

bool SimConnection::isClientConnected()
{
    return clientConnected_;
}

bool SimConnection::isDaemonConnected()
{
    return daemonConnected_;
}

void SimConnection::tearDownClientConnection()
{
    if (clientConnected_) {
        close(clientConnectionFd_);
        clientConnectionFd_ = -1;
        clientConnected_ = false;
    }

    if (serverSocketFd_ != -1) {
        close(serverSocketFd_);
        serverSocketFd_ = -1;
    }
}

void SimConnection::tearDownDaemonConnection()
{
    if (daemonConnected_) {
        close(daemonConnectionFd_);
        daemonConnectionFd_ = -1;
        daemonConnected_ = false;
    }
}

void SimConnection::sendApduRequest(const unsigned int id, const std::vector<uint8_t> &apdu)
{
    int msgLen = APDU_REQUEST_MSG_HEADER_LENGTH + apdu.size();
    uint8_t buf[msgLen];

    buf[0] = APDU_TRANSFER_MSG;
    buf[1] = id;

    for (unsigned int i = 0; i < apdu.size(); i++) {
        buf[i + APDU_REQUEST_MSG_HEADER_LENGTH] = apdu[i];
    }

    sendRequest(buf, msgLen);
}

void SimConnection::sendCardConnectRequest()
{
    int msgLen = 1;
    uint8_t buf[] = {CARD_CONNECT_MSG};

    sendRequest(buf, msgLen);
}

void SimConnection::sendCardDisconnectRequest()
{
    int msgLen = 1;
    uint8_t buf[] = {CARD_DISCONNECT_MSG};

    sendRequest(buf, msgLen);
}

void SimConnection::sendCardPowerUpRequest()
{
    int msgLen = 1;
    uint8_t buf[] = {CARD_POWER_UP_MSG};

    sendRequest(buf, msgLen);
}

void SimConnection::sendCardPowerDownRequest()
{
    int msgLen = 1;
    uint8_t buf[] = {CARD_POWER_DOWN_MSG};

    sendRequest(buf, msgLen);
}

void SimConnection::sendCardResetRequest()
{
    int msgLen = 1;
    uint8_t buf[] = {CARD_RESET_MSG};

    sendRequest(buf, msgLen);
}

void SimConnection::sendApduResponse(const std::vector<int> &data, uint8_t apduId)
{
    int msgLen = APDU_RESPONSE_MSG_HEADER_LENGTH + data.size();
    uint8_t buf[msgLen];

    buf[0] = APDU_TRANSFER_MSG;
    buf[1] = apduId;
    buf[2] = data.size();

    for (unsigned int i = 0; i < data.size(); i++) {
        buf[i + APDU_RESPONSE_MSG_HEADER_LENGTH] = data[i];
    }

    sendResponse(buf, msgLen);
}

void SimConnection::sendCardResetResponse(const std::vector<int> &responseAtr)
{
    int msgLen = 2 + responseAtr.size();
    uint8_t buf[msgLen];

    buf[0] = CARD_RESET_MSG;
    buf[1] = responseAtr.size();

    for (unsigned int i = 0; i < responseAtr.size(); i++) {
        buf[i + 2] = responseAtr[i];
    }

    sendResponse(buf, msgLen);
}

void SimConnection::sendCardInsertedMessage(const std::vector<int> &responseAtr)
{
    int msgLen = 2 + responseAtr.size();
    uint8_t buf[msgLen];

    buf[0] = CARD_INSERTED_MSG;
    buf[1] = responseAtr.size();

    for (unsigned int i = 0; i < responseAtr.size(); i++) {
        buf[i + 2] = responseAtr[i];
    }

    sendResponse(buf, msgLen);
}

void SimConnection::sendCardRemovedMessage()
{
    int msgLen = 1;
    uint8_t buf[] = {CARD_REMOVED_MSG};

    sendResponse(buf, msgLen);
}

void SimConnection::sendConnKeepaliveResponse()
{
    int msgLen = 1;
    uint8_t buf[] = {CONN_KEEPALIVE_MSG};

    sendResponse(buf, msgLen);
}

void SimConnection::sendConnKeepaliveRequest()
{
    int msgLen = 1;
    uint8_t buf[] = {CONN_KEEPALIVE_MSG};

    sendRequest(buf, msgLen);
}

void SimConnection::sendRequest(const uint8_t *buf, const int msgLen)
{
    if (!clientConnected_) {
        LOGE("Client not connected, cannot send message!\n");
        return;
    }

    int bytes;
    if ((bytes = write(clientConnectionFd_, buf, msgLen)) < 0) {
        LOGE("Write to client failed! %s\n", strerror(errno));
    } else if (bytes != msgLen) {
        LOGE("Write to client only sent %d bytes out of %d!\n", bytes, msgLen);
    }
}

void SimConnection::sendResponse(const uint8_t *buf, const int msgLen)
{
    if (!daemonConnected_) {
        LOGE("Daemon not connected, cannot send message!\n");
        return;
    }

    int bytes;
    if ((bytes = write(daemonConnectionFd_, buf, msgLen)) < 0) {
        LOGE("Write to daemon failed! %s\n", strerror(errno));
    } else if (bytes != msgLen) {
        LOGE("Write to daemon only sent %d bytes out of %d!\n", bytes, msgLen);
    }
}

Status SimConnection::connectToDaemon()
{
    if ((daemonConnectionFd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOGE("Failed to create socket! %s\n", strerror(errno));
        return Status::FAILED;
    }

    if (connect(daemonConnectionFd_, (struct sockaddr *)&clientAddr_,
                sizeof(struct sockaddr_in)) < 0) {
        LOGE("Failed to connect to daemon! %s\n", strerror(errno));
        daemonConnectionFd_ = -1;
        return Status::FAILED;
    }

    LOGD("Connected to daemon.\n");
    daemonConnected_ = true;
    updateClientFds();

    return Status::SUCCESS;
}

void SimConnection::updateDaemonFds()
{
    daemonFds_.fd = clientConnectionFd_;
    daemonFds_.events = POLLIN;
}

void SimConnection::updateClientFds()
{
    clientFds_.fd = daemonConnectionFd_;
    clientFds_.events = POLLIN;
}
