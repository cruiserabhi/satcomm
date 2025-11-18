/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Data Filter Manager Sample Application using Telematics SDK
 * This is used to demonstrate data filter manager APIs like enable/disable data
 * filter mode and add/remove data filters.
 */

#ifndef DATAFILTERMENU_HPP
#define DATAFILTERMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>

#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataProfile.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/tel/PhoneManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"

#include "../DataListener.hpp"
#include "../DataResponseCallback.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataFilterManager.hpp>
#include <telux/data/DataFilterListener.hpp>
#include <telux/data/DataFilterListener.hpp>

#include "MyDataFilterListener.hpp"
#include "DataConfigParser.hpp"

using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;

class DataFilterMenu : public IDataFilterListener, public ConsoleApp {
 public:

    // initialize menu and sdk
    bool init();

    // Data Filter APIs
    void sendSetDataRestrictMode(DataRestrictMode mode);
    void getFilterMode();
    void addFilter();
    void removeAllFilter();

    IpProtocol getTypeOfFilter(DataConfigParser instance,
            std::map<std::string, std::string> filter);
    void addIPParameters(std::shared_ptr<telux::data::IIpFilter> &dataFilter,
        DataConfigParser instance, std::map<std::string, std::string> filterMap);
    void commandCallback(ErrorCode errorCode);
    int getPortInfo(DataConfigParser cfgParser, std::map<std::string, std::string> pairMap,
        std::string key, std::string errorStr);

    DataFilterMenu(std::string appName, std::string cursor);
    ~DataFilterMenu();

 private:
    bool initDataFilterManagerAndListener(SlotId slotId);

    std::shared_ptr<telux::tel::IPhoneManager> phoneManager_;
    std::map<SlotId, std::shared_ptr<telux::data::IDataConnectionManager>> dataConnManagerMap_;
    telux::data::ProfileParams params_;
    std::map<SlotId, std::shared_ptr<DataListener>> dataListener_;

    std::map<SlotId, std::shared_ptr<telux::data::IDataFilterManager>> dataFilterManagerMap_;
    std::map<SlotId, std::shared_ptr<MyDataFilterListener>> dataFilterListener_;
    std::map<SlotId, ResponseCallback> responseCbMap_;

};
#endif
