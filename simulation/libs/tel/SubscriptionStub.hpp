/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @file       SubscriptionStub.hpp
 *
 * @brief      Implementation of ISubscription
 *
 */

#ifndef SUBSCRIPTION_STUB_HPP
#define SUBSCRIPTION_STUB_HPP

#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Subscription.hpp>

namespace telux {
namespace tel {

class SubscriptionStub : public ISubscription {
public:
    SubscriptionStub(int slotId, std::string carrierName, std::string iccId, int mcc,
        int mnc, std::string number, std::string imsi, std::string gid1, std::string gid2);
    ~SubscriptionStub();
    std::string getCarrierName() override ;
    std::string getIccId() override;
    int getMcc() override;
    int getMnc() override;
    std::string getMobileCountryCode() override;
    std::string getMobileNetworkCode() override;
    std::string getPhoneNumber() override;
    int getSlotId() override;
    std::string getImsi() override;
    std::string getGID1() override;
    std::string getGID2() override;
    void updateSubscription(int slotId, std::string carrierName,
    std::string iccId, int mcc, int mnc, std::string number, std::string imsi, std::string gid1,
    std::string gid2);
    void cleanup();
private:
    int simSlotIndex_;
    std::string carrierName_;
    std::string iccId_;
    int mcc_;
    int mnc_;
    std::string number_;
    std::string imsi_;
    std::string gid1_;
    std::string gid2_;
};

} // end of namespace tel

} // end of namespace telux

#endif // SUBSCRIPTION_STUB_HPP