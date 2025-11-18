/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef QOSMANAGEMENTMENU_HPP
#define QOSMANAGEMENTMENU_HPP

#include "console_app_framework/ConsoleApp.hpp"
#include <memory>
#include <string>

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/TrafficFilter.hpp>
#include <telux/data/net/QoSManager.hpp>

class QoSManagementMenu : public ConsoleApp,
                          public telux::data::net::IQoSListener,
                          public std::enable_shared_from_this<QoSManagementMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // Initialization Callback
    void onInitComplete(telux::common::ServiceStatus status);

    // QoS Manager APIs
    void createTrafficClass(std::vector<std::string> &inputCommand);
    void getAllTrafficClasses(std::vector<std::string> &inputCommand);
    void deleteTrafficClass(std::vector<std::string> &inputCommand);
    void addQoSFilter(std::vector<std::string> &inputCommand);
    void getQosFilter(std::vector<std::string> &inputCommand);
    void getQosFilters(std::vector<std::string> &inputCommand);
    void deleteQosFilter(std::vector<std::string> &inputCommand);
    void deleteAllQosConfigs(std::vector<std::string> &inputCommand);

    QoSManagementMenu(std::string appName, std::string cursor);
    ~QoSManagementMenu();

 private:
    bool initQoSManager();
    std::string qosFilterErrorCodeToString(telux::data::net::QoSFilterErrorCode qosFilterErr);
    std::string tcConfigErrorCodeToString(telux::data::net::TcConfigErrorCode tcErr);
    std::shared_ptr<telux::data::ITrafficFilter> getTrafficFilter();
    void getIPAddressParamsFromUser(
        telux::data::TrafficFilterBuilder &tdBuilder, telux::data::FieldType fieldType);
    void getPortsFromUser(
        telux::data::TrafficFilterBuilder &tdBuilder, telux::data::FieldType fieldType);
    void getVlanInfo(
        telux::data::TrafficFilterBuilder &tdBuilder, telux::data::FieldType fieldType);

    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<telux::data::net::IQoSManager> qosManager_;
};
#endif
