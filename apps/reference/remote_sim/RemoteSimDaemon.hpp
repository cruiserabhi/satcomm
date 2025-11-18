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
 * @file    RemoteSimDaemon.hpp
 * @brief   This file contains class and function declarations for the Remote SIM Daemon.
 *          The program can be executed with the '-s' flag to print log messages to the
 *          console (instead of syslog), and the '-d' flag can be used to enable debug-
 *          level log messages. Once the daemon is running, it can accept a connection
 *          from the Sap Card Provider (running on another device with a SIM card). The
 *          modem will then be able to make use of the SIM on the remote device (as long
 *          as the modem has the Remote SIM service enabled).
 */

#ifndef REMOTESIMDAEMON_HPP
#define REMOTESIMDAEMON_HPP

#include <telux/tel/PhoneFactory.hpp>

#include "SimConnection.hpp"

class RemoteSimDaemon {
public:
    static RemoteSimDaemon &getInstance();

    int runDaemon(int argc, char **argv);

    void notifyModemConnAvailable();

    void notifyModemConnUnavailable();

private:
    static void signalHandler(int signum);

    static void eventCallback(telux::common::ErrorCode errorCode);

    void printUsage(char **argv);

    telux::common::Status readArguments(int argc, char **argv, int& slotId);

    telux::common::Status initDaemon();

    void deInit();

    void handleApduTransfer(uint8_t *buf, int bytes);

    void handleCardConnect(uint8_t *buf, int bytes);

    void handleCardDisconnect(uint8_t *buf, int bytes);

    void handleCardPowerUp(uint8_t *buf, int bytes);

    void handleCardPowerDown(uint8_t *buf, int bytes);

    void handleCardReset(uint8_t *buf, int bytes);

    void handleCardInserted(uint8_t *buf, int bytes);

    void handleCardRemoved(uint8_t *buf, int bytes);

    void handleConnKeepalive(uint8_t *buf, int bytes);

    void handleClientMsg(uint8_t *buf, int bytes);

    std::shared_ptr<telux::tel::IRemoteSimManager> remoteSimMgr_;
    std::shared_ptr<telux::tel::IRemoteSimListener> listener_;
    SimConnection &simConnection_ = SimConnection::getInstance();
    bool modemConnected_ = false;
    int slotId_ = DEFAULT_SLOT_ID;
};

class RemoteSimListener : public telux::tel::IRemoteSimListener {
public:
    void onApduTransfer(const unsigned int id, const std::vector<uint8_t> &apdu) override;

    void onCardConnect() override;

    void onCardDisconnect() override;

    void onCardPowerUp() override;

    void onCardPowerDown() override;

    void onCardReset() override;

    void onServiceStatusChange(telux::common::ServiceStatus status) override;
};

#endif // REMOTESIMDAEMON_HPP
