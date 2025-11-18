/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef IPFILTERIMPL_HPP
#define IPFILTERIMPL_HPP

#include <telux/data/IpFilter.hpp>

namespace telux {
namespace data {

class IpFilterImpl : virtual public IIpFilter {
 public:
    IpFilterImpl(IpProtocol proto);
    ~IpFilterImpl();

    /**
     * Get the IPv4 header info
     *
     * @returns @ref telux::data::IPv4Info
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    IPv4Info getIPv4Info() override;

    /**
     * sets the IPv4 header info
     *
     * @param [in] ipv4Info     IPv4 structure @ref telux::data::IPv4Info
     *
     * @returns Immediate status of setIPv4Info() request sent
     *                   i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    telux::common::Status setIPv4Info(const IPv4Info &ipv4Info) override;

    /**
     * Get the IPv6 header info
     *
     * @returns @ref telux::data::IPv6Info
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    IPv6Info getIPv6Info() override;

    /**
     * sets the IPv6 header info
     *
     * @param [in] ipv6Info     IPv6 structure @ref telux::data::IPv6Info
     *
     * @returns Immediate status of setIPv6Info() request sent
     *                   i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    telux::common::Status setIPv6Info(const IPv6Info &ipv6Info) override;

    /**
     * Get the IpProtocol Number
     *
     * @returns @ref telux::data::IpProtocol
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    IpProtocol getIpProtocol() override;

    /**
     * Get the IP family type
     *
     * @returns @ref telux::data::IpFamilyType
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    IpFamilyType getIpFamily() override;

    uint32_t getFilterHandle();
    telux::common::Status setFilterHandle(uint32_t handle);
    IpFamilyType ipFamilyType_ = IpFamilyType::UNKNOWN;

 private:
    IPv4Info ipv4Info_;
    IPv6Info ipv6Info_;
    uint32_t filterHandle_ = -1;
    IpProtocol proto_ = -1;
};

/**
 * @brief  This class represents a IP Filter for the UDP, get
 * the new instance from telux::data::DataFactory
 */
class UdpFilterImpl : public IUdpFilter, public IpFilterImpl {
 public:
    UdpFilterImpl(IpProtocol proto);
    ~UdpFilterImpl();
    /**
     * Get the UDP header info
     *
     * @returns @ref telux::data::UdpInfo
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    UdpInfo getUdpInfo() override;

    /**
     * sets the UDP header info
     *
     * @param [in] udpInfo     UdpInfo structure @ref telux::data::UdpInfo
     *
     * @returns Immediate status of setUdpInfo() request sent
     *                   i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    telux::common::Status setUdpInfo(const UdpInfo &udpInfo) override;

 private:
    UdpInfo udpInfo_;
};

/**
 * @brief  This class represents a IP Filter for the TCP, get
 * the new instance from telux::data::DataFactory
 */
class TcpFilterImpl : public ITcpFilter, public IpFilterImpl {
 public:
    TcpFilterImpl(IpProtocol proto);
    ~TcpFilterImpl();
    /**
     * Get the TCP header info
     *
     * @returns @ref telux::data::TcpInfo
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    TcpInfo getTcpInfo() override;

    /**
     * sets the TCP header info
     *
     * @param [in] tcpInfo     TcpInfo structure @ref telux::data::TcpInfo
     *
     * @returns Immediate status of setTcpInfo() request sent
     *                   i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    telux::common::Status setTcpInfo(const TcpInfo &tcpInfo) override;

 private:
    TcpInfo tcpInfo_;
};

/**
 * @brief  This class represents a IP Filter for the ICMP, get
 * the new instance from telux::data::DataFactory
 */
class IcmpFilterImpl : public IIcmpFilter, public IpFilterImpl {
 public:
    IcmpFilterImpl(IpProtocol proto);
    ~IcmpFilterImpl();
    /**
     * Get the ICMP header info
     *
     * @returns @ref telux::data::IcmpInfo
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    IcmpInfo getIcmpInfo() override;

    /**
     * sets the ICMP header info
     *
     * @param [in] icmpInfo     TcpInfo structure @ref telux::data::IcmpInfo
     *
     * @returns Immediate status of setIcmpInfo() request sent
     *                   i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    telux::common::Status setIcmpInfo(const IcmpInfo &icmpInfo) override;

 private:
    IcmpInfo icmpInfo_;
};

/**
 * @brief  This class represents a IP Filter for the ESP, get
 * the new instance from telux::data::DataFactory
 */
class EspFilterImpl : public IEspFilter, public IpFilterImpl {
 public:
    EspFilterImpl(IpProtocol proto);
    ~EspFilterImpl();
    /**
     * Get the ESP header info
     *
     * @returns @ref telux::data::EspInfo
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    EspInfo getEspInfo() override;

    /**
     * sets the ICMP header info
     *
     * @param [in] espInfo     EspInfo structure @ref telux::data::EspInfo
     *
     * @returns Immediate status of setEspInfo() request sent
     *                   i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backwards compatibility.
     */
    telux::common::Status setEspInfo(const EspInfo &espInfo) override;

 private:
    EspInfo espInfo_;
};
}
}

#endif