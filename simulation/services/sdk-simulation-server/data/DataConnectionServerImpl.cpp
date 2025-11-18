/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <ifaddrs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <thread>

#include "DataConnectionServerImpl.hpp"
#include "SimulationServer.hpp"

#include "libs/common/SimulationConfigParser.hpp"
#include "libs/data/DataUtilsStub.hpp"
#include "event/EventService.hpp"
#include "common/event-manager/EventParserUtil.hpp"


#define DATA_CONNECTION_API_SLOT1_JSON "api/data/IDataConnectionManagerSlot1.json"
#define DATA_CONNECTION_API_SLOT2_JSON "api/data/IDataConnectionManagerSlot2.json"
#define DATA_CONNECTION_STATE_JSON "system-state/data/IDataConnectionManagerState.json"
#define DATA_SETTINGS_API_LOCAL_JSON "api/data/IDataSettingsManagerLocal.json"
#define DATA_SETTINGS_STATE_JSON "system-state/data/IDataSettingsManagerState.json"

#define SLOT_2 2
#define DELIMITER ','
#define DEFAULT_DELIMITER " "
#define DATA_CONNECTION "data_connection"
#define THROTTLE_APN_INFO_TOKEN "throttle_apn_event"
#define RESET "RESET"
#define START "START"
#define STOP "STOP"

DataConnectionServerImpl::DataConnectionServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    timerStarted_ = false;
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

DataConnectionServerImpl::~DataConnectionServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = nullptr;
    timerStarted_ = false; // Stop the timer incase it is running
    if (timerFuture_.valid()) {
        timerFuture_.get(); // Wait for the async task to complete
    }
}

void DataConnectionServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == DATA_CONNECTION) {
        onEventUpdate(message.event());
    }
}

void DataConnectionServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__," Event injected :", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__," Token String is ", token );

    if (token == THROTTLE_APN_INFO_TOKEN) {
        //INPUT-token: throttle_apn_event
        //INPUT-event: RESET | START | STOP
        handleThrottleApnEvent(event);
    } else {
        LOG(ERROR, __FUNCTION__," Unknown Token! ");
    }
}

void DataConnectionServerImpl::startThrottleRetryTimer() {
    LOG(DEBUG, __FUNCTION__ );
    timerStarted_ = true;
    timerFuture_ = std::async(std::launch::async, [this] {
        int counter = 0;
        while (timerStarted_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            counter++;
            {
                std::lock_guard<std::mutex> lock(apnThrottleInfoMtx_);
                for (auto& info : apnThrottleInfo_) {
                    if (info.ipv4Time >= 1000) {
                        info.ipv4Time -= 1000;
                    } else {
                        info.ipv4Time = 0;
                    }

                    if (info.ipv6Time >= 1000) {
                        info.ipv6Time -= 1000;
                    } else {
                        info.ipv6Time = 0;
                    }
                }
            }
            if (counter % 5 == 0) {
                this->notifyThrottledApnInfoEvent();
            }
            if (checkRetryTimeElapsed()) {
                timerStarted_ = false;
                this->notifyThrottledApnInfoEvent();
            }
        }
    });
}

bool DataConnectionServerImpl::checkRetryTimeElapsed() {
    std::lock_guard<std::mutex> lock(apnThrottleInfoMtx_);
    for (const auto& info : apnThrottleInfo_) {
        if (info.ipv4Time > 0 || info.ipv6Time > 0) {
            return false;
        }
    }
    apnThrottleInfo_.clear();
    return true;
}

void DataConnectionServerImpl::handleThrottleApnEvent(std::string event) {
    LOG(DEBUG, __FUNCTION__," processing for ", event );
    if (event == RESET  || event == STOP) {
        if (timerStarted_) {
            timerStarted_ = false;
            if (timerFuture_.valid()) {
                timerFuture_.get(); // Wait for the async task to complete
            }
        }

        if (event == RESET){
            // reset apnThrottleInfo_
            updateThrottleApnInfo();
        }

    } else if(event == START) {
        if (!timerStarted_) {
            {
                std::unique_lock<std::mutex> lock(apnThrottleInfoMtx_);
                LOG(DEBUG, __FUNCTION__," apnThrottleInfo_.size() ", apnThrottleInfo_.size() );
                if(apnThrottleInfo_.size() == 0){
                    lock.unlock();
                    updateThrottleApnInfo();
                }
            }
            startThrottleRetryTimer();
        }
    } else {
        LOG(ERROR, __FUNCTION__," Unknown event! ");
    }
}

