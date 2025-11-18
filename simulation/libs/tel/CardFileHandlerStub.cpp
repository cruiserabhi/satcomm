/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CardFileHandlerStub.hpp"
#include <telux/tel/CardDefines.hpp>
#include "common/CommonUtils.hpp"
#include <thread>
using namespace telux::common;

namespace telux {

namespace tel {


CardFileHandlerStub::CardFileHandlerStub(SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    stub_ = CommonUtils::getGrpcStub<CardService>();
    slotId_ = slotId;
}

void CardFileHandlerStub::cleanup() {
   LOG(DEBUG, __FUNCTION__);
   cardApps_.clear();
}

telux::common::Status CardFileHandlerStub::updateCardApps(
    std::vector<std::shared_ptr<CardAppStub>> cardApps) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lk(mtx_);
    cardApps_ = cardApps;
    return telux::common::Status::SUCCESS;
}
bool CardFileHandlerStub::isAppReady(std::string aid) {
    if (aid.length() == 0) {
        // This means EF is not read from card apps such as USIM, ISIM and SIM
        return true;
    } else {
        for (auto cardApp : cardApps_) {
            if (cardApp->getAppId().compare(aid) == 0 &&
                cardApp->getAppState() == AppState::APPSTATE_READY)  {
                return true;
            }
        }
    }
    return false;
}
telux::common::Status CardFileHandlerStub::readEFLinearFixed(std::string filePath,
    uint16_t fileId, int recordNum, std::string aid, EfOperationCallback callback) {
    LOG(DEBUG, __FUNCTION__, " filePath: ", filePath, " recordNum: ", recordNum,
    " fileId: ", fileId, " aid: ", aid);
    if (filePath.length() == 0) {
        LOG(ERROR, __FUNCTION__, " filePath is empty: ");
        return telux::common::Status::INVALIDPARAM;
    }
    if (!callback) {
        LOG(ERROR, __FUNCTION__, " callback is null");
        return telux::common::Status::INVALIDPARAM;
    }
    if (recordNum <= 0 ) {
        LOG(ERROR, __FUNCTION__, " recordNum is invalid: ", recordNum);
        return telux::common::Status::INVALIDPARAM;
    }
    bool appReady = isAppReady(aid);
    if (!appReady) {
        LOG(ERROR, __FUNCTION__, " app ready: ", appReady);
        return telux::common::Status::INVALIDSTATE;
    }
    ::telStub::ReadEFLinearFixedRequest request;
    ::telStub::ReadEFLinearFixedReply response;
    ClientContext context;

    telux::tel::IccResult iccresult;
    request.set_slot_id(slotId_);
    request.set_file_path(filePath);
    request.set_file_id(fileId);
    request.set_record_number(recordNum);
    request.set_aid(aid);

    grpc::Status reqstatus = stub_->ReadEFLinearFixed(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    iccresult.sw1  = static_cast<int>((response.result()).sw1());
    iccresult.sw2  = static_cast<int>((response.result()).sw2());
    iccresult.payload  = static_cast<std::string>((response.result()).pay_load());
    std::vector<int> store;
    for (auto &r : (response.result()).data()) {
        int tmp =  static_cast<uint8_t>(r);
        LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
        store.emplace_back(tmp);
    }
    (iccresult.data).assign(store.begin(), store.end());

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());

    int cbDelay = static_cast<int>(response.delay());

    LOG(DEBUG, __FUNCTION__,"sw1 " ,iccresult.sw1 ,"sw2 "
        , iccresult.sw2 ,"payload " ,iccresult.payload);
    LOG(DEBUG, __FUNCTION__,"error ", static_cast<int>(error),
        "status ", static_cast<int>(status));
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());

