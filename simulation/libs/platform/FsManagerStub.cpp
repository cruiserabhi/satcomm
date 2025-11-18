/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "FsManagerStub.hpp"
#include "libs/common/CommonUtils.hpp"
#include "CommandCallbackManager.hpp"

#define RPC_FAIL_SUFFIX " RPC Request failed - "
#define FS_MANAGER_FILTER "fs_manager"

namespace telux {
namespace platform {

using namespace telux::common;

FsManagerStub::FsManagerStub()
   : SimulationManagerStub<FsManagerService>(std::string("IFsManager"))
   , clientEventMgr_(ClientEventManager::getInstance())
   , otaStartCmdCallbackId_(INVALID_COMMAND_ID)
   , otaResumeCmdCallbackId_(INVALID_COMMAND_ID)
   , otaEndCmdCallbackId_(INVALID_COMMAND_ID)
   , abSyncCmdCallbackId_(INVALID_COMMAND_ID) {
    LOG(DEBUG, __FUNCTION__);
}

FsManagerStub::~FsManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void FsManagerStub::createListener() {
    LOG(DEBUG, __FUNCTION__);

    listenerMgr_ = std::make_shared<ListenerManager<IFsListener>>();
}

void FsManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}

void FsManagerStub::setInitCbDelay(uint32_t cbDelay) {
    cbDelay_ = cbDelay;
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
}

uint32_t FsManagerStub::getInitCbDelay() {
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
    return cbDelay_;
}

Status FsManagerStub::init() {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::SUCCESS;

    try {
        createListener();
    } catch (std::bad_alloc &e) {
        LOG(ERROR, __FUNCTION__, ": Invalid listener instance");
        return Status::FAILED;
    }
    status = registerDefaultIndications();
    return status;
}

void FsManagerStub::notifyServiceStatus(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::weak_ptr<IFsListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    LOG(DEBUG, __FUNCTION__, ":: Notifying fs manager service status: ",
            static_cast<int>(srvcStatus), " to listeners: ", applisteners.size());
    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onServiceStatusChange(srvcStatus);
        }
    }
}

ServiceStatus FsManagerStub::getServiceStatus() {
    return SimulationManagerStub::getServiceStatus();
}

Status FsManagerStub::registerDefaultIndications() {
    Status status = Status::FAILED;
    LOG(INFO, __FUNCTION__, ":: Registering default SSR indications");

    status = clientEventMgr_.registerListener(shared_from_this(), FS_MANAGER_FILTER);
    if ((status != Status::SUCCESS) &&
        (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }
    return status;
}

Status FsManagerStub::initSyncComplete(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    registerDefaultIndications();

    if (srvcStatus != ServiceStatus::SERVICE_AVAILABLE)
    {
        return Status::FAILED;
    }

    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, ":: Invalid instance ");
        return Status::FAILED;
    }

    return Status::SUCCESS;
}

void FsManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    // Execute all events in separate thread
    auto f = std::async(std::launch::deferred, [this, event]() {
            if (event.Is<commonStub::GetServiceStatusReply>()) {
                ::commonStub::GetServiceStatusReply ssrResp;
                event.UnpackTo(&ssrResp);
                handleSSREvent(ssrResp);
            } else if (event.Is<::platformStub::FsEventReply>()) {
                ::platformStub::FsEventReply fsEvent;
                event.UnpackTo(&fsEvent);
                handleFsEventReply(fsEvent);
            }else {
                LOG(ERROR, __FUNCTION__, ":: Invalid event");
    }}).share();

    taskQ_.add(f);
}

Status FsManagerStub::startEfsBackup() {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, " FS Manager subsystem is not ready");
        return telux::common::Status::NOTREADY;
    }

    Status status = Status::FAILED;
    ::platformStub::DefaultReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->StartEfsBackup(&context, request, &response);

    if(!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "EFS backup request failed: ", static_cast<int>(status));
        return status;
    }

    LOG(DEBUG, __FUNCTION__, "EFS backup request successful");

    return status;
}

Status FsManagerStub::prepareForEcall() {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, " FS Manager subsystem is not ready");
        return telux::common::Status::NOTREADY;
    }

    Status status = Status::FAILED;
    ::platformStub::DefaultReply response;
    const google::protobuf::Empty request;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->PrepareForEcall(&context, request, &response);

    if(!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ": Ecall preparation request failed: ", static_cast<int>(status));
        return status;
    }

    LOG(DEBUG, __FUNCTION__, ": Ecall preparation request successful");

    return status;
}