grpc::Status DataConnectionServerImpl::InitService(ServerContext* context,
    const dataStub::SlotInfo* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IDataConnectionManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IDataConnectionManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {DATA_CONNECTION};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::SetDefaultProfile(ServerContext* context,
    const dataStub::SetDefaultProfileRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "setDefaultProfile";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " updated json with profileId:",
            request->profile_id());
        data.stateRootObj["IDataConnectionManager"]["getDefaultProfile"]["profileId"]
            = request->profile_id();

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::GetDefaultProfile(ServerContext* context,
    const dataStub::GetDefaultProfileRequest* request,
    dataStub::GetDefaultProfileReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "getDefaultProfile";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    int profileId;
    int slotID;

    if (data.status == telux::common::Status::SUCCESS) {
        profileId =
            data.stateRootObj["IDataConnectionManager"]["getDefaultProfile"]
            ["profileId"].asInt();
        slotID =
            static_cast<SlotId>(data.stateRootObj["IDataConnectionManager"]
            ["getDefaultProfile"]["slotId"].asInt());
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);
    response->set_slot_id(slotID);
    response->set_profile_id(profileId);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::SetRoamingMode(ServerContext* context,
    const dataStub::SetRoamingModeRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "setRoamingMode";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " updated json with roaming_mode:",
            request->roaming_mode());
        data.stateRootObj["IDataConnectionManager"]["requestRoamingMode"]["isRoamingEnabled"]
            = request->roaming_mode();
        data.stateRootObj["IDataConnectionManager"]["requestRoamingMode"]["profileId"]
            = request->profile_id();

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::RequestRoamingMode(ServerContext* context,
    const dataStub::RequestRoamingModeRequest* request,
    dataStub::RequestRoamingModeReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "requestRoamingMode";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    bool roamingMode;
    int profileId;

    if (data.status == telux::common::Status::SUCCESS) {
        roamingMode =
            data.stateRootObj["IDataConnectionManager"]["requestRoamingMode"]
            ["isRoamingEnabled"].asBool();
        profileId =
            data.stateRootObj["IDataConnectionManager"]["requestRoamingMode"]
            ["profileId"].asInt();
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);
    response->set_roaming_mode(roamingMode);
    response->set_profile_id(profileId);

    return grpc::Status::OK;
}

void DataConnectionServerImpl::getInactiveInterfaces() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<SimulationConfigParser> config =
        std::make_shared<SimulationConfigParser>();

    std::stringstream iss(config->getValue("sim.data.physical_interface_name"));
    std::string parsedVal;
    std::vector<std::string> ifaces;
    inactiveNwIfaces_.clear();
    while (getline(iss, parsedVal, DELIMITER)) {
        if (std::find(activeNwIfaces_.begin(), activeNwIfaces_.end(), parsedVal) ==
            activeNwIfaces_.end()) {
            parsedVal.erase(remove_if(parsedVal.begin(), parsedVal.end(),
                ::isspace), parsedVal.end());
            inactiveNwIfaces_.push_back( parsedVal );
        }
    }
}

bool DataConnectionServerImpl::getIpv4Address(const std::string &ifaceName,
    std::string &ipAddress, std::string &gatewayAddress,
    std::string &dnsPrimaryAddress, std::string &dnsSecondaryAddress) {
    LOG(DEBUG, __FUNCTION__);
    bool ifaceFound = false;
    struct ifaddrs *ifaceAddresses;

    if(getifaddrs(&ifaceAddresses) < 0) {
        LOG(DEBUG, __FUNCTION__, " failure in fetching n/w interfaces");
        return ifaceFound;
    }
    struct ifaddrs *ifaddr;
    //traversing thru all the available interfaces
    for (ifaddr = ifaceAddresses; ifaddr != NULL; ifaddr=ifaddr->ifa_next) {
        if ((ifaddr->ifa_addr != NULL) &&
            (ifaddr->ifa_addr->sa_family == AF_INET)) {

            std::string ifName(ifaddr->ifa_name);
            //if type is v4 & iface name matches with user provided name
            if (ifaceName == ifName) {
                LOG(DEBUG, __FUNCTION__, " found interface:", ifaceName);

                //fetching ip address
                struct sockaddr_in* ipAddr =
                    (struct sockaddr_in*)ifaddr->ifa_addr;
                char ipAddrStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &ipAddr->sin_addr,
                    ipAddrStr, INET_ADDRSTRLEN);
                ipAddress = ipAddrStr;

                //fetching gw address
                char gwAddrStr[INET_ADDRSTRLEN];
                std::string command = "route -n | grep 'UG[ \t]' | awk '{print $2}'";
                FILE* fp = popen(command.c_str(), "r");

                if(fgets(gwAddrStr, INET_ADDRSTRLEN, fp) != NULL) {
                    gatewayAddress = gwAddrStr;
                    if(gatewayAddress.back() == '\n') {
                        gatewayAddress.pop_back();
                    }
                }
                pclose(fp);

                //fetching dns address
                struct __res_state addr;
                res_ninit(&addr);
                char addressStr[INET_ADDRSTRLEN];
                for (auto idx=0; idx < addr.nscount; idx++) {
                    if (addr.nsaddr_list[idx].sin_family == AF_INET) {
                        inet_ntop(AF_INET, &addr.nsaddr_list[idx].sin_addr.s_addr,
                        addressStr, INET_ADDRSTRLEN);
                        if (dnsPrimaryAddress.size() == 0) {
                            dnsPrimaryAddress = addressStr;
                        } else if (dnsSecondaryAddress.size() == 0) {
                            dnsSecondaryAddress = addressStr;
                        }
                    }
                }
                ifaceFound = true;
            }
        }
    }
    freeifaddrs(ifaceAddresses);
    return ifaceFound;
}

