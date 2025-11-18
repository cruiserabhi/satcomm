/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_CALL_STUB_HPP
#define DATA_CALL_STUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <string>
#include <mutex>
#include <memory>

#include "common/AsyncTaskQueue.hpp"

using namespace telux::common;

namespace telux {

namespace data {

class DataCallStub: public IDataCall {
public:
    DataCallStub(std::string ifaceName);
    ~DataCallStub();
    const std::string &getInterfaceName() override;
    DataBearerTechnology getCurrentBearerTech() override;
    DataCallEndReason getDataCallEndReason() override;
    DataCallStatus getDataCallStatus() override;
    void getDataCallStatus(DataCallStatus &ipv4, DataCallStatus &ipv6) const;
    IpFamilyInfo getIpv4Info() override;
    IpFamilyInfo getIpv6Info() override;
    TechPreference getTechPreference() override;
    std::list<IpAddrInfo> getIpAddressInfo() override;
    IpFamilyType getIpFamilyType() override;
    int getProfileId() override;
    SlotId getSlotId() override;
    OperationType getOperationType() override;
    telux::common::Status requestDataCallStatistics(StatisticsResponseCb callback = nullptr)
        override;
    telux::common::Status resetDataCallStatistics(
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestTrafficFlowTemplate(IpFamilyType family,
        TrafficFlowTemplateCb callback) override;
    telux::common::Status requestDataCallBitRate(
        requestDataCallBitRateResponseCb callback) override;

    void setProfileId(int id);
    void setSlotId(SlotId id);
    void setIpFamilyType(IpFamilyType family);
    void setTechPreference(TechPreference techPref);
    void setDataCallStatus(DataCallStatus status);
    void setDataCallStatus(DataCallStatus status, IpFamilyType family);
    void setDataCallEndReason(DataCallEndReason endReason);
    void setDataBearerTechnology(DataBearerTechnology bearerTech);
    void setInterfaceName(std::string ifName);
    void setOperationType(OperationType type);
    void setIpAddrList(std::list<IpAddrInfo> ipAddrList);

private:
    std::string ifaceName_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    int profileId_;
    SlotId slotId_ = DEFAULT_SLOT_ID;
    IpFamilyType family_ = IpFamilyType::UNKNOWN;
    std::list<IpAddrInfo> ipAddrList_;
    TechPreference techPref_ = TechPreference::TP_ANY;
    DataCallStatus ipv4Status_ = DataCallStatus::INVALID;
    DataCallStatus ipv6Status_ = DataCallStatus::INVALID;
    DataCallStatus status_ = DataCallStatus::INVALID;
    DataCallEndReason endReason_;
    DataBearerTechnology bearerTech_ = DataBearerTechnology::UNKNOWN;
    OperationType operationType_ = OperationType::DATA_LOCAL;
    mutable std::mutex statusMutex_;
};

} // end of namespace data

} // end of namespace telux

#endif // DATA_CALL_STUB_HPP