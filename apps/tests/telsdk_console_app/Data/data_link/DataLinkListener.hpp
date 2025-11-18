/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATALINKLISTENER_HPP
#define DATALINKLISTENER_HPP

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataLinkManager.hpp>

class DataLinkListener : public telux::data::IDataLinkListener {
public:
    DataLinkListener();
    ~DataLinkListener();
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onEthModeChangeRequest(telux::data::EthModeType ethModeType) override;
    void onEthModeChangeTransactionStatus(telux::data::EthModeType ethModeType,
        telux::data::LinkModeChangeStatus status) override;
    void onEthDataLinkStateChange(telux::data::LinkState linkState) override;

    static std::string ethModeTypeToString(telux::data::EthModeType ethModeType);
    static std::string linkModeChangeStatusToString(telux::data::LinkModeChangeStatus status);

};

#endif  // DATALINKLISTENER_HPP
