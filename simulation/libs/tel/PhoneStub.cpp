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

#include "PhoneStub.hpp"
#include <telux/common/DeviceConfig.hpp>

#define DELAY 100
static constexpr int INVALID_ECALL_OPRT_MODE = -1;

using namespace telux::tel;

PhoneStub::PhoneStub(int phoneId)
    : stub_(PhoneService::NewStub(grpc::CreateChannel("localhost:8089",
    grpc::InsecureChannelCredentials())))  {
    LOG(DEBUG, __FUNCTION__);
    phoneId_ = phoneId;
    ready_ = false;
    radioState_ = RadioState::RADIO_STATE_UNAVAILABLE;
    radioStateInitialized_ = false;
    serviceState_ = ServiceState::OUT_OF_SERVICE;
    serviceStateInitialized_ = false;
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
}

PhoneStub::~PhoneStub() {
    LOG(DEBUG, __FUNCTION__);
}

void PhoneStub::init() {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async, [this](){ this->updateReady();}).share();
    taskQ_->add(f);
}

telux::common::Status PhoneStub::getPhoneId(int &phId) {
    LOG(DEBUG, __FUNCTION__);
    phId = phoneId_;
    const ::google::protobuf::Empty request;
    telStub::GetPhoneIdReply response;
    ClientContext context;
    grpc::Status reqstatus = stub_->GetPhoneId(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " failed");
        return telux::common::Status::FAILED;
    }
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}

RadioState PhoneStub::getRadioState() {
    RadioState radioState = radioState_;
    LOG(DEBUG, __FUNCTION__, " Radio state: ", static_cast<int>(radioState));
    return radioState_;
}

void PhoneStub::setRadioState(RadioState radioState) {
    LOG(DEBUG, __FUNCTION__, " Radio state: ", static_cast<int>(radioState));
    radioState_ = radioState;
    radioStateInitialized_ = true;
    updateReady();
}

ServiceState PhoneStub::getServiceState() {
    ServiceState srvState = serviceState_;
    LOG(DEBUG, __FUNCTION__, " Service state: ", static_cast<int>(srvState));
    return serviceState_;
}

void PhoneStub::setServiceState(ServiceState serviceState) {
    LOG(DEBUG, __FUNCTION__, " Service state: ", static_cast<int>(serviceState));
    serviceState_ = serviceState;
    serviceStateInitialized_ = true;
    updateReady();
    LOG(DEBUG, __FUNCTION__, " ServiceState Initialized");
}

void PhoneStub::updateRadioState(RadioState radioState) {
    // Update Radio State in the phone if there is a change
    if ((getRadioState() != radioState) || (!radioStateInitialized_)) {
        setRadioState(radioState);
    }
}

void PhoneStub::updateReady() {
    // If the phone is not yet initialized, fetch voice registration state
    // and initialize service state of the phone.
    // This request is being made explicitly to server since unsolicited
    // notification for voice registration state won't be sent when there
    // is no change in the voice registration state.
    if (!ready_ && !serviceStateInitialized_) {
        requestVoiceServiceState(shared_from_this());
    }
    bool radioStateInitialized = radioStateInitialized_;
    bool serviceStateInitialized = serviceStateInitialized_;
    LOG(DEBUG, __FUNCTION__, " Status: ",  radioStateInitialized, serviceStateInitialized);
    if (!ready_ && radioStateInitialized_ && serviceStateInitialized_) {
        std::lock_guard<std::mutex> lock(phoneMutex_);
        ready_ = true;
        LOG(INFO, __FUNCTION__, " Phone is ready on phoneId ", phoneId_);
    } else if (!ready_) {
        LOG(DEBUG, __FUNCTION__, " Phone not ready yet");
    }
}

