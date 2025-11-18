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

#include <telux/common/CommonDefines.hpp>
#include "LocationManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/JsonParser.hpp"
#include "common/CommonUtils.hpp"
#include "common/SimulationConfigParser.hpp"
#include "common/event-manager/ClientEventManager.hpp"

#include <chrono>
#include <string>

//Default cb delay.
#define DEFAULT_CALLBACK_DELAY 100
#define SKIP_CALLBACK -1
// Year of HW used as below
#define YEAR_OF_HW 0
#define RPC_FAIL_SUFFIX " RPC Request failed - "

namespace telux {

namespace loc {

LocationManagerStub::LocationManagerStub() {
    LOG(DEBUG, __FUNCTION__, " Creating");
    managerStatus_ = ServiceStatus::SERVICE_UNAVAILABLE;
    stub_ = CommonUtils::getGrpcStub<LocationManagerService>();
    filter_ = nullptr;
}

std::future<bool> LocationManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async, [&] { return waitForInitialization(); });
    return f;
}

bool LocationManagerStub::waitForInitialization() {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> cvLock(mutex_);
    cv_.wait(cvLock);
    return isSubsystemReady();
}

bool LocationManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::ServiceStatus LocationManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return managerStatus_;
}

telux::common::Status LocationManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    auto f
        = std::async(std::launch::async, [this, callback]() { this->initSync(callback); }).share();
    taskQ_.add(f);
    return telux::common::Status::SUCCESS;
}

void LocationManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::string> filters = {"loc_mgr"};
    auto &clientEventManager = telux::common::ClientEventManager::getInstance();
    clientEventManager.registerListener(shared_from_this(), filters);
    ::locStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
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

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        callback(managerStatus_);
        bool enableFiltering = false;
        std::shared_ptr<SimulationConfigParser> configParser =
            std::make_shared<SimulationConfigParser>();
        std::string locFiltering = configParser->getValue("ENABLE_LOCATION_FILTERING");
        if (!locFiltering.empty()) {
            enableFiltering = (locFiltering == "TRUE") ? true : false;
        }
        if (enableFiltering) {
            try {
                filter_ = std::make_shared<LocationReportFilter>();
            } catch (std::bad_alloc & e) {
                LOG(ERROR, __FUNCTION__, " LocationReportFilter: ", e.what());
            }
        }
        auto myself = shared_from_this();
        myselfForReports_ = myself;
    }
    cv_.notify_all();
}

telux::common::Status LocationManagerStub::registerListenerEx(std::weak_ptr<ILocationListener>
        listener) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        bool existing = 0;
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, " Register Listener : Existing");
                break;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Register Listener : Adding");
        }
    }
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::deRegisterListenerEx(std::weak_ptr<ILocationListener>
        listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = listeners_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " In deRegister Listener : Removing");
                retVal=telux::common::Status::SUCCESS;
                break;
            }
        }
    }
    return (retVal);
}

void LocationManagerStub::adjustTimeInterval(uint32_t &interval) {
    //If multiple position sessions are started in a process, requesting for different time
    //intervals, the filtering logic usually passes the client requested interval to the lower
    //layers and will filter excess reports accordingly. But, in one of the corner cases where there
    //are two clients requesting 200ms and 500ms, the filtering logic will filter all the reports
    //for the 500ms client. This issue arises as 500 is not a multiple of 200. The other supported
    //intervals do not have this problem. Hence to avoid this case, changing the time interval from
    //200ms to 100ms before passing to LCA client.
    if (interval == 200) {
        LOG(DEBUG, __FUNCTION__);
        interval = 100;
    }
    return;
}

telux::common::Status LocationManagerStub::startDetailedReports(uint32_t interval,
    telux::common::ResponseCallback callback, GnssReportTypeMask reportMask) {
    LOG(DEBUG, __FUNCTION__);
    if(filter_ != nullptr) {
        adjustTimeInterval(interval);
    }
    interval_ = interval;
    //Register for Reports.
    std::vector<std::string> filters = {"LOC_REPORTS"};
    auto &locationReportListener = telux::common::LocationReportListener::getInstance();
    locationReportListener.registerListener(myselfForReports_, filters);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->StartDetailedReports(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        if (filter_ != nullptr) {
            telux::common::Status rc = filter_->startReportFilter(interval, ReportType::FUSED);
            if (rc != telux::common::Status::SUCCESS) {
                LOG(WARNING, __FUNCTION__, " Starting detailed report filter Failed");
            }
        }
        sessionMask_ = telux::loc::DETAILED;
        reportMask_ = reportMask;
    }
    return status;
}

telux::common::Status LocationManagerStub::startDetailedEngineReports(uint32_t interval,
    LocReqEngine engineType, telux::common::ResponseCallback callback,
    GnssReportTypeMask reportMask) {
    LOG(DEBUG, __FUNCTION__);
    if(filter_ != nullptr) {
        adjustTimeInterval(interval);
    }
    interval_ = interval;
    engineType_ = engineType;
    //Register for Reports.
    std::vector<std::string> filters = {"LOC_REPORTS"};
    auto &locationReportListener = telux::common::LocationReportListener::getInstance();
    locationReportListener.registerListener(myselfForReports_, filters);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->StartDetailedEngineReports(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        if (filter_ != nullptr) {
            telux::common::Status rc = telux::common::Status::SUCCESS;
            if (engineType_ & LocReqEngineType::LOC_REQ_ENGINE_FUSED_BIT) {
                rc = filter_->startReportFilter(interval, ReportType::FUSED);
                if (rc != telux::common::Status::SUCCESS) {
                    LOG(WARNING, __FUNCTION__, " Starting FUSED engine report filter Failed");
                }
            }
            if (engineType_ & LocReqEngineType::LOC_REQ_ENGINE_SPE_BIT) {
                rc = filter_->startReportFilter(interval, ReportType::SPE);
                if (rc != telux::common::Status::SUCCESS) {
                    LOG(WARNING, __FUNCTION__, " Starting SPE engine report filter Failed");
                }
            }
            if (engineType_ & LocReqEngineType::LOC_REQ_ENGINE_PPE_BIT) {
                rc = filter_->startReportFilter(interval, ReportType::PPE);
                if (rc != telux::common::Status::SUCCESS) {
                    LOG(WARNING, __FUNCTION__, " Starting PPE engine report filter Failed");
                }
            }
            if (engineType_ & LocReqEngineType::LOC_REQ_ENGINE_VPE_BIT) {
                rc = filter_->startReportFilter(interval, ReportType::VPE);
                if (rc != telux::common::Status::SUCCESS) {
                    LOG(WARNING, __FUNCTION__, " Starting VPE engine report filter Failed");
                }
            }
        }
        sessionMask_ = telux::loc::DETAILED_ENGINE;
        reportMask_ = reportMask;
    }
    return status;
}

