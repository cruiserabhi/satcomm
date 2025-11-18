 /*
  *  Copyright (c) 2021,2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
  *  SPDX-License-Identifier: BSD-3-Clause-Clear
  */

#include "DataFactoryImplStub.hpp"
#include "DataConnectionManagerStub.hpp"
#include "DataProfileManagerStub.hpp"
#include "DataSettingsManagerStub.hpp"
#include "DataFilterManagerStub.hpp"
#include "IpFilterImpl.hpp"
#include "DataHelper.hpp"
#include "ServingSystemManagerStub.hpp"
#include "DualDataManagerStub.hpp"
#include "DataControlManagerStub.hpp"
#include "KeepAliveManagerStub.hpp"
#include "DataLinkManagerStub.hpp"
#include "net/SocksManagerStub.hpp"
#include "net/NatManagerStub.hpp"
#include "net/VlanManagerStub.hpp"
#include "net/L2tpManagerStub.hpp"
#include "net/FirewallManagerStub.hpp"
#include "net/FirewallEntryImpl.hpp"
#include "net/BridgeManagerStub.hpp"
#include "net/QoSManagerStub.hpp"

#include "common/Logger.hpp"

std::mutex dataMutex_;

namespace telux {
namespace data {

DataFactoryImplStub::DataFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);
}

DataFactoryImplStub::~DataFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);

    // cleanup dataConnectionManagers
    for (auto& conMgrEntry : dataConnectionManagerMap_) {
        auto conMgr = conMgrEntry.second.lock();
        if(conMgr) {
            (std::static_pointer_cast<DataConnectionManagerStub>(conMgr))->cleanup();
        }
    }
    dataConnectionManagerMap_.clear();
    dataProfileManagerMap_.clear();
    dataServingSystemManagerMap_.clear();
}

DataFactory::DataFactory() {
    LOG(DEBUG, __FUNCTION__);
}

DataFactory::~DataFactory() {
    LOG(DEBUG, __FUNCTION__);
}

DataFactory &DataFactoryImplStub::getInstance() {
    static DataFactoryImplStub instance;
    return instance;
}

DataFactory &DataFactory::getInstance() {
    return DataFactoryImplStub::getInstance();
}


std::shared_ptr<IDataConnectionManager> DataFactoryImplStub::getDataConnectionManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    LOG(DEBUG, __FUNCTION__);
    std::function<std::shared_ptr<IDataConnectionManager>(telux::common::InitResponseCb)>
        createAndInit
        = [slotId, this](
              telux::common::InitResponseCb initCb) -> std::shared_ptr<IDataConnectionManager> {
        std::shared_ptr<DataConnectionManagerStub> manager
            = std::make_shared<DataConnectionManagerStub>(slotId);
        if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Data connection manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
       " for slotId = ", static_cast<int>(slotId),
       " , callback = ", &dataConnectionCallbacks_[slotId]);
    auto manager = getManager<IDataConnectionManager>(
        type, dataConnectionManagerMap_[slotId],
        dataConnectionCallbacks_[slotId], clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<IDataProfileManager> DataFactoryImplStub::getDataProfileManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<IDataProfileManager> dataProfileMgr = nullptr;
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto ItrMgr = dataProfileManagerMap_.find(slotId);
    if (ItrMgr != dataProfileManagerMap_.end()) {
        dataProfileMgr = ItrMgr->second.lock();
    }
    if (dataProfileMgr) {
        LOG(DEBUG, "Found Data Profile Manager with slot id: ", static_cast<int>(slotId));
        telux::common::ServiceStatus status =  dataProfileMgr->getServiceStatus();
        if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
            //Manager has failed initialization but callback is not called yet hence we still
            //have valid shared pointer.
            LOG(DEBUG, __FUNCTION__, " Data Profile Manager initialization failed.");
            return nullptr;
        }
        else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " Data Profile Manager initialization was successful");
            if (clientCallback) {
                dataProfileCallbacks_[slotId].push_back(clientCallback);
            }
            std::thread appCallback([this, status, slotId]() {
                this->initCompleteNotifierWithSlotId(dataProfileCallbacks_, status, slotId);});
            appCallback.detach();
        }
        else {
            LOG(DEBUG, __FUNCTION__, " Data Profile Manager initialization in progress.");
            if (clientCallback) {
                dataProfileCallbacks_[slotId].push_back(clientCallback);
            }
        }
        return dataProfileMgr;
    }
    else {
        std::shared_ptr<DataProfileManagerStub> dataProfileMgrImpl = nullptr;
        LOG(DEBUG, "Creating Data Profile Manager with slot id: ", slotId);
        auto initCb = [this, slotId](telux::common::ServiceStatus status) {
            if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                std::lock_guard<std::mutex> lock(dataMutex_);
                dataProfileManagerMap_.erase(slotId);
            }
            this->initCompleteNotifierWithSlotId(dataProfileCallbacks_, status, slotId);
        };
        try {
            dataProfileMgrImpl = std::make_shared<DataProfileManagerStub>(slotId, initCb);
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        dataProfileManagerMap_[slotId] = dataProfileMgrImpl;
        if (clientCallback) {
            dataProfileCallbacks_[slotId].push_back(clientCallback);
        }
        return dataProfileMgrImpl;
    }
}