Status FsManagerStub::eCallCompleted() {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, " FS Manager subsystem is not ready");
        return telux::common::Status::NOTREADY;
    }

    Status status = Status::FAILED;
    ::platformStub::DefaultReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->ECallCompleted(&context, request, &response);

    if(!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ": Ecall completion request failed: ", static_cast<int>(status));
        return status;
    }

    LOG(DEBUG, __FUNCTION__, ": Ecall completion request successful");

    return status;
}

Status FsManagerStub::prepareForOta(
    OtaOperation otaOperation, telux::common::ResponseCallback responseCb) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, " FS Manager subsystem is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::platformStub::FsEventName request;
    Status status = Status::FAILED;

    if (!responseCb) {
        LOG(ERROR, __FUNCTION__,
            ": Ota preparation request failed, callback cannot be null:", static_cast<int>(status));
        return Status::NOTALLOWED;
    }

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if ((otaOperation == OtaOperation::START)
            && (otaStartCmdCallbackId_ == INVALID_COMMAND_ID)) {
            otaStartCmdCallbackId_ = cmdCallbackMgr_.addCallback(responseCb);
            request.set_fs_event_name("MRC_OTA_START");
        } else if ((otaOperation == OtaOperation::RESUME)
                   && (otaResumeCmdCallbackId_ == INVALID_COMMAND_ID)) {
            otaResumeCmdCallbackId_ = cmdCallbackMgr_.addCallback(responseCb);
            request.set_fs_event_name("MRC_OTA_RESUME");
        } else {
            return Status::NOTALLOWED;
        }
    }

    ::platformStub::DefaultReply response;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->PrepareForOta(&context, request, &response);

    if(!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        LOG(ERROR, __FUNCTION__, ": Ota preparation request failed: ", static_cast<int>(status));
        if (otaOperation == OtaOperation::START) {
            cmdCallbackMgr_.findAndRemoveCallback(otaStartCmdCallbackId_);
            otaStartCmdCallbackId_ = INVALID_COMMAND_ID;
        } else if (otaOperation == OtaOperation::RESUME) {
            cmdCallbackMgr_.findAndRemoveCallback(otaResumeCmdCallbackId_);
            otaResumeCmdCallbackId_ = INVALID_COMMAND_ID;
        }
        return status;
    }

    LOG(DEBUG, __FUNCTION__, ": Ota preparation request successful");

    return status;
}

Status FsManagerStub::otaCompleted(
    OperationStatus operationStatus, telux::common::ResponseCallback responseCb) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, " FS Manager subsystem is not ready");
        return telux::common::Status::NOTREADY;
    }

    const google::protobuf::Empty request;
    Status status = Status::FAILED;

    if (!responseCb) {
        LOG(ERROR, __FUNCTION__,
            ": Ota completion request failed, callback cannot be null:", static_cast<int>(status));
        return Status::NOTALLOWED;
    }

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (otaEndCmdCallbackId_ == INVALID_COMMAND_ID) {
            otaEndCmdCallbackId_ = cmdCallbackMgr_.addCallback(responseCb);
        } else {
            return Status::NOTALLOWED;
        }
    }

    ::platformStub::DefaultReply response;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->OtaCompleted(&context, request, &response);

    if(!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        LOG(ERROR, __FUNCTION__, ": Ota completion request failed: ", static_cast<int>(status));
        cmdCallbackMgr_.findAndRemoveCallback(otaEndCmdCallbackId_);
        otaEndCmdCallbackId_ = INVALID_COMMAND_ID;
        return status;
    }

    LOG(DEBUG, __FUNCTION__, ": Ota completion request successful");

    return status;
}

Status FsManagerStub::startAbSync(telux::common::ResponseCallback responseCb) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, " FS Manager subsystem is not ready");
        return telux::common::Status::NOTREADY;
    }

    Status status = Status::FAILED;

    if (!responseCb) {
        LOG(ERROR, __FUNCTION__,
            ": Start absync request failed, callback cannot be null:", static_cast<int>(status));
        return Status::NOTALLOWED;
    }

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (abSyncCmdCallbackId_ == INVALID_COMMAND_ID) {
            abSyncCmdCallbackId_ = cmdCallbackMgr_.addCallback(responseCb);
        } else {
            return Status::NOTALLOWED;
        }
    }

    const google::protobuf::Empty request;
    ::platformStub::DefaultReply response;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->StartAbSync(&context, request, &response);

    if(!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }

    status = static_cast<telux::common::Status>(response.status());

    if (status != telux::common::Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        LOG(ERROR, __FUNCTION__, ": Start AbSync request failed: ", static_cast<int>(status));
        cmdCallbackMgr_.findAndRemoveCallback(abSyncCmdCallbackId_);
        abSyncCmdCallbackId_ = INVALID_COMMAND_ID;
        return status;
    }

    LOG(DEBUG, __FUNCTION__, ": Start AbSync request successful");

    return status;
}

