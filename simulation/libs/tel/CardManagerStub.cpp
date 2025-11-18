/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CardManagerStub.hpp"
#include <telux/common/DeviceConfig.hpp>
#include "common/event-manager/ClientEventManager.hpp"

using namespace telux::common;

namespace telux {

namespace tel {

CardManagerStub::CardManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_ = DEFAULT_DELAY;
}

telux::common::Status CardManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    myPid_ = getpid();

    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ICardListener>>();
    if(!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    stub_ = CommonUtils::getGrpcStub<CardService>();
    if(!stub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate card service");
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

CardManagerStub::~CardManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void CardManagerStub::cleanup() {
   LOG(DEBUG, __FUNCTION__);
   for(const auto &card : cardMap_) {
      if(card.second != nullptr) {
         card.second->cleanup();
      }
   }
   cardMap_.clear();
}

void CardManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
        std::lock_guard<std::mutex> lock(cardManagerMutex_);
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

void CardManagerStub::initSync() {
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    LOG(DEBUG, __FUNCTION__);
    grpc::Status reqstatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    if (reqstatus.ok()) {
        cbStatus = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay_ = static_cast<int>(response.delay());
        if(cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            slotCount_ = 1;
            if(telux::common::DeviceConfig::isMultiSimSupported()) {
                slotCount_ = 2;
            }
            for(int id = 1; id <= slotCount_; ++id) {
                simSlotIds_.emplace_back(id);
            }
            for(int &slotId : simSlotIds_) {
                auto card = std::make_shared<CardStub>(slotId);
            if (card) {
                cardMap_.emplace(slotId, card);
            } else {
                LOG(ERROR, __FUNCTION__, " Card is NULL for slotId: ", slotId);
            }
            }
            for (auto slotId:simSlotIds_) {
                LOG(DEBUG, __FUNCTION__," SlotId is ",slotId);
                cardMap_[slotId]->updateSimStatus();
            }
        }
    }
    LOG(DEBUG, __FUNCTION__, " Delay ", cbDelay_, " service status ", static_cast<int>(cbStatus));
    bool isSubsystemReady = (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)?
        true : false;
    setSubsystemReady(isSubsystemReady);
    setServiceStatus(cbStatus);
}

void CardManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(cardManagerMutex_);
    ready_ = status;
    cardManagerInitCV_.notify_all();
}

bool CardManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

bool CardManagerStub::waitForInitialization() {
    LOG(INFO, __FUNCTION__);
    std::unique_lock<std::mutex> lock(cardManagerMutex_);
    if (!isSubsystemReady()) {
        cardManagerInitCV_.wait(lock);
    }
    return isSubsystemReady();
}

std::future<bool> CardManagerStub::onSubsystemReady() {
    auto future
        = std::async(std::launch::async, [&] { return waitForInitialization(); });
    return future;
}

telux::common::ServiceStatus CardManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status CardManagerStub::getSlotIds(std::vector<int> &slotIds) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    std::lock_guard<std::mutex> lock(cardManagerMutex_);
    slotIds = simSlotIds_;

    return telux::common::Status::SUCCESS;
}

telux::common::Status CardManagerStub::getSlotCount(int &count) {
   LOG(DEBUG, __FUNCTION__);
   if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
       return telux::common::Status::NOTREADY;
    }
    std::lock_guard<std::mutex> lock(cardManagerMutex_);
    count = slotCount_;

    return telux::common::Status::SUCCESS;
}

std::shared_ptr<ICard> CardManagerStub::getCard(int slotId, telux::common::Status *status) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        if(status) {
            *status = telux::common::Status::NOTREADY;
        }
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(cardManagerMutex_);
        auto card = cardMap_.find(slotId);
        if(card != cardMap_.end()) {
            auto cardImpl = cardMap_[slotId];
            if(cardImpl) {
                if (status) {
                    *status = telux::common::Status::SUCCESS;
                }
                return cardImpl;
            } else {
                LOG(DEBUG, " cardImpl is empty");
            }
        }
    }
    LOG(INFO, "Unable to get the card instance for given slotId: ", slotId);
    if(status) {
        *status = telux::common::Status::NOTREADY;
    }
    return nullptr;
}

