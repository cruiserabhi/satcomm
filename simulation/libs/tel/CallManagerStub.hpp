/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       CallManagerStub.hpp
 *
 * @brief      Implementation of CallManager
 *
 */

#ifndef CALL_MANAGER_STUB_HPP
#define CALL_MANAGER_STUB_HPP


#include <telux/tel/CallManager.hpp>
#include <telux/tel/CallListener.hpp>
#include "CallStub.hpp"
#include "../common/event-manager/ClientEventManager.hpp"
#include "../common/ListenerManager.hpp"
#include "TelDefinesStub.hpp"
#include "Helper.hpp"

namespace telux {
namespace tel {

class CallManagerStub : public ICallManager,
                        public IEventListener,
                        public std::enable_shared_from_this<CallManagerStub> {
public:

    CallManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener
        (std::shared_ptr<telux::tel::ICallListener> listener) override;

    telux::common::Status removeListener
        (std::shared_ptr<telux::tel::ICallListener> listener) override;

    telux::common::Status makeCall(int phoneId, const std::string &dialNumber,
                                        std::shared_ptr<IMakeCallCallback> callback) override;
    telux::common::Status makeECall(int phoneId, const ECallMsdData &eCallMsdData,
                                           int category, int variant,
                                           std::shared_ptr<IMakeCallCallback> callback) override;
    telux::common::Status makeECall(int phoneId, const std::string dialNumber,
        const std::vector<uint8_t> &msdPdu, CustomSipHeader header,
        MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, const std::vector<uint8_t> &msdPdu,
                                            int category, int variant,
                                            MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, const std::string dialNumber,
                                           const std::vector<uint8_t> &msdPdu, int category,
                                           MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, int category, int variant,
                                            MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, const std::string dialNumber, int category,
                                           MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, const std::string dialNumber,
                                           const ECallMsdData &eCallMsdData, int category,
                                           std::shared_ptr<IMakeCallCallback> callback) override;
    telux::common::Status updateECallMsd(int phoneId, const ECallMsdData &eCallMsd,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback) override;
    telux::common::Status updateECallMsd(int phoneId, const std::vector<uint8_t> &msdPdu,
        telux::common::ResponseCallback callback) override;
    telux::common::Status requestECallHlapTimerStatus(int phoneId,
        ECallHlapTimerStatusCallback callback) override;
    std::vector<std::shared_ptr<ICall>> getInProgressCalls() override;
    telux::common::Status conference(std::shared_ptr<ICall> call1, std::shared_ptr<ICall> call2,
                    std::shared_ptr<telux::common::ICommandResponseCallback> callback) override;
    telux::common::Status swap(std::shared_ptr<ICall> callToHold,
        std::shared_ptr<ICall> callToActivate,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback) override;
    telux::common::Status hangupForegroundResumeBackground(int phoneId,
        common::ResponseCallback callback ) override;
    telux::common::Status hangupWaitingOrBackground(int phoneId,
        common::ResponseCallback callback) override;
    telux::common::Status requestEcbm(int phoneId, EcbmStatusCallback callback) override;
    telux::common::Status exitEcbm(int phoneId, common::ResponseCallback callback) override;
    telux::common::Status requestNetworkDeregistration(int phoneId,
        common::ResponseCallback callback) override;
    telux::common::Status updateEcallHlapTimer(int phoneId, HlapTimerType type,
        uint32_t timeDuration, common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestEcallHlapTimer(int phoneId, HlapTimerType type,
        ECallHlapTimerCallback callback) override;
    telux::common::Status setECallConfig(EcallConfig config) override;
    telux::common::Status getECallConfig(EcallConfig &config) override;
    telux::common::Status encodeEuroNcapOptionalAdditionalData(
        telux::tel::ECallOptionalEuroNcapData optionalEuroNcapData,
        std::vector<uint8_t> &data) override;
    telux::common::ErrorCode encodeECallMsd(telux::tel::ECallMsdData eCallMsdData,
        std::vector<uint8_t> &data) override;
    telux::common::Status makeRttCall(int phoneId, const std::string &dialNumber,
        std::shared_ptr<IMakeCallCallback> callback) override;
    telux::common::Status sendRtt(int phoneId,
      std::string message, common::ResponseCallback callback) override;
    telux::common::Status configureECallRedial(RedialConfigType config,
        const std::vector<int> &timeGap, common::ResponseCallback callback) override;
    telux::common::Status restartECallHlapTimer(int phoneId, EcallHlapTimerId timerId,
        int duration, common::ResponseCallback callback ) override;
    telux::common::ErrorCode getECallRedialConfig(std::vector<int> &callOrigTimeGap,
        std::vector<int> &callDropTimeGap) override;
    telux::common::Status makeECall(int phoneId, const std::string dialNumber,
        const std::vector<uint8_t> &msdPdu, MakeCallCallback callback) override;
    telux::common::Status updateECallPostTestRegistrationTimer(int phoneId, uint32_t timer,
        common::ResponseCallback callback) override;
    telux::common::ErrorCode getECallPostTestRegistrationTimer(int phoneId,
        uint32_t &timer) override;
    ~CallManagerStub();
    void cleanup();
    void onEventUpdate(google::protobuf::Any event)  override;
    void updateCurrentCalls();

private:
    int noOfSlots_;
    telux::common::InitResponseCb initCb_;
    int cbDelay_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::mutex callManagerMutex_;
    std::shared_ptr<telux::common::ListenerManager<ICallListener>> listenerMgr_;
    telux::common::ServiceStatus subSystemStatus_;
    void setServiceStatus(telux::common::ServiceStatus status);
    void initSync();
    void handleECallEvent(::telStub::ECallInfoEvent event);
    void handleCallInfoChanged(::telStub::CallStateChangeEvent event);
    void handleMsdUpdateRequest(::telStub::MsdPullRequestEvent event);
    void handleModifyCallRequest(::telStub::ModifyCallRequestEvent event);
    void handleRttMessage(::telStub::RttMessageEvent event);
    void handlECallRedial(::telStub::ECallRedialInfoEvent event);
    void invokeECallHlapTimerEventlisteners(int phoneId,
        ECallHlapTimerEvents timersStatus);
    void invokeECallMsdTransmissionStatuslisteners(int phoneId,
        telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus );
    void invokeECallMsdTransmissionStatuslisteners(int phoneId,
        telux::common::ErrorCode errorCode );
    void logCallDetails(std::shared_ptr<ICall> info);
    void findMatchingCall(int index, std::string remotePartyNumber, int phoneId, int cbDelay,
        std::shared_ptr<IMakeCallCallback> iMakecallback, MakeCallCallback callback,
        telux::common::ErrorCode error);
    bool find(int phoneId, std::shared_ptr<CallStub> call, std::string remotePartyNumber,
        int index);
    void findAndRemoveMatchingCall(int phoneId, int index);
    std::unique_ptr<::telStub::DialerService::Stub> stub_;
    std::vector<std::shared_ptr<CallStub>> calls_;
    std::vector<std::shared_ptr<CallStub>> droppedCalls_;
    void notifyIncomingCall(std::shared_ptr<ICall> call);
    void notifyCallInfoChange(std::shared_ptr<ICall> call);
    void addLatestCalls(std::vector<std::shared_ptr<CallStub>> &latestCalls);
    void refreshCachedCalls(int phoneId, std::vector<std::shared_ptr<CallStub>> &latestCalls);
    void notifyAndRemoveDroppedCalls();
    void onEventUpdate(std::string event);
    telux::common::Status dialCall(int phoneId, const std::string &dialNumber,
        std::shared_ptr<IMakeCallCallback> callback, CallApi inputApi);
    template <typename T>
    T createRequest(int phoneId,
        const std::string dialNumber, bool isMsdTransmitted, CallApi inputApi) {
        T request;
        CallInfo callInfo;
        callInfo.remotePartyNumber = dialNumber;
        callInfo.callDirection = CallDirection::OUTGOING;
        callInfo.callState = CallState::CALL_IDLE;
        callInfo.transmitMsd = isMsdTransmitted;
        auto call = std::make_shared<CallStub>(phoneId, callInfo);
        {
            std::lock_guard<std::mutex> lock(callManagerMutex_);
            calls_.emplace_back(call);
        }
        request.set_phone_id(phoneId);
        if (dialNumber != "") { // Regulatory eCall
            request.set_remote_party_number(dialNumber);
        }
        request.set_is_msd_transmitted(isMsdTransmitted);
        request.set_api(inputApi);
        return request;
    }
};

} // end of namespace tel

} // end of namespace telux

#endif // CALL_MANAGER_STUB_HPP
