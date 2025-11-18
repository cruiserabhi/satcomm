/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       SmsManagerStub.hpp
 *
 * @brief      Implementation of SmsManager
 *
 */

#ifndef SMS_MANAGER_STUB_HPP
#define SMS_MANAGER_STUB_HPP

#include "common/Logger.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include <telux/tel/SmsManager.hpp>
#include "common/ListenerManager.hpp"
#include <jsoncpp/json/json.h>
#include <list>
#include "TelDefinesStub.hpp"
#include "Helper.hpp"
#include "common/JsonParser.hpp"
#include <telux/common/CommonDefines.hpp>
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using telStub::SmsService;

#define INVALID -1

struct smsDeliveryInfo {
    telux::common::ErrorCode errorCode;
    int cbDelay;
    int msgRef;
};

namespace telux {
namespace tel {

struct MessageMetaData {
    std::string senderAddress;
    uint16_t refNumber;
    // overload '<' operator in order to use MessageMetaData as key in std::map
        bool operator < (const MessageMetaData &mp) const {
            return refNumber < mp.refNumber ||
                (refNumber == mp.refNumber && senderAddress < mp.senderAddress);
        }
};

class SmsManagerStub : public ISmsManager,
                       public IEventListener,
                       public std::enable_shared_from_this<SmsManagerStub> {
public:

    SmsManagerStub(int phoneId);
    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(std::weak_ptr<ISmsListener> listener) override;

    telux::common::Status removeListener(std::weak_ptr<ISmsListener> listener) override;

    telux::common::Status sendSms(const std::string &message, const std::string &receiverAddress,
        std::shared_ptr<telux::common::ICommandResponseCallback> sentCallback = nullptr,
        std::shared_ptr<telux::common::ICommandResponseCallback> deliveryCallback = nullptr)
        override;

    telux::common::Status sendRawSms(const std::vector<PduBuffer> rawPdus,
      SmsResponseCb sentCallback = nullptr) override;

    telux::common::Status requestSmscAddress(std::shared_ptr<ISmscAddressCallback> callback
        = nullptr) override;

    telux::common::Status setSmscAddress(const std::string &smscAddress,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestSmsMessageList(SmsTagType type,
      RequestSmsInfoListCb callback) override;

    telux::common::Status readMessage(uint32_t messageIndex, ReadSmsMessageCb callback) override;

    telux::common::Status deleteMessage(DeleteInfo info,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestPreferredStorage(RequestPreferredStorageCb callback) override;

    telux::common::Status setPreferredStorage(StorageType storageType,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status setTag(uint32_t msgIndex, SmsTagType tagType,
        telux::common::ResponseCallback callback = nullptr)override;

    telux::common::Status requestStorageDetails(RequestStorageDetailsCb callback) override;

    int getPhoneId() override;

    MessageAttributes calculateMessageAttributes(const std::string &message) override;

    telux::common::Status sendSms(std::string message, std::string receiverAddress,
      bool deliveryReportNeeded = true, SmsResponseCb sentCallback = nullptr,
      std::string smscAddr = "") override;

    ~SmsManagerStub();
    void cleanup();
    void onEventUpdate(google::protobuf::Any);

private:
    int phoneId_ = INVALID;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::mutex mtx_;
    telux::common::InitResponseCb initCb_;
    int cbDelay_;
    std::shared_ptr<telux::common::ListenerManager<ISmsListener>> listenerMgr_;
    std::unique_ptr<::telStub::SmsService::Stub> stub_;
    telux::common::ServiceStatus subSystemStatus_;
    void setServiceStatus(telux::common::ServiceStatus status);
    void initSync();
    void invokeCallback(int cbDelay, ErrorCode error, std::vector<int> msgRefs,
         SmsResponseCb sentCallback = nullptr);
    void invokeDeliveryReportListener(std::string receiverAddress, int noofdeliveryreport,
         std::vector<int> refs, telux::common::ErrorCode error, int deliveryCallbackDelay);
    void invokeDeliveryReportListener(std::string receiverAddress,
    int noofdeliveryreport, std::vector<smsDeliveryInfo> infos);
    void invokesendSmsCallback(int cbDelay,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback,
        telux::common::ErrorCode error);
    void invokeRequestSmsInfoListCb(std::vector<SmsMetaInfo> infos,
        telux::common::ErrorCode error, RequestSmsInfoListCb callback, int cbDelay );
    void invokeResponseCallback(int cbDelay, telux::common::ErrorCode error,
        telux::common::ResponseCallback callback);
    void invokeGetSmscCallback(int cbDelay, std::shared_ptr<ISmscAddressCallback> callback,
        std::string smscAddress, telux::common::ErrorCode error);
    void invokeReadSmsMessageCb(SmsMessage message, telux::common::ErrorCode error,
        ReadSmsMessageCb callback, int cbDelay );
    void invokeRequestPreferredStorageCb(telux::tel::StorageType storageType,
        int cbDelay, telux::common::ErrorCode error, RequestPreferredStorageCb callback);
    void invokeRequestStorageDetailsCb(int maxCount, int availableCount,
        int cbDelay, telux::common::ErrorCode error, RequestStorageDetailsCb callback);
    void handleEvent(std::string token, std::string event);
    void handleMemoryFullEvent(::telStub::memoryFullEvent event);
    void handleIncomingSms(::telStub::SmsMessage event );
    void invokeIncomingSmslisteners (int phoneId, std::shared_ptr<SmsMessage> message);
    void invokeMemoryFulllisteners(int phoneId, telux::tel::StorageType type);
    void invokeIncomingSmslisteners(int phoneId,
        std::shared_ptr<std::vector<SmsMessage>> messages);
    void isMemoryFull(int phoneId);
    void parseAndConcatenateSmsMessage (int phoneId, SmsMessage &msg);
    std::map<MessageMetaData, std::vector<SmsMessage>> smsMessageMap_;
    void onEventUpdate(std::string event);
};

} // end of namespace tel

} // end of namespace telux

#endif // SMS_MANAGER_STUB_HPP