telux::common::Status LocationManagerStub::startBasicReports(
    uint32_t distanceInMeters, uint32_t interval, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (filter_ != nullptr) {
        adjustTimeInterval(interval);
    }
    interval_ = interval;
    //Register for Reports.
    std::vector<std::string> filters = {"LOC_REPORTS"};
    auto &locationReportListener = telux::common::LocationReportListener::getInstance();
    locationReportListener.registerListener(myselfForReports_, filters);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->StartBasicReports(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        if (filter_ != nullptr) {
            telux::common::Status rc = filter_->startReportFilter(interval, ReportType::FUSED);
            if (rc != telux::common::Status::SUCCESS) {
                LOG(WARNING, __FUNCTION__, " Starting basic report filter Failed");
            }
        }
        sessionMask_ = telux::loc::BASIC;
    }
    return status;
}

telux::common::Status LocationManagerStub::startBasicReports(
    uint32_t interval, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (filter_ != nullptr) {
        adjustTimeInterval(interval);
    }
    interval_ = interval;
    //Register for Reports.
    std::vector<std::string> filters = {"LOC_REPORTS"};
    auto &locationReportListener = telux::common::LocationReportListener::getInstance();
    locationReportListener.registerListener(myselfForReports_, filters);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->StartBasicReports(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        if (filter_ != nullptr) {
            telux::common::Status rc = filter_->startReportFilter(interval, ReportType::FUSED);
            if (rc != telux::common::Status::SUCCESS) {
                LOG(WARNING, __FUNCTION__, " Starting basic report filter Failed");
            }
        }
        sessionMask_ = telux::loc::BASIC;
    }
    return status;
}

