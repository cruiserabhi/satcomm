/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       CallStub.hpp
 *
 * @brief      Implementation of Call
 *
 */

#ifndef CALL_STUB_HPP
#define CALL_STUB_HPP

#include <telux/tel/CallManager.hpp>
#include <telux/tel/CallListener.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/common/CommonDefines.hpp>
#include "../common/AsyncTaskQueue.hpp"
#include <grpcpp/grpcpp.h>
#include "../../protos/proto-src/tel_simulation.grpc.pb.h"

#define INVALID -1

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using telStub::DialerService;

namespace telux {
namespace tel {

struct CallInfo {
   int index = INVALID;                                // Connection Index
   CallDirection callDirection = CallDirection::NONE;  // enumeration for MO / MT call
   std::string remotePartyNumber = "";                 // Remote party number
   bool transmitMsd = false;
   CallState callState = CallState::CALL_IDLE;
   CallEndCause callEndCause = CallEndCause::NORMAL;
   int sipErrorCode = 0;
   bool isMultiPartyCall = false;
   bool isMpty = false;
   RttMode mode = RttMode::DISABLED;                // RTT mode of the call
   RttMode localRttCapability = RttMode::DISABLED;  // RTT capability of local device
   RttMode peerRttCapability  = RttMode::DISABLED;  // RTT capability of peer device
   CallType callType          = CallType::UNKNOWN;
};

class CallStub : public ICall {
public:

    CallStub(int phoneId, CallInfo callInfo);

    telux::common::Status answer(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr,
        RttMode mode = RttMode::DISABLED);

    telux::common::Status hold(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);

    telux::common::Status resume(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);

    telux::common::Status reject(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);

    telux::common::Status reject(const std::string &rejectSMS,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status hangup(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status playDtmfTone(
        char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status startDtmfTone(
        char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status stopDtmfTone(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    RttMode getRttMode();
    RttMode getLocalRttCapability();
    RttMode getPeerRttCapability();
    CallType getCallType();
    telux::common::Status modify(RttMode mode,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status respondToModifyRequest(bool modifyResponseType,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    CallState getCallState();
    int getCallIndex();
    CallEndCause getCallEndCause();
    int getSipErrorCode();
    CallDirection getCallDirection();
    std::string getRemotePartyNumber();
    int getPhoneId();
    bool isMultiPartyCall();
    void updateCallState(CallState callState);
    void updateCallDirection(CallDirection callDirection);
    void setCallIndex(int index);
    void setCallState(CallState callState);
    bool match(std::shared_ptr<CallStub> &ci);
    bool isInfoStale(const std::shared_ptr<CallStub> &ci);
    telux::common::Status updateCallInfo(std::shared_ptr<CallStub> &callInfo);
    void logCallDetails();
private:
    std::unique_ptr<::telStub::DialerService::Stub> stub_;
    int phoneId_;
    CallInfo callInfo_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    void invokeCommandCallback(std::shared_ptr<ICommandResponseCallback> callback,
        ErrorCode error, int cbDelay);
    telux::common::Status modifyOrRespondToModifyCall(RttMode mode, std::string api,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback);
};

} // end of namespace tel

} // end of namespace telux

#endif // CALL_STUB_HPP
