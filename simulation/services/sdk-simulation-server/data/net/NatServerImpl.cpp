/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "NatServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/data/DataUtilsStub.hpp"
#include "libs/data/DataHelper.hpp"

#define NAT_MANAGER_API_LOCAL_JSON "api/data/INatManagerLocal.json"
#define NAT_MANAGER_STATE_JSON "system-state/data/INatManagerState.json"

#define REMOTE 1

NatServerImpl::NatServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

NatServerImpl::~NatServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status NatServerImpl::InitService(ServerContext* context,
    const dataStub::InitRequest* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = NAT_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["INatManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["INatManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status NatServerImpl::AddStaticNatEntry(ServerContext* context,
    const dataStub::StaticNatRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = NAT_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = NAT_MANAGER_STATE_JSON;
    std::string subsystem = "INatManager";
    std::string method = "addStaticNatEntry";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->static_nat_entry().operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if ((!DataUtilsStub::isValidIpv4Address(
        request->static_nat_entry().nat_config().address()))
        && (!DataUtilsStub::isValidIpv6Address(
        request->static_nat_entry().nat_config().address()))) {

        LOG(ERROR, __FUNCTION__, " Address provided shall be in either IPv4 or Ipv6 format");
        data.error = telux::common::ErrorCode::INTERNAL;
    }

    if (!telux::data::DataHelper::isValidProtocol(DataUtilsStub::stringToProtocol(
        request->static_nat_entry().nat_config().ip_protocol()))) {
        LOG(ERROR, __FUNCTION__, " unexpected protocol");
        data.error = telux::common::ErrorCode::INTERNAL;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        int entryIdx = -1, currentEntryCount = 0, backhaul = 0;
        auto bh_info = request->static_nat_entry().backhaul_type();
        if (bh_info == ::dataStub::BackhaulPreference::PREF_WWAN) {
            backhaul = WWAN_BH_IDX;
        } else if (bh_info == ::dataStub::BackhaulPreference::PREF_ETH) {
            backhaul = ETH_BH_IDX;
        }

        currentEntryCount = data.stateRootObj[subsystem][backhaul]["snatEntries"].size();
        bool entryExists = isNatEntryAvailable(subsystem, data, request, entryIdx);

        Json::Value newSnatEntry;
        if (!entryExists) {
            if (bh_info == ::dataStub::BackhaulPreference::PREF_WWAN) {
                newSnatEntry["profileId"] = request->static_nat_entry().profile_id();
                newSnatEntry["slotId"] = request->static_nat_entry().slot_id();
            } else if (bh_info == ::dataStub::BackhaulPreference::PREF_ETH) {
                newSnatEntry["vlanId"] = request->static_nat_entry().vlan_id();
            }
            newSnatEntry["addr"] = request->static_nat_entry().nat_config().address();
            newSnatEntry["port"] = request->static_nat_entry().nat_config().port();
            newSnatEntry["globalPort"] = request->static_nat_entry().nat_config().global_port();
            newSnatEntry["proto"] = request->static_nat_entry().nat_config().ip_protocol();
            data.stateRootObj[subsystem][backhaul]["snatEntries"][currentEntryCount] =
                newSnatEntry;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            data.error = telux::common::ErrorCode::NO_EFFECT;
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status NatServerImpl::RemoveStaticNatEntry(ServerContext* context,
    const dataStub::StaticNatRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = NAT_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = NAT_MANAGER_STATE_JSON;
    std::string subsystem = "INatManager";
    std::string method = "removeStaticNatEntry";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->static_nat_entry().operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if ((!DataUtilsStub::isValidIpv4Address(
        request->static_nat_entry().nat_config().address()))
        && (!DataUtilsStub::isValidIpv6Address(
        request->static_nat_entry().nat_config().address()))) {

        LOG(ERROR, __FUNCTION__, " Address provided shall be in either IPv4 or Ipv6 format");
        data.error = telux::common::ErrorCode::INTERNAL;
    }

    if (!telux::data::DataHelper::isValidProtocol(DataUtilsStub::stringToProtocol(
        request->static_nat_entry().nat_config().ip_protocol()))) {
        LOG(ERROR, __FUNCTION__, " unexpected protocol");
        data.error = telux::common::ErrorCode::INTERNAL;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        int entryIdx = -1, currentEntryCount = 0, backhaul = 0;
        auto bh_info = request->static_nat_entry().backhaul_type();
        if (bh_info == ::dataStub::BackhaulPreference::PREF_WWAN) {
            backhaul = WWAN_BH_IDX;
        } else if (bh_info == ::dataStub::BackhaulPreference::PREF_ETH) {
            backhaul = ETH_BH_IDX;
        }

        currentEntryCount = data.stateRootObj[subsystem][backhaul]["snatEntries"].size();
        bool entryExists = isNatEntryAvailable(subsystem, data, request, entryIdx);
        if (entryExists) {
            int newCount = 0;
            int index = 0;
            Json::Value newRoot;
            for (; index < currentEntryCount; index++) {
                //skipping to add entry in new array for the matched index.
                if (entryIdx == index ) {
                    continue;
                }
                newRoot[subsystem][backhaul]["snatEntries"][newCount]
                    = data.stateRootObj[subsystem][backhaul]["snatEntries"][index];
                newCount++;
            }
            data.stateRootObj[subsystem][backhaul]["snatEntries"]
                = newRoot[subsystem][backhaul]["snatEntries"];
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            data.error = telux::common::ErrorCode::INTERNAL;
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status NatServerImpl::RequestStaticNatEntries(ServerContext* context,
    const dataStub::RequestStaticNatEntriesRequest* request,
    dataStub::RequestStaticNatEntriesReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = NAT_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = NAT_MANAGER_STATE_JSON;
    std::string subsystem = "INatManager";
    std::string method = "requestStaticNatEntries";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        int currentEntryCount = 0, backhaul = 0, profile_id = -1, slot_id = 1, vlan_id = -1;
        auto bh_info = request->backhaul_type();

        if (bh_info == ::dataStub::BackhaulPreference::PREF_WWAN) {
            backhaul = WWAN_BH_IDX;
            profile_id = request->profile_id();
            slot_id = request->slot_id();
        } else if (bh_info == ::dataStub::BackhaulPreference::PREF_ETH) {
            backhaul = ETH_BH_IDX;
            vlan_id = request->vlan_id();
        }

        currentEntryCount = data.stateRootObj[subsystem][backhaul]["snatEntries"].size();

        int index = 0;
        bool matched = false;
        for (; index < currentEntryCount; index++) {
            Json::Value requestedNatEntry =
                data.stateRootObj[subsystem][backhaul]["snatEntries"][index];

            if (bh_info == ::dataStub::BackhaulPreference::PREF_WWAN) {
                if ((requestedNatEntry["profileId"] == profile_id) &&
                        (requestedNatEntry["slotId"] == slot_id)) {
                    matched = true;
                }
            } else if (bh_info == ::dataStub::BackhaulPreference::PREF_ETH) {
                if (requestedNatEntry["vlanId"] == vlan_id) {
                    matched = true;
                }
            }

            if (matched) {
                dataStub::NatConfig *nat_config = response->add_nat_config();
                nat_config->set_address(requestedNatEntry["addr"].asString());
                nat_config->set_port(requestedNatEntry["port"].asInt());
                nat_config->set_global_port(requestedNatEntry["globalPort"].asInt());
                nat_config->set_ip_protocol(requestedNatEntry["proto"].asString());
                break;
            }
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}