bool DataConnectionServerImpl::getIpv6Address(const std::string &ifaceName,
    std::string &ipAddress, std::string &gatewayAddress,
    std::string &dnsPrimaryAddress, std::string &dnsSecondaryAddress) {
    LOG(DEBUG, __FUNCTION__);
    bool ifaceFound = false;
    struct ifaddrs *ifaceAddresses;

    if(getifaddrs(&ifaceAddresses) < 0) {
        LOG(DEBUG, __FUNCTION__, " failure in fetching n/w interfaces");
        return ifaceFound;
    }
    struct ifaddrs *ifaddr;
    //traversing thru all the available interfaces
    for (ifaddr = ifaceAddresses; ifaddr != NULL; ifaddr=ifaddr->ifa_next) {
        if ((ifaddr->ifa_addr != NULL) &&
        (ifaddr->ifa_addr->sa_family == AF_INET6)) {
            std::string ifName(ifaddr->ifa_name);
            //if type is v6 & iface name matches with user provided name
            if (ifaceName == ifName) {
                LOG(DEBUG, __FUNCTION__, " found interface:", ifaceName);

                //fetching ip address
                struct sockaddr_in6* ipAddr =
                    (struct sockaddr_in6*)ifaddr->ifa_addr;

                //if it is link local address not global unicast
                //address then we skip
                if (IN6_IS_ADDR_LINKLOCAL(&ipAddr->sin6_addr)) {
                    if (ifaddr->ifa_next != NULL) {
                        ifaddr = ifaddr->ifa_next;
                    }
                    continue;
                }
                char ipAddrStr[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &ipAddr->sin6_addr,
                    ipAddrStr, INET6_ADDRSTRLEN);
                ipAddress = ipAddrStr;

                //fetching gw address
                char gwAddrStr[INET6_ADDRSTRLEN];
                std::string command =
                    "ip -6 route | grep 'default[ \t]' | awk '{print $3}'";
                FILE* fp = popen(command.c_str(), "r");

                if(fgets(gwAddrStr, INET6_ADDRSTRLEN, fp) != NULL) {
                    gatewayAddress = gwAddrStr;
                    if(gatewayAddress.back() == '\n') {
                        gatewayAddress.pop_back();
                    }
                }
                pclose(fp);

                //fetching dns address
                std::ifstream ifs("/etc/resolv.conf");
                std::string line;
                if(!ifs.is_open()) {
                    LOG(DEBUG, __FUNCTION__, " failed to open file /etc/resolv.conf");
                    continue;
                }

                if(ifs.good()) {
                    while(std::getline(ifs, line)) {
                        if((line.size() != 0) && (line.substr(0,11) == "nameserver ")) {
                            //entries in "/etc/resolv.conf" are usually nameserver <dns_address>,
                            //so taking the <dns_address> into addrStr.
                            std::string addrStr = line.substr(11);
                            if (DataUtilsStub::isValidIpv6Address(addrStr)) {
                                if (dnsPrimaryAddress.size() == 0) {
                                    dnsPrimaryAddress = addrStr;
                                } else if (dnsSecondaryAddress.size() == 0) {
                                    dnsSecondaryAddress = addrStr;
                                    continue;
                                }
                            }
                        }
                    }
                }

                ifaceFound = true;
            }
        }
    }
    freeifaddrs(ifaceAddresses);
    return ifaceFound;
}