std::shared_ptr<IServingSystemManager> DataFactoryImplStub::getServingSystemManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    std::shared_ptr<IServingSystemManager> servingSystemMgr = nullptr;
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto ItrMgr = dataServingSystemManagerMap_.find(slotId);
    if (ItrMgr != dataServingSystemManagerMap_.end()) {
        servingSystemMgr = ItrMgr->second.lock();
    }
    if (servingSystemMgr) {
        LOG(DEBUG, "Found Serving System Manager with slot id: ", static_cast<int>(slotId));
        //Find the current status of the manager
        telux::common::ServiceStatus status = servingSystemMgr->getServiceStatus();
        if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
            //Manager has failed initialization but callback is not called yet hence we still
            //have valid shared pointer.
            LOG(DEBUG, __FUNCTION__, " Data Serving System Manager initialization failed.");
            return nullptr;
        }
        else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " Data Serving System Manager initialization was successful");
            if (clientCallback) {
                servingSystemCallbacks_[slotId].push_back(clientCallback);
            }
            std::thread appCallback([this, status, slotId]() {
                this->initCompleteNotifierWithSlotId(servingSystemCallbacks_, status, slotId);});
            appCallback.detach();
        }
        else {
            LOG(DEBUG, __FUNCTION__, " Data Serving System Manager initialization in progress.");
            if (clientCallback) {
                servingSystemCallbacks_[slotId].push_back(clientCallback);
            }
        }
        return servingSystemMgr;
    }
    else {
        std::shared_ptr<ServingSystemManagerStub> servingSystemMgrImpl = nullptr;
        LOG(DEBUG, "Creating Data Serving System Manager with slot id: ", slotId);
        auto initCb = [this, slotId](telux::common::ServiceStatus status) {
            if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                std::lock_guard<std::mutex> lock(dataMutex_);
                dataServingSystemManagerMap_.erase(slotId);
            }
            this->initCompleteNotifierWithSlotId(servingSystemCallbacks_, status, slotId);
        };
        servingSystemMgrImpl = std::make_shared<ServingSystemManagerStub>(slotId);
        if ((!servingSystemMgrImpl) ||
            (telux::common::Status::SUCCESS != servingSystemMgrImpl->init(initCb))) {
            LOG(DEBUG, "DataFactory unable to initialize ServingSystemManager");
            return nullptr;
        }
        dataServingSystemManagerMap_[slotId] = servingSystemMgrImpl;
        if (clientCallback) {
            servingSystemCallbacks_[slotId].push_back(clientCallback);
        }
        return servingSystemMgrImpl;
    }
}