telux::common::Status CardManagerStub::cardPowerUp(SlotId slotId,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::CardPowerRequest request;
    ::telStub::CardPowerResponse response;
    ClientContext context;
    request.set_phone_id(slotId);
    request.set_powerup(true);

    grpc::Status reqstatus = stub_->CardPower(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback, delay]() {
                this->invokeCallback(callback, error, delay);
            }).share();
        taskQ_->add(f1);

        if (error != telux::common::ErrorCode::NO_EFFECT) {
            int slotid = static_cast<int>(slotId);
            auto f2 = std::async(std::launch::async,
                [this, slotid]() {
                    this->invokelisteners(slotid);
                }).share();
            taskQ_->add(f2);
        }
    }
    return status;
}

void CardManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

void CardManagerStub::invokelisteners(int slotId) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICardListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onCardInfoChanged(slotId);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

telux::common::Status CardManagerStub::cardPowerDown(SlotId slotId,
    telux::common::ResponseCallback callback) {
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::CardPowerRequest request;
    ::telStub::CardPowerResponse response;
    ClientContext context;
    request.set_phone_id(slotId);
    request.set_powerup(false);

    grpc::Status reqstatus = stub_->CardPower(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback, delay]() {
                this->invokeCallback(callback, error, delay);
            }).share();
        taskQ_->add(f1);

        if (error != telux::common::ErrorCode::NO_EFFECT) {
            int slotid = static_cast<int>(slotId);
            auto f2 = std::async(std::launch::async,
                [this, slotid]() {
                    this->invokelisteners(slotid);
                }).share();
            taskQ_->add(f2);
        }
    }
    return status;
}

void CardManagerStub::setRpcRefreshParams(::telStub::RefreshParams* refreshs,
    const RefreshParams refreshParams) {
    if (not refreshs) {
        return;
    }
    auto ssType = static_cast<::telStub::SessionType>(refreshParams.sessionType);
    refreshs->set_sessiontype(ssType);
    if (refreshParams.sessionType == tel::SessionType::NONPROVISIONING_SLOT_1 ||
        refreshParams.sessionType == tel::SessionType::NONPROVISIONING_SLOT_2) {
        refreshs->set_aid(refreshParams.aid);
    } else {
        LOG(WARNING, __FUNCTION__, " ignore aid as ssType ", static_cast<int>(ssType));
    }
}

void CardManagerStub::convertRefreshParams(const RefreshParams userParams,
    RefreshParams& refreshParams) {
    refreshParams.sessionType = userParams.sessionType;
    if (userParams.sessionType == tel::SessionType::NONPROVISIONING_SLOT_1 ||
        userParams.sessionType == tel::SessionType::NONPROVISIONING_SLOT_2) {
        refreshParams.aid = userParams.aid;
    }
}

