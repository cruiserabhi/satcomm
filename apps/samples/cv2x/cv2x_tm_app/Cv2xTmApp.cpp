/*
 *  Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * @file: Cv2xTmApp.cpp
 *
 * @brief: sample app that connects to throttle manager.
 */

#include <iostream>
#include <string>
#include <future>
#include <unistd.h>

#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xThrottleManager.hpp>

#define LOOP_COUNT  10

using namespace telux::cv2x;
static std::promise<telux::common::ErrorCode> gCallbackPromise;

class Cv2xTmListener : public ICv2xThrottleManagerListener {
    void onFilterRateAdjustment(int rate) {
        std::cout << "Updated rate: " << rate << std::endl;
    }
    void onServiceStatusChange(telux::common::ServiceStatus status) {
        if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "SERVICE IS AVAILABLE" << std::endl;
        } else if (status == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "SERVICE IS UNAVAILABLE" << std::endl;
        } else {
            std::cout << "unknown SERVICE STATUS" << std::endl;
        }
    }

    void onSanityStateUpdate(bool state) {
        if (state) {
            std::cout << "Good State" << std::endl;
        } else {
            std::cout << "Bad State" << std::endl;
        }
    }
};

// Callback function for Cv2xThrottleManager->setVerificationLoad()
static void cv2xsetVerificationLoadCallback(telux::common::ErrorCode error) {
    std::cout << "error=" << static_cast<int>(error) << std::endl;
    gCallbackPromise.set_value(error);
}

int main(int argc, char *argv[]) {
    int loop = 0, load = 2000;
    auto listener = std::make_shared<Cv2xTmListener>();
    bool cv2xTmStatusUpdated = false;
    telux::common::ServiceStatus cv2xTmStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;

    std::cout << "Running TM app" << std::endl;

    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xTmStatusUpdated = true;
        cv2xTmStatus = status;
        cv.notify_all();
    };
    // Get handle to Cv2xThrottleManager
    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xThrottleManager = cv2xFactory.getCv2xThrottleManager(statusCb);
    if (!cv2xThrottleManager) {
        std::cout << "Error: failed to get Cv2xThrottleManager." << std::endl;
        return EXIT_FAILURE;
    }
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&] { return cv2xTmStatusUpdated; });
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
        cv2xTmStatus) {
        std::cout << "Error: failed to initialize Cv2xThrottleManager." << std::endl;
        return EXIT_FAILURE;
    }

    // register listener
    if (cv2xThrottleManager->registerListener(listener) !=
            telux::common::Status::SUCCESS) {
        std::cout << "Failed to register listener" << std::endl;
        return EXIT_FAILURE;
    }

    //periodically set the verification load
    while(loop < LOOP_COUNT) {
        std::cout << "Setting verification load to: " << load << std::endl;
        cv2xThrottleManager->setVerificationLoad(load, cv2xsetVerificationLoadCallback);
        if (telux::common::ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            std::cout << "Error : failed to set verification load" << std::endl;
            return EXIT_FAILURE;
        } else {
            std::cout << "set verification load success" << std::endl;
        }
        gCallbackPromise = std::promise<telux::common::ErrorCode>();

        load -= 20;
        sleep(2);
        loop++;
    };
    return EXIT_SUCCESS;
}
