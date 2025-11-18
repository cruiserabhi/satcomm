/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef FIREWALL_MANAGER_STUB_HPP
 #define FIREWALL_MANAGER_STUB_HPP

#include <telux/data/net/FirewallManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

#define PROTO_ICMP 1
#define PROTO_ICMP6 58
#define PROTO_IGMP 2
#define PROTO_TCP  6
#define PROTO_UDP 17
#define PROTO_ESP 50
#define PROTO_TCP_UDP 253

namespace telux {
namespace data {
namespace net {

class FirewallManagerStub : public IFirewallManager,
                            public IFirewallListener {
public:
    FirewallManagerStub (telux::data::OperationType oprType);
    ~FirewallManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;

    telux::common::Status registerListener(std::weak_ptr<IFirewallListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IFirewallListener> listener) override;

    void onServiceStatusChange(telux::common::ServiceStatus status);
    telux::data::OperationType getOperationType() override;

    telux::common::Status setFirewallConfig(FirewallConfig fwConfig,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestFirewallConfig(BackhaulInfo bhInfo,
        FirewallConfigCb callback) override;

    telux::common::Status addHwAccelerationFirewallEntry(FirewallEntryInfo entry,
        AddFirewallEntryCb callback = nullptr) override;

    telux::common::Status requestHwAccelerationFirewallEntries(BackhaulInfo bhInfo,
        FirewallEntryInfoCb callback) override;

    telux::common::Status addFirewallEntry(FirewallEntryInfo entry,
        AddFirewallEntryCb callback = nullptr) override;

    telux::common::Status requestFirewallEntries(BackhaulInfo bhInfo,
        FirewallEntryInfoCb callback) override;

    telux::common::Status removeFirewallEntry(BackhaulInfo bhInfo, uint32_t handle,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status enableDmz(DmzConfig config,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status disableDmz(BackhaulInfo bhInfo, const IpFamilyType ipType,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestDmzEntry(BackhaulInfo bhInfo, DmzEntryInfoCb callback) override;

    telux::common::Status requestFirewallStatus(int profileId,
        FirewallStatusCb callback, SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status setFirewall(int profileId, bool enable, bool allowPackets,
        telux::common::ResponseCallback callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status addFirewallEntry(int profileId, std::shared_ptr<IFirewallEntry> entry,
        telux::common::ResponseCallback callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status addHwAccelerationFirewallEntry(int profileId,
        std::shared_ptr<IFirewallEntry> entry, AddFirewallEntryCb callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status requestHwAccelerationFirewallEntries(
        int profileId, FirewallEntriesCb callback, SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status requestFirewallEntries(int profileId,
        FirewallEntriesCb callback, SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status removeFirewallEntry(int profileId, uint32_t handle,
        telux::common::ResponseCallback callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status enableDmz(int profileId, const std::string ipAddr,
        telux::common::ResponseCallback callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status disableDmz(int profileId, const telux::data::IpFamilyType ipType,
        telux::common::ResponseCallback callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) override;

    telux::common::Status requestDmzEntry(int profileId,
        DmzEntriesCb dmzCb, SlotId slotId = DEFAULT_SLOT_ID) override;

private:
    std::mutex mtx_;
    std::mutex initMtx_;
    std::condition_variable cv_;

    bool ready_ = false;
    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::dataStub::FirewallManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::vector<std::weak_ptr<IFirewallListener>> listeners_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<IFirewallListener>> listenerMgr_;
    telux::data::OperationType oprType_;

    void initSync(telux::common::InitResponseCb callback);
    bool waitForInitialization();
    void setSubsystemReady(bool status);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay );

    telux::common::Status addFirewallEntryRequest(FirewallEntryInfo entry,
        AddFirewallEntryCb callback = nullptr, bool isHwAccelerated = false);

    telux::common::Status getFirewallEntriesRequest(BackhaulInfo bhInfo,
        FirewallEntryInfoCb callback, bool isHwAccelerated = false);

};

} // end of namespace net
} // end of namespace data
} // end of namespace telux

 #endif //FIREWALL_MANAGER_STUB_HPP