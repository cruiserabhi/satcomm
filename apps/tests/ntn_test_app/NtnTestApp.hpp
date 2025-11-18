/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NTNTESTAPP_HPP
#define NTNTESTAPP_HPP

#include <memory>

#include "ConsoleApp.hpp"
#include <telux/satcom/NtnManager.hpp>

using namespace telux::satcom;
using namespace telux::common;

class NtnTestApp : public INtnListener,
                   public ConsoleApp,
                   public std::enable_shared_from_this<NtnTestApp> {
 public:
    NtnTestApp();
    ~NtnTestApp();

    void registerForUpdates();
    void deregisterForUpdates();
    void consoleInit();

    std::string toString(NtnState state);
    std::string toString(NtnCapabilities cap);
    std::string toString(SignalStrength ss);
    std::string toString(ServiceStatus status);
    void onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size);
    void onNtnStateChange(NtnState state);
    void onCapabilitiesChange(NtnCapabilities capabilities);
    void onSignalStrengthChange(SignalStrength newStrength);
    void onServiceStatusChange(ServiceStatus status);
    void onDataAck(ErrorCode err, TransactionId id);
    void onCellularCoverageAvailable(bool isCellularCoverageAvailable);
    void getServiceStatus(std::vector<std::string> inputCommand);
    void isNtnSupported(std::vector<std::string> inputCommand);
    void enableNtn(std::vector<std::string> inputCommand);
    void sendDataString(std::vector<std::string> inputCommand);
    void sendDataRaw(std::vector<std::string> inputCommand);
    void abortData(std::vector<std::string> inputCommand);
    void getNtnCapabilities(std::vector<std::string> inputCommand);
    void updateSystemSelectionSpecifiers(std::vector<std::string> inputCommand);
    void getNtnState(std::vector<std::string> inputCommand);
    void enableCellularScan(std::vector<std::string> inputCommand);
    // Member variable to keep the manager object alive till application ends.
    std::shared_ptr<telux::satcom::INtnManager> ntnMgr_ = nullptr;

};

#endif  // NTNTESTAPP_HPP
