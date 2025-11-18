/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _TELUX_CV2X_RADIO_STUB_HPP_
#define _TELUX_CV2X_RADIO_STUB_HPP_

#include <future>

#include "Cv2xRadioHelperStub.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>

#include "common/event-manager/ClientEventManager.hpp"

#include "protos/proto-src/cv2x_simulation.grpc.pb.h"

namespace telux {
namespace cv2x {

class Cv2xRadioEvtListener : public telux::common::IEventListener {
 public:
    Cv2xRadioEvtListener(std::shared_ptr<Cv2xRadioCapabilities> caps);
    void onEventUpdate(google::protobuf::Any event) override;
    telux::common::Status registerListener(
        std::weak_ptr<telux::cv2x::ICv2xRadioListener> listener);
    telux::common::Status deregisterListener(
        std::weak_ptr<telux::cv2x::ICv2xRadioListener> listener);

 private:
    telux::common::ListenerManager<telux::cv2x::ICv2xRadioListener> listenerMgr_;
    void onCv2xStatusChange(telux::cv2x::Cv2xStatus &status);
    void onL2AddrChanged(uint32_t newL2Address);
    void onDuplicateAddr(const bool detected);
    void onSpsScheduleInfo(const ::cv2xStub::SpsSchedulingInfo& schedulingInfo);
    void onCapabilitiesChange(const ::cv2xStub::RadioCapabilites& caps);
    std::shared_ptr<Cv2xRadioCapabilities> caps_ = nullptr;
};

class Cv2xRadioSimulation : public ICv2xRadio,
                            public ICv2xRadioListener,
                            public std::enable_shared_from_this<Cv2xRadioSimulation> {
 public:
    Cv2xRadioSimulation();
    ~Cv2xRadioSimulation();

    void init(telux::common::InitResponseCb callback);

    bool isInitialized() const override;
    bool isReady() const override;
    std::future<telux::common::Status> onReady() override;

    telux::common::Status registerListener(std::weak_ptr<ICv2xRadioListener> listener) override;

    telux::common::Status deregisterListener(std::weak_ptr<ICv2xRadioListener> listener) override;

    telux::common::Status createRxSubscription(TrafficIpType ipType, uint16_t port,
        CreateRxSubscriptionCallback cb,
        std::shared_ptr<std::vector<uint32_t>> idList = nullptr) override;

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status enableRxMetaDataReport(TrafficIpType ipType, bool enable,
        std::shared_ptr<std::vector<std::uint32_t>> idList,
        telux::common::ResponseCallback cb) override;

    telux::common::Status createTxSpsFlow(TrafficIpType ipType, uint32_t serviceId,
        const SpsFlowInfo &spsInfo, uint16_t spsSrcPort, bool eventSrcPortValid,
        uint16_t eventSrcPort, CreateTxSpsFlowCallback cb) override;

    telux::common::Status createTxEventFlow(TrafficIpType ipType, uint32_t serviceId,
        uint16_t eventSrcPort, CreateTxEventFlowCallback cb) override;

    telux::common::Status createTxEventFlow(TrafficIpType ipType, uint32_t serviceId,
        const EventFlowInfo &flowInfo, uint16_t eventSrcPort,
        CreateTxEventFlowCallback cb) override;

    telux::common::Status closeRxSubscription(
        std::shared_ptr<ICv2xRxSubscription> rxSub, CloseRxSubscriptionCallback cb) override;

    telux::common::Status closeTxFlow(
        std::shared_ptr<ICv2xTxFlow> txFlow, CloseTxFlowCallback cb) override;

    telux::common::Status changeSpsFlowInfo(std::shared_ptr<ICv2xTxFlow> txFlow,
        const SpsFlowInfo &spsInfo, ChangeSpsFlowInfoCallback cb) override;

    telux::common::Status requestSpsFlowInfo(
        std::shared_ptr<ICv2xTxFlow> txFlow, RequestSpsFlowInfoCallback cb) override;

    telux::common::Status changeEventFlowInfo(std::shared_ptr<ICv2xTxFlow> txFlow,
        const EventFlowInfo &flowInfo, ChangeEventFlowInfoCallback cb) override;

    telux::common::Status updateSrcL2Info(UpdateSrcL2InfoCallback cb) override;

    telux::common::Status updateTrustedUEList(
        const TrustedUEInfoList &infoList, UpdateTrustedUEListCallback cb) override;

    std::string getIfaceNameFromIpType(TrafficIpType ipType) override;

    telux::common::Status createCv2xTcpSocket(const EventFlowInfo &eventInfo,
        const SocketInfo &sockInfo, CreateTcpSocketCallback cb) override;

    telux::common::Status closeCv2xTcpSocket(
        std::shared_ptr<ICv2xTxRxSocket> sock, CloseTcpSocketCallback cb) override;

    telux::common::Status registerTxStatusReportListener(uint16_t port,
        std::shared_ptr<ICv2xTxStatusReportListener> listener,
        telux::common::ResponseCallback cb) override;

    telux::common::Status deregisterTxStatusReportListener(
        uint16_t port, telux::common::ResponseCallback cb) override;

    telux::common::Status setGlobalIPInfo(
        const IPv6AddrType &ipv6Addr, common::ResponseCallback cb) override;

    telux::common::Status setGlobalIPUnicastRoutingInfo(
        const GlobalIPUnicastRoutingInfo &destL2Addr, common::ResponseCallback cb) override;

    telux::common::Status requestCapabilities(RequestCapabilitiesCallback cb) override;

    telux::common::Status requestDataSessionSettings(
        RequestDataSessionSettingsCallback cb) override;

    Cv2xRadioCapabilities getCapabilities() const override;

    telux::common::Status
        injectVehicleSpeed(uint32_t speed, telux::common::ResponseCallback cb) override;

 private:
    common::Status waitForInitialization();
    void setInitializedStatus(telux::common::Status status, telux::common::InitResponseCb cb);
    int getV6AddrByIface(string &ifaceName, struct in6_addr &addr);
    void onStatusChanged(cv2x::Cv2xStatus status);
    void createRxSubscriptionSync(TrafficIpType ipType, uint16_t port,
        CreateRxSubscriptionCallback cb, shared_ptr<vector<uint32_t>> idList);
    telux::common::Status initRxSock(
        cv2x::TrafficIpType ipType, int &sock, uint16_t port, struct sockaddr_in6 &sockAddr);

    template <class T>
    telux::common::Status addFlow(
        std::shared_ptr<T> flow, std::map<uint32_t, std::shared_ptr<ICv2xTxFlow>> &vec);
    template <class T>
    telux::common::Status removeFlow(
        std::shared_ptr<T> flow, std::map<uint32_t, std::shared_ptr<ICv2xTxFlow>> &vec);
    telux::common::Status addSubscription(shared_ptr<ICv2xRxSubscription> sub);
    telux::common::Status removeSubscription(shared_ptr<ICv2xRxSubscription> sub);

    telux::common::Status initTxUdpSock(
        TrafficIpType ipType, int &sock, uint16_t port, struct sockaddr_in6 &sockAddr);
    telux::common::ErrorCode initTxSpsFlow(TrafficIpType ipType, uint32_t serviceId,
        const SpsFlowInfo &spsInfo, uint16_t spsSrcPort, bool eventSrcPortValid,
        uint16_t eventSrcPort, shared_ptr<ICv2xTxFlow> &txSpsFlow,
        shared_ptr<ICv2xTxFlow> &txEventFlow, telux::common::Status &spsStatus,
        telux::common::Status &eventStatus, int &delay);

    telux::common::Status createTxSpsFlowSync(TrafficIpType ipType, uint32_t serviceId,
        const SpsFlowInfo &spsInfo, uint16_t spsSrcPort, bool eventSrcPortValid,
        uint16_t eventSrcPort, CreateTxSpsFlowCallback cb);

    telux::common::ErrorCode initTxEventFlow(TrafficIpType ipType, uint32_t serviceId,
        const EventFlowInfo &flowInfo, uint16_t eventSrcPort, CreateTxEventFlowCallback cb);

    telux::common::Status closeRxSubscriptionSync(
        std::shared_ptr<ICv2xRxSubscription> rxSub, CloseRxSubscriptionCallback cb);

    telux::common::ErrorCode closeTxSpsFlowSync(
        std::shared_ptr<ICv2xTxFlow> txFlow, CloseTxFlowCallback cb);
    telux::common::ErrorCode closeTxEventFlowsSync(
        vector<shared_ptr<ICv2xTxFlow>> &txFlows, CloseTxFlowCallback cb);

    int getSockAddr(TrafficIpType ipType, uint16_t port, struct sockaddr_in6 &sockAddr);
    telux::common::Status initTcpSock(
        const SocketInfo &sockInfo, int &sock, struct sockaddr_in6 &sockAddr);
    void closeTcpSock(int sock);
    bool isTcpSocketPresent(uint32_t serviceId, bool isExclId, uint32_t exclId);
    telux::common::Status addTcpSocket(shared_ptr<ICv2xTxRxSocket> sock);
    telux::common::Status removeTcpSocket(shared_ptr<ICv2xTxRxSocket> sock);
    telux::common::Status createCv2xTcpSocketSync(
        const EventFlowInfo &eventInfo, const SocketInfo &sockInfo, CreateTcpSocketCallback cb);
    telux::common::Status closeCv2xTcpSocketSync(
        shared_ptr<ICv2xTxRxSocket> sock, CloseTcpSocketCallback cb);

    void closeAllCv2xTcpSockets();
    void unsubscribeAllRxSubs();
    void cleanupAllEventFlows();
    void cleanupAllSpsFlows();
    int getMTU(std::string interfaceName);

    std::shared_ptr<Cv2xRadioCapabilities> caps_ = nullptr;
    std::map<TrafficIpType, std::string> ifaces_;
    std::mutex mutex_;
    std::condition_variable initializedCv_;
    /*default telux::common::Status::NOTREADY means radio not complete
     * initialization yet*/
    telux::common::Status initializedStatus_    = telux::common::Status::NOTREADY;
    telux::common::ServiceStatus serviceStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

    std::recursive_mutex rxSubscriptionsMutex_;
    std::map<uint32_t, std::shared_ptr<ICv2xRxSubscription>> rxSubscriptions_;

    std::recursive_mutex flowsMutex_;
    std::map<uint32_t, std::shared_ptr<ICv2xTxFlow>> eventFlows_;
    std::map<uint32_t, std::shared_ptr<ICv2xTxFlow>> spsFlows_;

    // map of socket ID to TCP sockets
    std::recursive_mutex tcpSockMutex_;
    std::map<uint32_t, std::shared_ptr<ICv2xTxRxSocket>> tcpSockets_;

    std::shared_ptr<Cv2xRadioEvtListener> pEvtListener_ = nullptr;
    std::map<uint16_t, std::weak_ptr<ICv2xTxStatusReportListener>> txStatusListeners_;
    std::mutex txStatusMtx_;
    std::unique_ptr<::cv2xStub::Cv2xRadioService::Stub> serviceStub_ = nullptr;

    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_ = nullptr;
};

}  // namespace cv2x
}  // namespace telux
#endif
