/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/ECallDefines.hpp>
#include "CallManagerStub.hpp"
#include "ECallMsd.hpp"

using namespace telux::common;
using namespace telux::tel;
using namespace std;

#define INVALID_CALL_INDEX -1
#define MAX_NO_OF_CALLS_ALLOWED 2

CallManagerStub::CallManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    noOfSlots_ = 1;                            // Defaulting to 1 for Single SIM.
    if (telux::common::DeviceConfig::isMultiSimSupported()) {  // For DSDA slot count is 2.
        noOfSlots_ = 2;
    }
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_ = DEFAULT_DELAY;
}

telux::common::Status CallManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ICallListener>>();
    if(!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    stub_ = CommonUtils::getGrpcStub<DialerService>();
    if(!stub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate dialer service");
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

void CallManagerStub::initSync() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

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

void CallManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
        std::lock_guard<std::mutex> lock(callManagerMutex_);
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

CallManagerStub::~CallManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    calls_.clear();
    cleanup();
}

void CallManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus CallManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status CallManagerStub::registerListener(std::shared_ptr<ICallListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_CALL_FILTER};
        std::vector<std::weak_ptr<ICallListener>> applisteners;
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 1) {
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.registerListener(shared_from_this(), filters);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::removeListener(std::shared_ptr<ICallListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ICallListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_CALL_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeCall(int phoneId, const std::string &dialNumber,
    std::shared_ptr<IMakeCallCallback> callback) {
    LOG(DEBUG, __FUNCTION__, " Phone Id ", phoneId, " dial number ", dialNumber);
    telux::common::Status status = dialCall(phoneId, dialNumber, callback, makeVoiceCall);
    return status;
}

telux::common::Status CallManagerStub::dialCall(int phoneId, const std::string &dialNumber,
    std::shared_ptr<IMakeCallCallback> callback, CallApi inputApi) {
    LOG(DEBUG, " CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    //Restrict to have only two in progress calls per sub at an instance.
    //Call can be MO or MT or a conference call
    //Note: Conference supported is not added in simulation.
    int callsInConference = 0;
    int callsInProgress = 0;
    std::vector<std::shared_ptr<ICall>> inProgressCalls = getInProgressCalls();
    for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
        ++callIterator) {
        if (phoneId == (*callIterator)->getPhoneId()) {
            if ((*callIterator)->isMultiPartyCall()) {
                callsInConference++;
            } else {
                callsInProgress++;
            }
        }
    }
    //If a CS conference call is present, callsInProgress will treat as one call even though
    //we have the calls as two seperate in inProgressCalls.
    if (callsInConference) {
        callsInProgress++;
    }

    if (callsInProgress >= MAX_NO_OF_CALLS_ALLOWED) {
        LOG(ERROR, __FUNCTION__, " ", callsInProgress,
            " calls already in progress. So dial request not allowed.");
        return telux::common::Status::NOTALLOWED;
    }

    ::telStub::MakeCallRequest request =
        createRequest<::telStub::MakeCallRequest>(phoneId, dialNumber, false, inputApi);
    ::telStub::MakeCallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeCall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        std::string remotePartyNumber  =
            static_cast<std::string>(response.call().remote_party_number());
        int callIndex = static_cast<int>(response.call().call_index());
        int cbDelay = static_cast<int>(response.delay());

        if (status == telux::common::Status::SUCCESS ) {
            // Update call details from server and invoke callback
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, callback,
            nullptr, error);
        } else {
            // Update call details from server
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            nullptr, error);
        }
    }
    return status;
}

/* This function updates the call details in the library cache and invokes the callback and
 * same call object cached by client.
 */

void CallManagerStub::findMatchingCall(int index, std::string remotePartyNumber, int phoneId,
    int cbDelay, std::shared_ptr<IMakeCallCallback> iMakecallback, MakeCallCallback callback,
    telux::common::ErrorCode error) {
    LOG(DEBUG, __FUNCTION__," phoneId:: ", phoneId, "errorcode is ", static_cast<int>(error));
    std::vector<std::shared_ptr<CallStub>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);
    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallStub> call) {
        return find(phoneId, call, remotePartyNumber, index);
    });

    if (iter != std::end(calls_)) {
        size_t value = std::distance(calls_.begin(), iter);
        (*iter)->setCallIndex(index);
        if(iMakecallback) {
            auto f = std::async(std::launch::async,
                [this, error, iter, iMakecallback, cbDelay, value]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    LOG(DEBUG, __FUNCTION__, " invoking callback");
                    iMakecallback->makeCallResponse(error, calls_[value]);
                    if(error != telux::common::ErrorCode::SUCCESS) {
                        // update local cache to clear calls
                        updateCurrentCalls();
                    }
                }).share();
            taskQ_->add(f);

        }
        if(callback) {
            auto f = std::async(std::launch::async,
                [this, error, iter, callback, cbDelay, value]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    LOG(DEBUG, __FUNCTION__, " invoking callback");
                    callback(error, calls_[value]);
                    if(error != telux::common::ErrorCode::SUCCESS) {
                        // update local cache to clear calls
                        LOG(DEBUG, __FUNCTION__, " updating call cache");
                        updateCurrentCalls();
                    }
                }).share();
            taskQ_->add(f);
        }
    }
}

