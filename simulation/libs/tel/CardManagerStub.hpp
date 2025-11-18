/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       CardManagerStub.hpp
 *
 * @brief      Implementation of ICardManager
 *
 */

#ifndef CARD_MANAGER_STUB_HPP
#define CARD_MANAGER_STUB_HPP

#include "CardStub.hpp"
#include "common/Logger.hpp"
#include "common/ListenerManager.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "TelDefinesStub.hpp"
#include <telux/tel/CardManager.hpp>
#include <telux/common/CommonDefines.hpp>
#include "CardAppStub.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "common/event-manager/ClientEventManager.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using telStub::CardService;

#define INVALID_SLOT_COUNT -1
namespace telux {
namespace tel {


struct UserRefreshParam {
    bool isRegister = false;
    bool doVoting = false;
    std::vector<IccFile> efFiles;
    RefreshParams refreshParams;
};

class CardManagerStub : public ICardManager,
                        public IEventListener,
                        public std::enable_shared_from_this<CardManagerStub> {
public:
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status getSlotCount(int &count) override;
    telux::common::Status getSlotIds(std::vector<int> &slotIds) override;
    std::shared_ptr<ICard> getCard(int slotId = DEFAULT_SLOT_ID,
        telux::common::Status *status = nullptr) override;
    telux::common::Status cardPowerUp(SlotId slotId,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status cardPowerDown(SlotId slotId,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status setupRefreshConfig(
        SlotId slotId, bool isRegister, bool doVoting, std::vector<IccFile> efFiles,
        RefreshParams refreshParams, common::ResponseCallback callback) override;
    telux::common::Status allowCardRefresh(SlotId slotId, bool allowRefresh,
        RefreshParams refreshParams, telux::common::ResponseCallback callback) override;
    telux::common::Status confirmRefreshHandlingCompleted(SlotId slotId, bool isCompleted,
        RefreshParams refreshParams, telux::common::ResponseCallback callback)  override;
    telux::common::Status requestLastRefreshEvent(SlotId slotId,
        RefreshParams refreshParams, refreshLastEventResponseCallback callback) override;
    telux::common::Status registerListener(std::shared_ptr<ICardListener> listener) override;
    telux::common::Status removeListener(std::shared_ptr<ICardListener> listener) override;
    void onEventUpdate(google::protobuf::Any event) override;
    CardManagerStub();
    telux::common::Status init(telux::common::InitResponseCb callback);
    void cleanup();
    ~CardManagerStub();

private:
    int cbDelay_;
    int slotCount_ = INVALID_SLOT_COUNT;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_ = nullptr;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<ICardListener>> listenerMgr_ = nullptr;
    std::unique_ptr<::telStub::CardService::Stub> stub_;
    telux::common::ServiceStatus subSystemStatus_;
    std::mutex cardManagerMutex_;
    std::condition_variable cardManagerInitCV_;
    bool ready_ = false;
    std::vector<int> simSlotIds_;
    pid_t myPid_;
    std::vector<UserRefreshParam> userRefreshParams_;

    bool waitForInitialization();
    void setSubsystemReady(bool status);
    void setServiceStatus(telux::common::ServiceStatus status);
    void initSync();
    std::map<int, std::shared_ptr<CardStub>> cardMap_;
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay );
    void handleEvent(std::string token, std::string event);
    void invokelisteners (int slotId);
    void handleCardInfoChanged(::telStub::cardInfoChange event);
    void handleRefreshEvent(::telStub::RefreshEvent event);
    void setRpcRefreshParams(::telStub::RefreshParams* refreshs,
        const RefreshParams refreshParams);
    void convertRefreshParams(const RefreshParams userParams, RefreshParams& refreshParams);
    SlotId getSlotBySessionType(telux::tel::SessionType st);
    void findRefreshParams(const RefreshParams& refreshParams, bool& isRegister, bool* doVoting,
        std::vector<IccFile>* efFiles);
};

} // end of namespace tel

} // end of namespace telux



#endif // CARD_MANAGER_STUB_HPP
