/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <unistd.h>

#include "cv2x/Cv2xTxRxSocketStub.hpp"

namespace telux {

namespace cv2x {

Cv2xTxRxSocket::Cv2xTxRxSocket(
    uint32_t serviceId, int sock, const struct sockaddr_in6 &sockAddr, uint32_t flowId)
   : id_(flowId)
   , serviceId_(serviceId)
   , sock_(sock)
   , sockAddr_(sockAddr) {
}

uint32_t Cv2xTxRxSocket::getId() const {
    return id_;
}

uint32_t Cv2xTxRxSocket::getServiceId() const {
    return serviceId_;
}

int Cv2xTxRxSocket::getSocket() const {
    return sock_;
}

struct sockaddr_in6 Cv2xTxRxSocket::getSocketAddr() const {
    return sockAddr_;
}

uint16_t Cv2xTxRxSocket::getPortNum() const {
    return ntohs(sockAddr_.sin6_port);
}

}  // namespace cv2x

}  // namespace telux
