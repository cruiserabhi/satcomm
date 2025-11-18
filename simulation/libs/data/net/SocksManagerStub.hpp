/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef SOCKS_MANAGER_STUB_HPP
 #define SOCKS_MANAGER_STUB_HPP

#include <telux/data/net/SocksManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {
namespace data {
namespace net {

class SocksManagerStub : public ISocksManager,
                         public ISocksListener {
public:
    SocksManagerStub (telux::data::OperationType oprType);
    ~SocksManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;

    telux::common::Status enableSocks(bool enable,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status registerListener(std::weak_ptr<ISocksListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<ISocksListener> listener) override;

    void onServiceStatusChange(ServiceStatus status);
    telux::data::OperationType getOperationType() override;

private:
    std::mutex mtx_;
    std::mutex initMtx_;
    std::condition_variable cv_;

    bool ready_ = false;
    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::dataStub::SocksManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<ISocksListener>> listenerMgr_;
    telux::data::OperationType oprType_;

    void initSync(telux::common::InitResponseCb callback);
    bool waitForInitialization();
    void setSubsystemReady(bool status);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay );
};

} // end of namespace net
} // end of namespace data
} // end of namespace telux

 #endif //SOCKS_MANAGER_STUB_HPP