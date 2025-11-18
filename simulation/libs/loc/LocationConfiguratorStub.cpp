/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "LocationConfiguratorStub.hpp"
#include "common/Logger.hpp"
#include "common/JsonParser.hpp"
#include "common/CommonUtils.hpp"
#include "LocationDefinesStub.hpp"
#include "common/event-manager/ClientEventManager.hpp"

//Default cb delay.
#define DEFAULT_CALLBACK_DELAY 100
#define SKIP_CALLBACK -1

#define RPC_FAIL_SUFFIX " RPC Request failed - "

namespace telux {
namespace loc {

LocationConfiguratorStub::LocationConfiguratorStub() {
    LOG(DEBUG, __FUNCTION__);
    managerStatus_ = ServiceStatus::SERVICE_UNAVAILABLE;
    stub_ = CommonUtils::getGrpcStub<LocationConfiguratorService>();
}

std::future<bool> LocationConfiguratorStub::onSubsystemReady() {
  LOG(DEBUG, __FUNCTION__);
  auto f = std::async(std::launch::async, [&] {
    return waitForInitialization();
  });
  return f;
}

bool LocationConfiguratorStub::waitForInitialization() {
  LOG(DEBUG, __FUNCTION__);
  std::unique_lock<std::mutex> cvLock(mutex_);
  cv_.wait(cvLock);
  return isSubsystemReady();
}

bool LocationConfiguratorStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::ServiceStatus LocationConfiguratorStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return managerStatus_;
}

telux::common::Status LocationConfiguratorStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async,
    [this, callback]() {
        this->initSync(callback);
        }).share();
        taskQ_.add(f);
    return telux::common::Status::SUCCESS;
}

void LocationConfiguratorStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::GetServiceStatusReply response;
    ClientContext context;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->InitService(&context, request, &response);
    if(reqstatus.ok()) {
        std::lock_guard<std::mutex> lock(mutex_);
        managerStatus_ = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        std::lock_guard<std::mutex> lock(mutex_);
        managerStatus_ = telux::common::ServiceStatus::SERVICE_FAILED;
    }
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(managerStatus_));
    if(managerStatus_ == ServiceStatus::SERVICE_AVAILABLE ){
        auto myself = shared_from_this();
        myself_ = myself;
        std::vector<std::string> filters = {LOC_CONFIG};
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(myself_, filters);
    }
    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        callback(managerStatus_);
    }
    cv_.notify_all();
}

void LocationConfiguratorStub::onEventUpdate(google::protobuf::Any event){
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::locStub::XtraStatusEvent>()) {
        ::locStub::XtraStatusEvent xtraEvent;
        event.UnpackTo(&xtraEvent);
        handleXtraUpdateEvent(xtraEvent);
     } else if (event.Is<::locStub::GnssUpdateEvent>()) {
        ::locStub::GnssUpdateEvent GnssEvent;
        event.UnpackTo(&GnssEvent);
        handleGnssConstellationUpdateEvent(GnssEvent);
     }
}

void LocationConfiguratorStub::handleXtraUpdateEvent(::locStub::XtraStatusEvent xtraEvent){
    LOG(DEBUG, __FUNCTION__);
    uint32_t enable= xtraEvent.enable();
    uint32_t validity= xtraEvent.validity();
    uint32_t dataStatus=xtraEvent.datastatus();
    uint32_t consent = xtraEvent.consent();
    invokeXtraStatusUpdate(enable,dataStatus,validity,consent);
}

void LocationConfiguratorStub::handleGnssConstellationUpdateEvent(::locStub::GnssUpdateEvent
    GnssEvent) {
    uint32_t enabledMask = GnssEvent.enabledmask();
    invokeGnssConstellationUpdate(enabledMask);
}

