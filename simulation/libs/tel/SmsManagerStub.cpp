/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string>
#include <future>
#include <exception>
#include <algorithm>
#include <bits/stdc++.h>
#include "SmsManagerStub.hpp"
#include "common/SimulationConfigParser.hpp"
#include "common/CommonUtils.hpp"
#include "common/event-manager/ClientEventManager.hpp"

using namespace telux::common;
using namespace telux::tel;
using namespace std;

SmsManagerStub::SmsManagerStub(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    phoneId_ = phoneId;
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_ = DEFAULT_DELAY;
}

void SmsManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
        std::lock_guard<std::mutex> lock(mtx_);
        subSystemStatus_ = status;
    }
    if(initCb_) {
        auto f1 = std::async(std::launch::async,
        [this, status]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay_));
                initCb_(status);
        }).share();
        taskQ_->add(f1);
    } else {
        LOG(ERROR, __FUNCTION__, " Callback is NULL");
    }
}

telux::common::Status SmsManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ISmsListener>>();
    if(!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    stub_ = CommonUtils::getGrpcStub<SmsService>();
    if(!stub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate sms service");
        return telux::common::Status::FAILED;
    }
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    if(!taskQ_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate AsyncTaskQueue");
        return telux::common::Status::FAILED;
    }
    initCb_ = callback;
    auto f = std::async(std::launch::async,
        [this]() {
            this->initSync();
        }).share();
    auto status = taskQ_->add(f);
    return status;
}

void SmsManagerStub::initSync() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;
    request.set_phone_id(phoneId_);
    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " InitService request failed");
    } else {
        cbStatus = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay_ = static_cast<int>(response.delay());
    }
    LOG(DEBUG, __FUNCTION__, " callback delay ", cbDelay_,
        " callback status ", static_cast<int>(cbStatus));
    setServiceStatus(cbStatus);
}

SmsManagerStub::~SmsManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void SmsManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__, " PhoneId: ", phoneId_);
}

telux::common::ServiceStatus SmsManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status SmsManagerStub::registerListener(std::weak_ptr<ISmsListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_SMS_FILTER};
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(shared_from_this(), filters);
    }
    return status;
}

