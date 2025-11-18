/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       PhoneManagerStub.hpp
 *
 * @brief      Implementation of PhoneManager on client side
 *
 */

#ifndef TELUX_TEL_PHONEMANAGERSTUB_HPP
#define TELUX_TEL_PHONEMANAGERSTUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneManager.hpp>
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "common/AsyncTaskQueue.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/ListenerManager.hpp"
#include "PhoneStub.hpp"
#include "TelDefinesStub.hpp"

using telStub::PhoneService;
using telStub::CardService;

namespace telux {
namespace tel {

class PhoneManagerStub : public IPhoneManager,
                         public IEventListener,
                         public std::enable_shared_from_this<PhoneManagerStub> {
public:

    PhoneManagerStub();
    telux::common::Status init(telux::common::InitResponseCb callback);
    ~PhoneManagerStub();
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status getPhoneIds(std::vector<int> &phoneIds);
    int getPhoneIdFromSlotId(int slotId);
    int getSlotIdFromPhoneId(int phoneId);
    std::shared_ptr<IPhone> getPhone(int phoneId);
    telux::common::Status registerListener(std::weak_ptr<IPhoneListener> listener);
    telux::common::Status removeListener(std::weak_ptr<IPhoneListener> listener);
    telux::common::Status requestCellularCapabilityInfo(
        std::shared_ptr<ICellularCapabilityCallback> callback = nullptr) override;
    telux::common::Status setOperatingMode(telux::tel::OperatingMode operatingMode,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestOperatingMode(std::shared_ptr<IOperatingModeCallback> callback
        = nullptr) override;
    telux::common::Status resetWwan(telux::common::ResponseCallback callback
        = nullptr) override;
    void onEventUpdate(google::protobuf::Any event)  override;

private:
    int noOfSlots_;
    bool ready_ = false;
    std::condition_variable cv_;
    std::vector<int> phoneIds_;
    std::map<int, std::shared_ptr<PhoneStub>> phoneMap_;
    std::map<int, int> phoneSlotIdsMap_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::mutex phoneManagerMutex_;
    telux::common::InitResponseCb initCb_;
    int cbDelay_;
    std::shared_ptr<telux::common::ListenerManager<IPhoneListener>> listenerMgr_;
    std::unique_ptr<::telStub::PhoneService::Stub> phoneStub_;
    std::unique_ptr<::telStub::CardService::Stub> cardStub_;
    telux::common::ServiceStatus subSystemStatus_;
    void setServiceStatus(telux::common::ServiceStatus status);
    void initSync();
    void setSubsystemReady(bool status);
    bool waitForInitialization();
    void handleSignalStrengthChanged(::telStub::SignalStrengthChangeEvent event);
    void handleCellInfoListChanged(::telStub::CellInfoListEvent event);
    void handleVoiceServiceStateChanged(::telStub::VoiceServiceStateEvent event);
    void handleOperatingModeChanged(::telStub::OperatingModeEvent event);
    void handleECallOperatingModeChanged(::telStub::ECallModeInfoChangeEvent event);
    void handleOperatorInfoChanged(::telStub::OperatorInfoEvent event);
    void handleVoiceRadioTechChanged(::telStub::VoiceRadioTechnologyChangeEvent event);
    void handleServiceStateChanged(::telStub::ServiceStateChangeEvent event);
    void onEventUpdate(std::string event);
    void updateRadioState(OperatingMode optMode);
};

} // end of namespace tel
} // end of namespace telux

#endif // TELUX_TEL_PHONEMANAGERSTUB_HPP