    if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, error , iccresult, cbDelay, callback]() {
                this->invokeCallback(callback, error, iccresult, cbDelay );
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status CardFileHandlerStub::readEFLinearFixedAll(std::string filePath,
    uint16_t fileId, std::string aid, EfReadAllRecordsCallback callback) {
    LOG(DEBUG, __FUNCTION__, " filePath: ", filePath, " fileId: ", fileId, " aid: ", aid);
    if (filePath.length() == 0) {
        LOG(ERROR, __FUNCTION__, " filePath is empty: ");
        return telux::common::Status::INVALIDPARAM;
    }
    if (!callback) {
        LOG(ERROR,  __FUNCTION__, " callback is null");
        return telux::common::Status::INVALIDPARAM;
    }
    bool appReady = isAppReady(aid);
    if (!appReady) {
        LOG(ERROR,  __FUNCTION__, " app ready: ", appReady);
        return telux::common::Status::INVALIDSTATE;
    }
    ::telStub::ReadEFLinearFixedAllRequest request;
    ::telStub::ReadEFLinearFixedAllReply response;
    ClientContext context;
    std::vector<IccResult> records;

    request.set_slot_id(slotId_);
    request.set_file_path(filePath);
    request.set_file_id(fileId);
    request.set_aid(aid);

    grpc::Status reqstatus = stub_->ReadEFLinearFixedAll(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    for (int i = 0; i < response.records_size(); i++) {
        telux::tel::IccResult iccresult;
        iccresult.sw1  =  static_cast<int>(response.mutable_records(i)->sw1());
        iccresult.sw2  =  static_cast<int>(response.mutable_records(i)->sw2());
        iccresult.payload = static_cast<std::string>(response.mutable_records(i)->pay_load());
        LOG(DEBUG, __FUNCTION__, "sw1 " ,iccresult.sw1, "sw2 ", iccresult.sw2,
            "payload " ,iccresult.payload);
        std::vector<int> store;
        for (auto &r : response.mutable_records(i)->data()) {
            int tmp =  static_cast<uint8_t>(r);
            LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
            store.emplace_back(tmp);
        }
        (iccresult.data).assign(store.begin(), store.end());
        records.emplace_back(iccresult);
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());

    int cbDelay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, error , records, cbDelay, callback]() {
                this->invokeCallback(callback, error, records, cbDelay );
            }).share();
        taskQ_->add(f);
    }
    return status;

}

