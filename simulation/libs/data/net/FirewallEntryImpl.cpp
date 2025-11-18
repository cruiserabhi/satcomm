/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "FirewallEntryImpl.hpp"
#include "common/Logger.hpp"

namespace telux {
namespace data {
namespace net {

std::shared_ptr<IIpFilter> FirewallEntryImpl::getIProtocolFilter() {
    return ipFilter_;
}

telux::data::Direction FirewallEntryImpl::getDirection() {
    return direction_;
}

telux::data::IpFamilyType FirewallEntryImpl::getIpFamilyType() {
    return ipFamilyType_;
}

void FirewallEntryImpl::setHandle(uint32_t handle) {
    handle_ = handle;
}

uint32_t FirewallEntryImpl::getHandle() {
    return handle_;
}

FirewallEntryImpl::FirewallEntryImpl(
    std::shared_ptr<IIpFilter> ipfilter, Direction direction, IpFamilyType ipFamilyType)
   : ipFilter_(ipfilter)
   , direction_(direction)
   , ipFamilyType_(ipFamilyType) {
    LOG(DEBUG, __FUNCTION__);
    handle_ = INVALID_HANDLE;
}

FirewallEntryImpl::~FirewallEntryImpl() {
    LOG(DEBUG, __FUNCTION__);
}
}
}
}
