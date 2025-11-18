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
 * @file    SapCardProvider.cpp
 * @brief   This client connects to a remote SIM daemon via TCP and exchanges
 *          commands/data with it. It forwards this data to an onboard SIM
 *          card by using the SapCardManager API.
 */

#include <csignal>
#include <cstring>
#include <iostream>

extern "C" {
    #include <unistd.h>
}

#include "RemoteSimUtils.hpp"
#include "SapCardProvider.hpp"

using namespace telux::common;
using namespace telux::tel;

SapCardProvider & SapCardProvider::getInstance()
{
    static SapCardProvider instance;
    return instance;
}

int SapCardProvider::runClient(int argc, char **argv)
{
    std::signal(SIGHUP, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::string daemonIp;
    if (readArguments(argc, argv, daemonIp) != Status::SUCCESS) {
        return EXIT_FAILURE;
    }

    if (daemonIp.empty()) {
        printUsage(argv);
        return EXIT_FAILURE;
    }

    if (cardControl_.init() != Status::SUCCESS) {
        LOGE("Failed to initialize!\n");
        return EXIT_FAILURE;
    }

    if (simConnection_.setupClientConnection(daemonIp) != Status::SUCCESS) {
        LOGE("Failed to setup connection to daemon!\n");
        return EXIT_FAILURE;
    }

    int bytes;
    uint8_t buf[BUF_LEN];
    std::time_t lastReceivedMsgTime = std::time(nullptr);

    while (1) {
        bool msgReceived = simConnection_.handleDaemonConnection(lastReceivedMsgTime, buf, bytes);
        if (msgReceived) {
            handleDaemonMsg(buf, bytes);
        }
    }

    return EXIT_SUCCESS;
}

void SapCardProvider::signalHandler(int signum)
{
    LOGI("Received signal %d, terminating program.\n", signum);
    SapCardProvider::getInstance().cleanup();
    exit(EXIT_SUCCESS);
}

void SapCardProvider::cleanup()
{
    cardControl_.disconnectFromCard();
    simConnection_.tearDownDaemonConnection();
}

void SapCardProvider::printUsage(char **argv)
{
    std::cout << std::endl;
    std::cout << "\tUsage: " << argv[0] << " -i <Daemon IP address> [-flag]" << std::endl;
    std::cout << std::endl;

    std::cout << "\t-i <Daemon IP> \tThe IP address of the remote SIM daemon" << std::endl;
    std::cout << "\t-d \t\tEnables debug-level log messages" << std::endl;
    std::cout << "\t-s \t\tEnables the printing of log messages to console (instead of syslog)"
        << std::endl;
    std::cout << "\t-h \t\tPrints these usage instructions" << std::endl;
    std::cout << std::endl;
}

Status SapCardProvider::readArguments(int argc, char **argv, std::string &daemonIp)
{
    while (1) {
        switch (getopt(argc, argv, "i:dsh")) {
            case -1:
                return Status::SUCCESS;
            case 'i':
                daemonIp = optarg;
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

void SapCardProvider::handleDaemonMsg(uint8_t *buf, int bytes)
{
    switch (buf[0]) {
        case APDU_TRANSFER_MSG:
            cardControl_.sendApduToSim(buf, bytes);
            break;
        case CARD_CONNECT_MSG:
            cardControl_.connectToCard();
            break;
        case CARD_DISCONNECT_MSG:
            cardControl_.disconnectFromCard();
            break;
        case CARD_POWER_UP_MSG:
            cardControl_.powerUpCard();
            break;
        case CARD_POWER_DOWN_MSG:
            cardControl_.powerDownCard();
            break;
        case CARD_RESET_MSG:
            cardControl_.resetCard();
            break;
        case CONN_KEEPALIVE_MSG:
            simConnection_.sendConnKeepaliveResponse();
            break;
        default:
            LOGE("Received an unknown msg type from the daemon!\n");
            break;
    }
}

int main(int argc, char **argv)
{
    return SapCardProvider::getInstance().runClient(argc, argv);
}
