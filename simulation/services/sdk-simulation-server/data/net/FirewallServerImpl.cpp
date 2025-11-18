/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include <sys/time.h>

#include "FirewallServerImpl.hpp"
#include "libs/common/Logger.hpp"

#define FIREWALL_MANAGER_API_LOCAL_JSON "api/data/IFirewallManagerLocal.json"
#define FIREWALL_MANAGER_STATE_JSON "system-state/data/IFirewallManagerState.json"

FirewallServerImpl::FirewallServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

FirewallServerImpl::~FirewallServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status FirewallServerImpl::InitService(ServerContext* context,
    const dataStub::InitRequest* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = FIREWALL_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IFirewallManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IFirewallManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status FirewallServerImpl::SetFirewall(ServerContext* context,
    const dataStub::SetFirewallRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = FIREWALL_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = FIREWALL_MANAGER_STATE_JSON;
    std::string subsystem = "IFirewallManager";
    std::string method = "setFirewallConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int idx = 0;
        bool isFound = isConfigAvailable(subsystem, "firewallConfig", data, request, idx);

        if (isFound) {
            data.stateRootObj[subsystem]["firewallConfig"][idx]["enable"] =
                request->fw_enable();
            data.stateRootObj[subsystem]["firewallConfig"][idx]["allowPackets"] =
                request->allow_packets();
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            const Json::Value& config = data.stateRootObj[subsystem]["firewallConfig"];
            int count = config.size();
            Json::Value newConfig;
            newConfig["backhaul"] =
                DataUtilsStub::convertEnumToBackhaulPrefString(
                    static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
            newConfig["slotId"] = request->slot_id();
            newConfig["profileId"] = request->profile_id();
            newConfig["enable"] = request->fw_enable();
            newConfig["allowPackets"] = request->allow_packets();
            data.stateRootObj[subsystem]["firewallConfig"][count] = newConfig;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }
    LOG(DEBUG, __FUNCTION__, " enable::", request->fw_enable(),
        " allowPackets::", request->allow_packets());
    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status FirewallServerImpl::RequestFirewallStatus(ServerContext* context,
    const dataStub::FirewallStatusRequest* request,
    dataStub::RequestFirewallStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = FIREWALL_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = FIREWALL_MANAGER_STATE_JSON;
    std::string subsystem = "IFirewallManager";
    std::string method = "requestFirewallConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int idx = 0;
        bool isFound = isConfigAvailable(subsystem, "firewallConfig", data, request, idx);
        if (isFound) {
            response->set_fw_enable(
                data.stateRootObj[subsystem]["firewallConfig"][idx]["enable"].asBool());
            response->set_allow_packets(
                data.stateRootObj[subsystem]["firewallConfig"][idx]["allowPackets"].asBool());
            LOG(DEBUG, __FUNCTION__, " config found.");
        } else {
            response->set_fw_enable(false);
            response->set_allow_packets(false);
            LOG(DEBUG, __FUNCTION__, " config doesn't exist.");
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status FirewallServerImpl::AddFirewallEntry(ServerContext* context,
    const dataStub::AddFirewallEntryRequest* request, dataStub::AddFirewallEntryReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = FIREWALL_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = FIREWALL_MANAGER_STATE_JSON;
    std::string subsystem = "IFirewallManager";
    std::string method = "addFirewallEntry";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int idx = 0;
        bool isFound = isFirewallEntryAvailable(subsystem, "firewallEntry", data, request, idx);

        if (isFound) {
            LOG(DEBUG, __FUNCTION__, " fw entry already exist.");
            data.error = telux::common::ErrorCode::NO_EFFECT;
        } else {
            std::vector<std::string> protocols;
            if ("PROTO_TCP_UDP" == request->protocol()) {
                protocols.push_back("TCP");
                protocols.push_back("UDP");
            } else {
                protocols.push_back(request->protocol());
            }

            for (const auto& protocol: protocols) {
                const Json::Value& config = data.stateRootObj[subsystem]["firewallEntry"];
                int count = config.size();
                Json::Value newConfig;
                newConfig["backhaul"] =
                    DataUtilsStub::convertEnumToBackhaulPrefString(
                        static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
                newConfig["slotId"] = request->slot_id();
                newConfig["profileId"] = request->profile_id();

                newConfig["fw_direction"] = request->fw_direction().fw_direction();
                newConfig["isHwAccelerated"] = request->is_hw_accelerated();
                newConfig["protocol"] = protocol;
                std::string ipFamily = DataUtilsStub::convertIpFamilyEnumToString(
                    request->ip_family_type().ip_family_type());
                newConfig["ip_family_type"] = ipFamily;

                if (ipFamily ==  "IPV4") {
                    newConfig["ipv4_srcAddr"] = request->ipv4_params().ipv4_src_address();
                    newConfig["ipv4_srcSubnetMask"] =
                        request->ipv4_params().ipv4_src_subnet_mask();
                    newConfig["ipv4_destAddr"] = request->ipv4_params().ipv4_dest_address();
                    newConfig["ipv4_destSubnetMask"] =
                        request->ipv4_params().ipv4_dest_subnet_mask();

                    newConfig["ipv4_value"] = request->ipv4_params().ipv4_tos_val();
                    newConfig["ipv4_mask"] = request->ipv4_params().ipv4_tos_mask();
                }

                if (ipFamily ==  "IPV6") {
                    newConfig["ipv6_srcAddr"] = request->ipv6_params().ipv6_src_address();
                    newConfig["ipv6_srcPrefixLen"] =
                        request->ipv6_params().ipv6_src_prefix_len();
                    newConfig["ipv6_destAddr"] = request->ipv6_params().ipv6_dest_address();
                    newConfig["ipv6_dstPrefixLen"] =
                        request->ipv6_params().ipv6_dest_prefix_len();

                    newConfig["ipv6_val"] = request->ipv6_params().trf_value();
                    newConfig["ipv6_mask"] = request->ipv6_params().trf_mask();
                    newConfig["ipv6_flowLabel"] = request->ipv6_params().flow_label();
                    newConfig["ipv6_natEnabled"] = request->ipv6_params().nat_enabled();
                }

                newConfig["source_port"] = request->protocol_params().source_port();
                newConfig["source_port_range"] = request->protocol_params().source_port_range();
                newConfig["dest_port"] = request->protocol_params().dest_port();
                newConfig["dest_port_range"] = request->protocol_params().dest_port_range();
                newConfig["esp_spi"] = request->protocol_params().esp_spi();
                newConfig["icmp_type"] = request->protocol_params().icmp_type();
                newConfig["icmp_code"] = request->protocol_params().icmp_code();

                uint32_t handle = getHandle();
                newConfig["handle"] = handle;
                response->set_handle(handle);
                LOG(DEBUG, __FUNCTION__, " adding fw entry for handle::", handle);
                data.stateRootObj[subsystem]["firewallEntry"][count] = newConfig;
                JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
            }
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status FirewallServerImpl::RemoveFirewallEntry(ServerContext* context,
    const dataStub::RemoveFirewallEntryRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = FIREWALL_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = FIREWALL_MANAGER_STATE_JSON;
    std::string subsystem = "IFirewallManager";
    std::string method = "removeFirewallEntry";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        const Json::Value& config = data.stateRootObj[subsystem]["firewallEntry"];
        int count = config.size();
        bool isFound = false;
        int idx = 0;
        for (idx=0; idx < count; idx++) {
            if (config[idx]["handle"].asInt() != request->entry_handle()) {
                continue;
            }
            isFound = true;
            break;
        }

        if (isFound) {
            LOG(DEBUG, __FUNCTION__, " removing fw entry for handle::",
                request->entry_handle());
            int currentCount =
                data.stateRootObj[subsystem]["firewallEntry"].size();
            int newCount = 0;
            int index = 0;
            Json::Value newRoot;
            for (; index < currentCount; index++) {
                //skipping to add entry in new array for the matched index.
                if (idx == index ) {
                    continue;
                }
                newRoot[subsystem]["firewallEntry"][newCount]
                    = data.stateRootObj[subsystem]["firewallEntry"][index];
                newCount++;
            }
            data.stateRootObj[subsystem]["firewallEntry"]
                = newRoot[subsystem]["firewallEntry"];
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

uint32_t FirewallServerImpl::getHandle() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand(ts.tv_sec * 1000000000LL + ts.tv_nsec);
    return rand();
}

grpc::Status FirewallServerImpl::RequestFirewallEntries(ServerContext* context,
    const dataStub::FirewallEntriesRequest* request,
    dataStub::RequestFirewallEntriesReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = FIREWALL_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = FIREWALL_MANAGER_STATE_JSON;
    std::string subsystem = "IFirewallManager";
    std::string method = "requestFirewallEntries";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int idx = 0;
        auto profile_id = request->profile_id();
        auto slot_id = request->slot_id();
        auto backhaulType = DataUtilsStub::convertEnumToBackhaulPrefString(
            request->backhaul_type());
        auto isHwAccelerated = request->is_hw_accelerated();
        bool isFound = isConfigAvailable(subsystem, "firewallEntry", data, request, idx);
        if (isFound) {
            int count =
                data.stateRootObj[subsystem]["firewallEntry"].size();
            int index = 0;
            for (; index < count; index++) {
                Json::Value requestedFirewallEntry =
                    data.stateRootObj[subsystem]["firewallEntry"][index];

                if ((requestedFirewallEntry["profileId"].asInt() == profile_id) &&
                    (requestedFirewallEntry["slotId"].asInt() == slot_id) &&
                    (requestedFirewallEntry["backhaul"].asString() == backhaulType) &&
                    (requestedFirewallEntry["isHwAccelerated"].asBool() == isHwAccelerated)) {

                    dataStub::FirewallEntry *fw_entry = response->add_firewall_entries();
                    fw_entry->mutable_fw_direction()->set_fw_direction(
                        (dataStub::Direction::Fw_Direction)
                        requestedFirewallEntry["fw_direction"].asInt());
                    fw_entry->set_protocol(requestedFirewallEntry["protocol"].asString());
                    fw_entry->mutable_ip_family_type()->set_ip_family_type(
                        (DataUtilsStub::convertIpFamilyStringToEnum(
                        requestedFirewallEntry["ip_family_type"].asString())));

                    if (requestedFirewallEntry["ip_family_type"].asString() == "IPV4") {
                        fw_entry->mutable_ipv4_params()->set_ipv4_src_address(
                            requestedFirewallEntry["ipv4_srcAddr"].asString());
                        fw_entry->mutable_ipv4_params()->set_ipv4_src_subnet_mask(
                            requestedFirewallEntry["ipv4_srcSubnetMask"].asString());
                        fw_entry->mutable_ipv4_params()->set_ipv4_dest_address(
                            requestedFirewallEntry["ipv4_destAddr"].asString());
                        fw_entry->mutable_ipv4_params()->set_ipv4_dest_subnet_mask(
                            requestedFirewallEntry["ipv4_destSubnetMask"].asString());
                        fw_entry->mutable_ipv4_params()->set_ipv4_tos_val(
                            requestedFirewallEntry["ipv4_value"].asInt());
                        fw_entry->mutable_ipv4_params()->set_ipv4_tos_mask(
                            requestedFirewallEntry["ipv4_mask"].asInt());
                    }

                    if (requestedFirewallEntry["ip_family_type"].asString() == "IPV6") {
                        fw_entry->mutable_ipv6_params()->set_ipv6_src_address(
                            requestedFirewallEntry["ipv6_srcAddr"].asString());
                        fw_entry->mutable_ipv6_params()->set_ipv6_dest_address(
                            requestedFirewallEntry["ipv6_destAddr"].asString());
                        fw_entry->mutable_ipv6_params()->set_ipv6_src_prefix_len(
                            requestedFirewallEntry["ipv6_srcPrefixLen"].asInt());
                        fw_entry->mutable_ipv6_params()->set_ipv6_dest_prefix_len(
                            requestedFirewallEntry["ipv6_dstPrefixLen"].asInt());
                        fw_entry->mutable_ipv6_params()->set_trf_value(
                            requestedFirewallEntry["ipv6_val"].asInt());
                        fw_entry->mutable_ipv6_params()->set_trf_mask(
                            requestedFirewallEntry["ipv6_mask"].asInt());
                        fw_entry->mutable_ipv6_params()->set_flow_label(
                            requestedFirewallEntry["ipv6_flowLabel"].asInt());
                        fw_entry->mutable_ipv6_params()->set_nat_enabled(
                            requestedFirewallEntry["ipv6_natEnabled"].asInt());
                    }

                    fw_entry->mutable_protocol_params()->set_source_port(
                        requestedFirewallEntry["source_port"].asInt());
                    fw_entry->mutable_protocol_params()->set_source_port_range(
                        requestedFirewallEntry["source_port_range"].asInt());
                    fw_entry->mutable_protocol_params()->set_dest_port(
                        requestedFirewallEntry["dest_port"].asInt());
                    fw_entry->mutable_protocol_params()->set_dest_port_range(
                        requestedFirewallEntry["dest_port_range"].asInt());
                    fw_entry->mutable_protocol_params()->set_esp_spi(
                        requestedFirewallEntry["esp_spi"].asInt());
                    fw_entry->mutable_protocol_params()->set_icmp_type(
                        requestedFirewallEntry["icmp_type"].asInt());
                    fw_entry->mutable_protocol_params()->set_icmp_code(
                        requestedFirewallEntry["icmp_code"].asInt());

                    fw_entry->set_firewall_handle(
                        requestedFirewallEntry["handle"].asInt());
                    LOG(DEBUG, __FUNCTION__, " found fw entry for handle::",
                        requestedFirewallEntry["handle"].asInt());
                } else {
                    LOG(DEBUG, __FUNCTION__, " fw entry doesn't exist");
                    data.error = telux::common::ErrorCode::SUCCESS;
                }
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " fw entry doesn't exist");
            data.error = telux::common::ErrorCode::SUCCESS;
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status FirewallServerImpl::EnableDMZ(ServerContext* context,
    const dataStub::EnableDMZRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = FIREWALL_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = FIREWALL_MANAGER_STATE_JSON;
    std::string subsystem = "IFirewallManager";
    std::string method = "enableDmz";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (!DataUtilsStub::isValidIpv4Address(request->ip_address())) {
        data.error = telux::common::ErrorCode::INTERNAL;
        response->set_status(static_cast<commonStub::Status>(data.status));
        response->set_error(static_cast<commonStub::ErrorCode>(data.error));
        return grpc::Status::OK;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int idx = 0;
        bool isFound = isConfigAvailableForBackhaul(subsystem, "dmzConfig", data, request, idx);

        if (isFound) {
            data.error = telux::common::ErrorCode::NO_EFFECT;
        } else {
            const Json::Value& config = data.stateRootObj[subsystem]["dmzConfig"];
            int count = config.size();
            Json::Value newConfig;

            // currently supported backhauls are WWAN, WLAN and ETH
            if (request->backhaul_type() == ::dataStub::BackhaulPreference::PREF_WWAN) {
                newConfig["slotId"] = request->slot_id();
                newConfig["profileId"] = request->profile_id();
            } else if (request->backhaul_type() == ::dataStub::BackhaulPreference::PREF_ETH) {
                newConfig["vlanId"] = request->vlan_id();
            }

            newConfig["backhaul"] =
                DataUtilsStub::convertEnumToBackhaulPrefString(
                    static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
            newConfig["ipAddr"] = request->ip_address();
            data.stateRootObj[subsystem]["dmzConfig"][count] = newConfig;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status FirewallServerImpl::DisableDMZ(ServerContext* context,
    const dataStub::DisableDmzRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = FIREWALL_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = FIREWALL_MANAGER_STATE_JSON;
    std::string subsystem = "IFirewallManager";
    std::string method = "disableDmz";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int idx = 0;
        bool isFound = isConfigAvailableForBackhaul(subsystem, "dmzConfig", data, request, idx);

        if (isFound) {
            std::string ipFamily = DataUtilsStub::convertIpFamilyEnumToString(
                request->ip_family_type().ip_family_type());

            if (ipFamily == "IPV6") {
                data.error = telux::common::ErrorCode::INTERNAL;
            } else {
                int currentCount =
                    data.stateRootObj[subsystem]["dmzConfig"].size();
                int newCount = 0;
                int index = 0;
                Json::Value newRoot;
                for (; index < currentCount; index++) {
                    //skipping to add entry in new array for the matched index.
                    if (idx == index ) {
                        continue;
                    }
                    newRoot[subsystem]["dmzConfig"][newCount]
                        = data.stateRootObj[subsystem]["dmzConfig"][index];
                    newCount++;
                }
                data.stateRootObj[subsystem]["dmzConfig"]
                    = newRoot[subsystem]["dmzConfig"];
                JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
            }
        } else {
            data.error = telux::common::ErrorCode::INTERNAL;
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status FirewallServerImpl::RequestDMZEntry(ServerContext* context,
    const dataStub::DMZEntryRequest* request,
    dataStub::RequestDMZEntryReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = FIREWALL_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = FIREWALL_MANAGER_STATE_JSON;
    std::string subsystem = "IFirewallManager";
    std::string method = "requestDmzEntry";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int idx = 0;
        bool isFound = isConfigAvailableForBackhaul(subsystem, "dmzConfig", data, request, idx);
        if (isFound) {
            response->add_dmz_entries(
                data.stateRootObj[subsystem]["dmzConfig"][idx]["ipAddr"].asString());
        } else {
            response->add_dmz_entries("0.0.0.0");
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}