void DataConnectionServerImpl::triggerStartDataCallEvent(int profileId, int slotId,
    std::string ipFamilyType, unsigned int client_id, std::string ifaceName) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(mtx_);
    bool dataCallExist = true;

    if (ifaceName.empty()) {
        //Reading ifaces from .conf file
        getInactiveInterfaces();
    }

    std::shared_ptr<DataCallParams> call;
    if ((slotId == SLOT_ID_1) &&
        (dataCallsSlot1_.find(profileId) != dataCallsSlot1_.end())) {
        call = dataCallsSlot1_.at(profileId);
    } else if ((slotId == SLOT_ID_2) &&
        (dataCallsSlot2_.find(profileId) != dataCallsSlot2_.end())) {
        call = dataCallsSlot2_.at(profileId);
    }

    if (!call) {
        LOG(DEBUG, __FUNCTION__, " call not found, creating call instance");
        //creating cached datacall object if it doesn't already exist
        call = std::make_shared<DataCallParams>();
        {
            //if user provides interface name during start data call, we start
            //data call with user provided iface name else we take iface name
            //from .conf file.
            if (ifaceName.empty()) {
                if (inactiveNwIfaces_.size() != 0) {
                    auto itr = inactiveNwIfaces_.begin();
                    call->ifaceName = *itr;
                }
            } else {
                call->ifaceName = ifaceName;
            }
        }
        call->slotId = slotId;
        call->ipFamilyType = ipFamilyType;
        call->ownersId.insert(client_id);
        dataCallExist = false;
    }

    //getting IpFamily V4 details
    if (ipFamilyType ==
        DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV4) ||
        ipFamilyType ==
        DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV4V6)) {
            getIpv4Address(call->ifaceName, call->v4IpAddress, call->v4GwAddress,
                call->v4dnsPrimaryAddress, call->v4dnsSecondaryAddress);
    }

    //getting IpFamily V6 details
    if (ipFamilyType ==
        DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV6) ||
        ipFamilyType ==
        DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV4V6)) {
            getIpv6Address(call->ifaceName, call->v6IpAddress, call->v6GwAddress,
                call->v6dnsPrimaryAddress, call->v6dnsSecondaryAddress);
    }

    bool ipv4Supported = (call->v4IpAddress.length() == 0)? false : true;
    bool ipv6Supported = (call->v6IpAddress.length() == 0)? false : true;

    ::dataStub::StartDataCallEvent startDataCallEvent;
    ::eventService::EventResponse anyResponse;

    startDataCallEvent.set_profile_id(profileId);
    startDataCallEvent.set_slot_id(slotId);
    startDataCallEvent.set_ip_family_type(call->ipFamilyType);
    startDataCallEvent.set_iface_name(call->ifaceName);
    startDataCallEvent.set_ipv4_address(call->v4IpAddress);
    startDataCallEvent.set_gwv4_address(call->v4GwAddress);
    startDataCallEvent.set_v4dns_primary_address(call->v4dnsPrimaryAddress);
    startDataCallEvent.set_v4dns_secondary_address(call->v4dnsSecondaryAddress);
    startDataCallEvent.set_ipv6_address(call->v6IpAddress);
    startDataCallEvent.set_gwv6_address(call->v6GwAddress);
    startDataCallEvent.set_v6dns_primary_address(call->v6dnsPrimaryAddress);
    startDataCallEvent.set_v6dns_secondary_address(call->v6dnsSecondaryAddress);

    anyResponse.set_filter("data_connection");
    anyResponse.mutable_any()->PackFrom(startDataCallEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);

    //keeping local copy of data call params in server
    if ((!dataCallExist) && (ipv4Supported || ipv6Supported)) {
        LOG(DEBUG, __FUNCTION__, " caching data call params in server for ", call->ifaceName);
        auto it = std::find(inactiveNwIfaces_.begin(), inactiveNwIfaces_.end(), call->ifaceName);
        if (it != inactiveNwIfaces_.end()) {
            inactiveNwIfaces_.erase(it);
        }

        activeNwIfaces_.push_back(call->ifaceName);

        if (slotId == SLOT_ID_1) {
            dataCallsSlot1_[profileId] = call;
        } else {
            dataCallsSlot2_[profileId] = call;
        }
    }
}

