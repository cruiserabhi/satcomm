 /*
  *  Copyright (c) 2021,2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
  *  SPDX-License-Identifier: BSD-3-Clause-Clear
  */

/**
 * @file       DataFactoryImplStub.hpp
 *
 * @brief      Implementation of DataFactory
 *
 */

#ifndef DATA_FACTORY_IMPL_STUB_HPP
#define DATA_FACTORY_IMPL_STUB_HPP

#include <memory>
#include <map>

#include <telux/data/DataFactory.hpp>
#include "common/AsyncTaskQueue.hpp"
#include "common/FactoryHelper.hpp"

namespace telux {
namespace data {

class DataFactoryImplStub : public DataFactory,
                            public telux::common::FactoryHelper {
 public:
    static DataFactory &getInstance();

    virtual std::shared_ptr<IDataConnectionManager> getDataConnectionManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<IDataProfileManager> getDataProfileManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<IServingSystemManager> getServingSystemManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<IDataFilterManager> getDataFilterManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::INatManager> getNatManager(
        telux::data::OperationType oprType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::IFirewallManager> getFirewallManager(
        telux::data::OperationType oprType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::IFirewallEntry> getNewFirewallEntry(
        IpProtocol proto, Direction direction, IpFamilyType ipFamilyType) override;

    virtual std::shared_ptr<IIpFilter> getNewIpFilter(IpProtocol proto) override;

    virtual std::shared_ptr<telux::data::net::IVlanManager> getVlanManager(
        telux::data::OperationType oprType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::ISocksManager> getSocksManager(
        telux::data::OperationType oprType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::IBridgeManager> getBridgeManager(
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::IL2tpManager> getL2tpManager(
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::IDataSettingsManager> getDataSettingsManager(
        telux::data::OperationType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::IClientManager> getClientManager(
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::IDualDataManager> getDualDataManager(
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::IDataControlManager> getDataControlManager(
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::IDataLinkManager> getDataLinkManager(
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::IQoSManager> getQoSManager(
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::IKeepAliveManager> getKeepAliveManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) override;

 private:
    DataFactoryImplStub();
    ~DataFactoryImplStub();

    void initCompleteNotifier(
        std::vector<telux::common::InitResponseCb> &initCbs,
        telux::common::ServiceStatus status);
    void initCompleteNotifierWithSlotId(
        std::map<SlotId, std::vector<telux::common::InitResponseCb>> &initCbs,
        telux::common::ServiceStatus status, SlotId slotId);

    void initCompleteNotifierWithOprType(
        std::map<OperationType, std::vector<telux::common::InitResponseCb>> &initCbs,
        telux::common::ServiceStatus status, OperationType oprType);

    std::map<SlotId, std::weak_ptr<IDataProfileManager>> dataProfileManagerMap_;
    std::map<SlotId, std::weak_ptr<IDataConnectionManager>> dataConnectionManagerMap_;
    std::map<SlotId, std::weak_ptr<IServingSystemManager>> dataServingSystemManagerMap_;
    std::map<telux::data::OperationType, std::weak_ptr<IDataSettingsManager>>
        dataSettingsManagerMap_;
    std::map<SlotId, std::weak_ptr<IDataFilterManager>> dataFilterManagerMap_;
    std::map<telux::data::OperationType, std::weak_ptr<telux::data::net::ISocksManager>>
        socksManagerMap_;
    std::map<telux::data::OperationType, std::weak_ptr<telux::data::net::INatManager>>
        natManagerMap_;
    std::weak_ptr<telux::data::net::IL2tpManager> l2tpManager_;
    std::weak_ptr<telux::data::net::IBridgeManager> bridgeManager_;
    std::map<telux::data::OperationType, std::weak_ptr<telux::data::net::IFirewallManager>>
        firewallManagerMap_;
    std::map<telux::data::OperationType, std::weak_ptr<telux::data::net::IVlanManager>>
        vlanManagerMap_;
    std::weak_ptr<telux::data::IDualDataManager> dualDataManager_;
    std::weak_ptr<telux::data::IDataControlManager> dataControlManager_;
    std::weak_ptr<telux::data::IDataLinkManager> dataLinkManager_;
    std::weak_ptr<telux::data::net::IQoSManager> qosManager_;
	std::weak_ptr<telux::data::IKeepAliveManager> KeepAliveManager_;

    std::map<SlotId, std::vector<telux::common::InitResponseCb>> dataProfileCallbacks_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> servingSystemCallbacks_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> dataConnectionCallbacks_;
    std::map<OperationType, std::vector<telux::common::InitResponseCb>> dataSettingsCallbacks_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> dataFilterCallbacks_;
    std::vector<telux::common::InitResponseCb> socksCallbacks_;
    std::vector<telux::common::InitResponseCb> natCallbacks_;
    std::vector<telux::common::InitResponseCb> l2tpCallbacks_;
    std::vector<telux::common::InitResponseCb> bridgeCallbacks_;
    std::vector<telux::common::InitResponseCb> firewallCallbacks_;
    std::vector<telux::common::InitResponseCb> vlanCallbacks_;
    std::vector<telux::common::InitResponseCb> dualDataCallbacks_;
    std::vector<telux::common::InitResponseCb> dataControlCallbacks_;
    std::vector<telux::common::InitResponseCb> dataLinkCallbacks_;
    std::vector<telux::common::InitResponseCb> qosCallbacks_;
	std::vector<telux::common::InitResponseCb> keepAliveCallbacks_;
};

}  // namespace data
}  // namespace telux

#endif  // DATA_FACTORY_IMPL_STUB_HPP
