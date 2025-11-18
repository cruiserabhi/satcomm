/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ServingManagerServerImpl.hpp"
#include <thread>
#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include <telux/common/DeviceConfig.hpp>
#include <telux/common/CommonDefines.hpp>

#define JSON_PATH1 "api/tel/IServingSystemManagerSlot1.json"
#define JSON_PATH2 "api/tel/IServingSystemManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/IServingSystemManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/IServingSystemManagerStateSlot2.json"
#define MANAGER "IServingSystemManager"
#define SYSTEM_SELECTION_PREFERENCE "systemSelectionPreferenceUpdate"
#define SYSTEM_INFO "systemInfoUpdate"
#define NETWORK_TIME "networkTimeUpdate"
#define RF_BAND_INFO "rFBandInfoUpdate"
#define NETWORK_REJECTION "networkRejectionUpdate"
#define SLOT_1 1
#define SLOT_2 2

ServingManagerServerImpl::ServingManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

ServingManagerServerImpl::~ServingManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if(taskQ_) {
        taskQ_ = nullptr;
    }
}

grpc::Status ServingManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj[MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_SERVING_SYSTEM_FILTER,
            MODEM_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1
        : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    std::string srvStatus = rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestRATPreference(ServerContext* context,
    const ::telStub::RequestRATPreferenceRequest* request,
    telStub::RequestRATPreferenceReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestRatPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        std::vector<int> raTdata;
        std::string value =  data.stateRootObj[MANAGER]["RATPreference"].asString();
        LOG(DEBUG, __FUNCTION__,"String is ", value);
        raTdata = CommonUtils::convertStringToVector(value);
        for(auto &it : raTdata) {
            response->add_rat_pref_types(static_cast<telStub::RatPrefType>(it));
        }
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::SetRATPreference(ServerContext* context,
    const ::telStub::SetRATPreferenceRequest* request,
    telStub::SetRATPreferenceReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "setRatPreference";
    JsonData data;
    std::vector<uint8_t> ratPrefs;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        for (auto rat : request->rat_pref_types()) {
            ratPrefs.emplace_back(static_cast<uint8_t>(rat));
        }
        std::string value =  CommonUtils::convertVectorToString(ratPrefs, false);
        data.stateRootObj[MANAGER]["RATPreference"] = value;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    int phoneId = request->phone_id();
    auto f = std::async(std::launch::async, [this, phoneId, ratPrefs]() {
            std::string stateJsonPath = (phoneId == SLOT_1 ) ?
                "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";
            int domain = stoi(CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "ServiceDomainPreference"}));
            std::string gsmBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "gsmBands"});
            std::vector<int> gsmBands = CommonUtils::convertStringToVector(gsmBandsStr);
            std::string wcdmaBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "wcdmaBands"});
            std::vector<int> wcdmaBands = CommonUtils::convertStringToVector(wcdmaBandsStr);
            std::string lteBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "lteBands"});
            std::vector<int> lteBands = CommonUtils::convertStringToVector(lteBandsStr);
            std::string nsaBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "nsaBands"});
            std::vector<int> nsaBands = CommonUtils::convertStringToVector(nsaBandsStr);
            std::string saBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "saBands"});
            std::vector<int> saBands = CommonUtils::convertStringToVector(saBandsStr);
            this->triggerSystemSelectionPreferenceEvent(phoneId, ratPrefs, domain, gsmBands,
                wcdmaBands, lteBands, nsaBands, saBands);
        }).share();
    taskQ_->add(f);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestServiceDomainPreference(ServerContext* context,
    const ::telStub::RequestServiceDomainPreferenceRequest* request,
    telStub::RequestServiceDomainPreferenceReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestServiceDomainPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int serviceDomainPreference = data.stateRootObj[MANAGER]["ServiceDomainPreference"].asInt();
        response->set_service_domain_pref
        (static_cast<telStub::ServiceDomainPreference_Pref>(serviceDomainPreference));
    }

    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::SetServiceDomainPreference(ServerContext* context,
    const ::telStub::SetServiceDomainPreferenceRequest* request,
    telStub::SetServiceDomainPreferenceReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "setServiceDomainPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        ::telStub::ServiceDomainPreference_Pref pref = request->service_domain_pref();
        data.stateRootObj[MANAGER]["ServiceDomainPreference"] = static_cast<int>(pref);
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }
    int phoneId = request->phone_id();
    int domain = static_cast<int>(request->service_domain_pref());
    auto f = std::async(std::launch::async, [this, phoneId, domain]() {
            std::string stateJsonPath = (phoneId == SLOT_1 ) ?
                "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";
            std::string rats = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "RATPreference"});
            LOG(DEBUG, __FUNCTION__,"RAT string is ", rats);
            std::vector<int> raTdata = CommonUtils::convertStringToVector(rats);
            std::vector<uint8_t> raTs;
            for(int i : raTdata) {
                raTs.push_back(static_cast<uint8_t>(i));
            }
            std::string gsmBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "gsmBands"});
            std::vector<int> gsmBands = CommonUtils::convertStringToVector(gsmBandsStr);
            std::string wcdmaBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "wcdmaBands"});
            std::vector<int> wcdmaBands = CommonUtils::convertStringToVector(wcdmaBandsStr);
            std::string lteBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "lteBands"});
            std::vector<int> lteBands = CommonUtils::convertStringToVector(lteBandsStr);
            std::string nsaBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "nsaBands"});
            std::vector<int> nsaBands = CommonUtils::convertStringToVector(nsaBandsStr);
            std::string saBandsStr = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "BandPreference", "saBands"});
            std::vector<int> saBands = CommonUtils::convertStringToVector(saBandsStr);
            this->triggerSystemSelectionPreferenceEvent(phoneId, raTs, domain, gsmBands, wcdmaBands,
                lteBands, nsaBands, saBands);
        }).share();
    taskQ_->add(f);
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void ServingManagerServerImpl::triggerSystemSelectionPreferenceEvent(int slotId,
    std::vector<uint8_t> ratPrefs, int domain, std::vector<int> gsmBandPrefs,
    std::vector<int> wcdmaBandPrefs, std::vector<int> lteBandPrefs,
    std::vector<int> nsaBandPrefs, std::vector<int> saBandPrefs) {
    ::telStub::SystemSelectionPreferenceEvent systemSelectionPreference;
    ::eventService::EventResponse anyResponse;

    systemSelectionPreference.set_phone_id(slotId);
    for(auto &it : ratPrefs) {
         systemSelectionPreference.add_rat_pref_types(static_cast<telStub::RatPrefType>(it));
    }
    systemSelectionPreference.set_service_domain_pref
        (static_cast<telStub::ServiceDomainPreference_Pref>(domain));
    for(auto &g : gsmBandPrefs) {
         systemSelectionPreference.add_gsm_pref_bands(static_cast<telStub::GsmRFBand>(g));
    }
    for(auto &w : wcdmaBandPrefs) {
         systemSelectionPreference.add_wcdma_pref_bands(static_cast<telStub::WcdmaRFBand>(w));
    }
    for(auto &l : lteBandPrefs) {
         systemSelectionPreference.add_lte_pref_bands(static_cast<telStub::LteRFBand>(l));
    }
    for(auto &n : nsaBandPrefs) {
         systemSelectionPreference.add_nsa_pref_bands(static_cast<telStub::NrRFBand>(n));
    }
    for(auto &s: saBandPrefs) {
         systemSelectionPreference.add_sa_pref_bands(static_cast<telStub::NrRFBand>(s));
    }
    anyResponse.set_filter("tel_serv_sel_pref");
    anyResponse.mutable_any()->PackFrom(systemSelectionPreference);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

grpc::Status ServingManagerServerImpl::GetDcStatus(ServerContext* context,
    const ::telStub::GetDcStatusRequest* request,
    telStub::GetDcStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string stateJsonPath = (request->phone_id() == SLOT_1 ) ?
        "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";

    int endcAvailability = stoi(CommonUtils::readSystemDataValue(stateJsonPath, "0",
            {"IServingSystemManager", "DcStatus", "endcAvailability"}));
    int dcnrRestriction = stoi(CommonUtils::readSystemDataValue(stateJsonPath, "0",
            {"IServingSystemManager", "DcStatus", "dcnrRestriction"}));
    response->set_endc_availability(
    static_cast<telStub::EndcAvailability_Status>(endcAvailability));
    response->set_dcnr_restriction
    (static_cast<telStub::DcnrRestriction_Status>(dcnrRestriction));
    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetSystemInfo(ServerContext* context,
    const ::telStub::GetSystemInfoRequest* request,
    telStub::GetSystemInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "getSystemInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int serviceDomainPreference =  data.stateRootObj[MANAGER]["ServingSystemInfo"]\
            ["domain"].asInt();
        response->set_current_domain
        (static_cast<telStub::ServiceDomainInfo_Domain>(serviceDomainPreference));
        int rat =  data.stateRootObj[MANAGER]["ServingSystemInfo"]["rat"].asInt();
        response->set_current_rat(static_cast<telStub::RadioTechnology>(rat));
        int state = data.stateRootObj[MANAGER]["ServingSystemInfo"]\
            ["registrationState"].asInt();
        response->set_current_state(static_cast<telStub::ServiceRegistrationState>(state));
    }
    //Create response
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestNetworkTime(ServerContext* context,
    const ::telStub::RequestNetworkTimeRequest* request,
    telStub::RequestNetworkTimeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestNetworkTime";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        telux::tel::NetworkTimeInfo result;
        result.year = data.stateRootObj[MANAGER]["NetworkTimeInfo"]["year"].asInt();
        result.month = data.stateRootObj[MANAGER]["NetworkTimeInfo"]["month"].asInt();
        result.day =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["day"].asInt();
        result.hour = data.stateRootObj[MANAGER]["NetworkTimeInfo"]["hour"].asInt();
        result.minute = data.stateRootObj[MANAGER]["NetworkTimeInfo"]["minute"].asInt();
        result.second =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["second"].asInt();
        result.dayOfWeek =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["dayOfWeek"].asInt();
        result.timeZone =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["timeZone"].asInt();
        result.dstAdj =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["dstAdj"].asInt();
        result.nitzTime =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["nitzTime"].asString();
        telStub::NetworkTimeInfo info;
        info.set_year(result.year);
        info.set_month(result.month);
        info.set_day(result.day);
        info.set_hour(result.hour);
        info.set_minute(result.minute);
        info.set_second(result.second);
        info.set_day_of_week(result.dayOfWeek);
        info.set_time_zone(result.timeZone);
        info.set_dst_adj(result.dstAdj);
        info.set_nitz_time(result.nitzTime);
        *response->mutable_network_time_info() = info;
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestRFBandInfo(ServerContext* context,
    const ::telStub::RequestRFBandInfoRequest* request,
    telStub::RequestRFBandInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestRFBandInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        response->set_band(static_cast<telStub::RFBand>(data.stateRootObj[MANAGER]["RFBandInfo"]\
            ["rFBand"].asInt()));
        response->set_band_width(static_cast<telStub::RFBandWidth>(data.stateRootObj[MANAGER]\
            ["RFBandInfo"]["bandwidth"].asInt()));
        response->set_channel(data.stateRootObj[MANAGER]["RFBandInfo"]["channel"].asInt());
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetNetworkRejectInfo(ServerContext* context,
    const ::telStub::GetNetworkRejectInfoRequest* request,
    telStub::GetNetworkRejectInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "getNetworkRejectInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int serviceDomain =  data.stateRootObj[MANAGER]["NetworkRejectInfo"]\
            ["ServingSystemInfo"]["domain"].asInt();
        response->set_reject_domain
        (static_cast<telStub::ServiceDomainInfo_Domain>(serviceDomain));
        int rat =  data.stateRootObj[MANAGER]["NetworkRejectInfo"]["ServingSystemInfo"]\
            ["rat"].asInt();
        response->set_reject_rat(static_cast<telStub::RadioTechnology>(rat));
        int rejectCause = data.stateRootObj[MANAGER]["NetworkRejectInfo"]\
            ["rejectCause"].asInt();
        response->set_reject_cause(rejectCause);
        std::string mcc = data.stateRootObj[MANAGER]["NetworkRejectInfo"]\
            ["mcc"].asString();
        response->set_mcc(mcc);
        std::string mnc = data.stateRootObj[MANAGER]["NetworkRejectInfo"]\
            ["mnc"].asString();
        response->set_mnc(mnc);
    }
    //Create response
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetCallBarringInfo(ServerContext* context,
    const ::telStub::GetCallBarringInfoRequest* request,
    telStub::GetCallBarringInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "getCallBarringInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int count = data.stateRootObj[MANAGER] ["CallBarringInfo"]["infoList"].size();
        for (int i = 0 ; i < count; i++) {
            telStub::CallBarringInfo *result = response->add_barring_infos();
            Json::Value requestedInfo =
                data.stateRootObj[MANAGER] ["CallBarringInfo"]["infoList"][i];
            int rat = requestedInfo["rat"].asInt();
            result->set_rat(static_cast<telStub::RadioTechnology>(rat));
            int serviceDomain = requestedInfo["domain"].asInt();
            result->set_domain
                (static_cast<telStub::ServiceDomainInfo_Domain>(serviceDomain));
            int callType = requestedInfo["callType"].asInt();
            result->set_call_type
                (static_cast<telStub::CallsAllowedInCell_Type>(callType));
        }
    }
    //Create response
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetSmsCapabilityOverNetwork(ServerContext* context,
    const ::telStub::GetSmsCapabilityOverNetworkRequest* request,
    telStub::GetSmsCapabilityOverNetworkReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "getSmsCapabilityOverNetwork";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int domain = data.stateRootObj[MANAGER]["SmsCapability"]["domain"].asInt();
        response->set_domain(static_cast<telStub::SmsDomain>(domain));
        int rat = data.stateRootObj[MANAGER]["SmsCapability"]["rat"].asInt();
        response->set_rat(static_cast<telStub::RadioTechnology>(rat));
        int smsStatus = data.stateRootObj[MANAGER]["SmsCapability"]["ntnSmsStatus"].asInt();
        response->set_sms_status(static_cast<telStub::NtnSmsStatus>(smsStatus));
    }
    //Create response
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetLteCsCapability(ServerContext* context,
    const ::telStub::GetLteCsCapabilityRequest* request,
    telStub::GetLteCsCapabilityReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "getLteCsCapability";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int capability =  data.stateRootObj[MANAGER]["LteCsCapability"].asInt();
        response->set_capability
        (static_cast<telStub::LteCsCapability>(capability));
    }
    //Create response
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestRFBandPreferences(ServerContext* context,
    const ::telStub::RequestRFBandPreferencesRequest* request,
    telStub::RequestRFBandPreferencesReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestRFBandPreferences";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if(data.status == telux::common::Status::SUCCESS) {
        std::string gsmBandsStr = data.stateRootObj[MANAGER]["BandPreference"]\
            ["gsmBands"].asString();
        std::vector<int> gsmBands = CommonUtils::convertStringToVector(gsmBandsStr);
        std::string wcdmaBandsStr = data.stateRootObj[MANAGER]["BandPreference"]\
            ["wcdmaBands"].asString();
        std::vector<int> wcdmaBands = CommonUtils::convertStringToVector(wcdmaBandsStr);
        std::string lteBandsStr = data.stateRootObj[MANAGER]["BandPreference"]\
            ["lteBands"].asString();
        std::vector<int> lteBands = CommonUtils::convertStringToVector(lteBandsStr);
        std::string nsaBandsStr = data.stateRootObj[MANAGER]["BandPreference"]\
            ["nsaBands"].asString();
        std::vector<int> nsaBands = CommonUtils::convertStringToVector(nsaBandsStr);
        std::string saBandsStr = data.stateRootObj[MANAGER]["BandPreference"]\
            ["saBands"].asString();
        std::vector<int> saBands = CommonUtils::convertStringToVector(saBandsStr);
        for(auto &g : gsmBands) {
             response->add_gsm_pref_bands(static_cast<telStub::GsmRFBand>(g));
        }
        for(auto &w : wcdmaBands) {
             response->add_wcdma_pref_bands(static_cast<telStub::WcdmaRFBand>(w));
        }
        for(auto &l : lteBands) {
             response->add_lte_pref_bands(static_cast<telStub::LteRFBand>(l));
        }
        for(auto &n : nsaBands) {
             response->add_nsa_pref_bands(static_cast<telStub::NrRFBand>(n));
        }
        for(auto &s: saBands) {
             response->add_sa_pref_bands(static_cast<telStub::NrRFBand>(s));
        }
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::SetRFBandPreferences(ServerContext* context,
    const ::telStub::SetRFBandPreferencesRequest* request,
    telStub::SetRFBandPreferencesReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "setRFBandPreferences";
    JsonData data;
    std::vector<int> gsmBands   = {};
    std::vector<int> wcdmaBands = {};
    std::vector<int> lteBands   = {};
    std::vector<int> saBands    = {};
    std::vector<int> nsaBands   = {};
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        for (auto &g : request->gsm_pref_bands()) {
            gsmBands.push_back(static_cast<int>(g));
        }
        for (auto &w : request->wcdma_pref_bands()) {
            wcdmaBands.push_back(static_cast<int>(w));
        }
        for (auto &l : request->lte_pref_bands()) {
            lteBands.push_back(static_cast<int>(l));
        }
        for (auto &n : request->nsa_pref_bands()) {
            nsaBands.push_back(static_cast<int>(n));
        }
        for (auto &s : request->sa_pref_bands()) {
            saBands.push_back(static_cast<int>(s));
        }
        std::string gsmBandValue = CommonUtils::convertIntVectorToString(gsmBands);
        std::string wcdmaBandValue = CommonUtils::convertIntVectorToString(wcdmaBands);
        std::string lteBandValue = CommonUtils::convertIntVectorToString(lteBands);
        std::string nsaBandValue = CommonUtils::convertIntVectorToString(nsaBands);
        std::string saBandValue = CommonUtils::convertIntVectorToString(saBands);
        data.stateRootObj[MANAGER]["BandPreference"]["gsmBands"]
            = gsmBandValue;
        data.stateRootObj[MANAGER]["BandPreference"]["wcdmaBands"]
            = wcdmaBandValue;
        data.stateRootObj[MANAGER]["BandPreference"]["lteBands"]
            = lteBandValue;
        data.stateRootObj[MANAGER]["BandPreference"]["nsaBands"]
            = nsaBandValue;
        data.stateRootObj[MANAGER]["BandPreference"]["saBands"]
            = saBandValue;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    int phoneId = request->phone_id();
    auto f = std::async(std::launch::async, [this, phoneId, gsmBands,
                wcdmaBands, lteBands, nsaBands, saBands]() {
            std::string stateJsonPath = (phoneId == SLOT_1 ) ?
                "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";
            int domain = stoi(CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "ServiceDomainPreference"}));
            std::string rats = CommonUtils::readSystemDataValue(stateJsonPath, "0",
                {"IServingSystemManager", "RATPreference"});
            LOG(DEBUG, __FUNCTION__,"RAT string is ", rats);
            std::vector<int> raTdata = CommonUtils::convertStringToVector(rats);
            std::vector<uint8_t> raTs;
            for(int i : raTdata) {
                raTs.push_back(static_cast<uint8_t>(i));
            }
            this->triggerSystemSelectionPreferenceEvent(phoneId, raTs, domain, gsmBands,
                wcdmaBands, lteBands, nsaBands, saBands);
        }).share();
    taskQ_->add(f);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestRFBandCapability(ServerContext* context,
    const ::telStub::RequestRFBandCapabilityRequest* request,
    telStub::RequestRFBandCapabilityReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestRFBandCapability";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if(data.status == telux::common::Status::SUCCESS) {
        std::string gsmBandsStr = data.stateRootObj[MANAGER]["BandCapability"]\
            ["gsmBands"].asString();
        std::vector<int> gsmBands = CommonUtils::convertStringToVector(gsmBandsStr);
        std::string wcdmaBandsStr = data.stateRootObj[MANAGER]["BandCapability"]\
            ["wcdmaBands"].asString();
        std::vector<int> wcdmaBands = CommonUtils::convertStringToVector(wcdmaBandsStr);
        std::string lteBandsStr = data.stateRootObj[MANAGER]["BandCapability"]\
            ["lteBands"].asString();
        std::vector<int> lteBands = CommonUtils::convertStringToVector(lteBandsStr);
        std::string nrBandsStr = data.stateRootObj[MANAGER]["BandCapability"]\
            ["nrBands"].asString();
        std::vector<int> nrBands = CommonUtils::convertStringToVector(nrBandsStr);
        for(auto &g : gsmBands) {
             response->add_gsm_capability_bands(static_cast<telStub::GsmRFBand>(g));
        }
        for(auto &w : wcdmaBands) {
             response->add_wcdma_capability_bands(static_cast<telStub::WcdmaRFBand>(w));
        }
        for(auto &l : lteBands) {
             response->add_lte_capability_bands(static_cast<telStub::LteRFBand>(l));
        }
        for(auto &n : nrBands) {
             response->add_nr_capability_bands(static_cast<telStub::NrRFBand>(n));
        }
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void ServingManagerServerImpl::triggerChangeEvent(::eventService::EventResponse anyResponse) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void ServingManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__,"String is ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__,"Token is ", token );
    if ( SYSTEM_SELECTION_PREFERENCE == token) {
        handleSystemSelectionPreferenceChanged(event);
    } else if( SYSTEM_INFO == token) {
        handleSystemInfoUpdateEvent(event);
    } else if( NETWORK_TIME == token) {
        handleNetworkTimeUpdateEvent(event);
    } else if( RF_BAND_INFO == token) {
        handleRfBandInfoUpdateEvent(event);
    } else if( NETWORK_REJECTION == token) {
        handleNetworkRejectionUpdateEvent(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void ServingManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == telux::tel::TEL_SERVING_SYSTEM_FILTER) {
        onEventUpdate(message.event());
    }
}

void ServingManagerServerImpl::handleNetworkRejectionUpdateEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int slotId;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    try {
        if(token == "") {
            LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
            slotId = 1;
        } else {
            slotId = std::stoi(token);
        }
        if((slotId == SLOT_2) && (!(telux::common::DeviceConfig::isMultiSimSupported()))) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
        LOG(DEBUG, __FUNCTION__, "The Slot id is: ", slotId ,
            " leftover string is: ", eventParams);
        // Fetch rejectSrvInfoRat
        int rejectSrvInfoRat;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " Rat is not passed ");
            rejectSrvInfoRat = 0;
        } else {
            rejectSrvInfoRat = std::stoi(token);
        }

        // Fetch rejectSrvInfoDomain
        int rejectSrvInfoDomain;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, "Storage type not passed, assuming UNKNOWN");
            rejectSrvInfoDomain = 0;
        } else {
            rejectSrvInfoDomain = std::stoi(token);
        }
        // Fetch rejectCause
        int rejectCause;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " rejectCause not passed ");
            rejectCause = 0;
        } else {
            rejectCause = std::stoi(token);
        }

        // Fetch mcc
        std::string mcc;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " mcc type not passed ");
            mcc = "";
        } else {
            mcc = token;
        }

        // Fetch mnc
        std::string mnc;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " mnc type not passed ");
            mnc = "";
        } else {
            mnc = token;
        }

    LOG(INFO, __FUNCTION__, " rejectSrvInfoRat is ", rejectSrvInfoRat, " rejectCause is ",
        rejectCause, " rejectSrvInfoDomain is ", rejectSrvInfoDomain, " mcc is ", mcc,
        " mnc is ", mnc);

    std::string stateJsonPath = (slotId == SLOT_1 ) ?
        "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";

    CommonUtils::writeSystemDataValue<int>(stateJsonPath, rejectSrvInfoRat,
            {"IServingSystemManager", "NetworkRejectInfo", "ServingSystemInfo" , "rat"});
    CommonUtils::writeSystemDataValue<int>(stateJsonPath, rejectSrvInfoDomain,
            {"IServingSystemManager", "NetworkRejectInfo", "ServingSystemInfo", "domain"});
    CommonUtils::writeSystemDataValue<int>(stateJsonPath, rejectCause,
            {"IServingSystemManager", "NetworkRejectInfo", "rejectCause"});
    CommonUtils::writeSystemDataValue<std::string>(stateJsonPath, mcc,
            {"IServingSystemManager", "NetworkRejectInfo", "mcc"});
    CommonUtils::writeSystemDataValue<std::string>(stateJsonPath, mnc,
            {"IServingSystemManager", "NetworkRejectInfo", "mnc"});


    ::telStub::NetworkRejectInfoEvent networkRejectInfoEvent;
    ::eventService::EventResponse anyResponse;

    networkRejectInfoEvent.set_phone_id(slotId);
    networkRejectInfoEvent.set_reject_rat
        (static_cast<telStub::RadioTechnology>(rejectSrvInfoRat));
    networkRejectInfoEvent.set_reject_domain
        (static_cast<telStub::ServiceDomainInfo_Domain>(rejectSrvInfoDomain));
    networkRejectInfoEvent.set_reject_cause(rejectCause);
    networkRejectInfoEvent.set_mcc(mcc);
    networkRejectInfoEvent.set_mnc(mnc);
    anyResponse.set_filter("tel_serv_network_reject_info");
    anyResponse.mutable_any()->PackFrom(networkRejectInfoEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
    }
}

