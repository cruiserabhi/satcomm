/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TEST_TCPCLIENT_HPP
#define TEST_TCPCLIENT_HPP

#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <typeinfo>
#include <unistd.h>
#include <errno.h>
#include <memory>
#include <cstring>



namespace tcp_test {

template<typename T>
class TCPClientWorker {

    public:
    TCPClientWorker() {}
    virtual void onConnected() {}
    virtual void messageReceived(T &msg) {}
    virtual void onDisconnect() {}
};

template<typename T>
class TCPClient {

    public:
    TCPClient(std::shared_ptr<TCPClientWorker<T>> worker, int serverPort, std::string serverIpAddr,
    int clientPort = 0, std::string clientIpAddr = "") : worker_(worker), serverPort_(serverPort),
    strServerAddr_(serverIpAddr), clientPort_(clientPort), strClientAddr_(clientIpAddr)
    {}
    ~TCPClient() { disconnect(); }

    bool connectTo() {
        std::cout << " connectTo " << strServerAddr_ << ":" << serverPort_
        << "  via " << strClientAddr_ << ":" << clientPort_ << std::endl;
        receivedStopClient_ = false;
        int domain = 0;
        socklen_t sockSize = 0;
        struct sockaddr *sockAddrBind = NULL;
        struct sockaddr *sockAddrConnect = NULL;
        int reuse = 1;
        int bnd = 0;

        v4ServerAddr_ = {};
        v6ServerAddr_ = {};
        v4ClientAddr_ = {};
        v6ClientAddr_ = {};

        if(!strClientAddr_.empty()) {
            if (inet_pton(AF_INET, strClientAddr_.c_str(), &(v4ClientAddr_.sin_addr))) {
                v4ClientAddr_.sin_family = AF_INET;
                v4ClientAddr_.sin_port = htons(clientPort_);
                sockAddrBind = reinterpret_cast<struct sockaddr*>(&v4ClientAddr_);
                sockSize = sizeof(sockaddr_in);
            } else if (inet_pton(AF_INET6, strClientAddr_.c_str(), &(v6ClientAddr_.sin6_addr))) {
                v6ClientAddr_.sin6_family = AF_INET6;
                v6ClientAddr_.sin6_port = htons(clientPort_);
                sockAddrBind = reinterpret_cast<struct sockaddr*>(&v6ClientAddr_);
                sockSize = sizeof(sockaddr_in6);
            } else {
                return false;
            }
        }

        if (inet_pton(AF_INET, strServerAddr_.c_str(), &(v4ServerAddr_.sin_addr))) {
            domain = AF_INET;
            v4ServerAddr_.sin_family = AF_INET;
            v4ServerAddr_.sin_port = htons(serverPort_);
            sockAddrConnect = reinterpret_cast<struct sockaddr*>(&v4ServerAddr_);
            sockSize = sizeof(sockaddr_in);
        } else if (inet_pton(AF_INET6, strServerAddr_.c_str(), &(v6ServerAddr_.sin6_addr))) {
            domain = AF_INET6;
            v6ServerAddr_.sin6_family = AF_INET6;
            v6ServerAddr_.sin6_port = htons(serverPort_);
            sockAddrConnect = reinterpret_cast<struct sockaddr*>(&v6ServerAddr_);
            sockSize = sizeof(sockaddr_in6);
        } else {
            return false;
        }

        socket_ = socket(domain, SOCK_STREAM, 0);
        if ( socket_ < 0 ) {
            std::cout << " socket : " << std::string(strerror(errno)) ;
            return false;
        }
        if(sockAddrBind != NULL){
            setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
            setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
            bnd = bind(socket_, (struct sockaddr *)sockAddrBind, sockSize);
            if (bnd < 0 ) {
                std::cout << " bind : " << std::string(strerror(errno)) ;
                return false;
            }
        }

        if (connect(socket_, sockAddrConnect, sockSize) == -1 ) {
            std::cout << "connect : "<< std::string(strerror(errno));
            return false;
        }

        int no = 0;
        setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, &no, sizeof(int));
        worker_->onConnected();
        while(!receivedStopClient_) {
            T msg;
            memset(&msg, 0, sizeof(msg));
            ssize_t n = recv(socket_, static_cast<void *>(&msg), sizeof(msg), 0);
            std::cout <<" length : " << n << " ";
            if (n <= 0) {
                worker_->onDisconnect();
                break;
            }
            worker_->messageReceived(msg);
        }
        return true;
    }

    void disconnect() {
        std::cout << " Stopping TCP client " << std::endl;
        receivedStopClient_ = true;

        if(socket_) {
            if (shutdown(socket_, SHUT_RDWR) == -1) {
                std::cout << " shutdown : " << std::string(strerror(errno))<< std::endl;
            }
            if (close(socket_) == -1) {
                std::cout << " close : " << std::string(strerror(errno)) << std::endl;
            }
        }
        socket_ = 0;
    }

    void sendMessage(T *msg) {
        if(socket_) {
            if (send(socket_, static_cast<const void *>(msg), sizeof(T), 0) != sizeof(T)) {
                std::cout << " send : " << std::string(strerror(errno)) << std::endl;
                worker_->onDisconnect();
                close(socket_);
                socket_ = 0;
            }
        } else {
            std::cout << " Socket is not connected " << std::endl;
        }
    }

    private:
    std::shared_ptr<TCPClientWorker<T>> worker_;
    int serverPort_;
    std::string strServerAddr_;
    int clientPort_;
    std::string strClientAddr_;
    struct sockaddr_in v4ServerAddr_;
    struct sockaddr_in6 v6ServerAddr_;
    struct sockaddr_in v4ClientAddr_;
    struct sockaddr_in6 v6ClientAddr_;
    int socket_;
    std::atomic<bool> receivedStopClient_ = {false};
};
};
#endif // TEST_TCPCLIENT_HPP