telux::common::Status LocationManagerStub::stopReports(telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::string> filters = {"LOC_REPORTS"};
    auto &locationReportListener = telux::common::LocationReportListener::getInstance();
    locationReportListener.deregisterListener(myselfForReports_, filters);
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;
    ClientContext context;
    ::grpc::Status reqstatus = stub_->StopReports(&context, request, &response);
    if(reqstatus.ok()) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_CALLBACK_DELAY));
                callback(telux::common::ErrorCode::SUCCESS);
            }
        }).share();
        taskQ_.add(f);
        if (filter_ != nullptr) {
            filter_->resetAllFilters();
        }
        sessionMask_ = 0;
        reportMask_ = 0;
        interval_ = 0;
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::registerForSystemInfoUpdates(
    std::weak_ptr<ILocationSystemInfoListener> listener, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> listenerLock(listenerMutex_);
    auto spt = listener.lock();
    bool existing = 0;
    if (spt != nullptr) {
        for (auto iter=systemInfoListener_.begin(); iter<systemInfoListener_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, " System Info Listener : Existing");
                break;
            }
        }
        if (existing == 0) {
            systemInfoListener_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Registering SystemInfo Listener");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Invalid parameter, listener is null");
        return telux::common::Status::FAILED;
    }
    telux::common::Status status = telux::common::Status::SUCCESS;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::SUCCESS;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    if(existing == 0 && systemInfoListener_.size() == 1) {
        const ::google::protobuf::Empty request;
        ::locStub::LocManagerCommandReply response;
        ClientContext context;
        status = telux::common::Status::FAILED;
        errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
        ::grpc::Status reqstatus = stub_->RegisterLocationSystemInfo(&context, request, &response);
        if(reqstatus.ok()) {
            status = static_cast<telux::common::Status>(response.status());
            errorCode = static_cast<telux::common::ErrorCode>(response.error());
            cbDelay = static_cast<int>(response.delay());
        } else {
            LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        }
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

telux::common::Status LocationManagerStub::deRegisterForSystemInfoUpdates(
    std::weak_ptr<ILocationSystemInfoListener> listener, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> listenerLock(listenerMutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=systemInfoListener_.begin(); iter<systemInfoListener_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = systemInfoListener_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " Removing System Info Listener");
                break;
            }
        }
    }
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->DeregisterLocationSystemInfo(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

telux::common::Status LocationManagerStub::requestEnergyConsumedInfo(GetEnergyConsumedCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::RequestEnergyConsumedInfoReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->RequestEnergyConsumedInfo(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        telux::loc::GnssEnergyConsumedInfo energyConsumed = {};
        energyConsumed.valid = static_cast<int>(response.validity());
        energyConsumed.energySinceFirstBoot = static_cast<int>(response.energy_consumed());
        auto f = std::async(std::launch::async, [=]() {
            if (cb && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                cb(energyConsumed, errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

telux::common::Status LocationManagerStub::getYearOfHw(GetYearOfHwCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::GetYearOfHwReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->GetYearOfHw(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        uint16_t yearOfHw = static_cast<int>(response.year_of_hw());
        auto f = std::async(std::launch::async, [=]() {
            if (cb && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                cb(yearOfHw, errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

telux::loc::LocCapability LocationManagerStub::getCapabilities() {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::GetCapabilitiesReply response;
    ClientContext context;
    ::grpc::Status reqstatus = stub_->GetCapabilities(&context, request, &response);
    uint32_t capabilities = 0;
    if(reqstatus.ok()) {
        capabilities = static_cast<int>(response.loc_capability());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    return capabilities;
}

void LocationManagerStub::parseDetailedPvtReports(std::shared_ptr<LocationInfoEx> &loc,
    std::vector<std::string> &message) {
    LOG(DEBUG, __FUNCTION__);
    size_t itr = 2;
    loc->setUtcFixTime(std::stoull(message[itr++]));
    loc->setLocOutputEngType(
        static_cast<telux::loc::LocationAggregationType>(std::stoul(message[itr++])));
    loc->setLocationTechnology(std::stoul(message[itr++]));
    loc->setLatitude(std::stod(message[itr++]));
    loc->setLongitude(std::stod(message[itr++]));
    loc->setAltitude(std::stod(message[itr++]));
    loc->setHeading(std::stof(message[itr++]));
    loc->setSpeed(std::stof(message[itr++]));
    loc->setHeadingUncertainty(std::stof(message[itr++]));
    loc->setSpeedUncertainty(std::stof(message[itr++]));
    loc->setHorizontalUncertainty(std::stof(message[itr++]));
    loc->setVerticalUncertainty(std::stof(message[itr++]));
    loc->setLocationInfoValidity(std::stoul(message[itr++]));
    loc->setElapsedRealTime(std::stoull(message[itr++]));
    loc->setElapsedRealTimeUncertainty(std::stoull(message[itr++]));
    loc->setLocationInfoExValidity(std::stoull(message[itr++]));
    loc->setAltitudeMeanSeaLevel(std::stof(message[itr++]));
    loc->setPositionDop(std::stof(message[itr++]));
    loc->setHorizontalDop(std::stof(message[itr++]));
    loc->setVerticalDop(std::stof(message[itr++]));
    loc->setGeometricDop(std::stof(message[itr++]));
    loc->setTimeDop(std::stof(message[itr++]));
    loc->setMagneticDeviation(std::stof(message[itr++]));
    loc->setHorizontalReliability(
        static_cast<telux::loc::LocationReliability>(std::stoi(message[itr++])));
    loc->setVerticalReliability(
        static_cast<telux::loc::LocationReliability>(std::stoi(message[itr++])));
    loc->setHorizontalUncertaintySemiMajor(std::stof(message[itr++]));
    loc->setHorizontalUncertaintySemiMinor(std::stof(message[itr++]));
    loc->setHorizontalUncertaintyAzimuth(std::stof(message[itr++]));
    loc->setEastStandardDeviation(std::stof(message[itr++]));
    loc->setNorthStandardDeviation(std::stof(message[itr++]));
    loc->setNumSvUsed(std::stoul(message[itr++]));
    telux::loc::SvUsedInPosition svUsedInPosition;
    svUsedInPosition.gps = std::stoull(message[itr++]);
    svUsedInPosition.glo = std::stoull(message[itr++]);
    svUsedInPosition.gal = std::stoull(message[itr++]);
    svUsedInPosition.bds = std::stoull(message[itr++]);
    svUsedInPosition.qzss = std::stoull(message[itr++]);
    svUsedInPosition.navic = std::stoull(message[itr++]);
    loc->setSvUsedInPosition(svUsedInPosition);
    std::bitset<SBAS_COUNT> sbas = std::stoull(message[itr++]);
    loc->setSbasCorrection(sbas);
    loc->setPositionTechnology(std::stoul(message[itr++]));
    telux::loc::GnssKinematicsData bodyFrameData;
    bodyFrameData.latAccel = std::stof(message[itr++]);
    bodyFrameData.longAccel = std::stof(message[itr++]);
    bodyFrameData.vertAccel = std::stof(message[itr++]);
    bodyFrameData.yawRate = std::stof(message[itr++]);
    bodyFrameData.pitch = std::stof(message[itr++]);
    bodyFrameData.latAccelUnc = std::stof(message[itr++]);
    bodyFrameData.longAccelUnc = std::stof(message[itr++]);
    bodyFrameData.vertAccelUnc = std::stof(message[itr++]);
    bodyFrameData.yawRateUnc = std::stof(message[itr++]);
    bodyFrameData.pitchUnc = std::stof(message[itr++]);
    bodyFrameData.pitchRate = std::stof(message[itr++]);
    bodyFrameData.pitchRateUnc = std::stof(message[itr++]);
    bodyFrameData.roll = std::stof(message[itr++]);
    bodyFrameData.rollUnc = std::stof(message[itr++]);
    bodyFrameData.rollRate = std::stof(message[itr++]);
    bodyFrameData.rollRateUnc = std::stof(message[itr++]);
    bodyFrameData.yaw = std::stof(message[itr++]);
    bodyFrameData.yawUnc = std::stof(message[itr++]);
    bodyFrameData.bodyFrameDataMask = std::stoul(message[itr++]);
    loc->setBodyFrameData(bodyFrameData);
    loc->setTimeUncMs(std::stof(message[itr++]));
    loc->setLeapSeconds(std::stoul(message[itr++]));
    loc->setCalibrationConfidencePercent(std::stoul(message[itr++]));
    loc->setCalibrationStatus(std::stoul(message[itr++]));
    loc->setConformityIndex(std::stof(message[itr++]));
    telux::loc::LLAInfo llaVRPInfo = {0};
    llaVRPInfo.latitude = std::stod(message[itr++]);
    llaVRPInfo.longitude = std::stod(message[itr++]);
    llaVRPInfo.altitude = std::stod(message[itr++]);
    loc->setVRPBasedLLA(llaVRPInfo);
    std::vector<float> enuVelocity(3);
    enuVelocity[0] = std::stof(message[itr++]);
    enuVelocity[1] = std::stof(message[itr++]);
    enuVelocity[2] = std::stof(message[itr++]);
    loc->setVRPBasedENUVelocity(enuVelocity);
    loc->setAltitudeType(static_cast<telux::loc::AltitudeType>(std::stoi(message[itr++])));
    loc->setReportStatus(static_cast<telux::loc::ReportStatus>(std::stoi(message[itr++])));
    loc->setIntegrityRiskUsed(std::stoul(message[itr++]));
    loc->setProtectionLevelAlongTrack(std::stof(message[itr++]));
    loc->setProtectionLevelCrossTrack(std::stof(message[itr++]));
    loc->setProtectionLevelVertical(std::stof(message[itr++]));
    loc->setSolutionStatus(std::stoul(message[itr++]));
    size_t measInfoSize = std::stoi(message[itr++]);
    std::vector<GnssMeasurementInfo> measInfo;
    for(size_t i = 0; i < measInfoSize; i++) {
        telux::loc::GnssMeasurementInfo temp;
        temp.gnssSignalType = std::stoul(message[itr++]);
        temp.gnssConstellation =
            static_cast<telux::loc::GnssSystem>(std::stoi(message[itr++]));
        temp.gnssSvId = std::stoul(message[itr++]);
        measInfo.push_back(temp);
    }
    loc->setMeasUsageInfo(measInfo);
    size_t enuVelocitySize = std::stoi(message[itr++]);
    std::vector<float> velocityEastNorthUp;
    for(size_t i = 0; i < enuVelocitySize; i++) {
        velocityEastNorthUp.push_back(std::stof(message[itr++]));
    }
    loc->setVelocityEastNorthUp(velocityEastNorthUp);
    size_t enuVelocityUncertaintySize = std::stoi(message[itr++]);
    std::vector<float> setVelocityEastNorthUpUnc;
    for(size_t i = 0; i < enuVelocityUncertaintySize; i++) {
        setVelocityEastNorthUpUnc.push_back(std::stof(message[itr++]));
    }
    loc->setVelocityUncertaintyEastNorthUp(setVelocityEastNorthUpUnc);
    size_t usedSVsize = std::stoi(message[itr++]);
    std::vector<uint16_t> usedSvs;
    for(size_t i = 0; i < usedSVsize; i++) {
        auto msg = message[itr];
        usedSvs.push_back(msg.length() == 0 ? 0 : std::stoul(msg));
        ++itr;
    }
    loc->setUsedSVsIds(usedSvs);
    telux::loc::GnssSystem system =
        static_cast<telux::loc::GnssSystem>(std::stoi(message[itr++]));
    if (system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GLONASS) {
        telux::loc::SystemTime time;
        time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GLONASS;
        time.time.glo.validityMask = std::stoul(message[itr++]);
        time.time.glo.gloDays = std::stoul(message[itr++]);
        time.time.glo.gloMsec = std::stoul(message[itr++]);
        time.time.glo.gloClkTimeBias = std::stof(message[itr++]);
        time.time.glo.gloClkTimeUncMs = std::stof(message[itr++]);
        time.time.glo.refFCount = std::stoul(message[itr++]);
        time.time.glo.numClockResets = std::stoul(message[itr++]);
        time.time.glo.gloFourYear = std::stoul(message[itr++]);
        loc->setGnssSystemTime(time);
    } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_SBAS) {
        telux::loc::SystemTime time;
        time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_SBAS;
        loc->setGnssSystemTime(time);
    } else {
        telux::loc::SystemTime time;
        if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS) {
            time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS;
        }
        if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GALILEO) {
            time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GALILEO;
        }
        if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_BDS) {
            time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_BDS;
        }
        if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_QZSS) {
            time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_QZSS;
        }
        if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_NAVIC) {
            time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_NAVIC;
        }
        time.time.gps.validityMask = std::stoul(message[itr++]);
        time.time.gps.numClockResets = std::stoul(message[itr++]);
        time.time.gps.refFCount = std::stoul(message[itr++]);
        time.time.gps.systemClkTimeUncMs = std::stof(message[itr++]);
        time.time.gps.systemClkTimeBias = std::stof(message[itr++]);
        time.time.gps.systemMsec = std::stoul(message[itr++]);
        time.time.gps.systemWeek = std::stoul(message[itr++]);
        loc->setGnssSystemTime(time);
    }
    std::bitset<NAV_COUNT> navSol = std::stoull(message[itr++]);
    loc->setNavigationSolution(navSol);
    loc->setElapsedGptpTime(std::stoull(message[itr++]));
    loc->setElapsedGptpTimeUnc(std::stoull(message[itr++]));
    size_t dgnssStationIdsSize = std::stoi(message[itr++]);
    std::vector<uint16_t> dgnssStationIds;
    for(size_t i = 0; i < dgnssStationIdsSize; i++) {
        auto msg = message[itr];
        dgnssStationIds.push_back(msg.length() == 0 ? 0 : std::stoul(msg));
        ++itr;
    }
    loc->setDgnssStationIds(dgnssStationIds);
    loc->setBaselineLength(std::stod(message[itr++]));
    loc->setAgeOfCorrections(std::stoull(message[itr++]));
    loc->setLeapSecondsUncertainty(std::stoul(message[itr++]));
}

void LocationManagerStub::setLocationInfoBase(std::shared_ptr<LocationInfoBase> &loc,
    std::shared_ptr<LocationInfoEx> &locImpl) {
    LOG(DEBUG, __FUNCTION__);
    loc->setUtcFixTime(locImpl->getTimeStamp());
    loc->setLocationTechnology(locImpl->getTechMask());
    loc->setLatitude(locImpl->getLatitude());
    loc->setLongitude(locImpl->getLongitude());
    loc->setAltitude(locImpl->getAltitude());
    loc->setHeading(locImpl->getHeading());
    loc->setSpeed(locImpl->getSpeed());
    loc->setHeadingUncertainty(locImpl->getHeadingUncertainty());
    loc->setSpeedUncertainty(locImpl->getSpeedUncertainty());
    loc->setHorizontalUncertainty(locImpl->getHorizontalUncertainty());
    loc->setVerticalUncertainty(locImpl->getVerticalUncertainty());
    loc->setLocationInfoValidity(locImpl->getLocationInfoValidity());
    loc->setElapsedRealTime(locImpl->getElapsedRealTime());
    loc->setElapsedRealTimeUncertainty(locImpl->getElapsedRealTimeUncertainty());
    loc->setTimeUncMs(locImpl->getTimeUncMs());
    loc->setElapsedGptpTime(locImpl->getElapsedGptpTime());
    loc->setElapsedGptpTimeUnc(locImpl->getElapsedGptpTimeUnc());
}

std::shared_ptr<LocationInfoBase> LocationManagerStub::getLastLocation(
    bool defaultLocInfo) {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<LocationInfoBase> locInfo = std::make_shared<LocationInfoBase>();
    if (defaultLocInfo) {
        locInfo->setLatitude(0);
        locInfo->setLongitude(0);
        locInfo->setLocationInfoValidity(0);
    } else {
        const ::google::protobuf::Empty request;
        ::locStub::LastLocationInfo response;
        ClientContext context;
        ::grpc::Status reqstatus = stub_->GetLastLocation(&context, request, &response);
        if(reqstatus.ok()) {
            std::string msg = response.loc_report();
            if(!msg.empty()) {
                std::vector<std::string> message = CommonUtils::splitString(msg);
                uint64_t utcTimestamp;
                utcTimestamp =
                    (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 100) * 100 ;
                message[2] = std::to_string(utcTimestamp);
                std::shared_ptr<LocationInfoEx> locImpl = std::make_shared<LocationInfoEx>();
                parseDetailedPvtReports(locImpl, message);
                std::shared_ptr<LocationInfoBase> loc = std::make_shared<LocationInfoBase>();
                setLocationInfoBase(loc, locImpl);
            } else {
                locInfo->setLatitude(0);
                locInfo->setLongitude(0);
                locInfo->setLocationInfoValidity(0);
            }
        } else {
            LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
            locInfo->setLatitude(0);
            locInfo->setLongitude(0);
            locInfo->setLocationInfoValidity(0);
        }
    }
    return locInfo;
}

telux::common::Status LocationManagerStub::getTerrestrialPosition(uint32_t timeoutMsec,
    TerrestrialTechnology techMask, GetTerrestrialInfoCallback cb,
    telux::common ::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->GetTerrestrialPosition(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            uint32_t delay = cbDelay;
            std::shared_ptr<LocationInfoBase> locInfo;
            LOG(DEBUG, "Timeout: ", timeoutMsec, ", delay: ", delay);
            if (timeoutMsec <= delay) {
                LOG(INFO, "timeout shorter, will send default location");
                delay = timeoutMsec;
                LOG(DEBUG, "Timeout: ", timeoutMsec, ", delay: ", delay);
                locInfo = getLastLocation(true);
            } else {
                LOG(INFO, "timeout lengthier, will send last received location unless cancelled");
                locInfo = getLastLocation();
            }
            LOG(DEBUG, "Timeout: ", timeoutMsec, ", delay: ", delay);
            std::unique_lock<std::mutex> lk(terrestrialPositionMutex_);
            if (cvTerrestrialPosition_.wait_for(lk, std::chrono::milliseconds(delay))
                == std::cv_status::timeout) {
                    LOG(DEBUG, "Timed out, sending GTP callback");
                    cb(locInfo);
            } else {
                LOG(DEBUG, "GTP callback cancelled");
            }
            if (callback) {
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

telux::common::Status LocationManagerStub::cancelTerrestrialPositionRequest(
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->CancelTerrestrialPosition(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (errorCode == ErrorCode::SUCCESS) {
                std::lock_guard<std::mutex> lk(terrestrialPositionMutex_);
                cvTerrestrialPosition_.notify_all();
            }
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

LocationManagerStub::~LocationManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void LocationManagerStub::cleanup() {
}

void LocationManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::locStub::StartReportsEvent>()) {
        ::locStub::StartReportsEvent startEvent;
        event.UnpackTo(&startEvent);
        parseRequest(startEvent);
    } else if (event.Is<::locStub::CapabilitiesUpdateEvent>()) {
        LOG(DEBUG, __FUNCTION__, " Capabilities update");
        ::locStub::CapabilitiesUpdateEvent capabilitiesEvent;
        event.UnpackTo(&capabilitiesEvent);
        handleCapabilitiesUpdateEvent(capabilitiesEvent);
    } else if (event.Is<::locStub::SysInfoUpdateEvent>()) {
        LOG(DEBUG, __FUNCTION__, " SysInfo update");
        ::locStub::SysInfoUpdateEvent sysInfoEvent;
        event.UnpackTo(&sysInfoEvent);
        handleSysInfoUpdateEvent(sysInfoEvent);
    } else if (event.Is<::locStub::StreamingStoppedEvent>()) {
        LOG(DEBUG, __FUNCTION__, " StreamingStopped update");
        ::locStub::StreamingStoppedEvent streamingStoppedEvent;
        event.UnpackTo(&streamingStoppedEvent);
        handleStreamingStoppedEvent();
    } else if (event.Is<::locStub::ResetWindowEvent>()) {
        LOG(DEBUG, __FUNCTION__, " ResetWindow update");
        ::locStub::ResetWindowEvent resetWindowEvent;
        event.UnpackTo(&resetWindowEvent);
        handleResetWindowEvent();
    } else if (event.Is<::locStub::GnssDisasterCrisisReport>()) {
        LOG(DEBUG, __FUNCTION__, " Disaster Crisis update");
        ::locStub::GnssDisasterCrisisReport dcReport;
        event.UnpackTo(&dcReport);
        handleGnssDisasterCrisisReport(dcReport);
    }
}

void LocationManagerStub::parseRequest(::locStub::StartReportsEvent startEvent) {
    LOG(DEBUG, __FUNCTION__);
    std::string msg = startEvent.loc_report();
    std::vector<std::string> message = CommonUtils::splitString(msg);
    uint32_t opt = std::stoul(message[1]);
    switch(opt) {
        case telux::loc::GnssReportType::LOCATION :
        if (sessionMask_ & telux::loc::BASIC)
        {
            telux::loc::LocationAggregationType msgEngineType =
                static_cast<telux::loc::LocationAggregationType>(
                    std::stoul(message[3]));

            if (msgEngineType != telux::loc::LocationAggregationType::LOC_OUTPUT_ENGINE_FUSED) {
                return;
            }

            //1. Check TBF w.r.t UTC field and reject if outside the window.
            {
                std::unique_lock<std::mutex> lck(filterMutex_);
                if (filter_ != nullptr) {
                    uint64_t timestamp = telux::loc::UNKNOWN_TIMESTAMP;
                    telux::loc::LocationInfoValidity validity = std::stoul(message[14]);
                    if(validity & telux::loc::HAS_TIMESTAMP_BIT) {
                        timestamp = std::stoull(message[2]);
                    }
                    if (filter_->isReportIgnored(timestamp, ReportType::FUSED)) {
                        LOG(DEBUG, __FUNCTION__, " Report is filtered, hence not sending");
                        return;
                    }
                    //2. Update the timestamp
                    uint64_t utcTimestamp;
                    if(interval_ % 1000 == 0) {
                        utcTimestamp =
                            (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 1000) * 1000 ;
                    } else {
                        utcTimestamp =
                            (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 100) * 100 ;
                    }
                    message[2] = std::to_string(utcTimestamp);
                } else {
                    //2. Update the timestamp
                    uint64_t utcTimestamp;
                    utcTimestamp =
                            (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 100) * 100 ;
                    message[2] = std::to_string(utcTimestamp);
                }
            }
            //3. Parse.
            std::shared_ptr<LocationInfoEx> locImpl = std::make_shared<LocationInfoEx>();
            parseDetailedPvtReports(locImpl, message);
            std::shared_ptr<LocationInfoBase> loc = std::make_shared<LocationInfoBase>();
            setLocationInfoBase(loc, locImpl);

            //Send data to clients.
            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    spt->onBasicLocationUpdate(loc);
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        } else if ((reportMask_ & telux::loc::GnssReportType::LOCATION) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {

            telux::loc::LocationAggregationType msgEngineType =
                static_cast<telux::loc::LocationAggregationType>(
                    std::stoul(message[3]));

            if (sessionMask_ & telux::loc::DETAILED) {
                if (msgEngineType != telux::loc::LocationAggregationType::LOC_OUTPUT_ENGINE_FUSED) {
                    return;
                }
            }
            //1. Check TBF w.r.t UTC field and reject if outside the window.
            {
                std::unique_lock<std::mutex> lck(filterMutex_);
                if (filter_ != nullptr) {
                    uint64_t timestamp = telux::loc::UNKNOWN_TIMESTAMP;
                    telux::loc::LocationInfoValidity validity = std::stoul(message[14]);
                    if(validity & telux::loc::HAS_TIMESTAMP_BIT) {
                        timestamp = std::stoull(message[2]);
                    }
                    if(sessionMask_ & telux::loc::DETAILED) {
                        if (filter_->isReportIgnored(timestamp, ReportType::FUSED)) {
                            LOG(DEBUG, __FUNCTION__, " Report is filtered, hence not sending");
                            return;
                        }
                    } else { // DETAILED_ENGINE
                        if (filter_->isReportIgnored(
                            timestamp,
                            static_cast<ReportType>(std::stoul(message[3])))) {
                            LOG(DEBUG, __FUNCTION__, " Report is filtered, hence not sending");
                            return;
                        }
                    }
                    //2. Update the timestamp
                    uint64_t utcTimestamp;
                    if(interval_ % 1000 == 0) {
                        utcTimestamp =
                            (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 1000) * 1000 ;
                    } else {
                        utcTimestamp =
                            (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 100) * 100 ;
                    }
                    message[2] = std::to_string(utcTimestamp);
                } else {
                    //2. Update the timestamp
                    uint64_t utcTimestamp;
                    utcTimestamp =
                            (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 100) * 100 ;
                    message[2] = std::to_string(utcTimestamp);
                }
            }
            //Parse.
            std::shared_ptr<LocationInfoEx> loc = std::make_shared<LocationInfoEx>();
            parseDetailedPvtReports(loc, message);

            //Send data to clients.
            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    if (sessionMask_ & telux::loc::DETAILED) {
                        spt->onDetailedLocationUpdate(loc);
                    } else {
                        if (0 != (engineType_ & (0x1 << msgEngineType))) {
                            std::vector<std::shared_ptr<ILocationInfoEx>> infoEngineReports;
                            infoEngineReports.push_back(loc);
                            spt->onDetailedEngineLocationUpdate(infoEngineReports);
                        }
                    }
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        }
        break;

        case telux::loc::GnssReportType::SATELLITE_VEHICLE :
        if( (reportMask_ & telux::loc::GnssReportType::SATELLITE_VEHICLE) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {
            //Parse.
            std::shared_ptr<telux::loc::GnssSVInfo> gSV =
                std::make_shared<telux::loc::GnssSVInfo>();
            std::vector<std::shared_ptr<telux::loc::ISVInfo> > gnssSvList;
            size_t rowItr = 2;
            while ((rowItr + 11) < message.size()) {
                std::shared_ptr<telux::loc::SVInfo> svInfo = std::make_shared<SVInfo>();
                svInfo->setId(std::stoul(message[rowItr++]));
                svInfo->setConstellation(static_cast<telux::loc::GnssConstellationType>(
                    std::stoi(message[rowItr++])));
                svInfo->setHasEphemeris(static_cast<telux::loc::SVInfoAvailability>(
                    std::stoi(message[rowItr++])));
                svInfo->setHasAlmanac(static_cast<telux::loc::SVInfoAvailability>(
                    std::stoi(message[rowItr++])));
                svInfo->setHasFix(static_cast<telux::loc::SVInfoAvailability>(
                    std::stoi(message[rowItr++])));
                svInfo->setElevation(std::stof(message[rowItr++]));
                svInfo->setAzimuth(std::stof(message[rowItr++]));
                svInfo->setSnr(std::stof(message[rowItr++]));
                svInfo->setCarrierFrequency(std::stof(message[rowItr++]));
                svInfo->setSignalType(static_cast<telux::loc::GnssSignalType>(
                    std::stoull(message[rowItr++])));
                svInfo->setGlonassFcn(std::stoul(message[rowItr++]));
                svInfo->setBasebandCnr(std::stod(message[rowItr++]));

                if (svInfo != nullptr) {
                    gnssSvList.push_back(svInfo);
                } else {
                    LOG(ERROR, __FUNCTION__, " memory allocation failure for satVehicle");
                    return;
                }
            }
            //Send data to clients.
            if (gSV) {
                gSV->setAltitudeType(telux::loc::AltitudeType::UNKNOWN);
                gSV->setSVInfoList(gnssSvList);
                for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                    auto spt = (*iter).lock();
                    if (spt != nullptr) {
                        spt->onGnssSVInfo(gSV);
                        ++iter;
                    } else {
                        iter = listeners_.erase(iter);
                    }
                }
            } else {
                LOG(ERROR, __FUNCTION__, "memory allocation failure for gSV");
                return;
            }
        }
        break;

        case telux::loc::GnssReportType::NMEA :
        if( (reportMask_ & telux::loc::GnssReportType::NMEA) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {
            uint64_t timestamp =
                (std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000;
            std::string nmea = "";
            bool calcChecksum = false;
            // update the timestamp, format hhmmss.sss
            // nmea starts with $, end with *checksum
            //   1701338412905,4,1701338412903,$GNGSA,A,3,15,21,27,,,,,,,,,,2.0,1.7,0.9,3*3C
            // for NMEA ID of GNGGA, GNRMC, GNGNS, need to recalculate the checksum.
            // A checksum field is required and shall be transmitted in all sentences. The checksum
            // field is the last field in a sentence and follows the checksum delimiter "*".
            // The checksum is the 8-bit exclusive OR of all characters in the sentence,
            // including "," and "^" delimiters,
            // between but not including the "$" or "!" and "*" delimiters.
            if (0 == message[3].compare("$GNGGA") ||
                0 == message[3].compare("$GNRMC") ||
                0 == message[3].compare("$GNGNS")) {
                message[4] = CommonUtils::getCurrentTimeHHMMSS();
                calcChecksum = true;
                // erase the first '$'
                message[3].erase(message[3].begin());
                // erase the checksum at the end
                while (message.back().back() != '*') {
                    message.back().pop_back();
                }

                if (message.back().back() == '*') {
                    message.back().pop_back();
                } else {
                    // it is not a corrct format, error
                }
            }

            for(size_t itr = 3; itr < message.size(); itr++) {
                nmea += message[itr] + ", ";
            }
            //Remove the last ", ".
            nmea.pop_back();
            nmea.pop_back();

            if (calcChecksum) {
                nmea = "$" + nmea + "*" + std::to_string(CommonUtils::bitwiseXOR(nmea));
            }

            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    spt->onGnssNmeaInfo(timestamp, nmea);
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        }
        break;

        case telux::loc::GnssReportType::DATA :
        if( (reportMask_ & telux::loc::GnssReportType::DATA) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {
            //Parse.
            std::shared_ptr<GnssSignalInfo> gSI =
                std::make_shared<GnssSignalInfo>();
            telux::loc::GnssData gnssData = {};
            int rowItr = 2;
            for(auto i = 0;
                i < telux::loc::GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES; i++) {
                gnssData.gnssDataMask[i] = stoul(message[rowItr]);
                gnssData.jammerInd[i] = stod(message[rowItr + 1]);
                gnssData.agc[i] = stod(message[rowItr + 2]);
                rowItr = rowItr + 3;
            }
            gnssData.agcStatusL1 = static_cast<AgcStatus>(stoi(message[rowItr++]));
            gnssData.agcStatusL2 = static_cast<AgcStatus>(stoi(message[rowItr++]));
            gnssData.agcStatusL5 = static_cast<AgcStatus>(stoi(message[rowItr++]));
            if (gSI != nullptr) {
                gSI->setGnssData(gnssData);
            } else {
                LOG(ERROR, __FUNCTION__, " memory allocation failure for gSI");
                return;
            }
            //Send data to clients.
            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    spt->onGnssSignalInfo(gSI);
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        }
        break;

        case telux::loc::GnssReportType::MEASUREMENT :
        if( (reportMask_ & telux::loc::GnssReportType::MEASUREMENT) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {
            //Parse.
            telux::loc::GnssMeasurements gnssMeas;
            size_t rowItr = 2;
            gnssMeas.clock.valid = std::stoul(message[rowItr++]);
            gnssMeas.clock.leapSecond = std::stoul(message[rowItr++]);
            gnssMeas.clock.timeNs = std::stoull(message[rowItr++]);
            gnssMeas.clock.timeUncertaintyNs = std::stod(message[rowItr++]);
            gnssMeas.clock.fullBiasNs = std::stoull(message[rowItr++]);
            gnssMeas.clock.biasNs = std::stod(message[rowItr++]);
            gnssMeas.clock.biasUncertaintyNs = std::stod(message[rowItr++]);
            gnssMeas.clock.driftNsps = std::stod(message[rowItr++]);
            gnssMeas.clock.driftUncertaintyNsps = std::stod(message[rowItr++]);
            gnssMeas.clock.hwClockDiscontinuityCount = std::stoul(message[rowItr++]);
            gnssMeas.clock.elapsedRealTime = std::stoull(message[rowItr++]);
            gnssMeas.clock.elapsedRealTimeUnc = std::stoull(message[rowItr++]);
            gnssMeas.clock.elapsedgPTPTime = std::stoull(message[rowItr++]);
            gnssMeas.clock.elapsedgPTPTimeUnc = std::stoull(message[rowItr++]);
            while( (rowItr + 24) < message.size() - 1) {
                telux::loc::GnssMeasurementsData data;
                data.valid = std::stoul(message[rowItr++]);
                data.svId = std::stoul(message[rowItr++]);
                data.svType = static_cast<telux::loc::GnssConstellationType>(
                    std::stoi(message[rowItr++]));
                data.timeOffsetNs = std::stod(message[rowItr++]);
                data.stateMask = std::stoul(message[rowItr++]);
                data.receivedSvTimeNs = std::stoull(message[rowItr++]);
                data.receivedSvTimeSubNs = std::stof(message[rowItr++]);
                data.receivedSvTimeUncertaintyNs = std::stoull(message[rowItr++]);
                data.carrierToNoiseDbHz = std::stod(message[rowItr++]);
                data.pseudorangeRateMps = std::stod(message[rowItr++]);
                data.pseudorangeRateUncertaintyMps = std::stod(message[rowItr++]);
                data.adrStateMask = std::stoul(message[rowItr++]);
                data.adrMeters = std::stod(message[rowItr++]);
                data.adrUncertaintyMeters = std::stod(message[rowItr++]);
                data.carrierFrequencyHz = std::stof(message[rowItr++]);
                data.carrierCycles = std::stoull(message[rowItr++]);
                data.carrierPhase = std::stod(message[rowItr++]);
                data.carrierPhaseUncertainty = std::stod(message[rowItr++]);
                data.multipathIndicator = static_cast<
                  telux::loc::GnssMeasurementsMultipathIndicator>(std::stoi(message[rowItr++]));
                data.signalToNoiseRatioDb = std::stod(message[rowItr++]);
                data.agcLevelDb = std::stod(message[rowItr++]);
                data.gnssSignalType = std::stoul(message[rowItr++]);
                data.basebandCarrierToNoise = std::stod(message[rowItr++]);
                data.fullInterSignalBias = std::stod(message[rowItr++]);
                data.fullInterSignalBiasUncertainty = std::stod(message[rowItr++]);
                gnssMeas.measurements.push_back(data);
            }
            gnssMeas.isNHz = std::stoi(message[rowItr++]);
            gnssMeas.agcStatusL1 = static_cast<AgcStatus>(std::stoi(message[rowItr++]));
            gnssMeas.agcStatusL2 = static_cast<AgcStatus>(std::stoi(message[rowItr++]));
            gnssMeas.agcStatusL5 = static_cast<AgcStatus>(std::stoi(message[rowItr++]));
            //Send data to clients.
            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    spt->onGnssMeasurementsInfo(gnssMeas);
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        }
        break;

        case telux::loc::GnssReportType::EXTENDED_DATA :
        if( (reportMask_ & telux::loc::GnssReportType::EXTENDED_DATA) &&
            (reportMask_ & telux::loc::GnssReportType::LOCATION) &&
            (sessionMask_ & telux::loc::DETAILED_ENGINE) )
        {
            std::vector<uint8_t> payload;
            // Iterate from the 2nd index to the last index
            for (size_t rowItr = 2; rowItr < message.size(); ++rowItr) {
                int number = std::stoi(message[rowItr]);
                payload.push_back(static_cast<uint8_t>(number));
            }

            //Send data to clients.
            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    spt->onGnssExtendedDataInfo(payload);
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        }
        break;
        default :
            LOG(ERROR, __FUNCTION__, " No such report type supported");
            break;
    }
}

void LocationManagerStub::handleCapabilitiesUpdateEvent(
    ::locStub::CapabilitiesUpdateEvent capabilitiesEvent) {
    uint32_t capabilityMask = capabilitiesEvent.capability_mask();
    invokeCapabilitiesUpdateEvent(capabilityMask);
}

void LocationManagerStub::invokeCapabilitiesUpdateEvent(uint32_t capabilityMask) {
    for (auto iter = listeners_.begin(); iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onCapabilitiesInfo(capabilityMask);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

void LocationManagerStub::handleSysInfoUpdateEvent(::locStub::SysInfoUpdateEvent sysInfoEvent) {
    telux::loc::LocationSystemInfo locSystemInfo;
    locSystemInfo.valid = sysInfoEvent.sysinfo_validity();
    locSystemInfo.info.valid = sysInfoEvent.leapsecond_validity();
    locSystemInfo.info.current = sysInfoEvent.current();
    if(locSystemInfo.info.valid & LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT) {
        locSystemInfo.info.info.timeInfo.validityMask =
            static_cast<telux::loc::GnssTimeValidityType>(sysInfoEvent.gnss_validity());
        locSystemInfo.info.info.timeInfo.systemWeek = sysInfoEvent.system_week();
        locSystemInfo.info.info.timeInfo.systemMsec = sysInfoEvent.system_msec();
        locSystemInfo.info.info.timeInfo.systemClkTimeBias = sysInfoEvent.system_clk_time_bias();
        locSystemInfo.info.info.timeInfo.systemClkTimeUncMs = sysInfoEvent.system_clk_time_unc_ms();
        locSystemInfo.info.info.timeInfo.refFCount = sysInfoEvent.ref_f_count();
        locSystemInfo.info.info.timeInfo.numClockResets = sysInfoEvent.clock_resets();
        locSystemInfo.info.info.leapSecondsBeforeChange = sysInfoEvent.leap_seconds_before_change();
        locSystemInfo.info.info.leapSecondsAfterChange = sysInfoEvent.leap_seconds_after_change();
    }
    invokeSysInfoUpdateEvent(locSystemInfo);
}

void LocationManagerStub::handleGnssDisasterCrisisReport(
    ::locStub::GnssDisasterCrisisReport dcReport) {
    if ((reportMask_ & telux::loc::GnssReportType::DISASTER_CRISIS) &&
        ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE))) {

        LOG(DEBUG, __FUNCTION__, " report Disaster Crisis Info");

        telux::loc::GnssDisasterCrisisReport report;
        report.dcReportType = static_cast<telux::loc::GnssReportDCType>(dcReport.dc_report_type());
        report.numValidBits = static_cast<uint16_t>(dcReport.num_valid_bits());
        report.prnValid = static_cast<bool>(dcReport.prn_validity());
        report.prn = dcReport.prn();
        std::copy(dcReport.dc_report_data().begin(),
                  dcReport.dc_report_data().end(),
                  std::back_inserter(report.dcReportData));

        std::lock_guard<std::mutex> listenerLock(mutex_);
        for (auto iter = listeners_.begin(); iter != listeners_.end();) {
            auto spt = (*iter).lock();
            if (spt != nullptr) {
                spt->onGnssDisasterCrisisInfo(report);
                ++iter;
            } else {
                iter = listeners_.erase(iter);
            }
        }
    }
}

void LocationManagerStub::invokeSysInfoUpdateEvent(telux::loc::LocationSystemInfo &locSystemInfo) {
    LOG(DEBUG, __FUNCTION__);
    for (auto iter = systemInfoListener_.begin(); iter != systemInfoListener_.end();) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            LOG(DEBUG, __FUNCTION__, " Sending System Info");
            spt->onLocationSystemInfo(locSystemInfo);
            ++iter;
        } else {
            iter = systemInfoListener_.erase(iter);
        }
    }
}

void LocationManagerStub::handleStreamingStoppedEvent() {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async, [this](){
            this->stopReports(nullptr);
        }).share();
    taskQ_.add(f);
}

void LocationManagerStub::handleResetWindowEvent() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(filterMutex_);
    if (filter_ != nullptr) {
        filter_->resetAllFilters();
    }
}

}  // namespace loc

}  // namespace telux
