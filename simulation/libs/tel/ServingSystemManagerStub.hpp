/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ServingSystemManagerStub.hpp
 *
 * @brief      Implementation of ServingSystemManager
 *
 */

#ifndef SERVING_SYSTEM_MANAGER_STUB_HPP
#define SERVING_SYSTEM_MANAGER_STUB_HPP

#include <telux/tel/ServingSystemManager.hpp>

#include "common/event-manager/ClientEventManager.hpp"
#include "common/ListenerManager.hpp"

#include "protos/proto-src/tel_simulation.grpc.pb.h"

namespace telux {
namespace tel {

class RFBandList : public IRFBandList {
 public:
    RFBandList();
    ~RFBandList();

    void setGsmBands(std::vector<GsmRFBand> bands) override;
    void setWcdmaBands(std::vector<WcdmaRFBand> bands) override;
    void setLteBands(std::vector<LteRFBand> bands) override;
    void setNrBands(NrType type, std::vector<NrRFBand> bands) override;
    std::vector<GsmRFBand> getGsmBands() override;
    std::vector<WcdmaRFBand> getWcdmaBands() override;
    std::vector<LteRFBand> getLteBands() override;
    std::vector<NrRFBand> getNrBands(NrType type) override;
    bool isGSMBandPresent(GsmRFBand band) override;
    bool isWcdmaBandPresent(WcdmaRFBand band) override;
    bool isLteBandPresent(LteRFBand band) override;
    bool isNrBandPresent(NrType type, NrRFBand band) override;

 private:
    std::vector<GsmRFBand> gsmBands_;
    std::vector<WcdmaRFBand> wcdmaBands_;
    std::vector<LteRFBand> lteBands_;
    std::vector<NrRFBand> saBands_;
    std::vector<NrRFBand> nsaBands_;
    std::vector<NrRFBand> nrBands_;
    std::mutex mtx_;
};

class ServingSystemManagerStub : public IServingSystemManager,
                                 public IEventListener,
                                 public std::enable_shared_from_this<ServingSystemManagerStub> {
public:
    ServingSystemManagerStub(int phoneId);
    telux::common::Status init(telux::common::InitResponseCb callback);
    ~ServingSystemManagerStub();

    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status setRatPreference(RatPreference ratPref,
        common::ResponseCallback callback) override;
    telux::common::Status requestRatPreference(RatPreferenceCallback callback) override;

    telux::common::Status setServiceDomainPreference(ServiceDomainPreference serviceDomain,
        common::ResponseCallback callback) override;
    telux::common::Status requestServiceDomainPreference(
        ServiceDomainPreferenceCallback callback) override;

    telux::common::Status getSystemInfo(ServingSystemInfo &sysInfo) override;
    telux::tel::DcStatus getDcStatus() override;
    telux::common::Status requestNetworkTime(NetworkTimeResponseCallback callback) override;
    telux::common::Status requestLteSib16NetworkTime(NetworkTimeResponseCallback callback)
        override;
    telux::common::Status requestNr5gRrcUtcTime(NetworkTimeResponseCallback callback) override;
    telux::common::Status requestRFBandInfo(RFBandInfoCallback callback) override;
    telux::common::Status getNetworkRejectInfo(NetworkRejectInfo &rejectInfo) override;
    telux::common::Status getCallBarringInfo(std::vector<CallBarringInfo> &barringInfo) override;
    telux::common::Status getSmsCapabilityOverNetwork(SmsCapability &smsCapability) override;
    telux::common::Status getLteCsCapability(LteCsCapability &lteCapability) override;
    telux::common::Status requestRFBandPreferences(RFBandPrefCallback callback) override;
    telux::common::Status setRFBandPreferences(std::shared_ptr<IRFBandList> prefList,
        common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestRFBandCapability(RFBandCapabilityCallback callback) override;
    telux::common::Status registerListener(std::weak_ptr<IServingSystemListener> listener,
        ServingSystemNotificationMask mask) override;
    telux::common::Status deregisterListener(std::weak_ptr<IServingSystemListener> listener,
        ServingSystemNotificationMask mask) override;

    void cleanup();
    void onEventUpdate(google::protobuf::Any event)  override;

private:
    int phoneId_;
    std::mutex mtx_;
    std::condition_variable cv_;
    telux::common::InitResponseCb initCb_;
    int cbDelay_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_ = nullptr;
    std::shared_ptr<telux::common::ListenerManager<IServingSystemListener>> listenerMgr_ = nullptr;
    std::unique_ptr<::telStub::ServingSystemService::Stub> stub_;
    telux::common::ServiceStatus subSystemStatus_;
    bool ready_ = false;
    bool waitForInitialization();
    void setSubsystemReady(bool status);
    void setServiceStatus(telux::common::ServiceStatus status);
    void initSync();
    void handleSystemInfoChanged(::telStub::SystemInfoEvent event);
    void handleSystemSelectionPreferenceChanged(::telStub::SystemSelectionPreferenceEvent event);
    void handleNetworkTimeChange(::telStub::NetworkTimeInfoEvent event);
    void handleNetworkRejection(::telStub::NetworkRejectInfoEvent event);
    void handleRfBandInfoUpdateEvent(::telStub::RFBandInfoEvent event);
};

} // end of namespace tel

} // end of namespace telux

#endif // SERVING_SYSTEM_MANAGER_STUB_HPP
