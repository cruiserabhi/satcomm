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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file: Cv2xTxServiceStatusApp.cpp
 *
 * @brief: Application that demonstrates Cv2x Tx while monitoring
 *         cv2x service status. It attempts recreation of flows
 *         and packet transmission after cv2x status transitions from
 *         inactive to active.
 */

#include <assert.h>
#include <ifaddrs.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <map>

#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xRadioListener.hpp>

#include "../../../common/utils/Utils.hpp"

using std::array;
using std::cerr;
using std::cout;
using std::endl;
using std::promise;
using std::shared_ptr;
using std::static_pointer_cast;
using std::make_shared;
using std::mutex;
using std::lock_guard;
using std::string;
using std::map;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::common::ServiceStatus;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusEx;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::ICv2xTxFlow;
using telux::cv2x::Periodicity;
using telux::cv2x::Priority;
using telux::cv2x::TrafficCategory;
using telux::cv2x::TrafficIpType;
using telux::cv2x::SpsFlowInfo;
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xRadioListener;
using telux::cv2x::ICv2xRadio;

static constexpr uint32_t SPS_SERVICE_ID = 1u;
static constexpr uint16_t SPS_SRC_PORT_NUM = 2500u;
static constexpr uint32_t G_BUF_LEN = 128;
static constexpr uint16_t NUM_TEST_ITERATIONS = 128;
static constexpr int      PRIORITY = 3;

static constexpr char TEST_VERNO_MAGIC = 'Q';
static constexpr char UEID = 1;

static Cv2xStatusEx gCv2xStatus;
static promise<ErrorCode> gCallbackPromise;
static shared_ptr<ICv2xTxFlow> gSpsFlow;
static array<char, G_BUF_LEN> gBuf;

static map<ServiceStatus, string> serviceStatusToString = {
    {ServiceStatus::SERVICE_AVAILABLE, "AVAILABLE"},
    {ServiceStatus::SERVICE_UNAVAILABLE, "UNAVAILABLE"},
};

static map<Cv2xStatusType, string> cv2xStatusToString = {
    {Cv2xStatusType::INACTIVE, "INACTIVE"},
    {Cv2xStatusType::ACTIVE, "ACTIVE"},
    {Cv2xStatusType::SUSPENDED, "SUSPENDED"},
    {Cv2xStatusType::UNKNOWN, "UNKNOWN"},
};

class Cv2xListener : public ICv2xListener {
public:
    Cv2xListener(Cv2xStatusEx status) : status_(status) { }

    void onServiceStatusChange(ServiceStatus status) override {
        cout << "Service status changed to: " << serviceStatusToString[status] << endl;
    }

    void onStatusChanged(Cv2xStatusEx status) override {
        cout << "Cv2x TX status changed to: " <<  cv2xStatusToString[status.status.txStatus] << endl;
        lock_guard<mutex> lock(mutex_);
        status_ = status;
    }

    Cv2xStatusEx getStatus() {
        lock_guard<mutex> lock(mutex_);
        return status_;
    }

protected:
    Cv2xStatusEx status_;
    mutex mutex_;
};

// Resets the global callback promise
static inline void resetCallbackPromise(void) {
    gCallbackPromise = promise<ErrorCode>();
}

// Callback function for ICv2xRadioManager->requestCv2xStatus()
static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gCv2xStatus.status = status;
    }
    gCallbackPromise.set_value(error);
}

// Callback function for ICv2xRadio->createTxSpsFlow()
static void createSpsFlowCallback(shared_ptr<ICv2xTxFlow> txSpsFlow,
                                  shared_ptr<ICv2xTxFlow> unusedFlow,
                                  ErrorCode spsError,
                                  ErrorCode unusedError) {
    if (ErrorCode::SUCCESS == spsError) {
        gSpsFlow = txSpsFlow;
    }
    gCallbackPromise.set_value(spsError);
}

// Returns current timestamp
static uint64_t getCurrentTimestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ull + (uint64_t)tv.tv_usec;
}

// Fills buffer with dummy data
static void fillBuffer(void) {

    static uint16_t seq_num = 0u;
    auto timestamp = getCurrentTimestamp();

    // Very first payload is test Magic number, this is  where V2X Family ID would normally be.
    gBuf[0] = TEST_VERNO_MAGIC;

    // Next byte is the UE equipment ID
    gBuf[1] = UEID;

    // Sequence number
    auto dataPtr = gBuf.data() + 2;
    uint16_t tmp = htons(seq_num++);
    memcpy(dataPtr, reinterpret_cast<char *>(&tmp), sizeof(uint16_t));
    dataPtr += sizeof(uint16_t);

    // Timestamp

    dataPtr += snprintf(dataPtr, G_BUF_LEN - (2 + sizeof(uint16_t)),
            "<%llu> ", static_cast<long long unsigned>(timestamp));

    // Dummy payload
    constexpr int NUM_LETTERS = 26;
    auto i = 2 + sizeof(uint16_t) - sizeof(long long unsigned);
    for (; i < G_BUF_LEN; ++i) {
        gBuf[i] = 'a' + ((seq_num + i) % NUM_LETTERS);
    }
}

