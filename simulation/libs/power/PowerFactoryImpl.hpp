/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef POWERFACTORYIMPL_HPP
#define POWERFACTORYIMPL_HPP

#include <memory>
#include <mutex>
#include <map>
#include <vector>

#include "common/AsyncTaskQueue.hpp"
#include "common/FactoryHelper.hpp"

#include <telux/power/PowerFactory.hpp>

namespace telux {
namespace power {

/* Needed for element comparision for sorted map tcuActivityManagerClientsMap_ */
bool operator<(const ClientInstanceConfig &conf1, const ClientInstanceConfig &conf2) {
    if (conf1.clientType == conf2.clientType) {
        if (conf1.machineName == conf2.machineName) {
            return conf1.clientName < conf2.clientName;
        }
        return conf1.machineName < conf2.machineName;
    }
    return conf1.clientType < conf2.clientType;
}

class PowerFactoryImpl : public PowerFactory, public telux::common::FactoryHelper {
 public:
    static PowerFactory &getInstance();

    std::shared_ptr<ITcuActivityManager> getTcuActivityManager(
        ClientType clientType                  = ClientType::SLAVE,
        common::ProcType procType              = common::ProcType::LOCAL_PROC,
        telux::common::InitResponseCb callback = nullptr) override;

    std::shared_ptr<ITcuActivityManager> getTcuActivityManager(
        ClientInstanceConfig config, telux::common::InitResponseCb callback = nullptr) override;

 private:
    PowerFactoryImpl();
    ~PowerFactoryImpl();

    std::map<ClientInstanceConfig, std::weak_ptr<ITcuActivityManager>> tcuActivityManagerClientsMap_;
    std::mutex tcuActivityFactoryMutex_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    std::map<ClientInstanceConfig, std::vector<telux::common::InitResponseCb>>
        tcuActivityMgrClientsCallbacks_;
};

}  // end of namespace power
}  // end of namespace telux

#endif  // POWERFACTORYIMPL_HPP
