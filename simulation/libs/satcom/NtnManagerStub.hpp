/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NTNMANAGERIMPL_HPP
#define NTNMANAGERIMPL_HPP

#include <telux/satcom/NtnManager.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/satcom_simulation.grpc.pb.h"

namespace telux {
namespace satcom {

class IQmsNtnClientListener;

class NtnManagerStub : public INtnManager, public INtnListener {
 public:
    NtnManagerStub();
    ~NtnManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);
    void initSync(telux::common::InitResponseCb callback);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    telux::common::ServiceStatus getServiceStatus();

    telux::common::Status registerListener(std::weak_ptr<INtnListener> listener) override;

    telux::common::Status deregisterListener(std::weak_ptr<INtnListener> listener) override;

    telux::common::ErrorCode isNtnSupported(bool &isSupported) override;
    telux::common::ErrorCode enableNtn(bool enable, bool isEmergency, const std::string &iccid)
        override;
    telux::common::Status sendData(
        uint8_t *data, uint32_t size, bool isEmergency, TransactionId &TransactionId) override;
    telux::common::ErrorCode abortData() override;
    telux::common::ErrorCode getNtnCapabilities(NtnCapabilities &capabilities) override;
    telux::common::ErrorCode updateSystemSelectionSpecifiers(
        std::vector<SystemSelectionSpecifier> &params) override;
    NtnState getNtnState() override;
    telux::common::ErrorCode getSignalStrength(SignalStrength &signalStrength) override;
    telux::common::ErrorCode enableCellularScan(bool enable) override;

    void onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size) override;
    void onDataAck(telux::common::ErrorCode err, telux::satcom::TransactionId id) override;
    void onSignalStrengthChange(telux::satcom::SignalStrength newStrength) override;
    void onCapabilitiesChange(telux::satcom::NtnCapabilities capabilities) override;
    void onNtnStateChange(telux::satcom::NtnState state) override;
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onCellularCoverageAvailable(bool isCellularCoverageAvailable) override;

 private:
    std::mutex mtx_;
    std::mutex initMtx_;
    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::satcomStub::NtnManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<INtnListener>> listenerMgr_;
};

}  // namespace satcom
}  // namespace telux

#endif  // NTNMANAGERIMPL_HPP
