/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_SETTINGS_SERVER_HPP
#define DATA_SETTINGS_SERVER_HPP

#include <telux/data/DataSettingsManager.hpp>

#include "data/DataConnectionServerImpl.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class DataSettingsServerImpl final:
    public dataStub::DataSettingsManager::Service {
public:
    DataSettingsServerImpl(
        std::shared_ptr<DataConnectionServerImpl> dcmServerImpl);
    ~DataSettingsServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::InitRequest* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status SetDdsSwitch(ServerContext* context,
        const dataStub::SetDdsSwitchRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestCurrentDdsSwitch(ServerContext* context,
        const dataStub::CurrentDdsSwitchRequest* request,
        dataStub::CurrentDdsSwitchResponse* response) override;

    grpc::Status setBandInterferenceConfig(ServerContext* context,
        const dataStub::BandInterferenceConfig* request,
        dataStub::DefaultReply* response) override;

    grpc::Status requestBandInterferenceConfig(ServerContext* context,
        const dataStub::BandInterferenceRequest* request,
        dataStub::BandInterferenceReply* response) override;

    grpc::Status SetWwanConnectivityConfig(ServerContext* context,
        const dataStub::SetWwanConnectivityConfigRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestWwanConnectivityConfig(ServerContext* context,
        const dataStub::WwanConnectivityConfigRequest* request,
        dataStub::WwanConnectivityConfigReply* response) override;

    grpc::Status SetMacSecState(ServerContext* context,
        const dataStub::SetMacSecStateRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestMacSecState(ServerContext* context,
        const dataStub::MacSecStateRequest* request,
        dataStub::MacSecStateReply* response) override;

    grpc::Status setBackhaulPreference(ServerContext* context,
        const dataStub::setBackhaulPreferenceRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status requestBackhaulPreference(ServerContext* context,
        const dataStub::RequestBackhaulPreference* request,
        dataStub::BackhaulPreferenceReply* response) override;

    grpc::Status switchBackHaul(ServerContext* context,
        const dataStub::switchBackHaulRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status setIpPassThroughConfig(ServerContext* context,
        const dataStub::setIpptConfigRequest* request, dataStub::setIpptConfigReply* response)
        override;

    grpc::Status getIpPassThroughConfig(ServerContext* context,
        const dataStub::getIpptConfigRequest* request,
        dataStub::getIpptConfigReply* response) override;

    grpc::Status getIpConfig(ServerContext* context, const dataStub::getIpConfigRequest* request,
            dataStub::getIpConfigReply* response) override;

    grpc::Status setIpConfig(ServerContext* context, const dataStub::setIpConfigRequest* request,
            dataStub::setIpConfigReply* response) override;

    grpc::Status GetIpPassThroughNatConfig(ServerContext* context,
            const google::protobuf::Empty *request,
            dataStub::getIpptNatConfigReply* response) override;

    grpc::Status SetIpPassThroughNatConfig(ServerContext* context,
            const dataStub::setIpptNatConfigRequest* request,
            dataStub::setIpptNatConfigReply* response) override;
private:

    struct IpConfigStruct {
        uint32_t vlanId = -1;
        telux::data::InterfaceType ifType = telux::data::InterfaceType::UNKNOWN;
        telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::UNKNOWN;
        telux::data::IpAssignOperation ipAssign = telux::data::IpAssignOperation::UNKNOWN;
        telux::data::IpAssignType ipType = telux::data::IpAssignType::UNKNOWN;
        telux::data::IpAddrInfo ipAddr;
    };

    struct IpptStruct {
        std::string macAddr;
        std::string ifType;
        std::string ipptOpr;
    };

    // Map of VlanId, ipFamilyType with Ipconfig
    std::map<uint32_t, std::map<telux::data::IpFamilyType, IpConfigStruct>>
        ipConfigMap_;
    dataStub::BackhaulPreference convertBackhaulPrefStringToEnum(std::string pref);
    std::string convertEnumToBackhaulPrefString(::dataStub::BackhaulPreference pref);
    void updateDdsInfo();

    telux::data::DdsInfo ddsInfo_;
    std::shared_ptr<DataConnectionServerImpl> dcmServerImpl_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

    template <typename T>
    bool isIpptConfigExist(const T* request, const Json::Value &config, int configSize,
            int& configIdx, bool &isConfigSame, const IpptStruct *ipptStruct = nullptr);

    template <typename T>
    bool isIpConfigExist(const T* request, const telux::data::IpAssignType ipType, const
            telux::data::IpAssignOperation ipAssign);

    telux::common::ErrorCode modifyIpConfig(IpConfigStruct &ipConfigStruct);

    bool isIpConfigSame(const telux::data::IpAddrInfo &newIpConfig,
            const telux::data::IpAddrInfo &currentIpConfig);

    telux::common::ErrorCode validateV4IpAddr(const telux::data::IpAddrInfo &ipAddr);
};

#endif //DATA_SETTINGS_SERVER_HPP
