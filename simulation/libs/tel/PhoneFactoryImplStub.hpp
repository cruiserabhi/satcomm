/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       PhoneFactoryImplStub.hpp
 *
 * @brief      Implementation of PhoneFactory
 *
 */

#ifndef TEL_FACTORY_STUB_HPP
#define TEL_FACTORY_STUB_HPP

#include <telux/tel/PhoneFactory.hpp>

#include "CardManagerStub.hpp"
#include "SubscriptionManagerStub.hpp"
#include "PhoneManagerStub.hpp"
#include "SmsManagerStub.hpp"
#include "CellBroadcastManagerStub.hpp"
#include "CallManagerStub.hpp"
#include "MultiSimManagerStub.hpp"
#include "ImsServingSystemManagerStub.hpp"
#include "ImsSettingsManagerStub.hpp"
#include "ServingSystemManagerStub.hpp"
#include "NetworkSelectionManagerStub.hpp"
#include "SuppServicesManagerStub.hpp"

namespace telux {
namespace tel {

class PhoneFactoryImplStub : public PhoneFactory {
 public:
    static PhoneFactory &getInstance();

    virtual std::shared_ptr<IPhoneManager> getPhoneManager(
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ISmsManager> getSmsManager(int phoneId = DEFAULT_PHONE_ID,
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ICallManager> getCallManager(telux::common::InitResponseCb
        callback = nullptr) override;
    virtual std::shared_ptr<ICardManager> getCardManager(telux::common::InitResponseCb
        callback = nullptr) override;
    virtual std::shared_ptr<ISapCardManager> getSapCardManager(int slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ISubscriptionManager> getSubscriptionManager(
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<IServingSystemManager> getServingSystemManager(
        int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<INetworkSelectionManager> getNetworkSelectionManager(
        int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IRemoteSimManager> getRemoteSimManager(int slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IMultiSimManager> getMultiSimManager(
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ICellBroadcastManager> getCellBroadcastManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<ISimProfileManager> getSimProfileManager(
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IImsSettingsManager> getImsSettingsManager(
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IEcallManager> getEcallManager(
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<IHttpTransactionManager> getHttpTransactionManager(
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IImsServingSystemManager> getImsServingSystemManager(SlotId slotId,
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ISuppServicesManager> getSuppServicesManager(
        SlotId slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr)
            override;
    virtual std::shared_ptr<IApSimProfileManager> getApSimProfileManager(
        telux::common::InitResponseCb  callback = nullptr) override;

 private:
    PhoneFactoryImplStub();
    ~PhoneFactoryImplStub();
    std::map<int, std::shared_ptr<ICellBroadcastManager>> cbMap_;
    std::map<int, std::shared_ptr<ISmsManager>> smsManagerMap_;
    std::map<SlotId, std::shared_ptr<IImsServingSystemManager>> imsServSysManagerMap_;
    std::map<int, std::shared_ptr<IServingSystemManager>> servingSystemManagerMap_;
    std::map<int, std::shared_ptr<INetworkSelectionManager>> networkSelectionManagerMap_;
    std::shared_ptr<ICardManager> cardManager_;
    std::shared_ptr<IPhoneManager> phoneManager_;
    std::shared_ptr<ISubscriptionManager> subscriptionManager_;
    std::shared_ptr<ICallManager> callManager_;
    std::shared_ptr<IMultiSimManager> multiSimManager_;
    std::shared_ptr<IImsSettingsManager> imsSettingsManager_;
    std::map<int, std::shared_ptr<ISuppServicesManager>> suppSvcManagerMap_;
    std::vector<telux::common::InitResponseCb> cardMgrCallbacks_;
    std::vector<telux::common::InitResponseCb> phoneMgrCallbacks_;
    std::vector<telux::common::InitResponseCb> subscriptionMgrCallbacks_;
    std::vector<telux::common::InitResponseCb> multiSimMgrCallbacks_;
    telux::common::ServiceStatus subscriptionMgrInitStatus_;
    telux::common::ServiceStatus phoneMgrInitStatus_;
    telux::common::ServiceStatus multiSimMgrInitStatus_;
    std::map<int, std::vector<telux::common::InitResponseCb>> smsMgrCallbacks_;
    std::map<int, std::vector<telux::common::InitResponseCb>> cbMgrCallbacks_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> imsServSysCallbacks_;
    std::map<int, std::vector<telux::common::InitResponseCb>> servingSysMgrCallbacks_;
    std::map<int, std::vector<telux::common::InitResponseCb>> networkSelMgrCallbacks_;
    std::vector<telux::common::InitResponseCb> imssCallbacks_;
    std::map<int, std::vector<telux::common::InitResponseCb>> suppSvcCallbacks_;
    telux::common::ServiceStatus cardMgrInitStatus_;
    std::map<int, telux::common::ServiceStatus> smsMgrInitStatus_;
    std::map<int, telux::common::ServiceStatus> cbMgrInitStatus_;
    std::map<int, telux::common::ServiceStatus> imsServingSystemMgrInitStatus_;
    std::map<int, telux::common::ServiceStatus> servingSysMgrInitStatus_;
    std::map<int, telux::common::ServiceStatus> networkSelMgrInitStatus_;
    telux::common::ServiceStatus imssInitStatus_;
    std::map<int, telux::common::ServiceStatus> suppSvcInitStatus_;
    std::recursive_mutex mutex_;
    common::ServiceStatus callMgrInitStatus_ = common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::vector<telux::common::InitResponseCb> callMgrInitCallbacks_;
    void onCallMgrInitResponse(telux::common::ServiceStatus status);
    void onCardManagerResponse(telux::common::ServiceStatus status);
    void onSubscriptionManagerResponse(telux::common::ServiceStatus status);
    void onCellBroadcastManagerResponse(SlotId slotId, telux::common::ServiceStatus status);
    void onSmsMgrInitResponse(int phoneId, telux::common::ServiceStatus status);
    void onImsServingSystemMgrInitResponse(SlotId slotId, telux::common::ServiceStatus status);
    void onServingSystemMgrInitResponse(int slotId, telux::common::ServiceStatus status);
    void onPhoneManagerResponse(telux::common::ServiceStatus status);
    void onMultiSimManagerResponse(telux::common::ServiceStatus status);
    void onNetworkSelectionMgrInitResponse(int slotId, telux::common::ServiceStatus status);
    void onImsSettingsManagerResponse(telux::common::ServiceStatus status);
    void onSuppSvcInitResponse(SlotId slotId, telux::common::ServiceStatus status);
};

}  // namespace tel
}  // namespace telux

#endif  // TEL_FACTORY_STUB_HPP
