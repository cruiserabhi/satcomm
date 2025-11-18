/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIO_JSON_HELPER_HPP
#define AUDIO_JSON_HELPER_HPP

#include <algorithm>
#include <telux/common/CommonDefines.hpp>
#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"

#define AUDIO_MANAGER_API_JSON "api/audio/IAudioManager.json"

/*
 * This struct is used to get values from json for mentioned API.
 */
struct ApiResponse{
    int cbDelay;
    telux::common::ErrorCode error;
    telux::common::Status status;
};

class AudioJsonHelper {
    public:
        AudioJsonHelper();
        ~AudioJsonHelper();

        telux::common::Status loadJson();
        telux::common::ServiceStatus initServiceStatus();
        int getSubsystemReadyDelay();
        void getApiResponse(ApiResponse *apiResponse, std::string className, std::string apiname);
        telux::common::Status getApiRequestStatus(std::string apiname);

    private:
        Json::Value rootObj_;
        telux::common::ServiceStatus serviceStatus_ =
            telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
        std::mutex mutex_;
};

#endif // AUDIO_JSON_HELPER_HPP
