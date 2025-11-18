/*
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

extern "C"
{
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>

#include "../../../common/RefAppUtils.hpp"

#include "DataFilterController.hpp"
#define PROTO_TCP 6
#define PROTO_UDP 17

using namespace telux::data::net;

DataFilterController::DataFilterController() {
    LOG(DEBUG, __FUNCTION__);
}

DataFilterController::~DataFilterController() {
    LOG(DEBUG, __FUNCTION__);
    if (dataConnectionManager_ && dataConnectionListener_) {
        dataConnectionManager_->deregisterListener(dataConnectionListener_);
    }

    if (dataFilterListener_ && dataFilterMgr_) {
        dataFilterMgr_->deregisterListener(dataFilterListener_);
    }

    dataConnectionManager_ = nullptr;
    dataFilterMgr_ = nullptr;
    dataConnectionListener_ = nullptr;
    dataFilterListener_ = nullptr;

}

bool DataFilterController::initializeSDK(std::function<void(bool isActive)> defaultDataCallUpdateCb) {
    LOG(INFO, __FUNCTION__, " isDataFilterMgrReady = ", (int)isDataFilterMgrReady_,
        " ,isConnectionMgrReady = ", isConnectionMgrReady_);
    defaultDataCallUpdateCb_ = defaultDataCallUpdateCb;
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    std::weak_ptr<DataFilterController> weakFromThis = shared_from_this();

    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();

    do {
        if (!isDataFilterMgrReady_) {
            // data filter mananger
            std::promise<telux::common::ServiceStatus> dfsProm;
            // Get data filter manager object
            dataFilterMgr_ =
                dataFactory.getDataFilterManager(DEFAULT_SLOT_ID,
                                                [&dfsProm](telux::common::ServiceStatus status)
                                                { dfsProm.set_value(status); });
            if (!dataFilterMgr_) {
                LOG(ERROR, __FUNCTION__, " Failed to get DataFilterManager object");
                break;
            }

            //wait for filter manager to get ready
            LOG(DEBUG, __FUNCTION__, " Initializing Data filter manager subsystem Please wait");
            subSystemStatus = dfsProm.get_future().get();
            if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                LOG(DEBUG, __FUNCTION__, " Data Filter Manager is ready");
                dataFilterListener_ = std::make_shared<DataFilterListener>(weakFromThis);
                if (!dataFilterListener_) {
                    LOG(ERROR, __FUNCTION__, " unable to instantiate data filter listener");
                }
                telux::common::Status status =
                    dataFilterMgr_->registerListener(dataFilterListener_);
                if (status != telux::common::Status::SUCCESS) {
                    LOG(ERROR, __FUNCTION__, " Unable to register data filter manager listener");
                    break;
                }
                isDataFilterMgrReady_ = true;
                // To be sure deactivating filer while starting
                telux::data::DataRestrictMode mode;
                mode.filterMode = DataRestrictModeType::DISABLE;
                sendSetDataRestrictMode(mode);

            } else {
                LOG(ERROR, __FUNCTION__, " Data Filter Manager is failed");
                dataFilterMgr_ = nullptr;
                break;
            }
        }


        if (!isConnectionMgrReady_) {

            std::promise<telux::common::ServiceStatus> dcmProm;
            SlotId slotId = DEFAULT_SLOT_ID;
            // data connection mananger
            dataConnectionManager_ =
                dataFactory.getDataConnectionManager(slotId,
                                                    [&dcmProm](telux::common::ServiceStatus status)
                                                    { dcmProm.set_value(status); });

            if (!dataConnectionManager_) {
                LOG(ERROR, __FUNCTION__, " Failed to get DataConnectionManager object");
                break;
            }

            //wait for connection manager to get ready
            LOG(DEBUG, __FUNCTION__, " Initializing Data connection manager subsystem Please wait");
            subSystemStatus = dcmProm.get_future().get();

            if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                LOG(DEBUG, __FUNCTION__, " Data Connection Manager is ready");

                dataConnectionListener_ =
                    std::make_shared<DataConnectionListener>(weakFromThis);
                if (!dataConnectionListener_) {
                    LOG(ERROR, __FUNCTION__, " unable to instantiate data connection listener");
                }
                telux::common::Status status =
                    dataConnectionManager_->registerListener(dataConnectionListener_);

                if (status != telux::common::Status::SUCCESS) {
                    LOG(ERROR, __FUNCTION__,
                        " Unable to register data connection manager listener");
                    break;
                }
                isConnectionMgrReady_ = true;

                // check if data call on default profile is already active
                if (isDefaultDataCallUp()) {
                    defaultDataCallUpdateCb_(true);
                }
            } else {
                LOG(ERROR, __FUNCTION__, " Data Connection Manager is failed");
                dataConnectionManager_ = nullptr;
                break;
            }
        }

    } while (0);

    LOG(INFO, __FUNCTION__, " isDataFilterMgrReady = ", (int)isDataFilterMgrReady_,
        " ,isConnectionMgrReady = ", isConnectionMgrReady_);
    return isDataFilterMgrReady_ && isConnectionMgrReady_;
}

bool DataFilterController::sendSetDataRestrictMode(DataRestrictMode mode) {
    LOG(DEBUG, __FUNCTION__);
    std::promise<telux::common::ErrorCode> prom;

    if (!isDataFilterMgrReady_) {
        LOG(ERROR, __FUNCTION__, " Data restrict filter feature is not supported.");
        return false;
    }

    if (mode.filterMode == DataRestrictModeType::ENABLE) {
        LOG(DEBUG, __FUNCTION__, "  Sending command to enable Data Filter");
    } else if (mode.filterMode == DataRestrictModeType::DISABLE) {
        LOG(DEBUG, __FUNCTION__, "  Sending command to disable Data Filter");
    }

    if (mode.filterAutoExit == DataRestrictModeType::ENABLE) {
        LOG(DEBUG, __FUNCTION__, "  Sending command to enable filter auto exit");
    }
    telux::common::Status status = telux::common::Status::FAILED;

    status = dataFilterMgr_->setDataRestrictMode(mode, [&prom](ErrorCode errorCode) {
        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            LOG(DEBUG," sendSetDataRestrictMode command success callback ");
        } else {
            LOG(ERROR," sendSetDataRestrictMode command failed callback");
        };
        prom.set_value(errorCode);
    });

    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Failed to send Data Restrict command");
        return false;
    } else {
        telux::common::ErrorCode errCode = prom.get_future().get();
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " callback Error = ",
                RefAppUtils::getErrorCodeAsString(errCode));
                return false;
        } else {
            return true;
        }
    }
}

bool DataFilterController::getFilterMode() {
    LOG(DEBUG, __FUNCTION__);
    if (!isDataFilterMgrReady_) {
        LOG(DEBUG, __FUNCTION__, " Data restrict filter feature is not ready");
        return false;
    }
    std::promise<telux::common::ErrorCode> prom{};
    DataRestrictMode drMode;
    LOG(DEBUG, __FUNCTION__, "  Sending command to get Data Filter");
    // pass empty interface name as string.
    telux::common::Status status = dataFilterMgr_->requestDataRestrictMode(
        "", [&prom, &drMode](DataRestrictMode mode, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                LOG(DEBUG, __FUNCTION__, " requestDataRestrictMode Response is successful ");
                LOG(DEBUG, __FUNCTION__, " DataRestrictMode ", RefAppUtils::dataRestrictModeTypeToString(mode.filterMode));
            } else {
                LOG(ERROR, __FUNCTION__, " requestDataRestrictMode Response failed");
                LOG(ERROR, __FUNCTION__,
                   " description: ", RefAppUtils::getErrorCodeAsString(error));
            }
            drMode = mode;
            prom.set_value(error);
    });

    if (status == telux::common::Status::SUCCESS) {
        telux::common::ErrorCode errCode = prom.get_future().get();
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " callback Error = ",
                RefAppUtils::getErrorCodeAsString(errCode));
                return false;
        } else {
            return drMode.filterMode == telux::data::DataRestrictModeType::ENABLE;
        }
    } else {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Failed to send Data Restrict command");
        return false;
    }

}

IpProtocol DataFilterController::getTypeOfFilter(
    DataConfigParser instance, std::map<std::string, std::string> filter) {
    LOG(DEBUG, __FUNCTION__);
    IpProtocol type = PROTO_UDP;
    if (instance.getValue(filter, "FILTER_PROTOCOL_TYPE") != "") {
        std::string protoType = instance.getValue(filter, "FILTER_PROTOCOL_TYPE");
        if (strcmp(protoType.c_str(), "UDP") == 0) {
            type = PROTO_UDP;
        } else if (strcmp(protoType.c_str(), "TCP") == 0) {
            type = PROTO_TCP;
        }
        LOG(DEBUG, __FUNCTION__, " Set TCP Port and Range combination");
    }
    return type;
}

void DataFilterController::addIPParameters(
    std::shared_ptr<telux::data::IIpFilter> &dataFilter, DataConfigParser instance,
    std::map<std::string, std::string> filterMap) {
    LOG(DEBUG, __FUNCTION__);
    if (instance.getValue(filterMap, "SOURCE_IPV4_ADDRESS") != "" ||
        instance.getValue(filterMap, "DESTINATION_IPV4_ADDRESS") != "") {
        telux::data::IPv4Info ipv4Info_ = {};
        if (instance.getValue(filterMap, "SOURCE_IPV4_ADDRESS") != "") {
            ipv4Info_.srcAddr = instance.getValue(filterMap, "SOURCE_IPV4_ADDRESS");
        }
        if (instance.getValue(filterMap, "DESTINATION_IPV4_ADDRESS") != "") {
            ipv4Info_.destAddr = instance.getValue(filterMap, "DESTINATION_IPV4_ADDRESS");
        }
        dataFilter->setIPv4Info(ipv4Info_);
    }

    if (instance.getValue(filterMap, "SOURCE_IPV6_ADDRESS") != "" ||
        instance.getValue(filterMap, "DESTINATION_IPV6_ADDRESS") != "") {
        telux::data::IPv6Info ipv6Info_ = {};
        if (instance.getValue(filterMap, "SOURCE_IPV6_ADDRESS") != "") {
            ipv6Info_.srcAddr = instance.getValue(filterMap, "SOURCE_IPV6_ADDRESS");
        }
        if (instance.getValue(filterMap, "DESTINATION_IPV6_ADDRESS") != "") {
            ipv6Info_.destAddr = instance.getValue(filterMap, "DESTINATION_IPV6_ADDRESS");
        }
        dataFilter->setIPv6Info(ipv6Info_);
    }
}

int DataFilterController::getPortInfo(DataConfigParser cfgParser,
                                std::map<std::string, std::string> pairMap, std::string key,
                                std::string errorStr) {
    LOG(DEBUG, __FUNCTION__);
    int value = std::stoi(cfgParser.getValue(pairMap, key));

    if (value > std::numeric_limits<unsigned short>::max() ||
        value < std::numeric_limits<unsigned short>::min()) {
        throw invalid_argument(errorStr);
    }
    return value;
}

bool DataFilterController::addFilter() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    bool isSuccess = false;
    do {
        if (!isDataFilterMgrReady_) {
            LOG(ERROR, __FUNCTION__, " data filter manager is not ready ");
            break;
        }

        string configFilterFile =
            ConfigParser::getInstance()->getValue("NAOIP_TRIGGER", "NAOIP_FILTER_CONFIG_FILE");
        if (configFilterFile.empty()) {
            configFilterFile = DEFAULT_DATA_CONFIG_FILE_NAME;
        }

        DataConfigParser cfgParser("filter", configFilterFile);
        std::vector<std::map<std::string, std::string>> vectorFilter = cfgParser.getFilters();

        LOG(DEBUG, __FUNCTION__, " Total Filter = ", vectorFilter.size());
        for (uint8_t i = 0; i < vectorFilter.size(); i++) {

            IpProtocol typeOfFilter = getTypeOfFilter(cfgParser, vectorFilter[i]);
            std::shared_ptr<telux::data::IIpFilter> dataFilter;

            if (typeOfFilter == PROTO_TCP) {
                dataFilter = configureTCPFilter(cfgParser, vectorFilter[i]);
            } else if (typeOfFilter == PROTO_UDP) {
                dataFilter = configureUDPFilter(cfgParser, vectorFilter[i]);
            } else {
                LOG(DEBUG, __FUNCTION__, "  *** ERROR - Invalid conf file parameters");
                isSuccess = false;
                break;
            }
            LOG(DEBUG, __FUNCTION__, "  Sending command to Add Data Filter");
            std::promise<telux::common::ErrorCode> prom{};
            retStat = dataFilterMgr_->addDataRestrictFilter(dataFilter, [&prom]
                    (telux::common::ErrorCode error) {
                    prom.set_value(error);
                }
            );
            if (retStat == telux::common::Status::SUCCESS) {
                telux::common::ErrorCode errCode = prom.get_future().get();
                if (errCode != telux::common::ErrorCode::SUCCESS) {
                    LOG(ERROR, __FUNCTION__, " callback Error = ",
                        RefAppUtils::getErrorCodeAsString(errCode));
                        isSuccess = false;
                        break;
                } else {
                    isSuccess = true;
                }
            } else {
                LOG(ERROR, __FUNCTION__, " Error = ", RefAppUtils::teluxStatusToString(retStat));
                isSuccess = false;
                break;
            }
        }
    } while(0);
    LOG(INFO, __FUNCTION__, " add data filter status " );
    return isSuccess;

}

std::shared_ptr<telux::data::IIpFilter> DataFilterController::configureTCPFilter(
    DataConfigParser cfgParser, std::map<std::string, std::string> filter) {
    LOG(DEBUG, __FUNCTION__, " Creating TCP filter ");
    // Get data filter manager object
    std::shared_ptr<telux::data::IIpFilter> dataFilter =
        telux::data::DataFactory::getInstance().getNewIpFilter(PROTO_TCP);
    addIPParameters(dataFilter, cfgParser, filter);
    auto tcpRestrictFilter = std::dynamic_pointer_cast<ITcpFilter>(dataFilter);

    telux::data::TcpInfo tcpInfo_ = {};
    tcpInfo_.src.range = 0;
    tcpInfo_.dest.range = 0;
    try {
        if (cfgParser.getValue(filter, "TCP_SOURCE_PORT") != "") {
            tcpInfo_.src.port = getPortInfo(cfgParser, filter, "TCP_SOURCE_PORT",
                                            "TCP port value");
            if (cfgParser.getValue(filter, "TCP_SOURCE_PORT_RANGE") != "") {
                tcpInfo_.src.range =
                    getPortInfo(cfgParser, filter,"TCP_SOURCE_PORT_RANGE", "TCP Port range value");
            }
        }

        if (cfgParser.getValue(filter, "TCP_DESTINATION_PORT") != "") {
            tcpInfo_.dest.port =
                getPortInfo(cfgParser, filter,
                            "TCP_DESTINATION_PORT", "TCP port value");
            if (cfgParser.getValue(filter, "TCP_DESTINATION_PORT_RANGE") != "") {
                tcpInfo_.dest.range =
                    getPortInfo(cfgParser, filter,
                                "TCP_DESTINATION_PORT_RANGE", "TCP port range value");
            }
        }
    } catch (const std::exception &e) {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid ", string(e.what()),
            ", expected in range (0-65535)");
    }

    if (tcpRestrictFilter) {
        tcpRestrictFilter->setTcpInfo(tcpInfo_);
    } else {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid tcp filter");
    }

    return dataFilter;
}

std::shared_ptr<telux::data::IIpFilter> DataFilterController::configureUDPFilter(
    DataConfigParser cfgParser, std::map<std::string, std::string> filter ) {
    LOG(DEBUG, __FUNCTION__, " Creating UDP filter ");

    // Get data filter manager object
    std::shared_ptr<telux::data::IIpFilter> dataFilter =
        telux::data::DataFactory::getInstance().getNewIpFilter(PROTO_UDP);
    addIPParameters(dataFilter, cfgParser, filter);

    auto udpRestrictFilter = std::dynamic_pointer_cast<IUdpFilter>(dataFilter);
    PortInfo srcPort;
    PortInfo destPort;

    srcPort.port = 0;
    srcPort.range = 0;
    destPort.port = 0;
    destPort.range = 0;
    telux::data::UdpInfo udpInfo_ = {};
    udpInfo_.src.range = 0;
    udpInfo_.dest.range = 0;
    try {
        if (cfgParser.getValue(filter, "UDP_SOURCE_PORT") != "") {
            udpInfo_.src.port =
                getPortInfo(cfgParser, filter, "UDP_SOURCE_PORT",
                            "UDP port value");
            if (cfgParser.getValue(filter, "UDP_SOURCE_PORT_RANGE") != "") {
                udpInfo_.src.range =
                    getPortInfo(cfgParser, filter,
                                "UDP_SOURCE_PORT_RANGE", "UDP Port range value");
            }
        }

        if (cfgParser.getValue(filter, "UDP_DESTINATION_PORT") != "") {
            udpInfo_.dest.port = getPortInfo(cfgParser, filter,
                                                "UDP_DESTINATION_PORT", "UDP port value");
            if (cfgParser.getValue(filter, "UDP_DESTINATION_PORT_RANGE") != "") {
                udpInfo_.dest.range =
                    getPortInfo(cfgParser, filter,
                                "UDP_DESTINATION_PORT_RANGE", "UDP port range vlaue");
            }
        }
    }
    catch (const std::exception &e) {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid ", string(e.what()),
        ", expected in range (0-65535)");
    }
    if (udpRestrictFilter) {
        udpRestrictFilter->setUdpInfo(udpInfo_);
    } else {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid udp filter");
    }

    return dataFilter;
}

bool DataFilterController::removeAllFilter() {
    LOG(DEBUG, __FUNCTION__);
    if (!isDataFilterMgrReady_) {
        LOG(DEBUG, __FUNCTION__, " Data restrict filter feature is not supported.");
        return false;
    }
    LOG(DEBUG, __FUNCTION__, " Remove data filters");
    telux::common::Status status = telux::common::Status::FAILED;
    bool isSuccess = false;
    promise<ErrorCode> prom;
    status = dataFilterMgr_->removeAllDataRestrictFilters([&](ErrorCode errorCode) {
        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            LOG(DEBUG," removeAllFilter command success callback");
        } else {
            LOG(ERROR," removeAllFilter command failed callback");
        };
        prom.set_value(errorCode);
    });


    if (status == telux::common::Status::SUCCESS) {
        telux::common::ErrorCode errCode = prom.get_future().get();
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " callback Error = ",
                RefAppUtils::getErrorCodeAsString(errCode));
                isSuccess = false;
        } else {
            isSuccess = true;
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Error = ", RefAppUtils::teluxStatusToString(status));
        isSuccess = false;

    }
    return isSuccess;
}

int DataFilterController::getDefaultProfile() {
    LOG(DEBUG, __FUNCTION__);
    std::promise<telux::common::ErrorCode> prom;
    int profileId = DEFAULT_PROFILE;

    do {
        if (!isConnectionMgrReady_) {
            LOG(ERROR, __FUNCTION__, " data connection manager is not ready ");
            break;
        }

        telux::common::Status status =
            dataConnectionManager_->getDefaultProfile(OperationType::DATA_LOCAL,
                [&prom, &profileId](int pId, SlotId slotId, telux::common::ErrorCode error) {
                    if (error == telux::common::ErrorCode::SUCCESS) {
                        profileId = pId;
                    }
                    prom.set_value(error);
                }
            );

        if (status == telux::common::Status::SUCCESS) {
            telux::common::ErrorCode errCode = prom.get_future().get();
            if (errCode != telux::common::ErrorCode::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " callback Error = ",
                    RefAppUtils::getErrorCodeAsString(errCode));
            }
        } else {
            LOG(ERROR, __FUNCTION__, " Error = ", RefAppUtils::teluxStatusToString(status));
        }
    }while (0);
    LOG(INFO, __FUNCTION__, " profileId = ", profileId);
    return profileId;
}

bool DataFilterController::isDefaultDataCallUp() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    std::promise<telux::common::ErrorCode> prom;
    bool isDefaultDataCall = false;
    int defaultProfileId = getDefaultProfile();

    do {
        if (!isConnectionMgrReady_) {
            LOG(ERROR, __FUNCTION__, " data connection manager is not ready ");
            break;
        }

        retStat = dataConnectionManager_->requestDataCallList( OperationType::DATA_LOCAL,
            [&prom, &isDefaultDataCall, &defaultProfileId]
                (const std::vector<std::shared_ptr<IDataCall>> &dataCallList,
                                        telux::common::ErrorCode error) {

                for(std::shared_ptr<IDataCall> dataCall : dataCallList) {
                    LOG(DEBUG, __FUNCTION__, " \n data call profile ",
                        dataCall->getProfileId(), " defaultProfileId = ", defaultProfileId);
                    if (dataCall->getProfileId() == defaultProfileId &&
                        dataCall->getDataCallStatus() ==
                            telux::data::DataCallStatus::NET_CONNECTED) {
                        isDefaultDataCall = true;
                    }
                }
                prom.set_value(error);
            }
        );

        if (retStat == telux::common::Status::SUCCESS) {
            telux::common::ErrorCode errCode = prom.get_future().get();
            if (errCode != telux::common::ErrorCode::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " callback Error = ",
                    RefAppUtils::getErrorCodeAsString(errCode));
            }
        } else {
            LOG(ERROR, __FUNCTION__, " Error = ", RefAppUtils::teluxStatusToString(retStat));
        }
    }while(0);
    LOG(INFO, __FUNCTION__, " isDefaultDataCall = ",(int)isDefaultDataCall);
    return isDefaultDataCall;
}

DataFilterController::DataConnectionListener::DataConnectionListener(
    std::weak_ptr<DataFilterController> dataController): dataController_(dataController) {
    LOG(DEBUG, __FUNCTION__);
}

void DataFilterController::DataConnectionListener::onDataCallInfoChanged(
    const std::shared_ptr<telux::data::IDataCall> &dataCall) {
    LOG(DEBUG, __FUNCTION__);
    logDataCallDetails(dataCall);
    if (std::shared_ptr<DataFilterController> dataController = dataController_.lock()) {
        if (dataCall->getProfileId() == dataController->getDefaultProfile()) {
            if (dataCall->getDataCallStatus() == telux::data::DataCallStatus::NET_CONNECTED) {
                dataController->defaultDataCallUpdateCb_(true);
            } else {
                dataController->defaultDataCallUpdateCb_(false);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " unable to lock dataController");
    }
}

void DataFilterController::DataConnectionListener::onServiceStatusChange(
    telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " DataConnectionListener status = ",
        RefAppUtils::serviceStatusToString(status));
    bool dcmStatus = status == telux::common::ServiceStatus::SERVICE_AVAILABLE ? true : false;

    if (std::shared_ptr<DataFilterController> dataController = dataController_.lock()) {
        dataController->isConnectionMgrReady_ = dcmStatus;
    } else {
        LOG(ERROR, __FUNCTION__, " unable to lock dataController");
    }

    LOG(INFO, __FUNCTION__, " isConnectionMgrReady_ = ",
        (int)dcmStatus);
}

void DataFilterController::DataConnectionListener::logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
    LOG(DEBUG, __FUNCTION__);
    std::string tmpLog;
    tmpLog = " ** DataCall etails **\n SlotID: " + std::to_string((int)dataCall->getSlotId()) +
                "\n ProfileID: " + std::to_string((int)dataCall->getProfileId()) +
                "\n interfaceName: " + dataCall->getInterfaceName() +
                "\n DataCallStatus: " + std::to_string((int)dataCall->getDataCallStatus()) +
                "\n DataCallEndReason: Type = " +
                std::to_string(static_cast<int>(dataCall->getDataCallEndReason().type));

    LOG(DEBUG, __FUNCTION__, tmpLog);
    std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
    for (auto &it : ipAddrList) {
        tmpLog = "\n ifAddress: " + it.ifAddress + "\n primaryDnsAddress: " +
                            it.primaryDnsAddress + "\n secondaryDnsAddress: " +
                            it.secondaryDnsAddress;
        LOG(DEBUG, __FUNCTION__, tmpLog);
    }
    tmpLog = " IpFamilyType: " + std::to_string(static_cast<int>(dataCall->getIpFamilyType())) +
                "\nTechPreference: " +
                std::to_string(static_cast<int>(dataCall->getTechPreference())) +
                "\n DataBearerTechnology: " +
                std::to_string(static_cast<int>(dataCall->getCurrentBearerTech()));
    LOG(DEBUG, __FUNCTION__, tmpLog);
}

DataFilterController::DataFilterListener::DataFilterListener(std::weak_ptr<DataFilterController>
                        dataController) : dataController_(dataController) {
    LOG(DEBUG, __FUNCTION__);
}

void DataFilterController::DataFilterListener::onDataRestrictModeChange(DataRestrictMode mode) {
    LOG(DEBUG, __FUNCTION__);
    if (mode.filterMode == DataRestrictModeType::ENABLE) {
        LOG(DEBUG, __FUNCTION__,"Data Filter Mode : Enable");
    } else if (mode.filterMode == DataRestrictModeType::DISABLE) {
        LOG(DEBUG, __FUNCTION__, "Data Filter Mode : Disable");
    } else {
        LOG(ERROR, __FUNCTION__, " ERROR: Invalid Data Filter mode notified");
    }
}

void DataFilterController::DataFilterListener::onServiceStatusChange(
    telux::common::ServiceStatus status) {

    LOG(DEBUG, __FUNCTION__, " DataFilterListener status = ",
        RefAppUtils::serviceStatusToString(status));

    bool dfmStatus = status == telux::common::ServiceStatus::SERVICE_AVAILABLE ? true : false;
    if (std::shared_ptr<DataFilterController> dataController = dataController_.lock()) {
        dataController->isDataFilterMgrReady_ = dfmStatus;
    } else {
        LOG(ERROR, __FUNCTION__, " unable to lock dataController");
    }

    LOG(INFO, __FUNCTION__, " isDataFilterMgrReady_ = ",
        (int)dfmStatus);
}

/*
 * Returns true, if the TRANSPORT_PROTOCOL is set to UDP under [communication]
 * section in the file defined by NAOIP_FILTER_CONFIG_FILE.
 *
 * Returns false in all other cases.
 */
bool DataFilterController::isUDP() {

    std::string configFilterFile = ConfigParser::getInstance()->getValue(
        "NAOIP_TRIGGER", "NAOIP_FILTER_CONFIG_FILE");
    if (configFilterFile.empty()) {
        configFilterFile = DEFAULT_DATA_CONFIG_FILE_NAME;
    }

    DataConfigParser cfgParser("communication", configFilterFile);
    std::vector<std::map<std::string, std::string>> keyValMaps = cfgParser.getFilters();

    if (keyValMaps.size() > 0) {
        std::string proto = cfgParser.getValue(keyValMaps[0], "TRANSPORT_PROTOCOL");
        if (!proto.compare("UDP")) {
            LOG(DEBUG, __FUNCTION__, "Using UDP communication");
            return true;
        }
    }

    LOG(DEBUG, __FUNCTION__, "Using TCP communication");
    return false;
}