void PhoneStub::handleDeprecatedVoiceServiceStateResponse(
    const std::shared_ptr<VoiceServiceInfo> &serviceInfo) {
    LOG(DEBUG, __FUNCTION__);
    ServiceState srvState;
    srvState = getServiceState();
    if (serviceInfo) {
        // check for emergency based on registration state
        if (serviceInfo->isEmergency()) {
            LOG(DEBUG, "ServiceState : EMERGENCY_ONLY mode");
            srvState = ServiceState::EMERGENCY_ONLY;
        } else if (serviceInfo->isOutOfService()) {
            LOG(DEBUG, "ServiceState : OUT_OF_SERVICE mode");
            srvState = ServiceState::OUT_OF_SERVICE;
        } else {
            LOG(DEBUG, "ServiceState : IN_SERVICE mode");
            srvState = ServiceState::IN_SERVICE;
        }
    }

    if ((getServiceState() != srvState) || (!serviceStateInitialized_)) {
        setServiceState(srvState);
    }
}

telux::common::Status PhoneStub::requestVoiceRadioTechnology(VoiceRadioTechResponseCb callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
    PhoneStub::requestVoiceServiceState(std::weak_ptr<IVoiceServiceStateCallback> callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    ::telStub::RequestVoiceServiceStateRequest request;
    ::telStub::RequestVoiceServiceStateReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);
    grpc::Status reqstatus = stub_->RequestVoiceServiceState(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " failed on phoneId ", phoneId_);
        return telux::common::Status::FAILED;
    }
    std::shared_ptr<VoiceServiceInfo> vocSrvInfo = nullptr;
    VoiceServiceState voiceServiceState =
        static_cast<VoiceServiceState>(response.voice_service_state_info().voice_service_state());
    VoiceServiceDenialCause denialCause =
        static_cast<VoiceServiceDenialCause>(
            response.voice_service_state_info().voice_service_denial_cause());
    RadioTechnology radioTech = static_cast<RadioTechnology> (
        response.voice_service_state_info().radio_technology());
    vocSrvInfo = std::make_shared<VoiceServiceInfo>(voiceServiceState, denialCause, radioTech);
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback, vocSrvInfo]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (vocSrvInfo) {
                    handleDeprecatedVoiceServiceStateResponse(vocSrvInfo);
                }
                std::shared_ptr<IVoiceServiceStateCallback> cb = callback.lock();
                if (cb) {
                    cb->voiceServiceStateResponse(vocSrvInfo, error);
                } else {
                    LOG(DEBUG, __FUNCTION__, " Callback is null");
                }
        }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneStub::setRadioPower(
    bool enable, std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    ::telStub::SetRadioPowerRequest request;
    ::telStub::SetRadioPowerReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);
    request.set_enable(enable);
    grpc::Status reqstatus = stub_->SetRadioPower(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " failed on phoneId ", phoneId_);
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback->commandResponse(error);
                } else {
                    LOG(ERROR, __FUNCTION__, " Callback is null");
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneStub::requestCellInfo(telux::tel::CellInfoCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    ::telStub::RequestCellInfoListRequest request;
    ::telStub::RequestCellInfoListReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);
    grpc::Status reqstatus = stub_->RequestCellInfoList(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " failed on phoneId ", phoneId_);
        return telux::common::Status::FAILED;
    }
    // update cellInfoList
    std::vector<std::shared_ptr<CellInfo>> cellInfoList = {};
    for (int i = 0; i < response.cell_info_list_size(); i++) {
        CellType cellType = static_cast<CellType>
            (response.mutable_cell_info_list(i)->mutable_cell_type()->cell_type());
        int registered = response.mutable_cell_info_list(i)-> mutable_cell_type()->registered();
        LOG(DEBUG, __FUNCTION__, " any cell registered : ", registered);
        switch(cellType) {
            case CellType::GSM: {
                string gsmMcc = response.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->mcc();
                string gsmMnc = response.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->mnc();
                int gsmLac = response.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->lac();
                int gsmCid = response.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->cid();
                int gsmArfcn = response.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->arfcn();
                int gsmBsic = response.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->bsic();
                int gsmSignalStrength = response.mutable_cell_info_list(i)->
                    mutable_gsm_cell_info()->mutable_gsm_signal_strength_info()->
                    gsm_signal_strength();
                int gsmBitErrorRate = response.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_signal_strength_info()->gsm_bit_error_rate();
                GsmSignalStrengthInfo gsmCellSS(gsmSignalStrength, gsmBitErrorRate,
                                               INVALID_SIGNAL_STRENGTH_VALUE);
                GsmCellIdentity gsmCI(gsmMcc, gsmMnc, gsmLac, gsmCid, gsmArfcn, gsmBsic);
                auto gsmCellInfo = std::make_shared<GsmCellInfo>(registered, gsmCI, gsmCellSS);
                cellInfoList.emplace_back(gsmCellInfo);
                break;
            }
            case CellType::WCDMA: {
                string wcdmaMcc = response.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->mcc();
                string wcdmaMnc = response.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->mnc();
                int wcdmaLac = response.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->lac();
                int wcdmaCid = response.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->cid();
                int wcdmaArfcn = response.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->uarfcn();
                int wcdmaPsc = response.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->psc();
                int wcdmaSignalStrength = response.mutable_cell_info_list(i)->
                    mutable_wcdma_cell_info()-> mutable_wcdma_signal_strength_info()->
                    signal_strength();
                int wcdmaBitErrorRate = response.mutable_cell_info_list(i)->
                    mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                    bit_error_rate();
                int wcdmaEcio = response.mutable_cell_info_list(i)->
                    mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                    ecio();
                int wcdmaRscp = response.mutable_cell_info_list(i)->
                    mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                    rscp();
                WcdmaSignalStrengthInfo wcdmaCellSS(wcdmaSignalStrength, wcdmaBitErrorRate,
                    wcdmaEcio, wcdmaRscp);
                WcdmaCellIdentity wcdmaCI(wcdmaMcc, wcdmaMnc, wcdmaLac, wcdmaCid, wcdmaPsc,
                    wcdmaArfcn);
                auto wcdmaCellInfo = std::make_shared<telux::tel::WcdmaCellInfo>
                    (registered, wcdmaCI, wcdmaCellSS);
                cellInfoList.emplace_back(wcdmaCellInfo);
                break;
            }
            case CellType::LTE: {
                string lteMcc = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->mcc();
                string lteMnc = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->mnc();
                int lteTac = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->tac();
                int lteCi = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->ci();
                int lteEarfcn = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->earfcn();
                int ltePci = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->pci();
                int lteSignalStrength = response.mutable_cell_info_list(i)->
                    mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                    lte_signal_strength();
                int lteRsrp = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->lte_rsrp();
                int lteRsrq = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->lte_rsrq();
                int lteRssnr = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->lte_rssnr();
                int lteCqi = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->lte_cqi();
                int lteTimingAdvance = response.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->timing_advance();
                LteSignalStrengthInfo lteCellSS(lteSignalStrength, lteRsrp, lteRsrq, lteRssnr,
                     lteCqi, lteTimingAdvance);
                LteCellIdentity lteCI(lteMcc, lteMnc, lteCi, ltePci, lteTac, lteEarfcn);
                auto lteCellInfo = std::make_shared<LteCellInfo>(registered, lteCI, lteCellSS);
                cellInfoList.emplace_back(lteCellInfo);
                break;
            }
            case CellType::NR5G: {
                string nr5gMcc = response.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->mcc();
                string nr5gMnc = response.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->mnc();
                int nr5gTac = response.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->tac();
                int nr5gCi = response.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->ci();
                int nr5gArfcn = response.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->arfcn();
                int nr5gPci = response.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->pci();
                int nr5gRsrp = response.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_signal_strength_info()->rsrp();
                int nr5gRsrq = response.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_signal_strength_info()->rsrq();
                int nr5gRssnr = response.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_signal_strength_info()->rssnr();
                Nr5gSignalStrengthInfo nr5gCellSS(nr5gRsrp, nr5gRsrq, nr5gRssnr);
                Nr5gCellIdentity nr5gCI(nr5gMcc, nr5gMnc, nr5gCi, nr5gPci, nr5gTac, nr5gArfcn);
                auto nr5gCellInfo = std::make_shared<Nr5gCellInfo>(registered, nr5gCI,
                    nr5gCellSS);
                cellInfoList.emplace_back(nr5gCellInfo);
                break;
            }
            case CellType::NB1_NTN: {
                string nb1NtnMcc = response.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->mcc();
                string nb1NtnMnc = response.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->mnc();
                int nb1NtnTac = response.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->tac();
                int nb1NtnCi = response.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->ci();
                int nb1NtnEarfcn = response.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->earfcn();
                int nb1NtnSignalStrength = response.mutable_cell_info_list(i)->
                    mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_signal_strength_info()->
                        signal_strength();
                int nb1NtnRsrp = response.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_signal_strength_info()->rsrp();
                int nb1NtnRsrq = response.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_signal_strength_info()->rsrq();
                int nb1NtnRssnr = response.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_signal_strength_info()->rssnr();
                Nb1NtnSignalStrengthInfo nb1NtnCellSS(nb1NtnSignalStrength, nb1NtnRsrp, nb1NtnRsrq,
                    nb1NtnRssnr);
                Nb1NtnCellIdentity nb1NtnCI(nb1NtnMcc, nb1NtnMnc, nb1NtnCi, nb1NtnTac,
                    nb1NtnEarfcn);
                auto nb1NtnCellInfo = std::make_shared<Nb1NtnCellInfo>(registered, nb1NtnCI,
                    nb1NtnCellSS);
                cellInfoList.emplace_back(nb1NtnCellInfo);
                break;
            }
            case CellType::CDMA:
            case CellType::TDSCDMA:
            default:
                LOG(ERROR, __FUNCTION__, " invalid or deprecated cell type");
                break;
        }
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback, cellInfoList]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(cellInfoList, error);
                } else {
                    LOG(ERROR, __FUNCTION__, " Callback is null");
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneStub::setCellInfoListRate(uint32_t timeInterval,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::SetCellInfoListRateRequest request;
    ::telStub::SetCellInfoListRateReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_cell_info_rate(timeInterval);
    grpc::Status reqstatus = stub_->SetCellInfoListRate(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " request failed on phoneId ", phoneId_);
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(error);
                }  else {
                    LOG(ERROR, __FUNCTION__, " Callback is null");
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneStub::requestSignalStrength(
    std::shared_ptr<telux::tel::ISignalStrengthCallback> callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    ::telStub::GetSignalStrengthRequest request;
    ::telStub::GetSignalStrengthReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);
    grpc::Status reqstatus = stub_->GetSignalStrength(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " failed on phoneId ", phoneId_);
        return telux::common::Status::FAILED;
    }
    std::shared_ptr<SignalStrength> signalStrengthNotify = nullptr;
    std::shared_ptr<GsmSignalStrengthInfo> gsmSignalStrength =
         std::make_shared<GsmSignalStrengthInfo>(
             response.mutable_signal_strength()->mutable_gsm_signal_strength_info()->
                gsm_signal_strength(),
             response.mutable_signal_strength()->mutable_gsm_signal_strength_info()->
                gsm_bit_error_rate(),
             INVALID_SIGNAL_STRENGTH_VALUE);
    std::shared_ptr<LteSignalStrengthInfo> lteSignalStrength
        = std::make_shared<LteSignalStrengthInfo>(
            response.mutable_signal_strength()->mutable_lte_signal_strength_info()->
                lte_signal_strength(),
            response.mutable_signal_strength()->mutable_lte_signal_strength_info()->lte_rsrp(),
            response.mutable_signal_strength()->mutable_lte_signal_strength_info()->lte_rsrq(),
            response.mutable_signal_strength()->mutable_lte_signal_strength_info()->lte_rssnr(),
            response.mutable_signal_strength()->mutable_lte_signal_strength_info()->lte_cqi(),
            response.mutable_signal_strength()->mutable_lte_signal_strength_info()->
                timing_advance());
    std::shared_ptr<WcdmaSignalStrengthInfo> wcdmaSignalStrength
        = std::make_shared<WcdmaSignalStrengthInfo>(
            response.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                signal_strength(),
            response.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                bit_error_rate(),
            response.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->ecio(),
            response.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->rscp());
    std::shared_ptr<Nr5gSignalStrengthInfo> nr5gSignalStrength
        = std::make_shared<Nr5gSignalStrengthInfo>(
            response.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->rsrp(),
            response.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->rsrq(),
            response.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->rssnr());
    std::shared_ptr<Nb1NtnSignalStrengthInfo> nb1NtnSignalStrength
        = std::make_shared<Nb1NtnSignalStrengthInfo>(
            response.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
                signal_strength(),
            response.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->rsrp(),
            response.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->rsrq(),
            response.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->rssnr());
    signalStrengthNotify
        = std::make_shared<SignalStrength>(lteSignalStrength, gsmSignalStrength,
            nullptr/*cdma deprecated*/, wcdmaSignalStrength, nullptr/*tdscdma deprecated*/,
            nr5gSignalStrength, nb1NtnSignalStrength);
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback, signalStrengthNotify]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback->signalStrengthResponse(signalStrengthNotify, error);
                } else {
                    LOG(ERROR, __FUNCTION__, " Callback is null");
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneStub::setECallOperatingMode(ECallMode eCallMode,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    ::telStub::SetECallOperatingModeRequest request;
    ::telStub::SetECallOperatingModeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_ecall_mode(static_cast<telStub::ECallMode>(eCallMode));
    grpc::Status reqstatus = stub_->SetECallOperatingMode(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " failed on phoneId ", phoneId_);
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    LOG(DEBUG, __FUNCTION__, " Status: ", static_cast<int>(status), " Errorcode: ",
        static_cast<int>(error));
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(error);
                } else {
                    LOG(DEBUG, __FUNCTION__, " Callabck is null");
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneStub::requestECallOperatingMode(ECallGetOperatingModeCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    ::telStub::GetECallOperatingModeRequest request;
    ::telStub::GetECallOperatingModeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    ECallMode eCallMode = static_cast<ECallMode>(INVALID_ECALL_OPRT_MODE);
    grpc::Status reqstatus = stub_->GetECallOperatingMode(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " failed on phoneId ", phoneId_);
        return telux::common::Status::FAILED;
    }
    eCallMode = static_cast<ECallMode>(response.ecall_mode());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback, eCallMode]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(eCallMode, error);
                } else {
                    LOG(ERROR, __FUNCTION__, " Callback is null");
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

bool PhoneStub::isReady() {
    return ready_;
}

std::future<bool> PhoneStub::onReady() {
    LOG(DEBUG, __FUNCTION__);
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async,
        [this]() {
            while (!isReady()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));
            }
        return(isReady());
    });
    return((ready_future));
}

