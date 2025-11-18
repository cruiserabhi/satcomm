/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NAT_MANAGER_STUB_HPP
#define NAT_MANAGER_STUB_HPP

#include <telux/data/net/NatManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {
namespace data {
namespace net {

class NatManagerStub : public INatManager,
                       public INatListener {
public:
    NatManagerStub (telux::data::OperationType oprType);
    ~NatManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;

    telux::common::Status registerListener(std::weak_ptr<INatListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<INatListener> listener) override;

    void onServiceStatusChange(ServiceStatus status);

    telux::data::OperationType getOperationType() override;

    telux::common::Status addStaticNatEntry(int profileId, const NatConfig &snatConfig,
        telux::common::ResponseCallback callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status addStaticNatEntry(const BackhaulInfo &bhInfo, const NatConfig &snatConfig,
            telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status removeStaticNatEntry(int profileId, const NatConfig &snatConfig,
        telux::common::ResponseCallback callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status removeStaticNatEntry(const BackhaulInfo &bhInfo, const NatConfig
            &snatConfig, telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestStaticNatEntries(int profileId,
        StaticNatEntriesCb snatEntriesCb, SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status requestStaticNatEntries(const BackhaulInfo &bhInfo, StaticNatEntriesCb
            snatEntriesCb) override;

private:
    std::mutex mtx_;
    std::mutex initMtx_;
    std::condition_variable cv_;

    bool ready_ = false;
    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::dataStub::SnatManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<INatListener>> listenerMgr_;
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

 #endif //NAT_MANAGER_STUB_HPP