void FsManagerStub::handleFsEventReply(::platformStub::FsEventReply event) {
    LOG(DEBUG, __FUNCTION__);

    std::string fsEventName = "";
    fsEventName = event.fs_event_name().fs_event_name();
    if (fsEventName == "EFS_BACKUP_START" || fsEventName == "EFS_BACKUP_END") {
        handleEfsBackupEvent(event);
    } else if (fsEventName == "EFS_RESTORE_START" || fsEventName == "EFS_RESTORE_END") {
        handleEfsRestoreEvent(event);
    } else if (fsEventName == "MRC_OTA_START" || fsEventName == "MRC_OTA_RESUME"
                || fsEventName == "MRC_OTA_END" || fsEventName == "MRC_ABSYNC") {
        handleOtaEvent(event);
    } else if (fsEventName == "FS_OPERATION_IMMINENT") {
        handleFsOpImminentEvent(event);
    } else {
        LOG(DEBUG, "Invalid event received: ", fsEventName);
    }
}

void FsManagerStub::handleSSREvent(::commonStub::GetServiceStatusReply ssrResp) {
    LOG(DEBUG, __FUNCTION__);

    ServiceStatus srvcStatus = ServiceStatus::SERVICE_FAILED;

    if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_AVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_AVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_UNAVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_FAILED) {
        srvcStatus = ServiceStatus::SERVICE_FAILED;
    } else {
        // Ignore
        LOG(ERROR, __FUNCTION__, ":: INVALID SSR event");
        return;
    }
    setServiceReady(srvcStatus);
    onFsServiceStatusChange(srvcStatus);
}

void FsManagerStub::onFsServiceStatusChange(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__, ":: Service Status: ", static_cast<int>(srvcStatus));

    if (srvcStatus == getServiceStatus()) {
        return;
    }
    if (srvcStatus != ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: Fs Manager Service is UNAVAILABLE/FAILED");
        setServiceStatus(srvcStatus);
    } else {
        LOG(INFO, __FUNCTION__, ":: Fs Manager Service is AVAILABLE");
        auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
        taskQ_.add(f);
    }
}

void FsManagerStub::handleFsOpImminentEvent(::platformStub::FsEventReply event){
    LOG(DEBUG, __FUNCTION__);

    uint32_t timeToExpiry;
    timeToExpiry = event.reply().delay();

    onFsOpImminentEvent(timeToExpiry);
}

void FsManagerStub::onFsOpImminentEvent(uint32_t timeToExpiry) {
    LOG(DEBUG, __FUNCTION__);

    LOG(DEBUG, __FUNCTION__, "FS operation enable imminent event, time to expire: ", timeToExpiry);
    std::vector<std::weak_ptr<IFsListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    if (applisteners.size() == 0) {
        LOG(DEBUG, __FUNCTION__, "No listener Registered by application");
    } else {
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->OnFsOperationImminentEvent(timeToExpiry);
            }
        }
    }
}

void FsManagerStub::handleOtaEvent(::platformStub::FsEventReply event){
    LOG(DEBUG, __FUNCTION__);

    std::string fsEventName = " ";
    telux::common::ErrorCode error;

    fsEventName = event.fs_event_name().fs_event_name();
    error = static_cast<telux::common::ErrorCode>(event.reply().error());

    onOtaABSyncEvent(fsEventName, error);
}

void FsManagerStub::handleEfsBackupEvent(::platformStub::FsEventReply event){
    LOG(DEBUG, __FUNCTION__);

    std::string fsEventName = " ";
    telux::common::ErrorCode error;

    fsEventName = event.fs_event_name().fs_event_name();
    error = static_cast<telux::common::ErrorCode>(event.reply().error());

    onEfsBackupEvent(fsEventName, error);
}

