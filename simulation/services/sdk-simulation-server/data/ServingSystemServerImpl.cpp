/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ServingSystemServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#define SERVING_SYSTEM_API_SLOT1_JSON "api/data/IServingSystemManagerSlot1.json"
#define SERVING_SYSTEM_API_SLOT2_JSON "api/data/IServingSystemManagerSlot2.json"
#define SERVING_SYSTEM_STATE_SLOT1_JSON "system-state/data/IServingSystemManagerStateSlot1.json"
#define SERVING_SYSTEM_STATE_SLOT2_JSON "system-state/data/IServingSystemManagerStateSlot2.json"

#define SLOT_2 2

ServingSystemServerImpl::ServingSystemServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

ServingSystemServerImpl::~ServingSystemServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status ServingSystemServerImpl::InitService(ServerContext* context,
    const dataStub::SlotInfo* request, dataStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;
    std::string filePath = (request->slot_id() == SLOT_2)? SERVING_SYSTEM_API_SLOT2_JSON
        : SERVING_SYSTEM_API_SLOT1_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IServingSystemManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IServingSystemManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status ServingSystemServerImpl::GetDrbStatus(ServerContext* context,
    const dataStub::GetDrbStatusRequest* request,
    dataStub::GetDrbStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->drb_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_API_SLOT2_JSON : SERVING_SYSTEM_API_SLOT1_JSON;
    std::string stateJsonPath = (request->drb_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_STATE_SLOT2_JSON : SERVING_SYSTEM_STATE_SLOT1_JSON;
    std::string subsystem = "IServingSystemManager";
    std::string method = "getDrbStatus";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        response->mutable_drb_status()->set_drb_status(convertDrbStatusStringToEnum(
            data.stateRootObj[subsystem][method]["drbStatus"].asString()));
    }

    return grpc::Status::OK;
}

grpc::Status ServingSystemServerImpl::RequestServiceStatus(ServerContext* context,
    const dataStub::ServingStatusRequest* request,
    dataStub::ServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->serving_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_API_SLOT2_JSON : SERVING_SYSTEM_API_SLOT1_JSON;
    std::string stateJsonPath = (request->serving_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_STATE_SLOT2_JSON : SERVING_SYSTEM_STATE_SLOT1_JSON;
    std::string subsystem = "IServingSystemManager";
    std::string method = "requestServiceStatus";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        response->mutable_data_service_state()->set_data_service_state(
            convertServiceStateStringToEnum(
                data.stateRootObj[subsystem][method]["serviceState"].asString()));
        response->mutable_network_rat()->set_network_rat(convertNetworkRatStringToEnum(
            data.stateRootObj[subsystem][method]["networkRat"].asString()));
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status ServingSystemServerImpl::RequestRoamingStatus(ServerContext* context,
    const dataStub::RoamingStatusRequest* request,
    dataStub::RomingStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->roaming_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_API_SLOT2_JSON : SERVING_SYSTEM_API_SLOT1_JSON;
    std::string stateJsonPath = (request->roaming_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_STATE_SLOT2_JSON : SERVING_SYSTEM_STATE_SLOT1_JSON;
    std::string subsystem = "IServingSystemManager";
    std::string method = "requestRoamingStatus";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        response->mutable_roaming_type()->set_roaming_type(convertRoamingTypeStringToEnum(
            data.stateRootObj[subsystem][method]["type"].asString()));
        response->set_is_roaming(data.stateRootObj[subsystem][method]["isRoaming"].asBool());
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status ServingSystemServerImpl::RequestNrIconType(ServerContext* context,
    const dataStub::NrIconTypeRequest* request,
    dataStub::NrIconTypeReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->nr_icon_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_API_SLOT2_JSON : SERVING_SYSTEM_API_SLOT1_JSON;
    std::string stateJsonPath = (request->nr_icon_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_STATE_SLOT2_JSON : SERVING_SYSTEM_STATE_SLOT1_JSON;
    std::string subsystem = "IServingSystemManager";
    std::string method = "requestNrIconType";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        response->mutable_nr_icon_type()->set_nr_icon_type(convertNrIconTypeStringToEnum(
            data.stateRootObj[subsystem][method]["type"].asString()));
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status ServingSystemServerImpl::MakeDormant(ServerContext* context,
    const dataStub::MakeDormantStatusRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->make_dormant_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_API_SLOT2_JSON : SERVING_SYSTEM_API_SLOT1_JSON;
    std::string stateJsonPath = (request->make_dormant_status().slot_id() == SLOT_2)?
        SERVING_SYSTEM_STATE_SLOT2_JSON : SERVING_SYSTEM_STATE_SLOT1_JSON;
    std::string subsystem = "IServingSystemManager";
    std::string method = "makeDormant";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

dataStub::DrbStatus::Status ServingSystemServerImpl::convertDrbStatusStringToEnum(
    std::string status) {

    if (status == "ACTIVE") {
        return ::dataStub::DrbStatus::ACTIVE;
    } else if (status == "DORMANT") {
        return ::dataStub::DrbStatus::DORMANT;
    } else if (status == "UNKNOWN") {
        return ::dataStub::DrbStatus::UNKNOWN;
    }

    return ::dataStub::DrbStatus::UNKNOWN;
}

dataStub::DataServiceState::ServiceState ServingSystemServerImpl::convertServiceStateStringToEnum(
    std::string serviceState) {

    if (serviceState == "OUT_OF_SERVICE") {
        return ::dataStub::DataServiceState::OUT_OF_SERVICE;
    } else if (serviceState == "IN_SERVICE") {
        return ::dataStub::DataServiceState::IN_SERVICE;
    } else if (serviceState == "UNKNOWN") {
        return ::dataStub::DataServiceState::UNKNOWN;
    }

    return ::dataStub::DataServiceState::UNKNOWN;
}

dataStub::NetworkRat::Rat ServingSystemServerImpl::convertNetworkRatStringToEnum(
    std::string nwRat) {

    if (nwRat == "CDMA_1X") {
        return ::dataStub::NetworkRat::CDMA_1X;
    } else if (nwRat == "CDMA_EVDO") {
        return ::dataStub::NetworkRat::CDMA_EVDO;
    } else if (nwRat == "GSM") {
        return ::dataStub::NetworkRat::GSM;
    } else if (nwRat == "WCDMA") {
        return ::dataStub::NetworkRat::WCDMA;
    } else if (nwRat == "LTE") {
        return ::dataStub::NetworkRat::LTE;
    } else if (nwRat == "TDSCDMA") {
        return ::dataStub::NetworkRat::TDSCDMA;
    } else if (nwRat == "NR5G") {
        return ::dataStub::NetworkRat::NR5G;
    }

    return ::dataStub::NetworkRat::UNKNOWN;
}

dataStub::RoamingType::Type ServingSystemServerImpl::convertRoamingTypeStringToEnum(
    std::string type) {

    if (type == "DOMESTIC") {
        return ::dataStub::RoamingType::DOMESTIC;
    } else if (type == "INTERNATIONAL") {
        return ::dataStub::RoamingType::INTERNATIONAL;
    } else if (type == "UNKNOWN") {
        return ::dataStub::RoamingType::UNKNOWN;
    }

    return ::dataStub::RoamingType::UNKNOWN;
}

dataStub::NrIconType::Type ServingSystemServerImpl::convertNrIconTypeStringToEnum(
    std::string type) {

    if (type == "BASIC") {
        return ::dataStub::NrIconType::BASIC;
    } else if (type == "UWB") {
        return ::dataStub::NrIconType::UWB;
    } else if (type == "NONE") {
        return ::dataStub::NrIconType::NONE;
    }

    return ::dataStub::NrIconType::NONE;
}