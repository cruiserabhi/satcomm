/*
 *  Copyright (c) 2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DATAFILTERCONTROLLER_HPP
#define DATAFILTERCONTROLLER_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>

#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataFilterManager.hpp>
#include <telux/data/DataFilterListener.hpp>
#include <telux/common/Log.hpp>

#include "DataConfigParser.hpp"

#define DEFAULT_PROFILE 1

using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;
using namespace std;

/**
 * @brief Data filter controller deals with communication with telsdk with respect to filter
 * management and connection information
 *
 */

class DataFilterController : public enable_shared_from_this<DataFilterController>
{
public:
    bool initializeSDK(std::function<void(bool isActive)> defaultDataCallUpdateCb);

    // Data Filter APIs
    bool sendSetDataRestrictMode(DataRestrictMode mode);
    bool getFilterMode();
    bool addFilter();
    bool removeAllFilter();
    int getDefaultProfile();
    // check if data call on default profile is already active
    bool isDefaultDataCallUp();
    bool isDataCallExist();

    IpProtocol getTypeOfFilter(DataConfigParser instance,
        std::map<std::string, std::string> filter);
    void addIPParameters(std::shared_ptr<telux::data::IIpFilter> &dataFilter,
        DataConfigParser instance, std::map<std::string, std::string> filterMap);
    ResponseCallback responseCb;
    void commandCallback(ErrorCode errorCode);
    int getPortInfo(DataConfigParser cfgParser, std::map<std::string, std::string> pairMap,
        std::string key, std::string errorStr);
    std::shared_ptr<telux::data::IIpFilter> configureTCPFilter( DataConfigParser cfgParser,
        std::map<std::string, std::string> filter);
    std::shared_ptr<telux::data::IIpFilter> configureUDPFilter(DataConfigParser cfgParser,
        std::map<std::string, std::string> filter);
    bool isUDP();

    DataFilterController();
    ~DataFilterController();

private:
    bool isDataFilterMgrReady_ = false;
    bool isConnectionMgrReady_ = false;
    std::condition_variable cvDataFilterMgrReady_;

    std::shared_ptr<telux::data::IDataConnectionManager> dataConnectionManager_;
    std::shared_ptr<telux::data::IDataFilterManager> dataFilterMgr_;

    std::function<void(bool isActive)> defaultDataCallUpdateCb_;

    /** Listener to update change in data filter info */
    class DataFilterListener : public telux::data::IDataFilterListener {
        std::weak_ptr<DataFilterController> dataController_;
        public:
        DataFilterListener(std::weak_ptr<DataFilterController> dataController);

        void onDataRestrictModeChange(DataRestrictMode mode) override;
        void onServiceStatusChange(telux::common::ServiceStatus status) override;
    };
    std::shared_ptr<DataFilterListener> dataFilterListener_;

    /**
     * Listener to update change in data call info
     */
    class DataConnectionListener : public telux::data::IDataConnectionListener {

        std::weak_ptr<DataFilterController> dataController_;
        public:
        DataConnectionListener(std::weak_ptr<DataFilterController> DataFilterController);
        void onDataCallInfoChanged(
            const std::shared_ptr<telux::data::IDataCall> &dataCall) override;
        void onServiceStatusChange(telux::common::ServiceStatus status) override;

        private:
        void logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall);
    };
    std::shared_ptr<DataConnectionListener> dataConnectionListener_;


};
#endif