telux::common::Status CardManagerStub::setupRefreshConfig(
    SlotId slotId, bool isRegister, bool doVoting, std::vector<IccFile> efFiles,
    RefreshParams refreshParams, common::ResponseCallback callback) {
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RefreshConfigReq request;
    ::telStub::TelCommonReply response;
    ClientContext context;
    UserRefreshParam userPref;
    request.set_identifier(static_cast<uint32_t>(myPid_));

    LOG(DEBUG, __FUNCTION__, " slotId ", static_cast<int>(slotId),
        ", isRegister ", static_cast<int>(isRegister),
        ", doVoting ", static_cast<int>(doVoting),
        ", refreshParams.sessionType ", static_cast<int>(refreshParams.sessionType),
        ", refreshParams.aid ", refreshParams.aid,
        ", efFiles.size ", efFiles.size());

    if (slotId < DEFAULT_SLOT_ID || slotId > MAX_SLOT_ID) {
        LOG(ERROR, __FUNCTION__, " invalid slotId");
        return telux::common::Status::FAILED;
    }
    if (slotId != getSlotBySessionType(refreshParams.sessionType)) {
        LOG(ERROR, __FUNCTION__, " conflict slotId and sessionType");
        return telux::common::Status::FAILED;
    }
    if (doVoting && (not isRegister)) {
        LOG(ERROR, __FUNCTION__, " not register but voting, INVALIDPARAM");
        return telux::common::Status::INVALIDPARAM;
    }

    userPref.isRegister = isRegister;
    userPref.doVoting = doVoting;
    userPref.efFiles = efFiles;
    userPref.refreshParams = refreshParams;

    request.set_phone_id(slotId);
    request.set_isregister(isRegister);
    request.set_dovoting(doVoting);

    int i = 0;
    for (auto ef : efFiles) {
        ::telStub::IccFile* efFile = request.add_effiles();
        efFile->set_fileid(ef.fileId);
        efFile->set_filepath(ef.filePath);
        LOG(DEBUG, __FUNCTION__, " ef[", i, "].fileId ", static_cast<int>(ef.fileId),
            ", ef[", i, "].filePath ", ef.filePath);
        ++i;
    }

    setRpcRefreshParams(request.mutable_refreshs(), refreshParams);

    grpc::Status reqstatus = stub_->setupRefreshConfig(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        bool hasMatchedEntry = false;
        for (auto it = userRefreshParams_.begin(); it != userRefreshParams_.end(); ++it) {
            if ((it->refreshParams.sessionType == refreshParams.sessionType) &&
                (it->refreshParams.aid == refreshParams.aid)) {
                hasMatchedEntry = true;
                if (isRegister) {
                    it->isRegister = isRegister;
                    it->doVoting = doVoting;
                    LOG(DEBUG, __FUNCTION__, " Registered, update the cached entry");
                    break;
                } else {
                    userRefreshParams_.erase(it);
                    LOG(DEBUG, __FUNCTION__, " unRegistered, erase the entry");
                    break;
                }
            }
        }
        if (not hasMatchedEntry) {
            userRefreshParams_.push_back(userPref);
            LOG(DEBUG, __FUNCTION__, " Registered, store the new entry");
        }
        this->invokeCallback(callback, error, delay);
    }
    return status;
}

telux::common::Status CardManagerStub::allowCardRefresh(SlotId slotId,
    bool allowRefresh, RefreshParams refreshParams, telux::common::ResponseCallback callback) {
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::AllowCardRefreshReq request;
    ::telStub::TelCommonReply response;
    ClientContext context;
    UserRefreshParam userPref;
    request.set_identifier(static_cast<uint32_t>(myPid_));

    LOG(DEBUG, __FUNCTION__, " slotId ", static_cast<int>(slotId),
        ", allowRefresh ", static_cast<int>(allowRefresh),
        ", refreshParams.sessionType ", static_cast<int>(refreshParams.sessionType),
        ", refreshParams.aid ", refreshParams.aid);

    if (slotId != getSlotBySessionType(refreshParams.sessionType)) {
        LOG(ERROR, __FUNCTION__, " conflict slotId and sessionType");
        return telux::common::Status::FAILED;
    }

    /*Need to check whether the refreshParams match with the setupConfig*/
    bool isRegistered = false;
    bool doVoting = false;;
    findRefreshParams(refreshParams, isRegistered, &doVoting, nullptr);
    if (not doVoting) {
        /*User did not setup refresh config to voting*/
        LOG(ERROR, __FUNCTION__, " did not request voting in setupconfig");
        return telux::common::Status::NOTALLOWED;
    }

    request.set_phone_id(slotId);
    request.set_allowrefresh(allowRefresh);
    setRpcRefreshParams(request.mutable_refreshs(), refreshParams);

    grpc::Status reqstatus = stub_->allowCardRefresh(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        this->invokeCallback(callback, error, delay);
    }
    return status;
}

telux::common::Status CardManagerStub::confirmRefreshHandlingCompleted(SlotId slotId,
    bool isCompleted, RefreshParams refreshParams, telux::common::ResponseCallback callback) {
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::ConfirmRefreshHandlingCompleteReq request;
    ::telStub::TelCommonReply response;
    ClientContext context;
    LOG(DEBUG, __FUNCTION__, " slotId ", static_cast<int>(slotId),
        ", isCompleted ", static_cast<int>(isCompleted),
        ", refreshParams.sessionType ", static_cast<int>(refreshParams.sessionType),
        ", refreshParams.aid ", refreshParams.aid);

    if (slotId != getSlotBySessionType(refreshParams.sessionType)) {
        LOG(ERROR, __FUNCTION__, " conflict slotId and sessionType");
        return telux::common::Status::FAILED;
    }

    /*Need to check whether the refreshParams match with the setupConfig*/
    bool isRegistered = false;
    findRefreshParams(refreshParams, isRegistered, nullptr, nullptr);
    if (not isRegistered) {
        LOG(ERROR, __FUNCTION__, " did not register cardRefresh evts");
        return telux::common::Status::NOTALLOWED;
    }

    request.set_identifier(static_cast<uint32_t>(myPid_));
    request.set_phone_id(slotId);
    request.set_iscompleted(isCompleted);
    setRpcRefreshParams(request.mutable_refreshs(), refreshParams);

    grpc::Status reqstatus = stub_->confirmRefreshHandlingCompleted(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        this->invokeCallback(callback, error, delay);
    }
    return status;
}

