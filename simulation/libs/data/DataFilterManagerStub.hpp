/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_FILTER_MANAGER_STUB_HPP
#define DATA_FILTER_MANAGER_STUB_HPP

#include <telux/data/DataFilterManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {
namespace data {

class DataFilterManagerStub : public IDataFilterManager,
                              public IDataFilterListener,
                              public telux::common::IEventListener,
                              public std::enable_shared_from_this<DataFilterManagerStub> {

public:
    DataFilterManagerStub (SlotId slotId);
    ~DataFilterManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    bool isReady() override;

    std::future<bool> onReady() override;

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status setDataRestrictMode(DataRestrictMode mode,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestDataRestrictMode(DataRestrictModeCb callback) override;

    telux::common::Status addDataRestrictFilter(std::shared_ptr<IIpFilter> &filter,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status removeAllDataRestrictFilters(
        telux::common::ResponseCallback callback = nullptr) override;

    SlotId getSlotId() override;

    telux::common::Status registerListener(std::weak_ptr<IDataFilterListener> listener) override;

    telux::common::Status deregisterListener(std::weak_ptr<IDataFilterListener> listener) override;

    virtual void onDataRestrictModeChange(DataRestrictMode mode) override;
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) override;

    /* @deprecated because NAO IP filters are global filters */
    telux::common::Status setDataRestrictMode(DataRestrictMode mode,
        telux::common::ResponseCallback callback, int profileId,
        IpFamilyType ipFamilyType = IpFamilyType::UNKNOWN) override;

    /* @deprecated because NAO IP filters are global filters */
    telux::common::Status requestDataRestrictMode(
        std::string ifaceName, DataRestrictModeCb callback) override;

    /* @deprecated because NAO IP filters are global filters */
    telux::common::Status addDataRestrictFilter(std::shared_ptr<IIpFilter> &filter,
        telux::common::ResponseCallback callback, int profileId,
        IpFamilyType ipFamilyType = IpFamilyType::UNKNOWN) override;

    /* @deprecated because NAO IP filters are global filters */
    telux::common::Status removeAllDataRestrictFilters(
        telux::common::ResponseCallback callback, int profileId,
        IpFamilyType ipFamilyType = IpFamilyType::UNKNOWN) override;

    void onEventUpdate(google::protobuf::Any event) override;

private:
    void initSync(telux::common::InitResponseCb callback);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay);
    bool waitForInitialization();

    std::mutex mtx_;
    std::mutex initMtx_;
    std::mutex mutex_;
    std::condition_variable initCV_;

    std::unique_ptr<::dataStub::DataFilterManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<telux::common::ListenerManager<IDataFilterListener>> listenerMgr_;
    telux::common::InitResponseCb initCb_;

    SlotId slotId_ = DEFAULT_SLOT_ID;
    telux::common::ServiceStatus subSystemStatus_;
};

} // end of namespace data
} // end of namespace telux

#endif //DATA_FILTER_MANAGER_STUB_HPP