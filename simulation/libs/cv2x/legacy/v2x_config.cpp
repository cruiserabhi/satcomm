/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdint.h>
#include <stdlib.h>

#include "v2x_log.h"

#include "telux/cv2x/legacy/v2x_config_api.h"
#include <future>
#include <string>
#include <telux/cv2x/Cv2xConfig.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>

using std::make_shared;
using std::promise;
using std::remove;
using std::shared_ptr;
using std::string;
using telux::common::ErrorCode;
using telux::cv2x::ConfigEventInfo;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xConfig;
using telux::cv2x::ICv2xConfigListener;

static shared_ptr<ICv2xConfigListener> gConfigListener  = nullptr;
static cv2x_config_event_listener gConfigChangeCallback = nullptr;

//*****************************************************************************
// Forward declarations
//*****************************************************************************
static void cv2x_config_file_changed_listener(const ConfigEventInfo &config);

//*****************************************************************************
// This class implements the ICv2xConfigListener interface
//*****************************************************************************
class ConfigListener : public ICv2xConfigListener {
    void onConfigChanged(const ConfigEventInfo &info) override {
        cv2x_config_file_changed_listener(info);
    }
};

//*****************************************************************************
// Template function to convert from one enum to another. This assumes that
// the two enum classes match in terms of possible enum values and their
// associated ordinals. It also assumes that the enum types can be safely
// converted into a signed integer.
//*****************************************************************************
template <typename A, typename B>
static void convert_enum(A src, B &dst) {
    int tmp = static_cast<int>(src);
    dst     = static_cast<B>(tmp);
}

static std::shared_ptr<ICv2xConfig> get_cv2x_config_handle() {
    bool cv2xConfigStatusUpdated = false;
    telux::common::ServiceStatus cv2xConfigStatus
        = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xConfigStatusUpdated = true;
        cv2xConfigStatus        = status;
        cv.notify_all();
    };

    auto &factory = Cv2xFactory::getInstance();
    auto config   = factory.getCv2xConfig(statusCb);
    if (!config) {
        LOGE("Failed to get Cv2xConfig\n");
        return nullptr;
    }
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&] { return cv2xConfigStatusUpdated; });
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != cv2xConfigStatus) {
        LOGE("Failed to initialize Cv2xConfig\n");
        return nullptr;
    }

    return config;
}

v2x_status_enum_type v2x_update_configuration(const char *config_file_path) {
    auto config = get_cv2x_config_handle();

    if (not config) {
        LOGE("Failed to acquire Cv2xConfig\n");
        return V2X_STATUS_FAIL;
    }

    promise<ErrorCode> p;
    config->updateConfiguration(
        std::string(config_file_path), [&p](ErrorCode error) { p.set_value(error); });

    if (ErrorCode::SUCCESS == p.get_future().get()) {
        return V2X_STATUS_SUCCESS;
    }

    LOGE("Failed to update configuration file\n");
    return V2X_STATUS_FAIL;
}

v2x_status_enum_type v2x_retrieve_configuration(const char *config_file_path) {
    auto config = get_cv2x_config_handle();
    if (not config) {
        LOGE("Failed to acquire Cv2xConfig\n");
        return V2X_STATUS_FAIL;
    }

    promise<ErrorCode> p;
    config->retrieveConfiguration(
        std::string(config_file_path), [&p](ErrorCode error) { p.set_value(error); });

    if (ErrorCode::SUCCESS == p.get_future().get()) {
        return V2X_STATUS_SUCCESS;
    }

    LOGE("Failed to retrieve configuration file\n");
    return V2X_STATUS_FAIL;
}

v2x_status_enum_type v2x_register_for_config_change_ind(cv2x_config_event_listener callback) {
    LOGD("%s", __FUNCTION__);

    if (not callback) {
        LOGE("%s:Error callbak\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    auto config = get_cv2x_config_handle();
    if (not config) {
        LOGE("Failed to acquire Cv2xConfig\n");
        return V2X_STATUS_FAIL;
    }
    gConfigListener = make_shared<ConfigListener>();
    if (not gConfigListener) {
        LOGE("%s:Error instantiating config listener\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }
    config->registerListener(gConfigListener);
    gConfigChangeCallback = callback;

    LOGE("%s:Succeeded to register for configuration change indication\n", __FUNCTION__);
    return V2X_STATUS_SUCCESS;
}

static void cv2x_config_file_changed_listener(const ConfigEventInfo &info) {

    LOGD("%s:CV2X config changed, source:%d, event:%d.\n", __FUNCTION__, info.source, info.event);

    v2x_config_event_info_t config;
    convert_enum(info.source, config.source);
    convert_enum(info.event, config.event);

    if (gConfigChangeCallback) {
        gConfigChangeCallback(config);
    }
}
