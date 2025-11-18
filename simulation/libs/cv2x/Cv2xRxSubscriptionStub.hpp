/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_CV2XRXSUBSCRIPTION_STUB_HPP
#define TELUX_CV2XRXSUBSCRIPTION_STUB_HPP

#include <atomic>
#include <memory>
#include <vector>

#include <telux/cv2x/Cv2xRxSubscription.hpp>

namespace telux {

namespace cv2x {

class Cv2xRxSubscription : public ICv2xRxSubscription {
 public:
    Cv2xRxSubscription(int sock, const struct sockaddr_in6 &sockAddr, TrafficIpType type,
        const std::shared_ptr<std::vector<uint32_t>> idList);

    virtual uint32_t getSubscriptionId() const;

    virtual TrafficIpType getIpType() const;

    virtual int getSock() const;

    virtual struct sockaddr_in6 getSockAddr() const;

    virtual uint16_t getPortNum() const;

    virtual void closeSock();

    virtual std::shared_ptr<std::vector<uint32_t>> getServiceIDList() const;

    virtual ~Cv2xRxSubscription() {
    }

    // deprecated
    virtual void setServiceIDList(const std::shared_ptr<std::vector<uint32_t>> idList);

 protected:
    static std::atomic<uint32_t> Id;

    uint32_t id_;
    int sock_;
    struct sockaddr_in6 sockAddr_;
    TrafficIpType ipType_;
    std::shared_ptr<std::vector<uint32_t>> idList_ = nullptr;
};

}  // namespace cv2x

}  // namespace telux

#endif  // #ifndef TELUX_CV2XRXSUBSCRIPTION_STUB_HPP