telux::common::Status CardManagerStub::requestLastRefreshEvent(SlotId slotId,
    RefreshParams refreshParams, refreshLastEventResponseCallback callback) {
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestLastRefreshEventReq request;
    ::telStub::RequestLastRefreshEventResp response;
    ClientContext context;
    LOG(DEBUG, __FUNCTION__, " slotId ", static_cast<int>(slotId),
        ", refreshParams.sessionType ", static_cast<int>(refreshParams.sessionType),
        ", refreshParams.aid ", refreshParams.aid);

    if (slotId != getSlotBySessionType(refreshParams.sessionType)) {
        LOG(ERROR, __FUNCTION__, " conflict slotId and sessionType");
        return telux::common::Status::FAILED;
    }

    /*Need to check whether the refreshParams match with the setupConfig*/
    bool isRegistered = false;
    findRefreshParams(refreshParams, isRegistered, nullptr, nullptr);
    if (not isRegistered) {
        LOG(ERROR, __FUNCTION__, " did not register cardRefresh evts");
        return telux::common::Status::NOTALLOWED;
    }

    request.set_phone_id(slotId);
    setRpcRefreshParams(request.mutable_refreshs(), refreshParams);

    grpc::Status reqstatus = stub_->requestLastRefreshEvent(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());

    if (status == telux::common::Status::SUCCESS && callback) {
        int delay = static_cast<int>(response.delay());
        RefreshStage stage = static_cast<telux::tel::RefreshStage>(response.stage());
        RefreshMode mode = static_cast<telux::tel::RefreshMode>(response.mode());

        std::vector<IccFile> efFiles;
        auto efCnt = response.effiles_size();
        for (int i = 0; i < efCnt; ++i) {
            IccFile file;
            file.fileId = response.effiles(i).fileid();
            file.filePath = response.effiles(i).filepath();
            efFiles.push_back(file);
        }

        RefreshParams respRefreshParams;
        if (response.has_refreshs()) {
            ::telStub::RefreshParams* respRefreshs = request.mutable_refreshs();
            respRefreshParams.sessionType = static_cast<SessionType>(respRefreshs->sessiontype());
            respRefreshParams.aid         = respRefreshs->aid();;
        }

        auto f = std::async(std::launch::async,
            [this, stage, mode, efFiles, respRefreshParams, error, callback, delay]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            callback(stage, mode, efFiles, respRefreshParams, error);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

void CardManagerStub::handleRefreshEvent(::telStub::RefreshEvent event) {
    RefreshParams refreshParams;

    /*1. sanity check of the event*/
    if (not event.has_refreshs()) {
        LOG(ERROR, __FUNCTION__, " something wrong when assemble the event.");
        return;
    }
    refreshParams.sessionType = static_cast<SessionType>(event.refreshs().sessiontype());
    refreshParams.aid = event.refreshs().aid();
    SlotId slotId = getSlotBySessionType(refreshParams.sessionType);

    LOG(DEBUG, __FUNCTION__, " slotId ", static_cast<int>(slotId),
        ", sessionType ", static_cast<int>(refreshParams.sessionType),
        ", aid ", refreshParams.aid);

    /*2. Need to check whether the refreshParams match with the setupConfig*/
    bool isRegistered = false;
    bool doVoting = false;
    std::vector<IccFile> efFiles;
    findRefreshParams(refreshParams, isRegistered, &doVoting, &efFiles);
    if (not isRegistered) {
        LOG(ERROR, __FUNCTION__, " did not register cardRefresh evts");
        return;
    }

    /*3. check whether client registered EFs include the EF in this event*/
    int userOptionEfssize = efFiles.size();
    int efMatch = 0;
    int notificationEfssize = event.effiles_size();
    std::vector<IccFile> evtEfFiles;
    LOG(ERROR, __FUNCTION__, " userOptionEfssize ", userOptionEfssize,
        ", notificationEfssize ", notificationEfssize);
    for (int i = 0; i < notificationEfssize; ++i) {
        IccFile ef;
        ::telStub::IccFile* efFile = event.mutable_effiles(i);
        if (efFile) {
            ef.fileId   = efFile->fileid();
            ef.filePath = efFile->filepath();
            evtEfFiles.push_back(ef);
            LOG(DEBUG, __FUNCTION__, " ef[", i, "].fileId ", static_cast<int>(ef.fileId),
                ", ef[", i, "].filePath ", ef.filePath);
            for (int j = 0; j < userOptionEfssize; j++) {
                if (ef.fileId == efFiles[j].fileId &&
                    ef.filePath == efFiles[j].filePath) {
                    efMatch++;
                    break;
                }
            }
        }
    }
    if (notificationEfssize != efMatch) {
        LOG(WARNING, __FUNCTION__, " IccFiles matche ", efMatch,
        " of total notification Efssize ", notificationEfssize, " abort!");
        return;
    }

    /*Finally notify listeners if all path*/
    std::vector<std::weak_ptr<ICardListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onRefreshEvent(slotId, static_cast<RefreshStage>(event.stage()),
                    static_cast<RefreshMode>(event.mode()), evtEfFiles, refreshParams);
            }
        }
    }
}