std::shared_ptr<IDataFilterManager> DataFactoryImplStub::getDataFilterManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    std::shared_ptr<IDataFilterManager> dataFilterManager = nullptr;
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto ItrMgr = dataFilterManagerMap_.find(slotId);
    if (ItrMgr != dataFilterManagerMap_.end()) {
        dataFilterManager = ItrMgr->second.lock();
    }
    if(dataFilterManager) {
        LOG(DEBUG, "Found Data Filter Manager with slot id: ", static_cast<int>(slotId));
        //Find the current status of the manager
        telux::common::ServiceStatus status = dataFilterManager->getServiceStatus();
        if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
            //Manager has failed initialization but callback is not called yet hence we still
            //have valid shared pointer.
            LOG(DEBUG, __FUNCTION__, " Data Filter Manager initialization failed.");
            dataFilterManagerMap_.erase(slotId);
            return nullptr;
        } else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " Data Filter Manager initialization was successful");
            if (clientCallback) {
                dataFilterCallbacks_[slotId].push_back(clientCallback);
            }
            std::thread appCallback([this, status, slotId]() {
                this->initCompleteNotifierWithSlotId(dataFilterCallbacks_, status, slotId);});
            appCallback.detach();
        } else {
            LOG(DEBUG, __FUNCTION__, " Data Filter Manager initialization in progress.");
            if (clientCallback) {
                dataFilterCallbacks_[slotId].push_back(clientCallback);
            }
        }
        return dataFilterManager;
    } else {
        std::shared_ptr<DataFilterManagerStub> dataFilterManagerImpl = nullptr;
        LOG(DEBUG, "Creating Data Filter Manager with slot id: ", slotId);
        auto initCb = [this, slotId](telux::common::ServiceStatus status) {
            if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                std::lock_guard<std::mutex> lock(dataMutex_);
                dataFilterManagerMap_.erase(slotId);
            }
            this->initCompleteNotifierWithSlotId(dataFilterCallbacks_, status, slotId);
        };
        try {
            dataFilterManagerImpl = std::make_shared<DataFilterManagerStub>(slotId);
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        if ((!dataFilterManagerImpl) ||
            (telux::common::Status::SUCCESS != dataFilterManagerImpl->init(initCb))) {
            LOG(DEBUG, "DataFactory unable to initialize DataFilterManager");
            return nullptr;
        }
        dataFilterManagerMap_[slotId] = dataFilterManagerImpl;
        if (clientCallback) {
            dataFilterCallbacks_[slotId].push_back(clientCallback);
        }
        return dataFilterManagerImpl;
    }
}

std::shared_ptr<IIpFilter> DataFactoryImplStub::getNewIpFilter(IpProtocol proto) {
    switch (proto) {
        case PROTO_TCP: {
            return std::make_shared<TcpFilterImpl>(proto);
        } break;
        case PROTO_UDP: {
            return std::make_shared<UdpFilterImpl>(proto);
        }
        case PROTO_ICMP:
        case PROTO_ICMP6: {
            return std::make_shared<IcmpFilterImpl>(proto);
        }
        case PROTO_ESP: {
            return std::make_shared<EspFilterImpl>(proto);
        }
        default: { return nullptr; }
    }
}

