/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       ImsServingSystemManagerStub.hpp
 *
 * @brief      Implementation of ImsServingSystemManager
 *
 */

#ifndef IMS_SERVING_SYSTEM_MANAGER_STUB_HPP
#define IMS_SERVING_SYSTEM_MANAGER_STUB_HPP

#include <telux/tel/ImsServingSystemManager.hpp>

#include "common/event-manager/ClientEventManager.hpp"
#include "common/ListenerManager.hpp"

#include "protos/proto-src/tel_simulation.grpc.pb.h"

namespace telux {
namespace tel {

class ImsServingSystemManagerStub : public IImsServingSystemManager,
                                    public IEventListener,
                                    public std::enable_shared_from_this<ImsServingSystemManagerStub> {
public:
    ImsServingSystemManagerStub(SlotId slotId);
    telux::common::Status init(telux::common::InitResponseCb callback);
    ~ImsServingSystemManagerStub();

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener
        (std::weak_ptr<IImsServingSystemListener> listener) override;
    telux::common::Status
       deregisterListener(std::weak_ptr<telux::tel::IImsServingSystemListener> listener) override;

    telux::common::Status requestRegistrationInfo(ImsRegistrationInfoCb callback) override;

    telux::common::Status requestServiceInfo(ImsServiceInfoCb callback) override;
    telux::common::Status requestPdpStatus(ImsPdpStatusInfoCb callback) override;

    void cleanup();
    void onEventUpdate(google::protobuf::Any event)  override;

private:
    int phoneId_;
    std::mutex mtx_;
    telux::common::InitResponseCb initCb_;
    int cbDelay_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<telux::common::ListenerManager<IImsServingSystemListener>> listenerMgr_;
    std::unique_ptr<::telStub::ImsServingSystem::Stub> stub_;
    telux::common::ServiceStatus subSystemStatus_;
    void setServiceStatus(telux::common::ServiceStatus status);
    void initSync();
    void handleImsRegStatusChanged(::telStub::ImsRegStatusChangeEvent event);
    void handleImsServiceInfoChanged(::telStub::ImsServiceInfoChangeEvent event);
    void handleImsPdpStatusInfoChanged(::telStub::ImsPdpStatusInfoChangeEvent event);
};

} // end of namespace tel

} // end of namespace telux

#endif // IMS_SERVING_SYSTEM_MANAGER_STUB_HPP