telux::common::Status PhoneStub::requestOperatorName(OperatorNameCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneStub::requestOperatorInfo(OperatorInfoCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    ::telStub::RequestOperatorInfoRequest request;
    ::telStub::RequestOperatorInfoReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    grpc::Status reqstatus = stub_->RequestOperatorInfo(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " failed on phoneId ", phoneId_);
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    PlmnInfo plmnInfo;
    telux::common::BoolValue isHome = BoolValue::STATE_FALSE;

    // TODO set operator name only when device is service mode
    if (getServiceState() == ServiceState::IN_SERVICE) {
        plmnInfo.longName = response.mutable_plmn_info()->long_name();
        plmnInfo.shortName = response.mutable_plmn_info()->short_name();
        plmnInfo.plmn = response.mutable_plmn_info()->plmn();

        if (response.mutable_plmn_info()->ishome()) {
            isHome = BoolValue::STATE_TRUE;
            plmnInfo.isHome = isHome;
        }
    }
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay,  error, callback, plmnInfo]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(plmnInfo, error);
                } else {
                    LOG(ERROR, __FUNCTION__, " Callback is null");
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneStub::configureSignalStrength(
    std::vector<SignalStrengthConfig> signalStrengthConfig, telux::common::ResponseCallback
    callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    ::telStub::ConfigureSignalStrengthRequest request;
    ::telStub::ConfigureSignalStrengthReply response;
    ClientContext context;

    if (signalStrengthConfig.size() == 0) {
        LOG(DEBUG, __FUNCTION__, " Invalid signal strength configuration");
        return telux::common::Status::INVALIDPARAM;
    }

    request.set_phone_id(phoneId_);
    for (size_t i = 0; i < signalStrengthConfig.size(); i++) {
        telStub::ConfigureSignalStrength *sigConfig = request.add_config();
        sigConfig->set_config_type(static_cast<telStub::SignalStrengthConfigType>
            (signalStrengthConfig[i].configType));
        sigConfig->set_rat_sig_type(static_cast<telStub::RadioSignalStrengthType>
            (signalStrengthConfig[i].ratSigType));
        switch(signalStrengthConfig[i].configType) {
            case SignalStrengthConfigType::DELTA:
                sigConfig->set_delta(signalStrengthConfig[i].delta);
                break;
            case SignalStrengthConfigType::THRESHOLD:
                sigConfig->mutable_threshold()->set_lower_range_threshold(
                    signalStrengthConfig[i].threshold.lowerRangeThreshold);
                sigConfig->mutable_threshold()->set_upper_range_threshold(
                    signalStrengthConfig[i].threshold.upperRangeThreshold);
                break;
            default:
                LOG(ERROR, " Invalid SignalStrength config type");
                break;
        }
    }
    grpc::Status reqstatus = stub_->ConfigureSignalStrength(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " failed on phoneId ", phoneId_);
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(error);
                } else {
                    LOG(ERROR, __FUNCTION__, " Callback is null");
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneStub::configureSignalStrength(
    std::vector<SignalStrengthConfigEx> signalStrengthConfigEx, uint16_t hysteresisMs,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId ", phoneId_);
    ::telStub::ConfigureSignalStrengthExRequest request;
    ::telStub::ConfigureSignalStrengthExReply response;
    ClientContext context;

    if (signalStrengthConfigEx.size() == 0) {
        LOG(DEBUG, __FUNCTION__, " Invalid signal strength configuration");
        return telux::common::Status::INVALIDPARAM;
    }

    request.set_phone_id(phoneId_);
    for (auto elem : signalStrengthConfigEx) {
        telStub::ConfigureSignalStrengthEx *sigConfigEx = request.add_config();
        sigConfigEx->set_radio_tech(static_cast<telStub::RadioTechnology>
            (elem.radioTech));
        int configSize = elem.configTypeMask.size();
        for (int idx = 0; idx < configSize; idx++) {
            if (elem.configTypeMask.test(idx)) {
                sigConfigEx->add_config_types
                    (static_cast<telStub::SignalStrengthConfigExType>(idx));
            }
        }
        for (int idx = 0; idx < static_cast<int>(elem.sigConfigData.size()); idx++) {
            telStub::SignalStrengthConfigData *sigConfigData = sigConfigEx->add_sig_config_data();
            sigConfigData->set_sig_meas_type(
                static_cast<telStub::SignalStrengthMeasurementType>
                    (elem.sigConfigData[idx].sigMeasType));
            if (elem.configTypeMask
                [static_cast<int>(SignalStrengthConfigExType::DELTA)]) {
               sigConfigData->set_delta
                   (elem.sigConfigData[idx].delta);
            }
            if (elem.configTypeMask
                [static_cast<int>(SignalStrengthConfigExType::THRESHOLD)]) {
                for (int arrIdx = 0;
                    arrIdx < static_cast<int>(elem.sigConfigData[idx].thresholdList.size());
                    arrIdx++) {
                    if (elem.sigConfigData[idx].thresholdList[arrIdx]){
                        sigConfigData->mutable_elements()
                            ->add_threshold_list(elem.sigConfigData[idx].thresholdList[arrIdx]);
                    }
                }
            }
            if (elem.configTypeMask
                [static_cast<int>(SignalStrengthConfigExType::HYSTERESIS_DB)]) {
                sigConfigData->mutable_elements()->set_hysteresis_db
                    (elem.sigConfigData[idx].hysteresisDb);
            }
        }
    }
    request.set_hysteresis_ms(hysteresisMs);

    grpc::Status reqstatus = stub_->ConfigureSignalStrengthEx(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " failed on phoneId ", phoneId_);
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(error);
                } else {
                    LOG(ERROR, __FUNCTION__, " Callback is null");
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}
