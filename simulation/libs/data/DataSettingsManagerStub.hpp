/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_SETTINGS_MANAGER_STUB_HPP
#define DATA_SETTINGS_MANAGER_STUB_HPP

#include <mutex>
#include <vector>
#include <memory>
#include <condition_variable>

#include <telux/data/DataSettingsManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {
namespace data {

class DataSettingsManagerStub : public IDataSettingsManager,
                                public IDataSettingsListener {
public:
    DataSettingsManagerStub(OperationType oprType);
    ~DataSettingsManagerStub();

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status restoreFactorySettings(OperationType operationType,
        telux::common::ResponseCallback callback = nullptr, bool isRebootNeeded = true) override;

    telux::common::Status setBackhaulPreference(std::vector<BackhaulType> backhaulPref,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestBackhaulPreference(
        RequestBackhaulPrefResponseCb callback) override;

    telux::common::Status setBandInterferenceConfig(bool enable,
        std::shared_ptr<BandInterferenceConfig> config,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestBandInterferenceConfig(
        RequestBandInterferenceConfigResponseCb callback) override;

    telux::common::Status requestDdsSwitch(DdsInfo info,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestCurrentDds(RequestCurrentDdsResponseCb callback) override;

    telux::common::Status setWwanConnectivityConfig (SlotId slotId, bool allow,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestWwanConnectivityConfig(SlotId slotId,
        requestWwanConnectivityConfigResponseCb callback) override;

    telux::common::Status setMacSecState(bool enable,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestMacSecState(RequestMacSecSateResponseCb callback) override;

    telux::common::Status switchBackHaul(BackhaulInfo source, BackhaulInfo dest,
        bool applyToAll = false, telux::common::ResponseCallback callback = nullptr) override;

    telux::common::ErrorCode getIpPassThroughConfig( const IpptParams &ipptParms,
            IpptConfig &config) override;

    telux::common::ErrorCode setIpPassThroughConfig(const IpptParams &ipptParms,
            const IpptConfig &config) override;

    telux::common::ErrorCode setIpPassThroughNatConfig(bool enableNat = true);
    telux::common::ErrorCode getIpPassThroughNatConfig(bool &isNatEnabled);

    telux::common::ErrorCode getIpConfig(const IpConfigParams &ipConfigParams,
            IpConfig &ipConfig) override;

    telux::common::ErrorCode setIpConfig(const IpConfigParams &ipConfigParams,
            const IpConfig &ipConfig) override;

    bool isDeviceDataUsageMonitoringEnabled() override;

    telux::common::Status registerListener(
        std::weak_ptr<IDataSettingsListener> listener) override;

    telux::common::Status deregisterListener(
        std::weak_ptr<IDataSettingsListener> listener) override;

    telux::common::Status init(telux::common::InitResponseCb callback);

    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onWwanConnectivityConfigChange(SlotId slotId, bool isConnectivityAllowed) override;
    void onDdsChange(DdsInfo currentState) override;

private:
    std::mutex mtx_;
    std::mutex initMtx_;
    std::mutex mutex_;

    telux::data::OperationType oprType_;
    telux::common::ServiceStatus subSystemStatus_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::unique_ptr<::dataStub::DataSettingsManager::Stub> stub_;
    std::vector<std::weak_ptr<IDataSettingsListener>> listeners_;
    telux::common::InitResponseCb initCb_;

    void initSync(telux::common::InitResponseCb callback);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay );
    void getAvailableListeners(
        std::vector<std::shared_ptr<IDataSettingsListener>> &listeners);
};

} // end of namespace data
} // end of namespace telux

#endif //DATA_SETTINGS_MANAGER_STUB_HPP
