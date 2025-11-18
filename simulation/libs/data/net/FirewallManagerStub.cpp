/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "FirewallManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include "data/DataUtilsStub.hpp"
#include "data/DataFactoryImplStub.hpp"
#include "data/net/FirewallEntryImpl.hpp"
#include "data/IpFilterImpl.hpp"
#include <thread>
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {
namespace net {

FirewallManagerStub::FirewallManagerStub (telux::data::OperationType oprType)
: oprType_(oprType) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IFirewallListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

FirewallManagerStub::~FirewallManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status FirewallManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void FirewallManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::FirewallManager>();

    ::dataStub::InitRequest request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
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

void FirewallManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void FirewallManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

void FirewallManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

std::future<bool> FirewallManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(
        std::launch::async, [&] { return FirewallManagerStub::waitForInitialization(); });
    return future;
}

bool FirewallManagerStub::waitForInitialization() {
    LOG(INFO, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
    if (!isSubsystemReady()) {
        cv_.wait(lock);
    }
    return isSubsystemReady();
}

void FirewallManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus FirewallManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

bool FirewallManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

telux::data::OperationType FirewallManagerStub::getOperationType() {
    LOG(DEBUG, __FUNCTION__);
    return oprType_;
}

telux::common::Status FirewallManagerStub::registerListener(
    std::weak_ptr<IFirewallListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status FirewallManagerStub::deregisterListener(
    std::weak_ptr<IFirewallListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void FirewallManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IFirewallListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Firewall Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::Status FirewallManagerStub::setFirewallConfig(FirewallConfig fwConfig,
        telux::common::ResponseCallback callback) {

    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Firewall manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::SetFirewallRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_profile_id(fwConfig.bhInfo.profileId);
    request.set_fw_enable(fwConfig.enable);
    request.set_allow_packets(fwConfig.allowPackets);
    request.set_slot_id(fwConfig.bhInfo.slotId);
    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        fwConfig.bhInfo.backhaul));
    grpc::Status reqStatus = stub_->SetFirewall(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request failed");
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

telux::common::Status FirewallManagerStub::requestFirewallConfig(BackhaulInfo bhInfo,
        FirewallConfigCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Firewall manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::FirewallStatusRequest request;
    ::dataStub::RequestFirewallStatusReply response;
    ClientContext context;

    request.set_profile_id(bhInfo.profileId);
    request.set_slot_id(bhInfo.slotId);
    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        bhInfo.backhaul));
    grpc::Status reqStatus = stub_->RequestFirewallStatus(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        FirewallConfig config;
        config.enable = response.fw_enable();
        config.allowPackets = response.allow_packets();
        config.bhInfo = bhInfo;

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, config, delay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(config, error);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status FirewallManagerStub::addFirewallEntryRequest(FirewallEntryInfo entry,
    AddFirewallEntryCb callback, bool isHwAccelerated) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Firewall manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay = DEFAULT_DELAY;

    ::dataStub::AddFirewallEntryRequest request;
    ::dataStub::AddFirewallEntryReply response;
    ClientContext context;

    do {
        std::shared_ptr<FirewallEntryImpl> entryImpl
            = std::dynamic_pointer_cast<FirewallEntryImpl>(entry.fwEntry);
        if (!entry.fwEntry || !entryImpl) {
            LOG(ERROR, __FUNCTION__, " Empty firewall entry instance");
            error = telux::common::ErrorCode::INVALID_ARG;
            auto handle = -1;
            if (callback && (delay != SKIP_CALLBACK)) {
                auto f1 = std::async(std::launch::async,
                    [this, error, callback, handle, delay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback(handle, error);
                    }).share();
                taskQ_->add(f1);
            }
            break;
        }

        request.set_slot_id(entry.bhInfo.slotId);
        request.set_profile_id(entry.bhInfo.profileId);
        request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
            entry.bhInfo.backhaul));
        request.set_is_hw_accelerated(isHwAccelerated);
        request.mutable_fw_direction()->set_fw_direction(
            static_cast<::dataStub::Direction::Fw_Direction>(entry.fwEntry->getDirection()));
        request.set_protocol(DataUtilsStub::protocolToString(
            entry.fwEntry->getIProtocolFilter()->getIpProtocol()));

        telux::data::IpFamilyType ipFam = entry.fwEntry->getIpFamilyType();
        request.mutable_ip_family_type()->set_ip_family_type(
            (::dataStub::IpFamilyType::Type)ipFam);

        std::shared_ptr<telux::data::IIpFilter> ipfilter = entry.fwEntry->getIProtocolFilter();
        IPv4Info ipv4Info = ipfilter->getIPv4Info();
        IPv6Info ipv6Info = ipfilter->getIPv6Info();

        if (ipFam == telux::data::IpFamilyType::IPV4) {
            request.mutable_ipv4_params()->set_ipv4_src_address(ipv4Info.srcAddr);
            request.mutable_ipv4_params()->set_ipv4_src_subnet_mask(ipv4Info.srcSubnetMask);
            request.mutable_ipv4_params()->set_ipv4_dest_address(ipv4Info.destAddr);
            request.mutable_ipv4_params()->set_ipv4_dest_subnet_mask(ipv4Info.destSubnetMask);
            request.mutable_ipv4_params()->set_ipv4_tos_val(ipv4Info.value);
            request.mutable_ipv4_params()->set_ipv4_tos_mask(ipv4Info.mask);
        }

        if (ipFam == telux::data::IpFamilyType::IPV6) {
            request.mutable_ipv6_params()->set_ipv6_src_address(ipv6Info.srcAddr);
            request.mutable_ipv6_params()->set_ipv6_dest_address(ipv6Info.destAddr);
            request.mutable_ipv6_params()->set_ipv6_src_prefix_len(ipv6Info.srcPrefixLen);
            request.mutable_ipv6_params()->set_ipv6_dest_prefix_len(ipv6Info.dstPrefixLen);
            request.mutable_ipv6_params()->set_trf_value(ipv6Info.val);
            request.mutable_ipv6_params()->set_trf_mask(ipv6Info.mask);
            request.mutable_ipv6_params()->set_flow_label(ipv6Info.flowLabel);
            request.mutable_ipv6_params()->set_nat_enabled(ipv6Info.natEnabled);
        }

        IpProtocol proto = ipfilter->getIpProtocol();
        switch (proto) {
            case PROTO_TCP:
            case PROTO_TCP_UDP:{
                auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(ipfilter);
                if (tcpFilter) {
                    TcpInfo portInfo_ = tcpFilter->getTcpInfo();
                    request.mutable_protocol_params()->set_source_port(portInfo_.src.port);
                    request.mutable_protocol_params()->set_source_port_range(portInfo_.src.range);
                    request.mutable_protocol_params()->set_dest_port(portInfo_.dest.port);
                    request.mutable_protocol_params()->set_dest_port_range(portInfo_.dest.range);
                }
            } break;
            case PROTO_UDP:{
                auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(ipfilter);
                if (udpFilter) {
                    UdpInfo portInfo_ = udpFilter->getUdpInfo();
                    request.mutable_protocol_params()->set_source_port(portInfo_.src.port);
                    request.mutable_protocol_params()->set_source_port_range(portInfo_.src.range);
                    request.mutable_protocol_params()->set_dest_port(portInfo_.dest.port);
                    request.mutable_protocol_params()->set_dest_port_range(portInfo_.dest.range);
                }
            } break;
            case PROTO_ESP: {
                auto espFilter = std::dynamic_pointer_cast<IEspFilter>(ipfilter);
                if (espFilter) {
                    EspInfo portInfo_ = espFilter->getEspInfo();
                    request.mutable_protocol_params()->set_esp_spi(portInfo_.spi);
                }
            }break;
            case PROTO_ICMP:
            case PROTO_ICMP6: {
                auto icmpFilter = std::dynamic_pointer_cast<IIcmpFilter>(ipfilter);
                if (icmpFilter) {
                    IcmpInfo portInfo_ = icmpFilter->getIcmpInfo();
                    request.mutable_protocol_params()->set_icmp_type(portInfo_.type);
                    request.mutable_protocol_params()->set_icmp_code(portInfo_.code);
                }
            }break;
            default: {
                LOG(ERROR, " Unexpected filter type IpProtocol = ", proto);
            }
        }

        grpc::Status reqStatus = stub_->AddFirewallEntry(&context, request, &response);

        error = static_cast<telux::common::ErrorCode>(response.reply().error());
        status = static_cast<telux::common::Status>(response.reply().status());
        delay = static_cast<int>(response.reply().delay());

        if (status == telux::common::Status::SUCCESS) {
            if (!reqStatus.ok()) {
                LOG(ERROR, __FUNCTION__, " request failed");
                error = telux::common::ErrorCode::INTERNAL_ERROR;
            }
            auto handle = response.handle();

            if (callback && (delay != SKIP_CALLBACK)) {
                auto f1 = std::async(std::launch::async,
                    [this, error, callback, handle, delay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback(handle, error);
                    }).share();
                taskQ_->add(f1);
            }
        }
    } while(0);

    return status;
}

telux::common::Status FirewallManagerStub::getFirewallEntriesRequest(BackhaulInfo bhInfo,
    FirewallEntryInfoCb callback, bool isHwAccelerated) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Firewall manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::FirewallEntriesRequest request;
    ::dataStub::RequestFirewallEntriesReply response;
    ClientContext context;

    request.set_slot_id(bhInfo.slotId);
    request.set_profile_id(bhInfo.profileId);
    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        bhInfo.backhaul));
    request.set_is_hw_accelerated(isHwAccelerated);
    grpc::Status reqStatus = stub_->RequestFirewallEntries(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        std::vector<FirewallEntryInfo> fwEntries;
        IPv4Info ipv4Info;
        IPv6Info ipv6Info;

        for (auto& entry: response.firewall_entries()) {
            FirewallEntryInfo fwEntryInfo;
            fwEntryInfo.bhInfo = bhInfo;

            telux::data::Direction fw_direction =
                (telux::data::Direction)entry.fw_direction().fw_direction();
            std::string protocol = entry.protocol();
            telux::data::IpFamilyType ip_family_type =
                ((telux::data::IpFamilyType)entry.ip_family_type().ip_family_type());
             auto fwEntry = DataFactoryImplStub::getInstance().getNewFirewallEntry(
                DataUtilsStub::stringToProtocol(protocol), fw_direction, ip_family_type);
            if(fwEntry) {
                auto fwEntryImpl = std::dynamic_pointer_cast<FirewallEntryImpl>(fwEntry);
                std::shared_ptr<telux::data::IIpFilter> ipFilter =
                    fwEntry->getIProtocolFilter();
                if(fwEntryImpl) {
                    if (ip_family_type == telux::data::IpFamilyType::IPV4) {
                        ipv4Info.srcAddr = entry.ipv4_params().ipv4_src_address();
                        ipv4Info.srcSubnetMask = entry.ipv4_params().ipv4_src_subnet_mask();
                        ipv4Info.destAddr = entry.ipv4_params().ipv4_dest_address();
                        ipv4Info.destSubnetMask = entry.ipv4_params().ipv4_dest_subnet_mask();
                        ipv4Info.value = entry.ipv4_params().ipv4_tos_val();
                        ipv4Info.mask = entry.ipv4_params().ipv4_tos_mask();
                        ipFilter->setIPv4Info(ipv4Info);
                    }

                    if (ip_family_type == telux::data::IpFamilyType::IPV6) {
                        ipv6Info.srcAddr = entry.ipv6_params().ipv6_src_address();
                        ipv6Info.destAddr = entry.ipv6_params().ipv6_dest_address();
                        ipv6Info.srcPrefixLen = entry.ipv6_params().ipv6_src_prefix_len();
                        ipv6Info.dstPrefixLen = entry.ipv6_params().ipv6_dest_prefix_len();
                        ipv6Info.val = entry.ipv6_params().trf_value();
                        ipv6Info.mask = entry.ipv6_params().trf_mask();
                        ipv6Info.flowLabel = entry.ipv6_params().flow_label();
                        ipv6Info.natEnabled = entry.ipv6_params().nat_enabled();
                        ipFilter->setIPv6Info(ipv6Info);
                    }

                    IpProtocol proto = DataUtilsStub::stringToProtocol(protocol);
                    switch (proto) {
                        case PROTO_TCP:{
                            auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(ipFilter);
                            if (tcpFilter) {
                                TcpInfo portInfo_;
                                portInfo_.src.port = entry.protocol_params().source_port();
                                portInfo_.src.range = entry.protocol_params().source_port_range();
                                portInfo_.dest.port = entry.protocol_params().dest_port();
                                portInfo_.dest.range = entry.protocol_params().dest_port_range();
                                tcpFilter->setTcpInfo(portInfo_);
                            }
                        } break;
                        case PROTO_UDP:{
                            auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(ipFilter);
                            if (udpFilter) {
                                UdpInfo portInfo_;
                                portInfo_.src.port = entry.protocol_params().source_port();
                                portInfo_.src.range = entry.protocol_params().source_port_range();
                                portInfo_.dest.port = entry.protocol_params().dest_port();
                                portInfo_.dest.range = entry.protocol_params().dest_port_range();
                                udpFilter->setUdpInfo(portInfo_);
                            }
                        } break;
                        case PROTO_ESP: {
                            auto espFilter = std::dynamic_pointer_cast<IEspFilter>(ipFilter);
                            if (espFilter) {
                                EspInfo portInfo_;
                                portInfo_.spi = entry.protocol_params().esp_spi();
                                espFilter->setEspInfo(portInfo_);
                            }
                        }break;
                        case PROTO_ICMP:
                        case PROTO_ICMP6: {
                            auto icmpFilter = std::dynamic_pointer_cast<IIcmpFilter>(ipFilter);
                            if (icmpFilter) {
                                IcmpInfo portInfo_;
                                portInfo_.type = entry.protocol_params().icmp_type();
                                portInfo_.code = entry.protocol_params().icmp_code();
                                icmpFilter->setIcmpInfo(portInfo_);
                            }
                        }break;
                        default: {
                            LOG(ERROR, " Unexpected filter type IpProtocol = ", proto);
                        }
                    }

                    fwEntryImpl->setHandle(entry.firewall_handle());
                    fwEntryInfo.fwEntry = fwEntry;
                    fwEntries.push_back(fwEntryInfo);
                }
            }
        }
        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, fwEntries, delay]() {
                    callback(fwEntries, error);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status FirewallManagerStub::addHwAccelerationFirewallEntry(FirewallEntryInfo entry,
    AddFirewallEntryCb callback) {
    LOG(DEBUG, __FUNCTION__);
    return this->addFirewallEntryRequest(entry, callback, true);
}

telux::common::Status FirewallManagerStub::requestHwAccelerationFirewallEntries(
    BackhaulInfo bhInfo, FirewallEntryInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    return this->getFirewallEntriesRequest(bhInfo, callback, true);
}

telux::common::Status FirewallManagerStub::addFirewallEntry(FirewallEntryInfo entry,
    AddFirewallEntryCb callback) {
    return this->addFirewallEntryRequest(entry, callback);
}

telux::common::Status FirewallManagerStub::requestFirewallEntries(BackhaulInfo bhInfo,
    FirewallEntryInfoCb callback) {
    return this->getFirewallEntriesRequest(bhInfo, callback);
}

telux::common::Status FirewallManagerStub::removeFirewallEntry(BackhaulInfo bhInfo,
    uint32_t handle, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Firewall manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::RemoveFirewallEntryRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_slot_id(bhInfo.slotId);
    request.set_profile_id(bhInfo.profileId);
    request.set_entry_handle(handle);
    grpc::Status reqStatus = stub_->RemoveFirewallEntry(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request failed");
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

telux::common::Status FirewallManagerStub::enableDmz(DmzConfig config,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Firewall manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::EnableDMZRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    // currently supported backhauls are WWAN, WLAN and ETH
    if (config.bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
        request.set_slot_id(config.bhInfo.slotId);
        request.set_profile_id(config.bhInfo.profileId);
    } else if (config.bhInfo.backhaul == telux::data::BackhaulType::ETH) {
        request.set_vlan_id(config.bhInfo.vlanId);
    }

    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        config.bhInfo.backhaul));
    request.set_ip_address(config.ipAddr);
    grpc::Status reqStatus = stub_->EnableDMZ(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request failed");
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

telux::common::Status FirewallManagerStub::disableDmz(BackhaulInfo bhInfo,
    const IpFamilyType ipType, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Firewall manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::DisableDmzRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    // currently supported backhauls are WWAN, WLAN and ETH
    if (bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
        request.set_slot_id(bhInfo.slotId);
        request.set_profile_id(bhInfo.profileId);
    } else if (bhInfo.backhaul == telux::data::BackhaulType::ETH) {
        request.set_vlan_id(bhInfo.vlanId);
    }

    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        bhInfo.backhaul));
    request.mutable_ip_family_type()->set_ip_family_type((::dataStub::IpFamilyType::Type)ipType);
    grpc::Status reqStatus = stub_->DisableDMZ(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request failed");
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

telux::common::Status FirewallManagerStub::requestDmzEntry(BackhaulInfo bhInfo,
    DmzEntryInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Firewall manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::DMZEntryRequest request;
    ::dataStub::RequestDMZEntryReply response;
    ClientContext context;

    // currently supported backhauls are WWAN, WLAN and ETH
    if (bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
        request.set_slot_id(bhInfo.slotId);
        request.set_profile_id(bhInfo.profileId);
    } else if (bhInfo.backhaul == telux::data::BackhaulType::ETH) {
        request.set_vlan_id(bhInfo.vlanId);
    }

    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        bhInfo.backhaul));

    grpc::Status reqStatus = stub_->RequestDMZEntry(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::vector<DmzConfig> dmzEntries;
            for (auto& entry: response.dmz_entries()) {
                DmzConfig config;
                config.bhInfo = bhInfo;
                config.ipAddr = entry;
                dmzEntries.push_back(config);
            }
            auto f1 = std::async(std::launch::async,
                [this, error, callback, dmzEntries, delay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(dmzEntries, error);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status FirewallManagerStub::requestFirewallStatus(int profileId,
    FirewallStatusCb callback, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status FirewallManagerStub::setFirewall(int profileId, bool enable,
    bool allowPackets, telux::common::ResponseCallback callback, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status FirewallManagerStub::addFirewallEntry(int profileId,
    std::shared_ptr<IFirewallEntry> entry,
    telux::common::ResponseCallback callback, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status FirewallManagerStub::addHwAccelerationFirewallEntry(int profileId,
    std::shared_ptr<IFirewallEntry> entry, AddFirewallEntryCb callback,
    SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status FirewallManagerStub::requestHwAccelerationFirewallEntries(
    int profileId, FirewallEntriesCb callback, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status FirewallManagerStub::requestFirewallEntries(int profileId,
    FirewallEntriesCb callback, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status FirewallManagerStub::removeFirewallEntry(int profileId, uint32_t handle,
    telux::common::ResponseCallback callback, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status FirewallManagerStub::enableDmz(int profileId, const std::string ipAddr,
    telux::common::ResponseCallback callback , SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status FirewallManagerStub::disableDmz(int profileId,
    const telux::data::IpFamilyType ipType,
    telux::common::ResponseCallback callback, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status FirewallManagerStub::requestDmzEntry(int profileId,
    DmzEntriesCb dmzCb, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

} // end of namespace net
} // end of namespace data
} // end of namespace telux