std::shared_ptr<telux::data::net::INatManager> DataFactoryImplStub::getNatManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {

    if (oprType == telux::data::OperationType::DATA_REMOTE) {
        return nullptr;
    }

    std::function<std::shared_ptr<telux::data::net::INatManager>(
        telux::common::InitResponseCb)>
        createAndInit = [oprType](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::net::INatManager> {
        std::shared_ptr<telux::data::net::NatManagerStub> manager
            = std::make_shared<telux::data::net::NatManagerStub>(oprType);
        if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("NAT manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
       " for operationType = ", static_cast<int>(oprType), " , callback = ", &natCallbacks_);
    auto manager
        = getManager<telux::data::net::INatManager>(type,
            natManagerMap_[oprType], natCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::net::IFirewallManager> DataFactoryImplStub::getFirewallManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {
    if (oprType == telux::data::OperationType::DATA_REMOTE) {
        return nullptr;
    }

    std::function<std::shared_ptr<telux::data::net::IFirewallManager>(
        telux::common::InitResponseCb)>
        createAndInit = [oprType](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::net::IFirewallManager> {
        std::shared_ptr<telux::data::net::FirewallManagerStub> manager
            = std::make_shared<telux::data::net::FirewallManagerStub>(oprType);
        if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Firewall manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
       " for operationType = ", static_cast<int>(oprType), " , callback = ", &firewallCallbacks_);
    auto manager
        = getManager<telux::data::net::IFirewallManager>(type,
            firewallManagerMap_[oprType], firewallCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::net::IFirewallEntry> DataFactoryImplStub::getNewFirewallEntry(
    IpProtocol proto, Direction direction, IpFamilyType ipFamilyType) {
    std::shared_ptr<IIpFilter> ipFilter = getNewIpFilter(proto);
    if (!ipFilter) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(dataMutex_);
    return std::make_shared<net::FirewallEntryImpl>(ipFilter, direction, ipFamilyType);
}

std::shared_ptr<telux::data::net::IVlanManager> DataFactoryImplStub::getVlanManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {
    if (oprType == telux::data::OperationType::DATA_REMOTE) {
        return nullptr;
    }

    std::function<std::shared_ptr<telux::data::net::IVlanManager>(
        telux::common::InitResponseCb)>
        createAndInit = [oprType](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::net::IVlanManager> {
        std::shared_ptr<telux::data::net::VlanManagerStub> manager
            = std::make_shared<telux::data::net::VlanManagerStub>(oprType);
        if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Vlan manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
       " for operationType = ", static_cast<int>(oprType), " , callback = ",
       &vlanCallbacks_);
    auto manager
        = getManager<telux::data::net::IVlanManager>(type,
            vlanManagerMap_[oprType], vlanCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::net::ISocksManager> DataFactoryImplStub::getSocksManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {

    if (oprType == telux::data::OperationType::DATA_REMOTE) {
        return nullptr;
    }

    std::function<std::shared_ptr<telux::data::net::ISocksManager>(
        telux::common::InitResponseCb)>
        createAndInit = [oprType](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::net::ISocksManager> {
        std::shared_ptr<telux::data::net::SocksManagerStub> manager
            = std::make_shared<telux::data::net::SocksManagerStub>(oprType);
        if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Socks manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
       " for operationType = ", static_cast<int>(oprType), " , callback = ", &socksCallbacks_);
    auto manager
        = getManager<telux::data::net::ISocksManager>(type,
            socksManagerMap_[oprType], socksCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::net::IBridgeManager> DataFactoryImplStub::getBridgeManager(
    telux::common::InitResponseCb clientCallback) {
    std::function<std::shared_ptr<telux::data::net::IBridgeManager>(
        telux::common::InitResponseCb)> createAndInit
        = [](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::net::IBridgeManager> {
            std::shared_ptr<telux::data::net::BridgeManagerStub> manager
                = std::make_shared<telux::data::net::BridgeManagerStub>();
            if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
                return nullptr;
            }
            return manager;
    };
    auto type = std::string("Bridge manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(), " , callback = ", &bridgeCallbacks_);
    auto manager
        = getManager<telux::data::net::IBridgeManager>(type,
            bridgeManager_, bridgeCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::IDualDataManager> DataFactoryImplStub::getDualDataManager(
    telux::common::InitResponseCb clientCallback) {
    std::function<std::shared_ptr<telux::data::IDualDataManager>(
        telux::common::InitResponseCb)> createAndInit
        = [](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::IDualDataManager> {
            std::shared_ptr<telux::data::DualDataManagerStub> manager
                = std::make_shared<telux::data::DualDataManagerStub>();
            if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
                return nullptr;
            }
            return manager;
    };
    auto type = std::string("DualData manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(), " , callback = ", &dualDataCallbacks_);
    auto manager
        = getManager<telux::data::IDualDataManager>(type,
            dualDataManager_, dualDataCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::IDataControlManager> DataFactoryImplStub::getDataControlManager(
    telux::common::InitResponseCb clientCallback) {
    std::function<std::shared_ptr<telux::data::IDataControlManager>(
        telux::common::InitResponseCb)> createAndInit
        = [](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::IDataControlManager> {
            std::shared_ptr<telux::data::DataControlManagerStub> manager
                = std::make_shared<telux::data::DataControlManagerStub>();
            if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
                return nullptr;
            }
            return manager;
    };
    auto type = std::string("DataControl manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(), " , callback = ",
            &dataControlCallbacks_);
    auto manager
        = getManager<telux::data::IDataControlManager>(type,
            dataControlManager_, dataControlCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::IKeepAliveManager> DataFactoryImplStub::getKeepAliveManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    std::function<std::shared_ptr<telux::data::IKeepAliveManager>(
        telux::common::InitResponseCb)> createAndInit
        = [slotId](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::IKeepAliveManager> {
            std::shared_ptr<telux::data::KeepAliveManagerStub> manager
                = std::make_shared<telux::data::KeepAliveManagerStub>(slotId);
            if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
                return nullptr;
            }
            return manager;
    };
    auto type = std::string("KeepAlive manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(), " , callback = ", &keepAliveCallbacks_);
    auto manager
        = getManager<telux::data::IKeepAliveManager>(type,
            KeepAliveManager_, keepAliveCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::IDataLinkManager> DataFactoryImplStub::getDataLinkManager(
    telux::common::InitResponseCb clientCallback) {
    std::function<std::shared_ptr<telux::data::IDataLinkManager>(
        telux::common::InitResponseCb)> createAndInit
        = [](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::IDataLinkManager> {
            std::shared_ptr<telux::data::DataLinkManagerStub> manager
                = std::make_shared<telux::data::DataLinkManagerStub>();
            if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
                return nullptr;
            }
            return manager;
    };
    auto type = std::string("DataLink manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(), " , callback = ",
            &dataLinkCallbacks_);
    auto manager
        = getManager<telux::data::IDataLinkManager>(type,
            dataLinkManager_, dataLinkCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::net::IL2tpManager> DataFactoryImplStub::getL2tpManager(
    telux::common::InitResponseCb clientCallback) {
    std::function<std::shared_ptr<telux::data::net::IL2tpManager>(
        telux::common::InitResponseCb)> createAndInit
        = [](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::data::net::IL2tpManager> {
            std::shared_ptr<telux::data::net::L2tpManagerStub> manager
                = std::make_shared<telux::data::net::L2tpManagerStub>();
            if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
                return nullptr;
            }
            return manager;
    };
    auto type = std::string("L2TP manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(), " , callback = ", &l2tpCallbacks_);
    auto manager
        = getManager<telux::data::net::IL2tpManager>(type,
            l2tpManager_, l2tpCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<telux::data::IClientManager> DataFactoryImplStub::getClientManager(
    telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<telux::data::IDataSettingsManager> DataFactoryImplStub::getDataSettingsManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {
    std::shared_ptr<IDataSettingsManager> settingsMgr = nullptr;
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto ItrMgr = dataSettingsManagerMap_.find(oprType);
    if (ItrMgr != dataSettingsManagerMap_.end()) {
        settingsMgr = ItrMgr->second.lock();
    }
    if(settingsMgr) {
        LOG(DEBUG, "Found IDataSettingsManager for oprType: ", static_cast<int>(oprType));
        //Find the current state of manager
        telux::common::ServiceStatus status = settingsMgr->getServiceStatus();
        if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
            //Manager has failed initialization but callback is not called yet hence we still
            //have valid shared pointer. Return nullptr and callback will be executed to inform
            //client and clear instance pointer as soon as we release mutex
            LOG(DEBUG, __FUNCTION__, " Data Settings Manager initialization failed.");
            return nullptr;
        }
        else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " Data Settings Manager initialization was successful");
            if (clientCallback) {
                dataSettingsCallbacks_[oprType].push_back(clientCallback);
            }
            std::thread appCallback([this, status, oprType]() {
                this->initCompleteNotifierWithOprType(dataSettingsCallbacks_, status, oprType);});
            appCallback.detach();
        }
        else {
            LOG(DEBUG, __FUNCTION__, " DataSettings Manager initialization in progress.");
            if (clientCallback) {
                dataSettingsCallbacks_[oprType].push_back(clientCallback);
            }
        }
        return settingsMgr;
    } else {
        std::shared_ptr<DataSettingsManagerStub> settingsMgrImpl = nullptr;
        LOG(DEBUG, "Creating IDataSettingsManager with operation type ", static_cast<int>(oprType));
        auto initCb = [this, oprType](telux::common::ServiceStatus status) {
            if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                std::lock_guard<std::mutex> lock(dataMutex_);
                dataSettingsCallbacks_.erase(oprType);
            }
            this->initCompleteNotifierWithOprType( dataSettingsCallbacks_, status, oprType);
        };
        try {
            settingsMgrImpl =
                    std::make_shared<DataSettingsManagerStub>(oprType);
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        if (telux::common::Status::SUCCESS != settingsMgrImpl->init(initCb)) {
            LOG(DEBUG, __FUNCTION__, " FAILED to create Settings Manager instance");
            return nullptr;
        }
        dataSettingsManagerMap_[oprType] = settingsMgrImpl;
        if (clientCallback) {
            dataSettingsCallbacks_[oprType].push_back(clientCallback);
        }
        return settingsMgrImpl;
    }
}

void DataFactoryImplStub::initCompleteNotifierWithSlotId(
    std::map<SlotId, std::vector<telux::common::InitResponseCb>>& initCbs,
    telux::common::ServiceStatus status, SlotId slotId) {

    LOG(DEBUG, __FUNCTION__);
    std::vector<telux::common::InitResponseCb> Callbacks;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        Callbacks = initCbs[slotId];
        initCbs.erase(slotId);
    }
    for (auto &callback : Callbacks) {
        callback(status);
    }
}

void DataFactoryImplStub::initCompleteNotifierWithOprType(
    std::map<OperationType, std::vector<telux::common::InitResponseCb>>& initCbs,
    telux::common::ServiceStatus status, OperationType oprType) {

    LOG(DEBUG, __FUNCTION__);
    std::vector<telux::common::InitResponseCb> Callbacks;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        Callbacks = initCbs[oprType];
        initCbs.erase(oprType);
    }
    for (auto &callback : Callbacks) {
        callback(status);
    }
}

void DataFactoryImplStub::initCompleteNotifier(std::vector<telux::common::InitResponseCb>& initCbs,
    telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<telux::common::InitResponseCb> Callbacks;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        Callbacks = initCbs;
        initCbs.clear();
    }
    for (auto &callback : Callbacks) {
        callback(status);
    }
}

std::shared_ptr<telux::data::net::IQoSManager> DataFactoryImplStub::getQoSManager(
        telux::common::InitResponseCb clientCallback) {
    std::function<std::shared_ptr<telux::data::net::IQoSManager>(telux::common::InitResponseCb)>
        createAndInit
        = [this](
              telux::common::InitResponseCb initCb) -> std::shared_ptr<telux::data::net::IQoSManager> {
        std::shared_ptr<telux::data::net::QoSManagerStub> manager
            = std::make_shared<telux::data::net::QoSManagerStub>();
        if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("QoS manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
       " , callback = ", &qosCallbacks_);
    auto manager = getManager<telux::data::net::IQoSManager>(
        type, qosManager_,
        qosCallbacks_, clientCallback, createAndInit);
    return manager;
}

}  // namespace data
}