bool CallManagerStub::find(int phoneId, std::shared_ptr<CallStub> call,
    std::string remotePartyNumber, int index) {
    // To identify between when two MO calls to same remote party number
    if (call->getCallIndex() != INVALID_CALL_INDEX) {
        // Remote party number is known by client for a custom number ecall over PS/CS or voice
        // call when the call is dialed
        if((call->getRemotePartyNumber() == remotePartyNumber) && (call->getPhoneId() == phoneId)
            && (call->getCallIndex() == index)) {
            return true;
        } else if((call->getRemotePartyNumber() == "") && (call->getPhoneId() == phoneId)
            && (call->getCallIndex() == index)) {
            // Remote party number is not known by client for a standard ecall when the call is
            // dialed.
            return true;
        } else {
            return false;
        }
    } else {
        // Remote party number is known by client for a custom number ecall over PS/CS or voice
        // call when the call is dialed
        if((call->getRemotePartyNumber() == remotePartyNumber)
            && (call->getPhoneId() == phoneId)) {
            return true;
        } else if((call->getRemotePartyNumber() == "") && (call->getPhoneId() == phoneId)) {
            // Remote party number is not known by client for a standard ecall when the call is
            // dialed.
            return true;
        } else {
            return false;
        }
    }
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::string dialNumber,
    const ECallMsdData &eCallMsdData, int category, std::shared_ptr<IMakeCallCallback> callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, dialNumber, true,
        makeTpsECallOverCSWithMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        std::string remotePartyNumber =
            static_cast<std::string>(response.call().remote_party_number());
        int callIndex = static_cast<int>(response.call().call_index());

        if (status == telux::common::Status::SUCCESS ) {
            // Update call details from server and invoke callback
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, callback,
            nullptr, error);
        } else {
            // Update call details from server
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            nullptr, error);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const ECallMsdData &eCallMsdData,
    int category, int variant, std::shared_ptr<IMakeCallCallback> callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, "", true,
        makeECallWithMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        std::string remotePartyNumber =
            static_cast<std::string>(response.call().remote_party_number());
        int callIndex = static_cast<int>(response.call().call_index());

        if (status == telux::common::Status::SUCCESS ) {
            // Update call details from server and invoke callback
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, callback,
            nullptr, error);
        } else {
            // Update call details from server
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            nullptr, error);
        }
    }
    return status;
}

void CallManagerStub::logCallDetails(std::shared_ptr<ICall> info) {
    LOG(DEBUG, __FUNCTION__,
        " Call Info: remotePartyNumber = ", info->getRemotePartyNumber(),
        ", callIndex = ", info->getCallIndex(),
        ", callDirection = ", static_cast<int>(info->getCallDirection()),
        ", callState = ", static_cast<int>(info->getCallState()));
}

void CallManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::telStub::ECallInfoEvent>()) {
        ::telStub::ECallInfoEvent callevent;
        event.UnpackTo(&callevent);
        handleECallEvent(callevent);
    } else if(event.Is<::telStub::MsdPullRequestEvent>()) {
        ::telStub::MsdPullRequestEvent callevent;
        event.UnpackTo(&callevent);
        handleMsdUpdateRequest(callevent);
    } else if(event.Is<::telStub::CallStateChangeEvent>()) {
        ::telStub::CallStateChangeEvent callevent;
        event.UnpackTo(&callevent);
        handleCallInfoChanged(callevent);
    } else if(event.Is<::telStub::ModifyCallRequestEvent>()) {
        ::telStub::ModifyCallRequestEvent callevent;
        event.UnpackTo(&callevent);
        handleModifyCallRequest(callevent);
    } else if(event.Is<::telStub::RttMessageEvent>()) {
        ::telStub::RttMessageEvent callevent;
        event.UnpackTo(&callevent);
        handleRttMessage(callevent);
    } else if(event.Is<::telStub::ECallRedialInfoEvent>()) {
        ::telStub::ECallRedialInfoEvent callevent;
        event.UnpackTo(&callevent);
        handlECallRedial(callevent);
    } else {
        LOG(DEBUG, __FUNCTION__, "No handling required for other events");
    }
}

