/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef TELUX_CV2XTXRXSOCKET_STUB_HPP
#define TELUX_CV2XTXRXSOCKET_STUB_HPP

#include <telux/cv2x/Cv2xTxRxSocket.hpp>

namespace telux {

namespace cv2x {

/**
 * This class encapsulates a CV2X socket associated with a Tx flow
 * and a Rx subscription.
 */
class Cv2xTxRxSocket : public ICv2xTxRxSocket {
 public:
    Cv2xTxRxSocket(uint32_t serviceId, int sock, const sockaddr_in6 &srcSockAddr, uint32_t flowId);

    virtual uint32_t getId() const;

    virtual uint32_t getServiceId() const;

    virtual int getSocket() const;

    virtual struct sockaddr_in6 getSocketAddr() const;

    virtual uint16_t getPortNum() const;

    virtual ~Cv2xTxRxSocket() {
    }

 protected:
    uint32_t id_;

    uint32_t serviceId_;

    int sock_ = -1;

    struct sockaddr_in6 sockAddr_ = {0};
};

}  // namespace cv2x

}  // namespace telux

#endif  // #ifndef TELUX_CV2XTXRXSOCKET_STUB_HPP