telux::common::Status SmsManagerStub::removeListener(
        std::weak_ptr<ISmsListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ISmsListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_SMS_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

telux::common::Status SmsManagerStub::sendSms(const std::string &message,
    const std::string &receiverAddress,
    std::shared_ptr<telux::common::ICommandResponseCallback> sentCallback,
    std::shared_ptr<telux::common::ICommandResponseCallback> deliveryCallback) {
    LOG(DEBUG, __FUNCTION__);
    bool isDeliveryReportNeeded = true;

    if (message.empty() || receiverAddress.empty()) {
      LOG(ERROR, __FUNCTION__, " Either message or receiver address is empty");
      return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
      LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
      return telux::common::Status::NOTREADY;
    }

    if (!sentCallback) {
      LOG(DEBUG, __FUNCTION__, " Sent callback is null");
    }
    if (!deliveryCallback) {
      LOG(DEBUG, __FUNCTION__, " Delivery callback is null");
      isDeliveryReportNeeded = false;
    }
    ::telStub::SendSmsWithoutSmscRequest request;
    ::telStub::SendSmsWithoutSmscReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->SendSmsWithoutSmsc(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::common::ErrorCode sentCallbackErrorcode =
    static_cast<telux::common::ErrorCode>(response.sentcallback_errorcode());
    int noofsegments = static_cast<int>(response.noofsegments());
    int sentCallbackDelay = static_cast<int>(response.sentcallback_callbackdelay());
    std::string ref = static_cast<std::string>(response.sentcallback_msgrefs());
    std::vector<int> refs = CommonUtils::convertStringToVector(ref);
    telux::common::ErrorCode deliveryCallbackErrorCode =
        static_cast<telux::common::ErrorCode>(response.deliverycallback_errorcode());
    int deliveryCallbackDelay = static_cast<int>(response.deliverycallback_callbackdelay());

    LOG(DEBUG, __FUNCTION__, " Invoking callback for old SMS API");
    // Sending the callback response.
    if (status == telux::common::Status::SUCCESS) {
        auto f1 = std::async(std::launch::async,
            [this, sentCallbackDelay, sentCallback, sentCallbackErrorcode]() {
                this->invokesendSmsCallback(sentCallbackDelay, sentCallback,
                    sentCallbackErrorcode);
            }).share();
            taskQ_->add(f1);
        // Send delivery report to listeners.
        if((isDeliveryReportNeeded) &&
            (sentCallbackErrorcode == telux::common::ErrorCode::SUCCESS)) {
            LOG(DEBUG, __FUNCTION__, " Invoking delivery report to listeners");
            auto f2 = std::async(std::launch::async,
            [this, receiverAddress, noofsegments, refs,
                deliveryCallbackErrorCode, deliveryCallbackDelay]() {
                this->invokeDeliveryReportListener(receiverAddress, noofsegments,
                refs, deliveryCallbackErrorCode, deliveryCallbackDelay );
            }).share();
            taskQ_->add(f2);

            // Sending the delivery callback Response.
            LOG(DEBUG, __FUNCTION__, " Invoking delivery callback");
            auto f3 = std::async(std::launch::async,
            [this, deliveryCallbackDelay, deliveryCallback, deliveryCallbackErrorCode]() {
                this->invokesendSmsCallback(deliveryCallbackDelay,
                    deliveryCallback, deliveryCallbackErrorCode);
            }).share();
            taskQ_->add(f3);
        }
    }
    return status;
}

telux::common::Status SmsManagerStub::sendSms(std::string message, std::string receiverAddress,
    bool deliveryReportNeeded, SmsResponseCb sentCallback, std::string smscAddr) {
    LOG(DEBUG, __FUNCTION__);
    if (message.empty() || receiverAddress.empty()) {
        LOG(ERROR, __FUNCTION__, " either message or receiver address is empty");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    if (!sentCallback) {
        LOG(DEBUG, __FUNCTION__, " Sent callback is null");
    }
    ::telStub::SendSmsRequest request;
    ::telStub::SendSmsReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->SendSms(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int noofsegments = static_cast<int>(response.noofsegments());
    telux::common::ErrorCode smsResponsecbErrorCode =
    static_cast<telux::common::ErrorCode>(response.smsresponsecb_errorcode());
    int smsResponseCbDelay = static_cast<int>(response.smsresponsecb_callbackdelay());
    std::string ref = static_cast<std::string>(response.sentcallback_msgrefs());
    std::vector<int> refs = CommonUtils::convertStringToVector(ref);
    std::vector<smsDeliveryInfo> infos;

    for (int i = 0; i < response.records_size(); i++) {
        smsDeliveryInfo info;
        info.errorCode  = static_cast<telux::common::ErrorCode>
            (response.mutable_records(i)->ondeliveryreport_errorcode());
        info.cbDelay  =  static_cast<int>
            (response.mutable_records(i)->deliverycallbackdelay());
        info.msgRef = static_cast<int>(response.mutable_records(i)->ondeliveryreportmsgref());
        LOG(DEBUG, __FUNCTION__, "errorCode " ,
            static_cast<int>(info.errorCode), "cbDelay ", info.cbDelay,
            "msgRef " ,info.msgRef);
        infos.emplace_back(info);
    }

    if (status == telux::common::Status::SUCCESS) {
        // Invoking response callback
        auto f1 = std::async(std::launch::async,
        [this, smsResponseCbDelay, smsResponsecbErrorCode, refs, sentCallback]() {
            this->invokeCallback(smsResponseCbDelay,
            smsResponsecbErrorCode, refs, sentCallback);
        }).share();
        taskQ_->add(f1);

        // Notifying listeners about the change event.
        if((deliveryReportNeeded) &&
            (smsResponsecbErrorCode == telux::common::ErrorCode::SUCCESS)) {
            auto f2 = std::async(std::launch::async,
                [this, receiverAddress, noofsegments, infos]() {
                    this->invokeDeliveryReportListener(receiverAddress, noofsegments,
                    infos);
                }).share();
            taskQ_->add(f2);
        }
    }
    return status;
}

void SmsManagerStub::invokeDeliveryReportListener(std::string receiverAddress,
    int noofdeliveryreport, std::vector<smsDeliveryInfo> infos) {
    LOG(DEBUG, __FUNCTION__);
    for (int i = 0; i < noofdeliveryreport ;i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(infos[i].cbDelay));
        std::vector<std::weak_ptr<ISmsListener>> applisteners;
        if (listenerMgr_) {
            listenerMgr_->getAvailableListeners(applisteners);
            // Notify respective events
            for(auto &wp : applisteners) {
                if(auto sp = wp.lock()) {
                    sp->onDeliveryReport(phoneId_, infos[i].msgRef,
                        receiverAddress, infos[i].errorCode);
                }
            }
        } else {
            LOG(ERROR, __FUNCTION__, " listenerMgr is null");
        }
    }
}

void SmsManagerStub::invokeDeliveryReportListener(std::string receiverAddress,
    int noofdeliveryreport, std::vector<int> refs, telux::common::ErrorCode error,
    int deliveryCallbackDelay) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(deliveryCallbackDelay));
    std::vector<std::weak_ptr<ISmsListener>> applisteners;
        if (listenerMgr_) {
            listenerMgr_->getAvailableListeners(applisteners);
            // Notify respective events
            for(auto &wp : applisteners) {
                if(auto sp = wp.lock()) {
                    for (int i =0; i < noofdeliveryreport ;i++) {
                        sp->onDeliveryReport(phoneId_, refs[i], receiverAddress, error);
                    }
                }
            }
        } else {
            LOG(ERROR, __FUNCTION__, " listenerMgr is null");
        }
}

telux::common::Status SmsManagerStub::sendRawSms(const std::vector<PduBuffer> rawPdus,
    SmsResponseCb sentCallback) {
    LOG(DEBUG, __FUNCTION__);
    if (rawPdus.empty()) {
        LOG(ERROR, __FUNCTION__, " Raw PDU is empty");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    if (!sentCallback) {
        LOG(DEBUG, __FUNCTION__, " Sent callback is null");
    }

    ::telStub::SendRawSmsRequest request;
    ::telStub::SendRawSmsReply response;
    int size = rawPdus.size();
    ClientContext context;

    request.set_phone_id(phoneId_);
    request.set_size(size);

    grpc::Status reqstatus = stub_->SendRawSms(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int noofsegments = size;
    std::string receiverAddress = static_cast<std::string>(response.reciever_address());
    telux::common::ErrorCode smsResponsecbErrorCode =
    static_cast<telux::common::ErrorCode>(response.smsresponsecb_errorcode());
    int smsResponseCbDelay = static_cast<int>(response.smsresponsecb_callbackdelay());
    std::string ref = static_cast<std::string>(response.sentcallback_msgrefs());
    std::vector<int> refs = CommonUtils::convertStringToVector(ref);
    std::vector<smsDeliveryInfo> infos;

    for (int i = 0; i < response.records_size(); i++) {
        smsDeliveryInfo info;
        info.errorCode  =  static_cast<telux::common::ErrorCode>
            (response.mutable_records(i)->ondeliveryreport_errorcode());
        info.cbDelay  =  static_cast<int>
            (response.mutable_records(i)->deliverycallbackdelay());
        info.msgRef = static_cast<int>
            (response.mutable_records(i)->ondeliveryreportmsgref());
        LOG(DEBUG, __FUNCTION__, "errorCode " ,
            static_cast<int>(info.errorCode), "cbDelay ", info.cbDelay,
            "msgRef " ,info.msgRef);
        infos.emplace_back(info);
    }

    if (status == telux::common::Status::SUCCESS) {
        // Invoking response callback
        auto f1 = std::async(std::launch::async,
        [this, smsResponseCbDelay, smsResponsecbErrorCode, refs, sentCallback]() {
            this->invokeCallback(smsResponseCbDelay,
                smsResponsecbErrorCode, refs, sentCallback);
        }).share();
        taskQ_->add(f1);

        // Notifying listeners about the change event.
        if(smsResponsecbErrorCode == telux::common::ErrorCode::SUCCESS) {
            auto f2 = std::async(std::launch::async,
                [this, receiverAddress, noofsegments, infos]() {
                    this->invokeDeliveryReportListener(receiverAddress, noofsegments,
                    infos);
                }).share();
            taskQ_->add(f2);
        }
    }
    return status;
}
void SmsManagerStub::invokeCallback(int cbDelay, ErrorCode error, std::vector<int> msgRefs,
     SmsResponseCb sentCallback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    if (sentCallback) {
        sentCallback(msgRefs,error);
    }
}

void SmsManagerStub::invokesendSmsCallback(int cbDelay,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback,
    telux::common::ErrorCode error) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f1 = std::async(std::launch::async,
        [this, callback, error]() {
            callback->commandResponse(error);
        }).share();
    taskQ_->add(f1);
}

telux::common::Status SmsManagerStub::requestSmscAddress
    (std::shared_ptr<ISmscAddressCallback> callback) {
    LOG(DEBUG, __FUNCTION__);
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::GetSmscAddressRequest request;
    ::telStub::GetSmscAddressReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetSmscAddress(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    std::string smscAddress = static_cast<std::string>(response.smsc_address());
    LOG(DEBUG, __FUNCTION__, "smscAddress is ", smscAddress );

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, cbDelay, callback, smscAddress, error]() {
                this->invokeGetSmscCallback(cbDelay, callback, smscAddress, error);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeGetSmscCallback(int cbDelay,
    std::shared_ptr<ISmscAddressCallback> callback, std::string smscAddress,
    telux::common::ErrorCode error) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback->smscAddressResponse(smscAddress, error);
    }
}

telux::common::Status SmsManagerStub::setSmscAddress(const std::string &smscAddress,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " SlotId : ", phoneId_);
    if(smscAddress.empty()) {
        LOG(ERROR, __FUNCTION__, "  smscAddress address is empty");
        return telux::common::Status::INVALIDPARAM;
    }
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetSmscAddressRequest request;
    ::telStub::SetSmscAddressReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);
    request.set_number(smscAddress);

    grpc::Status reqstatus = stub_->SetSmscAddress(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            this->invokeResponseCallback(cbDelay, error, callback);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeResponseCallback(int cbDelay, telux::common::ErrorCode error,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(error);
    }
}

telux::common::Status SmsManagerStub::requestSmsMessageList(SmsTagType type,
    RequestSmsInfoListCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::RequestSmsMessageListRequest request;
    ::telStub::RequestSmsMessageListReply response;
    ClientContext context;
    ::telStub::SmsTagType_TagType tag =  static_cast<::telStub::SmsTagType_TagType>(type);
    request.set_phone_id(phoneId_);
    request.set_tag_type(tag);
    std::vector<SmsMetaInfo> infos;

    grpc::Status reqstatus = stub_->RequestSmsMessageList(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    for (int i = 0; i < response.meta_info_size(); i++) {
        telux::tel::SmsMetaInfo info;
        info.msgIndex  =  static_cast<int>(response.mutable_meta_info(i)->msg_index());
        info.tagType  =  static_cast<telux::tel::SmsTagType>
            (response.mutable_meta_info(i)->tag_type());
        LOG(DEBUG, __FUNCTION__, "msgIndex " ,info.msgIndex,
            "tagType ", static_cast<int>(info.tagType));
        infos.emplace_back(info);
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, infos ,error, callback, cbDelay]() {
                this->invokeRequestSmsInfoListCb(infos, error, callback, cbDelay);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeRequestSmsInfoListCb(std::vector<SmsMetaInfo> infos,
    telux::common::ErrorCode error, RequestSmsInfoListCb callback, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(infos, error);
    }
}

telux::common::Status SmsManagerStub::readMessage(uint32_t messageIndex,
    ReadSmsMessageCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::ReadMessageRequest request;
    ::telStub::ReadMessageReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_msg_index(messageIndex);

    grpc::Status reqstatus = stub_->ReadMessage(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    std::string text =  static_cast<std::string>((response.sms_message()).text());
    std::string sender  =  static_cast<std::string>((response.sms_message()).sender());
    std::string receiver  =  static_cast<std::string>((response.sms_message()).receiver());
    telux::tel::SmsEncoding encoding  =
        static_cast<telux::tel::SmsEncoding>((response.sms_message()).encoding());
    std::string pdu  =  static_cast<std::string>((response.sms_message()).pdu());
    std::vector <uint8_t> rawPdu;
    std::shared_ptr<MessagePartInfo> Info = nullptr;
    SmsMetaInfo metaInfo;
    const uint8_t* tmp = reinterpret_cast<const uint8_t*>(pdu.c_str());
    while (*tmp !='\0')
    {
        rawPdu.push_back(*tmp);
        tmp++;
    }

    int numberOfSegments  =  static_cast<int>(
            (response.sms_message()).messageinfono_of_segments());
    LOG(DEBUG, __FUNCTION__,"numberOfSegments", numberOfSegments);
    if(numberOfSegments > 1) {
        Info = std::make_shared< MessagePartInfo >();
        Info->refNumber  =  static_cast<int>((response.sms_message()).messageinfosegment_no());
        Info->numberOfSegments  =  numberOfSegments;
        Info->segmentNumber  =  static_cast<int>((response.sms_message()).messageinfosegment_no());
    }
    bool isMetaInfoValid  = static_cast<bool>(response.sms_message().ismetainfo_valid());
    metaInfo.tagType = static_cast<telux::tel::SmsTagType>(response.sms_message().tag_type());
    metaInfo.msgIndex = static_cast<int>((response.sms_message()).msg_index());

    SmsMessage msg
        (text, sender, receiver, encoding, pdu, rawPdu, Info, isMetaInfoValid, metaInfo);

    // Sending the Callback Response.
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, msg, error, callback, cbDelay]() {
                this->invokeReadSmsMessageCb(msg, error, callback, cbDelay);
            }).share();
        taskQ_->add(f1);
    }
    return status;
}

void SmsManagerStub::invokeReadSmsMessageCb(SmsMessage message, telux::common::ErrorCode error,
    ReadSmsMessageCb callback, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(message, error);
    }
}