telux::common::Status LocationConfiguratorStub::configureCTunc(bool enable,
        telux::common::ResponseCallback callback, float timeUncertainty, uint32_t energyBudget) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureCTUNCRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_enable(enable);
    request.set_time_uncertainty(timeUncertainty);
    request.set_energy_budget(energyBudget);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureCTUNC(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configurePACE(bool enable,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigurePACERequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_enable(enable);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigurePACE(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::deleteAllAidingData(telux::common::ResponseCallback
        callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->DeleteAllAidingData(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureLeverArm(const LeverArmConfigInfo& info,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureLeverArmRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    auto& configMap = *request.mutable_lever_arm_config_info();
    for(auto itr: info) {
        ::locStub::LeverArmParams params;
        params.set_forward_offset(itr.second.forwardOffset);
        params.set_sideways_offset(itr.second.sidewaysOffset);
        params.set_up_offset(itr.second.upOffset);
        if(itr.first == LeverArmType::LEVER_ARM_TYPE_GNSS_TO_VRP) {
            configMap.insert({1, params});
        }
        if(itr.first == LeverArmType::LEVER_ARM_TYPE_DR_IMU_TO_GNSS) {
            configMap.insert({2, params});
        }
        if( (itr.first == LeverArmType::LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS) ||
            (itr.first == LeverArmType::LEVER_ARM_TYPE_VPE_IMU_TO_GNSS) ) {
            configMap.insert({3, params});
        }
    }

    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureLeverArm(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureConstellations(const SvBlackList& list,
        telux::common::ResponseCallback callback,  bool resetToDefault) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureConstellationsRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_reset_to_default(resetToDefault);
    for(auto itr: list) {
        ::locStub::SvBlackListInfo *blacklist = request.add_sv_black_list_info();
        blacklist->set_sv_id(itr.svId);
        int constel = static_cast<int>(itr.constellation);
        blacklist->set_constellation(static_cast<::locStub::GnssConstellationType>(constel));
    }

    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureConstellations(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureRobustLocation(bool enable,
        bool enableForE911, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureRobustLocationRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_enable(enable);
    request.set_enable_for_e911(enableForE911);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureRobustLocation(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestRobustLocation(
        GetRobustLocationCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::RequestRobustLocationReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    RobustLocationConfiguration rLConfig = {};
    ::grpc::Status reqstatus = stub_->RequestRobustLocation(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
        rLConfig.enabled = static_cast<bool>(response.robust_location_configuration().enabled());
        rLConfig.enabledForE911 = static_cast<bool>(
            response.robust_location_configuration().enabled_for_e911());
        rLConfig.validMask = static_cast<telux::loc::RobustLocationConfig>(
            response.robust_location_configuration().valid_mask());
        rLConfig.version.major = static_cast<int>(
            response.robust_location_configuration().version().major_version());
        rLConfig.version.minor = static_cast<int>(
            response.robust_location_configuration().version().minor_version());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (cb && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            cb(rLConfig, errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}


telux::common::Status LocationConfiguratorStub::configureMinGpsWeek(uint16_t minGpsWeek,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureMinGpsWeekRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_min_gps_week(minGpsWeek);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureMinGpsWeek(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestMinGpsWeek(GetMinGpsWeekCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::RequestMinGpsWeekReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    uint16_t minGpsWeek = 0;
    ::grpc::Status reqstatus = stub_->RequestMinGpsWeek(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
        minGpsWeek = static_cast<int>(response.min_gps_week());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (cb && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            cb(minGpsWeek, errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureMinSVElevation(uint8_t minSVElevation,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureMinSVElevationRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_min_sv_elevation(minSVElevation);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureMinSVElevation(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestMinSVElevation(GetMinSVElevationCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::RequestMinSVElevationReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    uint16_t minSVElevation = 0;
    ::grpc::Status reqstatus = stub_->RequestMinSVElevation(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
        minSVElevation = static_cast<int>(response.min_sv_elevation());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (cb && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            cb(minSVElevation, errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureSecondaryBand(const ConstellationSet& set,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureSecondaryBandRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    for(auto itr: set) {
        int constel = static_cast<int>(itr);
        request.add_constellation_set(static_cast<::locStub::GnssConstellationType>(constel));
    }

    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureSecondaryBand(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestSecondaryBandConfig(GetSecondaryBandCallback
    cb) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::RequestSecondaryBandConfigReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->RequestSecondaryBandConfig(&context, request, &response);
    telux::loc::ConstellationSet set;
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
        for(int itr = 0; itr < response.constellation_set_size(); itr++) {
            int constel = static_cast<int>(response.constellation_set(itr));
            telux::loc::GnssConstellationType constelType;
            switch(constel) {
                case 1: constelType = telux::loc::GnssConstellationType::GPS;
                        break;
                case 2: constelType = telux::loc::GnssConstellationType::GALILEO;
                        break;
                case 3: constelType = telux::loc::GnssConstellationType::SBAS;
                        break;
                case 5: constelType = telux::loc::GnssConstellationType::GLONASS;
                        break;
                case 6: constelType = telux::loc::GnssConstellationType::BDS;
                        break;
                case 7: constelType = telux::loc::GnssConstellationType::QZSS;
                        break;
                case 8: constelType = telux::loc::GnssConstellationType::NAVIC;
                        break;
            }
            set.insert(constelType);
        }
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (cb && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            cb(set, errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::deleteAidingData(AidingData aidingDataMask,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::DeleteAidingDataRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_aiding_data_mask(aidingDataMask);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->DeleteAidingData(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureDR(const DREngineConfiguration& config,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureDRRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.mutable_config()->set_valid_mask(config.validMask);
    request.mutable_config()->mutable_mount_param()->set_roll_offset(config.mountParam.rollOffset);
    request.mutable_config()->mutable_mount_param()->set_yaw_offset(config.mountParam.yawOffset);
    request.mutable_config()->mutable_mount_param()->set_pitch_offset(config.mountParam.pitchOffset);
    request.mutable_config()->mutable_mount_param()->set_offset_unc(config.mountParam.offsetUnc);
    request.mutable_config()->set_speed_factor(config.speedFactor);
    request.mutable_config()->set_speed_factor_unc(config.speedFactorUnc);
    request.mutable_config()->set_gyro_factor(config.gyroFactor);
    request.mutable_config()->set_gyro_factor_unc(config.gyroFactorUnc);

    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureDR(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureEngineState(const EngineType engineType,
      const LocationEngineRunState engineState, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureEngineStateRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_engine_type(static_cast<int>(engineType));
    request.set_engine_state(static_cast<int>(engineState));
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureEngineState(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::provideConsentForTerrestrialPositioning(
      bool ConsentForTerrestrialPositioning, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ProvideConsentForTerrestrialPositioningRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_user_consent(ConsentForTerrestrialPositioning);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ProvideConsentForTerrestrialPositioning(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureNmeaTypes(
      const NmeaSentenceConfig nmeaType, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureNmeaTypesRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_nmea_type(nmeaType);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureNmeaTypes(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureNmea(const NmeaConfig configParams,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureNmeaRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_nmea_type(configParams.sentenceConfig);
    if(configParams.datumType == telux::loc::GeodeticDatumType::GEODETIC_TYPE_WGS_84) {
        request.set_datum_type(::locStub::DatumType::WGS_84);
    } else if(configParams.datumType == telux::loc::GeodeticDatumType::GEODETIC_TYPE_PZ_90) {
        request.set_datum_type(::locStub::DatumType::PZ_90);
    }
    request.set_engine_type(configParams.engineType);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureNmea(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureEngineIntegrityRisk(
    const EngineType engineType, uint32_t integrityRisk, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureEngineIntegrityRiskRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    int enginetype = static_cast<int>(engineType);
    request.set_engine_type(enginetype);
    request.set_integrity_risk(integrityRisk);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureEngineIntegrityRisk(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureXtraParams(bool enable,
    const XtraConfig configParams, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureXtraParamsRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_enable(enable);
    request.set_download_interval_minute(configParams.downloadIntervalMinute);
    request.set_download_timeout_sec(configParams.downloadTimeoutSec);
    request.set_download_retry_interval_minute(configParams.downloadRetryIntervalMinute);
    request.set_download_retry_attempts(configParams.downloadRetryAttempts);
    request.set_ca_path(configParams.caPath);
    std::string urls = "";
    for(auto url: configParams.serverURLs) {
        urls += url + ", ";
    }
    if(!urls.empty()) {
        urls.pop_back();
        urls.pop_back();
    }
    request.set_server_urls(urls);
    std::string ntpurls = "";
    for(auto url: configParams.ntpServerURLs) {
        ntpurls += url + ", ";
    }
    if(!ntpurls.empty()) {
        ntpurls.pop_back();
        ntpurls.pop_back();
    }
    request.set_ntp_server_urls(ntpurls);
    request.set_daemon_debug_log_level(static_cast<int>(configParams.daemonDebugLogLevel));
    request.set_integrity_download_enabled(configParams.isIntegrityDownloadEnabled);
    request.set_integrity_download_interval_minute(configParams.integrityDownloadIntervalMinute);
    request.set_nts_server_url(configParams.ntsServerURL);
    request.set_diag_logging_enabled(configParams.isDiagLoggingEnabled);

    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureXtraParams(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestXtraStatus(GetXtraStatusCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::RequestXtraStatusReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->RequestXtraStatus(&context, request, &response);
    telux::loc::XtraStatus xtraStatus = {};
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
        xtraStatus.featureEnabled = static_cast<int>(response.xtra_status().feature_enabled());
        int dataStatus = static_cast<int>(response.xtra_status().xtra_data_status());
        switch(dataStatus) {
            case 0: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_UNKNOWN;
                    break;
            case 1: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_NOT_AVAIL;
                    break;
            case 2: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_NOT_VALID;
                    break;
            case 3: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_VALID;
                    break;
        }
        xtraStatus.xtraValidForHours = static_cast<int>(
            response.xtra_status().xtra_valid_for_hours());
        xtraStatus.userConsent = static_cast<int>(response.xtra_status().consent());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (cb && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            cb(xtraStatus, errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::registerListener(
    LocConfigIndications indicationList, std::weak_ptr<ILocationConfigListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    auto sp = listener.lock();
    if(sp == nullptr) {
        return telux::common::Status::INVALIDPARAM;
    }
    for(size_t itr = 0; itr < indicationList.size(); itr++) {
        if(indicationList.test(itr)) {
            registrationMap_[itr].insert(sp);
        }
    }
    ::locStub::RegisterListenerRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    if(indicationList.test(
        static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS))) {
        uint32_t indication =
            static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS);
        if( !(registrationMask_ & (1 << indication)) ) {
            request.set_xtra_indication(true);
            //Updating the mask after the first registration.
            registrationMask_ |= (1 << indication);
        }
    }

    if(indicationList.test(
        static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_SIGNAL_UPDATE))) {
        uint32_t indication =
            static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_SIGNAL_UPDATE);
        if( !(registrationMask_ & (1 << indication)) ) {
            request.set_gnss_indication(true);
            registrationMask_ |= (1 << indication);
            }
    }
    ::grpc::Status reqstatus = stub_->RegisterListener(&context, request, &response);
    if(reqstatus.ok()) {
        status = telux::common::Status::SUCCESS;
    }
    auto f = std::async(std::launch::async, [=]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::deRegisterListener(
    LocConfigIndications indicationList, std::weak_ptr<ILocationConfigListener> listener) {
    auto sp = listener.lock();
    if(sp == nullptr) {
        return telux::common::Status::INVALIDPARAM;
    }
    bool listenerExisted = false;
    for(size_t itr = 0; itr < indicationList.size(); itr++) {
        if(indicationList.test(itr)) {
            if(registrationMap_.find(itr) != registrationMap_.end()) {
                if(registrationMap_[itr].erase(sp)) {
                    listenerExisted = true;
                }
            }
        }
    }
    if(listenerExisted) {
        if(indicationList.test(
            static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS))) {
            uint32_t indication =
                static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS);
            updateRegistrationMask(indication);
        }
        if(indicationList.test(
            static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_SIGNAL_UPDATE))) {
            uint32_t indication =
                static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_SIGNAL_UPDATE);
            updateRegistrationMask(indication);
        }
        return telux::common::Status::SUCCESS;
    } else {
        return telux::common::Status::NOSUCH;
    }
}

void LocationConfiguratorStub::updateRegistrationMask(uint32_t indication) {
    std::vector<std::weak_ptr<ILocationConfigListener>> retList {};
    getAvailableListeners(indication, retList);
    if(retList.empty()) {
        //Resetting the indication to get the update for the next first registration.
        registrationMask_ ^= (1 << indication);
    }
}

void LocationConfiguratorStub::getAvailableListeners(uint32_t indication,
    std::vector<std::weak_ptr<ILocationConfigListener>> &vec) {
    if(registrationMap_.find(indication) != registrationMap_.end()) {
        vec.assign(registrationMap_[indication].begin(), registrationMap_[indication].end());
    }
}

void LocationConfiguratorStub::invokeXtraStatusUpdate(uint32_t enable,
    uint32_t dataStatus, uint32_t validHours, uint32_t consent) {
    uint32_t indication =
            static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS);
    XtraStatus xtraStatus;
    xtraStatus.featureEnabled = enable;
    switch(dataStatus) {
        case 0: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_UNKNOWN;
                break;
        case 1: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_NOT_AVAIL;
                break;
        case 2: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_NOT_VALID;
                break;
        case 3: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_VALID;
                break;
    }
    xtraStatus.xtraValidForHours = validHours;
    xtraStatus.userConsent = consent;
    std::vector<std::weak_ptr<ILocationConfigListener>> retList {};
    getAvailableListeners(indication, retList);
    if(!retList.empty()) {
        for (auto listener : retList) {
            auto l = listener.lock();
            // Prevent accessing a dangling listener reference.
            if(l != nullptr) {
                l->onXtraStatusUpdate(xtraStatus);
            }
        }
    }
}

void LocationConfiguratorStub::invokeGnssConstellationUpdate(uint32_t enabledMask) {
    LOG(DEBUG, __FUNCTION__);
    uint32_t indication =
        static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_SIGNAL_UPDATE);
    std::vector<std::weak_ptr<ILocationConfigListener>> retList {};
    getAvailableListeners(indication, retList);
    if(!retList.empty()) {
        for (auto listener : retList) {
            auto l = listener.lock();
            // Prevent accessing a dangling listener reference.
            if(l != nullptr) {
                l->onGnssSignalUpdate(enabledMask);
            }
        }
    }

}

telux::common::Status LocationConfiguratorStub::injectMerkleTreeInformation(
    std::string merkleTreeInfo, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->InjectMerkleTree(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureOsnma(bool enable,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ConfigureOsnmaRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_enable(enable);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ConfigureOsnma(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::provideConsentForXtra(bool userConsent,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::XtraConsentRequest request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    request.set_consent(userConsent);
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->ProvideXtraConsent(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    auto f = std::async(std::launch::async, [=]() {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            callback(errorCode);
        }
    }).share();
    taskQ_.add(f);
    return status;
}

void LocationConfiguratorStub::cleanup() {

}

LocationConfiguratorStub::~LocationConfiguratorStub() {}

} // namespace loc

} //namespace telux
