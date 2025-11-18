/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include <thread>

#include "DataSettingsServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/data/DataUtilsStub.hpp"

#define DATA_SETTINGS_API_LOCAL_JSON "api/data/IDataSettingsManagerLocal.json"
#define DATA_SETTINGS_STATE_JSON "system-state/data/IDataSettingsManagerState.json"
#define DBG_LOG_LEVEL_1 0

#define SLOT_2 2
#define REMOTE 1
#define PERM "PERMANENT"
#define TEMP "TEMPORARY"

DataSettingsServerImpl::DataSettingsServerImpl(
    std::shared_ptr<DataConnectionServerImpl> dcmServerImpl):
    dcmServerImpl_(dcmServerImpl) {
    LOG(DEBUG, __FUNCTION__);
    updateDdsInfo();
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

DataSettingsServerImpl::~DataSettingsServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status DataSettingsServerImpl::InitService(ServerContext* context,
    const dataStub::InitRequest* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = DATA_SETTINGS_API_LOCAL_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IDataSettingsManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IDataSettingsManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

void DataSettingsServerImpl::updateDdsInfo() {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestDdsSwitch";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return;
    }

    ddsInfo_.type = static_cast<telux::data::DdsType>(
        (data.stateRootObj[subsystem][method]["DdsType"].asString()
        == PERM) ? 0 : 1);
    ddsInfo_.slotId = static_cast<SlotId>(
        data.stateRootObj[subsystem][method]["SlotId"].asInt());
}

grpc::Status DataSettingsServerImpl::SetDdsSwitch(ServerContext* context,
    const dataStub::SetDdsSwitchRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestDdsSwitch";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    } else if (!telux::common::DeviceConfig::isMultiSimSupported()) {
        data.error = telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
    } else if ((ddsInfo_.slotId == static_cast<SlotId>(request->slot_id())) &&
        ((ddsInfo_.type == static_cast<telux::data::DdsType>(
        request->switch_type())) || ((ddsInfo_.type == telux::data::DdsType::PERMANENT)
        && (static_cast<telux::data::DdsType>(request->switch_type()) ==
        telux::data::DdsType::TEMPORARY)))) {
        //If for a slot_id, the requested switch_type is same as existing switch_type or
        //switch_type is from PERMANENT to TEMPORARY, it is not allowed.
        data.error = telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        ddsInfo_.type = static_cast<telux::data::DdsType>(request->switch_type());
        ddsInfo_.slotId = static_cast<SlotId>(request->slot_id());

        // we are only updating json if it is PERM switch, since TEMP
        //switch is not persistent sccross reboots.
        if (request->switch_type() == 0) {
            data.stateRootObj[subsystem][method]["DdsType"]
                = (request->switch_type() == 0) ? PERM : TEMP;
            data.stateRootObj[subsystem][method]["SlotId"]
                = request->slot_id();
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::RequestCurrentDdsSwitch(ServerContext* context,
    const dataStub::CurrentDdsSwitchRequest* request,
    dataStub::CurrentDdsSwitchResponse* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestDdsSwitch";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        response->set_current_switch(static_cast<int>(ddsInfo_.type));
        response->set_slot_id(ddsInfo_.slotId);
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::setBandInterferenceConfig(ServerContext* context,
    const dataStub::BandInterferenceConfig* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestBandInterferenceConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        data.stateRootObj[subsystem][method]["enable"] = request->enable();
        if (request->enable()) {
            data.stateRootObj[subsystem][method]["priority"]
                = (request->priority() == 0) ? "N79" : "WLAN";
            data.stateRootObj[subsystem][method]["wlanWaitTimeInSec"]
                = request->wlan_wait_time_in_sec();
            data.stateRootObj[subsystem][method]["n79WaitTimeInSec"]
                = request->n79_wait_time_in_sec();
        }
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::requestBandInterferenceConfig(ServerContext* context,
    const dataStub::BandInterferenceRequest* request,
    dataStub::BandInterferenceReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestBandInterferenceConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int isenabled =
            data.stateRootObj[subsystem][method]["enable"].asBool();
        response->mutable_config()->set_enable(isenabled);
        if (isenabled) {
            response->mutable_config()->set_priority(
                data.stateRootObj[subsystem][method]["priority"].asString() == "N79" ? 0 : 1);
            response->mutable_config()->set_wlan_wait_time_in_sec(
                data.stateRootObj[subsystem][method]["wlanWaitTimeInSec"].asInt());
            response->mutable_config()->set_n79_wait_time_in_sec(
                data.stateRootObj[subsystem][method]["n79WaitTimeInSec"].asInt());
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::SetWwanConnectivityConfig(ServerContext* context,
    const dataStub::SetWwanConnectivityConfigRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestWwanConnectivityConfig";
    std::string stateMethod = "requestWwanConnectivityConfig";

    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int slotId = request->slot_id();
        int slotIdx = (request->slot_id() == SLOT_2) ? 1 : 0;
        bool isAllowed = request->is_wwan_connectivity_allowed();
        data.stateRootObj[subsystem][stateMethod]["isAllowed"][slotIdx] = isAllowed;

        auto f = std::async(std::launch::async,
            [this, slotId, isAllowed, data]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(data.cbDelay + 100));
                //stopping active datacalls on the requested slot_id
                if ((!isAllowed) && (this->dcmServerImpl_)) {
                    this->dcmServerImpl_->stopActiveDataCalls(
                        static_cast<SlotId>(slotId));
                }
            }).share();
        taskQ_->add(f);

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::RequestWwanConnectivityConfig(ServerContext* context,
    const dataStub::WwanConnectivityConfigRequest* request,
    dataStub::WwanConnectivityConfigReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestWwanConnectivityConfig";
    std::string stateMethod = "requestWwanConnectivityConfig";

    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int slotId = (request->slot_id() == SLOT_2) ? 1 : 0;
        response->set_is_wwan_connectivity_allowed(
            data.stateRootObj[subsystem][stateMethod]["isAllowed"][slotId].asBool());
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::SetMacSecState(ServerContext* context,
    const dataStub::SetMacSecStateRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestMacSecState";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj[subsystem][method]["enabled"] = request->enabled();
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::RequestMacSecState(ServerContext* context,
    const dataStub::MacSecStateRequest* request,
    dataStub::MacSecStateReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestMacSecState";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        response->set_enabled(
            data.stateRootObj[subsystem][method]["enabled"].asBool());
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::setBackhaulPreference(ServerContext* context,
    const dataStub::setBackhaulPreferenceRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestBackhaulPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        Json::Value newPref;
        for (auto pref : request->backhaul_pref()) {
            newPref.append(Json::Value(convertEnumToBackhaulPrefString(
                static_cast<::dataStub::BackhaulPreference>(pref))));
        }
        data.stateRootObj[subsystem][method]["backhaulPref"] = newPref;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::requestBackhaulPreference(ServerContext* context,
    const dataStub::RequestBackhaulPreference* request,
    dataStub::BackhaulPreferenceReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestBackhaulPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int size = data.stateRootObj[subsystem][method]["backhaulPref"].size();
        for (auto idx = 0; idx < size; idx++) {
            response->add_backhaul_pref(convertBackhaulPrefStringToEnum(
                data.stateRootObj[subsystem][method]["backhaulPref"][idx].asString()));
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::switchBackHaul(ServerContext* context,
    const dataStub::switchBackHaulRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "switchBackHaul";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj[subsystem][method]["backhaul"] =
            convertEnumToBackhaulPrefString(
            static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
        data.stateRootObj[subsystem][method]["slotId"] = request->slot_id();
        data.stateRootObj[subsystem][method]["profileId"] = request->profile_id();
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::setIpPassThroughConfig(ServerContext* context,
        const dataStub::setIpptConfigRequest* request, dataStub::setIpptConfigReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "setIpPassThroughConfig";
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        auto &ipptConfigs = data.stateRootObj[subsystem]["getIpPassThroughConfig"];
        int slotId = (request->slot_id()-1), idx =0;
        auto &ipptConfig = ipptConfigs[slotId];
        auto &configs = ipptConfig["ipptConfig"];
        auto configSize = configs.size();
        auto profileId  = request->profile_id();
        auto vlanId     = request->vlan_id();
        bool isConfigSame = false;

        IpptStruct ipptS;
        IpptStruct *ipptStruct = &ipptS;

        ipptStruct->ipptOpr  = DataUtilsStub::convertEnumToIpptOprString(request->ippt_opr());
        ipptStruct->ifType   = DataUtilsStub::convertEnumToInterfaceTypeString(
                request->interface_type());
        ipptStruct->macAddr  = request->mac_address();

        auto configFound = isIpptConfigExist(request, configs, configSize, idx, isConfigSame,
                ipptStruct);

        Json::Value newConfig;
        if (!configFound) {
            LOG(DEBUG, __FUNCTION__, " ipptConfig not found, adding new config");

            newConfig["profileId"] = profileId;;
            newConfig["vlanId"] = vlanId;
            newConfig["interfaceType"] = ipptStruct->ifType;
            newConfig["macAddr"] = ipptStruct->macAddr;
            newConfig["ipptOperation"] = ipptStruct->ipptOpr;
            newConfig["newConfig"]     = true;
            data.stateRootObj[subsystem]["getIpPassThroughConfig"][slotId]["ipptConfig"][configSize]
                = newConfig;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            LOG(DEBUG, __FUNCTION__, " ipptConfig found at idx: ", idx);
            // Existing configuration
            if (isConfigSame) {
                LOG(DEBUG, __FUNCTION__, " Same ipptConfig exist: ", idx);
                data.error = telux::common::ErrorCode::NO_EFFECT;
            } else {
                LOG(DEBUG, __FUNCTION__, " ipptConfig found for vlan: ", vlanId, ", profileId: ",
                        profileId, ", updating to new ipptConfig");

                if (ipptStruct->ipptOpr == "ENABLE") {
                    if ((ipptStruct->ifType != "UNKNOWN") &&
                            ((!ipptStruct->macAddr.empty()))) {
                        LOG(DEBUG, __FUNCTION__, " updating new ipptConfig for vlan: ",
                                vlanId, ", profileId: ", profileId);

                        configs[idx]["interfaceType"] = ipptStruct->ifType;
                        configs[idx]["macAddr"]       = ipptStruct->macAddr;
                        configs[idx]["newConfig"]     = true;
                    } else {
                        configs[idx]["newConfig"]     = false;
                    }
                }

                LOG(DEBUG, __FUNCTION__, " updating ipptConfig operation: ", ipptStruct->ipptOpr,
                        "for vlan: ", vlanId, ", profileId: ");

                configs[idx]["ipptOperation"] = ipptStruct->ipptOpr;
                JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
            }
        }
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::getIpPassThroughConfig(ServerContext* context,
        const dataStub::getIpptConfigRequest* request,
        dataStub::getIpptConfigReply* response) {

    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "getIpPassThroughConfig";
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " failed, code: ", static_cast<int>(error));
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        auto &ipptConfigs = data.stateRootObj[subsystem][method];
        int slotId = (request->slot_id()-1), idx =0;
        auto &ipptConfig = ipptConfigs[slotId];
        auto &configs = ipptConfig["ipptConfig"];
        auto configSize = configs.size();
        auto ifType     = ::dataStub::InterfaceType::UNKNOWN;
        auto ipptOpr    = ::dataStub::IpptOperation_Operation_UNKNOWN;
        auto macAddr    = std::string("");
        bool isConfigSame = false;

        auto configFound = isIpptConfigExist(request, configs, configSize, idx, isConfigSame);

        if (configFound) {
            LOG(DEBUG, __FUNCTION__, " ipptConfig found at idx: ", idx);

            ifType = DataUtilsStub::convertInterfaceTypeStringToEnum(
                    configs[idx]["interfaceType"].asString());
            ipptOpr = DataUtilsStub::convertIpptOprStringToEnum(
                    configs[idx]["ipptOperation"].asString());
            macAddr  = configs[idx]["macAddr"].asString();
            response->set_interface_type(ifType);
            response->mutable_ippt_opr()->set_ippt_opr(ipptOpr);
            response->set_mac_address(macAddr);
        }

        response->set_interface_type(ifType);
        response->mutable_ippt_opr()->set_ippt_opr(ipptOpr);
        response->set_mac_address(macAddr);
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::GetIpPassThroughNatConfig(ServerContext* context,
        const google::protobuf::Empty *request, dataStub::getIpptNatConfigReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "getIpPassThroughNatConfig";
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " failed, code: ", static_cast<int>(error));
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        auto isNatEnabled =
            data.stateRootObj[subsystem][method]["natEnable"].asBool();

        LOG(DEBUG, __FUNCTION__, " isNatEnabled: ", isNatEnabled);
        response->set_enable_nat(isNatEnabled);
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::SetIpPassThroughNatConfig(ServerContext* context,
        const dataStub::setIpptNatConfigRequest* request,
        dataStub::setIpptNatConfigReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "setIpPassThroughNatConfig";
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " failed, code: ", static_cast<int>(error));
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        auto isNatEnabled =
            data.stateRootObj[subsystem]["getIpPassThroughNatConfig"]["natEnable"].asBool();

        if (isNatEnabled == request->enable_nat()) {
            LOG(DEBUG, __FUNCTION__, " No change in NAT config");
            data.error = telux::common::ErrorCode::NO_EFFECT;
            response->set_error(static_cast<commonStub::ErrorCode>(data.error));
            return grpc::Status::OK;
        }

        LOG(DEBUG, __FUNCTION__, " isNatEnabled: ", isNatEnabled);
        data.stateRootObj[subsystem]["getIpPassThroughNatConfig"]["natEnable"] =
            request->enable_nat();
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::getIpConfig(ServerContext* context,
        const dataStub::getIpConfigRequest* request, dataStub::getIpConfigReply* response) {

    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "getIpConfig";
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " failed, code: ", static_cast<int>(error));
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        auto vlanId      = request->vlan_id();
        auto ipFamilyType= DataUtilsStub::convertIpFamilyToStruct(request->ip_family_type());
        auto ipType      = dataStub::IpType_IpAssignType_UNKNOWN;
        auto ipAssign    = dataStub::IpAssign_IpAssignOperation_UNKNOWN;
        auto *ipAddrInfo = response->mutable_ip_addr_info();

        auto configFound = isIpConfigExist(request, telux::data::IpAssignType::UNKNOWN,
                telux::data::IpAssignOperation::UNKNOWN);

        if (configFound) {
            LOG(DEBUG, __FUNCTION__, " ipConfig found for VlanId: ", vlanId);
            IpConfigStruct ipConfig;
            auto itr = ipConfigMap_.find(vlanId);
            auto ipConfigM = itr->second.find(ipFamilyType);

            ipType = DataUtilsStub::convertIpTypeToGrpc(ipConfigM->second.ipType);
            ipAssign = DataUtilsStub::convertIpAssignToGrpc(ipConfigM->second.ipAssign);
            response->mutable_ip_type()->set_ip_type(ipType);
            response->mutable_ip_assign()->set_ip_assign(ipAssign);
            DataUtilsStub::convertIpAddrInfoToGrpc(ipConfigM->second.ipAddr, ipAddrInfo);
        } else {
            LOG(DEBUG, __FUNCTION__, " No ipConfig found for VlanId: ", vlanId);
        }
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    return grpc::Status::OK;
}

telux::common::ErrorCode DataSettingsServerImpl::validateV4IpAddr(
        const telux::data::IpAddrInfo &ipAddr) {

    if (!DataUtilsStub::isValidIpv4Address(ipAddr.ifAddress)) {
        LOG(ERROR, __FUNCTION__, " Invalid: ifAddress");
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }
    if (!DataUtilsStub::isValidIpv4Address(ipAddr.gwAddress)) {
        LOG(ERROR, __FUNCTION__, " Invalid: gwAddress");
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }
    if (!DataUtilsStub::isValidIpv4Address(ipAddr.primaryDnsAddress)) {
        LOG(ERROR, __FUNCTION__, " Invalid: primaryDnsAddress");
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }
    if (!DataUtilsStub::isValidIpv4Address(ipAddr.secondaryDnsAddress)) {
        LOG(ERROR, __FUNCTION__, " Invalid: secondaryDnsAddress");
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode DataSettingsServerImpl::modifyIpConfig(IpConfigStruct &ipConfigStruct) {

    auto itr = ipConfigMap_.find(ipConfigStruct.vlanId);

#ifdef DBG_LOG_LEVEL_1
    LOG(DEBUG, __FUNCTION__, " vlanId: ", ipConfigStruct.vlanId);
    LOG(DEBUG, __FUNCTION__, " ifType: ", (ipConfigStruct.ifType ==
                telux::data::InterfaceType::ETH ? "ETH" : "UNKNOWN"));
    LOG(DEBUG, __FUNCTION__, " ipFamily: ", (ipConfigStruct.ipFamilyType ==
                telux::data::IpFamilyType::IPV4 ? "IPV4" : (ipConfigStruct.ipFamilyType ==
                    telux::data::IpFamilyType::IPV6 ? "IPV6" : "UNKNOWN") ));
    LOG(DEBUG, __FUNCTION__, " ipType: ", (ipConfigStruct.ipType ==
                telux::data::IpAssignType::STATIC_IP ? "STATIC_IP" :
                (ipConfigStruct.ipType ==
                 telux::data::IpAssignType::DYNAMIC_IP ? "DYNAMIC_IP" : "UNKNOWN") ));
    LOG(DEBUG, __FUNCTION__, " ipAssign: ", (ipConfigStruct.ipAssign ==
                telux::data::IpAssignOperation::ENABLE ? "ENABLE" : (ipConfigStruct.ipAssign ==
                    telux::data::IpAssignOperation::DISABLE ? "DISABLE" : "RECONFIGURE")));
    LOG(DEBUG, __FUNCTION__, " ifAddr: ", ipConfigStruct.ipAddr.ifAddress);
    LOG(DEBUG, __FUNCTION__, " ifMask: ", ipConfigStruct.ipAddr.ifMask);
    LOG(DEBUG, __FUNCTION__, " gwAddr: ", ipConfigStruct.ipAddr.gwAddress);
    LOG(DEBUG, __FUNCTION__, " pDnsAddr: ", ipConfigStruct.ipAddr.primaryDnsAddress);
    LOG(DEBUG, __FUNCTION__, " sDnsAddr: ",
            ipConfigStruct.ipAddr.secondaryDnsAddress);
#endif

    switch (ipConfigStruct.ipAssign) {
        case telux::data::IpAssignOperation::ENABLE: {
            LOG(DEBUG, __FUNCTION__, " State: Enable");
            if (ipConfigStruct.ipType == telux::data::IpAssignType::STATIC_IP) {
                auto errCode =validateV4IpAddr(ipConfigStruct.ipAddr);
                if( errCode != telux::common::ErrorCode::SUCCESS) {
                    return errCode;
                }
            }
            if ((itr == ipConfigMap_.end()) || (itr->second.find(ipConfigStruct.ipFamilyType)
                        == itr->second.end()))  {
                // Add new config for Vlan/ipFamilyType
                LOG(DEBUG, __FUNCTION__, " Config not found for VlandId: ", ipConfigStruct.vlanId,
                        ", or FamilyType, adding new config");
                ipConfigMap_[ipConfigStruct.vlanId].insert({ipConfigStruct.ipFamilyType,
                        ipConfigStruct});
                return telux::common::ErrorCode::SUCCESS;
            }
            // Vlan config exist for any ipFamilyType
            auto ipFamilyConfig = itr->second.find(ipConfigStruct.ipFamilyType);
            if (ipFamilyConfig->second.ipType != ipConfigStruct.ipType) {
                /* Curr config: IPV4(STATIC) and  State: DISABLE
                 * Req  config: IPV4(DYNAMIC) and State: ENABLE
                 * Above is invalid since current IPV4 is disabled for STATIC_IP where as enable
                 * IPV4 is requested for DYNAMIC_IP, State transition for ENABLE should happen
                 * between same ipType(STATIC_IP -> STATIC_IP or DYNAMIC_IP -> DYNAMIC_IP)
                 */
                return telux::common::ErrorCode::INTERNAL;
            }
            return telux::common::ErrorCode::INTERNAL;
        }
        case telux::data::IpAssignOperation::RECONFIGURE: {
            LOG(DEBUG, __FUNCTION__, " State: RECONFIGURE");
            if (ipConfigStruct.ipType == telux::data::IpAssignType::STATIC_IP) {
                auto errCode =validateV4IpAddr(ipConfigStruct.ipAddr);
                if( errCode != telux::common::ErrorCode::SUCCESS) {
                    return errCode;
                }
            }

            if (itr == ipConfigMap_.end()) {
                // VlanConfig not exist, RECONFIGURE should happen only if vlan config exist
                return telux::common::ErrorCode::INTERNAL;
            }

            auto ipConfigM = itr->second.find(ipConfigStruct.ipFamilyType);
            if (ipConfigM == itr->second.end()) {
                /* VlanConfig exist, but ipFamilyType(V4/V6) config not exist
                 * In case of current state is DISABLE and requested state is RECONFIGURE
                 */
                return telux::common::ErrorCode::INTERNAL;
            }

            LOG(DEBUG, __FUNCTION__, " Config Found for VlanId: ", ipConfigStruct.vlanId);
            LOG(DEBUG, __FUNCTION__, " Config Found for ipFamilyType: ",
                    (ipConfigStruct.ipFamilyType == telux::data::IpFamilyType::IPV4 ?
                     "IPV4" : "IPV6"));

            if ((ipConfigStruct.ipType == telux::data::IpAssignType::STATIC_IP) &&
                    (isIpConfigSame(ipConfigStruct.ipAddr, ipConfigM->second.ipAddr)) ) {
                // Reconfigure request for same ipconfig(only in case of STATIC_IP)
                return telux::common::ErrorCode::NO_EFFECT;

            }
            ipConfigM->second = ipConfigStruct;
            return telux::common::ErrorCode::SUCCESS;
        }
        case telux::data::IpAssignOperation::DISABLE: {
            LOG(DEBUG, __FUNCTION__, " State: DISABLE");
            if (itr == ipConfigMap_.end()) {
                LOG(DEBUG, __FUNCTION__, " Vlan not exists");
                // To DISABLE, VlanConfig should exist
                return telux::common::ErrorCode::INTERNAL;
            }

            auto ipFamilyConfig = itr->second.find(ipConfigStruct.ipFamilyType);

            if (ipFamilyConfig->second.ipType != ipConfigStruct.ipType) {
                LOG(ERROR, __FUNCTION__, " ipType are not same");
                /* Curr config: IPV4(STATIC) and  State: ENABLE
                 * Req  config: IPV4(DYNAMIC) and State: DISABLE
                 * Above is invalid since current IPV4 is enabled for STATIC_IP where as disable
                 * IPV4 is requested for DYNAMIC_IP, State transition for DISABLE should happen
                 * between same ipType(STATIC_IP -> STATIC_IP or DYNAMIC_IP -> DYNAMIC_IP)
                 */
                return telux::common::ErrorCode::INTERNAL;
            }

            // Remove ipConfig entry for ipFamilyType when DISABLE requested
            itr->second.erase(ipConfigStruct.ipFamilyType);
            return telux::common::ErrorCode::SUCCESS;
        }
        default: {
            LOG(DEBUG, __FUNCTION__, " State: Invalid");
            return telux::common::ErrorCode::INTERNAL;
        }
    }
}

grpc::Status DataSettingsServerImpl::setIpConfig(ServerContext* context,
        const dataStub::setIpConfigRequest* request, dataStub::setIpConfigReply* response) {
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "setIpConfig";
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        IpConfigStruct ipConfigStruct;
        telux::data::IpAddrInfo ipAddrStruct;
        ipConfigStruct.vlanId      = request->vlan_id();
        ipConfigStruct.ipType      = DataUtilsStub::convertIpTypeToStruct(request->ip_type());
        ipConfigStruct.ipAssign    = DataUtilsStub::convertIpAssignToStruct(request->ip_assign());
        ipConfigStruct.ifType      = DataUtilsStub::convertInterfaceTypeToStruct(
                request->interface_type());
        ipConfigStruct.ipFamilyType= DataUtilsStub::convertIpFamilyToStruct(
                request->ip_family_type());
        auto &ipAddrInfoGrpc = request->ip_addr_info();

        if (ipConfigStruct.ipType == telux::data::IpAssignType::STATIC_IP) {
            DataUtilsStub::convertIpAddrInfoToStruct(ipAddrInfoGrpc, ipAddrStruct);
        }
        ipConfigStruct.ipAddr = ipAddrStruct;

        switch (ipConfigStruct.ipAssign) {
            case telux::data::IpAssignOperation::ENABLE:
                LOG(DEBUG, __FUNCTION__, " State: Enable");
            case telux::data::IpAssignOperation::DISABLE: {
                LOG(DEBUG, __FUNCTION__, " State: Disable");
                auto configFound = isIpConfigExist(request, ipConfigStruct.ipType,
                        ipConfigStruct.ipAssign);
                if (configFound) {
                    data.error = telux::common::ErrorCode::NO_EFFECT;
                } else {
                    data.error = modifyIpConfig(ipConfigStruct);
                }
                break;
            }
            case telux::data::IpAssignOperation::RECONFIGURE: {
                LOG(DEBUG, __FUNCTION__, " State: Recondifure");
                    data.error = modifyIpConfig(ipConfigStruct);
                break;
            }
            default:
                LOG(DEBUG, __FUNCTION__, " State: Invalid");
                break;
        }
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    return grpc::Status::OK;
}

dataStub::BackhaulPreference DataSettingsServerImpl::convertBackhaulPrefStringToEnum(
    std::string pref) {

    if (pref == "ETH") {
        return ::dataStub::BackhaulPreference::PREF_ETH;
    } else if (pref == "USB") {
        return ::dataStub::BackhaulPreference::PREF_USB;
    } else if (pref == "WLAN") {
        return ::dataStub::BackhaulPreference::PREF_WLAN;
    } else if (pref == "WWAN") {
        return ::dataStub::BackhaulPreference::PREF_WWAN;
    } else if (pref == "BLE") {
        return ::dataStub::BackhaulPreference::PREF_BLE;
    }

    return ::dataStub::BackhaulPreference::INVALID;
}

std::string DataSettingsServerImpl::convertEnumToBackhaulPrefString(
    ::dataStub::BackhaulPreference pref) {

    switch (pref) {
        case ::dataStub::BackhaulPreference::PREF_ETH:
            return "ETH";
        case ::dataStub::BackhaulPreference::PREF_USB:
            return "USB";
        case ::dataStub::BackhaulPreference::PREF_WLAN:
            return "WLAN";
        case ::dataStub::BackhaulPreference::PREF_WWAN:
            return "WWAN";
        case ::dataStub::BackhaulPreference::PREF_BLE:
            return "BLE";
        case ::dataStub::BackhaulPreference::INVALID:
        default:
            return "INVALID";
    }

    return "INVALID";
}

template <typename T>
bool DataSettingsServerImpl::isIpConfigExist(const T* request, const telux::data::IpAssignType
        ipType, const telux::data::IpAssignOperation ipAssign) {
    auto itr = ipConfigMap_.find(request->vlan_id());
    auto ipFamilyType = DataUtilsStub::convertIpFamilyToStruct(request->ip_family_type());
    bool ipV4Found = false, ipV6Found = false;

    IpConfigStruct ipV4ConfigStruct, ipv6ConfigStruct;
    if (itr != ipConfigMap_.end()) {
        // Existing Vlan
        auto ipV4Config = itr->second.find(telux::data::IpFamilyType::IPV4);
        if (ipV4Config != itr->second.end()) {
            LOG(DEBUG, __FUNCTION__, " Found ipV4 config");
            ipV4Found = true;
        }
        auto ipV6Config = itr->second.find(telux::data::IpFamilyType::IPV6);
        if (ipV6Config != itr->second.end()) {
            LOG(DEBUG, __FUNCTION__, " Found ipV6 config");
            ipV6Found = true;
        }

        if (ipType == telux::data::IpAssignType::UNKNOWN) {
            if ((ipV4Found) && (ipFamilyType == telux::data::IpFamilyType::IPV4)) {
                LOG(DEBUG, __FUNCTION__, " ipV4 config exist");
                return true;
            }
            if ((ipV6Found) && (ipFamilyType == telux::data::IpFamilyType::IPV6)) {
                LOG(DEBUG, __FUNCTION__, " ipV6 config exist");
                return true;
            }
            return false;
        }

        if ( (ipAssign == telux::data::IpAssignOperation::ENABLE) ||
                (ipAssign == telux::data::IpAssignOperation::RECONFIGURE)) {
            if ((ipV4Found) && (ipFamilyType == telux::data::IpFamilyType::IPV4)) {
                LOG(DEBUG, __FUNCTION__, " ipV4 config is already EANBLED/RECONFIGED");
                return true;
            }
            if ((ipV6Found) && (ipFamilyType == telux::data::IpFamilyType::IPV6)) {
                LOG(DEBUG, __FUNCTION__, " ipV6 config is already EANBLED/RECONFIGED");
                return true;
            }
        } else if (ipAssign == telux::data::IpAssignOperation::DISABLE) {
            if ((!ipV4Found) && (ipFamilyType == telux::data::IpFamilyType::IPV4)) {
                LOG(DEBUG, __FUNCTION__, " ipV4 config is already DISABLED");
                return true;
            }
            if ((!ipV6Found) && (ipFamilyType == telux::data::IpFamilyType::IPV6)) {
                LOG(DEBUG, __FUNCTION__, " ipV6 config is already DISABLED");
                return true;
            }
        }
        LOG(DEBUG, __FUNCTION__, " ipV4/ipV6 config not exist");
        return false;
    }
    LOG(DEBUG, __FUNCTION__, " VlanId: ", request->vlan_id(), " config not exist");
    return false;
}

template <typename T>
bool DataSettingsServerImpl::isIpptConfigExist(const T *request, const Json::Value &configs,
        int configSize, int& configIdx, bool &isConfigSame, const IpptStruct *ipptStruct) {

    bool isConfigFound = false;
    int idx = 0;
    for (idx=0; idx < configSize; idx++) {
        if (configs[idx]["profileId"] != request->profile_id()) {
            continue;
        }

        if (configs[idx]["vlanId"] != request->vlan_id()) {
            continue;
        }

        isConfigFound = true;
        if (isConfigFound) {
            configIdx = idx;
        }

        if (ipptStruct) {
            if ((ipptStruct->ipptOpr == "DISABLE") &&
                    (ipptStruct->ipptOpr == configs[idx]["ipptOperation"].asString())) {
                isConfigSame = true;
                break;
            } else if (ipptStruct->ipptOpr == "ENABLE") {
                if ((ipptStruct->ifType == "UNKNOWN") || (ipptStruct->macAddr.empty())) {
                    if ( (configs[idx]["newConfig"].asBool() == false) || (ipptStruct->ipptOpr ==
                                configs[idx]["ipptOperation"].asString()) ) {
                        isConfigSame = true;
                        break;
                    }
                }
                if (configs[idx]["ipptOperation"] != ipptStruct->ipptOpr) {
                    continue;
                }
                if (configs[idx]["interfaceType"] != ipptStruct->ifType) {
                    continue;
                }
                if (configs[idx]["macAddr"] != ipptStruct->macAddr) {
                    continue;
                }
                isConfigSame = true;
                break;
            }
        }
    }
    return isConfigFound;
}

bool DataSettingsServerImpl::isIpConfigSame(const telux::data::IpAddrInfo &newIpConfig,
        const telux::data::IpAddrInfo &currentIpConfig) {
    LOG(DEBUG, __FUNCTION__);

    if (newIpConfig.ifAddress != currentIpConfig.ifAddress) {
        return false;
    }
    if (newIpConfig.ifMask != currentIpConfig.ifMask) {
        return false;
    }
    if (newIpConfig.gwAddress != currentIpConfig.gwAddress) {
        return false;
    }
    if (newIpConfig.primaryDnsAddress != currentIpConfig.primaryDnsAddress) {
        return false;
    }
    if (newIpConfig.secondaryDnsAddress != currentIpConfig.secondaryDnsAddress) {
        return false;
    }
    return true;
}