telux::common::Status SmsManagerStub::deleteMessage(DeleteInfo info,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " PhoneId: ", phoneId_, " MessageIndex: ", info.msgIndex,
        " Delete Type: ", static_cast<int>(info.delType), " SMS Tag Type: ",
        static_cast<int>(info.tagType));
    ::telStub::DeleteMessageRequest request;
    ::telStub::DeleteMessageRequestReply response;
    ::telStub::SmsTagType_TagType tag =  static_cast<::telStub::SmsTagType_TagType>(info.tagType);
    ::telStub::DelType_DeleteType deltype =  static_cast<::telStub::DelType_DeleteType>(info.delType);
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_msg_index(info.msgIndex);
    request.set_tag_type(tag);
    request.set_del_type(deltype);

    grpc::Status reqstatus = stub_->DeleteMessage(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            this->invokeResponseCallback(cbDelay, error, callback);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status SmsManagerStub::requestPreferredStorage(RequestPreferredStorageCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestPreferredStorageRequest request;
    ::telStub::RequestPreferredStorageReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestPreferredStorage(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::tel::StorageType storageType =
        static_cast<telux::tel::StorageType>(response.storage_type());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, callback, storageType, cbDelay, error]() {
                this->invokeRequestPreferredStorageCb(storageType,cbDelay, error, callback);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeRequestPreferredStorageCb(telux::tel::StorageType storageType,
    int cbDelay, telux::common::ErrorCode error, RequestPreferredStorageCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(storageType, error);
    }
}

telux::common::Status SmsManagerStub::setPreferredStorage(StorageType storageType,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " PhoneId : ", phoneId_);

    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetPreferredStorageRequest request;
    ::telStub::SetPreferredStorageReply response;
    ::telStub::StorageType_Type type =  static_cast<::telStub::StorageType_Type>(storageType);
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_storage_type(type);

    grpc::Status reqstatus = stub_->SetPreferredStorage(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            this->invokeResponseCallback(cbDelay, error, callback);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status SmsManagerStub::setTag(uint32_t msgIndex, SmsTagType tagType,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " PhoneId : ", phoneId_);
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetTagRequest request;
    ::telStub::SetTagReply response;
    ::telStub::SmsTagType_TagType tag =  static_cast<::telStub::SmsTagType_TagType>(tagType);
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_msg_index(msgIndex);
    request.set_tag_type(tag);

    grpc::Status reqstatus = stub_->SetTag(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    LOG(DEBUG, __FUNCTION__, "Status is ", static_cast<int>(status));
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            this->invokeResponseCallback(cbDelay, error, callback);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status SmsManagerStub::requestStorageDetails(RequestStorageDetailsCb callback) {

    LOG(DEBUG, __FUNCTION__);
    ::telStub::RequestStorageDetailsRequest request;
    ::telStub::RequestStorageDetailsReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestStorageDetails(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int maxCount = response.max_count();
    int availableCount = response.available_count();
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, maxCount, availableCount, cbDelay, error, callback]() {
            this->invokeRequestStorageDetailsCb(maxCount, availableCount,
                cbDelay, error, callback);
        }).share();
    taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeRequestStorageDetailsCb(int maxCount, int availableCount,
    int cbDelay, telux::common::ErrorCode error, RequestStorageDetailsCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(maxCount, availableCount, error);
    }
}

int SmsManagerStub::getPhoneId() {
    LOG(DEBUG, __FUNCTION__,"PhoneId is ",phoneId_);
    return phoneId_;
}

MessageAttributes SmsManagerStub::calculateMessageAttributes(const std::string &message) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::GetMessageAttributesRequest request;
    ::telStub::GetMessageAttributesReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetMessageAttributes(&context, request, &response);
    MessageAttributes msg = {};
    msg.encoding  = static_cast<telux::tel::SmsEncoding>
        ((response.message_attribute()).encoding());
    msg.numberOfSegments = static_cast<int>((response.message_attribute()).number_of_segments());
    msg.segmentSize = static_cast<int>((response.message_attribute()).segment_size());
    msg.numberOfCharsLeftInLastSegment =
        static_cast<int>((response.message_attribute()).number_of_chars_left_in_last_segment());
    return msg;
}

/**
 * SmsMessage class to expose details to user application..
 */
SmsMessage::SmsMessage(std::string text, std::string sender, std::string receiver,
    SmsEncoding encoding, std::string pdu, PduBuffer rawPdu, std::shared_ptr<MessagePartInfo> info)
   : text_(text)
   , sender_(sender)
   , receiver_(receiver)
   , encoding_(encoding)
   , pdu_(pdu)
   , rawPdu_(rawPdu)
   , msgPartInfo_(info) {
}

SmsMessage::SmsMessage(std::string text, std::string sender, std::string receiver,
    SmsEncoding encoding, std::string pdu, PduBuffer rawPdu, std::shared_ptr<MessagePartInfo> info
    , bool isMetaInfoValid, SmsMetaInfo metaInfo)
   : text_(text)
   , sender_(sender)
   , receiver_(receiver)
   , encoding_(encoding)
   , pdu_(pdu)
   , rawPdu_(rawPdu)
   , msgPartInfo_(info)
   , isMetaInfoValid_(isMetaInfoValid)
   , metaInfo_(metaInfo) {
}

const std::string &SmsMessage::getText() const {
   return text_;
}

const std::string &SmsMessage::getSender() const {
   return sender_;
}

const std::string &SmsMessage::getReceiver() const {
   return receiver_;
}

SmsEncoding SmsMessage::getEncoding() const {
   return encoding_;
}

const std::string &SmsMessage::getPdu() const {
   return pdu_;
}

PduBuffer SmsMessage::getRawPdu() const {
    return rawPdu_;
}

std::shared_ptr<MessagePartInfo> SmsMessage::getMessagePartInfo() {
    return msgPartInfo_;
}

telux::common::Status SmsMessage::getMetaInfo(SmsMetaInfo &metaInfo) {

    if (isMetaInfoValid_) {
        metaInfo = metaInfo_;
        return telux::common::Status::SUCCESS;
    }
    return telux::common::Status::NOSUCH;
}

const std::string SmsMessage::toString() const {
   std::stringstream ss;
   ss << "Message: " << text_ << ", From: " << sender_ << ", To: " << receiver_;
   return ss.str();
}

void SmsManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::telStub::SmsMessage>()) {
        ::telStub::SmsMessage smsEvent;
        event.UnpackTo(&smsEvent);
        handleIncomingSms(smsEvent);
    } else if(event.Is<::telStub::memoryFullEvent>()) {
        ::telStub::memoryFullEvent memoryFullEvent;
        event.UnpackTo(&memoryFullEvent);
        handleMemoryFullEvent(memoryFullEvent);
    } else {}
}

