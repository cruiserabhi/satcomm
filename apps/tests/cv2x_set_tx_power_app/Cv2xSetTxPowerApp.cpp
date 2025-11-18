/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xSetTxPowerApp.cpp
 *
 * @brief: Application that set global override of CV2X Tx-power .
 */

#include <iostream>
#include <future>
#include <string>
#include <mutex>
#include <memory>
#include "../../common/utils/Utils.hpp"
#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

using std::cout;
using std::cerr;
using std::endl;
using std::cin;
using std::promise;

using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xRadioManager;

//per 3GPP TS 36.311
#define CV2X_TX_POWER_MAX (31)
#define CV2X_TX_POWER_MIN (-40)

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X Set Tx-power app" << endl;
    int txPower = CV2X_TX_POWER_MAX;
    telux::common::Status ret;
    bool inputInRange = false;

    do {
        cout << "Enter desired global cv2x Tx peak power:";
        cin >> txPower;
        Utils::validateInput(txPower);
        if (txPower > CV2X_TX_POWER_MAX || txPower < CV2X_TX_POWER_MIN) {
            cout << txPower << " is out of range. ";
            cout << "Supported range " << CV2X_TX_POWER_MIN << " - " << CV2X_TX_POWER_MAX << endl;
        } else {
            inputInRange = true;
        }
    } while (not inputInRange);

    cout << "Desired tx power " << txPower << endl;
    std::vector<std::string> groups{"system", "diag", "radio", "logd", "dlt"};
    if (-1 == Utils::setSupplementaryGroups(groups)) {
        cout << "Adding supplementary group failed!" << std::endl;
    }

    // Get handle to Cv2xRadioManager
    bool cv2xRadioManagerStatusUpdated = false;
    telux::common::ServiceStatus cv2xRadioManagerStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xRadioManagerStatusUpdated = true;
        cv2xRadioManagerStatus = status;
        cv.notify_all();
    };

    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!cv2xRadioMgr) {
        cout << "Error: failed to get Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
        cv2xRadioManagerStatus) {
        cerr << "C-V2X Radio Manager initialization failed, exiting" << endl;
        return EXIT_FAILURE;
    }

    promise<ErrorCode> p;
    ret = cv2xRadioMgr->setPeakTxPower(static_cast<int8_t>(txPower), [&p](ErrorCode error) {
        if (ErrorCode::SUCCESS != error) {
            cout << "Set Cv2x Tx Power fail, error code " << static_cast<int>(error) << endl;
        }
        p.set_value(error);
    });
    if (telux::common::Status::SUCCESS == ret && ErrorCode::SUCCESS == p.get_future().get()) {
        cout << "success set_peak_tx_power " << txPower << endl;
    }

    return EXIT_SUCCESS;
}