bool DataConnectionServerImpl::isWwanConnectivityAllowed(int slotId) {
    LOG(DEBUG, __FUNCTION__);
    bool isAllowed = true;

    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestWwanConnectivityConfig";
    std::string stateMethod = "requestWwanConnectivityConfig";

    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return false;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int slotIdx = (slotId == SLOT_2) ? 1 : 0;
        isAllowed = data.stateRootObj[subsystem][stateMethod]["isAllowed"][slotIdx].asBool();
    }

    return isAllowed;
}

grpc::Status DataConnectionServerImpl::StartDatacall(ServerContext* context,
    const dataStub::DataCallInputParams* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "startDataCall";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    int slotId = request->slot_id();
    std::string ifaceName = request->iface_name();
    if (!isWwanConnectivityAllowed(slotId)) {
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    }

    if (std::find(activeNwIfaces_.begin(), activeNwIfaces_.end(), ifaceName)
        != activeNwIfaces_.end()) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        int profileId = request->profile_id();
        bool ipFamilyMismatch = false;
        std::string ipFamilyType = DataUtilsStub::convertIpFamilyEnumToString(
                    request->ip_family_type().ip_family_type());
        std::shared_ptr<DataCallParams> dataCall;
        unsigned int client_id = request->client_id();

        if ((slotId == SLOT_ID_1) &&
            (dataCallsSlot1_.find(profileId) != dataCallsSlot1_.end())) {
            dataCall = dataCallsSlot1_.at(profileId);
        } else if ((slotId == SLOT_ID_2) &&
            (dataCallsSlot2_.find(profileId) != dataCallsSlot2_.end())) {
            dataCall = dataCallsSlot2_.at(profileId);
        }

        //updating the datacall status, of locally stored datacall in server.
        if (dataCall) {
            LOG(DEBUG, __FUNCTION__, " datacall already exist");
            auto currentFamily = dataCall->ipFamilyType;
            if (ipFamilyType != currentFamily) {
                dataCall->ipFamilyType =
                    DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV4V6);
                //to cover IpFamilyType mismatch usecases For ex: user starts v4 datacall first
                // & later starts v6 datacall for same profile.
                ipFamilyMismatch = true;
            }
            dataCall->ownersId.insert(client_id);
        }

        //If datacall doesn't exist or there is an IPFamily mismatch
        //trigger the start datacall event with new IPFamilyType.
        if ((!dataCall) || (ipFamilyMismatch)) {
            auto f = std::async(std::launch::deferred,
                    [this, profileId, slotId, ipFamilyType, data, client_id, ifaceName]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(data.cbDelay));
                    this->triggerStartDataCallEvent(profileId, slotId, ipFamilyType,
                        client_id, ifaceName);
                }).share();
            taskQ_->add(f);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