void ServingManagerServerImpl::handleRfBandInfoUpdateEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int slotId;
    try {
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
            slotId = 1;
        } else {
            slotId = std::stoi(token);
        }
        if((slotId == SLOT_2) && (!(telux::common::DeviceConfig::isMultiSimSupported()))) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
        LOG(DEBUG, __FUNCTION__, "The Slot id is: ", slotId ,
            " leftover string is: ", eventParams);
        // Fetch band
        int band;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " band not passed" );
            band = 0;
        } else {
            band = std::stoi(token);
        }
        // Fetch channel
        int channel;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " Channel type not passed");
            channel = 0;
        } else {
            channel = std::stoi(token);
        }

        // Fetch bandWidth
        int bandWidth;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " bandWidth not passed ");
            bandWidth = 0;
        } else {
            bandWidth = std::stoi(token);
        }

        LOG(INFO, __FUNCTION__, " band is ", band, " channel is ", channel,
            " bandWidth is ", bandWidth);

        std::string stateJsonPath = (slotId == SLOT_1 ) ?
            "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";

        CommonUtils::writeSystemDataValue<int>(stateJsonPath, band,
                {"IServingSystemManager", "RFBandInfo", "rFBand"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, channel,
                {"IServingSystemManager", "RFBandInfo", "channel"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, bandWidth,
                {"IServingSystemManager", "RFBandInfo", "bandwidth"});

        ::telStub::RFBandInfoEvent rFBandInfoEvent;
        ::eventService::EventResponse anyResponse;

        rFBandInfoEvent.set_phone_id(slotId);
        rFBandInfoEvent.set_band(static_cast<telStub::RFBand>(band));
        rFBandInfoEvent.set_channel(channel);
        rFBandInfoEvent.set_band_width(static_cast<telStub::RFBandWidth>(bandWidth));
        anyResponse.set_filter("tel_serv_rf_band_info");
        anyResponse.mutable_any()->PackFrom(rFBandInfoEvent);
        //posting the event to EventService event queue
        auto& eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
    }
}

