/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/CommonDefines.hpp>

#include "L2tpManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include "common/AsyncTaskQueue.hpp"
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1
#define DEFAULT_MTU_SIZE 1422

namespace telux {
namespace data {
namespace net {

L2tpManagerStub::L2tpManagerStub () {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IL2tpListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

L2tpManagerStub::~L2tpManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status L2tpManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void L2tpManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::L2tpManager>();

    const google::protobuf::Empty request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    int cbDelay = DEFAULT_DELAY;

    do {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " InitService request failed");
            break;
        }

        cbStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());

        this->onServiceStatusChange(cbStatus);
        LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
    } while (0);

    bool isSubsystemReady = (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)?
        true : false;
    setSubSystemStatus(cbStatus);
    setSubsystemReady(isSubsystemReady);

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}

void L2tpManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
        initCb_ = nullptr;
    }
}

void L2tpManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

void L2tpManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

std::future<bool> L2tpManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(
        std::launch::async, [&] { return L2tpManagerStub::waitForInitialization(); });
    return future;
}

bool L2tpManagerStub::waitForInitialization() {
    LOG(INFO, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
    if (!isSubsystemReady()) {
        cv_.wait(lock);
    }
    return isSubsystemReady();
}

void L2tpManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus L2tpManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

bool L2tpManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

telux::common::Status L2tpManagerStub::setConfig(bool enable, bool enableMss,
    bool enableMtu, telux::common::ResponseCallback callback,
    uint32_t mtuSize) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " L2tp manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::SetConfigRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    if (mtuSize == 0) {
        mtuSize = DEFAULT_MTU_SIZE;
    }
    request.set_enable_config(enable);
    request.set_enable_mss(enableMss);
    request.set_enable_mtu(enableMtu);
    request.set_mtu_size(mtuSize);
    grpc::Status reqStatus = stub_->SetConfig(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setConfig request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status L2tpManagerStub::requestConfig(L2tpConfigCb l2tpConfigCb) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " L2tp manager not ready");
        return telux::common::Status::NOTREADY;
    }

    const google::protobuf::Empty request;
    ::dataStub::RequestConfigReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->RequestConfig(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        L2tpSysConfig l2tpSysConfig;
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " requestConfig failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        l2tpSysConfig.enableMtu = response.enable_mtu();
        l2tpSysConfig.enableTcpMss = response.enable_tcp_mss();
        l2tpSysConfig.mtuSize = response.mtu_size();
        for (auto config : response.l2tp_tunnel_config()) {
            L2tpTunnelConfig l2tpTunnelConfig;
            l2tpTunnelConfig.prot = (L2tpProtocol)config.l2tp_prot();
            l2tpTunnelConfig.locId = config.loc_id();
            l2tpTunnelConfig.peerId = config.peer_id();
            l2tpTunnelConfig.localUdpPort = config.local_udp_port();
            l2tpTunnelConfig.peerUdpPort = config.peer_udp_port();
            l2tpTunnelConfig.peerIpv6Addr = config.peer_ipv6_addr();
            l2tpTunnelConfig.peerIpv6GwAddr = config.peer_ipv6_gw_addr();
            l2tpTunnelConfig.peerIpv4Addr = config.peer_ipv4_addr();
            l2tpTunnelConfig.peerIpv4GwAddr = config.peer_ipv4_gw_addr();
            l2tpTunnelConfig.locIface = config.loc_iface();
            l2tpTunnelConfig.ipType =
                static_cast<telux::data::IpFamilyType>(
                config.ip_family_type().ip_family_type());
            for (auto session: config.session_config()) {
                L2tpSessionConfig sessionConfig;
                sessionConfig.locId = session.loc_id();
                sessionConfig.peerId = session.peer_id();
                l2tpTunnelConfig.sessionConfig.push_back(sessionConfig);
            }
            l2tpSysConfig.configList.push_back(l2tpTunnelConfig);
        }
        if (l2tpConfigCb && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
                [this, l2tpSysConfig, error, delay, l2tpConfigCb]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        l2tpConfigCb(l2tpSysConfig, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status L2tpManagerStub::addTunnel(const L2tpTunnelConfig &l2tpTunnelConfig,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " L2tp manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::AddTunnelRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.mutable_l2tp_tunnel_config()->set_l2tp_prot(
        (::dataStub::L2tpProtocol)l2tpTunnelConfig.prot);
    request.mutable_l2tp_tunnel_config()->set_loc_id(l2tpTunnelConfig.locId);
    request.mutable_l2tp_tunnel_config()->set_peer_id(l2tpTunnelConfig.peerId);
    request.mutable_l2tp_tunnel_config()->set_local_udp_port(l2tpTunnelConfig.localUdpPort);
    request.mutable_l2tp_tunnel_config()->set_peer_udp_port(l2tpTunnelConfig.peerUdpPort);
    request.mutable_l2tp_tunnel_config()->set_peer_ipv6_addr(l2tpTunnelConfig.peerIpv6Addr);
    request.mutable_l2tp_tunnel_config()->set_peer_ipv6_gw_addr(l2tpTunnelConfig.peerIpv6GwAddr);
    request.mutable_l2tp_tunnel_config()->set_peer_ipv4_addr(l2tpTunnelConfig.peerIpv4Addr);
    request.mutable_l2tp_tunnel_config()->set_peer_ipv4_gw_addr(l2tpTunnelConfig.peerIpv4GwAddr);
    request.mutable_l2tp_tunnel_config()->set_loc_iface(l2tpTunnelConfig.locIface);
    request.mutable_l2tp_tunnel_config()->mutable_ip_family_type()->set_ip_family_type(
        (::dataStub::IpFamilyType::Type)l2tpTunnelConfig.ipType);
    for (size_t i = 0; i < l2tpTunnelConfig.sessionConfig.size(); ++i) {
        ::dataStub::L2tpSessionConfig* session =
            request.mutable_l2tp_tunnel_config()->add_session_config();
        session->set_loc_id(l2tpTunnelConfig.sessionConfig[i].locId);
        session->set_peer_id(l2tpTunnelConfig.sessionConfig[i].peerId);
    }
    grpc::Status reqStatus = stub_->AddTunnel(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " addTunnel request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status L2tpManagerStub::removeTunnel(
    uint32_t tunnelId, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " L2tp manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::RemoveTunnelRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_tunnel_id(tunnelId);
    grpc::Status reqStatus = stub_->RemoveTunnel(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " removeTunnel request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status L2tpManagerStub::addSession(uint32_t tunnelId,
    L2tpSessionConfig sessionConfig, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " L2tp manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::AddSessionRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_tunnel_id(tunnelId);
    request.mutable_session_config()->set_loc_id(sessionConfig.locId);
    request.mutable_session_config()->set_peer_id(sessionConfig.peerId);
    grpc::Status reqStatus = stub_->AddSession(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " addSession request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status L2tpManagerStub::removeSession(uint32_t tunnelId,
    uint32_t sessionId, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " L2tp manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::RemoveSessionRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_tunnel_id(tunnelId);
    request.set_session_id(sessionId);
    grpc::Status reqStatus = stub_->RemoveSession(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " removeSession request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status L2tpManagerStub::bindSessionToBackhaul(
    L2tpSessionBindConfig sessionBindConfig, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " L2tp manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::SessionConfigRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_loc_id(sessionBindConfig.locId);
    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        sessionBindConfig.bhInfo.backhaul));
    request.set_slot_id(sessionBindConfig.bhInfo.slotId);
    request.set_profile_id(sessionBindConfig.bhInfo.profileId);

    grpc::Status reqStatus = stub_->BindSessionToBackhaul(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " bindSessionToBackhaul request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status L2tpManagerStub::unbindSessionFromBackhaul(
    L2tpSessionBindConfig sessionBindConfig,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " L2tp manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::SessionConfigRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_loc_id(sessionBindConfig.locId);
    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        sessionBindConfig.bhInfo.backhaul));
    request.set_slot_id(sessionBindConfig.bhInfo.slotId);
    request.set_profile_id(sessionBindConfig.bhInfo.profileId);

    grpc::Status reqStatus = stub_->UnBindSessionToBackhaul(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " unbindSessionFromBackhaul request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status L2tpManagerStub::querySessionToBackhaulBindings(
    BackhaulType backhaul, L2tpSessionBindingsResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " L2tp manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::QueryBindSessionRequest request;
    ::dataStub::QueryBindSessionReply response;
    ClientContext context;

    request.set_backhaul_type((::dataStub::BackhaulPreference)backhaul);
    grpc::Status reqStatus = stub_->QueryBindSessionToBackhaul(
        &context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " removeTunnel request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::vector<L2tpSessionBindConfig> bindings;

            for (auto binding : response.session_configs()) {
                L2tpSessionBindConfig config;
                config.locId = binding.loc_id();
                config.bhInfo.backhaul =
                    static_cast<telux::data::BackhaulType>(binding.backhaul_type());
                config.bhInfo.slotId =
                    static_cast<SlotId>(binding.slot_id());
                config.bhInfo.profileId = binding.profile_id();
                bindings.push_back(config);
            }
            auto f1 = std::async(std::launch::async,
                [this, error, callback, bindings, delay]() {
                    callback(bindings, error);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status L2tpManagerStub::registerListener(
    std::weak_ptr<IL2tpListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status L2tpManagerStub::deregisterListener(
    std::weak_ptr<IL2tpListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void L2tpManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IL2tpListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "L2tp Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

} // end of namespace net
} // end of namespace data
} // end of namespace telux
