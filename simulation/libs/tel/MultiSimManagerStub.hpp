/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       MultiSimManagerStub.hpp
 *
 * @brief      Implementation of IMultiSimManager
 *
 */

#ifndef MULTISIM_MANAGER_STUB_HPP
#define MULTISIM_MANAGER_STUB_HPP

#include <telux/tel/MultiSimManager.hpp>
#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"

namespace telux {
namespace tel {

class MultiSimManagerStub : public IMultiSimManager,
                                public IEventListener,
                                public std::enable_shared_from_this<MultiSimManagerStub>  {
public:
    MultiSimManagerStub();
    telux::common::Status init(telux::common::InitResponseCb callback);
    ~MultiSimManagerStub();
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<IMultiSimListener>) override;
    telux::common::Status deregisterListener(std::weak_ptr<IMultiSimListener>) override;
    telux::common::Status getSlotCount(int &count) override;
    telux::common::Status requestHighCapability(HighCapabilityCallback callback) override;
    telux::common::Status setHighCapability(int slotId,
        common::ResponseCallback callback) override;
    telux::common::Status switchActiveSlot(SlotId slotId,
        common::ResponseCallback callback) override;
    telux::common::Status requestSlotStatus(SlotStatusCallback callback) override;
    void onEventUpdate(google::protobuf::Any event);
    void cleanup();
private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    void initSync(telux::common::InitResponseCb callback);
};

} // end of namespace tel

} // end of namespace telux

#endif // MULTISIM_MANAGER_STUB_HPP