void CallManagerStub::handlECallRedial(::telStub::ECallRedialInfoEvent event) {
    int phoneId = event.phone_id();
    ECallRedialInfo info;
    info.willECallRedial = event.will_ecall_redial();
    info.reason = static_cast<telux::tel::ReasonType>(event.reason());
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onECallRedial(phoneId, info);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::handleRttMessage(::telStub::RttMessageEvent event) {
    int phoneId = event.phone_id();
    std::string message = event.message();
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onRttMessage(phoneId, message);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::handleModifyCallRequest(::telStub::ModifyCallRequestEvent event) {
    int phoneId = event.phone_id();
    int callIndex = event.call_index();
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                // RttMode is hardcoded to FULL because modem invokes notification only during
                // upgrade of the call.
                sp->onModifyCallRequest(RttMode::FULL, callIndex, phoneId);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::handleMsdUpdateRequest(::telStub::MsdPullRequestEvent event) {
    int phoneId = event.phone_id();
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->OnMsdUpdateRequest(phoneId);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::handleCallInfoChanged(::telStub::CallStateChangeEvent event) {
    int phoneId = event.phone_id();
    LOG(DEBUG, __FUNCTION__," phoneId ", phoneId);
    std::vector<std::shared_ptr<CallStub>> calls ;
    for (int i = 0; i < event.calls_size(); i++) {
        CallInfo callInfo;
        callInfo.callState = static_cast<telux::tel::CallState>(
            event.calls(i).call_state());
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"CallState is ",
            static_cast<int>(callInfo.callState));
        callInfo.index  =  static_cast<int>(event.calls(i).call_index());
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"CallIndex is ", static_cast<int>(callInfo.index));
        callInfo.callDirection  =  static_cast<telux::tel::CallDirection>(
            event.calls(i).call_direction());
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"Calldirection is ",
            static_cast<int>(callInfo.callDirection));
        callInfo.remotePartyNumber = static_cast<std::string>(
            event.calls(i).remote_party_number());
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"remotePartyNumber is ",
            static_cast<std::string>(callInfo.remotePartyNumber));
        callInfo.callEndCause = static_cast<telux::tel::CallEndCause>(
            event.calls(i).call_end_cause());
        callInfo.sipErrorCode = event.calls(i).sip_error_code();
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"callEndCause is ",
            static_cast<int>(callInfo.callEndCause), " sipErrorCode is ", callInfo.sipErrorCode);
        callInfo.isMultiPartyCall = event.calls(i).is_multi_party_call();
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"isMultiPartyCall is ", callInfo.isMultiPartyCall);
        callInfo.isMpty = event.calls(i).is_mpty();
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"isMpty is ", callInfo.isMpty);
        callInfo.mode = static_cast<telux::tel::RttMode>(event.calls(i).mode());
        callInfo.localRttCapability =
            static_cast<telux::tel::RttMode>(event.calls(i).local_rtt_capability());
        callInfo.peerRttCapability =
            static_cast<telux::tel::RttMode>(event.calls(i).peer_rtt_capability());
        callInfo.callType =
            static_cast<telux::tel::CallType>(event.calls(i).call_type());
        LOG(DEBUG, __FUNCTION__,
            " Rtt mode: ", static_cast<int>(callInfo.mode),
            " Local Rtt capability: ", static_cast<int>(callInfo.localRttCapability),
            " Peer Rtt capability:", static_cast<int>(callInfo.peerRttCapability),
            " Call Type:", static_cast<int>(callInfo.callType));


        auto Info = std::make_shared<CallStub>(phoneId, callInfo);
        {
            calls.emplace_back(Info);
        }
    }
    // updates/removes cached calls
    refreshCachedCalls(phoneId, calls);

    // adds new calls into calls_ list
    addLatestCalls(calls);
}

void CallManagerStub::updateCurrentCalls() {
    for (int i = 1; i <= noOfSlots_; i++) {
        ::telStub::UpdateCurrentCallsRequest request;
        ::google::protobuf::Empty response;
        ClientContext context;

        request.set_phone_id(i);
        LOG(DEBUG, __FUNCTION__, " Requested calls information for slot " , i);
        grpc::Status reqstatus = stub_->updateCalls(&context, request, &response);
        if (reqstatus.ok()) {
            LOG(DEBUG, __FUNCTION__, " Requested calls information for slot is successful ");
        }
        else {
            LOG(ERROR, __FUNCTION__, " Requested calls information for slot failed ");
        }
    }
}


void CallManagerStub::refreshCachedCalls(int phoneId,
    std::vector<std::shared_ptr<CallStub>> &latestCalls) {
    LOG(DEBUG, __FUNCTION__, " Number of latest calls: ", latestCalls.size());

    std::vector<std::shared_ptr<CallStub>> callsToBeNotified;
    {
        std::lock_guard<std::mutex> lock(callManagerMutex_);
        LOG(DEBUG, "Number of inProgress calls: ", calls_.size());

        // we are taking each cached call and checking whether it matches any of the calls
        // in the latest call list. Id it is found then we update the cached call info.
        // If it is not found we assume that the modem has dropped the call.
        // So we add it to the dropped call list.
        for (auto cachedCall = std::begin(calls_); cachedCall != std::end(calls_);) {
            if ((*cachedCall)->getPhoneId() != phoneId) {
                // ignore if this function was called for a different slot.
                cachedCall++;
                continue;
            }
            auto iter = std::find_if(
                std::begin(latestCalls), std::end(latestCalls), [=](
                    std::shared_ptr<CallStub> latestCallInfo) {
                    return (*cachedCall)->match(latestCallInfo);
                });

            if (iter != std::end(latestCalls)) {  // cached call found, update the call with latest
                                                  // call info from ril
                LOG(DEBUG, "Updating call details, Call pointer address ", *cachedCall);
                if ((*cachedCall)->isInfoStale(*iter)) {
                    LOG(DEBUG, "Updating stale call details: ");
                    (*cachedCall)->updateCallInfo(*iter);
                    callsToBeNotified.emplace_back(*cachedCall);
                }
                latestCalls.erase(iter);
                cachedCall++;
            } else {
                LOG(DEBUG,
                    "dropped call found, adding it to droppedCalls_ list and removing from "
                    "calls_ list., Call pointer address ",
                    *cachedCall);
                (*cachedCall)->logCallDetails();  // Logging call details for debugging purposes
                                                  // only
                droppedCalls_.emplace_back(*cachedCall);
                calls_.erase(cachedCall);  // dropped call, remove it
            }
        }
    }
    for (auto callIter = std::begin(callsToBeNotified); callIter != std::end(callsToBeNotified);
        ++callIter) {
        if((*callIter)->getCallState() != CallState::CALL_ENDED) {
            /* Call ended notification is informed to application only after server drops
             * the call.
            */
            notifyCallInfoChange(*callIter);
        }
    }
    notifyAndRemoveDroppedCalls();
}

