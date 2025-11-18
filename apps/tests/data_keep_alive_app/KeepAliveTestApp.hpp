/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef KEEPALIVETESTAPP_HPP
#define KEEPALIVETESTAPP_HPP

#include <memory>

#include <telux/data/DataFactory.hpp>
#include "ConsoleApp.hpp"
#include "tcp_socket/TCPServer.hpp"
#include "tcp_socket/TCPClient.hpp"

#define APP_NAME "keepAlive_test_app"

using namespace telux::data;
using namespace telux::common;
using namespace tcp_test;

struct kaproto {
    char msg[1024];
};

void printMessage(kaproto &msg) {
    std::cout << msg.msg << std::endl;
}
namespace tcp_test {
    template <>
    class TCPClientWorker<kaproto>
    {
        public:
        TCPClientWorker() {}
        virtual void messageReceived(kaproto &msg) {
            std::cout << "Received: ";
            printMessage(msg);
        }
        virtual void onDisconnect() {std::cout << "disconnected\n";}
        virtual void onConnected() {
            std::cout << "connected\n";
        }
    };
    template <>
    class TCPServerWorker<kaproto>
    {

        public:
        TCPServerWorker() {}
        ~TCPServerWorker() { s_->disconnect(); }
        void setServer( std::shared_ptr<TCPServer<kaproto>> s) { s_ = s; }
        virtual void onAccept(std::string ip, int port) {
            std::cout << "Connected: " << ip << ":" << port << std::endl;
        }
        virtual void messageReceived(kaproto &msg) {
            std::cout << "Received: ";
            printMessage(msg);
        }
        virtual void onDisconnect() {std::cout << "disconnected\n";}

        private:
         std::shared_ptr<TCPServer<kaproto>> s_;
    };
}

class KeepAliveTestApp : public IKeepAliveListener,
                       public ConsoleApp,
                       public std::enable_shared_from_this<KeepAliveTestApp> {
public:

    KeepAliveTestApp();
    ~KeepAliveTestApp();

    void startTCPServer(std::vector<std::string> inputCommand);
    void stopTCPServer(std::vector<std::string> inputCommand);
    void startServerThread();

    void startTCPClient(std::vector<std::string> inputCommand);
    void stopTCPClient(std::vector<std::string> inputCommand);
    void startClientThread();

    void sendMessage(std::vector<std::string> inputCommand);
    void enableTCPMonitor(std::vector<std::string> inputCommand);
    void disableTCPMonitor(std::vector<std::string> inputCommand);
    void startTCPKeepAliveOffload(std::vector<std::string> inputCommand);
    void stopTCPKeepAliveOffload(std::vector<std::string> inputCommand);

    void onKeepAliveStatusChange(ErrorCode error, TCPKAOffloadHandle handle) override;
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void registerForUpdates();
    void deregisterForUpdates();
    void consoleInit(bool isServer);

    std::shared_ptr<telux::data::IKeepAliveManager> keepAliveMgr_;
private:

    KeepAliveTestApp(KeepAliveTestApp const &) = delete;
    KeepAliveTestApp &operator=(KeepAliveTestApp const &) = delete;

    void onTFTResponse(const std::vector<std::shared_ptr<TrafficFlowTemplate>> &tft,
        ErrorCode error);
    void logQosDetails(std::shared_ptr<TrafficFlowTemplate> &tft);
    void printFilterDetails(std::shared_ptr<telux::data::IIpFilter> filter);

    int profileId_;
    std::shared_ptr<TCPServer<kaproto>> server_;
    std::shared_ptr<TCPServerWorker<kaproto>> serverWorker_;

    std::shared_ptr<TCPClient<kaproto>> client_;
    std::shared_ptr<TCPClientWorker<kaproto>> clientWorker_;

    std::thread serverThread_;
    std::thread clientThread_;
    bool isServer_ = false;
};

#endif  // KEEPALIVETESTAPP_HPP