void DataConnectionServerImpl::triggerStopDataCallEvent(int profileId, int slotId,
    std::string ipFamilyType, std::string ifaceName) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(mtx_);
    ::dataStub::StopDataCallEvent stopDataCallEvent;
    ::eventService::EventResponse anyResponse;

    stopDataCallEvent.set_profile_id(profileId);
    stopDataCallEvent.set_slot_id(slotId);
    stopDataCallEvent.set_ip_family_type(ipFamilyType);

    anyResponse.set_filter("data_connection");
    anyResponse.mutable_any()->PackFrom(stopDataCallEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);

    LOG(DEBUG, __FUNCTION__, " for::", ifaceName);
    inactiveNwIfaces_.push_back(ifaceName);
    auto itr = std::find(activeNwIfaces_.begin(), activeNwIfaces_.end(), ifaceName);
    if (itr != activeNwIfaces_.end()) {
        activeNwIfaces_.erase(itr);
    }

    //To handle the use cases which are impacted if no datacall exists, we are letting
    //other managers know that all the active calls have been teared down.
    //For ex: DataFilterManager DataRestrictMode shall be diabled if no active datacall.
    bool triggerNotification = false;
    if (slotId == SLOT_ID_1) {
        if (dataCallsSlot1_.size() == 0) {
            triggerNotification = true;
        }
    } else {
        if (dataCallsSlot2_.size() == 0) {
            triggerNotification = true;
        }
    }

    if (triggerNotification) {
        ::dataStub::NoActiveDataCall dataCallNotification;
        ::eventService::ServerEvent anyResponse;

        dataCallNotification.set_slot_id(slotId);
        anyResponse.set_filter("data_connection_server");
        anyResponse.mutable_any()->PackFrom(dataCallNotification);
        auto& serverEventManager = ServerEventManager::getInstance();
        serverEventManager.sendServerEvent(anyResponse);
    }
}