void SmsManagerStub::handleIncomingSms(::telStub::SmsMessage event) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    int numberOfSegments = event.messageinfono_of_segments();
    int refNumber = event.messageinforef_no();
    int segmentNumber = event.messageinfosegment_no();
    int msgIndex = event.msg_index();
    telux::tel::SmsTagType tagType = static_cast<telux::tel::SmsTagType>(event.tag_type());
    telux::tel::SmsEncoding encoding = static_cast<telux::tel::SmsEncoding>(event.encoding());
    bool isMetaInfoValid = event.ismetainfo_valid();
    std::string pdu = event.pdu();
    std::string rawPdu = event.pdu();
    std::string receiver= event.receiver();
    std::string sender = event.sender();
    std::string text = event.text();

    const uint8_t* p = reinterpret_cast<const uint8_t*>(pdu.c_str());
    std::vector <uint8_t> pduBuffer;
    while (*p !='\0')
    {
        pduBuffer.push_back(*p);
        p++;
    }
    /* Construct sendMessage based on the inputs */
    std::shared_ptr<MessagePartInfo> info = std::make_shared< MessagePartInfo >();
    SmsMetaInfo metaInfo = {};
    info->refNumber = refNumber;
    info->numberOfSegments = numberOfSegments;
    info->segmentNumber = segmentNumber;

    metaInfo.msgIndex = msgIndex;
    metaInfo.tagType = tagType;
    SmsMessage msg(text, sender, receiver, encoding, pdu, pduBuffer,
        info, isMetaInfoValid, metaInfo);
    auto sharedPtr = std::make_shared<SmsMessage> (msg);
    // Invoke incomingSms notification to clients
    invokeIncomingSmslisteners(phoneId, sharedPtr);
    isMemoryFull(phoneId);

    // Consolidated message for all incomingSms segments and send the notification to clients
    if ((msg.getMessagePartInfo())->numberOfSegments > 1 ) {
        parseAndConcatenateSmsMessage (phoneId, msg);
    } else if((((msg.getMessagePartInfo())->numberOfSegments == 1) //Single part messages
        && ((msg.getMessagePartInfo())->segmentNumber == 1))) {
        auto ptr = std::make_shared<std::vector<SmsMessage>> (std::vector<SmsMessage>{msg});
        invokeIncomingSmslisteners(phoneId, ptr);
    } else {
        LOG(ERROR, __FUNCTION__, " Invalid input for current segment "
            , (msg.getMessagePartInfo())->segmentNumber ,
            " and total number of segments " , (msg.getMessagePartInfo())->numberOfSegments);
        return;
    }
}

