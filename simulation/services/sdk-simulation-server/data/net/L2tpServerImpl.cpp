/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "L2tpServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/data/DataUtilsStub.hpp"

#define L2TP_MANAGER_API_JSON "api/data/IL2tpManager.json"
#define L2TP_MANAGER_STATE_JSON "system-state/data/IL2tpManagerState.json"

#define MAX_TUNNEL 2
#define MAX_SESSION 4

L2tpServerImpl::L2tpServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

L2tpServerImpl::~L2tpServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status L2tpServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = L2TP_MANAGER_API_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IL2tpManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IL2tpManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status L2tpServerImpl::SetConfig(ServerContext* context,
    const dataStub::SetConfigRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = L2TP_MANAGER_API_JSON;
    std::string stateJsonPath = L2TP_MANAGER_STATE_JSON;
    std::string subsystem = "IL2tpManager";
    std::string method = "setConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        data.stateRootObj[subsystem]["l2tpConfig"]["enable"]
            = request->enable_config();
        data.stateRootObj[subsystem]["l2tpConfig"]["enableMtu"]
            = request->enable_mtu();
        data.stateRootObj[subsystem]["l2tpConfig"]["enableTcpMss"]
            = request->enable_mss();
        data.stateRootObj[subsystem]["l2tpConfig"]["mtuSize"]
            = request->mtu_size();

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status L2tpServerImpl::RequestConfig(ServerContext* context,
    const google::protobuf::Empty* request,
    dataStub::RequestConfigReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = L2TP_MANAGER_API_JSON;
    std::string stateJsonPath = L2TP_MANAGER_STATE_JSON;
    std::string subsystem = "IL2tpManager";
    std::string method = "requestConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (!isL2tpEnabled(data)) {
        LOG(DEBUG, __FUNCTION__, " L2tp not enabled");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
            response->set_enable_mtu(
                data.stateRootObj[subsystem]["l2tpConfig"]["enableMtu"].asBool());
            response->set_enable_tcp_mss(
                data.stateRootObj[subsystem]["l2tpConfig"]["enableTcpMss"].asBool());
            response->set_mtu_size(
                data.stateRootObj[subsystem]["l2tpConfig"]["mtuSize"].asInt());

            int currentConfigCount =
                data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"].size();
            int configIdx = 0;
            for (; configIdx < currentConfigCount; configIdx++) {
                Json::Value requestedConfig =
                    data.stateRootObj[subsystem]["l2tpConfig"]
                    ["tunnelConfigs"][configIdx];

                dataStub::L2tpTunnelConfig *tunnelConfig = response->add_l2tp_tunnel_config();
                tunnelConfig->set_l2tp_prot(
                    DataUtilsStub::stringToL2tpProtocol(requestedConfig["prot"].asString()));
                tunnelConfig->set_loc_id(requestedConfig["locId"].asInt());
                tunnelConfig->set_peer_id(requestedConfig["peerId"].asInt());
                tunnelConfig->set_local_udp_port(requestedConfig["localUdpPort"].asInt());
                tunnelConfig->set_peer_udp_port(requestedConfig["peerUdpPort"].asInt());
                tunnelConfig->set_peer_ipv6_addr(requestedConfig["peerIpv6Addr"].asString());
                tunnelConfig->set_peer_ipv6_gw_addr(requestedConfig["peerIpv6GwAddr"].asString());
                tunnelConfig->set_peer_ipv4_addr(requestedConfig["peerIpv4Addr"].asString());
                tunnelConfig->set_peer_ipv4_gw_addr(requestedConfig["peerIpv4GwAddr"].asString());
                tunnelConfig->set_loc_iface(requestedConfig["locIface"].asString());
                tunnelConfig->mutable_ip_family_type()->set_ip_family_type(
                    (DataUtilsStub::convertIpFamilyStringToEnum(
                        requestedConfig["ipType"].asString())));
                int currentSessionCount =
                    requestedConfig["sessionConfig"].size();
                for (auto sessionIdx = 0; sessionIdx < currentSessionCount; sessionIdx++) {
                    Json::Value requestedSession =
                        requestedConfig["sessionConfig"][sessionIdx];
                    ::dataStub::L2tpSessionConfig *sessionConfig =
                        tunnelConfig->add_session_config();
                    sessionConfig->set_loc_id(requestedSession["locId"].asInt());
                    sessionConfig->set_peer_id(requestedSession["peerId"].asInt());
                }
            }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status L2tpServerImpl::AddTunnel(ServerContext* context,
    const ::dataStub::AddTunnelRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = L2TP_MANAGER_API_JSON;
    std::string stateJsonPath = L2TP_MANAGER_STATE_JSON;
    std::string subsystem = "IL2tpManager";
    std::string method = "addTunnel";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    int tunnelIdx = 0;
    if (!isL2tpEnabled(data)) {
        LOG(DEBUG, __FUNCTION__, " L2tp not enabled");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    } else if (configExists(data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"],
        request->l2tp_tunnel_config().loc_id(), tunnelIdx)) {
        LOG(DEBUG, __FUNCTION__, " tunnel already exist");
        data.error = telux::common::ErrorCode::NO_EFFECT;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        //adding tunnel config
        Json::Value newTunnel;
        int currentCount =
            data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"].size();
        do {
            if (request->l2tp_tunnel_config().ip_family_type().ip_family_type()
                == dataStub::IpFamilyType::IPV4) {
                if (!DataUtilsStub::isValidIpv4Address(
                    request->l2tp_tunnel_config().peer_ipv4_addr())) {
                    LOG(ERROR, __FUNCTION__, " Invalid Ipv4 Addr Provided ");
                    data.error = telux::common::ErrorCode::INTERNAL;
                    break;
                }
                if (!DataUtilsStub::isValidIpv4Address(
                    request->l2tp_tunnel_config().peer_ipv4_gw_addr())) {
                    LOG(ERROR, __FUNCTION__, " Invalid Ipv4 Gateway Addr Provided ");
                }
            } else if (request->l2tp_tunnel_config().ip_family_type().ip_family_type()
                == dataStub::IpFamilyType::IPV6) {
                 if (!DataUtilsStub::isValidIpv6Address(
                    request->l2tp_tunnel_config().peer_ipv6_addr())) {
                    LOG(ERROR, __FUNCTION__, " Invalid Ipv6 Addr Provided ");
                    data.error = telux::common::ErrorCode::INTERNAL;
                    break;
                }
                if (!DataUtilsStub::isValidIpv6Address(
                    request->l2tp_tunnel_config().peer_ipv6_gw_addr())) {
                    LOG(ERROR, __FUNCTION__, " Invalid Ipv6 Gateway Addr Provided ");
                }
            } else {
                data.error = telux::common::ErrorCode::INTERNAL;
                LOG(ERROR, __FUNCTION__, " Invalid IP Type entered ");
                break;
            }

            if (currentCount > MAX_TUNNEL) {
                LOG(DEBUG, __FUNCTION__, " exceeding max tunnels supported");
                data.error = telux::common::ErrorCode::NOT_SUPPORTED;
            } else {
                LOG(DEBUG, __FUNCTION__, " adding tunnel ",
                request->l2tp_tunnel_config().loc_id());
                std::string protocol = DataUtilsStub::l2tpProtocolToString(
                    request->l2tp_tunnel_config().l2tp_prot());
                newTunnel["prot"] = protocol;
                if (protocol == "UDP") {
                    newTunnel["localUdpPort"] =
                        request->l2tp_tunnel_config().local_udp_port();
                    newTunnel["peerUdpPort"] =
                        request->l2tp_tunnel_config().peer_udp_port();
                }
                newTunnel["locId"] = request->l2tp_tunnel_config().loc_id();
                newTunnel["peerId"] = request->l2tp_tunnel_config().peer_id();

                std::string ipFamily = DataUtilsStub::convertIpFamilyEnumToString(
                    request->l2tp_tunnel_config().ip_family_type().ip_family_type());

                if (ipFamily ==  "IPV4") {
                    newTunnel["peerIpv4Addr"] =
                        request->l2tp_tunnel_config().peer_ipv4_addr();
                    newTunnel["peerIpv4GwAddr"] =
                        request->l2tp_tunnel_config().peer_ipv4_gw_addr();
                } else if (ipFamily ==  "IPV6") {
                    newTunnel["peerIpv6Addr"] =
                        request->l2tp_tunnel_config().peer_ipv6_addr();
                    newTunnel["peerIpv6GwAddr"] =
                        request->l2tp_tunnel_config().peer_ipv6_gw_addr();
                }

                newTunnel["locIface"] = request->l2tp_tunnel_config().loc_iface();
                newTunnel["ipType"] = ipFamily;

                int sessionIdx = 0;
                for (auto& session: request->l2tp_tunnel_config().session_config()) {
                    //adding sessionconfig
                    Json::Value newSession;
                    newSession["locId"] = session.loc_id();
                    newSession["peerId"] = session.peer_id();
                    newTunnel["sessionConfig"][sessionIdx] = newSession;
                    sessionIdx++;
                }

                data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"][currentCount]
                    = newTunnel;

                JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
            }
        } while (0);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status L2tpServerImpl::RemoveTunnel(ServerContext* context,
    const ::dataStub::RemoveTunnelRequest* request, ::dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = L2TP_MANAGER_API_JSON;
    std::string stateJsonPath = L2TP_MANAGER_STATE_JSON;
    std::string subsystem = "IL2tpManager";
    std::string method = "removeTunnel";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    int tunnelIdx = 0;
    if (!isL2tpEnabled(data)) {
        LOG(DEBUG, __FUNCTION__, " L2tp not enabled");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    } else if (!configExists(data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"],
        request->tunnel_id(), tunnelIdx)) {
        LOG(DEBUG, __FUNCTION__, " tunnel doesn't exist");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int currentCount =
            data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"].size();

        int newCount = 0;
        int index = 0;
        Json::Value newRoot;
        for (; index < currentCount; index++) {
            //skipping to add entry in new array for the tunnelIdx.
            if (index == tunnelIdx) {
                continue;
            }
            newRoot[subsystem]["l2tpConfig"]["tunnelConfigs"][newCount]
                = data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"][index];
            newCount++;
        }
        LOG(DEBUG, __FUNCTION__, " removed tunnel ", request->tunnel_id());
        data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"]
            = newRoot[subsystem]["l2tpConfig"]["tunnelConfigs"];
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status L2tpServerImpl::AddSession(ServerContext* context,
    const ::dataStub::AddSessionRequest* request, ::dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = L2TP_MANAGER_API_JSON;
    std::string stateJsonPath = L2TP_MANAGER_STATE_JSON;
    std::string subsystem = "IL2tpManager";
    std::string method = "addSession";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    int tunnelIdx = 0;
    if (!isL2tpEnabled(data)) {
        LOG(DEBUG, __FUNCTION__, " L2tp not enabled");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    } else if (!configExists(data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"],
        request->tunnel_id(), tunnelIdx)) {
        LOG(DEBUG, __FUNCTION__, " tunnel doesn't exist");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        Json::Value& sessionConfig =
            data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"]
            [tunnelIdx]["sessionConfig"];
        int currentSessionCount = sessionConfig.size();
        int sessionIdx = 0;

        if (MAX_SESSION == currentSessionCount) {
            LOG(DEBUG, __FUNCTION__, " exceeding max session supported");
            data.error = telux::common::ErrorCode::NOT_SUPPORTED;
        } else if (configExists(sessionConfig,
            request->session_config().loc_id(), sessionIdx)) {
            LOG(DEBUG, __FUNCTION__, " session with Id ",
                request->session_config().loc_id()," already exist");
            data.error = telux::common::ErrorCode::NO_EFFECT;
        } else {
            //adding sessionconfig
            Json::Value newSession;
            newSession["locId"] = request->session_config().loc_id();
            newSession["peerId"] = request->session_config().peer_id();
            sessionConfig[currentSessionCount] = newSession;
            LOG(DEBUG, __FUNCTION__, " added session with Id ",
                request->session_config().loc_id());
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status L2tpServerImpl::RemoveSession(ServerContext* context,
    const ::dataStub::RemoveSessionRequest* request, ::dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = L2TP_MANAGER_API_JSON;
    std::string stateJsonPath = L2TP_MANAGER_STATE_JSON;
    std::string subsystem = "IL2tpManager";
    std::string method = "removeSession";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    int tunnelIdx = 0;
    if (!isL2tpEnabled(data)) {
        LOG(DEBUG, __FUNCTION__, " L2tp not enabled");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    } else if (!configExists(data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"],
        request->tunnel_id(), tunnelIdx)) {
        LOG(DEBUG, __FUNCTION__, " tunnel doesn't exist");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int sessionIdx = 0;
        Json::Value& sessionConfig =
            data.stateRootObj[subsystem]["l2tpConfig"]["tunnelConfigs"]
            [tunnelIdx]["sessionConfig"];
        if (!configExists(sessionConfig,
            request->session_id(), sessionIdx)) {
            LOG(DEBUG, __FUNCTION__, " session with Id ", request->session_id()," doesn't exist");
            data.error = telux::common::ErrorCode::NOT_SUPPORTED;
        } else {
            int currentSessionCount = sessionConfig.size();
            int newCount = 0;
            int index = 0;
            Json::Value newRoot;
            for (; index < currentSessionCount; index++) {
                if (index == sessionIdx) {
                    //skipping to add entry in new array for the matched sessionIdx
                    continue;
                }
                newRoot[subsystem]["l2tpConfig"]["tunnelConfigs"][tunnelIdx]
                    ["sessionConfig"][newCount] = sessionConfig[index];
                newCount++;
            }
            LOG(DEBUG, __FUNCTION__, " removing session with Id ",
                request->session_id());
            sessionConfig =
                newRoot[subsystem]["l2tpConfig"]["tunnelConfigs"][tunnelIdx]["sessionConfig"];
        }

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status L2tpServerImpl::BindSessionToBackhaul(ServerContext* context,
    const dataStub::SessionConfigRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = L2TP_MANAGER_API_JSON;
    std::string stateJsonPath = L2TP_MANAGER_STATE_JSON;
    std::string subsystem = "IL2tpManager";
    std::string method = "bindSessionToBackhaul";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (!isL2tpEnabled(data)) {
        LOG(DEBUG, __FUNCTION__, " L2tp not enabled");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    } else {
        //since currently Qcmap only supports binding with WWAN
        std::string backhaul = DataUtilsStub::convertEnumToBackhaulPrefString(
            static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
        if (backhaul != "WWAN") {
            LOG(DEBUG, __FUNCTION__, backhaul, " not supported currently");
            data.error = telux::common::ErrorCode::NOT_SUPPORTED;
        }
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        Json::Value& bindings =
            data.stateRootObj[subsystem]["sessionToBackhaulBindings"];

        int bindingIdx = 0;
        if (configExists(bindings, request->loc_id(), bindingIdx)) {
            LOG(DEBUG, __FUNCTION__, " binding with Id ", request->loc_id()," already exist");
            data.error = telux::common::ErrorCode::NO_EFFECT;
        } else {
            Json::Value newbinding;
            int currentCount = bindings.size();
            newbinding["locId"] = request->loc_id();
            newbinding["backhaul"] =
                DataUtilsStub::convertEnumToBackhaulPrefString(
                static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
            newbinding["slotId"] = request->slot_id();
            newbinding["profileId"] = request->profile_id();
            bindings[currentCount] = newbinding;

            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status L2tpServerImpl::UnBindSessionToBackhaul(ServerContext* context,
    const dataStub::SessionConfigRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = L2TP_MANAGER_API_JSON;
    std::string stateJsonPath = L2TP_MANAGER_STATE_JSON;
    std::string subsystem = "IL2tpManager";
    std::string method = "unbindSessionFromBackhaul";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (!isL2tpEnabled(data)) {
        LOG(DEBUG, __FUNCTION__, " L2tp not enabled");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    } else {
        //since currently Qcmap only supports binding with WWAN
        std::string backhaul = DataUtilsStub::convertEnumToBackhaulPrefString(
            static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
        if (backhaul != "WWAN") {
            LOG(DEBUG, __FUNCTION__, backhaul, " not supported currently");
            data.error = telux::common::ErrorCode::NOT_SUPPORTED;
        }
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        Json::Value& bindings =
            data.stateRootObj[subsystem]["sessionToBackhaulBindings"];

        int bindingIdx = 0;
        if (!bindingExists(bindings, request, bindingIdx)) {
            LOG(DEBUG, __FUNCTION__, " binding doesn't exist");
            data.error = telux::common::ErrorCode::NOT_SUPPORTED;
        } else {
            int currentCount = bindings.size();
            int newCount = 0;
            int index = 0;
            Json::Value newRoot;
            for (; index < currentCount; index++) {
                //skipping to add entry in new array.
                if (index == bindingIdx) {
                    continue;
                }
                newRoot[subsystem]["sessionToBackhaulBindings"][newCount] =
                    bindings[index];
                newCount++;
            }
            bindings
                = newRoot[subsystem]["sessionToBackhaulBindings"];
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status L2tpServerImpl::QueryBindSessionToBackhaul(ServerContext* context,
        const dataStub::QueryBindSessionRequest* request,
        dataStub::QueryBindSessionReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = L2TP_MANAGER_API_JSON;
    std::string stateJsonPath = L2TP_MANAGER_STATE_JSON;
    std::string subsystem = "IL2tpManager";
    std::string method = "querySessionToBackhaulBindings";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    std::string reqBackhaul = DataUtilsStub::convertEnumToBackhaulPrefString(
            static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
    if (!isL2tpEnabled(data)) {
        LOG(DEBUG, __FUNCTION__, " L2tp not enabled");
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    } else {
        //since currently Qcmap only supports binding with WWAN
        if (reqBackhaul != "WWAN") {
            LOG(DEBUG, __FUNCTION__, reqBackhaul, " not supported currently");
            data.error = telux::common::ErrorCode::NOT_SUPPORTED;
        }
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        int currentCount =
            data.stateRootObj[subsystem]["sessionToBackhaulBindings"].size();
        int configIdx = 0;
        for (; configIdx < currentCount; configIdx++) {
            Json::Value requestedBinding =
                data.stateRootObj[subsystem]["sessionToBackhaulBindings"]
                [configIdx];
            std::string currBackhaul = requestedBinding["backhaul"].asString();
            if (reqBackhaul == currBackhaul) {
                dataStub::SessionConfigRequest *config = response->add_session_configs();
                config->set_loc_id(requestedBinding["locId"].asInt());
                config->set_backhaul_type(
                    DataUtilsStub::convertBackhaulPrefStringToEnum(currBackhaul));
                config->set_slot_id(requestedBinding["slotId"].asInt());
                config->set_profile_id(requestedBinding["profileId"].asInt());
            }
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

bool L2tpServerImpl::isL2tpEnabled(const JsonData& data) {
    return data.stateRootObj["IL2tpManager"]["l2tpConfig"]["enable"].asBool();
}

bool L2tpServerImpl::configExists(const Json::Value& config, int configID, int &idx) {
    bool isFound = false;
    int configCount = config.size();
    idx = 0;
    for (; idx < configCount; idx++) {
        if (configID == config[idx]["locId"].asInt()) {
            isFound = true;
            break;
        }
    }
    return isFound;
}

bool L2tpServerImpl::bindingExists(const Json::Value& bindings,
    const dataStub::SessionConfigRequest* request, int &idx) {
    bool isFound = false;

    int count = bindings.size();
    for (idx=0; idx < count; idx++) {
        if (bindings[idx]["locId"].asInt() != request->loc_id()) {
            continue;
        }
        if (bindings[idx]["backhaul"].asString() !=
            DataUtilsStub::convertEnumToBackhaulPrefString(request->backhaul_type())) {
            continue;
        }
        if (bindings[idx]["slotId"].asInt() != request->slot_id()) {
            continue;
        }
        if (bindings[idx]["profileId"].asInt() != request->profile_id()) {
            continue;
        }
        isFound = true;
        break;
    }

    return isFound;
}