/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "IpFilterImpl.hpp"
#include "common/Logger.hpp"

namespace telux {
namespace data {

IpFilterImpl::IpFilterImpl(IpProtocol proto)
   : proto_(proto) {
    LOG(DEBUG, __FUNCTION__);
}

IpFilterImpl::~IpFilterImpl() {
    LOG(DEBUG, __FUNCTION__);
}

IPv4Info IpFilterImpl::getIPv4Info() {
    return ipv4Info_;
}

telux::common::Status IpFilterImpl::setIPv4Info(const IPv4Info &ipv4Info) {
    LOG(DEBUG, __FUNCTION__);
    ipv4Info_ = ipv4Info;
    return telux::common::Status::SUCCESS;
}

IPv6Info IpFilterImpl::getIPv6Info() {
    LOG(DEBUG, __FUNCTION__);
    return ipv6Info_;
}

telux::common::Status IpFilterImpl::setIPv6Info(const IPv6Info &ipv6Info) {
    LOG(DEBUG, __FUNCTION__);
    ipv6Info_ = ipv6Info;
    return telux::common::Status::SUCCESS;
}

IpProtocol IpFilterImpl::getIpProtocol() {
    LOG(DEBUG, __FUNCTION__);
    return proto_;
}

IpFamilyType IpFilterImpl::getIpFamily() {
    LOG(DEBUG, __FUNCTION__);
    return ipFamilyType_;
}

uint32_t IpFilterImpl::getFilterHandle() {
    LOG(DEBUG, __FUNCTION__);
    return filterHandle_;
}

telux::common::Status IpFilterImpl::setFilterHandle(uint32_t handle) {
    LOG(DEBUG, __FUNCTION__);
    filterHandle_ = handle;
    return telux::common::Status::SUCCESS;
}

UdpFilterImpl::UdpFilterImpl(IpProtocol proto)
   : IpFilterImpl(proto) {
    LOG(DEBUG, __FUNCTION__);
}

UdpFilterImpl::~UdpFilterImpl() {
    LOG(DEBUG, __FUNCTION__);
}

UdpInfo UdpFilterImpl::getUdpInfo() {
    LOG(DEBUG, __FUNCTION__);
    return udpInfo_;
}

telux::common::Status UdpFilterImpl::setUdpInfo(const UdpInfo &udpInfo) {
    LOG(DEBUG, __FUNCTION__);
    udpInfo_ = udpInfo;
    return telux::common::Status::SUCCESS;
}

TcpFilterImpl::TcpFilterImpl(IpProtocol proto)
   : IpFilterImpl(proto) {
    LOG(DEBUG, __FUNCTION__);
}

TcpFilterImpl::~TcpFilterImpl() {
    LOG(DEBUG, __FUNCTION__);
}

TcpInfo TcpFilterImpl::getTcpInfo() {
    LOG(DEBUG, __FUNCTION__);
    return tcpInfo_;
}

telux::common::Status TcpFilterImpl::setTcpInfo(const TcpInfo &tcpInfo) {
    LOG(DEBUG, __FUNCTION__);
    tcpInfo_ = tcpInfo;
    return telux::common::Status::SUCCESS;
}

IcmpFilterImpl::IcmpFilterImpl(IpProtocol proto)
   : IpFilterImpl(proto) {
    LOG(DEBUG, __FUNCTION__);
}

IcmpFilterImpl::~IcmpFilterImpl() {
    LOG(DEBUG, __FUNCTION__);
}

IcmpInfo IcmpFilterImpl::getIcmpInfo() {
    LOG(DEBUG, __FUNCTION__);
    return icmpInfo_;
}

telux::common::Status IcmpFilterImpl::setIcmpInfo(const IcmpInfo &icmpInfo) {
    LOG(DEBUG, __FUNCTION__);
    icmpInfo_ = icmpInfo;
    return telux::common::Status::SUCCESS;
}

EspFilterImpl::EspFilterImpl(IpProtocol proto)
   : IpFilterImpl(proto) {
    LOG(DEBUG, __FUNCTION__);
}

EspFilterImpl::~EspFilterImpl() {
    LOG(DEBUG, __FUNCTION__);
}

EspInfo EspFilterImpl::getEspInfo() {
    LOG(DEBUG, __FUNCTION__);
    return espInfo_;
}

telux::common::Status EspFilterImpl::setEspInfo(const EspInfo &espInfo) {
    LOG(DEBUG, __FUNCTION__);
    espInfo_ = espInfo;
    return telux::common::Status::SUCCESS;
}
}
}
