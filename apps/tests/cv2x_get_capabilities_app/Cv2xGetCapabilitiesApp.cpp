/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file: Cv2xGetCapabilitiesApp.cpp
 *
 * @brief: Simple application that queries C-V2X status and
 *         capabilities prints to stdout.
 */

#include <iostream>
#include <future>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <random>
#include <limits>
#include <iomanip>
#include <cstdint>
#include <map>
#include <vector>

#include "../../common/utils/Utils.hpp"

#include "../../common/utils/SignalHandler.hpp"
#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xRadioListener.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>

using std::cout;
using std::cin;
using std::cerr;
using std::atomic;
using std::endl;
using std::promise;
using std::string;
using std::shared_ptr;
using std::stringstream;
using std::hex;
using std::map;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::common::ServiceStatus;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusEx;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::Cv2xCauseType;
using telux::cv2x::TrafficCategory;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::Cv2xRadioCapabilities;
using telux::cv2x::RadioConcurrencyMode;

static Cv2xStatusEx gCv2xStatus;
static promise<ErrorCode> gCallbackPromise;
static promise<ErrorCode> gCapabilityPromise;

static map<Cv2xStatusType, string> gCv2xStatusToString = {
    { Cv2xStatusType::INACTIVE, "INACTIVE" },
    { Cv2xStatusType::ACTIVE, "ACTIVE" },
    { Cv2xStatusType::SUSPENDED, "SUSPENDED" },
    { Cv2xStatusType::UNKNOWN, "UNKNOWN" },
};

static map<Cv2xCauseType, string> gCv2xCauseToString = {
    {Cv2xCauseType::TIMING, "TIMING"},
    {Cv2xCauseType::CONFIG, "CONFIG"},
    {Cv2xCauseType::UE_MODE, "UE_MODE"},
    {Cv2xCauseType::GEOPOLYGON, "GEOPOLYGON"},
    {Cv2xCauseType::THERMAL, "THERMAL"},
    {Cv2xCauseType::THERMAL_ECALL, "THERMAL_ECALL"},
    {Cv2xCauseType::GEOPOLYGON_SWITCH, "GEOPOLYGON_SWITCH"},
    {Cv2xCauseType::SENSING, "SENSING"},
    {Cv2xCauseType::LPM, "LPM"},
    {Cv2xCauseType::DISABLED, "DISABLED"},
    {Cv2xCauseType::NO_GNSS, "NO_GNSS"},
    {Cv2xCauseType::INVALID_LICENSE, "INVALID_LICENSE"},
    {Cv2xCauseType::NOT_READY, "NOT_READY"},
    {Cv2xCauseType::NTN, "NTN"},
    {Cv2xCauseType::NO_DATA_CALL, "NO_DATA_CALL"},
    {Cv2xCauseType::UNKNOWN, "UNKNOWN"}
};

static string statusToString(const Cv2xStatus &status, bool printUnknown = true) {
    stringstream ss;

    if (printUnknown or status.rxStatus != Cv2xStatusType::UNKNOWN) {
        ss << "\tRx Status= " << static_cast<int>(status.rxStatus);
        ss << " : " << gCv2xStatusToString[status.rxStatus] << "\n";
        if (status.rxStatus != Cv2xStatusType::ACTIVE) {
            ss << "\tRx Cause= " << static_cast<int>(status.rxCause);
            ss << " : " << gCv2xCauseToString[status.rxCause] << "\n";
        }
    }

    if (printUnknown or status.txStatus != Cv2xStatusType::UNKNOWN) {
        ss << "\tTx Status= " << static_cast<int>(status.txStatus);
        ss << " : " << gCv2xStatusToString[status.txStatus] << "\n";
        if (status.txStatus != Cv2xStatusType::ACTIVE) {
            ss << "\tTx Cause= " << static_cast<int>(status.txCause);
            ss << " : " << gCv2xCauseToString[status.txCause] << "\n";
        }
    }

    if (status.cbrValueValid) {
        ss << "\tCBR= " << static_cast<unsigned int>(status.cbrValue) << "\n";
    }
    return ss.str();
}

static string statusToString(const Cv2xStatusEx &status) {

    stringstream ss;
    ss << "Status:\n";
    ss << "Overall:\n" << statusToString(status.status);

    for (auto i = 0u; i < status.poolStatus.size(); ++i) {
        ss << "Pool ID " << static_cast<unsigned int>(status.poolStatus[i].poolId) << ":\n";
        ss << statusToString(status.poolStatus[i].status, false);
    }

    if (status.timeUncertaintyValid) {
        ss << "\tTime uncertainty= "
            << std::fixed << std::setprecision(10) << status.timeUncertainty << "\n";
    }
    return ss.str();
}

