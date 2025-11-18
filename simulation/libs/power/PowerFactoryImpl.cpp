/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/CommonUtils.hpp"
#include "PowerFactoryImpl.hpp"
#include "common/EnvUtils.hpp"
#include "unistd.h"
#include "TcuActivityManagerWrapper.hpp"
#include "telux/power/TcuActivityDefines.hpp"

#define CLIENTNAME_LENGTH 63
#define MACHINE_NAME_LENGTH 63

namespace telux {
namespace power {

using namespace telux::common;

PowerFactory::PowerFactory() {
    LOG(DEBUG, __FUNCTION__);
}

PowerFactory::~PowerFactory() {
    LOG(DEBUG, __FUNCTION__);
}

/* Provides PowerFactory instance */
PowerFactory &PowerFactory::getInstance() {
    return PowerFactoryImpl::getInstance();
}

/* Constructor */
PowerFactoryImpl::PowerFactoryImpl() {
    LOG(DEBUG, __FUNCTION__);
//    CommonUtils::logSdkVersion();
}

/* Destructor */
PowerFactoryImpl::~PowerFactoryImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_.shutdown();
}

PowerFactory &PowerFactoryImpl::getInstance() {
    static PowerFactoryImpl instance;
    return instance;
}

/*
 * Deprecated.
 * Performs initializations and returns TcuActivityManagerWrapper object.
 */
std::shared_ptr<ITcuActivityManager> PowerFactoryImpl::getTcuActivityManager(
    ClientType clientType, ProcType procType, InitResponseCb callback) {
    LOG(WARNING, __FUNCTION__, " deprecated API used!");

    if (procType == ProcType::REMOTE_PROC) {
        LOG(ERROR, __FUNCTION__, " REMOTE_PROC not supported");
        return nullptr;
    }

    ClientInstanceConfig config;
    std::string machineName      = "PVM";
    config.clientName
        = machineName + "_" + EnvUtils::getCurrentAppName() +  "_" +std::to_string(getpid());
    LOG(INFO, __FUNCTION__, "  clientName = ", config.clientName);
    config.clientType  = clientType;
    config.machineName = LOCAL_MACHINE;

    return PowerFactoryImpl::getTcuActivityManager(config, callback);
}

/*
 * Performs initializations and returns TcuActivityManagerWrapper object.
 */
std::shared_ptr<ITcuActivityManager> PowerFactoryImpl::getTcuActivityManager(
    ClientInstanceConfig config, telux::common::InitResponseCb callback) {

    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(tcuActivityFactoryMutex_);

    if (config.clientType == ClientType::SLAVE) {
        /* Ensure slave is identifiable. Uniqueness of the name can't be verified. */
        if (config.clientName.empty() || config.clientName.length() > CLIENTNAME_LENGTH
            || config.machineName.empty()
            || config.machineName.length() > MACHINE_NAME_LENGTH) {
            LOG(ERROR, __FUNCTION__, " unexpected client or machine name; client name length = ",
                config.clientName.length(), " machine name length = ", config.machineName.length());
            return nullptr;
        }
    }

    std::function<std::shared_ptr<ITcuActivityManager>(telux::common::InitResponseCb)> createAndInit
        = [config, this](
              telux::common::InitResponseCb initCb) -> std::shared_ptr<ITcuActivityManager> {
        std::shared_ptr<TcuActivityManagerWrapper> tcuActivityMgrWrpr = nullptr;
        try {
            tcuActivityMgrWrpr = std::shared_ptr<TcuActivityManagerWrapper>(
                new TcuActivityManagerWrapper(), [this](TcuActivityManagerWrapper *impl) {
                    LOG(INFO, " TcuActivityManagerWrapper shared pointer custom deletor");
                    auto f = std::async(std::launch::deferred, [this, impl]() {
                        FactoryHelper::cleanup(impl);
                    }).share();
                    taskQ_.add(f);
                });
        } catch (std::bad_alloc &e) {
            LOG(ERROR, __FUNCTION__, e.what());
            return nullptr;
        }
        if (!tcuActivityMgrWrpr) {
            LOG(ERROR, __FUNCTION__, " Failed to create tcuActivityMgrWrpr instance");
            return nullptr;
        }
        std::shared_ptr<TcuActivityManagerImpl> tcuActivityMgr = tcuActivityMgrWrpr->init(config);
        if (!tcuActivityMgr) {
            LOG(ERROR, __FUNCTION__, " Failed to create TcuActivityManagerImpl instance");
            return nullptr;
        }

        /*
         * Serialize initialization and de-initialization of the TcuActivityManagerImpl to ensure
         * only one master (atomically created/destroyed) exist at any point in time. A new Master
         * object can be initialized successfully only after the previous master object is
         * destroyed/de-initialized completely. This also helps in achieving deterministic behavior.
         */
        auto f = std::async(std::launch::deferred, [tcuActivityMgr, initCb]() {
            tcuActivityMgr->init(initCb);
        }).share();
        taskQ_.add(f);
        return tcuActivityMgrWrpr;
    };
    auto type = std::string("TCUActivity Manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(), " for clientName = ", config.clientName,
        " , clientType = ", static_cast<int>(config.clientType),
        " , machineName = ", config.machineName,
        " , callback = ", &tcuActivityMgrClientsCallbacks_[config]);
    auto manager = getManager<ITcuActivityManager>(type, tcuActivityManagerClientsMap_[config],
        tcuActivityMgrClientsCallbacks_[config], callback, createAndInit);
    return manager;
}

}  // end namespace power
}  // end namespace telux
