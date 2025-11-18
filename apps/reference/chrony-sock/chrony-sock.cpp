/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2022-2023,2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a reference application that is used to feed GNSS time data
 * obtained from the Location APIs to the Chrony NTP server via the SOCK
 * interface. The application also calls chronyc to update the RTC file
 * periodically.
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <glib.h>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/TimeManager.hpp>
#include <telux/platform/TimeListener.hpp>

#include "../../common/utils/SignalHandler.hpp"

using telux::platform::PlatformFactory;
using telux::platform::ITimeManager;
using telux::platform::ITimeListener;


#define SOCK_NAME "/var/run/chrony.sock"
#define SOCK_MAGIC 0x534f434b
#define RTC_TIMER_SEC (60 * 11)

// default offset threshold in seconds to set inital system time
#define INITIAL_OFFSET_TREHSHOLD (24 * 3600)

void chronylog(int level, const char *fmt, ...);

#define LOGI(fmt, args...) \
    chronylog(LOG_NOTICE, "[I][%s:%d] " fmt, __func__, __LINE__, ## args)
#define LOGD(fmt, args...) \
    chronylog(LOG_DEBUG, "[D][%s:%d] " fmt, __func__, __LINE__, ## args)
#define LOGE(fmt, args...) \
    chronylog(LOG_ERR, "[E][%s:%d] " fmt, __func__, __LINE__, ## args)

using namespace telux::platform;
using namespace telux::common;

// Chrony SOCK sample
struct TimeSample {
    struct timeval tv;
    double offset;
    int pulse;
    int leap;
    int _pad;
    int magic;
};

static int chronyfd;
static bool gExit = false;
std::atomic<bool> gCv2xUtcValid{false};
std::atomic<bool> gFirstSample{true};

bool enableDebug = false;
bool enableSyslog = false;
bool enableWriteRtc = false;
bool enableSlssUtc = false;
uint64_t gOffsetThreshold = INITIAL_OFFSET_TREHSHOLD;

// Used to get the Telux async result
std::mutex mtx;
std::condition_variable cv;

static void setInitialTime(uint64_t utc);
static void sendUtcToChronyd(uint64_t utc);

int system_call(const char *command)
{
    FILE *stream = NULL;
    int result = -1;
    stream = popen(command, "w");
    if (stream == NULL) {
        LOGE("system call failed popen failed\n");
    } else {
        result = pclose(stream);
        if (WIFEXITED(result)) {
            result = WEXITSTATUS(result);
        }
        LOGD("popen closed with %d status", result);
    }
    return result;
}

void chronylog(int level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    if (level != LOG_DEBUG || enableDebug) {
        if (!enableSyslog) {
            vprintf(fmt, args);
        } else {
            vsyslog(level, fmt, args);
        }
    }
    va_end(args);
}

void printUsage(char *app_name) {
    printf("Usage: %s -d -s -r -a -o\n", app_name);
    printf("\t-d: Enable debug logs\n");
    printf("\t-s: Log to syslog instead of stdout\n");
    printf("\t-r: Enable updating the rtc file\n");
    printf("\t-a: Enable listening utc from cv2x\n");
    printf("\t-o <threshold>: Set system time to the first UTC sample");
    printf(" if the offset exceeds the threshold (unit in seconds)\n");
}

static void writeRtcFile(int sig, siginfo_t *si, void *uc) {
    int rc;

    LOGI("Updating rtc file using: chronyc writertc\n");
    rc = system_call("chronyc writertc");
    if (rc) {
        LOGE("Error sending the writertc command\n");
    }
}

int installRtcTimer() {
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its = {0};
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = writeRtcFile;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        LOGE("Error setting rtc signal\n");
        goto error;
    }

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerid;

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        LOGE("Error creating the rtc timer\n");
        goto error;
    }

    its.it_value.tv_sec = RTC_TIMER_SEC;
    its.it_interval.tv_sec = its.it_value.tv_sec;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        LOGE("Error starting the rtc timer\n");
        goto error;
    }

    LOGI("Set timer to update rtc file every 11 minutes\n");

    return 0;

error:
    return -EINVAL;
}

class MyTimeListener : public ITimeListener {
public:
    void onGnssUtcTimeUpdate(const uint64_t utc) {
        // ignore invalid utc
        if (utc == 0) {
            return;
        }

        // CV2X UTC has higher priority if it's available and user has enabled
        if (enableSlssUtc and gCv2xUtcValid) {
            LOGD("GNSS report ignored with UTC = %" PRIu64 " due to CV2X UTC is valid\n", utc);
            return;
        }

        if (gFirstSample) {
            gFirstSample = false;
            setInitialTime(utc);
        }

        if (utc % 1000 == 0) {
           sendUtcToChronyd(utc);
           LOGD("GNSS report with UTC = %" PRIu64 "\n", utc);
        } else {
           LOGD("GNSS report ignored with UTC = %" PRIu64 "\n", utc);
        }
    }

    void onCv2xUtcTimeUpdate(const uint64_t utc) {
        bool utcValid = false;
        if (0 != utc) {
            utcValid = true;
            if (gFirstSample) {
                gFirstSample = false;
                setInitialTime(utc);
            }
            sendUtcToChronyd(utc);
            LOGD("CV2X report with UTC = %" PRIu64 "\n", utc);
        }
        if (gCv2xUtcValid != utcValid) {
            LOGI("CV2X UTC valid:%d\n", utcValid);
            gCv2xUtcValid = utcValid;
        }
    }
};