void FsManagerStub::handleEfsRestoreEvent(::platformStub::FsEventReply event){
    LOG(DEBUG, __FUNCTION__);

    std::string fsEventName = " ";
    telux::common::ErrorCode error;

    fsEventName = event.fs_event_name().fs_event_name();
    error = static_cast<telux::common::ErrorCode>(event.reply().error());

    onEfsRestoreEvent(fsEventName, error);
}

void FsManagerStub::onOtaABSyncEvent(std::string fsEventName, ErrorCode error) {
    LOG(DEBUG, __FUNCTION__);

    std::shared_ptr<ICommandCallback> callback = nullptr;
    int cmdId                                  = INVALID_COMMAND_ID;

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (fsEventName == "MRC_OTA_START") {
            // On filesystem operation disabled
            cmdId = getCmdCallbackId(otaStartCmdCallbackId_, callback);
        } else if (fsEventName == "MRC_OTA_END") {
            // On filesystem operation enabled
            cmdId = getCmdCallbackId(otaEndCmdCallbackId_, callback);
        } else if (fsEventName == "MRC_OTA_RESUME") {
            // On filesystem operation disabled
            cmdId = getCmdCallbackId(otaResumeCmdCallbackId_, callback);
        } else if (fsEventName == "MRC_ABSYNC") {
            cmdId = getCmdCallbackId(abSyncCmdCallbackId_, callback);
        } else {
            LOG(ERROR, __FUNCTION__, ": Unhandled indication for filesystem operation: ");
            return;
        }

        if (callback == nullptr) {
            LOG(ERROR, __FUNCTION__, ": callback is null for cmdId = ", cmdId);
            return;
        }
    }

    cmdCallbackMgr_.executeCallback(callback, error);
}

int FsManagerStub::getCmdCallbackId(int &cmdId, std::shared_ptr<ICommandCallback> &callback) {
    LOG(DEBUG, __FUNCTION__);

    if (cmdId == INVALID_COMMAND_ID) {
        LOG(ERROR, __FUNCTION__, ": cmdId is invalid");
        return cmdId;
    }
    callback = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    cmdId    = INVALID_COMMAND_ID;
    return cmdId;
}

void FsManagerStub::onEfsBackupEvent(std::string fsEventName, ErrorCode error) {
    LOG(DEBUG, __FUNCTION__);

    telux::platform::EfsEventInfo eventInfo{};

    if (fsEventName == "EFS_BACKUP_START") {
        eventInfo.event = telux::platform::EfsEvent::START;
    } else if (fsEventName == "EFS_BACKUP_END") {
        eventInfo.event = telux::platform::EfsEvent::END;
    } else {
        LOG(ERROR, __FUNCTION__,
            "Unhandled EFS backup event: ", fsEventName);
        return;
    }

    eventInfo.error = error;
    LOG(DEBUG, __FUNCTION__, "EFS backup event: ", static_cast<int>(eventInfo.event));

    std::vector<std::weak_ptr<IFsListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    if (applisteners.size() == 0) {
        LOG(DEBUG, __FUNCTION__, "No listener Registered by application");
    } else {
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->OnEfsBackupEvent(eventInfo);
            }
        }
    }
}

void FsManagerStub::onEfsRestoreEvent(std::string fsEventName, ErrorCode error) {
    LOG(DEBUG, __FUNCTION__);

    telux::platform::EfsEventInfo eventInfo{};

    if (fsEventName == "EFS_RESTORE_START") {
        eventInfo.event = telux::platform::EfsEvent::START;
    } else if (fsEventName == "EFS_RESTORE_END") {
        eventInfo.event = telux::platform::EfsEvent::END;
    } else {
        LOG(ERROR, __FUNCTION__,
            "Unhandled EFS restore event: ", fsEventName);
        return;
    }

    eventInfo.error = error;
    LOG(DEBUG, __FUNCTION__, "EFS restore event: ", static_cast<int>(eventInfo.event));

    std::vector<std::weak_ptr<IFsListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    if (applisteners.size() == 0) {
        LOG(DEBUG, __FUNCTION__, "No listener Registered by application");
    } else {
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->OnEfsRestoreEvent(eventInfo);
            }
        }
    }
}

Status FsManagerStub::registerListener(std::weak_ptr<IFsListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return listenerMgr_->registerListener(listener);
}

Status FsManagerStub::deregisterListener(std::weak_ptr<IFsListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return listenerMgr_->deRegisterListener(listener);
}

}  // end of namespace platform
}  // end of namespace telux
