/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataCallStub.hpp"
#include "DataHelper.hpp"
#include "common/Logger.hpp"

using namespace telux::common;
using namespace std;

#define MAX_LTE_TX_RATE 1000
#define MAX_LTE_RX_RATE 1000
#define LTE_AVG_TX_RATE 500
#define LTE_AVG_RX_RATE 500

namespace telux {

namespace data {

DataCallStub::DataCallStub(std::string ifaceName) {
   LOG(DEBUG, __FUNCTION__);
   taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
   ifaceName_ = ifaceName;
}

DataCallStub::~DataCallStub() {
   LOG(DEBUG, __FUNCTION__);
}

const std::string &DataCallStub::getInterfaceName() {
    lock_guard<mutex> lock(statusMutex_);
    return ifaceName_;
}

DataBearerTechnology DataCallStub::getCurrentBearerTech() {
    lock_guard<mutex> lock(statusMutex_);
    return bearerTech_;
}

DataCallEndReason DataCallStub::getDataCallEndReason() {
    lock_guard<mutex> lock(statusMutex_);
    return endReason_;
}

DataCallStatus DataCallStub::getDataCallStatus() {
    lock_guard<mutex> lock(statusMutex_);
    return status_;
}

void DataCallStub::getDataCallStatus(DataCallStatus &ipv4, DataCallStatus &ipv6) const {
    lock_guard<mutex> lock(statusMutex_);
    ipv4 = ipv4Status_;
    ipv6 = ipv6Status_;
}

IpFamilyInfo DataCallStub::getIpv4Info() {
    IpFamilyInfo ipFamilyInfo{};
    lock_guard<mutex> lock(statusMutex_);
    ipFamilyInfo.status = ipv4Status_;
    for (auto &it : ipAddrList_) {
        if (DataHelper::isValidIpv4Address(it.ifAddress)) {
            ipFamilyInfo.addr = it;
            break;
        }
    }
    return ipFamilyInfo;
}

IpFamilyInfo DataCallStub::getIpv6Info() {
    IpFamilyInfo ipFamilyInfo{};
    lock_guard<mutex> lock(statusMutex_);
    ipFamilyInfo.status = ipv6Status_;
    for (auto &it : ipAddrList_) {
        if (DataHelper::isValidIpv6Address(it.ifAddress)) {
            ipFamilyInfo.addr = it;
            break;
        }
    }
    return ipFamilyInfo;
}

TechPreference DataCallStub::getTechPreference() {
    lock_guard<mutex> lock(statusMutex_);
    return techPref_;
}

std::list<IpAddrInfo> DataCallStub::getIpAddressInfo() {
    lock_guard<mutex> lock(statusMutex_);
    return ipAddrList_;
}

IpFamilyType DataCallStub::getIpFamilyType() {
    lock_guard<mutex> lock(statusMutex_);
    return family_;
}

int DataCallStub::getProfileId() {
    lock_guard<mutex> lock(statusMutex_);
    return profileId_;
}

SlotId DataCallStub::getSlotId() {
    lock_guard<mutex> lock(statusMutex_);
    return slotId_;
}

OperationType DataCallStub::getOperationType() {
    lock_guard<mutex> lock(statusMutex_);
    return operationType_;
}

telux::common::Status DataCallStub::requestDataCallStatistics(StatisticsResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ErrorCode error = ErrorCode::SUCCESS;
    lock_guard<mutex> lock(statusMutex_);
    DataCallStats stats;
    auto f = std::async(std::launch::async,
             [this, stats, error, callback]() {
                   callback(stats, error);
               }).share();
    taskQ_->add(f);
    return Status::SUCCESS;
}

telux::common::Status DataCallStub::resetDataCallStatistics(
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ErrorCode error = ErrorCode::SUCCESS;
    lock_guard<mutex> lock(statusMutex_);
    auto f = std::async(std::launch::async,
             [this, error, callback]() {
                   callback(error);
               }).share();
    taskQ_->add(f);
    return Status::SUCCESS;
}

telux::common::Status DataCallStub::requestTrafficFlowTemplate(IpFamilyType family,
        TrafficFlowTemplateCb callback) {
    LOG(ERROR, __FUNCTION__, "Not Supported");
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DataCallStub::requestDataCallBitRate(
    requestDataCallBitRateResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async,
             [this, callback]() {
                    BitRateInfo bitRate{};
                    bitRate.maxTxRate = MAX_LTE_TX_RATE;
                    bitRate.maxRxRate = MAX_LTE_RX_RATE;
                    bitRate.txRate = LTE_AVG_TX_RATE;
                    bitRate.rxRate = LTE_AVG_RX_RATE;
                    ErrorCode error = ErrorCode::SUCCESS;
                   callback(bitRate, error);
               }).share();
    taskQ_->add(f);
    return Status::SUCCESS;
}

void DataCallStub::setProfileId(int id) {
    lock_guard<mutex> lock(statusMutex_);
    profileId_ = id;
}

void DataCallStub::setSlotId(SlotId id) {
    lock_guard<mutex> lock(statusMutex_);
    slotId_ = id;
}

void DataCallStub::setIpFamilyType(IpFamilyType family) {
    lock_guard<mutex> lock(statusMutex_);
    family_ = family;
}

void DataCallStub::setIpAddrList(std::list<IpAddrInfo> ipAddrList) {
    lock_guard<mutex> lock(statusMutex_);
    ipAddrList_ = ipAddrList;
}

void DataCallStub::setTechPreference(TechPreference techPref) {
    lock_guard<mutex> lock(statusMutex_);
    techPref_ = techPref;
}

void DataCallStub::setDataCallStatus(DataCallStatus status) {
    setDataCallStatus(status, IpFamilyType::IPV4V6);
}

/*
 * This API derives the overall status by looking at the status for the individual IP family types.
 * Logic is encoded according to table below :
 * X = don't care
 * INVALID = Datacall was not requested for this IP family - initial value for both Ip families
 *           when datacall object is constructed is INVALID
 * --------------------|-------------------|---------------------|---------
 * |    IPv4 Status    |    IPv6 Status    |   Datacall Status   | Usecase
 * |-------------------|-------------------|---------------------|---------
 * |   NET_CONNECTED   |         X         |   NET_CONNECTED     |    1
 * |         X         |   NET_CONNECTED   |   NET_CONNECTED     |    2
 * |     NET_NO_NET    |     NET_NO_NET    |     NET_NO_NET      |    3
 * |     NET_NO_NET    |       INVALID     |     NET_NO_NET      |    4
 * |       INVALID     |     NET_NO_NET    |     NET_NO_NET      |    5
 * |  NET_CONNECTING   |   NET_CONNECTING  |   NET_CONNECTING    |    6
 * |     NET_NO_NET    |   NET_CONNECTING  |   NET_CONNECTING    |    7
 * |       INVALID     |   NET_CONNECTING  |   NET_CONNECTING    |    8
 * |  NET_CONNECTING   |     NET_NO_NET    |   NET_CONNECTING    |    9
 * |  NET_CONNECTING   |       INVALID     |   NET_CONNECTING    |    10
 * | NET_DISCONNECTING | NET_DISCONNECTING |  NET_DISCONNECTING  |    11
 * |     NET_NO_NET    | NET_DISCONNECTING |  NET_DISCONNECTING  |    12
 * |       INVALID     | NET_DISCONNECTING |  NET_DISCONNECTING  |    13
 * | NET_DISCONNECTING |     NET_NO_NET    |  NET_DISCONNECTING  |    14
 * | NET_DISCONNECTING |       INVALID     |  NET_DISCONNECTING  |    15
 * --------------------|-------------------|---------------------|---------
 */

void DataCallStub::setDataCallStatus(DataCallStatus status, IpFamilyType family) {
    //ToDo: Break up this function to simpler short functions
    LOG(DEBUG, __FUNCTION__);
    lock_guard<mutex> lock(statusMutex_);
    if(family == IpFamilyType::UNKNOWN) {
        LOG(DEBUG, __FUNCTION__, " invalid family ", static_cast<int>(family));
        return;
    }

    if((family == IpFamilyType::IPV4) || (family == IpFamilyType::IPV4V6)) {
        //If IP Family is in NET_NO_NET state and status is NET_DISCONNECTING, do not change it
        // it will show datacall disconnecting if though it is already discconeted
        if (!((ipv4Status_ == DataCallStatus::NET_NO_NET) &&
              (status == DataCallStatus::NET_DISCONNECTING))) {
                  ipv4Status_ = status;
              }
    }

    if((family == IpFamilyType::IPV6) || (family == IpFamilyType::IPV4V6)) {
        //If IP Family is in NET_NO_NET state and status is NET_DISCONNECTING, do not change it
        // it will show datacall disconnecting if though it is already discconeted
        if (!((ipv6Status_ == DataCallStatus::NET_NO_NET) &&
              (status == DataCallStatus::NET_DISCONNECTING))) {
                  ipv6Status_ = status;
              }
    }

    // Usecase 1 and 2
    if (ipv4Status_ == DataCallStatus::NET_CONNECTED ||
        ipv6Status_ == DataCallStatus::NET_CONNECTED) {
        status_ = DataCallStatus::NET_CONNECTED;
    }
    // Usecase 3, 4 and 5
    else if ((ipv4Status_ == DataCallStatus::NET_NO_NET || ipv4Status_ == DataCallStatus::INVALID)
        && (ipv6Status_ == DataCallStatus::NET_NO_NET || ipv6Status_ == DataCallStatus::INVALID)){
        status_ = DataCallStatus::NET_NO_NET;
    // Usecase 6, 7 and 8
    } else if ((ipv4Status_ == DataCallStatus::NET_NO_NET || ipv4Status_ == DataCallStatus::INVALID
        || ipv4Status_ == DataCallStatus::NET_CONNECTING) &&
        (ipv6Status_ == DataCallStatus::NET_CONNECTING )){
        status_ = DataCallStatus::NET_CONNECTING;
    // Usecase 9 and 10
    } else if ((ipv6Status_ == DataCallStatus::NET_NO_NET || ipv6Status_ == DataCallStatus::INVALID)
        && (ipv4Status_ == DataCallStatus::NET_CONNECTING )){
        status_ = DataCallStatus::NET_CONNECTING;
    // Usecase 11, 12 and 13
    } else if ((ipv4Status_ == DataCallStatus::NET_NO_NET || ipv4Status_ == DataCallStatus::INVALID
        || ipv4Status_ == DataCallStatus::NET_DISCONNECTING) &&
        (ipv6Status_ == DataCallStatus::NET_DISCONNECTING )){
        status_ = DataCallStatus::NET_DISCONNECTING;
    // Usecase 14 and 15
    } else if ((ipv6Status_ == DataCallStatus::NET_NO_NET || ipv6Status_ == DataCallStatus::INVALID)
        && (ipv4Status_ == DataCallStatus::NET_DISCONNECTING )){
        status_ = DataCallStatus::NET_DISCONNECTING;
    }
    LOG(DEBUG, __FUNCTION__, " family ", static_cast<int>(family),
        " status ", static_cast<int>(status), " status _ ", static_cast<int>(status_));
}

void DataCallStub::setDataCallEndReason(DataCallEndReason endReason) {
    LOG(DEBUG, __FUNCTION__);
    lock_guard<mutex> lock(statusMutex_);
    endReason_ = endReason;
}

void DataCallStub::setDataBearerTechnology(DataBearerTechnology bearerTech) {
    LOG(DEBUG, __FUNCTION__);
    lock_guard<mutex> lock(statusMutex_);
    bearerTech_ = bearerTech;
}

void DataCallStub::setInterfaceName(std::string ifName) {
    LOG(DEBUG, __FUNCTION__);
    lock_guard<mutex> lock(statusMutex_);
    ifaceName_ = ifName;
}

void DataCallStub::setOperationType(OperationType type) {
    LOG(DEBUG, __FUNCTION__);
    lock_guard<mutex> lock(statusMutex_);
    operationType_ = type;
}


} // end of namespace data

} // end of namespace telux