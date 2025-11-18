/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "TrafficFilterImpl.hpp"
#include <iostream>
#include <sstream>

namespace telux {
namespace data {

std::string TrafficFilterImpl::getIPv4Address(FieldType fieldType) {
    if (fieldType == FieldType::SOURCE) {
        return sourceIPv4Address_;
    } else {
        return destIPv4Address_;
    }
}

void TrafficFilterImpl::setIPv4Address(std::string ipv4Addr, FieldType fieldType) {
    if (fieldType == FieldType::SOURCE) {
        validityMask_ = validityMask_ | TrafficFilterValidField::TF_SOURCE_IPV4_ADDRESS_VALID;
        sourceIPv4Address_ = ipv4Addr;
    } else {
        validityMask_ = validityMask_ | TrafficFilterValidField::TF_DESTINATION_IPV4_ADDRESS_VALID;
        destIPv4Address_ = ipv4Addr;
    }
}

std::string TrafficFilterImpl::getIPv6Address(FieldType fieldType) {
    if (fieldType == FieldType::SOURCE) {
        return sourceIPv6Address_;
    } else {
        return destIPv6Address_;
    }
}
void TrafficFilterImpl::setIPv6Address(std::string ipv6Addr, FieldType fieldType) {
    if (fieldType == FieldType::SOURCE) {
        validityMask_ = validityMask_ | TrafficFilterValidField::TF_SOURCE_IPV6_ADDRESS_VALID;
        sourceIPv6Address_ = ipv6Addr;
    } else {
        validityMask_ = validityMask_ | TrafficFilterValidField::TF_DESTINATION_IPV6_ADDRESS_VALID;
        destIPv6Address_ = ipv6Addr;
    }
}

uint16_t TrafficFilterImpl::getPort(FieldType fieldType) {
    if (fieldType == FieldType::SOURCE) {
        return sourcePort_;
    } else {
        return destPort_;
    }
}
void TrafficFilterImpl::setPort(uint16_t port, FieldType fieldType) {
    if (fieldType == FieldType::SOURCE) {
        validityMask_ = validityMask_ | TrafficFilterValidField::TF_SOURCE_PORT_VALID;
        sourcePort_ = port;
    } else {
        validityMask_ = validityMask_ | TrafficFilterValidField::TF_DESTINATION_PORT_VALID;
        destPort_ = port;
    }
}

void TrafficFilterImpl::getPortRange(FieldType fieldType, uint16_t &startPort, uint16_t &range) {
    if (fieldType == FieldType::SOURCE) {
        startPort = sourceStartPort_;
        range     = sourcePortRange_;
        return;
    } else {
        startPort = destStartPort_;
        range     = destPortRange_;
        return;
    }
}

void TrafficFilterImpl::setPortRange(uint16_t startPort, uint16_t range, FieldType fieldType) {
    if (fieldType == FieldType::SOURCE) {
        validityMask_    = validityMask_ | TrafficFilterValidField::TF_SOURCE_PORT_RANGE_VALID;
        sourceStartPort_ = startPort;
        sourcePortRange_ = range;
    } else {
        validityMask_  = validityMask_ | TrafficFilterValidField::TF_DESTINATION_PORT_RANGE_VALID;
        destStartPort_ = startPort;
        destPortRange_ = range;
    }
}

std::vector<int> TrafficFilterImpl::getVlanList(FieldType fieldType) {
    if (fieldType == FieldType::SOURCE) {
        return sourceVlanList_;
    } else {
        return destVlanList_;
    }
}
void TrafficFilterImpl::setVlanList(std::vector<int> vlanList, FieldType fieldType) {
    if (fieldType == FieldType::SOURCE) {
        validityMask_ = validityMask_ | TrafficFilterValidField::TF_SOURCE_VLAN_LIST_VALID;
        sourceVlanList_ = vlanList;
    } else {
        validityMask_ = validityMask_ | TrafficFilterValidField::TF_DESTINATION_VLAN_LIST_VALID;
        destVlanList_ = vlanList;
    }
}

IpProtocol TrafficFilterImpl::getIPProtocol() {
    return ipProtocol_;
}
void TrafficFilterImpl::setIPProtocol(IpProtocol ipProtocol) {
    validityMask_ = validityMask_ | TrafficFilterValidField::TF_IP_PROTOCOL_VALID;
    ipProtocol_ = ipProtocol;
}

Direction TrafficFilterImpl::getDirection() {
    return direction_;
}
void TrafficFilterImpl::setDirection(Direction direction) {
    validityMask_ = validityMask_ | TrafficFilterValidField::TF_DIRECTION_VALID;
    direction_ = direction;
}

int8_t TrafficFilterImpl::getPCP() {
    return pcp_;
}
void TrafficFilterImpl::setPCP(int8_t pcp) {
    validityMask_ = validityMask_ | TrafficFilterValidField::TF_PCP_VALID;
    pcp_ = pcp;
}


DataPath TrafficFilterImpl::getDataPath() {
    return dataPath_;
}
void TrafficFilterImpl::setDataPath(DataPath dataPath) {
    validityMask_ = validityMask_ | TrafficFilterValidField::TF_DATA_PATH_VALID;
    dataPath_ = dataPath;
}

TrafficFilterValidFields TrafficFilterImpl::getTrafficFilterValidFields() {
    return validityMask_;
}

TrafficFilterImpl::~TrafficFilterImpl() {
}

std::string TrafficFilterImpl::toString() {
    std::stringstream outStr;

    if (validityMask_ & TF_IP_PROTOCOL_VALID) {
        outStr << " ipProtocol: " << static_cast<int>(ipProtocol_) << std::endl;
    }
    if (validityMask_ & TF_DIRECTION_VALID) {
        outStr << " direction: " << TrafficFilterImpl::directionToString(direction_) << std::endl;
    }
    if (validityMask_ & TF_DATA_PATH_VALID) {
        outStr << " Data path: " << TrafficFilterImpl::dataPathToString(dataPath_) << std::endl;
    }
    if (validityMask_ & TF_PCP_VALID) {
        outStr << " PCP: " << static_cast<int>(pcp_) << std::endl;
    }
    if (validityMask_ & TF_SOURCE_IPV4_ADDRESS_VALID) {
        outStr << " sourceIPv4Address: " << sourceIPv4Address_ << std::endl;
    }
    if (validityMask_ & TF_SOURCE_IPV6_ADDRESS_VALID) {
        outStr << " sourceIPv6Address: " << sourceIPv6Address_ << std::endl;
    }
    if (validityMask_ & TF_SOURCE_PORT_VALID) {
        outStr << " sourcePorts: " << sourcePort_ << std::endl;
    }
    if (validityMask_ & TF_SOURCE_VLAN_LIST_VALID) {
        outStr << " sourceVlanList: ";
        for (size_t i = 0; i < sourceVlanList_.size(); i++) {
            outStr << " " << sourceVlanList_[i];
        }
        outStr << std::endl;
    }

    if (validityMask_ & TF_DESTINATION_IPV4_ADDRESS_VALID) {
        outStr << " destIPv4Address: " << destIPv4Address_ << std::endl;
    }
    if (validityMask_ & TF_DESTINATION_IPV6_ADDRESS_VALID) {
        outStr << " destIPv6Address: " << destIPv6Address_ << std::endl;
    }
    if (validityMask_ & TF_DESTINATION_PORT_VALID) {
        outStr << " destPorts: " << destPort_ << std::endl;
    }
    if (validityMask_ & TF_DESTINATION_VLAN_LIST_VALID) {
        outStr << " destVlanList: ";
        for (size_t i = 0; i < destVlanList_.size(); i++) {
            outStr << " " << destVlanList_[i];
        }
        outStr << std::endl;
    }
    return outStr.str();
}

std::string TrafficFilterImpl::dataPathToString(DataPath dataPath) {
    std::string dataPathStr = "";
    switch (dataPath) {
        case DataPath::TETHERED_TO_WAN_HW:
            dataPathStr = "TETHERED_TO_WAN_HW";
            break;
        case DataPath::TETHERED_TO_APPS_SW:
            dataPathStr = "TETHERED_TO_APPS_SW";
            break;
        case DataPath::APPS_TO_WAN:
            dataPathStr = "APPS_TO_WAN";
            break;

    }
    return dataPathStr;
}

std::string TrafficFilterImpl::directionToString(Direction direction) {
    std::string directionStr = "";
    switch (direction) {
        case Direction::UPLINK:
            directionStr = "UPLINK";
            break;
        case Direction::DOWNLINK:
            directionStr = "DOWNLINK";
            break;
    }
    return directionStr;
}

TrafficFilterBuilder::TrafficFilterBuilder() {
    trafficFilter_ = nullptr;
}

TrafficFilterBuilder &TrafficFilterBuilder::setIPv4Address(
    std::string ipv4Addr, FieldType fieldType) {
    if (trafficFilter_ == nullptr) {
        trafficFilter_ = std::make_shared<TrafficFilterImpl>();
    }
    std::static_pointer_cast<TrafficFilterImpl>(trafficFilter_)->setIPv4Address(ipv4Addr, fieldType);
    return *this;
}
TrafficFilterBuilder &TrafficFilterBuilder::setIPv6Address(
    std::string ipv6Addr, FieldType fieldType) {
    if (trafficFilter_ == nullptr) {
        trafficFilter_ = std::make_shared<TrafficFilterImpl>();
    }
    std::static_pointer_cast<TrafficFilterImpl>(trafficFilter_)->setIPv6Address(ipv6Addr, fieldType);
    return *this;
}

TrafficFilterBuilder &TrafficFilterBuilder::setPort(uint16_t port, FieldType fieldType) {
    if (trafficFilter_ == nullptr) {
        trafficFilter_ = std::make_shared<TrafficFilterImpl>();
    }
    std::static_pointer_cast<TrafficFilterImpl>(trafficFilter_)->setPort(port, fieldType);
    return *this;
}

TrafficFilterBuilder &TrafficFilterBuilder::setPortRange(
    uint16_t startPort, uint16_t range, FieldType fieldType) {
    if (trafficFilter_ == nullptr) {
        trafficFilter_ = std::make_shared<TrafficFilterImpl>();
    }
    std::static_pointer_cast<TrafficFilterImpl>(trafficFilter_)
        ->setPortRange(startPort, range, fieldType);
    return *this;
}

TrafficFilterBuilder &TrafficFilterBuilder::setVlanList(
    std::vector<int> vlanList, FieldType fieldType) {
    if (trafficFilter_ == nullptr) {
        trafficFilter_ = std::make_shared<TrafficFilterImpl>();
    }
    std::static_pointer_cast<TrafficFilterImpl>(trafficFilter_)->setVlanList(vlanList, fieldType);
    return *this;
}

TrafficFilterBuilder &TrafficFilterBuilder::setIPProtocol(IpProtocol ipProtocol) {
    if (trafficFilter_ == nullptr) {
        trafficFilter_ = std::make_shared<TrafficFilterImpl>();
    }
    std::static_pointer_cast<TrafficFilterImpl>(trafficFilter_)->setIPProtocol(ipProtocol);
    return *this;
}

TrafficFilterBuilder &TrafficFilterBuilder::setDirection(Direction direction) {
    if (trafficFilter_ == nullptr) {
        trafficFilter_ = std::make_shared<TrafficFilterImpl>();
    }
    std::static_pointer_cast<TrafficFilterImpl>(trafficFilter_)->setDirection(direction);
    return *this;
}


TrafficFilterBuilder &TrafficFilterBuilder::setDataPath(DataPath dataPath) {
    if (trafficFilter_ == nullptr) {
        trafficFilter_ = std::make_shared<TrafficFilterImpl>();
    }
    std::static_pointer_cast<TrafficFilterImpl>(trafficFilter_)->setDataPath(dataPath);
    return *this;
}

TrafficFilterBuilder &TrafficFilterBuilder::setPCP(int8_t pcp) {
    if (trafficFilter_ == nullptr) {
        trafficFilter_ = std::make_shared<TrafficFilterImpl>();
    }
    std::static_pointer_cast<TrafficFilterImpl>(trafficFilter_)->setPCP(pcp);
    return *this;
}

std::shared_ptr<ITrafficFilter> TrafficFilterBuilder::build() {
    std::shared_ptr<ITrafficFilter> trafficFilter = trafficFilter_;
    trafficFilter_ = nullptr;
    return trafficFilter;
}

}  // namespace data
}  // namespace telux