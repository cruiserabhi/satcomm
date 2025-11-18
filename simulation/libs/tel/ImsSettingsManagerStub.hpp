/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       ImsSettingsManagerStub.hpp
 *
 * @brief      Implementation of ImsSettingsManager
 *
 */

#ifndef IMS_SETTINGS_MANAGER_STUB_HPP
#define IMS_SETTINGS_MANAGER_STUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/ImsSettingsManager.hpp>

#include "common/event-manager/ClientEventManager.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"

namespace telux {
namespace tel {

class ImsSettingsManagerStub : public IImsSettingsManager,
                               public IEventListener,
                               public std::enable_shared_from_this<ImsSettingsManagerStub> {
public:
    ImsSettingsManagerStub();
    telux::common::Status init(telux::common::InitResponseCb callback);
    ~ImsSettingsManagerStub();

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(std::weak_ptr<IImsSettingsListener> listener) override;
    telux::common::Status
        deregisterListener(std::weak_ptr<telux::tel::IImsSettingsListener> listener) override;

    telux::common::Status
        requestServiceConfig(SlotId slotId, ImsServiceConfigCb callback) override;
    telux::common::Status
        requestSipUserAgent(SlotId slotId, ImsSipUserAgentConfigCb callback) override;
    telux::common::Status setSipUserAgent(SlotId slotId,
        std::string userAgent, telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status setServiceConfig(SlotId slotId,
        ImsServiceConfig config, telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestVonrStatus(SlotId slotId, ImsVonrStatusCb callback) override;
    telux::common::Status toggleVonr(SlotId slotId,
        bool isEnable, common::ResponseCallback callback = nullptr) override;
    void onServiceStatusChange(telux::common::ServiceStatus status);

    void cleanup();
    void onEventUpdate(google::protobuf::Any event)  override;

private:
    int noOfSlots_ = 0;
    std::mutex mtx_;
    telux::common::InitResponseCb initCb_;
    int cbDelay_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<telux::common::ListenerManager<IImsSettingsListener>> listenerMgr_;
    std::unique_ptr<::telStub::ImsService::Stub> stub_;
    telux::common::ServiceStatus subSystemStatus_;
    void setServiceStatus(telux::common::ServiceStatus status);
    void initSync();
    void handleImsServiceConfigsChange(::telStub::ImsServiceConfigsChangeEvent event);
    void handleImsSipUserAgentChange(::telStub::ImsSipUserAgentChangeEvent event);
};

} // end of namespace tel
} // end of namespace telux

#endif // IMS_SETTINGS_MANAGER_STUB_HPP