// Function for transmitting data
static void sampleSpsTx(void) {

    static uint32_t txCount = 0u;
    int sock = gSpsFlow->getSock();

    cout << "sampleSpsTx(" << sock << ")" << endl;

    struct msghdr message = {0};
    struct iovec iov[1] = {0};
    struct cmsghdr * cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];

    // Send data using sendmsg to provide IPV6_TCLASS per packet
    iov[0].iov_base = gBuf.data();
    iov[0].iov_len = G_BUF_LEN;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    // Fill ancillary data
    int priority = PRIORITY;
    cmsghp = CMSG_FIRSTHDR(&message);
    cmsghp->cmsg_level = IPPROTO_IPV6;
    cmsghp->cmsg_type = IPV6_TCLASS;
    cmsghp->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsghp), &priority, sizeof(int));

    // Send data
    cout << "sending msg" << endl;
    auto bytes_sent = sendmsg(sock, &message, 0);
    cout << "bytes_sent=" << bytes_sent << endl;

    // Check bytes sent
    if (bytes_sent < 0) {
        cerr << "Error sending message: " << bytes_sent << endl;
        bytes_sent = -1;
    } else {
        if (bytes_sent == G_BUF_LEN) {
           ++txCount;
        } else {
            cerr << "Error : " << bytes_sent << " bytes sent." << endl;
        }
    }

    cout << "TX count: " << txCount << endl;
}

// Callback for ICv2xRadio->closeTxFlow()
static void closeFlowCallback(shared_ptr<ICv2xTxFlow> flow, ErrorCode error) {
    gCallbackPromise.set_value(error);
}

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X TX app" << endl;
    std::vector<std::string> groups{"system", "diag", "radio", "logd"};
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

    // Get C-V2X status and make sure Tx is enabled
    assert(Status::SUCCESS == cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    if (Cv2xStatusType::ACTIVE == gCv2xStatus.status.txStatus) {
        cout << "C-V2X TX status is active" << endl;
    }
    else {
        cerr << "C-V2X TX is inactive" << endl;
        return EXIT_FAILURE;
    }

    shared_ptr<Cv2xListener> listener = make_shared<Cv2xListener>(gCv2xStatus);
    cv2xRadioManager->registerListener(static_pointer_cast<ICv2xListener>(listener));

     // Create new Tx SPS flow
    SpsFlowInfo spsInfo;
    spsInfo.priority = Priority::PRIORITY_2;
    spsInfo.periodicity = Periodicity::PERIODICITY_100MS;
    spsInfo.nbytesReserved = G_BUF_LEN;
    spsInfo.autoRetransEnabledValid = true;
    spsInfo.autoRetransEnabled = true;

    bool flowUp = false;

    std::shared_ptr<ICv2xRadio> cv2xRadio;

    // Send message in a loop
    uint16_t i = 0;
    while (i < NUM_TEST_ITERATIONS) {
        gCv2xStatus = listener->getStatus();
        if (gCv2xStatus.status.txStatus == Cv2xStatusType::INACTIVE) {
            flowUp = false;
        } else if (gCv2xStatus.status.txStatus == Cv2xStatusType::ACTIVE) {
            if (not flowUp) {   // Get handle to Cv2xRadio
                // Get handle to Cv2xRadio
                bool cv2x_radio_status_updated = false;
                telux::common::ServiceStatus cv2xRadioStatus =
                    telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

                auto cb = [&](ServiceStatus status) {
                    std::lock_guard<std::mutex> lock(mtx);
                    cv2x_radio_status_updated = true;
                    cv2xRadioStatus = status;
                    cv.notify_all();
                };

                cv2xRadio = cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE, cb);

                if (not cv2xRadio) {
                    cerr << "C-V2X Radio creation failed." << endl;
                    return EXIT_FAILURE;
                }

                {
                    // Wait for cv2x radio to complete initialization
                    std::unique_lock<std::mutex> lc(mtx);
                    cv.wait(lc,
                            [&cv2x_radio_status_updated]() {
                                return cv2x_radio_status_updated;
                            });

                    if (cv2xRadioStatus != ServiceStatus::SERVICE_AVAILABLE) {
                        cerr << "C-V2X Radio initialization failed." << endl;
                        return EXIT_FAILURE;
                    } else {
                        cout << "C-V2X Radio is ready" << endl;
                    }
                }

                resetCallbackPromise();
                assert(Status::SUCCESS == cv2xRadio->createTxSpsFlow(TrafficIpType::TRAFFIC_NON_IP,
                                                                     SPS_SERVICE_ID,
                                                                     spsInfo,
                                                                     SPS_SRC_PORT_NUM,
                                                                     false,
                                                                     0,
                                                                     createSpsFlowCallback));
                assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());
                flowUp = true;
            }
            fillBuffer();
            sampleSpsTx();
            ++i;
        }
        usleep(100000u);

    }

    // Deregister SPS flow
    resetCallbackPromise();
    assert(Status::SUCCESS == cv2xRadio->closeTxFlow(gSpsFlow, closeFlowCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    cout << "Done." << endl;

    return EXIT_SUCCESS;
}
