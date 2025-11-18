/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <unistd.h>

#include "cv2x/Cv2xTxFlowStub.hpp"

using std::atomic;

namespace telux {

namespace cv2x {

Cv2xTxEventFlow::Cv2xTxEventFlow(
    uint32_t id, TrafficIpType ipType, uint32_t serviceId, int sock, const sockaddr_in6 &sockAddr)
   : id_(id)
   , ipType_(ipType)
   , serviceId_(serviceId)
   , sock_(sock)
   , sockAddr_(sockAddr) {
}

uint32_t Cv2xTxEventFlow::getFlowId() const {
    return id_;
}

TrafficIpType Cv2xTxEventFlow::getIpType() const {
    return ipType_;
}

uint32_t Cv2xTxEventFlow::getServiceId() const {
    return serviceId_;
}

int Cv2xTxEventFlow::getSock() const {
    return sock_;
}

struct sockaddr_in6 Cv2xTxEventFlow::getSockAddr() const {
    return sockAddr_;
}

uint16_t Cv2xTxEventFlow::getPortNum() const {
    return ntohs(sockAddr_.sin6_port);
}

Cv2xTxFlowType Cv2xTxEventFlow::getFlowType() const {
    return Cv2xTxFlowType::EVENT;
}

void Cv2xTxEventFlow::closeSock() {
    if (sock_ >= 0) {
        close(sock_);
    }
    sock_ = -1;
}
void Cv2xTxEventFlow::setFlowInfo(const EventFlowInfo &flowInfo) {
    flowInfo_ = flowInfo;
}

// Cv2xTxSpsFlow methods
Cv2xTxSpsFlow::Cv2xTxSpsFlow(uint32_t id, TrafficIpType ipType, uint32_t serviceId, int sock,
    const sockaddr_in6 &sockAddr, const SpsFlowInfo &spsInfo)
   : Cv2xTxEventFlow(id, ipType, serviceId, sock, sockAddr) {
    EventFlowInfo flowInfo;
    flowInfo.autoRetransEnabledValid = false;
    flowInfo.peakTxPowerValid        = false;
    flowInfo.mcsIndexValid           = false;
    flowInfo.txPoolIdValid           = false;

    setFlowInfo(flowInfo);
    spsInfo_ = spsInfo;
}

Cv2xTxFlowType Cv2xTxSpsFlow::getFlowType() const {
    return Cv2xTxFlowType::SPS;
}

void Cv2xTxSpsFlow::setSpsFlowInfo(const SpsFlowInfo &spsInfo) {
    spsInfo_ = spsInfo;
}

SpsFlowInfo Cv2xTxSpsFlow::getSpsFlowInfo(void) {
    return spsInfo_;
}

}  // namespace cv2x

}  // namespace telux
