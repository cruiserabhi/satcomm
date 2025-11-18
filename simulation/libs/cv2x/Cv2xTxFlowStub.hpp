/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_CV2XTXFLOW_STUB_HPP
#define TELUX_CV2XTXFLOW_STUB_HPP

#include <atomic>
#include <vector>

#include <telux/cv2x/Cv2xTxFlow.hpp>

namespace telux {

namespace cv2x {

enum class Cv2xTxFlowType {
    EVENT,
    SPS,
};

/**
 * This class encapsulates a V2X Tx Event Flow and is returned
 * to the client.
 */
class Cv2xTxEventFlow : public ICv2xTxFlow {
 public:
    Cv2xTxEventFlow(uint32_t id, TrafficIpType ipType, uint32_t serviceId, int sock,
        const sockaddr_in6 &sockAddr);

    virtual uint32_t getFlowId() const;

    virtual TrafficIpType getIpType() const;

    virtual uint32_t getServiceId() const;

    virtual int getSock() const;

    virtual struct sockaddr_in6 getSockAddr() const;

    virtual uint16_t getPortNum() const;

    virtual Cv2xTxFlowType getFlowType() const;

    virtual void closeSock();

    void setFlowInfo(const EventFlowInfo &flowInfo);

    virtual ~Cv2xTxEventFlow() {
    }

 protected:
    uint32_t id_;
    TrafficIpType ipType_;
    uint32_t serviceId_;
    int sock_                     = -1;
    struct sockaddr_in6 sockAddr_ = {0};
    EventFlowInfo flowInfo_;
};

/**
 * This class encapsulates a V2X Tx SPS Flow and is returned
 * to the client.
 */
class Cv2xTxSpsFlow : public Cv2xTxEventFlow {
 public:
    Cv2xTxSpsFlow(uint32_t id, TrafficIpType ipType, uint32_t serviceId, int sock,
        const sockaddr_in6 &sockAddr, const SpsFlowInfo &spsInfo);

    virtual Cv2xTxFlowType getFlowType() const;

    virtual ~Cv2xTxSpsFlow() {
    }

    SpsFlowInfo getSpsFlowInfo(void);

    void setSpsFlowInfo(const SpsFlowInfo &spsInfo);

 protected:
    uint32_t id_;
    TrafficIpType ipType_;
    uint32_t serviceId_;
    int sock_                     = -1;
    struct sockaddr_in6 sockAddr_ = {0};
    SpsFlowInfo spsInfo_;
};

}  // namespace cv2x

}  // namespace telux

#endif  // #ifndef TELUX_CV2XTXFLOW_STUB_HPP