void SmsManagerStub::isMemoryFull(int phoneId) {
    LOG(DEBUG, __FUNCTION__);

    ::telStub::IsMemoryFullRequest request;
    ::telStub::IsMemoryFullReply response;
    ClientContext context;
    request.set_phone_id(phoneId);

    grpc::Status reqstatus = stub_->IsMemoryFull(&context, request, &response);

    bool isMemoryFull = response.ismemoryfull();

    if(isMemoryFull) {
        invokeMemoryFulllisteners(phoneId, telux::tel::StorageType::SIM);
    }
}

void SmsManagerStub::invokeIncomingSmslisteners (int phoneId,
    std::shared_ptr<SmsMessage> message) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ISmsListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onIncomingSms(phoneId, message);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void SmsManagerStub::invokeIncomingSmslisteners(int phoneId,
    std::shared_ptr<std::vector<SmsMessage>> messages) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ISmsListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onIncomingSms(phoneId_, messages);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void SmsManagerStub::parseAndConcatenateSmsMessage (int phoneId, SmsMessage& message) {
    LOG(DEBUG, __FUNCTION__);
    bool found = false;
    for ( auto &smsInfo : smsMessageMap_ ) {
        if (smsInfo.first.refNumber == ((message.getMessagePartInfo())->refNumber) &&
            smsInfo.first.senderAddress == message.getSender()) {
            LOG(DEBUG, __FUNCTION__, " Key with refNumber: ", (smsInfo.first).refNumber,
                " and senderAddress: ", smsInfo.first.senderAddress, " exists.");

            MessageMetaData metaData = smsInfo.first;
            std::vector<SmsMessage> smsInfos = smsMessageMap_[metaData];
            bool isPartAlreadyExist = false;
            int index = 0;
            for ( auto &smsInfoElement : smsInfos ) {
                if ((smsInfoElement.getMessagePartInfo())->segmentNumber ==
                    (message.getMessagePartInfo())->segmentNumber) {
                    LOG(DEBUG, __FUNCTION__, " Already part exist with segment no: ",
                        static_cast<int>((message.getMessagePartInfo())->segmentNumber));
                    isPartAlreadyExist = true;
                    break;
                }
                index++;
            }
            if (isPartAlreadyExist) {
                LOG(ERROR, __FUNCTION__,
                    " Duplicate or latest updated SMS info received at index: ", index);
                smsInfos[index] = message;
            } else {
                LOG(DEBUG, __FUNCTION__, " Add SMS info");
                smsInfos.emplace_back(message);
            }

            auto comp = [] ( SmsMessage& lhs,
                 SmsMessage& rhs )
                    {return (lhs.getMessagePartInfo())->segmentNumber <
                            (rhs.getMessagePartInfo())->segmentNumber;};
            sort(smsInfos.begin(), smsInfos.end(), comp);
            smsMessageMap_[metaData] = smsInfos;
            found = true;
            if (static_cast<int>(smsInfos.size())
                == (message.getMessagePartInfo())->numberOfSegments)  {
                LOG(DEBUG, __FUNCTION__, " All the parts of SMS is received");
                std::vector<SmsMessage> messages;
                for ( auto &smsInfoElement : smsInfos ) {
                    messages.emplace_back(smsInfoElement);
                }

                auto sharedPtr = std::make_shared<std::vector<SmsMessage>> (messages);
                invokeIncomingSmslisteners(phoneId, sharedPtr);
                smsMessageMap_.erase(metaData);
                return;
            }
            break;
        }
    }
        if(!found) {
            LOG(DEBUG, __FUNCTION__,
                " Key with refNumber: ", (message.getMessagePartInfo())->segmentNumber,
                " and senderAddress: ", message.getSender(), " does not exists.");
            MessageMetaData metaData;
            metaData.refNumber = (message.getMessagePartInfo())->segmentNumber;
            metaData.senderAddress = message.getSender();
            std::vector<SmsMessage> smsInfos;
            smsInfos.emplace_back(message);
            smsMessageMap_[metaData] = smsInfos;
        }
}

void SmsManagerStub::handleMemoryFullEvent(::telStub::memoryFullEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    telux::tel::StorageType type = static_cast<telux::tel::StorageType>(event.storage_type());
    LOG(DEBUG, __FUNCTION__, "The Storage type is : ", static_cast<int>(type));
    LOG(DEBUG, __FUNCTION__, "Phone Id is  : ", phoneId);
    invokeMemoryFulllisteners(phoneId, type);
}

void SmsManagerStub::invokeMemoryFulllisteners(int phoneId, telux::tel::StorageType type) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ISmsListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onMemoryFull(phoneId, type);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}