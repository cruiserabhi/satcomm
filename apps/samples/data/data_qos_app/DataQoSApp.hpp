/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATAQOSAPP_HPP
#define DATAQOSAPP_HPP

#include <memory>

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/QoSManager.hpp>
#include <telux/data/net/VlanManager.hpp>
#include <telux/data/DataConnectionManager.hpp>

#include "ConsoleApp.hpp"
using namespace telux::common;

class DataQoSApp : public telux::data::IDataConnectionListener,
                    public ConsoleApp,
                    public std::enable_shared_from_this<telux::data::IDataConnectionListener> {
public:
    DataQoSApp();
    ~DataQoSApp();

    static std::shared_ptr<DataQoSApp> getInstance();
    void onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) override;
    bool initDataConnectionManager();
    bool initVlanManager();
    bool loadDefaultProfile();
    bool startAndWaitForDataCall();
    bool createAndWaitForVlan(int vlanId, bool isAccelerated = false, int pcp = 0);
    bool bindVlanToBackhaul(int vlanId);
    void stopDataCall();
    void removeVlan(int vlanId);
    bool createUplinkTrafficClass(int trafficClass, telux::data::net::DataPath dataPath);
    bool createDownLinkTrafficClass(int trafficClass, telux::data::net::DataPath dataPath,
        int minBandwidth, int maxBandwidth);
    uint32_t addVlanPcpQoSFilter(int trafficClass, telux::data::Direction direction,
        telux::data::net::DataPath dataPath, int vlanId, int pcp = 0);
    uint32_t addVlanQoSFilter(int trafficClass, telux::data::Direction direction,
        telux::data::net::DataPath dataPath, int vlanId);
    uint32_t addIPv4QoSFilter(int trafficClass, telux::data::Direction direction,
        telux::data::net::DataPath dataPath, int protocol, std::string srcIPv4, int destPort = -1,
        int srcPort = -1);
    bool parseArguments(int argc, char **argv);
    void printUseCases();
    void runUseCase(int useCase);
    void showAllConfigs();
    void printHelp();

    bool createTCAndAddQoSFilterForDLTetheredToAppsSWPath();
    bool createTCAndAddQoSFilterForULTetheredToAppsSWPath();
    bool createTCAndAddQoSFilterForDLTetheredToWanHWAccPath();
    bool createTCAndAddQoSFilterForULTetheredToWanHWAccPath();
    bool createTCAndAddQoSFilterForULAppsToWanPath();

    bool configureVLANs();
    void clearVlanConfigs();
    void cleanup(int signum = SIGINT);
    std::string vlanInterfaceToString(
        telux::data::InterfaceType interface, telux::data::OperationType oprType);
    std::string backhaulToString(telux::data::BackhaulType backhaul);

    void consoleInit();
private:
    void logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall);

    std::shared_ptr<telux::data::net::IQoSManager> dataQoSManager_;
    std::shared_ptr<telux::data::IDataConnectionManager> dataConnectionManager_;
    std::shared_ptr<telux::data::net::IVlanManager> vlanManager_;

    std::condition_variable dataCallCv_;
    std::mutex dataCallMtx_;
    std::string rmnetIp_ = "";
    int profileId_;
    SlotId slotId_;

    std::vector<int> qosFilterHandles_;
    std::vector<int> trafficClass_;
};

#endif  // DATAQOSAPP_HPP