SlotId CardManagerStub::getSlotBySessionType(telux::tel::SessionType st) {
    switch (st) {
        case telux::tel::SessionType::PRIMARY:
        case telux::tel::SessionType::NONPROVISIONING_SLOT_1:
        case telux::tel::SessionType::CARD_ON_SLOT_1:
            return SLOT_ID_1;
            break;
        case telux::tel::SessionType::SECONDARY:
        case telux::tel::SessionType::NONPROVISIONING_SLOT_2:
        case telux::tel::SessionType::CARD_ON_SLOT_2:
            return SLOT_ID_2;
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid sessionType ", static_cast<int>(st));
            break;
    }
    return INVALID_SLOT_ID;
}

void CardManagerStub::findRefreshParams(const RefreshParams& refreshParams, bool& isRegister,
    bool* doVoting, std::vector<IccFile>* efFiles) {
    RefreshParams sessionAid;
    convertRefreshParams(refreshParams, sessionAid);

    for (auto it = userRefreshParams_.begin(); it != userRefreshParams_.end(); ++it) {
        if ((it->refreshParams.sessionType == sessionAid.sessionType) &&
            (it->refreshParams.aid == sessionAid.aid)) {
            LOG(DEBUG, __FUNCTION__, " found matched entry");
            isRegister = it->isRegister;
            if (doVoting) {
                *doVoting = it->doVoting;
            }
            if (efFiles) {
                *efFiles = it->efFiles;
            }
            break;
        }
    }
    LOG(DEBUG, __FUNCTION__, " isRegister ", +isRegister);
    if (doVoting) {
        LOG(DEBUG, __FUNCTION__, " doVoting ", +(*doVoting));
    }
}

telux::common::Status CardManagerStub::registerListener(std::shared_ptr<ICardListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_CARD_FILTER};
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(shared_from_this(), filters);
    }
    return status;
}

telux::common::Status  CardManagerStub::removeListener(std::shared_ptr<ICardListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ICardListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_CARD_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

void CardManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::telStub::cardInfoChange>()) {
        ::telStub::cardInfoChange cardEvent;
        event.UnpackTo(&cardEvent);
        handleCardInfoChanged(cardEvent);
    } else if (event.Is<::telStub::RefreshEvent>()) {
         ::telStub::RefreshEvent refreshEvt;
         event.UnpackTo(&refreshEvt);
         handleRefreshEvent(refreshEvt);
    }
}

void CardManagerStub::handleCardInfoChanged(::telStub::cardInfoChange event) {
    int slotId = event.phone_id();
    LOG(DEBUG, __FUNCTION__, " The Slot id is: ", slotId);
    invokelisteners(slotId);
}

} // end of namespace tel

} // end of namespace telux

