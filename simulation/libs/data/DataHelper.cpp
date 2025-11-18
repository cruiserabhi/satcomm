/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataHelper.hpp"
#include "common/Logger.hpp"

extern "C" {
#include <arpa/inet.h>
}

namespace telux {
namespace data {

std::string DataHelper::callEndReasonTypeToString(EndReasonType type) {
    switch (type) {
    case EndReasonType::CE_MOBILE_IP:
        return " CE_MOBILE_IP ";
    case EndReasonType::CE_INTERNAL:
        return " CE_INTERNAL ";
    case EndReasonType::CE_CALL_MANAGER_DEFINED:
        return " CE_CALL_MANAGER_DEFINED ";
    case EndReasonType::CE_3GPP_SPEC_DEFINED:
        return " CE_3GPP_SPEC_DEFINED ";
    case EndReasonType::CE_PPP:
        return " CE_PPP ";
    case EndReasonType::CE_EHRPD:
        return " CE_EHRPD ";
    case EndReasonType::CE_IPV6:
        return " CE_IPV6 ";
    case EndReasonType::CE_UNKNOWN:
        return " CE_UNKNOWN ";
    default: {
        LOG(ERROR, __FUNCTION__, " not a valid DataCallFailType");
        return " unable to map DataCallFailType ";
    }
    }
    return {};
}

bool DataHelper::isValidIpv4Address(const std::string &addr) {
    struct sockaddr_in sa;
    int res = inet_pton(AF_INET, addr.c_str(), &(sa.sin_addr));
    return res != 0;
}

bool DataHelper::isValidIpv6Address(const std::string &addr) {
    struct sockaddr_in6 sa;
    int res = inet_pton(AF_INET6, addr.c_str(), &(sa.sin6_addr));
    return res != 0;
}

int DataHelper::convertAddress(const std::string &inAddr, unsigned char *outAddr, int af) {
    int res = inet_pton(af, inAddr.c_str(), outAddr);

    if (res <= 0) {
        LOG(ERROR, " Address Invalid format");
        return -1;
    }
    return 0;
}

int DataHelper::converAddrToString(int af, uint32_t *addr, char *str) {
    if (inet_ntop(af, addr, str, INET6_ADDRSTRLEN) == NULL) {
        return -1;
    }
    return 0;
}

}
}