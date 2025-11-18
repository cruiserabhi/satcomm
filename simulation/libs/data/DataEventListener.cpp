/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>

#include "DataEventListener.hpp"
#include "DataConnectionManagerStub.hpp"
#include "DataUtilsStub.hpp"

namespace telux {
namespace data {

DataEventListener::DataEventListener(std::weak_ptr<DataConnectionManagerStub> manager)
: dataConnectionMngr_(manager) {
    LOG(DEBUG, __FUNCTION__);
}

DataEventListener::~DataEventListener() {
    LOG(DEBUG, __FUNCTION__);
}

void DataEventListener::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    auto mngr = dataConnectionMngr_.lock();

    if(mngr) {
        if (event.Is<::dataStub::StartDataCallEvent>()) {
            ::dataStub::StartDataCallEvent startEvent;
            event.UnpackTo(&startEvent);
            mngr->handleStartDataCallEvent(startEvent);
        } else if (event.Is<::dataStub::StopDataCallEvent>()) {
            ::dataStub::StopDataCallEvent stopEvent;
            event.UnpackTo(&stopEvent);
            mngr->handleStopDataCallEvent(stopEvent);
        } else if (event.Is<::dataStub::APNThrottleInfoList>()){
            ::dataStub::APNThrottleInfoList throttleInfoList;
            event.UnpackTo(&throttleInfoList);
            mngr->handleThrottledApnInfoChangedEvent(throttleInfoList);
        }
    }
}

}
}