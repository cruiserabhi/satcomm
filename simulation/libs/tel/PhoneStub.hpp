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


/**
 * @file       PhoneStub.hpp
 *
 * @brief      Implementation of Phone
 *
 */

#ifndef PHONE_STUB_HPP
#define PHONE_STUB_HPP

#include "common/AsyncTaskQueue.hpp"
#include "common/event-manager/EventManager.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Phone.hpp>

#define INVALID -1
using telStub::PhoneService;

namespace telux {
namespace tel {

class PhoneStub : public IPhone,
                  public IVoiceServiceStateCallback,
                  public std::enable_shared_from_this<PhoneStub> {
public:
    PhoneStub(int phoneId);
    ~PhoneStub();
    void init();
    telux::common::Status getPhoneId(int &phId);
    RadioState getRadioState();
    void setRadioState(RadioState radioState);
    ServiceState getServiceState();
    void setServiceState(ServiceState serviceState);
    void updateRadioState(RadioState radioState);
    telux::common::Status requestVoiceRadioTechnology(VoiceRadioTechResponseCb callback) override;
    telux::common::Status
        requestVoiceServiceState(std::weak_ptr<IVoiceServiceStateCallback> callback);
    telux::common::Status setRadioPower(
        bool enable, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status requestCellInfo(telux::tel::CellInfoCallback callback);
    telux::common::Status setCellInfoListRate(uint32_t timeInterval,
        telux::common::ResponseCallback callback);
   virtual telux::common::Status requestECallOperatingMode(ECallGetOperatingModeCallback callback);
   bool isReady();
   bool isSubsystemReady();
   std::future<bool> onReady();
   telux::common::Status requestOperatorName(OperatorNameCallback callback);
   telux::common::Status configureSignalStrength(
      std::vector<SignalStrengthConfig> signalStrengthConfig, telux::common::ResponseCallback
      callback);
    telux::common::Status requestSignalStrength(
        std::shared_ptr<telux::tel::ISignalStrengthCallback> callback = nullptr);
    virtual telux::common::Status setECallOperatingMode(ECallMode eCallMode,
        telux::common::ResponseCallback callback);
    telux::common::Status requestOperatorInfo(OperatorInfoCallback callback);
    telux::common::Status configureSignalStrength(
        std::vector<SignalStrengthConfigEx> signalStrengthConfigEx, uint16_t hysteresisMs = 0,
        telux::common::ResponseCallback callback = nullptr);

private:
    int phoneId_ = INVALID;
    std::atomic<bool> ready_;
    std::atomic<RadioState> radioState_;
    std::atomic<bool> radioStateInitialized_;
    std::atomic<ServiceState> serviceState_;
    std::atomic<bool> serviceStateInitialized_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::unique_ptr<::telStub::PhoneService::Stub> stub_;
    std::mutex phoneMutex_;
    void updateReady();
    void handleDeprecatedVoiceServiceStateResponse(
        const std::shared_ptr<VoiceServiceInfo> &serviceInfo);
    void initSync(telux::common::InitResponseCb callback);
};

} // end of namespace tel
} // end of namespace telux

#endif // PHONE_STUB_HPP