std::vector<int> ServingManagerServerImpl::readBandPreferenceFromEvent(std::string eventParams) {
    std::vector<int> bandPrefs = {};
    std::string band = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    while(!band.empty()) {
       bandPrefs.emplace_back(stoi(band));
       band = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    }
    return bandPrefs;
}

void ServingManagerServerImpl::handleSystemSelectionPreferenceChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int slotId;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    try {
        if(token == "") {
            LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
            slotId = 1;
        } else {
            slotId = std::stoi(token);
        }
        if((slotId == SLOT_2) && (!(telux::common::DeviceConfig::isMultiSimSupported()))) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
        LOG(DEBUG, __FUNCTION__, "The Slot id is: ", slotId ,
            " leftover string is: ", eventParams);

        // Fetch serviceDomainPreference
        int domain;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " serviceDomainPreference not passed ");
            domain = -1; // UNKNOWN
        } else {
            domain = std::stoi(token);
        }
        LOG(INFO, __FUNCTION__, "domain is ", domain);
        std::vector<uint8_t> ratPrefs;
        std::string ratPref;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        ratPref = token;
        if(token == "") {
            LOG(INFO, __FUNCTION__, " Rat preference not passed");
            ratPref = "0"; // PREF_CDMA_1X
        } else {
            LOG(INFO, __FUNCTION__, " Rat is ", ratPref);
        }
        int lengthOfRatPreferenceInput = ratPref.size();
        reverse(ratPref.begin(), ratPref.end());
        int reverseRat = stoi(ratPref);
        for(int i = 0; i < lengthOfRatPreferenceInput; i++ ) {

            ratPrefs.emplace_back(static_cast<uint8_t>((reverseRat%10)));
            reverseRat = reverseRat/10;
        }
        std::string value = CommonUtils::convertVectorToString(ratPrefs, false);
        LOG(INFO, __FUNCTION__, "Rat data for json file  is ", value);

        // Split the event string into parameters( for ... ,gsmBands ,wcdmaBands ,lteBands ...)
        // based on delimeter as ","
        std::stringstream ss(eventParams);
        std::vector<string> params;
        while (getline(ss, eventParams, ',')) {
            params.emplace_back(eventParams);
        }
        for(std::string str:params) {
            LOG(DEBUG, __FUNCTION__," Param: ", str);
        }

        std::vector<int> gsmBandPrefs = readBandPreferenceFromEvent(params[1]);
        std::vector<int> wcdmaBandPrefs = readBandPreferenceFromEvent(params[2]);
        std::vector<int> lteBandPrefs = readBandPreferenceFromEvent(params[3]);
        std::vector<int> nsaBandPrefs = readBandPreferenceFromEvent(params[4]);
        std::vector<int> saBandPrefs = readBandPreferenceFromEvent(params[5]);
        std::string gsmBandValue = CommonUtils::convertIntVectorToString(gsmBandPrefs);
        std::string wcdmaBandValue = CommonUtils::convertIntVectorToString(wcdmaBandPrefs);
        std::string lteBandValue = CommonUtils::convertIntVectorToString(lteBandPrefs);
        std::string nsaBandValue = CommonUtils::convertIntVectorToString(nsaBandPrefs);
        std::string saBandValue = CommonUtils::convertIntVectorToString(saBandPrefs);
        LOG(INFO, __FUNCTION__, "Band data for json file is: gsm bands ", gsmBandValue,
            ", wcdma bands ", wcdmaBandValue, ", lte bands ", lteBandValue,
            ", nsa bands ", nsaBandValue, ", sa bands ", saBandValue);

        std::string stateJsonPath = (slotId == SLOT_1 ) ?
            "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";

        CommonUtils::writeSystemDataValue<std::string>(stateJsonPath, value,
                {"IServingSystemManager", "RATPreference"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, domain,
                {"IServingSystemManager", "ServiceDomainPreference"});
        CommonUtils::writeSystemDataValue<std::string>(stateJsonPath, gsmBandValue,
                {"IServingSystemManager", "BandPreference", "gsmBands"});
        CommonUtils::writeSystemDataValue<std::string>(stateJsonPath, wcdmaBandValue,
                {"IServingSystemManager", "BandPreference", "wcdmaBands"});
        CommonUtils::writeSystemDataValue<std::string>(stateJsonPath, lteBandValue,
                {"IServingSystemManager", "BandPreference", "lteBands"});
        CommonUtils::writeSystemDataValue<std::string>(stateJsonPath, nsaBandValue,
                {"IServingSystemManager", "BandPreference", "nsaBands"});
        CommonUtils::writeSystemDataValue<std::string>(stateJsonPath, saBandValue,
                {"IServingSystemManager", "BandPreference", "saBands"});

        triggerSystemSelectionPreferenceEvent(slotId, ratPrefs, domain, gsmBandPrefs,
            wcdmaBandPrefs, lteBandPrefs, nsaBandPrefs, saBandPrefs);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
    }
}