static string capabilitiesToString(const Cv2xRadioCapabilities &capabilities) {
    stringstream ss;

    ss << "\t" << "V2X Capabilities:" << "\n";
    ss << "\t" << "linkIpMtuBytes: " << static_cast<int>(capabilities.linkIpMtuBytes) << "\n";
    ss << "\t" << "linkNonMtuBytes: " << static_cast<int>(capabilities.linkNonIpMtuBytes) << "\n";
    ss << "\t" << "maxSupportedConcurrency: "
        << ((capabilities.maxSupportedConcurrency == RadioConcurrencyMode::WWAN_CONCURRENT) ?
        "WWAN_CONCURRENT" : "WWAN_NONCONCURRENT") << "\n";
    ss << "\t" << "nonIpTxPayloadOffsetBytes: " <<
        static_cast<int>(capabilities.nonIpTxPayloadOffsetBytes) << "\n";
    ss << "\t" << "nonIpRxPayloadOffsetBytes: "
        << static_cast<int>(capabilities.nonIpRxPayloadOffsetBytes) << "\n";
    ss << "\t" << "Periodicities Supported - size: "
        << capabilities.periodicities.size() << "\n";
    for (auto i = 0u; i < capabilities.periodicities.size(); ++i) {
        ss << "\t" << static_cast<int64_t>(capabilities.periodicities[i]);
    }
    ss << "\n";
    ss << "\t" << "maxNumAutoRetransmissions: "
        << static_cast<int>(capabilities.maxNumAutoRetransmissions) << "\n";
    ss << "\t" << "layer2MacAddressSize: "
        << static_cast<int>(capabilities.layer2MacAddressSize) << "\n";
    ss << "\t" << "prioritiesSupported: " << capabilities.prioritiesSupported << "\n";
    ss << "\t" << "maxNumSpsFlows: " << static_cast<int>(capabilities.maxNumSpsFlows) << "\n";
    ss << "\t" << "maxNumNonSpsFlows: "
        << static_cast<int>(capabilities.maxNumNonSpsFlows) << "\n";
    ss << "\t" << "maxTxPower: " << static_cast<int>(capabilities.maxTxPower) << "\n";
    ss << "\t" << "minTxPower: " << static_cast<int>(capabilities.minTxPower) << "\n";
    ss << "\t" << "TX pool ids supported - size: "
        << capabilities.txPoolIdsSupported.size() << "\n";
    for (auto i = 0u; i < capabilities.txPoolIdsSupported.size(); ++i) {
        ss << "\t" << "Pool ID: " << static_cast<int>(capabilities.txPoolIdsSupported[i].poolId)
            << " minFreq " << static_cast<int>(capabilities.txPoolIdsSupported[i].minFreq)
            << " maxFreq " << static_cast<int>(capabilities.txPoolIdsSupported[i].maxFreq);
    }
    return ss.str();
}

// Callback function for Cv2xRadioManager->requestCv2xStatus()
static void cv2xStatusCallback(Cv2xStatusEx status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gCv2xStatus = status;
        cout << statusToString(status);
    }
    gCallbackPromise.set_value(error);
}

static void requestCapabilitiesCallback(const Cv2xRadioCapabilities & capabilities,
                                        ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        cout << "Request capabilities success" << endl;
        cout << capabilitiesToString(capabilities) << endl;
    } else {
        cout << "Request capabilities failure" << endl;
    }
    gCapabilityPromise.set_value(error);
}

int main(int argc, char *argv[]) {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGHUP);
    SignalHandlerCb cb = [](int sig) {
        // We can call exit() here if no cleanups needed,
        // or maybe just set a flag, and let the main thread to decide
        // when to exit.
        exit(sig);
    };
    SignalHandler::registerSignalHandler(sigset, cb);

    cout << "Running C-V2X testing app" << endl;

    std::vector<std::string> groups{"system", "diag", "radio", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(groups);
    if (rc == -1){
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
    auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!cv2xRadioManager) {
        cout << "Error: failed to get Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioManagerStatus) {
            cerr << "C-V2X Radio Manager initialization failed, exiting" << endl;
            return EXIT_FAILURE;
        }
    }

    // Get C-V2X status
    if (Status::SUCCESS != cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback)) {
        cout << "Error : request for C-V2X status failed." << endl;
        return EXIT_FAILURE;
    }
    if (ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cout << "Error : failed to retrieve C-V2X status." << endl;
        return EXIT_FAILURE;
    }

    // Get handle to Cv2xRadio
    bool cv2x_radio_status_updated = false;
    telux::common::ServiceStatus cv2xRadioStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

    auto radioCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2x_radio_status_updated = true;
        cv2xRadioStatus = status;
        cv.notify_all();
    };

    std::shared_ptr<ICv2xRadio> cv2xRadio;
    cv2xRadio = cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE, radioCb);

    if (not cv2xRadio) {
        cerr << "C-V2X Radio creation failed." << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lc(mtx);
        cv.wait(lc, [&cv2x_radio_status_updated]() { return cv2x_radio_status_updated; });

        if (cv2xRadioStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            cerr << "C-V2X Radio initialization failed." << endl;
            return EXIT_FAILURE;
        } else {
            cout << "C-V2X Radio is ready" << endl;
        }
    }

    // Get C-V2X capabilities
    if (Status::SUCCESS != cv2xRadio->requestCapabilities(requestCapabilitiesCallback)) {
        cout << "Error : request for C-V2X capabilities failed." << endl;
        return EXIT_FAILURE;
    }
    if (ErrorCode::SUCCESS != gCapabilityPromise.get_future().get()) {
        cout << "Error : failed to retrieve C-V2X capabilities." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

