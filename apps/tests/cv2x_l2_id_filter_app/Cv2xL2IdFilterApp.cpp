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
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file: Cv2xL2IdFilter.cpp
 *
 * @brief: Application that set/remove cv2x remote vehicle L2 Id filter list .
 */

#include <string.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <future>
#include <string>
#include <mutex>
#include <memory>
#include "../../common/utils/Utils.hpp"
#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>

using std::cout;
using std::endl;
using std::hex;
using std::dec;
using std::promise;

using telux::common::ErrorCode;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::L2FilterInfo;

#define CV2X_MAX_PPPP (8)
#ifndef MAX_FILTER_IDS_LIST_LEN
#define MAX_FILTER_IDS_LIST_LEN (50)
#endif

std::vector<L2FilterInfo> filterList;
std::vector<uint32_t> removeL2IdList;

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << "\n"
         << "-s<set filter>  rv_l2_id(HEX),duration(in second),pppp(0-7)\n"
         << "-r<remove filter> rv_l2_id(HEX)\n" << endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int rc = -1;
    int c;
    char *savePtr;
    char *splitStr;
    L2FilterInfo filterItem;
    uint32_t removeL2Id;

    while ((c = getopt(argc, argv, "?:s:r:")) != -1) {
        switch (c) {
        case 's':
            if (optarg) {
                filterItem = {.srcL2Id = 0, .durationMs = 0, .pppp = 0};
                splitStr = strtok_r(optarg, ",", &savePtr);
                if (splitStr != NULL) {
                    filterItem.srcL2Id = strtoul(splitStr, NULL, 16);
                }
                if (filterItem.srcL2Id == 0 ) {
                    cout << "skip due to unexpected srcL2Id input" << endl;
                    break;
                }

                splitStr = strtok_r(NULL, ",", &savePtr);
                if (splitStr != NULL) {
                    filterItem.durationMs = 1000*strtoul(splitStr, NULL, 10);
                    if (filterItem.durationMs == 0 ) {
                        cout << "skip due to unexpected duration input" << endl;
                        break;
                    }
                } else {
                    cout << "unexpected parameters format, skip" << endl;
                    break;
                }

                splitStr = strtok_r(NULL, ",", &savePtr);
                if (splitStr != NULL) {
                    filterItem.pppp = strtoul(splitStr, NULL, 10);
                    if (filterItem.pppp >= CV2X_MAX_PPPP) {
                        cout << "skip due to unexpected pppp " << filterItem.pppp << endl;
                        break;
                    }
                }

                if (filterList.size() < MAX_FILTER_IDS_LIST_LEN) {
                    filterList.emplace_back(filterItem);
                    cout << "set filter for " << hex << filterItem.srcL2Id << ", duration "
                        << dec << filterItem.durationMs/1000 << " seconds, " << " pppp "
                        << +filterItem.pppp << endl;
                    rc = 0;
                }
            }
            break;
        case 'r':
            if (optarg) {
                removeL2Id = strtoul(optarg, NULL, 16);
                if (removeL2IdList.size() < MAX_FILTER_IDS_LIST_LEN) {
                    removeL2IdList.emplace_back(removeL2Id);
                    rc = 0;
                    cout << "remove filter for " << hex << removeL2Id << endl;
                }
            }
            break;
        case '?':
        default:
            rc = -1;
            break;
        }
    }

    if (rc == -1) {
        printUsage(argv[0]);
    }
    return rc;
}

int main(int argc, char *argv[]) {
    ErrorCode ret = telux::common::ErrorCode::GENERIC_FAILURE;

    if (parseOpts(argc, argv) < 0) {
        return EXIT_FAILURE;
    }
    std::vector<std::string> groups{"system", "diag", "radio", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(groups);
    if (rc == -1){
        cout << "Adding supplementary group failed!" << std::endl;
    }

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
    if (not cv2xRadioMgr) {
        cout << "Error: get Cv2x RadioManager failed" << endl;
        return EXIT_FAILURE;
    }

    // Wait for radio manager to complete initialization
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioManagerStatus) {
            cout << "Cv2x Radio Manager initialization failed!" << endl;
            return EXIT_FAILURE;
        }
    }

    if (filterList.size() > 0) {
        promise<ErrorCode> p;

        if (telux::common::Status::SUCCESS == cv2xRadioMgr->setL2Filters(filterList,
            [&p](ErrorCode error) {p.set_value(error);})) {
            ret = p.get_future().get();
        }
        if (ErrorCode::SUCCESS != ret) {
            cout << "set filter error " << static_cast<int>(ret) << endl;
        }
    }

    if (removeL2IdList.size() > 0) {
        promise<ErrorCode> p;
        ret = telux::common::ErrorCode::GENERIC_FAILURE;
        if (telux::common::Status::SUCCESS == cv2xRadioMgr->removeL2Filters(removeL2IdList,
            [&p](ErrorCode error) {p.set_value(error);})) {
            ret = p.get_future().get();
        }
        if (ErrorCode::SUCCESS != ret) {
            cout << "remove filter error " << static_cast<int>(ret) << endl;
        }
    }

    return EXIT_SUCCESS;
}
