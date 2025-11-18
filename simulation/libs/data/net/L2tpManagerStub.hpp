/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef L2TP_MANAGER_STUB_HPP
#define L2TP_MANAGER_STUB_HPP

#include <telux/data/net/L2tpManager.hpp>

#include "common/ListenerManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {

namespace common{
    template <typename T>
    class AsyncTaskQueue;
}

namespace data {
namespace net {

class L2tpManagerStub : public IL2tpManager,
                        public IL2tpListener {
public:
    L2tpManagerStub ();
    ~L2tpManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;

    telux::common::Status setConfig(bool enable, bool enableMss, bool enableMtu,
        telux::common::ResponseCallback callback = nullptr, uint32_t mtuSize = 1422) override;

    telux::common::Status requestConfig(L2tpConfigCb l2tpConfigCb) override;

    telux::common::Status addTunnel(const L2tpTunnelConfig &l2tpTunnelConfig,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status removeTunnel(
        uint32_t tunnelId, telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status addSession(uint32_t tunnelId, L2tpSessionConfig sessionConfig,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status removeSession(uint32_t tunnelId,
        uint32_t sessionId, telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status bindSessionToBackhaul(L2tpSessionBindConfig sessionBindConfig,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status unbindSessionFromBackhaul(L2tpSessionBindConfig sessionBindConfig,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status querySessionToBackhaulBindings(
        BackhaulType backhaul, L2tpSessionBindingsResponseCb callback) override;

    telux::common::Status registerListener(std::weak_ptr<IL2tpListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IL2tpListener> listener) override;

    void onServiceStatusChange(ServiceStatus status);

private:
    std::mutex mtx_;
    std::mutex initMtx_;
    std::condition_variable cv_;

    bool ready_ = false;
    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::dataStub::L2tpManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<IL2tpListener>> listenerMgr_;

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

 #endif //L2TP_MANAGER_STUB_HPP