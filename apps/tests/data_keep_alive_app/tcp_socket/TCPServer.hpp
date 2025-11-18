/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TEST_TCPSERVER_HPP
#define TEST_TCPSERVER_HPP

#include <arpa/inet.h>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>


namespace tcp_test {

template<typename T>
class TCPServerWorker {

    public:
    TCPServerWorker() {}
    virtual void onAccept(std::string) {}
    virtual void messageReceived(T &msg) {}
    virtual void onDisconnect() {}
};

template<typename T>
class TCPServer {

    public:
    TCPServer(std::shared_ptr<TCPServerWorker<T>> worker, int serverPort, std::string serverIpAddr)
    : serverPort_(serverPort), worker_(worker), strServerAddr_(serverIpAddr)
    {}

    ~TCPServer() { disconnect(); }

    bool startServer() {
        receivedStopServer_ = false;
        struct sockaddr *sockAddr = NULL;
        struct sockaddr *clientAddr = NULL;
        int reuse = 1;
        int bnd = 0;
        socklen_t sockSize = 0;
        int domain = 0;

        v4ServerAddr_ = {};
        v6ServerAddr_ = {};
        v4ClientAddr_ = {};
        v6ClientAddr_ = {};
        if (inet_pton(AF_INET, strServerAddr_.c_str(), &(v4ServerAddr_.sin_addr))) {
            domain = AF_INET;
            v4ServerAddr_.sin_family = AF_INET;
            v4ServerAddr_.sin_port = htons(serverPort_);
            sockAddr = reinterpret_cast<struct sockaddr*>(&v4ServerAddr_);
            clientAddr = reinterpret_cast<struct sockaddr*>(&v4ClientAddr_);
            sockSize = sizeof(sockaddr_in);
        } else if (inet_pton(AF_INET6, strServerAddr_.c_str(), &(v6ServerAddr_.sin6_addr))) {
            domain = AF_INET6;
            v6ServerAddr_.sin6_family = AF_INET6;
            v6ServerAddr_.sin6_port = htons(serverPort_);
            sockAddr = reinterpret_cast<struct sockaddr*>(&v6ServerAddr_);
            clientAddr = reinterpret_cast<struct sockaddr*>(&v6ClientAddr_);
            sockSize = sizeof(sockaddr_in6);
        } else {
            return false;
        }

        listenSocket_ = socket(domain, SOCK_STREAM, 0 );

        if (listenSocket_ == -1 ) {
            std::cout << " socket : "<< std::string(strerror(errno));
            return false;
        }
        setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        bnd = bind(listenSocket_, (struct sockaddr *)sockAddr, sockSize);
        if (bnd < 0 ) {
            std::cout << " bind : "<< std::string(strerror(errno));
            return false;
        }

        listen(listenSocket_, 1);
        do {
            socket_ = accept(listenSocket_, (struct sockaddr *)clientAddr, &sockSize);
            if (socket_ < 0 ) {
                std::cout << " accept : "<< std::string(strerror(errno));
                return false;
            }
            int no = 0;
            setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, &no, sizeof(int));

            char addrbuf[40];
            switch(clientAddr->sa_family) {
                case AF_INET:
                    inet_ntop(AF_INET, &(((struct sockaddr_in *)clientAddr)->sin_addr), addrbuf, 40);
                    clientPort_ = ntohs((((struct sockaddr_in *)clientAddr)->sin_port));
                    break;
                case AF_INET6:
                    inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)clientAddr)->sin6_addr), addrbuf, 40);
                    clientPort_ = ntohs((((struct sockaddr_in6 *)clientAddr)->sin6_port));
                    break;
                default:
                    return false;
            }
            strClientAddr_ = addrbuf;
            worker_->onAccept(strClientAddr_, clientPort_);
            while(!receivedStopServer_) {
                T msg;
                memset(&msg, 0, sizeof(msg));
                ssize_t n = recv(socket_, static_cast<void *>(&msg), sizeof(msg)-1, 0);
                std::cout <<" length : " << n << " ";
                if (n <= 0) {
                    std::cout << " recv : "<< std::string(strerror(errno));
                    worker_->onDisconnect();
                    break;
                }
                worker_->messageReceived(msg);
            }
        } while (!receivedStopServer_);

        return false;
    }

    void sendMessage(T *msg) {
        if(socket_) {
            if (send(socket_, static_cast<const void *>(msg), sizeof(T), 0) != sizeof(T)) {
                std::cout << " send : "<< std::string(strerror(errno));
                worker_->onDisconnect();
                close(socket_);
                socket_ = 0;
            }
        } else {
            std::cout << " Socket is not connected " << std::endl;
        }
    }

    void disconnect() {
        std::cout << " Stopping TCP Server " << std::endl;
        receivedStopServer_ = true;
        if(listenSocket_) {
            if(shutdown(listenSocket_, SHUT_RDWR) == -1) {
                std::cout << " shutdown " << std::string(strerror(errno))
                << std::endl;
            }
            if (close(listenSocket_) == -1) {
                std::cout << " close " << std::string(strerror(errno))
                << std::endl;
            }
        }

        if(socket_) {
            if (shutdown(socket_, SHUT_RDWR) == -1) {
                std::cout << "client  shutdown  "
                << std::string(strerror(errno))<< std::endl;
            }
            if (close(socket_) == -1) {
                std::cout << "client close " << std::string(strerror(errno))
                << std::endl;
            }
        }
        listenSocket_ = 0;
        socket_ = 0;
    }

    private:

    int serverPort_;
    int clientPort_;
    std::shared_ptr<TCPServerWorker<T>> worker_;
    std::string strServerAddr_;
    std::string strClientAddr_;
    struct sockaddr_in v4ServerAddr_;
    struct sockaddr_in6 v6ServerAddr_;
    struct sockaddr_in v4ClientAddr_;
    struct sockaddr_in6 v6ClientAddr_;
    int listenSocket_ = 0;
    int socket_ = 0;
    std::atomic<bool> receivedStopServer_ = {false};
};

};


#endif // TEST_TCPSERVER_HPP