void ServingManagerServerImpl::handleSystemInfoUpdateEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int slotId;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    try {
        if(token == "") {
            LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
            slotId = 1;
        } else {
            slotId = std::stoi(token);
        }
        if((slotId == SLOT_2) && (!(telux::common::DeviceConfig::isMultiSimSupported()))) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
        LOG(DEBUG, __FUNCTION__, "The Slot id is: ", slotId
            , " leftover string is: ", eventParams);
        // Fetch currentServingRat
        int currentServingRat;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " currentServingRat not passed ");
            currentServingRat = 0; // PREF_CDMA_1X
        } else {
            currentServingRat = std::stoi(token);
        }
        // Fetch currentServingDomain
        int currentServingDomain;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " currentServingDomain not passed");
            currentServingDomain = -1;
        } else {
            currentServingDomain = std::stoi(token);
        }

        // Fetch currentRegistrationState
        int currentRegistrationState;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if (token == "") {
            LOG(INFO, __FUNCTION__, " currentRegistrationState not passed");
            currentRegistrationState = -1;
        } else {
            currentRegistrationState = std::stoi(token);
        }
        if (currentRegistrationState
            < (static_cast<int>(telStub::ServiceRegistrationState::REG_UNKNOWN)) ||
            currentRegistrationState
            > (static_cast<int>(telStub::ServiceRegistrationState::REG_POWER_SAVE))) {
            LOG(ERROR, " invalid currentRegistrationState");
            return;
        }
        // Fetch endcAvailability
        int endcAvailability;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " endcAvailability not passed");
            endcAvailability = -1;
        } else {
            endcAvailability = std::stoi(token);
        }

        // Fetch dcnrRestriction
        int dcnrRestriction;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " dcnrRestriction not passed");
            dcnrRestriction = -1; // UNKNOWN
        } else {
            dcnrRestriction = std::stoi(token);

        }

        // Fetch smsRat
        int smsRat;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " smsRat not passed");
            smsRat = 0; // UNKNOWN
        } else {
            smsRat = std::stoi(token);
        }
        if (smsRat < (static_cast<int>(telStub::RadioTechnology ::RADIO_TECH_UNKNOWN)) ||
            smsRat > (static_cast<int>(telStub::RadioTechnology ::RADIO_TECH_NB1_NTN))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for SMS radio technology ");
            return;
        }

        // Fetch smsDomain
        int smsDomain;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " smsDomain not passed");
            smsDomain = -1; // UNKNOWN
        } else {
            smsDomain = std::stoi(token);
        }
        if (smsDomain < (static_cast<int>(telStub::SmsDomain::UNKNOWN_DOMAIN)) ||
            smsDomain > (static_cast<int>(telStub::SmsDomain::SMS_ON_3GPP))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for SMS domain ");
            return;
        }

        // fetch ntnSmsStatus
        int ntnSmsStatus;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if (token == "") {
            LOG(INFO, __FUNCTION__, " ntnSmsStatus not passed");
            ntnSmsStatus = -1; // UNKNOWN
        } else {
            ntnSmsStatus = std::stoi(token);
        }
        if (ntnSmsStatus < (static_cast<int>(telStub::NtnSmsStatus::SMS_UNKNOWN)) ||
            ntnSmsStatus > (static_cast<int>(telStub::NtnSmsStatus::SMS_AVAILABLE))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for ntnSmsStatus");
            return;
        }

        // Fetch LteCapability
        int lteCapability;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " lteCapability not passed");
            lteCapability = -1; // UNKNOWN
        } else {
            lteCapability = std::stoi(token);
        }
        if (lteCapability < (static_cast<int>(telStub::LteCsCapability::UNKNOWN_SERVICE)) ||
            lteCapability > (static_cast<int>(telStub::LteCsCapability::BARRED))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for LTE CS capability ");
            return;
        }

        LOG(INFO, __FUNCTION__, " Rat is ", currentServingRat , " Domain is ",
            currentServingDomain, " currentRegistrationState is ", currentRegistrationState,
            " EndcAvailability is ", endcAvailability, " DcnrRestriction is ", dcnrRestriction,
            " SmsRat is ", smsRat, " SmsDomain is ", smsDomain, " NtnSmsStatus is ", ntnSmsStatus,
            " LteCapability is ", lteCapability);

        std::string stateJsonPath = (slotId == SLOT_1 ) ?
            "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";

        CommonUtils::writeSystemDataValue<int>(stateJsonPath, currentServingRat,
                {"IServingSystemManager", "ServingSystemInfo", "rat"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, currentServingDomain,
                {"IServingSystemManager", "ServingSystemInfo", "domain"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, currentRegistrationState,
                {"IServingSystemManager", "ServingSystemInfo", "registrationState"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, endcAvailability,
                {"IServingSystemManager", "DcStatus", "endcAvailability"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, dcnrRestriction,
                {"IServingSystemManager", "DcStatus", "dcnrRestriction"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, smsRat,
                {"IServingSystemManager", "SmsCapability", "rat"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, smsDomain,
                {"IServingSystemManager", "SmsCapability", "domain"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, ntnSmsStatus,
                {"IServingSystemManager", "SmsCapability", "ntnSmsStatus"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, lteCapability,
                {"IServingSystemManager", "LteCsCapability"});

        ::telStub::SystemInfoEvent systemInfoEvent;
        systemInfoEvent.set_phone_id(slotId);
        systemInfoEvent.set_current_rat(static_cast<telStub::RadioTechnology>(currentServingRat));
        systemInfoEvent.set_current_domain
            (static_cast<telStub::ServiceDomainInfo_Domain>(currentServingDomain));
        systemInfoEvent.set_current_state
            (static_cast<telStub::ServiceRegistrationState>(currentRegistrationState));
        systemInfoEvent.set_endc_availability
            (static_cast<telStub::EndcAvailability_Status>(endcAvailability));
        systemInfoEvent.set_dcnr_restriction
            (static_cast<telStub::DcnrRestriction_Status>(dcnrRestriction));
        systemInfoEvent.set_sms_rat(static_cast<telStub::RadioTechnology>(smsRat));
        systemInfoEvent.set_sms_domain(static_cast<telStub::SmsDomain>(smsDomain));
        systemInfoEvent.set_sms_status(static_cast<telStub::NtnSmsStatus>(ntnSmsStatus));
        systemInfoEvent.set_lte_capability
            (static_cast<telStub::LteCsCapability>(lteCapability));

        // Split the event string into parameters( for ... ,BarringInfo1 ,BarringInfo2 ...)
        // based on delimeter as ","
        std::stringstream ss(eventParams);
        std::vector<string> params;
        while (getline(ss, eventParams, ',')) {
            params.emplace_back(eventParams);
        }
        for(std::string str:params) {
            LOG(DEBUG, __FUNCTION__," Param: ", str);
        }
        Json::Value rootObj;
        std::string jsonfilename = (slotId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return;
        }
        rootObj[MANAGER] ["CallBarringInfo"]["infoList"].clear();
        int jsonInfoCount = rootObj[MANAGER]["CallBarringInfo"]["infoList"].size();
        int newInfoCount = params.size() - 1;
        LOG(DEBUG, " jsonInfoCount ", jsonInfoCount , " newInfoCount ", newInfoCount);

        for (int i = 1; i <= newInfoCount; i++) {
            LOG(DEBUG, " Parsing Params:" , params[i]);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int rat = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Rat is: ", rat);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int domain = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Domain is: ", domain);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int callType = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " CallType is: ", callType);
            rootObj[MANAGER] ["CallBarringInfo"]["infoList"][i-1]["rat"] = rat;
            rootObj[MANAGER] ["CallBarringInfo"]["infoList"][i-1]["domain"] = domain;
            rootObj[MANAGER] ["CallBarringInfo"]["infoList"][i-1]["callType"] = callType;

            telStub::CallBarringInfo *result = systemInfoEvent.add_barring_infos();
            result->set_rat(static_cast<telStub::RadioTechnology>(rat));
            result->set_domain
                (static_cast<telStub::ServiceDomainInfo_Domain>(domain));
            result->set_call_type
                (static_cast<telStub::CallsAllowedInCell_Type>(callType));
        }
        JsonParser::writeToJsonFile(rootObj, jsonfilename);

        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter("tel_serv_sys_info");
        anyResponse.mutable_any()->PackFrom(systemInfoEvent);
        //posting the event to EventService event queue
        auto& eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
    }
}

void ServingManagerServerImpl::handleNetworkTimeUpdateEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int slotId;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    try {
        if(token == "") {
            LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
            slotId = 1;
        } else {
            try {
                slotId = std::stoi(token);
            } catch(exception const & ex) {
                LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            }
        }
        if((slotId == SLOT_2) && (!(telux::common::DeviceConfig::isMultiSimSupported()))) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
        LOG(DEBUG, __FUNCTION__, "The Slot id is: ", slotId ,
             " leftover string is: ", eventParams);
        // Fetch year
        int year;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " year not passed");
            year = 0;
        } else {
            year = std::stoi(token);
        }
        // Fetch month
        int month;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " month not passed");
            month = 0;
        } else {
            month = std::stoi(token);
        }
        // Fetch day
        int day;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " day not passed");
            day = 0;
        } else {
            day = std::stoi(token);
        }
        // Fetch hour
        int hour;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " hour not passed");
            hour = 0;
        } else {
            hour = std::stoi(token);
        }
        // Fetch minute
        int minute;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " minute not passed");
            minute = 0;
        } else {
            minute = std::stoi(token);
        }
        // Fetch second
        int second;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " second not passed");
            second = 0;
        } else {
            second = std::stoi(token);
        }
        // Fetch dayOfWeek
        int dayOfWeek;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " dayOfWeek not passed");
            dayOfWeek = 0;
        } else {
            dayOfWeek = std::stoi(token);
        }
        // Fetch timeZone
        int timeZone;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " timeZone not passed");
            timeZone = 0;
        } else {
            timeZone = std::stoi(token);
        }
        // Fetch dstAdj
        int dstAdj;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " dstAdj not passed");
            dstAdj = 0;
        } else {
            dstAdj = std::stoi(token);
        }

        // Fetch nitzTime
        std::string nitzTime;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(INFO, __FUNCTION__, " nitzTime not passed");
            nitzTime = "";
        } else {
            nitzTime = token;
        }

        LOG(INFO, __FUNCTION__, " year is ", year , " month is ", month
        , " day is ", day, " hour is ", hour, " minute is ", minute, " dayOfWeek is ", dayOfWeek,
        " timeZone is ", timeZone, " dstAdj is ", dstAdj, " nitzTime is ", nitzTime,
        " second is", second );

        std::string stateJsonPath = (slotId == SLOT_1 ) ?
            "tel/IServingSystemManagerStateSlot1" : "tel/IServingSystemManagerStateSlot2";

        CommonUtils::writeSystemDataValue<int>(stateJsonPath, year,
                {"IServingSystemManager", "NetworkTimeInfo", "year"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, month,
                {"IServingSystemManager", "NetworkTimeInfo", "month"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, day,
                {"IServingSystemManager", "NetworkTimeInfo", "day"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, hour,
                {"IServingSystemManager", "NetworkTimeInfo", "hour"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, minute,
                {"IServingSystemManager", "NetworkTimeInfo", "minute"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, second,
                {"IServingSystemManager", "NetworkTimeInfo", "second"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, dayOfWeek,
                {"IServingSystemManager", "NetworkTimeInfo", "dayOfWeek"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, timeZone,
                {"IServingSystemManager", "NetworkTimeInfo", "timeZone"});
        CommonUtils::writeSystemDataValue<int>(stateJsonPath, dstAdj,
                {"IServingSystemManager", "NetworkTimeInfo", "dstAdj"});
        CommonUtils::writeSystemDataValue<std::string>(stateJsonPath, nitzTime,
                {"IServingSystemManager", "NetworkTimeInfo", "nitzTime"});

        ::telStub::NetworkTimeInfoEvent networkTimeInfoEvent;
        ::eventService::EventResponse anyResponse;

        networkTimeInfoEvent.set_phone_id(slotId);
        networkTimeInfoEvent.set_year(year);
        networkTimeInfoEvent.set_month(month);
        networkTimeInfoEvent.set_day(day);
        networkTimeInfoEvent.set_hour(hour);
        networkTimeInfoEvent.set_minute(minute);
        networkTimeInfoEvent.set_second(second);
        networkTimeInfoEvent.set_day_of_week(dayOfWeek);
        networkTimeInfoEvent.set_time_zone(timeZone);
        networkTimeInfoEvent.set_dst_adj(dstAdj);
        networkTimeInfoEvent.set_nitz_time(nitzTime);
        anyResponse.set_filter("tel_serv_network_time");
        anyResponse.mutable_any()->PackFrom(networkTimeInfoEvent);
        //posting the event to EventService event queue
        auto& eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
    }
}
void ServingManagerServerImpl::onServerEvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::telStub::OperatingModeEvent>()) {
        LOG(DEBUG, __FUNCTION__, "Received Operating Mode Change Event");
    }
}
