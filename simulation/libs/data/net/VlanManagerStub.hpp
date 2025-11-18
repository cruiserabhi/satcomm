/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

 #ifndef VLAN_MANAGER_STUB_HPP
 #define VLAN_MANAGER_STUB_HPP

#include <telux/data/net/VlanManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {
namespace data {
namespace net {

class VlanManagerStub : public IVlanManager,
                        public IVlanListener {
public:
    VlanManagerStub (telux::data::OperationType oprType);
    ~VlanManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;

    telux::common::Status registerListener(std::weak_ptr<IVlanListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IVlanListener> listener) override;

    void onServiceStatusChange(ServiceStatus status);
    telux::data::OperationType getOperationType() override;

    telux::common::Status createVlan(
        const VlanConfig &vlanConfig, CreateVlanCb callback = nullptr) override;

    telux::common::Status removeVlan(int16_t vlanId, InterfaceType ifaceType,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status queryVlanInfo(QueryVlanResponseCb callback) override;

    telux::common::Status bindWithProfile(int profileId, int vlanId,
        telux::common::ResponseCallback callback = nullptr,
        SlotId slotId                            = DEFAULT_SLOT_ID) override;

    telux::common::Status unbindFromProfile(int profileId, int vlanId,
        telux::common::ResponseCallback callback = nullptr,
        SlotId slotId                            = DEFAULT_SLOT_ID) override;

    telux::common::Status queryVlanMappingList(
        VlanMappingResponseCb callback, SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status bindToBackhaul(
        VlanBindConfig vlanBindConfig, telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status unbindFromBackhaul(
        VlanBindConfig vlanBindConfig, telux::common::ResponseCallback callback = nullptr) override;

    virtual telux::common::Status queryVlanToBackhaulBindings(BackhaulType backhaulType,
        VlanBindingsResponseCb callback, SlotId slotId = DEFAULT_SLOT_ID) override;

private:
    std::mutex mtx_;
    std::mutex initMtx_;
    std::condition_variable cv_;

    bool ready_ = false;
    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::dataStub::VlanManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<IVlanListener>> listenerMgr_;
    telux::data::OperationType oprType_;

    void initSync(telux::common::InitResponseCb callback);
    bool waitForInitialization();
    void setSubsystemReady(bool status);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay);
};

} // end of namespace net
} // end of namespace data
} // end of namespace telux

 #endif //VLAN_MANAGER_STUB_HPP