// set sys time according to the first sample if the time diff exceeds the threshold,
// otherwise chronyd might not sync with the time due to huge diff
static void setInitialTime(uint64_t utc) {
    struct timeval curTime, newTime;
    newTime.tv_sec = (time_t)(utc / 1000);
    newTime.tv_usec = (suseconds_t)((utc % 1000) * 1000);
    gettimeofday(&curTime, NULL);
    if (newTime.tv_sec > curTime.tv_sec + (time_t)gOffsetThreshold) {
        if (settimeofday(&newTime, NULL) != 0) {
            LOGE("Failed to set sys time, errno:%d\n", errno);
        }
    }
    LOGI("Got first UTC report:%ld\n", utc);
}

static void sendUtcToChronyd(uint64_t utc) {
    struct TimeSample sample = { 0 };
    struct timeval gps_time, offset_time;

    sample.magic = SOCK_MAGIC;
    gettimeofday(&sample.tv, NULL);
    gps_time.tv_sec = (time_t)(utc / 1000);
    gps_time.tv_usec = (suseconds_t)((utc % 1000) * 1000);
    timersub(&gps_time, &sample.tv, &offset_time);
    sample.offset = (double)offset_time.tv_sec +
                    ((double)offset_time.tv_usec / 1000000);

    ssize_t bytesSent = send(chronyfd, &sample, sizeof(sample), 0);
    // Checking if the socket was closed
    if (-1 == bytesSent) {
        LOGE("Failed to send sample to chrony, error = %d\n", errno);
        exit(errno);
    } else if (sizeof(sample) != bytesSent) {
        LOGE("Failed to send sample to chrony, bytesSent = %d\n",
             bytesSent);
        exit(-EIO);
    }
}

int setupSocket(int *fd) {
    struct sockaddr_un name;

    *fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (*fd < 0) {
        LOGE("Failed to create chrony socket ret=%d\n", errno);
        return errno;
    }

    name.sun_family = AF_UNIX;
    g_strlcpy(name.sun_path, SOCK_NAME, sizeof(name.sun_path));

    if (connect(*fd, (struct sockaddr *)&name, sizeof(name))) {
        LOGE("Failed to connect chrony socket ret=%d\n", errno);
        return errno;
    }

    LOGI("Connected to the chronyd socket\n");

    return 0;
}

void parseArguments(int& argc, char **argv) {
    int opt;

    while ((opt = getopt(argc, argv, "adsrho:")) != -1) {
        switch (opt) {
        case 'd':
            enableDebug = true;
            break;
        case 's':
            enableSyslog = true;
            break;
        case 'r':
            enableWriteRtc = true;
            break;
        case 'a':
            enableSlssUtc = true;
            break;
        case 'o':
            if (optarg) {
                gOffsetThreshold = (uint64_t)atoll(optarg);
                LOGD("set offset threshold to %ld\n", gOffsetThreshold);
            }
            break;
        case 'h':
        default:
            printUsage(argv[0]);
            exit(-EINVAL);
        }
    }
}

int main(int argc, char *argv[]) {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGHUP);
    SignalHandlerCb cb = [](int sig) {
        std::unique_lock<std::mutex> lck(mtx);
        gExit = true;
        cv.notify_all();
    };

    SignalHandler::registerSignalHandler(sigset, cb);

    int ret = 0;

    // Exits if invalid arguments passed
    parseArguments(argc, argv);

    // Open chrony UNIX socket
    if ((ret = setupSocket(&chronyfd))) {
        return ret;
    }

    if (enableWriteRtc) {
        installRtcTimer();
    }

    // Initialize the TelSDK utc info manager
    std::shared_ptr<ITimeListener> myTimeListener
        = std::make_shared<MyTimeListener>();
    std::shared_ptr<ITimeManager> timeManager;

    auto &platformFactory = PlatformFactory::getInstance();

    bool statusUpdated = false;
    auto servicStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&statusUpdated, &servicStatus](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        statusUpdated = true;
        servicStatus = status;
        cv.notify_all();
    };

    timeManager = platformFactory.getTimeManager(statusCb);
    if (timeManager) {
        // wait for utc manager to be ready
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&statusUpdated] { return statusUpdated; });
    }

    if (gExit) {
        return 0;
    }

    if (servicStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGI("Time manager is ready\n");
    } else {
        LOGE("Unable to initialize time manager\n");
        return -EINVAL;
    }

    TimeTypeMask mask;
    mask.set(SupportedTimeType::GNSS_UTC_TIME);
    if (enableSlssUtc) {
        mask.set(SupportedTimeType::CV2X_UTC_TIME);
    }
    auto status = timeManager->registerListener(myTimeListener, mask);
    if (status != Status::SUCCESS) {
        LOGE("Failed to register utc listener\n");
        return -EINVAL;
    }

    LOGI("Started providing fixes to chronyd\n");

    {
        std::unique_lock<std::mutex> lck(mtx);
        while (!gExit) {
            cv.wait(lck);
        }
    }

    timeManager->deregisterListener(myTimeListener, mask);

    return 0;
}
