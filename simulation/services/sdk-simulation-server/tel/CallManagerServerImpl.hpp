/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * @file       CallManagerServerImpl.hpp
 *
 */

#ifndef CALL_MANAGER_SERVER_HPP
#define CALL_MANAGER_SERVER_HPP

#include <telux/common/CommonDefines.hpp>
#include "../../../libs/common/JsonParser.hpp"
#include "../../../protos/proto-src/tel_simulation.grpc.pb.h"
#include "../../../libs/common/event-manager/EventParserUtil.hpp"
#include "../../../libs/common/CommonUtils.hpp"
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/ECallDefines.hpp>
#include "EcallStateMachine.hpp"
#include "../event/ServerEventManager.hpp"
#include "../event/EventService.hpp"
#include "../../../libs/tel/Helper.hpp"
#include "TelUtil.hpp"
#include <mutex>
#include <condition_variable>

namespace telux {
namespace tel {
    class EcallStateMachine;
}  // namespace tel
}  // namespace telux

#define CALL_INDEX_INVALID -1

using namespace telux::tel;

struct CallInfo {
   CallState callState;
   int index = CALL_INDEX_INVALID;
   CallDirection callDirection = CallDirection::NONE;
   std::string remotePartyNumber = "";
   telux::tel::CallEndCause callEndCause = telux::tel::CallEndCause::NORMAL;
   int sipErrorCode = 0;
   int phoneId;
   bool isRegulatoryeCall = false;
   bool isMultiPartyCall = false;
   bool isMsdTransmitted = false;
   bool isMpty = false;
   bool isTpseCallOverIms = false;
   RttMode mode = RttMode::DISABLED;
   RttMode localRttCapability = RttMode::DISABLED;
   RttMode peerRttCapability  = RttMode::DISABLED;
   CallType callType  = CallType::UNKNOWN;
};