void CallManagerStub::addLatestCalls(std::vector<std::shared_ptr<CallStub>> &latestCalls) {
    LOG(DEBUG, __FUNCTION__, " Number of latest calls: ", latestCalls.size());
    for (std::shared_ptr<CallStub> ci : latestCalls) {
        {
            std::lock_guard<std::mutex> lock(callManagerMutex_);
            calls_.emplace_back(ci);
        }
        if (ci->getCallState() == CallState::CALL_INCOMING
            || ci->getCallState() == CallState::CALL_WAITING) {
            notifyIncomingCall(ci);
        } else {
            LOG(DEBUG, __FUNCTION__, " CallManager: notifying listeners about the new call");
            notifyCallInfoChange(ci);
        }
    }
}

/**
 * Update call state on dropped calls and remove them
 */
void CallManagerStub::notifyAndRemoveDroppedCalls() {
    std::vector<std::shared_ptr<CallStub>> callsToBeNotified;
    LOG(DEBUG, __FUNCTION__);
    {
        std::lock_guard<std::mutex> lock(callManagerMutex_);
        LOG(DEBUG, "Size of droppedCalls_ vector is ", droppedCalls_.size());
        for (auto droppedCall = std::begin(droppedCalls_); droppedCall != std::end(droppedCalls_);
             droppedCall++) {
            (*droppedCall)->setCallState(CallState::CALL_ENDED);
            // Move dropped calls into a local list for notifying listeners
            callsToBeNotified.emplace_back(std::move(*droppedCall));
            droppedCalls_.erase(droppedCall--);
        }
    }

    // Notify listeners about the call info change
    for (auto callIter = std::begin(callsToBeNotified); callIter != std::end(callsToBeNotified);
         ++callIter) {
        LOG(DEBUG, "Processing droppedCall ", *callIter);
        notifyCallInfoChange(*callIter);
        LOG(DEBUG, "Processing droppedCall ", *callIter, " completed.");
    }
}

/**
 * notifyCallInfoChange is a call back implementation
 * This will be invoked to notify call info changes
 */
void CallManagerStub::notifyCallInfoChange(std::shared_ptr<ICall> call) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        for (auto &wp : applisteners) {
            if (auto sp = std::dynamic_pointer_cast<ICallListener>(wp.lock())) {
                LOG(DEBUG, "found listener for call info change:", sp);
                sp->onCallInfoChange(call);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

/**
 * notifyIncomingCall is a call back implementation
 * This will be invoked to notify the listeners about incoming calls
 */
void CallManagerStub::notifyIncomingCall(std::shared_ptr<ICall> call) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        for (auto &wp : applisteners) {
            if (auto sp = std::dynamic_pointer_cast<ICallListener>(wp.lock())) {
                LOG(DEBUG, "found listener for incoming call:", sp);
                sp->onIncomingCall(call);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::handleECallEvent(::telStub::ECallInfoEvent event) {

    int slotId = event.phone_id();
    HlapTimerEvent action = static_cast<HlapTimerEvent>(event.action());
    std::string input = event.timer();

    ECallHlapTimerEvents timersStatus;
    timersStatus.t2 = HlapTimerEvent::UNCHANGED;
    timersStatus.t5 = HlapTimerEvent::UNCHANGED;
    timersStatus.t6 = HlapTimerEvent::UNCHANGED;
    timersStatus.t7 = HlapTimerEvent::UNCHANGED;
    timersStatus.t9 = HlapTimerEvent::UNCHANGED;
    timersStatus.t10 = HlapTimerEvent::UNCHANGED;
    if(input == "T2Timer") {
        timersStatus.t2 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T5Timer") {
        timersStatus.t5 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T6Timer") {
        timersStatus.t6 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T7Timer") {
        timersStatus.t7 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T9Timer") {
        timersStatus.t9 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T10Timer") {
        timersStatus.t10 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "START_RECEIVED") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::START_RECEIVED);
    } else if(input == "MSD_TRANSMISSION_STARTED") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::MSD_TRANSMISSION_STARTED);
    } else if(input == "LL_ACK_RECEIVED") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::LL_ACK_RECEIVED);
    } else if(input == "MSD_TRANSMISSION_SUCCESS") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::SUCCESS);
        invokeECallMsdTransmissionStatuslisteners(slotId, telux::common::ErrorCode::SUCCESS);
    } else if(input == "MSD_TRANSMISSION_FAILURE") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::FAILURE);
        invokeECallMsdTransmissionStatuslisteners(slotId,
            telux::common::ErrorCode::GENERIC_FAILURE);
    } else if(input == "OUTBAND_MSD_TRANSMISSION_STARTED") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_STARTED);
    } else if(input == "OUTBAND_MSD_TRANSMISSION_SUCCESS") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_SUCCESS);
        invokeECallMsdTransmissionStatuslisteners(slotId, telux::common::ErrorCode::SUCCESS);
    } else if(input == "OUTBAND_MSD_TRANSMISSION_FAILURE") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_FAILURE);
        invokeECallMsdTransmissionStatuslisteners(slotId,
            telux::common::ErrorCode::GENERIC_FAILURE);
    } else if(input == "LL_NACK_DUE_TO_T7_EXPIRY") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::LL_NACK_DUE_TO_T7_EXPIRY);
    } else if(input == "MSD_AL_ACK_CLEARDOWN") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::MSD_AL_ACK_CLEARDOWN);
    } else {
        LOG(ERROR, __FUNCTION__, "No supported event ");
    }
}

