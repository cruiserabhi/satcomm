/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       CellBroadcastManagerStub.hpp
 *
 * @brief      Implementation of CellBroadcastManager
 *
 */

#ifndef CELLBROADCAST_MANAGER_STUB_HPP
#define CELLBROADCAST_MANAGER_STUB_HPP

#include "common/event-manager/ClientEventManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "common/ListenerManager.hpp"
#include <telux/tel/CellBroadcastDefines.hpp>
#include <telux/tel/CellBroadcastManager.hpp>
#include <telux/common/CommonDefines.hpp>
#include "TelDefinesStub.hpp"

namespace telux {
namespace tel {

class CellBroadcastManagerStub : public ICellBroadcastManager,
                                 public IEventListener,
                                 public std::enable_shared_from_this<CellBroadcastManagerStub> {
public:

    CellBroadcastManagerStub(int phoneId);
    telux::common::Status init(telux::common::InitResponseCb callback);
    bool isSubsystemReady();
    std::future<bool> onSubsystemReady();
    telux::common::ServiceStatus getServiceStatus();
    SlotId getSlotId();
    telux::common::Status updateMessageFilters(std::vector<CellBroadcastFilter> filters,
        telux::common::ResponseCallback callback = nullptr);
    telux::common::Status requestMessageFilters(
        RequestFiltersResponseCallback callback);
    telux::common::Status setActivationStatus(bool activate,
        telux::common::ResponseCallback callback = nullptr);
    telux::common::Status requestActivationStatus(
        RequestActivationStatusResponseCallback callback);
    telux::common::Status registerListener(std::weak_ptr<ICellBroadcastListener> listener);
    telux::common::Status deregisterListener(std::weak_ptr<ICellBroadcastListener> listener);

private:
    void initSync(telux::common::InitResponseCb callback);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error);
    void invokeCallback(RequestFiltersResponseCallback callback,
        telux::common::ErrorCode error, std::vector<CellBroadcastFilter> filters);
    void invokeCallback(RequestActivationStatusResponseCallback callback,
        telux::common::ErrorCode error, bool isActivated);
    void invokeInitResponseCallback(telux::common::ServiceStatus cbStatus,
        telux::common::InitResponseCb callback);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
        telux::common::InitResponseCb initCb_;
    std::mutex mutex_;
    std::shared_ptr<telux::common::ListenerManager<ICellBroadcastListener>> listenerMgr_;
};

} // end of namespace tel

} // end of namespace telux

#endif // CELLBROADCAST_MANAGER_STUB_HPP