telux::common::Status CardFileHandlerStub::readEFTransparent(std::string filePath,
    uint16_t fileId, int size, std::string aid, EfOperationCallback callback) {
    LOG(DEBUG, __FUNCTION__, " fileId: ", fileId, " size: ", size, " aid: ", aid);
    if (filePath.length() == 0) {
        LOG(ERROR, __FUNCTION__, " filePath is empty: ");
        return telux::common::Status::INVALIDPARAM;
    }
    if (!callback) {
        LOG(ERROR, __FUNCTION__, " callback is null");
        return telux::common::Status::INVALIDPARAM;
    }
    if (size < 0 ) {
        LOG(ERROR, __FUNCTION__, " Size is invalid: ", size);
        return telux::common::Status::INVALIDPARAM;
    }
    bool appReady = isAppReady(aid);
    if (!appReady) {
        LOG(ERROR, __FUNCTION__, " app ready: ", appReady);
        return telux::common::Status::INVALIDSTATE;
    }
    ::telStub::ReadEFTransparentRequest request;
    ::telStub::ReadEFTransparentReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
    request.set_file_path(filePath);
    request.set_file_id(fileId);
    request.set_size(size);
    request.set_aid(aid);

    telux::tel::IccResult iccresult;

    grpc::Status reqstatus = stub_->ReadEFTransparent(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    iccresult.sw1  = static_cast<int>((response.result()).sw1());
    iccresult.sw2  = static_cast<int>((response.result()).sw2());
    iccresult.payload  = static_cast<std::string>((response.result()).pay_load());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int cbDelay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    std::vector<int> store;
    for (auto &r : (response.result()).data()) {
        int tmp =  static_cast<uint8_t>(r);
        LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
        store.emplace_back(tmp);
    }
    (iccresult.data).assign(store.begin(), store.end());

    LOG(DEBUG, __FUNCTION__,"sw1 " ,iccresult.sw1, "sw2 "
        ,iccresult.sw2,"payload " ,iccresult.payload );

    if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, error , iccresult, callback, cbDelay]() {
                this->invokeCallback(callback, error, iccresult, cbDelay);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status CardFileHandlerStub::writeEFLinearFixed(std::string filePath,
    uint16_t fileId, int recordNum, std::vector<uint8_t> data, std::string pin2,
    std::string aid, EfOperationCallback callback) {
    LOG(DEBUG, __FUNCTION__, " fileId: ", fileId, " recordNum: ", recordNum, " aid: ", aid);
    if (filePath.length() == 0) {
        LOG(ERROR, __FUNCTION__, " filePath is empty: ");
        return telux::common::Status::INVALIDPARAM;
    }
    if (!callback) {
        LOG(ERROR, __FUNCTION__, " callback is null");
        return telux::common::Status::INVALIDPARAM;
    }
    bool appReady = isAppReady(aid);
    if (!appReady) {
        LOG(ERROR,  __FUNCTION__, " app ready: ", appReady);
        return telux::common::Status::INVALIDSTATE;
    }
    ::telStub::WriteEFLinearFixedRequest request;
    ::telStub::WriteEFLinearFixedReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
    request.set_file_path(filePath);
    request.set_file_id(fileId);
    request.set_aid(aid);
    request.set_record_number(recordNum);
    int size = data.size();
    for (int j = 0; j < size ; j++)
    {
        int d = static_cast<int>(data[j]);
        request.add_data(d);
    }

    telux::tel::IccResult iccresult;

    grpc::Status reqstatus = stub_->WriteEFLinearFixed(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    iccresult.sw1  = static_cast<int>((response.result()).sw1());
    iccresult.sw2  = static_cast<int>((response.result()).sw2());
    iccresult.payload  = static_cast<std::string>((response.result()).pay_load());
    std::vector<int> store;
    for (auto &r : (response.result()).data()) {
        int tmp =  static_cast<uint8_t>(r);
        LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
        store.emplace_back(tmp);
    }
    (iccresult.data).assign(store.begin(), store.end());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());

    LOG(DEBUG, __FUNCTION__,"sw1 " ,iccresult.sw1,
        "sw2 " ,iccresult.sw2,"payload " ,iccresult.payload);
    int cbDelay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, error , iccresult, callback, cbDelay]() {
                this->invokeCallback(callback, error, iccresult, cbDelay);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status CardFileHandlerStub::writeEFTransparent(std::string filePath,
    uint16_t fileId, std::vector<uint8_t> data, std::string aid, EfOperationCallback callback) {
    LOG(DEBUG, __FUNCTION__, " fileId: ", fileId, " aid: ", aid);
    if (filePath.length() == 0) {
        LOG(ERROR, __FUNCTION__, " filePath is empty: ");
        return telux::common::Status::INVALIDPARAM;
    }
    if (!callback) {
        LOG(ERROR, __FUNCTION__, " callback is null");
        return telux::common::Status::INVALIDPARAM;
    }
    bool appReady = isAppReady(aid);
    if (!appReady) {
        LOG(ERROR,  __FUNCTION__, " app ready: ", appReady);
        return telux::common::Status::INVALIDSTATE;
    }
    ::telStub::WriteEFTransparentRequest request;
    ::telStub::WriteEFTransparentReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
    request.set_file_path(filePath);
    request.set_file_id(fileId);
    request.set_aid(aid);

    telux::tel::IccResult iccresult;

    int size = data.size();
    for (int j = 0; j < size ; j++)
    {
        int d = static_cast<int>(data[j]);
        request.add_data(d);
    }

    grpc::Status reqstatus = stub_->WriteEFTransparent(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    iccresult.sw1  = static_cast<int>((response.result()).sw1());
    iccresult.sw2  = static_cast<int>((response.result()).sw2());
    iccresult.payload  = static_cast<std::string>((response.result()).pay_load());
    std::vector<int> store;
    for (auto &r : (response.result()).data()) {
        int tmp =  static_cast<uint8_t>(r);
        LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
        store.emplace_back(tmp);
    }
    (iccresult.data).assign(store.begin(), store.end());

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int cbDelay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());

    LOG(DEBUG, __FUNCTION__,"sw1 " ,iccresult.sw1, "sw2 " ,iccresult.sw2,
        "payload " ,iccresult.payload );
    if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, error , iccresult, callback, cbDelay]() {
                this->invokeCallback(callback, error, iccresult, cbDelay);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status CardFileHandlerStub::requestEFAttributes(EfType efType,
    std::string filePath, uint16_t fileId, std::string aid,
    EfGetFileAttributesCallback callback) {
    LOG(DEBUG, __FUNCTION__, " filePath: ", filePath, " fileId: ", fileId, " efType: ",
    static_cast<int>(efType), " aid: ", aid);
    if (filePath.length() == 0) {
        LOG(ERROR, __FUNCTION__, " filePath is empty: ");
        return telux::common::Status::INVALIDPARAM;
    }
    if (!callback) {
        LOG(ERROR, __FUNCTION__, " callback is null");
        return telux::common::Status::INVALIDPARAM;
    }
    if (efType != EfType::TRANSPARENT && efType != EfType::LINEAR_FIXED ) {
        LOG(ERROR, __FUNCTION__, " Invalid EF type");
        return telux::common::Status::INVALIDPARAM;
    }
    ::telStub::EFAttributesRequest request;
    ::telStub::RequestEFAttributesReply response;
    ::telStub::EfType type = static_cast<::telStub::EfType>(efType);
    ClientContext context;

    telux::tel::FileAttributes attributes;
    telux::tel::IccResult iccresult;

    request.set_slot_id(slotId_);
    request.set_ef_type(type);
    request.set_file_path(filePath);
    request.set_file_id(fileId);
    request.set_aid(aid);

    grpc::Status reqstatus = stub_->RequestEFAttributes(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    iccresult.sw1  = static_cast<int>((response.result()).sw1());
    iccresult.sw2  = static_cast<int>((response.result()).sw2());
    iccresult.payload  = static_cast<std::string>((response.result()).pay_load());
    attributes.fileSize  = static_cast<int>((response.file_attributes()).file_size());
    attributes.recordSize  = static_cast<int>((response.file_attributes()).record_size());
    attributes.recordCount  = static_cast<int>((response.file_attributes()).record_count());

    LOG(DEBUG, __FUNCTION__,"sw1 " ,iccresult.sw1, "sw2 ",
        iccresult.sw2, "payload " ,iccresult.payload );
    LOG(DEBUG, __FUNCTION__,"fileSize " ,attributes.fileSize,
        "recordSize " ,attributes.recordSize,"recordCount " ,attributes.recordCount);

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int cbDelay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    std::vector<int> store;
    for (auto &r : (response.result()).data()) {
        int tmp =  static_cast<uint8_t>(r);
        LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
        store.emplace_back(tmp);
    }
    (iccresult.data).assign(store.begin(), store.end());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, error , iccresult, attributes, callback, cbDelay]() {
                this->invokeCallback(callback, error, iccresult, attributes, cbDelay);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

void CardFileHandlerStub::invokeCallback(EfGetFileAttributesCallback callback,
    telux::common::ErrorCode error, telux::tel::IccResult iccresult,
    telux::tel::FileAttributes attributes, int cbDelay ) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , iccresult, attributes, callback]() {
            callback(error, iccresult, attributes);
        }).share();
    taskQ_->add(f);
}

void CardFileHandlerStub::invokeCallback(EfReadAllRecordsCallback callback,
    telux::common::ErrorCode error, std::vector<IccResult> records, int cbDelay) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , records, callback]() {
            callback(error, records);
        }).share();
    taskQ_->add(f);
}

void CardFileHandlerStub::invokeCallback(EfOperationCallback callback,
    telux::common::ErrorCode error, telux::tel::IccResult iccresult, int cbDelay) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , iccresult, callback]() {
            callback(error, iccresult);
        }).share();
    taskQ_->add(f);
}

SlotId CardFileHandlerStub::getSlotId() {
    return slotId_;
}
} // end of namespace tel

} // end of namespace telux