void CallManagerStub::invokeECallMsdTransmissionStatuslisteners(int phoneId,
    telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus ) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onECallMsdTransmissionStatus(phoneId, msdTransmissionStatus);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::invokeECallMsdTransmissionStatuslisteners(int phoneId,
    telux::common::ErrorCode errorCode ) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onECallMsdTransmissionStatus(phoneId, errorCode);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::invokeECallHlapTimerEventlisteners(int phoneId,
    ECallHlapTimerEvents timersStatus) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onECallHlapTimerEvent(phoneId, timersStatus);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::string dialNumber,
    const std::vector<uint8_t> &msdPdu, CustomSipHeader header, MakeCallCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, dialNumber, true,
        makeTpsECallOverIMS);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        std::string remotePartyNumber =
            static_cast<std::string>(response.call().remote_party_number());
        int callIndex = static_cast<int>(response.call().call_index());
        if (status == telux::common::Status::SUCCESS ) {
            // Update call details from server and invoke callback
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            callback, error);
        } else {
            // Update call details from server
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            nullptr, error);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::vector<uint8_t> &msdPdu,
    int category, int variant, MakeCallCallback callback) {
	LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, "", true,
        makeECallWithRawMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;
    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;

    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        std::string remotePartyNumber =
            static_cast<std::string>(response.call().remote_party_number());
        int callIndex = static_cast<int>(response.call().call_index());
        if (status == telux::common::Status::SUCCESS ) {
            // Update call details from server and invoke callback
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            callback, error);
        } else {
            // Update call details from server
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            nullptr, error);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::string dialNumber,
    const std::vector<uint8_t> &msdPdu, int category, MakeCallCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, dialNumber, true,
        makeTpsECallOverCSWithRawMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        std::string remotePartyNumber =
            static_cast<std::string>(response.call().remote_party_number());
        int callIndex = static_cast<int>(response.call().call_index());

        if (status == telux::common::Status::SUCCESS ) {
            // Update call details from server and invoke callback
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            callback, error);
        } else {
            // Update call details from server
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            nullptr, error);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, int category, int variant,
    MakeCallCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, "", false,
        makeECallWithoutMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        std::string remotePartyNumber =
            static_cast<std::string>(response.call().remote_party_number());
        int callIndex = static_cast<int>(response.call().call_index());

        if (status == telux::common::Status::SUCCESS ) {
            // Update call details from server and invoke callback
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            callback, error );
        } else {
            // Update call details from server
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            nullptr, error);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::string dialNumber,
    int category, MakeCallCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeCallRequest request =
        createRequest<::telStub::MakeCallRequest>(phoneId, dialNumber, false,
        makeTpsECallOverCSWithoutMsd);
    ::telStub::MakeCallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeCall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        std::string remotePartyNumber =
            static_cast<std::string>(response.call().remote_party_number());
        int callIndex = static_cast<int>(response.call().call_index());
        if (status == telux::common::Status::SUCCESS ) {
            // Update call details from server and invoke callback
            findMatchingCall(callIndex, remotePartyNumber,phoneId, cbDelay, nullptr,
                callback, error);
        } else {
            // Update call details from server
            findMatchingCall(callIndex, remotePartyNumber, phoneId, cbDelay, nullptr,
            nullptr, error);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::updateECallMsd(int phoneId, const ECallMsdData &eCallMsd,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {

    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::UpdateECallMsdRequest request;
    ::telStub::UpdateECallMsdResponse response;
    ClientContext context;

    request.set_phone_id(phoneId);
    request.set_api(updateEcallMsd);

    grpc::Status reqstatus = stub_->UpdateECallMsd(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback->commandResponse(error);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::updateECallMsd(int phoneId,
    const std::vector<uint8_t> &msdPdu, telux::common::ResponseCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::UpdateECallMsdRequest request;
    ::telStub::UpdateECallMsdResponse response;
    ClientContext context;

    request.set_phone_id(phoneId);
    request.set_api(updateECallRawMsd);

    grpc::Status reqstatus = stub_->UpdateECallMsd(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::requestECallHlapTimerStatus(int phoneId,
    ECallHlapTimerStatusCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::RequestECallHlapTimerStatusRequest request;
    ::telStub::RequestECallHlapTimerStatusReply response;
    ClientContext context;

    request.set_phone_id(phoneId);

    grpc::Status reqstatus = stub_->RequestECallHlapTimerStatus(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        ECallHlapTimerStatus timersStatus;
        timersStatus.t2 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t2());
        timersStatus.t5 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t5());
        timersStatus.t6 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t6());
        timersStatus.t7 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t7());
        timersStatus.t9 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t9());
        timersStatus.t10 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t10());
        int cbDelay = static_cast<int>(response.delay());
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, phoneId, timersStatus, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error, phoneId, timersStatus);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

std::vector<std::shared_ptr<ICall>> CallManagerStub::getInProgressCalls() {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<ICall>> iCalls(calls_.begin(), calls_.end());
    return iCalls;
}

telux::common::Status CallManagerStub::conference(std::shared_ptr<ICall> call1,
    std::shared_ptr<ICall> call2,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status CallManagerStub::swap(std::shared_ptr<ICall> callToHold,
    std::shared_ptr<ICall> callToActivate,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "CallMgr - ", __FUNCTION__);
    if ((callToHold == nullptr) || (callToActivate == nullptr)) {
        LOG(ERROR, "Unable to initiate swap operation as null call objects are passed");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    if (callToHold->getPhoneId() != callToActivate->getPhoneId()) {
        LOG(ERROR, "Unable to initiate swap operation as phoneId for both calls are different");
        return telux::common::Status::INVALIDPARAM;
    }
    ::telStub::SwapRequest request;
    ::telStub::SwapReply response;
    ClientContext context;

    if ((callToHold->getCallState() == CallState::CALL_ON_HOLD)
         && (callToActivate->getCallState() == CallState::CALL_ACTIVE)) {
        request.set_call_to_hold_index(callToHold->getCallIndex());
        request.set_phone_id(callToHold->getPhoneId());
        request.set_call_to_activate_index(callToActivate->getCallIndex());
    }
    else if ((callToHold->getCallState() == CallState::CALL_ACTIVE)
        && (callToActivate->getCallState() == CallState::CALL_ON_HOLD)) {
        request.set_call_to_hold_index(callToActivate->getCallIndex());
        request.set_phone_id(callToActivate->getPhoneId());
        request.set_call_to_activate_index(callToHold->getCallIndex());
    } else {
        LOG(ERROR, "Unable to initiate swap calls due to calls in wrong state");
        return telux::common::Status::INVALIDSTATE;
    }

    grpc::Status reqstatus = stub_->Swap(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());

        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            if(callback) {
                auto f = std::async(std::launch::async,
                    [this, error, callback, delay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback->commandResponse(error);
                    }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::hangupForegroundResumeBackground(int phoneId,
    common::ResponseCallback callback ) {
    LOG(DEBUG, __FUNCTION__, " SlotId: ", phoneId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::HangupForegroundResumeBackgroundRequest request;
    ::telStub::HangupForegroundResumeBackgroundReply response;
    ClientContext context;
    request.set_phone_id(phoneId);
    grpc::Status reqstatus = stub_->HangupForegroundResumeBackground(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            if(callback) {
                auto f = std::async(std::launch::async,
                    [this, error, callback, delay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback(error);
                    }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::hangupWaitingOrBackground(int phoneId,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " SlotId: ", phoneId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::HangupWaitingOrBackgroundRequest request;
    ::telStub::HangupWaitingOrBackgroundReply response;
    ClientContext context;
    request.set_phone_id(phoneId);
    grpc::Status reqstatus = stub_->HangupWaitingOrBackground(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        if(callback) {
                auto f = std::async(std::launch::async,
                    [this, error, callback, delay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback(error);
                    }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::requestEcbm(int phoneId, EcbmStatusCallback callback) {
   LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::RequestEcbmRequest request;
    ::telStub::RequestEcbmReply response;
    ClientContext context;
    request.set_phone_id(phoneId);

    grpc::Status reqstatus = stub_->RequestEcbm(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        telux::tel::EcbMode ecbMode = static_cast<telux::tel::EcbMode>(response.ecbmode());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
            if(callback) {
                auto f = std::async(std::launch::async,
                    [this, error , callback, ecbMode, cbDelay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                        callback(ecbMode, error);
                    }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::exitEcbm(int phoneId, common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::ExitEcbmRequest request;
    ::telStub::ExitEcbmReply response;
    ClientContext context;
    request.set_phone_id(phoneId);
    grpc::Status reqstatus = stub_->ExitEcbm(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
            auto f = std::async(std::launch::async,
                [this, error, cbDelay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback(error);
                }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::requestNetworkDeregistration(int phoneId,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::RequestNetworkDeregistrationRequest request;
    ::telStub::RequestNetworkDeregistrationReply response;
    ClientContext context;
    request.set_phone_id(phoneId);
    grpc::Status reqstatus = stub_->RequestNetworkDeregistration(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
            auto f = std::async(std::launch::async,
                [this, error, cbDelay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback(error);
                }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::updateEcallHlapTimer(int phoneId, HlapTimerType type,
    uint32_t timeDuration, common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    if(type == HlapTimerType::T10_TIMER) {
        ::telStub::UpdateEcallHlapTimerRequest request;
        ::telStub::UpdateEcallHlapTimerResponse response;
        ClientContext context;
        request.set_phone_id(phoneId);
        request.set_type(static_cast<::telStub::HlapTimerType>(type));
        request.set_time_duration(timeDuration);
        grpc::Status reqstatus = stub_->UpdateEcallHlapTimer(&context, request, &response);
        if (reqstatus.ok()) {
            telux::common::ErrorCode error =
                static_cast<telux::common::ErrorCode>(response.error());
            status = static_cast<telux::common::Status>(response.status());
            int cbDelay = static_cast<int>(response.delay());
            bool isCallbackNeeded = static_cast<bool>(response.iscallback());

            if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
                auto f = std::async(std::launch::async,
                    [this, error , callback, cbDelay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                        callback(error);
                    }).share();
                taskQ_->add(f);
            }
        }
    } else {
        status = telux::common::Status::NOTSUPPORTED;
    }
    return status;
}

telux::common::Status CallManagerStub::requestEcallHlapTimer(int phoneId, HlapTimerType type,
    ECallHlapTimerCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;

    if(type == HlapTimerType::T10_TIMER) {
        ::telStub::RequestEcallHlapTimerRequest request;
        ::telStub::RequestEcallHlapTimerReply response;
        ClientContext context;

        request.set_phone_id(phoneId);
        request.set_type(static_cast<::telStub::HlapTimerType>(type));
        grpc::Status reqstatus = stub_->RequestEcallHlapTimer(&context, request, &response);
        if (reqstatus.ok()) {
            telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
            status = static_cast<telux::common::Status>(response.status());
            int cbDelay = static_cast<int>(response.delay());
            bool isCallbackNeeded = static_cast<bool>(response.iscallback());
            int timeDuration = static_cast<int>(response.time_duration());
            if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
                if(callback) {
                    auto f = std::async(std::launch::async,
                    [this, error , callback, cbDelay, timeDuration]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                        callback(error, timeDuration);
                    }).share();
                    taskQ_->add(f);
                }
            }
        }
    } else {
        status = telux::common::Status::NOTSUPPORTED;
    }
    return status;
}

telux::common::Status CallManagerStub::setECallConfig(EcallConfig config) {
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetConfigRequest request;
    ::telStub::SetConfigReply response;
    ClientContext context;

    auto validityMask = config.configValidityMask;
    LOG(INFO, __FUNCTION__, " configValidityMask: ", validityMask.to_string());
    if(validityMask.test(ECALL_CONFIG_MUTE_RX_AUDIO)) {
        request.set_mute_rx_audio(static_cast<bool>(config.muteRxAudio));
        request.set_is_mute_rx_audio_valid(static_cast<bool>(true));
    }
    if(validityMask.test(ECALL_CONFIG_NUM_TYPE)) {
        request.set_is_num_type_valid(static_cast<bool>(true));
        if(config.numType == ECallNumType::DEFAULT) {
            request.set_num_type(static_cast<::telStub::ECallNumType>(
                telux::tel::ECallNumType::DEFAULT));
        } else {
            request.set_num_type(static_cast<::telStub::ECallNumType>(
                telux::tel::ECallNumType::OVERRIDDEN));
        }
    }
    if(validityMask.test(ECALL_CONFIG_OVERRIDDEN_NUM)) {
        request.set_is_overridden_num_valid(static_cast<bool>(true));
        request.set_overridden_num(config.overriddenNum);
    }
    if(validityMask.test(ECALL_CONFIG_USE_CANNED_MSD)) {
        request.set_is_use_canned_msd_valid(static_cast<bool>(true));
        request.set_use_canned_msd(config.useCannedMsd);
    }
    if(validityMask.test(ECALL_CONFIG_GNSS_UPDATE_INTERVAL)) {
        request.set_is_gnss_update_interval_valid(static_cast<bool>(true));
        request.set_gnss_update_interval(config.gnssUpdateInterval);
    }
    if(validityMask.test(ECALL_CONFIG_T2_TIMER)) {
        request.set_is_t2_timer_valid(static_cast<bool>(true));
        LOG(INFO, __FUNCTION__, " t2 timer value is : ", config.t2Timer);
        request.set_t2_timer(config.t2Timer);
    }
    if(validityMask.test(ECALL_CONFIG_T7_TIMER)) {
        request.set_is_t7_timer_valid(static_cast<bool>(true));
        request.set_t7_timer(config.t7Timer);
    }
    if(validityMask.test(ECALL_CONFIG_T9_TIMER)) {
        request.set_is_t9_timer_valid(static_cast<bool>(true));
        request.set_t9_timer(config.t9Timer);
    }
    if(validityMask.test(ECALL_CONFIG_MSD_VERSION)) {
        request.set_is_msd_version_valid(static_cast<bool>(true));
        request.set_msd_version(config.msdVersion);
    }
    grpc::Status reqstatus = stub_->SetConfig(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
    }
    return status;
}

telux::common::Status CallManagerStub::getECallConfig(EcallConfig &config) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    const ::google::protobuf::Empty request;
    ::telStub::GetConfigResponse response;
    ClientContext context;
    EcallConfigValidity validityMask;
    validityMask.reset();
    config = {};
    grpc::Status reqstatus = stub_->GetConfig(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        if(response.is_mute_rx_audio_valid() == true) {
            validityMask.set(ECALL_CONFIG_MUTE_RX_AUDIO);
            config.muteRxAudio = response.mute_rx_audio();
        }
        if(response.is_num_type_valid() == true) {
            validityMask.set(ECALL_CONFIG_NUM_TYPE);
            config.numType = static_cast<telux::tel::ECallNumType>(response.num_type());
        }
        if(response.is_overridden_num_valid() == true) {
                validityMask.set(ECALL_CONFIG_OVERRIDDEN_NUM);
                config.overriddenNum = response.overridden_num();
        }
        if(response.is_use_canned_msd_valid() == true) {
            validityMask.set(ECALL_CONFIG_USE_CANNED_MSD);
            config.useCannedMsd = response.use_canned_msd();
        }
        if(response.is_gnss_update_interval_valid() == true) {
            validityMask.set(ECALL_CONFIG_GNSS_UPDATE_INTERVAL);
            config.gnssUpdateInterval = response.gnss_update_interval();
        }
        if(response.is_t2_timer_valid() == true) {
            validityMask.set(ECALL_CONFIG_T2_TIMER);
            config.t2Timer = response.t2_timer();
        }
        if(response.is_t7_timer_valid() == true) {
            validityMask.set(ECALL_CONFIG_T7_TIMER);
            config.t7Timer = response.t7_timer();
        }
        if(response.is_t9_timer_valid() == true) {
            validityMask.set(ECALL_CONFIG_T9_TIMER);
            config.t9Timer = response.t9_timer();
        }
        if(response.is_msd_version_valid() == true) {
            validityMask.set(ECALL_CONFIG_MSD_VERSION);
            config.msdVersion = response.msd_version();
        }
        config.configValidityMask = validityMask;
        LOG(INFO, __FUNCTION__, " configValidityMask: ", validityMask.to_string());
    }
    return status;
}

telux::common::Status CallManagerStub::encodeEuroNcapOptionalAdditionalData(
    telux::tel::ECallOptionalEuroNcapData optionalEuroNcapData, std::vector<uint8_t> &data) {
    LOG(DEBUG, __FUNCTION__);
    ECallMsd &eCallMsd = ECallMsd::getInstance();
    auto status = eCallMsd.encodeEuroNcapOptionalAdditionalDataContent(optionalEuroNcapData, data);
    return status;
}

telux::common::ErrorCode CallManagerStub::encodeECallMsd(telux::tel::ECallMsdData eCallMsdData,
    std::vector<uint8_t> &data) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode errCode = telux::common::ErrorCode::UNKNOWN;
    ECallMsd &eCallMsd               = ECallMsd::getInstance();
    eCallMsd.logMsd(eCallMsdData);
    auto status = eCallMsd.generateECallMsd(eCallMsdData, data);
    LOG(DEBUG, __FUNCTION__, " Status : ", static_cast<int>(status));
    switch (status) {
        case telux::common::Status::SUCCESS:
            errCode = telux::common::ErrorCode::SUCCESS;
            break;
        case telux::common::Status::FAILED:
            errCode = telux::common::ErrorCode::GENERIC_FAILURE;
            break;
        case telux::common::Status::INVALIDPARAM:
            errCode = telux::common::ErrorCode::INVALID_ARGUMENTS;
            break;
        default:
            errCode = telux::common::ErrorCode::GENERIC_FAILURE;
            break;
    }
    if (errCode != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to generate MSD and error is ",
            static_cast<int>(errCode));
    }
    return errCode;
}

telux::common::Status CallManagerStub::makeRttCall(int phoneId, const std::string &dialNumber,
    std::shared_ptr<IMakeCallCallback> callback) {
    LOG(DEBUG, __FUNCTION__, " Phone Id ", phoneId, " dial number ", dialNumber);
    telux::common::Status status = dialCall(phoneId, dialNumber, callback,
        makeRttVoiceCall);
    return status;
}

telux::common::Status CallManagerStub::sendRtt(int phoneId, std::string message,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " Phone Id ", phoneId);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " CallManager is not ready ");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SendRttRequest request;
    ::telStub::SendRttReply response;
    ClientContext context;
    // Text message obtained from the user is not sent to server as there is no need to store
    // or manipulate messages at server.
    request.set_phone_id(phoneId);
    telux::common::Status status = telux::common::Status::FAILED;
    grpc::Status reqstatus = stub_->SendRtt(&context, request, &response);
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::configureECallRedial(RedialConfigType config,
    const std::vector<int> &timeGap, common::ResponseCallback callback) {
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " CallManager is not ready ");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::ConfigureECallRedialRequest request;
    ::telStub::ConfigureECallRedialResponse response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;

    int size = timeGap.size();
    for (int j = 0; j < size ; j++)
    {
        int d = timeGap[j];
        request.add_time_gap(d);
    }
    request.set_config(static_cast<telStub::RedialConfigType>(config));
    grpc::Status reqstatus = stub_->ConfigureECallRedial(&context, request, &response);
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::restartECallHlapTimer(int phoneId, EcallHlapTimerId timerId,
    int duration, common::ResponseCallback callback ) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::string dialNumber,
    const std::vector<uint8_t> &msdPdu, MakeCallCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status CallManagerStub::updateECallPostTestRegistrationTimer(int phoneId,
    uint32_t timer, common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::ErrorCode CallManagerStub::getECallPostTestRegistrationTimer(int phoneId,
    uint32_t &timer) {
    return telux::common::ErrorCode::GENERIC_FAILURE;
}

telux::common::ErrorCode CallManagerStub::getECallRedialConfig(std::vector<int> &callOrigTimeGap,
    std::vector<int> &callDropTimeGap)  {
    return telux::common::ErrorCode::GENERIC_FAILURE;
}
