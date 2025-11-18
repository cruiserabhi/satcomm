/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TRAFFIC_FILTER_IMPL_HPP
#define TRAFFIC_FILTER_IMPL_HPP

#include <telux/data/TrafficFilter.hpp>

namespace telux {
namespace data {

class TrafficFilterImpl : public ITrafficFilter {
 public:
    std::string getIPv4Address(FieldType fieldType) override;
    void setIPv4Address(std::string ipv4Addr, FieldType fieldType);

    std::string getIPv6Address(FieldType fieldType) override;
    void setIPv6Address(std::string ipv6Addr, FieldType fieldType);

    uint16_t getPort(FieldType fieldType) override;
    void setPort(uint16_t sourcePort, FieldType fieldType);

    void getPortRange(FieldType fieldType, uint16_t &startPort, uint16_t &range) override;
    void setPortRange(uint16_t startPort, uint16_t range, FieldType fieldType);

    std::vector<int> getVlanList(FieldType fieldType) override;
    void setVlanList(std::vector<int> vlanList, FieldType fieldType);

    IpProtocol getIPProtocol() override;
    void setIPProtocol(IpProtocol ipProtocol);

    Direction getDirection() override;
    void setDirection(Direction direction);

    int8_t getPCP() override;
    void setPCP(int8_t pcp);

    DataPath getDataPath() override;
    void setDataPath(DataPath dataPath);

    TrafficFilterValidFields getTrafficFilterValidFields() override;

    std::string toString() override;

    /**
     * Destructor for TrafficFilterImpl
     */
    ~TrafficFilterImpl();


 static std::string dataPathToString(DataPath dataPath);
 static std::string directionToString(Direction direction);

 private:
    std::string sourceIPv4Address_ = "";
    std::string sourceIPv6Address_ = "";
    uint16_t sourcePort_;
    std::string sourceMacAddress_ = "";
    std::string sourceInterfaceName_ = "";
    std::vector<int> sourceVlanList_;
    uint16_t sourceStartPort_ = 0;
    uint16_t sourcePortRange_ = 0;

    std::string destIPv4Address_ = "";
    std::string destIPv6Address_ = "";
    uint16_t destPort_;
    std::string destMacAddress_ = "";
    std::string destInterfaceName_ = "";
    std::vector<int> destVlanList_;
    uint16_t destStartPort_ = 0;
    uint16_t destPortRange_ = 0;

    IpProtocol ipProtocol_ = 0;
    Direction direction_ = Direction::UPLINK;
    int8_t pcp_ = 0;
    TrafficFilterValidFields validityMask_ = 0;
    DataPath dataPath_ = DataPath::TETHERED_TO_WAN_HW;



};
}  // namespace data
}  // namespace telux
#endif  // TRAFFIC_FILTER_IMPL_HPP