class CallManagerServerImpl final : public telStub::DialerService::Service,
                                    public IServerEventListener,
                                    public std::enable_shared_from_this<CallManagerServerImpl> {

public:
    CallManagerServerImpl();
    ~CallManagerServerImpl();
    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status GetServiceStatus(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status MakeECall(ServerContext* context, const telStub::MakeECallRequest*
        request, telStub::MakeECallReply* response) override;
    grpc::Status Hangup(ServerContext* context, const telStub::HangupRequest* request,
        telStub::HangupReply* response);
    grpc::Status UpdateECallMsd(ServerContext* context,
        const telStub::UpdateECallMsdRequest* request, telStub::UpdateECallMsdResponse* response);
    grpc::Status RequestECallHlapTimerStatus(ServerContext* context,
        const telStub::RequestECallHlapTimerStatusRequest* request,
        telStub::RequestECallHlapTimerStatusReply* response);
    grpc::Status CleanUpService(ServerContext* context,
        const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response);
    grpc::Status SetConfig(ServerContext* context,
        const telStub::SetConfigRequest* request,  telStub::SetConfigReply* response);
    grpc::Status GetConfig(ServerContext* context,
        const ::google::protobuf::Empty* request,  telStub::GetConfigResponse* response);
    grpc::Status UpdateEcallHlapTimer(ServerContext* context,
        const telStub::UpdateEcallHlapTimerRequest* request,
        telStub::UpdateEcallHlapTimerResponse* response);
    grpc::Status RequestEcallHlapTimer(ServerContext* context,
        const telStub::RequestEcallHlapTimerRequest* request,
        telStub::RequestEcallHlapTimerReply* response);
    grpc::Status ExitEcbm(ServerContext* context, const telStub::RequestEcbmRequest* request,
        telStub::RequestEcbmReply* response);
    grpc::Status RequestEcbm(ServerContext* context,
        const telStub::RequestEcbmRequest* request, telStub::RequestEcbmReply* response);
    grpc::Status MakeCall(ServerContext* context,
        const telStub::MakeCallRequest* request, telStub::MakeCallReply* response);
    grpc::Status Reject(ServerContext* context,
        const telStub::RejectRequest* request, telStub::RejectReply* response);
    grpc::Status RejectWithSMS(ServerContext* context,
        const telStub::RejectWithSMSRequest* request, telStub::RejectWithSMSReply* response);
    grpc::Status Answer(ServerContext* context,
        const telStub::AnswerRequest* request, telStub::AnswerReply* response);
    grpc::Status HangupForegroundResumeBackground(ServerContext* context,
        const telStub::HangupForegroundResumeBackgroundRequest* request,
        telStub::HangupForegroundResumeBackgroundReply* response);
    grpc::Status HangupWaitingOrBackground(ServerContext* context,
        const telStub::HangupWaitingOrBackgroundRequest* request,
        telStub::HangupWaitingOrBackgroundReply* response);
    grpc::Status Hold(ServerContext* context, const telStub::HoldRequest* request,
        telStub::HoldReply* response);
    grpc::Status Resume(ServerContext* context, const telStub::ResumeRequest* request,
        telStub::ResumeReply* response);
    grpc::Status Swap(ServerContext* context, const telStub::SwapRequest* request,
        telStub::SwapReply* response);
    grpc::Status RequestNetworkDeregistration(ServerContext* context,
        const telStub::RequestNetworkDeregistrationRequest* request,
        telStub::RequestNetworkDeregistrationReply* response);
    grpc::Status ModifyOrRespondToModifyCall(ServerContext* context,
        const telStub::ModifyOrRespondToModifyCallRequest* request,
        telStub::ModifyOrRespondToModifyCallReply* response);
    grpc::Status SendRtt(ServerContext* context,
        const telStub::SendRttRequest* request, telStub::SendRttReply* response);
    grpc::Status updateCalls(ServerContext* context,
        const telStub::UpdateCurrentCallsRequest* request, ::google::protobuf::Empty* response);
    grpc::Status ConfigureECallRedial(ServerContext* context,
        const telStub::ConfigureECallRedialRequest* request,
        telStub::ConfigureECallRedialResponse* response);
    void startTimer(std::string timer);
    void msdTransmissionStatus(std::string msdtransmision );
    void changeCallState(int phoneId, std::string callstate, int index);
    void expiryTimer(std::string timer);
    void sendEvent(std::string timer, std::string status );
    void onEventUpdate(::eventService::UnsolicitedEvent event) override;
    void onECallRedial(int phoneId, bool willECallRedial, telux::tel::ReasonType reason);

private:
    Json::Value rootObjSystemStateSlot1_;
    Json::Value rootObjSystemStateSlot2_;
    Json::Value rootObjApiResponseSlot1_;
    Json::Value rootObjApiResponseSlot2_;
    std::map <int, Json::Value> jsonObjSystemStateSlot_;
    std::map <int, std::string> jsonObjSystemStateFileName_;
    std::map <int, Json::Value> jsonObjApiResponseSlot_;
    std::map <int, std::string> jsonObjApiResponseFileName_;
    std::mutex callManagerMutex_;
    std::shared_ptr<EcallStateMachine> ecallStateMachine_;
    bool iseCallNumTypeOverridden_ = false;
    grpc::Status readJson();
    void getJsonForSystemData (int phoneId, std::string& jsonfilename, Json::Value& rootObj );
    void getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
        Json::Value& rootObj );
    void handleMsdUpdateRequest(std::string eventParams);
    void handleHangupRequest(std::string eventParams);
    void handleIncomingCallRequest(std::string eventParams);
    void handleModifyCallRequest(std::string eventParams);
    void handleRttMessageRequest(std::string eventParams);
    telux::common::Status handleStateMachine(int phoneId, int callIndex);
    void startTimers(std::string timer);
    void triggerTimerExpiry(std::string timer, int phoneId);
    void triggerECallInfoChangeEvent(std::string timer, telux::tel::HlapTimerEvent action);
    void triggerCallInfoChangeEvent(int phoneId, std::shared_ptr<CallInfo> call);
    void triggerCallInfoChange(int phoneId);
    void triggerMsdPullrequestEvent(int phoneId);
    void triggerCallStateChangeEvent(int phoneId, std::string action,
        std::string remotepartyNumber);
    void triggerCallListAfterCallEnd(int phoneId);
    std::vector<std::shared_ptr<CallInfo>> fetchSlotIdCalls(int phoneId);
    void triggerModifyCallRequestEvent(int phoneId, int callIndex);
    void triggerRttMessageEvent(int phoneId, std::string message);
    bool findAndRemoveMatchingCall(int callIndex);
    void updateEcallHlapTimer(std::string timer, HlapTimerStatus status);
    std::vector<std::string> parseUserInput();
    bool getUserConfiguredeCallRat();
    std::string getUserConfiguredECallRedialConfig();
    std::string getRemotePartyNumber(int phoneId);
    std::string fetchNextToken(std::string& inputString, std::string delimiter);
    std::vector<std::shared_ptr<CallInfo>> calls_;
    CallInfo callInfo_;
    std::shared_ptr<CallInfo> redialECallCache_ = nullptr;
    bool eCallRedialIsOngoing_ = false;
    bool callEndOperationCompleted_ = false;
    std::condition_variable cv;
    std::mutex mtx;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    bool match(std::shared_ptr<CallInfo> call, CallInfo callToCompare);
    bool match(std::shared_ptr<CallInfo> call, int slotId, int callIndex);
    void logCallDetails(std::shared_ptr<CallInfo> call);
    std::shared_ptr<CallInfo> findCallAndUpdateCallState(int index,
        CallState callState, int phoneId);
    std::shared_ptr<CallInfo> findCallAndUpdateRttMode(int index, RttMode mode, int phoneId);
    bool findMatchingCall(CallInfo callToCompare);
    std::shared_ptr<CallInfo> findMatchingCall(int slotId, int callIndex);
    bool find(std::shared_ptr<CallInfo> call, int index, int phoneId);
    void onEventUpdate(std::string event);
    void handleCallMachine();
    void changeCallStateofActiveCalls(CallInfo info);
    void changeRttModeOfCall(RttMode mode, int index, int phoneId);
    void resumeBackgroundCalls(int phoneId);
    void hangupWaitingOrBackgroundCalls(int phoneId);
    void hangupForegroundgroundCalls(int phoneId);
    void holdCall(int phoneId, int callIndex);
    void swapCalls(int callHoldIndex, int phoneIndex, int callActivateIndex);
    int getCallIndexOfActiveCall(int phoneId);
    // Find the lowest unfilled index in the call list.
    int setCallIndexForNewCall();
    bool getUserConfiguredALACKParameter();
    template <typename T>
    bool addNewCallDetails(const T* request) {
        CallInfo callInfo;
        callInfo.phoneId = request->phone_id();
        callInfo.index = setCallIndexForNewCall();
        callInfo.callDirection = CallDirection::OUTGOING;
        callInfo.callState = CallState::CALL_IDLE;
        callInfo.isMultiPartyCall = true;
        CallApi makeCallApiType = static_cast<CallApi>(request->api());
        if((makeCallApiType == CallApi::makeECallWithMsd) ||
            (makeCallApiType == CallApi::makeECallWithRawMsd) ||
            (makeCallApiType == CallApi::makeECallWithoutMsd)) {
            callInfo.isRegulatoryeCall = true;
        } else {
            callInfo.isRegulatoryeCall = false;
        }
        if(makeCallApiType == makeTpsECallOverIMS) {
            callInfo.isTpseCallOverIms = true;
            callInfo.callType = CallType::VOICE_IP_CALL;
        } else {
            callInfo.isTpseCallOverIms = false;
            callInfo.callType = CallType::VOICE_CALL;
        }
        if(request->remote_party_number() == "") {
            // No input will be passed from client for regulatory eCall
            callInfo.remotePartyNumber = getRemotePartyNumber(request->phone_id());
        } else {
            // Normal Voice call and custom number eCall
            callInfo.remotePartyNumber = request->remote_party_number();
            telStub::RadioTechnology rat;
            std::vector<std::string> eccNumberList = {"112", "911"};
            std::vector<telStub::RadioTechnology> psRatList =
                {telStub::RadioTechnology::RADIO_TECH_NR5G,
                telStub::RadioTechnology::RADIO_TECH_LTE};
            if(telux::common::ErrorCode::SUCCESS ==
                TelUtil::readVoiceRadioTechnologyFromJsonFile(callInfo.phoneId, rat)) {
                if(std::find(eccNumberList.begin(), eccNumberList.end(),
                    callInfo.remotePartyNumber) != eccNumberList.end()) {
                    if (std::find(psRatList.begin(), psRatList.end(), rat)
                        != psRatList.end()) {
                        callInfo.callType = CallType::EMERGENCY_IP_CALL;
                    } else {
                        callInfo.callType = CallType::EMERGENCY_CALL;
                    }
                } else {
                    if (std::find(psRatList.begin(), psRatList.end(), rat)
                        != psRatList.end()) {
                        callInfo.callType = CallType::VOICE_IP_CALL;
                    } else {
                        callInfo.callType = CallType::VOICE_CALL;
                    }
                }
            } else {
                if (std::find(eccNumberList.begin(), eccNumberList.end(),
                    callInfo.remotePartyNumber) != eccNumberList.end()) {
                    callInfo.callType = CallType::EMERGENCY_CALL;
                } else {
                    callInfo.callType = CallType::VOICE_CALL;
                }
            }
        }
        if (callInfo.isRegulatoryeCall){
            callInfo.callType = CallType::EMERGENCY_CALL;
        }
        if(makeCallApiType == CallApi::makeRttVoiceCall) {
            callInfo.mode = RttMode::FULL;
            // Local and peer capability is assumed to be available during a RTT call.
            // TODO: During intermanager implementation, we can consider local capability of the
            // simulation framework dependent on IMS Settings.
            callInfo.localRttCapability = RttMode::FULL;
            callInfo.peerRttCapability = RttMode::FULL;
        }
        callInfo.isMsdTransmitted = request->is_msd_transmitted();
        callInfo_ = callInfo;
        auto call = std::make_shared<CallInfo>(callInfo);
        logCallDetails(call);
        if((makeCallApiType != CallApi::makeVoiceCall )
            && (makeCallApiType != CallApi::makeRttVoiceCall)) {
            // Regulatory eCall and custom number eCall over CS and PS allowed only one at a time.
            // It is a limitation in simulation state handling.
            if(!findMatchingCall(callInfo)) {
                calls_.emplace_back(call);
                return true;
            }
        } else {
            // Voice calls of same remote party number are allowed
            calls_.emplace_back(call);
            return true;
        }
        return false;
    }
};

#endif // CALL_MANAGER_SERVER_HPP
