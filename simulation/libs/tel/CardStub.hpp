/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/**
 * @file       CardStub.hpp
 *
 * @brief      Implementation of ICard
 *
 */

#ifndef CARD_STUB_HPP
#define CARD_STUB_HPP

#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include <telux/tel/CardManager.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include "CardFileHandlerStub.hpp"
#include "CardAppStub.hpp"
#include <telux/common/CommonDefines.hpp>
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using telStub::CardService;

namespace telux {
namespace tel {

class CardStub : public ICard {
public:
    telux::common::Status getState(CardState &cardState) override;
    std::vector<std::shared_ptr<ICardApp>> getApplications(
        telux::common::Status *status = nullptr) override;
    telux::common::Status openLogicalChannel(std::string applicationId,
        std::shared_ptr<ICardChannelCallback> callback = nullptr) override;
    telux::common::Status closeLogicalChannel(
    int channelId, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
        override;
    telux::common::Status transmitApduLogicalChannel(int channel, uint8_t cla,
        uint8_t instruction, uint8_t p1, uint8_t p2, uint8_t p3, std::vector<uint8_t> data,
        std::shared_ptr<ICardCommandCallback> callback = nullptr) override;
    telux::common::Status transmitApduBasicChannel(uint8_t cla, uint8_t instruction,
        uint8_t p1, uint8_t p2, uint8_t p3, std::vector<uint8_t> data,
        std::shared_ptr<ICardCommandCallback> callback = nullptr) override;
    telux::common::Status exchangeSimIO(uint16_t fileId, uint8_t command, uint8_t p1,
        uint8_t p2, uint8_t p3, std::string filePath, std::vector<uint8_t> data, std::string pin2,
        std::string aid, std::shared_ptr<ICardCommandCallback> callback = nullptr) override;
    int getSlotId() override;
    telux::common::Status requestEid(EidResponseCallback callback) override;
    std::shared_ptr<ICardFileHandler> getFileHandler() override;
    bool isNtnProfileActive() override;
    void updateSimStatus();
    void setlisteners(std::vector<std::weak_ptr<ICardListener>> listeners);
    std::vector<std::weak_ptr<ICardListener>> listeners_;
    void cleanup();
    CardStub(int slotId);
    ~CardStub();

private:
    void invokeCallback(std::shared_ptr<ICardCommandCallback> callback, int delay,
        telux::tel::IccResult iccresult, telux::common::ErrorCode error);
    void invokeCallback(std::shared_ptr<telux::common::ICommandResponseCallback> callback,
        int delay, telux::common::ErrorCode error);
    void invokeCallback(EidResponseCallback callback, std::string eid, int delay,
        telux::common::ErrorCode error);
    std::vector<std::shared_ptr<CardAppStub>> cardApps_;
    std::shared_ptr<CardFileHandlerStub> cardFileHandler_ = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICardApp>> applications_;
    int slotId_;
    std::unique_ptr<::telStub::CardService::Stub> stub_;
    bool validateAppId(std::string applicationId);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    void invokeCallback(std::shared_ptr<ICardChannelCallback> callback, int channel,
    IccResult result, telux::common::ErrorCode error, int delay);
    std::mutex cardMutex_;
};

} // end of namespace tel

} // end of namespace telux



#endif // CARD_STUB_HPP
