/*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*
*     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
* GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
* HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include"SubscriptionManagerServerImpl.hpp"
#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include <telux/common/DeviceConfig.hpp>

#define PATH "system-state/tel/ISubscriptionManagerState.json"
#define SUBSCRIPTION_EVENT "subscriptionInfoChanged"
SubscriptionManagerServerImpl::SubscriptionManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    readJson();
}

grpc::Status SubscriptionManagerServerImpl::readJson() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, PATH);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    return grpc::Status::OK;
}
grpc::Status SubscriptionManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        int cbDelay = rootObj["ISubscriptionManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus =
            rootObj["ISubscriptionManager"]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);

        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::vector<std::string> filters = {"tel_sub"};
            auto &serverEventManager = ServerEventManager::getInstance();
            serverEventManager.registerListener(shared_from_this(), filters);
        }
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status SubscriptionManagerServerImpl::GetServiceStatus(ServerContext* context,
    const google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        int cbDelay = rootObj["ISubscriptionManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus =
            rootObj["ISubscriptionManager"]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);

        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status SubscriptionManagerServerImpl::IsSubsystemReady(ServerContext* context,
    const google::protobuf::Empty* request, commonStub::IsSubsystemReadyReply* response) {
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        bool status = false;
        std::string IsSubsystemReady = rootObj["ISubscriptionManager"]\
            ["IsSubsystemReady"].asString();
        telux::common::ServiceStatus servstatus = CommonUtils::mapServiceStatus(IsSubsystemReady);
        if(servstatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            status = true;
        }
        response->set_is_ready(status);
    }
    return readStatus;
}
grpc::Status SubscriptionManagerServerImpl::GetSubscription(ServerContext* context,
    const ::telStub::GetSubscriptionRequest* request, telStub::Subscription* response) {
    LOG(DEBUG, __FUNCTION__);

    int slotId = request->phone_id();
    int i = 0;
    if(slotId == 2) {
        i = slotId - 1;
    }
    std::string carrier_name =
        rootObj["ISubscriptionManager"]["Subscription"][i]["carrierName"].asString();
    LOG(DEBUG, __FUNCTION__, "Carrier name is",carrier_name);
    std::string phone_number =
        rootObj["ISubscriptionManager"]["Subscription"][i]["phoneNumber"].asString();
    LOG(DEBUG, __FUNCTION__, "Phone number is",phone_number);
    std::string icc_id =
        rootObj["ISubscriptionManager"]["Subscription"][i]["iccId"].asString();
    LOG(DEBUG, __FUNCTION__, "iccid is",icc_id);
    int mcc = rootObj["ISubscriptionManager"]["Subscription"][i]["mcc"].asInt();
    LOG(DEBUG, __FUNCTION__, "mcc is",mcc);
    int mnc = rootObj["ISubscriptionManager"]["Subscription"][i]["mnc"].asInt();
    LOG(DEBUG, __FUNCTION__, "mnc is",mnc);
    std::string imsi = rootObj["ISubscriptionManager"]["Subscription"][i]["imsi"].asString();
    LOG(DEBUG, __FUNCTION__, "imsi is",imsi);
    std::string gid_1 = rootObj["ISubscriptionManager"]["Subscription"][i]["gid1"].asString();
    LOG(DEBUG, __FUNCTION__, "gid1 is",gid_1);
    std::string gid_2 = rootObj["ISubscriptionManager"]["Subscription"][i]["gid2"].asString();
    LOG(DEBUG, __FUNCTION__, "gid2 is",gid_2);

    // Create response
    response->set_carrier_name(carrier_name);
    response->set_icc_id(icc_id);
    response->set_mcc(mcc);
    response->set_mnc(mnc);
    response->set_phone_number(phone_number);
    response->set_imsi(imsi);
    response->set_gid_1(gid_1);
    response->set_gid_2(gid_2);

    return grpc::Status::OK;
}

void SubscriptionManagerServerImpl::onEventUpdate(std::string event) {
    std::string token;
    LOG(DEBUG, __FUNCTION__,"String is ", event );
    if ( SUBSCRIPTION_EVENT == EventParserUtil::getNextToken(event, DEFAULT_DELIMITER)) {
        handlesubscriptionInfoChanged(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void SubscriptionManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == "tel_sub") {
        onEventUpdate(message.event());
    }
}

void SubscriptionManagerServerImpl::handlesubscriptionInfoChanged(std::string eventParams) {
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    int slotId;
    std::string carrierName;
    std::string phoneNumber;
    std::string iccId;
    int mcc;
    int mnc;
    std::string imsi;
    std::string gid1;
    std::string gid2;
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        slotId = 1;
    } else {
        try {
            slotId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    if(slotId == SLOT_ID_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched slot id is: ", slotId
        ,"The leftover string is: ", eventParams);
    //Fetch carrier name
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The carrierName is not passed!");
        carrierName = "";
    } else {
        try {
            carrierName = token;
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched carrierName is: ", carrierName
        , "The leftover string is: ", eventParams);

    //Fetch phoneNumber
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The phonenumber is not passed!");
        phoneNumber = "";
    } else {
        try {
            phoneNumber = token;
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched phoneNumber is: ", phoneNumber
        , "The leftover string is: ", eventParams);

    //Fetch iccId
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The iccId is not passed!");
        iccId = "";
    } else {
        try {
            iccId = token;
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched iccId is: ", iccId,
         "The leftover string is: ", eventParams);

    //Fetch mcc
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The mcc is not passed!");
        mcc = 0;
    } else {
        try {
            mcc = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched mcc is: ", mcc
        , "The leftover string is: ", eventParams);

    //Fetch mnc
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The mnc is not passed!");
        mnc = 0;
    } else {
        try {
            mnc = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched mnc is: ", mnc);

    //Fetch imsi
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The imsi is not passed!");
        imsi = "";
    } else {
        try {
            imsi = token;
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched imsi is: ", imsi
        , "The leftover string is: ", eventParams);

    //Fetch gid1
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The gid1 is not passed!");
        gid1 = "";
    } else {
        try {
            gid1 = token;
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched gid1 is: ", gid1
        , "The leftover string is: ", eventParams);

    //Fetch gid2
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The gid2 is not passed!");
        gid2 = "";
    } else {
        try {
            gid2 = token;
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched gid2 is: ", gid2
        , "The leftover string is: ", eventParams);
    int i = 0;
    if(slotId == 2) {
        i = 1;
    }
    rootObj["ISubscriptionManager"]["Subscription"][i]["carrierName"] = carrierName;
    rootObj["ISubscriptionManager"]["Subscription"][i]["phoneNumber"] = phoneNumber;
    rootObj["ISubscriptionManager"]["Subscription"][i]["iccId"] = iccId;
    rootObj["ISubscriptionManager"]["Subscription"][i]["mcc"] = mcc;
    rootObj["ISubscriptionManager"]["Subscription"][i]["mnc"] = mnc;
    rootObj["ISubscriptionManager"]["Subscription"][i]["imsi"] = imsi;
    rootObj["ISubscriptionManager"]["Subscription"][i]["gid1"] = gid1;
    rootObj["ISubscriptionManager"]["Subscription"][i]["gid2"]= gid2;
    LOG(DEBUG, __FUNCTION__, "Carrier name is",carrierName
    , "Phone number is", phoneNumber
    , "iccid is", iccId
    , "mcc is", mcc
    , "mnc is", mnc
    , "imsi is", imsi
    , "gid1 is", gid1
    , "gid2 is", gid2);
    JsonParser::writeToJsonFile(rootObj, PATH);
    ::telStub::SubscriptionEvent SubscriptionInfoChangeEvent;
    ::eventService::EventResponse anyResponse;

    SubscriptionInfoChangeEvent.set_phone_id(slotId);
    anyResponse.set_filter("tel_sub");
    anyResponse.mutable_any()->PackFrom(SubscriptionInfoChangeEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}