grpc::Status DataConnectionServerImpl::StopDatacall(ServerContext* context,
    const dataStub::DataCallInputParams* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string ipFamilyType = DataUtilsStub::convertIpFamilyEnumToString(
            request->ip_family_type().ip_family_type());
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "stopDataCall";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        int profileId = request->profile_id();
        int slotId = request->slot_id();
        unsigned int client_id = request->client_id();
        std::shared_ptr<DataCallParams> dataCall;

        if ((slotId == SLOT_ID_1) &&
            (dataCallsSlot1_.find(profileId) != dataCallsSlot1_.end())) {
            dataCall = dataCallsSlot1_.at(profileId);
        } else if ((slotId == SLOT_ID_2) &&
            (dataCallsSlot2_.find(profileId) != dataCallsSlot2_.end())) {
            dataCall = dataCallsSlot2_.at(profileId);
        }

        if (dataCall) {
            LOG(DEBUG, __FUNCTION__, " datacall ref_count::",
                dataCall->ownersId.size());
            dataCall->ownersId.erase(client_id);
            if (dataCall->ownersId.size() != 0) {
                //To handle datacall ownership usecase, triggerStopDataCallEvent only if
                //last owner triggers stopDataCall.
                response->set_error(commonStub::ErrorCode::DEVICE_IN_USE);
                return grpc::Status::OK;
            }

            //removing cached datacall object
            auto currentFamily = dataCall->ipFamilyType;
            if (ipFamilyType == currentFamily) {
                if (slotId == SLOT_ID_1) {
                    dataCallsSlot1_.erase(profileId);
                } else {
                    dataCallsSlot2_.erase(profileId);
                }
            }

            std::string ifaceName = dataCall->ifaceName;
            auto f = std::async(std::launch::async, [this, profileId, slotId,
                ipFamilyType, ifaceName, data]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(data.cbDelay));
                    this->triggerStopDataCallEvent(profileId, slotId, ipFamilyType, ifaceName);
                }).share();
            taskQ_->add(f);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::RequestDatacallList(ServerContext* context,
    const dataStub::DataCallInputParams* request, dataStub::RequestDataCallListReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "requestDataCallList";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

void DataConnectionServerImpl::stopActiveDataCalls(SlotId slotId) {
    if (slotId == SLOT_ID_1) {
        clearCachedDataCall(dataCallsSlot1_, true);
    } else if (slotId == SLOT_ID_2) {
        clearCachedDataCall(dataCallsSlot2_, true);
    }
}

void DataConnectionServerImpl::clearCachedDataCall(
    std::map<int, std::shared_ptr<DataCallParams>>& dataCallsMap, bool stopAllCalls,
    const unsigned int& client_id) {
    LOG(DEBUG, __FUNCTION__);

    for (auto itr = dataCallsMap.begin(); itr != dataCallsMap.end();) {
        uint32_t profileId = itr->first;
        std::shared_ptr<DataCallParams> callObj = itr->second;
        auto &owners = callObj->ownersId;

        if (stopAllCalls) {
            this->triggerStopDataCallEvent(profileId, callObj->slotId,
                callObj->ipFamilyType, callObj->ifaceName);
            ++itr;
        } else {
            if (owners.find(client_id) != owners.end()) {
                owners.erase(client_id);

                //To handle datacall ownership usecase, triggerStopDataCallEvent only if
                //last owner exits.
                if (owners.size() == 0) {
                    this->triggerStopDataCallEvent(profileId, callObj->slotId,
                        callObj->ipFamilyType, callObj->ifaceName);
                    itr = dataCallsMap.erase(itr);
                    continue;
                }
                ++itr;
            } else {
                ++itr;
            }
        }
    }

    if (stopAllCalls) {
        dataCallsMap.clear();
    }
}

grpc::Status DataConnectionServerImpl::CleanUpService(ServerContext* context,
    const ::dataStub::ClientInfo* request, ::google::protobuf::Empty* response) {

    LOG(DEBUG, __FUNCTION__, " clearing cached datacalls from server");
    unsigned int client_id = request->client_id();
    clearCachedDataCall(dataCallsSlot1_, false, client_id);
    clearCachedDataCall(dataCallsSlot2_, false, client_id);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::requestConnectedDataCallLists(ServerContext* context,
    const dataStub::CachedDataCallsRequest* request, dataStub::CachedDataCalls* response) {
    LOG(DEBUG, __FUNCTION__);

    int slotId = request->slot_id();
    std::map<int, std::shared_ptr<DataCallParams>> dataCalls;

    if (slotId == SLOT_ID_1) {
        dataCalls = dataCallsSlot1_;
    } else {
        dataCalls = dataCallsSlot2_;
    }

    for (const auto &itr: dataCalls) {
        uint32_t profileId = itr.first;
        const auto &callObj = itr.second;
        dataStub::StartDataCallEvent *call = response->add_datacalls();
        call->set_profile_id(profileId);
        call->set_iface_name(callObj->ifaceName);
        call->set_ip_family_type(callObj->ipFamilyType);
        call->set_ipv4_address(callObj->v4IpAddress);
        call->set_gwv4_address(callObj->v4GwAddress);
        call->set_v4dns_primary_address(callObj->v4dnsPrimaryAddress);
        call->set_v4dns_secondary_address(callObj->v4dnsSecondaryAddress);
        call->set_ipv6_address(callObj->v6IpAddress);
        call->set_gwv6_address(callObj->v6GwAddress);
        call->set_v6dns_primary_address(callObj->v6dnsPrimaryAddress);
        call->set_v6dns_secondary_address(callObj->v6dnsSecondaryAddress);
    }

    return grpc::Status::OK;
}


bool DataConnectionServerImpl::updateThrottleApnInfo(){
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "requestThrottledApnInfo";

    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return false;
    }

    data.status = static_cast<telux::common::Status>(0);
    if (data.status == telux::common::Status::SUCCESS){
        int size = data.stateRootObj[subsystem][method]["apnThrottleInfo"].size();
        std::lock_guard<std::mutex> lock(apnThrottleInfoMtx_);
        apnThrottleInfo_.clear();
        LOG(DEBUG, __FUNCTION__, " Throttled APN info list size: ", size);
        for (auto idx = 0; idx < size; idx++) {
            APNThrottleInfo apnThrottleInfo;
            apnThrottleInfo.apn = data.stateRootObj[subsystem][method]["apnThrottleInfo"]
                [idx]["apnName"].asString();
            int profile_id_size = data.stateRootObj[subsystem][method]["apnThrottleInfo"]
                [idx]["profileIds"].size();
            for(auto pidx = 0; pidx < profile_id_size; pidx++)
            {
                apnThrottleInfo.profileIds.emplace_back( data.stateRootObj[subsystem][method]
                ["apnThrottleInfo"][idx]["profileIds"][pidx].asInt());
            }
            apnThrottleInfo.ipv4Time = data.stateRootObj[subsystem][method]["apnThrottleInfo"]
                [idx]["ipv4time"].asInt();
            apnThrottleInfo.ipv6Time = data.stateRootObj[subsystem][method]["apnThrottleInfo"]
                [idx]["ipv6time"].asInt();
            apnThrottleInfo.isBlocked = data.stateRootObj[subsystem][method]["apnThrottleInfo"]
                [idx]["isBlocked"].asBool();
            apnThrottleInfo.mcc = data.stateRootObj[subsystem][method]["apnThrottleInfo"]
                [idx]["mcc"].asString();
            apnThrottleInfo.mnc = data.stateRootObj[subsystem][method]["apnThrottleInfo"]
                [idx]["mnc"].asString();
            apnThrottleInfo_.push_back(apnThrottleInfo);
        }
    }
    return true;
}

grpc::Status DataConnectionServerImpl::RequestThrottledApnInfo(ServerContext* context,
        const dataStub::SlotInfo* request,dataStub::ThrottleInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "requestThrottledApnInfo";

    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    data.status = static_cast<telux::common::Status>(0);
    if (data.status == telux::common::Status::SUCCESS){
        std::vector<APNThrottleInfo> throttleInfo;

        {
            std::lock_guard<std::mutex> lock(apnThrottleInfoMtx_);
            throttleInfo.assign(apnThrottleInfo_.begin(), apnThrottleInfo_.end());
        }

        int size = throttleInfo.size();
        LOG(DEBUG, __FUNCTION__, " Throttled APN info list size: ", size);
        for (auto itr = throttleInfo.begin(); itr != throttleInfo.end(); itr++) {
            dataStub::APNThrottleInfo* call = response->mutable_apn_throttle_info_list()
                ->add_rep_apn_throttle_info();
            call->set_apn_name(itr->apn);

            for(auto pitr = itr->profileIds.begin(); pitr != itr->profileIds.end(); pitr++)
            {
                call->add_profile_ids(*pitr);
            }

            call->set_ipv4time(itr->ipv4Time);
            call->set_ipv6time(itr->ipv6Time);
            call->set_is_blocked(itr->isBlocked);
            call->set_mcc(itr->mcc);
            call->set_mnc(itr->mnc);
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

void DataConnectionServerImpl::notifyThrottledApnInfoEvent(){
    LOG(DEBUG, __FUNCTION__);
    dataStub::APNThrottleInfoList apnThrottleInfo;
    std::vector<APNThrottleInfo> throttleInfo;

    {
        std::lock_guard<std::mutex> lock(apnThrottleInfoMtx_);
        throttleInfo.assign(apnThrottleInfo_.begin(), apnThrottleInfo_.end());
    }

    int size = throttleInfo.size();
    LOG(DEBUG, __FUNCTION__, " Throttled APN info list size: ", size);
    for (auto itr = throttleInfo.begin(); itr != throttleInfo.end(); itr++) {
        dataStub::APNThrottleInfo* call = apnThrottleInfo.add_rep_apn_throttle_info();
        call->set_apn_name(itr->apn);

        for(auto pitr = itr->profileIds.begin(); pitr != itr->profileIds.end(); pitr++)
        {
            call->add_profile_ids(*pitr);
        }

        call->set_ipv4time(itr->ipv4Time);
        call->set_ipv6time(itr->ipv6Time);
        call->set_is_blocked(itr->isBlocked);
        call->set_mcc(itr->mcc);
        call->set_mnc(itr->mnc);
    }
    triggerThrottledApnInfoChangedEvent(&apnThrottleInfo);

}


void DataConnectionServerImpl::triggerThrottledApnInfoChangedEvent(
    dataStub::APNThrottleInfoList* response) {
    LOG(DEBUG, __FUNCTION__, " Throttled APN info list size: ",
        response->rep_apn_throttle_info_size());

    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter("data_connection");
    anyResponse.mutable_any()->PackFrom(*response);

    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);

}


bool DataConnectionServerImpl::isAnyDataCallActive(SlotId slotId) {
    bool callActive = false;
    if (slotId == SLOT_ID_1) {
        callActive = (dataCallsSlot1_.size() == 0)? false : true;
    } else if (slotId == SLOT_ID_2) {
        callActive = (dataCallsSlot2_.size() == 0)? false : true;
    }

    return callActive;
}
