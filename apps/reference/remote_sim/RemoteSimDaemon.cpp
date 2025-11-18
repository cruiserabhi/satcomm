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
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file    RemoteSimDaemon.cpp
 * @brief   This daemon interfaces with the modem to provide it with WWAN functionality
 *          from a remote SIM card. It accepts a TCP connection from a client that
 *          runs on a device with an attached SIM. This daemon passes data between
 *          the modem and the remote SIM client.
 */

#include <csignal>
#include <iostream>

extern "C" {
    #include <unistd.h>
}

#include "RemoteSimDaemon.hpp"
#include "RemoteSimUtils.hpp"

#define SUBSYSTEM_READY_TIMEOUT_SEC 5
#define APDU_MSG_HEADER_LENGTH 3

using namespace telux::common;
using namespace telux::tel;

RemoteSimDaemon & RemoteSimDaemon::getInstance()
{
    static RemoteSimDaemon instance;
    return instance;
}

int RemoteSimDaemon::runDaemon(int argc, char **argv)
{
    if (readArguments(argc, argv, slotId_) != Status::SUCCESS) {
        return EXIT_FAILURE;
    }

    std::signal(SIGHUP, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    if (initDaemon() != Status::SUCCESS) {
        RemoteSimDaemon::getInstance().deInit();
        return EXIT_FAILURE;
    }

    int bytes;
    uint8_t buf[BUF_LEN];
    std::time_t lastReceivedMsgTime = std::time(nullptr);

    while (1) {
        bool receivedMsg = simConnection_.handleClientConnection(lastReceivedMsgTime, buf, bytes);
        if (receivedMsg) {
            handleClientMsg(buf, bytes);
        }

        if (simConnection_.isClientConnected() && !modemConnected_) {
            notifyModemConnAvailable();
        } else if (!simConnection_.isClientConnected() && modemConnected_) {
            notifyModemConnUnavailable();
        }
    }

    return EXIT_SUCCESS;
}

void RemoteSimDaemon::notifyModemConnAvailable()
{
    if (!modemConnected_) {
        if (remoteSimMgr_->sendConnectionAvailable(eventCallback) != Status::SUCCESS) {
            LOGE("Failed to send connection available event request to the modem!\n");
        }
        modemConnected_ = true;
    }
}

void RemoteSimDaemon::notifyModemConnUnavailable()
{
    if (modemConnected_) {
        if (remoteSimMgr_->sendConnectionUnavailable(eventCallback) != Status::SUCCESS) {
            LOGE("Failed to send connection available event request to the modem!\n");
        }
        modemConnected_ = false;
    }
}

void RemoteSimDaemon::signalHandler(int signum)
{
    LOGI("Received signal %d, terminating program.\n", signum);
    RemoteSimDaemon::getInstance().deInit();
    exit(EXIT_SUCCESS);
}

void RemoteSimDaemon::eventCallback(ErrorCode errorCode)
{
    LOGD("Received event response with errorcode %d.\n", static_cast<int>(errorCode));
}

void RemoteSimDaemon::printUsage(char **argv)
{
    std::cout << std::endl;
    std::cout << "\tUsage: " << argv[0] << " -i <Slot Id> [-flag]" << std::endl;
    std::cout << std::endl;

    std::cout << "\t-i <Slot Id> \tThe slot Id of the DUT which needs to be bounded to remote SIM"
        << std::endl;
    std::cout << "\t-d \t\tEnables debug-level log messages" << std::endl;
    std::cout << "\t-s \t\tEnables the printing of log messages to console (instead of syslog)"
        << std::endl;
    std::cout << "\t-h \t\tPrints these usage instructions" << std::endl;
    std::cout << std::endl;
}

Status RemoteSimDaemon::readArguments(int argc, char **argv, int& slotId)
{
    while (1) {
        switch (getopt(argc, argv, "i:dsh")) {
            case -1:
                return Status::SUCCESS;
            case 'i':
                slotId = atoi(optarg);
                if (slotId <= 0) {
                    LOGE("Invalid slotId entered!\n");
                    return Status::FAILED;
                }
                continue;
            case 'd':
                RemoteSimUtils::enableDebug_ = true;
                continue;
            case 's':
                RemoteSimUtils::enableStdOut_ = true;
                continue;
            case 'h':
            default:
                printUsage(argv);
                return Status::FAILED;
        }
    }
}

Status RemoteSimDaemon::initDaemon()
{
    std::promise<telux::common::ServiceStatus> remoteSimMgrprom;
    remoteSimMgr_ = PhoneFactory::getInstance().getRemoteSimManager(slotId_,
        [&](telux::common::ServiceStatus status) {
        remoteSimMgrprom.set_value(status);
    });
    if (remoteSimMgr_ == nullptr) {
        LOGE("Failed to create RemoteSimManager!\n");
        return Status::FAILED;
    }
    telux::common::ServiceStatus remoteSimMgrStatus = remoteSimMgr_->getServiceStatus();
    if (remoteSimMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("Remote SIM subsystem not ready yet, waiting...\n");
    }
    remoteSimMgrStatus = remoteSimMgrprom.get_future().get();
    listener_ = std::make_shared<RemoteSimListener>();
    if (remoteSimMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("Remote SIM subsystem is ready now.\n");
        if (remoteSimMgr_->registerListener(listener_) != Status::SUCCESS) {
           LOGE("Listener registration failed!\n");
           return Status::FAILED;
        }
    } else {
        LOGE("RemoteSim subsystem failed to initialize.\n");
        return Status::FAILED;
    }

    if (simConnection_.setupDaemonSocket() != Status::SUCCESS) {
        LOGE("Failed to setup server socket!\n");
        return Status::FAILED;
    }
    simConnection_.acceptClientConnection();

    if (remoteSimMgr_->sendConnectionAvailable(eventCallback) != Status::SUCCESS) {
        LOGE("Failed to send connection available event request to the modem!\n");
        simConnection_.tearDownClientConnection();
        return Status::FAILED;
    }
    modemConnected_ = true;

    LOGD("Daemon initialized successfully.\n");
    return Status::SUCCESS;
}

void RemoteSimDaemon::deInit()
{
    notifyModemConnUnavailable();
    simConnection_.tearDownClientConnection();
    if (remoteSimMgr_) {
        remoteSimMgr_->deregisterListener(listener_);
    }
    remoteSimMgr_ = nullptr;
}

void RemoteSimDaemon::handleApduTransfer(uint8_t *buf, int bytes)
{
    if (bytes < APDU_MSG_HEADER_LENGTH) {
        LOGE("Client APDU transfer msg length is too short!\n");
        return;
    }

    uint8_t apduId = buf[1];

    if (buf[2] != bytes - APDU_MSG_HEADER_LENGTH) {
        LOGE("Client APDU transfer msg is malformed! Received %d data bytes out of %d!\n",
             bytes - APDU_MSG_HEADER_LENGTH, buf[2]);
    } else {
        std::vector<uint8_t> apdu;

        for (int i = APDU_MSG_HEADER_LENGTH; i < APDU_MSG_HEADER_LENGTH + buf[2]; i++) {
            apdu.push_back(buf[i]);
        }

        if (remoteSimMgr_->sendApdu(apduId, apdu, true, apdu.size(), 0, eventCallback)
            != Status::SUCCESS) {
            LOGE("Failed to send APDU transfer request to the modem!\n");
        }
    }
}

void RemoteSimDaemon::handleCardConnect(uint8_t *buf, int bytes)
{
    LOGD("Received Card Connect msg response from client.\n");
}

void RemoteSimDaemon::handleCardDisconnect(uint8_t *buf, int bytes)
{
    LOGD("Received Card Disconnect msg response from client.\n");
}

void RemoteSimDaemon::handleCardPowerUp(uint8_t *buf, int bytes)
{
    LOGD("Received Card Power Up msg response from client.\n");
}

void RemoteSimDaemon::handleCardPowerDown(uint8_t *buf, int bytes)
{
    LOGD("Received Card Power Down msg response from client.\n");
}

void RemoteSimDaemon::handleCardReset(uint8_t *buf, int bytes)
{
    LOGD("Received Card Reset msg response from client.\n");

    if (buf[1] != bytes - 2) {
        LOGE("Client Card Reset msg is malformed! Received %d AtR values out of %d!\n", bytes - 2,
             buf[1]);
    } else {
        std::vector<uint8_t> atr;

        for (int i = 2; i < 2 + buf[1]; i++) {
            atr.push_back(buf[i]);
        }

        if (remoteSimMgr_->sendCardReset(atr, eventCallback) != Status::SUCCESS) {
            LOGE("Failed to send card reset event request to the modem!\n");
        }
    }
}

void RemoteSimDaemon::handleCardInserted(uint8_t *buf, int bytes)
{
    LOGD("Received Card Inserted msg from client.\n");

    if (buf[1] != bytes - 2) {
        LOGE("Client Card Inserted msg is malformed! Received %d AtR values out of %d!\n",
             bytes - 2, buf[1]);
    } else {
        std::vector<uint8_t> atr;

        for (int i = 2; i < 2 + buf[1]; i++) {
            atr.push_back(buf[i]);
        }

        if (remoteSimMgr_->sendCardInserted(atr, eventCallback) != Status::SUCCESS) {
            LOGE("Failed to send card inserted event request to the modem!\n");
        }
    }
}

void RemoteSimDaemon::handleCardRemoved(uint8_t *buf, int bytes)
{
    LOGD("Received Card Removed msg from client.\n");

    if (remoteSimMgr_->sendCardRemoved(eventCallback) != Status::SUCCESS) {
        LOGE("Failed to send card removed event request to the modem!\n");
    }
}

void RemoteSimDaemon::handleConnKeepalive(uint8_t *buf, int bytes)
{
    LOGD("Received Connection Keepalive msg from client.\n");
}

void RemoteSimDaemon::handleClientMsg(uint8_t *buf, int bytes)
{
    switch (buf[0]) {
        case APDU_TRANSFER_MSG:
            handleApduTransfer(buf, bytes);
            break;
        case CARD_CONNECT_MSG:
            handleCardConnect(buf, bytes);
            break;
        case CARD_DISCONNECT_MSG:
            handleCardDisconnect(buf, bytes);
            break;
        case CARD_POWER_UP_MSG:
            handleCardPowerUp(buf, bytes);
            break;
        case CARD_POWER_DOWN_MSG:
            handleCardPowerDown(buf, bytes);
            break;
        case CARD_RESET_MSG:
            handleCardReset(buf, bytes);
            break;
        case CARD_INSERTED_MSG:
            handleCardInserted(buf, bytes);
            break;
        case CARD_REMOVED_MSG:
            handleCardRemoved(buf, bytes);
            break;
        case CONN_KEEPALIVE_MSG:
            handleConnKeepalive(buf, bytes);
            break;
        default:
            LOGE("Received unknown msg type from client!\n");
            break;
    }
}

void RemoteSimListener::onApduTransfer(const unsigned int id, const std::vector<uint8_t> &apdu) {
    LOGD("Received Apdu transfer notification from modem.\n");
    SimConnection::getInstance().sendApduRequest(id, apdu);
}

void RemoteSimListener::onCardConnect() {
    LOGD("Received Card Connect notification from modem.\n");
    SimConnection::getInstance().sendCardConnectRequest();
}

void RemoteSimListener::onCardDisconnect() {
    LOGD("Received Card Disconnect notification from modem.\n");
    SimConnection::getInstance().sendCardDisconnectRequest();
}

void RemoteSimListener::onCardPowerUp() {
    LOGD("Received Card Power Up notification from modem.\n");
    SimConnection::getInstance().sendCardPowerUpRequest();
}

void RemoteSimListener::onCardPowerDown() {
    LOGD("Received Card Power Down notification from modem.\n");
    SimConnection::getInstance().sendCardPowerDownRequest();
}

void RemoteSimListener::onCardReset() {
    LOGD("Received Card Reset notification from modem.\n");
    SimConnection::getInstance().sendCardResetRequest();
}

void RemoteSimListener::onServiceStatusChange(ServiceStatus status) {
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        LOGD("Received Service Unavailable notification.\n");
        RemoteSimDaemon::getInstance().notifyModemConnUnavailable();
    } else if (status == ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("Received Service Available notification.\n");
        if (SimConnection::getInstance().isClientConnected()) {
            RemoteSimDaemon::getInstance().notifyModemConnAvailable();
        }
    } else {
        LOGD("Received unknown service status notification.\n");
    }
}

int main(int argc, char **argv)
{
    return RemoteSimDaemon::getInstance().runDaemon(argc, argv);
}
