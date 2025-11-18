/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ThermalFactoryImplStub.hpp"
#include "ThermalManagerImplStub.hpp"
#include "ThermalShutdownManagerImplStub.hpp"

#include <memory>

namespace telux {

namespace therm {

ThermalFactory &ThermalFactory::getInstance() {
    return ThermalFactoryImplStub::getInstance();
}

ThermalFactory &ThermalFactoryImplStub::getInstance() {
    static ThermalFactoryImplStub instance;
    return instance;
}

ThermalFactory::ThermalFactory() {
}

ThermalFactoryImplStub::ThermalFactoryImplStub()
   : thermalShutdownManager_(nullptr) {
}

std::shared_ptr<IThermalManager> ThermalFactoryImplStub::getThermalManager(
    InitResponseCb callback, ProcType procType) {
    std::function<std::shared_ptr<IThermalManager>(InitResponseCb)> createAndInit
        = [procType](telux::common::InitResponseCb initCb) -> std::shared_ptr<IThermalManager> {
        std::shared_ptr<ThermalManagerImplStub> manager
            = std::make_shared<ThermalManagerImplStub>(procType);
        if (telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Thermal manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
        " for procType = ", static_cast<int>(procType),
        " , callback = ", &thermalManagerCallbacks_[procType]);
    auto manager = getManager<IThermalManager>(type, thermalManagerMap_[procType],
        thermalManagerCallbacks_[procType], callback, createAndInit);
    return manager;
}

/**
 * API to get IThermalShutdownManager instance. Also performs initialization task.
 *
 * @return  Shared pointer to ThermalShutdownManagerImpl object.
 */
std::shared_ptr<IThermalShutdownManager> ThermalFactoryImplStub::getThermalShutdownManager(
    telux::common::InitResponseCb callback) {
    if (thermalShutdownManager_ == nullptr) {
        thermalShutdownManager_ = std::make_shared<ThermalShutdownManagerImplStub>();
    }
    return thermalShutdownManager_;
}

ThermalFactory::~ThermalFactory() {
}

ThermalFactoryImplStub::~ThermalFactoryImplStub() {
    thermalShutdownManager_.reset();
}

}  // end of namespace therm

}  // end of namespace telux
