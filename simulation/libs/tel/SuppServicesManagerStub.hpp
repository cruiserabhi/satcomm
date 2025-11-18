/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SuppServicesManagerStub.hpp
 *
 * @brief      Implementation of SuppServicesManager
 *
 */

#ifndef SUPL_SERVICES_MANAGER_STUB_HPP
#define SUPP_SERVICES_MANAGER_STUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SuppServicesManager.hpp>

#include "common/event-manager/ClientEventManager.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"

namespace telux {
namespace tel {

class SuppServicesManagerStub : public ISuppServicesManager,
                                public IEventListener,
                                public std::enable_shared_from_this<SuppServicesManagerStub> {
public:
    SuppServicesManagerStub(SlotId slotId);
    telux::common::Status init(telux::common::InitResponseCb callback);
    ~SuppServicesManagerStub();

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(std::weak_ptr<ISuppServicesListener> listener) override;
    telux::common::Status
        removeListener(std::weak_ptr<telux::tel::ISuppServicesListener> listener) override;

    telux::common::Status setCallWaitingPref(SuppServicesStatus suppSvcStatus,
        SetSuppSvcPrefCallback callback = nullptr) override;
    telux::common::Status requestCallWaitingPref(GetCallWaitingPrefExCb callback) override;
    telux::common::Status setForwardingPref(ForwardReq forwardReq,
        SetSuppSvcPrefCallback callback = nullptr) override;
    telux::common::Status requestForwardingPref(ServiceClass serviceClass,
        ForwardReason reason, GetForwardingPrefExCb callback) override;
    telux::common::Status setOirPref(ServiceClass serviceClass,
        SuppServicesStatus suppSvcStatus, SetSuppSvcPrefCallback callback = nullptr) override;
    telux::common::Status requestOirPref(ServiceClass serviceClass,
        GetOirPrefCb callback) override;
    telux::common::Status requestCallWaitingPref(GetCallWaitingPrefCb callback) override;
    telux::common::Status requestForwardingPref(ServiceClass serviceClass,
        ForwardReason reason, GetForwardingPrefCb callback) override;

    void onServiceStatusChange(telux::common::ServiceStatus status);
    void cleanup();

private:
    int slotId_ = 0;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<telux::common::ListenerManager<ISuppServicesListener>> listenerMgr_;
    std::unique_ptr<::telStub::SuppServicesService::Stub> suppServiceStub_;
    void initSync(telux::common::InitResponseCb callback);
};

} // end of namespace tel
} // end of namespace telux

#endif // SUPP_SERVICES_MANAGER_STUB_HPP
