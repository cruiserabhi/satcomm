/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <unistd.h>

#include "Cv2xRxSubscriptionStub.hpp"

using std::atomic;

namespace telux {

namespace cv2x {

atomic<uint32_t> Cv2xRxSubscription::Id{0};

Cv2xRxSubscription::Cv2xRxSubscription(
    int sock, const struct sockaddr_in6 &sockAddr, TrafficIpType type,
    const std::shared_ptr<std::vector<uint32_t>> idList)
   : id_(Cv2xRxSubscription::Id++)
   , sock_(sock)
   , sockAddr_(sockAddr)
   , ipType_(type)
   , idList_(idList) {
}

uint32_t Cv2xRxSubscription::getSubscriptionId() const {
    return id_;
}

TrafficIpType Cv2xRxSubscription::getIpType() const {
    return ipType_;
}

int Cv2xRxSubscription::getSock() const {
    return sock_;
}

struct sockaddr_in6 Cv2xRxSubscription::getSockAddr() const {
    return sockAddr_;
}

uint16_t Cv2xRxSubscription::getPortNum() const {
    return ntohs(sockAddr_.sin6_port);
}

std::shared_ptr<std::vector<uint32_t>> Cv2xRxSubscription::getServiceIDList() const {
    return idList_;
}

void Cv2xRxSubscription::setServiceIDList(const std::shared_ptr<std::vector<uint32_t>> idList) {
    // deprecated, do nothing
}

void Cv2xRxSubscription::closeSock() {
    if (sock_ >= 0) {
        close(sock_);
    }
    sock_ = -1;
}

}  // namespace cv2x

}  // namespace telux
