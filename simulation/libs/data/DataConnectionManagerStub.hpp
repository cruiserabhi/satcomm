/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_CONNECTION_MANAGER_STUB_HPP
#define DATA_CONNECTION_MANAGER_STUB_HPP

#include <telux/data/DataConnectionManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "DataCallStub.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using ::dataStub::DataConnectionManager;

namespace telux {
namespace data {

class DataEventListener;

class DataConnectionManagerStub : public IDataConnectionManager,
                                  public IDataConnectionListener,
                                  public std::enable_shared_from_this<DataConnectionManagerStub> {
public:
    DataConnectionManagerStub(SlotId slotId);
    ~DataConnectionManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;

    telux::common::Status setDefaultProfile(OperationType oprType, uint8_t profileId,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status getDefaultProfile(
        OperationType oprType, DefaultProfileIdResponseCb callback) override;

    telux::common::Status setRoamingMode(bool enable, uint8_t profileId,
        OperationType operationType,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestRoamingMode(uint8_t profileId, OperationType operationType,
        requestRoamingModeResponseCb callback) override;

    telux::common::Status startDataCall(
        const DataCallParams &dataCallParams, DataCallResponseCb callback = nullptr) override;
    telux::common::Status stopDataCall(
        const DataCallParams &dataCallParams, DataCallResponseCb callback = nullptr) override;

    telux::common::Status startDataCall(int profileId,
        IpFamilyType ipFamilyType = IpFamilyType::IPV4V6, DataCallResponseCb callback = nullptr,
        OperationType operationType = OperationType::DATA_LOCAL, std::string apn = "") override;
    telux::common::Status stopDataCall(int profileId,
        IpFamilyType ipFamilyType = IpFamilyType::IPV4V6, DataCallResponseCb callback = nullptr,
        OperationType operationType = OperationType::DATA_LOCAL, std::string apn = "") override;
    telux::common::Status requestDataCallList(OperationType type,
        DataCallListResponseCb callback) override;

    telux::common::Status requestThrottledApnInfo(ThrottleInfoCb callback) override;

    telux::common::Status registerListener(std::weak_ptr<IDataConnectionListener> listener)
        override;
    telux::common::Status deregisterListener(std::weak_ptr<IDataConnectionListener> listener)
        override;

    int getSlotId() override;
    void onServiceStatusChange(common::ServiceStatus status) override;

    void handleStartDataCallEvent(::dataStub::StartDataCallEvent startEvent);
    void handleStopDataCallEvent(::dataStub::StopDataCallEvent stopEvent);
    void handleThrottledApnInfoChangedEvent(::dataStub::APNThrottleInfoList throttleInfoList);

    telux::common::Status cleanup();

private:
    std::mutex initMtx_;
    std::mutex mtx_;
    std::condition_variable cv_;

    SlotId slotId_ = DEFAULT_SLOT_ID;
    bool ready_ = false;
    telux::common::ServiceStatus subSystemStatus_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::map<uint8_t, bool> roamingModemap_;
    std::map<OperationType, uint8_t> defaultProfileIdMap_;
    std::mutex mutex_;
    std::vector<std::weak_ptr<IDataConnectionListener>> listeners_;
    std::map<int, std::shared_ptr<DataCallStub>> dataCalls_;
    std::map<int, std::shared_ptr<DataCallStub>> cacheDataCalls_;
    std::unique_ptr<::dataStub::DataConnectionManager::Stub> stub_;
    std::shared_ptr<DataEventListener> eventListener_;

    void initSync(telux::common::InitResponseCb callback);
    bool waitForInitialization();
    void setSubsystemReady(bool status);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay);
    void getAvailableListeners(
        std::vector<std::shared_ptr<IDataConnectionListener>> &listeners);
    void invokeDataConnectionListener(std::shared_ptr<IDataCall> call);
    void handleEvent(std::string token , std::string event);
    void requestConnectedDataCallLists();
};

} // end of namespace data

} // end of namespace telux

#endif // DATA_CONNECTION_MANAGER_STUB_HPP
