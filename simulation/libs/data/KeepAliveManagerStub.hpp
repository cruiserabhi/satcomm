/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef KEEPALIVE_MANAGER_STUB_HPP
 #define KEEPALIVE_MANAGER_STUB_HPP

#include <telux/data/KeepAliveManager.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {
namespace data {

class KeepAliveManagerStub : public IKeepAliveManager,
                             public IKeepAliveListener {
public:
    KeepAliveManagerStub (SlotId slotId);
    ~KeepAliveManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::ErrorCode enableTCPMonitor(
        const TCPKAParams &tcpKaParams, MonitorHandleType &monHandle) override;

    telux::common::ErrorCode disableTCPMonitor(const MonitorHandleType monHandle) override;

    telux::common::ErrorCode startTCPKeepAliveOffload(
        const TCPKAParams &tcpKaParams, const TCPSessionParams &tcpSessionParams,
        const uint32_t interval, TCPKAOffloadHandle &handle) override;

    telux::common::ErrorCode startTCPKeepAliveOffload(
        const MonitorHandleType monHandle, const uint32_t interval, TCPKAOffloadHandle &handle) override;

    telux::common::ErrorCode stopTCPKeepAliveOffload(const TCPKAOffloadHandle handle) override;

    telux::common::Status registerListener(std::weak_ptr<IKeepAliveListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IKeepAliveListener> listener) override;

private:
    SlotId slotId_ = DEFAULT_SLOT_ID;
    telux::data::OperationType oprType_;

    std::mutex mtx_;
    std::mutex initMtx_;

    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::dataStub::KeepAliveManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<IKeepAliveListener>> listenerMgr_;

    void initSync(telux::common::InitResponseCb callback);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void onServiceStatusChange(telux::common::ServiceStatus status);
};

} // end of namespace data
} // end of namespace telux

 #endif //KEEPALIVE_MANAGER_STUB_HPP