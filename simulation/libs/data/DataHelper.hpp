/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATAHELPER_HPP
#define DATAHELPER_HPP

#include <algorithm>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

#define PROTO_ICMP 1        // Internet Control Message Protocol - RFC 792
#define PROTO_ICMP6 58      // Internet Control Message Protocol - RFC 4443
#define PROTO_IGMP 2        // Internet Group Management Protocol - RFC 1112
#define PROTO_TCP  6        // Transmission Control Protocol - RFC 793
#define PROTO_UDP 17        // User Datagram Protocol - RFC 768
#define PROTO_ESP 50        // Encapsulating Security Payload - RFC 4303
#define PROTO_TCP_UDP 253   // Contain both TCP and UDP info

namespace telux {
namespace data {

class DataHelper {
 public:
    static std::string callEndReasonTypeToString(EndReasonType reasonType);
    static bool isValidIpv4Address(const std::string &addr);
    static bool isValidIpv6Address(const std::string &addr);
    static bool isValidProtocol(const IpProtocol &protocol) {
        std::vector<IpProtocol> protocolList { PROTO_ICMP, PROTO_ICMP6, PROTO_IGMP, PROTO_TCP,
            PROTO_UDP, PROTO_ESP, PROTO_TCP_UDP };
        if (std::find(protocolList.begin(), protocolList.end(), protocol) != protocolList.end()) {
            return true;
        }
        return false;
    }

    // Convert given string address into outAddress
    static int convertAddress(const std::string &inAddr, unsigned char *outAddr, int af);

    // convert a numeric address into a text string suitable  for presentation
    static int converAddrToString(int af, uint32_t *addr, char *str);
};

}
}

#endif
