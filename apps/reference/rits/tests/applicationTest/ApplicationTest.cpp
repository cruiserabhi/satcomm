/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
  * @file: ApplicationTest.cpp
  *
  * @brief: Implements several functionalities of qApplication library.
  *
  */
extern "C" {
#include <sys/capability.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/time.h>
}

#include <thread>
#include <list>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <regex>
#include "SaeApplication.hpp"
#ifdef ETSI
#include "EtsiApplication.hpp"
#endif
#include "safetyapp_util.h"
#include "bsm_utils.h"
#include "../../../../common/utils/Utils.hpp"
#include "../../../../common/utils/SignalHandler.hpp"
#include <telux/common/Version.hpp>

using std::thread;
using std::string;
using std::list;
using std::ref;
using std::lock_guard;
using std::mutex;
using std::shared_ptr;
using std::make_shared;

#define IP_ADDR_RETRY_INTERVAL_MS (100) // interval for re-getting V2X IP address
#define IP_ADDR_RETRY_TIMES (2) // maximum retries for getting V2X IP address
#define SETUP_RETRY_INTERVAL_MS (500) // interval for re-setup CV2X radio
#define SETUP_RETRY_TIMES (10) // maximum retries for setup CV2X radio

// Global variables
shared_ptr<ApplicationBase> application = nullptr;
vector<thread> threads;
bool csv = false;
bool enableDiagLog = false;
int timerFd = -1;
string csvFileName;
sem_t cnt_sem;
auto rxsuccess = 0;
auto rxfail = 0;
std::mutex gTerminateMtx;
std::condition_variable gTerminateCv;
bool stopThread = false;
bool dump_raw = false;
bool print_rv = true;
std::condition_variable statusCv;
bool haltRx = false;
std::mutex cv2xStatusMtx;
bool simMode = false;

// stop threads due to error
void stopThreads() {
    std::unique_lock<std::mutex> lk(gTerminateMtx);
    if (not stopThread) {
        stopThread = true;
        if (timerFd >= 0) {
            close(timerFd);
        }

        if (application) {
            application->prepareForExit();
            if (application->qMon) {
                application->qMon->stop();
            }
        }
        gTerminateCv.notify_all();
    }
}

// catch specified signals and gracefully shut down program
void signalHandler(int signum) {
    fprintf(stderr, "Interrupt signal (%d) received.\n", signum);
    stopThreads();
}

// allow the main thread to wait on the threads to join
void joinThreads() {
    for (int i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }
}

/**
 * Initialize timer
 * @param[in] intervalMs timer interval value in miliseconds
 * @return timer's file descriptor if success or -1 on failure.
 */
int startTimerMs(uint32_t intervalMs) {
    int timerfd;
    struct itimerspec its = {0};

    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd < 0) {
        return -1;
    }

    /* Start the timer */
    its.it_value.tv_sec = intervalMs / 1000;
    its.it_value.tv_nsec = (intervalMs%1000) * 1000000;
    its.it_interval = its.it_value;

    if (timerfd_settime(timerfd, 0, &its, NULL) < 0) {
        close(timerfd);
        return -1;
    }

    return timerfd;

}


//Returns the value of enableL2filtering config
bool isL2SrcFilteringEnabled() {
    return application->configuration.enableL2Filtering;
}

//Function to trigger L2 src filtering
void rvL2SrcFiltering(shared_ptr<ApplicationBase> application) {
    std::thread ([application]() {
        while (!stopThread) {
            application->filterRate=application->cv2xTmListener->getFilterRate();
            if (application->filterRate)
            {
                if (application->configuration.appVerbosity > 3) {
                    printf("Filter rate is %d \n", application->filterRate);
                }
                application->setL2RvFilteringList(application->filterRate);
            }
            application->tmCommunication();
            std::this_thread::sleep_for(std::chrono::milliseconds(
                                                        application->configuration.filterInterval));
        }
    }).detach();
}


// do we want a thread for each command interval and a thread for
// evaluation interval. or for both?
void l2FloodingMitigation(shared_ptr<ApplicationBase> application) {
    // do we want the states to be tracked in application base or here?
    std::thread ([application]() {
        struct timeval currTime;
        gettimeofday(&currTime, NULL);
        time_t startTime = currTime.tv_sec;
        int timer_misses = 0;
        uint64_t exp = 0;
        ssize_t s;
        int ciTimerFd, tshiftTimerFd = 0;
        ciTimerFd = startTimerMs(application->configuration.commandInterval);
        if (ciTimerFd == -1) {
            cerr << "Failed to start command interval timer" << endl;
            return;
        }

        tshiftTimerFd = startTimerMs(application->configuration.tShiftInterval);
        if (tshiftTimerFd == -1) {
            cerr << "Failed to start t shift timer" << endl;
            return;
        }

        bool stateOn = false;
        int commandIntervalCtr = 0;
        std::vector<L2FilterInfo> rvListToFilter;
        //create timer for command interval and then when N command intervals happen
        // call evaluation function
        while (!stopThread) {
            // wait logic varies based on state
            // both of these should be equal to Command Interval
            if(!stateOn){
                if(application->configuration.floodDetectVerbosity > 3){
                    std::cout << "STATE 0: command interval counter is: " <<
                        commandIntervalCtr << "\n";
                }
                s = read(ciTimerFd, &exp, sizeof(exp));
                if (s == sizeof(uint64_t) && exp > 1) {
                    timer_misses += (exp-1);
                    if(application->configuration.driverVerbosity){
                        cout << "TX timer overruns: Total missed: " << timer_misses << endl;
                    }
                }
            }else{
                // now we are in active state, so we need to actively filter in each
                // command interval. but we only set the filter after T shift time
                // wait shift time * num of CI passed after last CI after state change
                if(application->configuration.floodDetectVerbosity > 3){
                    std::cout << "STATE 1: command interval counter is: " <<
                        commandIntervalCtr << "\n";
                    std::cout << "SHIFT VALUE: " <<
                        (application->configuration.tShiftInterval*commandIntervalCtr) << "\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                        application->configuration.tShiftInterval*commandIntervalCtr));
                // then call l2 filtering for T_ON_1 time
                if(application->configuration.floodDetectVerbosity > 3){
                    std::cout << "Setting l2 flood attack filters in application test\n";
                }
                application->radioReceives[0].setL2Filters(rvListToFilter);
                // then wait T_off_1 - T_shift * num of CI passed
                // wait shift time * num of CI passed after last CI after state change
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(application->configuration.commandInterval -
                    application->configuration.tShiftInterval*commandIntervalCtr));
            }
            commandIntervalCtr++;
            // if current interval is now equal to evaluation interval
            if(commandIntervalCtr ==
                    application->configuration.nCommandInterval_0 && !stateOn
                || commandIntervalCtr ==
                    application->configuration.nCommandInterval_1 && stateOn){

                // based on state, we will wait a set amount of time
                // the function will determine which state the app is currently in
                // then will filter out attacker l2 addresses for set time
                application->detectFloodAndMitigate(stateOn, &rvListToFilter);
                //reset the counter after we reach evaluation interval
                commandIntervalCtr = 0;
            }
        }
    }).detach();
}

int reSetupRadio(MessageType msgType) {
    if (!application) {
        return -1;
    }

    for (int retryTimes = 0; retryTimes < SETUP_RETRY_TIMES; ++retryTimes) {
        if (0 == application->setup(msgType, true)) {
            return 0;
        }
        if (application->configuration.driverVerbosity) {
            cout << "radio setup fail, retry later!" << endl;
        }
        std::unique_lock<std::mutex> lck(gTerminateMtx);
        if (gTerminateCv.wait_for(lck,
                std::chrono::milliseconds(SETUP_RETRY_INTERVAL_MS),
                []{return (stopThread == true);})) {
            break;
        }
    }

    cerr << "re-setup radio failed!" << endl;
    return -1;
}

/**
 * receiving thread function.
 *
 * @param[in] msgType type of the messsage we are processing.
 */
void receive(MessageType msgType, int index) {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    int tid = (int)std::stoul(ss.str());
    auto count = 0;
    FILE *fp;
    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    time_t startTime = currTime.tv_sec;

    if (application->configuration.driverVerbosity > 4) {
        cout << "Thread id: " << std::this_thread::get_id()
                << " Wating for message..." << endl;
    }

    // will need to make this compatible for multiple rx ports
    int ret;
    gettimeofday(&application->startRxIntervalTime, NULL);
    // setup thread for post process async verification statistics
    if (application->configuration.enableAsync) {
        dynamic_pointer_cast<SaeApplication>(application)->PostProcessingThread();
    }
    while (!stopThread) {
        if(!simMode) {
            //Check if CV2X is active, if not wait for CV2X Status to be ACTIVE
            sem_wait(&cnt_sem);
            //Check CV2X RX status when only RX is enabled
            if (!application->configuration.enableTxAlways) {
                bool restartFlow = false;
                if (application->radioReceives[index].waitForCv2xToActivate(restartFlow)) {
                    // break out if return fail
                    break;
                }
                if (restartFlow) {
                    if (reSetupRadio(msgType)) {
                        sem_post(&cnt_sem);
                        break;
                    }
                }
            } else {
                // if TX is also enabled check the CV2X status in TX only
                std::unique_lock<std::mutex> lk(cv2xStatusMtx);
                statusCv.wait(lk, []{return (!haltRx or stopThread);});
                if (stopThread) {
                    sem_post(&cnt_sem);
                    break;
                }
            }
            sem_post(&cnt_sem);
        }

        // call application's receive() function to process the packet across
        // stack layers.
        ret = application->receive(index, MAX_PACKET_LEN);
        sem_wait(&cnt_sem);
        if(ret >= 0){
            rxsuccess++;
            if (application->configuration.driverVerbosity) {
                if (rxsuccess % 50 == 0 && rxsuccess > 0){
                    gettimeofday(&currTime, NULL);
                    cout << "Dur(s): " << std::dec
                        << (currTime.tv_sec-startTime) <<
                        " Decode/Rx Success #: " << std::dec << rxsuccess <<
                        " Decode/Rx Fail #: " << std::dec << rxfail << std::endl;
                }
            }
        } else {
            rxfail++;
        }
        sem_post(&cnt_sem);
    }

    if (application->configuration.driverVerbosity) {
        printf("Thread (%08x) closing\n", tid);
    }

    if(application->configuration.enableVerifResLog){
        application->writeResultsLogging();
    }
    if(application->configuration.enableVerifStatLog){
        application->writeVerifLogging();
    }
    if(application->configuration.enableMbdStatLog){
        application->writeMisbehaviorLogging();
    }
    if(msgType == MessageType::BSM || msgType == MessageType::WSA) {
        auto sp = dynamic_pointer_cast<SaeApplication>(application);
        if (sp) {
            sp->printRxStats();
        }
    }
    printf("Total of RX packets is: %d\n", application->totalRxSuccess);

    if(application->ldm != nullptr)
        application->ldm->stopGb();
    if (application->configuration.driverVerbosity) {
        printf("Thread (%08x) closed\n", tid);
    }
}
/**
 * receive function for LDM functionality test.
 *
 * @param [in] msgType, so far only BSM is supported.
 */
void ldmRx(void) {
    auto count = 0;
    FILE *fp;
    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    time_t startTime = currTime.tv_sec;

    if (nullptr == application) {
        cerr << "application is nullptr" << endl;
        return;
    }
    if(application->configuration.enableVerifResLog){
        application->initResultsLogging();
    }
    if(application->configuration.enableVerifStatLog){
        application->initVerifLogging();
    }
    if(application->configuration.enableMbdStatLog)
        application->initMisbehaviorLogging();

    if (application->ldm == nullptr) {
        printf("LDM not initialized properly, exiting\n");
        return;
    }

    int ret = 0;
    uint32_t ldmIndex = 0;
    while (!stopThread)
    {
        // if a new ldm slot index is necessary, retrieve one
        if(ret >= 0){
            ldmIndex = application->ldm->getFreeBsmSlotIdx();
        }
        // else keep the old ldm index and reuse slot
        ret = application->receive(0, MAX_PACKET_LEN, ldmIndex);
        sem_wait(&cnt_sem);
        if(ret >= 0){
            rxsuccess++;
            if (application->configuration.driverVerbosity) {
                if (rxsuccess % 50 == 0 && rxsuccess > 0){
                    gettimeofday(&currTime, NULL);
                    cout << "Dur(s): " << std::dec << (currTime.tv_sec-startTime) <<
                        " Decode/Rx Success #: " << std::dec << rxsuccess <<
                        " Decode/Rx Fail #: " << std::dec << rxfail << std::endl;
                }
            }
        } else {
            rxfail++;
        }
        sem_post(&cnt_sem);
    }
    std::stringstream ss;
    ss << std::this_thread::get_id();
    int tid = (int)std::stoul(ss.str());
    printf("Thread (%08x) closing\n", tid);
    if(application->configuration.enableVerifStatLog){
        application->writeVerifLogging();
    }
    if(application->configuration.enableMbdStatLog){
        application->writeMisbehaviorLogging();
    }

    auto sp = dynamic_pointer_cast<SaeApplication>(application);
    if (sp) {
        sp->printRxStats();
    }
    printf("Total of RX packets is: %d\n", application->totalRxSuccess);

    if(application->ldm != nullptr)
        application->ldm->stopGb();
    printf("Thread (%08x) closed\n", tid);
}

void onSrcL2AddrUpdate(uint32_t addr) {
    if (application) {
        if (application->configuration.driverVerbosity > 3) {
            cout << "new L2 addr:" << addr << endl;
        }

        // update local V2X-IP rmnet addr in a new thread
        std::thread([]() {
            int i = 0;
            while (!stopThread and application) {
                if (0 == application->updateCachedV2xIpIfaceAddr()) {
                    break;
                }
                // if update failed, try again in 100ms and break out after 2 retries
                if (++i > IP_ADDR_RETRY_TIMES) {
                    break;
                }
                cerr << "Try to update V2X-IP rmnet addr later!" << endl;
                std::unique_lock<std::mutex> lck(gTerminateMtx);
                if (gTerminateCv.wait_for(lck,
                                          std::chrono::milliseconds(IP_ADDR_RETRY_INTERVAL_MS),
                                          []{return stopThread == true;})) {
                    if (application->configuration.driverVerbosity > 3) {
                        cout << "Abort updating cached IP addr due to exiting" << endl;
                    }
                    break;
                }
            }
        }).detach();
    }
}

void clearWsaTxSettings() {
    if (!application) {
        cerr << "application invalid!" << endl;
        return;
    }

    if (application->configuration.driverVerbosity > 3) {
        cout << "Clear WSA Tx settings" << endl;
    }

    // deregister L2 addr callback if configured to use dynamic rmnet address
    if (application->configuration.defaultGateway.empty()) {
        if (application->spsTransmits.empty()
            or application->spsTransmits[0].deregisterL2AddrCallback(onSrcL2AddrUpdate)) {
            cerr << "Failed to deregister L2 address callback!" << endl;
        }
    }

    // clear global IP prefix
    auto sp = dynamic_pointer_cast<SaeApplication>(application);
    if (sp) {
        sp->clearGlobalIPv6Prefix();
    }
}

int prepareWsaTx() {
    if (!application) {
        cerr << "application invalid!" << endl;
        return -1;
    }

    if (application->configuration.driverVerbosity > 3) {
        cout << "Prepare WSA Tx" << endl;
    }

    // clear previous WSA Tx settings if exist
    clearWsaTxSettings();

    // set global IPv6 prefix
    auto sp = dynamic_pointer_cast<SaeApplication>(application);
    if (not sp or sp->setGlobalIPv6Prefix() < 0) {
        cerr << "Failed to set global IP info!" << endl;;
        return -1;
    }

    // if configured to use dynamic rmnet address in WSA Tx msgs
    // register listener for src L2 addr updates and get the inital rmnet address
    if (application->configuration.defaultGateway.empty()) {
        if (application->spsTransmits.empty()
            or application->spsTransmits[0].registerL2AddrCallback(onSrcL2AddrUpdate)
            or application->updateCachedV2xIpIfaceAddr()) {
            cerr << "Failed to register L2 address callback!" << endl;
            // clear global IPv6 prefix
            sp->clearGlobalIPv6Prefix();
            return -1;
        }
    }

    return 0;
}

void transmitEventMsg() {
    // default timer
    int tx_timer_fd = 0;
    int timer_misses = 0;
    uint64_t exp = 0;
    ssize_t s;
    int critEventMsgCtr = 0;
    uint64_t lastEventTxTime = 0;
    uint64_t nextSchedTxTime = 0;
    uint64_t currTimeTmp = 0;
    uint64_t waitTime = 0;
    struct itimerspec its = { 0 };
    tx_timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if(tx_timer_fd == -1){
        cerr << "failed to start event timer \n";
        return;
    }
    while (!stopThread) {
        //TODO: need provide a proper way to sync the tx/rx threads
        if(application){
            if(!application->eventTransmits.empty()){
                if (application->pendingTillEmergency() &&
                    application->eventTransmits[0].getCurrentStatus().txStatus ==
                        Cv2xStatusType::ACTIVE) {
                    if(nextSchedTxTime == 0){
                        lastEventTxTime = timestamp_now();
                        nextSchedTxTime = lastEventTxTime + 100;
                    }
                    int ret = application->send(0, TransmitType::EVENT);
                    if (ret <= 0) {
                        cerr << "Failed to send critical event message." << endl;
                    }else{
                        critEventMsgCtr++;
                    }
                    currTimeTmp = timestamp_now();
                    waitTime = nextSchedTxTime - currTimeTmp; //ms
                    if(nextSchedTxTime < currTimeTmp){
                        waitTime = 0;
                    }

                    its.it_value.tv_sec = 0;
                    its.it_value.tv_nsec = (waitTime * 1000000LL);
                    its.it_interval = its.it_value;
                    if(waitTime != 0){
                        if (s = timerfd_settime(tx_timer_fd, 0, &its, NULL) < 0) {
                            std::cerr << "Error setting time\n";
                            close(tx_timer_fd);
                            return;
                        }
                    }
                    s = read(tx_timer_fd, &exp, sizeof(exp));
                    if (s == sizeof(uint64_t) && exp > 1) {
                        timer_misses += (exp-1);
                        if(application->configuration.driverVerbosity){
                            cout << "Event TX timer overruns: Total missed: "
                                << timer_misses << endl;
                        }
                    }
                    // schedule the next tx time based on the first event tx time
                    nextSchedTxTime = nextSchedTxTime + 100;
                }
            }
        }
    }
    if (tx_timer_fd != -1) {
        close(tx_timer_fd);
    }
}

/**
 * transmit thread function
 * @param[in] msgType, type of message we are transmitting, so far only BSM and
 * CAM are supported. DENM is not supported
 * @returns none.
 */
void transmit(MessageType msgType) {
    int timer_misses = 0;
    uint64_t exp;
    ssize_t s;
    int txsuccess = 0;
    int txfail = 0;
    int ret = 0;
    uint32_t txInterval = 0;

    // check if sign stat logging on
    // check off for now
    if(application->configuration.enableSignStatLog)
        application->initSignLogging();

    //Perform message protocol specific setup here
    switch (msgType){
        case MessageType::BSM:
            txInterval = application->configuration.transmitRate;
            if(!application->configuration.enableCongCtrl){
                cout << "Sending BSM messages with period " << txInterval << "ms" << endl;
            }
            break;
        case MessageType::WSA:
            txInterval = application->configuration.wsaInterval;
            cout << "Sending WSA messages with period " << txInterval << "ms" << endl;
            //sending WSA, transmit only, we are simulating RSU, so set the global IP prefix
            if (prepareWsaTx() < 0) {
                cerr << "Failed to prepare WSA Tx" << endl;;
                return;
            }
            break;
        case MessageType::CAM:
            txInterval = application->configuration.transmitRate;
            break;
        case MessageType::DENM:
            cerr << "DENM transmit is not supported" << endl;
            break;
        default:
            break;
    }

    /* Logic here changes if congestion control is enabled */

    // default timer
    int tx_timer_fd = 0;
    if(!application->configuration.enableCongCtrl){
        tx_timer_fd = startTimerMs(txInterval);
        if (tx_timer_fd == -1) {
            cerr << "Failed to start Tx timer" << endl;
            return;
        }
    }

    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    time_t startTime = currTime.tv_sec;
    // main transmitting code
    while (!stopThread) {
        if (application->pendingTillNoEmergency()) {

            if(!simMode){
                //Check if CV2X is active, if not wait for CV2X Status to be ACTIVE
                auto status = application->spsTransmits[0].getCurrentStatus();
                if (Cv2xStatusType::ACTIVE != status.txStatus) {
                    // if both Tx and Rx thread exist, check status in Tx thread
                    // halt Rx if Tx status is not active because flows may need to restart
                    {
                        std::unique_lock<std::mutex> lk(cv2xStatusMtx);
                        haltRx = true;
                    }
                    bool restartFlow = false;
                    if (application->spsTransmits[0].waitForCv2xToActivate(restartFlow)) {
                        // break out if return fail
                        break;
                    }
                    if (restartFlow) {
                        if (reSetupRadio(msgType)) {
                            break;
                        }

                        {
                            // notify rx thread to resume
                            std::lock_guard<std::mutex> lk(cv2xStatusMtx);
                            haltRx = false;
                            statusCv.notify_all();
                        }

                        if(!application->configuration.enableCongCtrl){
                            close(tx_timer_fd);
                            tx_timer_fd = startTimerMs(txInterval);
                        }
                        // need to re-set the WSA Tx after the radio instance is re-created
                        if (MessageType::WSA == msgType
                            and prepareWsaTx() < 0) {
                            cerr << "Failed to prepare WSA Tx" << endl;;
                            break;
                        }
                    }
                }
            }
            ret = application->send(0, TransmitType::SPS);

            if(ret > 0){
                txsuccess++;
                if (application->configuration.driverVerbosity) {
                    if (txsuccess % 50 == 0 && txsuccess > 0){
                        gettimeofday(&currTime, NULL);
                        cout << "Dur(s): " << (currTime.tv_sec-startTime) <<
                            " Encode/Tx Success #: " << txsuccess <<
                            " Encode/Tx Fail #: " << txfail << std::endl;
                    }
                }
            } else {
                txfail++;
            }
        }
        // default
        if(!application->configuration.enableCongCtrl){
            s = read(tx_timer_fd, &exp, sizeof(exp));
            if (s == sizeof(uint64_t) && exp > 1) {
                timer_misses += (exp-1);
                if(application->configuration.driverVerbosity){
                    cout << "TX timer overruns: Total missed: " << timer_misses << endl;
                }
            }
        }
    }

    // notify rx thread in case it's waiting for status notification
    statusCv.notify_all();

    if(msgType == MessageType::WSA) {
        clearWsaTxSettings();
    }

    // dump out any logging information related to signing
    if(application->configuration.enableSignStatLog){
        application->writeSignLogging();
    }
    if(msgType == MessageType::BSM || msgType == MessageType::WSA) {
        auto sp = dynamic_pointer_cast<SaeApplication>(application);
        if (sp) {
            sp->printTxStats();
        }
    }
    printf("Total of TX packets is: %d\n", application->totalTxSuccess);

    if(application->ldm != nullptr)
        application->ldm->stopGb();

    if (tx_timer_fd != -1) {
        close(tx_timer_fd);
    }
}
/**
 * transmit BSM from pre-recorded csv file.
 *
 * @param[in] file name of the csv file.
 * @msgType type of the message, so far only BSM is supported for this test.
 * @returns none.
 */
void txRecorded(string file) {
    srand(timestamp_now());
    ifstream recordFile(file);
    string line;
    bool go = true;
    bool bsmLog = false;


    if (recordFile.is_open())
    {
        if (getline(recordFile, line)) {
            // check if it is bsm format
            if (line != LOG_HEADER || application->configuration.preRecordedBsmLog) {
                bsmLog = true;
            }
        } else {
            cout << "txRecorded - fail to read " << file << endl;
            return;
        }

        int tx_timer_fd = startTimerMs(application->configuration.transmitRate);
        if (tx_timer_fd == -1) {
            cerr << "Failed to start record Tx timer" << endl;
            return;
        }

        uint64_t exp = 0;

        while (go and !stopThread) {
            if (getline(recordFile, line))
            {
                if (application->configuration.eventPorts.size()) {
                    const auto iEvent = rand() % application->configuration.eventPorts.size();
                    auto mc = application->eventContents[iEvent];
                    auto len = encode_singleline_fromCSV((char *)line.data(), mc.get(), bsmLog);
                    if (application->configuration.enableSecurity) {
                        len = application->encodeAndSignMsg(
                            mc, SecurityService::SignType::ST_CERTIFICATE);
                    }
                    // event priority is set per packet using traffic class
                    application->eventTransmits[iEvent].transmit(
                        mc->abuf.data, len,
                        application->configuration.eventPriority);
                }
                if (application->configuration.spsPorts.size()) {
                    if (getline(recordFile, line)) {
                        const auto iSps = rand() % application->configuration.spsPorts.size();
                        auto mc = application->spsContents[iSps];
                        auto len = encode_singleline_fromCSV((char*)line.data(),mc.get(), bsmLog);
                        if (application->configuration.enableSecurity) {
                            len = application->encodeAndSignMsg(
                                mc, SecurityService::SignType::ST_AUTO);
                        }
                        // SPS priority is set when creating the flow
                        application->spsTransmits[iSps].transmit(
                            mc->abuf.data, len,
                            Priority::PRIORITY_UNKNOWN);
                    }
                }

                if (read(tx_timer_fd, &exp, sizeof(exp)) == sizeof(uint64_t) && exp > 1) {
                    if(application->configuration.driverVerbosity){
                        cout << "Pre-record TX timer overruns" << endl;
                    }
                }
            } else {
                go = false;
            }
        }

        close(tx_timer_fd);
    }
    else {
        cout << "Recorded File doesn't exists.\n";
    }

}
/**
 * transmit pre-recorded BSM message via radio simulation
 *
 * @param [in] msgType , so far only BSM is supported in this mode
 * @returns none.
 */
void simTxRecorded(string file)
{
    ifstream recordFile(file);
    string line;
    if (recordFile.is_open())
    {
        auto timer = timestamp_now();
        while (!stopThread)
        {
            if (timer + application->configuration.transmitRate < timestamp_now()) {
                if (getline(recordFile, line))
                {
                    auto mc = application->txSimMsg;
                    auto len = encode_singleline_fromCSV((char*)line.data(), mc.get(), true);
                    abuf_put(&mc->abuf, len);
                    application->simTransmit->transmit(mc->abuf.data, len,
                                                       Priority::PRIORITY_UNKNOWN);
                    timer = timestamp_now();
                }else{
                    return;
                }
            }
        }
    }else {
        cout << "Recorded File doesn't exists.\n";
    }
}

void tunnelModeTx(void) {
    auto timer = timestamp_now();
    while (!stopThread)
    {
        if (timer + application->configuration.transmitRate < timestamp_now()) {
            auto SaeApp = dynamic_pointer_cast<SaeApplication>(application);
            if (not SaeApp) {
                break;
            }
            SaeApp->sendTuncBsm(0, TransmitType::SPS);
            timer = timestamp_now();
        }
    }
}

void tunnelModeRx(void) {
    while (!stopThread)
    {
        auto SaeApp = dynamic_pointer_cast<SaeApplication>(application);
        if (not SaeApp) {
            break;
        }
        const auto mc = SaeApp->receivedContents[0];
        const auto recCount =
                SaeApp->radioReceives[0].receive(mc->abuf.data,
                                                    MAX_PACKET_LEN-ABUF_HEADROOM);
        abuf_put(&mc->abuf, recCount);
        const auto ldmIndex = application->ldm->getFreeBsmSlotIdx();
        SaeApp->receiveTuncBsm(0, recCount, ldmIndex);
        if (!SaeApp->ldm->filterBsm(ldmIndex)) {
            const auto bsm = static_cast<bsm_value_t *>(mc->j2735_msg);
            SaeApp->ldm->setIndex(bsm->id, ldmIndex, mc);
        }
    }
}

/**
 * run safety application.
 */
void runApps(void) {
    auto hostMsg = std::make_shared<msg_contents>();
    rv_specs* rvSpecs = new rv_specs;
    while (!stopThread) {
        for (auto rvMsg : application->ldm->bsmSnapshot()) {
            application->fillMsg(hostMsg);
            fill_RV_specs(hostMsg.get(), rvMsg.get(), rvSpecs);
            forward_collision_warning(rvMsg.get(), rvSpecs);
            EEBL_warning(rvMsg.get(), rvSpecs);
            accident_ahead_warning(rvMsg.get(), rvSpecs);
            print_rvspecs(rvSpecs);
        }
    }
    delete rvSpecs;
}

/**
 * run for periodic logs.
 */
void periodicDiagLog(void) {
    uint64_t exp = 0;
    ssize_t s;
    timerFd = startTimerMs(application->configuration.transmitRate);
    if (timerFd == -1) {
        cerr << "Failed to start diag log timer" << endl;
        return;
    }

    while (!stopThread) {
        if (not application) {
            break;
        }
        application->diagLogPktGenericInfo();
        s = read(timerFd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            printf("periodicDiagLog error read from timerFd\n");
        }
    }
    return;
}

void printUse() {
    cout << "Usage: qits [options] <Config File Path>\n";
    cout << "  At least one option is needed and Config File Path is always required.\n";
    cout << "Options:\n";
    cout << "  General options: \n";
    cout << "  -h Prints help options.\n";
    cout << "  -D Dump raw received packet.\n";
    cout << "  -v Don't print received remote vehicle summary(for performance measurement).\n";
    cout << "  -o <CSV file path> write received BSM into CSV file.\n\n";
    cout << "  Modes:\n";
    cout << "  -t Transmits Cv2x data. Runs by default with -b. See -b.\n";
    cout << "  -r Receives Cv2x data. Runs by defaullt with -b. See -b.\n";
    cout << "  -i <other_device_ip_address> <port> Simulates CV2X and sends packets via TCP ";
    cout << "instead of OTA.\n";
    cout << "           note: You may enable UDP if desired via the config file\n";
    cout << "  -j <other_device_ip_address> <port> Simulates CV2X and receives packets via TCP ";
    cout << "instead of OTA.\n";
    cout << "           note: You may enable UDP if desired via the config file\n";
    cout << "  -b SAE WSMP BSMS.\n";
    cout << "  -w SAE WSMP WRA(in WSA).\n";
#ifdef ETSI
    cout << "  -c ETSI CAMs.\n";
    cout << "  -d ETSI DENMs.\n";
#endif
    cout << "  Incomplete Modes\n";
    cout << "  -l LDM mode; Adds -r if nothing specified. Use it with -r or -j.\n";
    cout << "  -s Safety Apps Mode; Adds -l if not specified. Runs by default with -b.\n";
    cout << "  -p <Pre-Recorded File Path> Transmits from pre-recorded file.\n";
    cout << "  -T Tunnel Transmit.\n";
    cout << "  -x Tunnel Receive. It automatically calls -l. See: -l.\n";
    cout << "Examples (assuming path to ObeConfig.conf is /etc/ObeConfig.conf):\n";
    cout << "  Example: qits -t /etc/ObeConfig.conf\n";
    cout << "  Example above will transmit BSMs (the default packet type)\n\n";
    cout << "  Example: qits -r -b /etc/ObeConfig.conf\n";
    cout << "  Example above will run: receive mode with basic safety messages.\n\n";
    cout << "  Example: qits -t -r -b /etc/ObeConfig.conf\n";
    cout << "  Example above will run: transmit and receive mode with basic safety messages.\n\n";
    cout << "  Example: qits -t -l -s /etc/ObeConfig.conf\n";
    cout << "  Example above will run: transmit mode, ldm receive mode and the safety apps.\n\n";
    cout << "  Example: qits -i 127.0.0.1 9000 /etc/ObeConfig.conf\n";
    cout << "  Example above will run: simulation transmit mode (TCP/UDP),\n";
    cout << "    sending BSMs over port 9000 to ip address 127.0.0.1\n\n";
    cout << "  Note: options -i and -j require SourceIpv4Address to be set\n";
}

void configFileCheck(string& configFile)
{
    if (configFile[0] == '-')
    {
        cout << "No config file specified.\n";
        cout << "Setting config file to default .\\ObeConfig.conf\n";
        configFile = string("ObeConfig.conf");
    }
    if (configFile.find(".conf") == std::string::npos) {
        cout << "Config file doesn't have .conf extension...\n";
        cout << "Are you sure you added or you added the right config file?\n\n\n\n";
        cout << "Setting config file to default .\\ObeConfig.conf\n";
        configFile = string("ObeConfig.conf");
    }
}

/* Sets parameters according to runtime arguments */
void getModes(char mode, int& idx, int& argc, char** argv, bool& tx, bool& rx,
    bool& ldm, bool& help, bool& safetyApps, bool& bsm, bool& wsa,
    bool& cam, bool& denm, bool& preRecorded, string& preRecordedFile,
    bool& txSim, bool& rxSim, bool& tunnelTx, bool& tunnelRx,
    bool& csv, string& txSimIp, string& rxSimIp,
    uint16_t& txSimPort, uint16_t& rxSimPort) {

    bsm = true; //default
    switch (mode)
    {
    case 'h':
        help = true;
        break;
    case 't':
        tx = true;
        break;
    case 'r':
        rx = true;
        break;
    case 's':
        safetyApps = true;
        ldm = true;
        rx = true;
        break;
    case 'p':
        preRecorded = true;
        idx += 1;
        preRecordedFile = string(argv[idx]);
        break;
    case 'x':
        tunnelRx = true;
        ldm = true;
        rx = true;
        break;
    case 'T':
        tunnelTx = true;
        tx = true;
        break;
    case 'l':
        ldm = true;
        rx = true;
        break;
    case 'b':
        bsm = true;
        wsa = false;
        break;
    case 'w':
        wsa = true;
        bsm = false;
        break;
#ifdef ETSI
    case 'c':
        cam = true;
        bsm = false;
        break;
    case 'd':
        denm = true;
        bsm = false;
        break;
#endif
    case 'i':
        txSim = true;
        simMode = true;
        if(idx+2 > argc-1){
           printUse();
           fprintf(stderr, "\nInvalid usage of -i option\n");
           exit(0);
        }
        // need to test for valid num of arguments here
        idx += 1;
        try {
            txSimIp = string(argv[idx]);
        }catch (std::invalid_argument& e){
            fprintf(stderr, "Invalid IP Address argument\n");
            exit(0);
        }
        idx += 1;
        try {
            txSimPort = stoi(string(argv[idx]), nullptr, 10);
        }catch (std::invalid_argument& e){
            fprintf(stderr, "Invalid port argument\n");
            fprintf(stdout, "Changing to 9000 by default\n");
            txSimPort = 9000;
        }
        break;
    case 'j':
        rxSim = true;
        simMode = true;
        if(idx+2 > argc-1){
           printUse();
           fprintf(stderr, "\nInvalid usage of -j option\n");
           exit(0);
        }
        idx += 1;
        try {
            rxSimIp = string(argv[idx]);
        }catch (std::invalid_argument& e){
            fprintf(stderr, "Invalid IP Address argument\n");
            fprintf(stdout, "Changing to accept all\n");
            rxSimIp = stoi(string("0.0.0.0"), nullptr, 10);
        }
        idx += 1;
        try {
            rxSimPort = stoi(string(argv[idx]), nullptr, 10);
        }catch (std::invalid_argument& e){
            fprintf(stderr, "Invalid port argument\n");
            fprintf(stdout, "Changing to 9000 by default\n");
            rxSimPort = 9000;
        }
        break;
    case 'o':
        csv = true;
        idx++;
        csvFileName = string(argv[idx]);
        break;
    case 'D':
        dump_raw = true;
        break;
    case 'v':
        print_rv = false;
        break;
    case 'q':
        enableDiagLog = true;
        break;
    default:
        break;
    }
}

int setup(const bool tx, const bool rx,
    const bool ldm, const bool help, const bool safetyApps,
    const bool bsm, const bool wsa, const bool cam, const bool denm, const bool preRecorded,
    const string preRecordedFile, const bool txSim, const bool rxSim,
    const bool tunnelTx,const bool tunnelRx, const string txSimIp,
    const string  rxSimIp, const  uint16_t txSimPort,
    const uint16_t rxSimPort, char* configFile)
{

    if (help)
    {
        printUse();
        return 0;
    }
#ifndef SIM_BUILD
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::cout << "Telematics SDK v" << std::to_string(sdkVersion.major) << "."
                          << std::to_string(sdkVersion.minor) << "."
                          << std::to_string(sdkVersion.patch) << std::endl <<
                          "Release name: " << sdkReleaseName << std::endl;
#endif
    sem_init(&cnt_sem, 0, 1);
    MessageType msgType = MessageType::BSM;

    if (bsm || wsa) {
        msgType = bsm? MessageType::BSM : MessageType::WSA;
        printf("Will be creating application for: ");
        if(msgType == MessageType::BSM)
            printf("BSMs\n");
        else
            printf("WSAs\n");
        // wsa not compatible with simulation mode
        if((txSim || rxSim) && wsa){
           fprintf(stderr, "WSA requires radio mode.\n");
           return -1;
        }
        if (txSim)
            application = make_shared<SaeApplication>(txSimIp, txSimPort, string(""), 0,
                                                      configFile, msgType, csv, enableDiagLog);
        else if (rxSim)
            application = make_shared<SaeApplication>(string(""), 0, rxSimIp, rxSimPort,
                                                      configFile, msgType, csv, enableDiagLog);
        else
            application = make_shared<SaeApplication>(configFile, msgType, csv, enableDiagLog);
        // prevent tx and rx during wsa mode
        if(rx && wsa){
            printf("Warning: Can only do either TX only or RX only when wsa is enabled.\n");
            printf("Disabling enableTxAlways config item. Now in RX only mode.\n");
            application->configuration.enableTxAlways = false;
        }
    } else {
#ifdef ETSI
        msgType = MessageType::CAM;
        printf("Will be creating application for: ");
        if (cam) {
            msgType = MessageType::CAM;
            printf("CAMs\n");
        } else if (denm) {
            msgType = MessageType::DENM;
            printf("DENM\n");
        } else {
            printf("Unknown\n");
            return -1;
        }
        if (txSim)
            application = make_shared<EtsiApplication>(txSimIp, txSimPort, string(""), 0, configFile);
        else if (rxSim)
            application = make_shared<EtsiApplication>(string(""), 0, rxSimIp, rxSimPort, configFile);
        else
            application = make_shared<EtsiApplication>(configFile, msgType);
#endif
    }

    if (not application
        or not application->configuration.isValid
        or not application->init()) {
        cerr << "Initialization Failed" << endl;
        return -1;
    }

    // check if we want to have tx on at same time as rx (either ethernet or radio)
    if(application->configuration.enableTxAlways &&
        (rx || rxSim) && application->configuration.driverVerbosity){
            printf("Device will be sending AND receiving packets\n");
    }else if(!tx && !txSim && application->configuration.driverVerbosity){
            printf("Device will be in receive-only mode\n");
    }

    if ((tx || application->configuration.enableTxAlways) && !txSim && !rxSim)
    {
        if(!rx && application->radioReceives.size()){
            // make sure to clean up the rx subscriptions since we will not use them
            for (uint8_t i = 0; i < application->radioReceives.size(); i++)
            {
                application->radioReceives[i].closeFlow();
            }
            application->radioReceives.erase(application->radioReceives.begin(),
                 application->radioReceives.end());
        }

        if (application->spsTransmits.empty() || application->eventTransmits.empty()) {
            cerr << "Tx flow not created, please check configuration" << endl;
            return -1;
        }

        if (tunnelTx) {
            if (cam || denm) {
                cout << "Tunnel Mode only supports BSM" << endl;
                return -1;
            }
            threads.push_back(thread(tunnelModeTx));
        } else {
            threads.push_back(thread(transmit, msgType));
            threads.push_back(thread(transmitEventMsg));
            // wait some time for congestion control to activate (if enabled)
            if(application->configuration.enableCongCtrl){
                usleep(500000);
            }
        }
    }

    if(application->configuration.driverVerbosity > 4)
        printf("Number of threads after tx is: %d\n", (int)threads.size());

    if (csv) {
        application->openLogFile(csvFileName);
        // TODO add option for only bsm related log
    }

    // for sae message configuration. initialize security-related features for qits
    if((rx || rxSim) && (bsm || wsa)){
        if(application->configuration.enableSecurity){
            if(application->configuration.enableVerifStatLog){
                application->initVerifLogging();
            }
            if(application->configuration.enableMbdStatLog)
                application->initMisbehaviorLogging();
        }
    }

    if (rx && !rxSim)
    {
        if (application->radioReceives.empty()) {
            cerr << "Rx flow not created, please check configuration" << endl;
            return -1;
        }

        // l2 filter and throttle manager timer thread
        if (isL2SrcFilteringEnabled())
            rvL2SrcFiltering(application);

        if(application->configuration.enableL2FloodingDetect){
            cout << "starting flood detection and mitigation thread\n";
            l2FloodingMitigation(application);
        }

        if (ldm)
        {
            if (cam || denm) {
                cout << "LDM Mode only supports BSM" << endl;
                return -1;
            }
            // setup full ldm
            if(application->configuration.ldmSize) {
                application->setupLdm();
            }
            if (tunnelRx) {
                threads.push_back(thread(tunnelModeRx));
            }
            else {
                // TODO: Implement for CAM, DENM as well
                if (application->configuration.driverVerbosity) {
                    cout << "Number of Radio LDM RX Threads: " <<
                            (int)application->configuration.numRxThreadsRadio << endl;
                }
                for (int i = 0; i < application->configuration.numRxThreadsRadio; i++) {
                    threads.push_back(thread(ldmRx));
                }
            }
        }
        else {
            if (cam) {
                threads.push_back(thread(receive, MessageType::CAM, 0));
            }
            else if (denm)
            {
                threads.push_back(thread(receive, MessageType::DENM, 0));
            }
            else {
                // TODO: Implement for CAM, DENM as well
                if (application->configuration.driverVerbosity) {
                    cout << "Number of Radio RX Threads: " <<
                            (int)application->configuration.numRxThreadsRadio << endl;
                }
                for (int i = 0; i < application->configuration.numRxThreadsRadio; i++) {
                    threads.push_back(thread(receive, msgType, 0));
                }
            }
        }
    }

    if(application->configuration.driverVerbosity > 4)
        printf("Number of threads after rx is: %d\n", (int)threads.size());

    if (txSim && rxSim) {
        cout <<
            "Per building specifications, Simulating tx and rx is not supported.\n";
        return -1;
    }

    if (txSim)
    {
        if(application->configuration.driverVerbosity)
            cout << "Starting sim transmit thread\n";
        if (preRecorded)
        {
            if (cam || denm) {
                    cout <<
                        "Transmit from pre-recorded file only supports BSM" << endl;
                return -1;
            }
            threads.push_back(thread(simTxRecorded, preRecordedFile));
        }
        else {
          threads.push_back(thread(transmit, msgType));
        }
    }
    if(application->configuration.driverVerbosity > 4)
        printf("Number of threads after simtransmit is: %d\n", (int)threads.size());

    if (rxSim)
    {
        if (ldm) {
            if (cam || denm) {
                    cout << "LDM Mode only supports BSM" << endl;
                return -1;
            }
            // setup full ldm
            if(application->configuration.ldmSize) {
                application->setupLdm();
            }
            // TODO: Implement for CAM, DENM as well
            if (application->configuration.driverVerbosity) {
                cout << "Number of Ethernet LDM RX Threads: " <<
                        (int)application->configuration.numRxThreadsEth << endl;
            }
            for (int i = 0; i < application->configuration.numRxThreadsEth; i++) {
                threads.push_back(thread(ldmRx));
            }
        }
        else {

            if (cam) {
                threads.push_back(thread(receive, MessageType::CAM, 0));
            }
            else if (denm)
            {
                threads.push_back(thread(receive, MessageType::DENM, 0));
            }
            else {
                // Multi-Threading Capability for RxSim
                if (application->configuration.driverVerbosity) {
                    cout << "Number of Ethernet RX Threads: " <<
                            (int)application->configuration.numRxThreadsEth << endl;
                }
                for(int i = 0; i < application->configuration.numRxThreadsEth; i++){
                    threads.push_back(thread(receive, msgType, 0));
                }
            }
        }
    }

    if(application->configuration.driverVerbosity > 4)
        printf("Number of threads after simreceive is %d\n", (int)threads.size());

    if (preRecorded && !txSim)
    {
        if (cam || denm) {
            cout << "Only BSM is supported for pre-recorded transmit" << endl;
            return -1;
        }
        threads.push_back(thread(txRecorded, preRecordedFile));
    }

    if (safetyApps)
    {
        if (cam || denm) {
            cout << "Only BSM is supported for safetyApp demo" << endl;
            return -1;
        }
        threads.push_back(thread(runApps));
    }

    if (enableDiagLog) {
        RadioInterface::enableDiagLog(enableDiagLog);
        threads.push_back(thread(periodicDiagLog));
    }

    return 0;
}

int main(int argc, char** argv) {
#ifndef SIM_BUILD
    std::vector<std::string> groups{"system", "diag", "radio", "locclient", "mvm",
                                    "dlt", "spi", "gpio", "logd"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cerr << "Adding supplementary group failed!" << std::endl;
        return -1;
    }

    auto uid = getuid();
    if (uid == 0) {
        /*Change running as non-root user*/
        std::unordered_set<int8_t> newUserCaps{CAP_NET_ADMIN, CAP_SYS_NICE};
        auto changeUser = Utils::changeUser("its", newUserCaps);
        if (telux::common::ErrorCode::SUCCESS != changeUser) {
            cerr << "change user failed " << Utils::getErrorCodeAsString(changeUser) << std::endl;
            //continue even if change to non-root user fail;
        }
    }
#endif
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGHUP);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
#ifndef SIM_BUILD
    SignalHandlerCb cb = (SignalHandlerCb)signalHandler;
    SignalHandler::registerSignalHandler(sigset, cb);
#endif
    string txSimIp, rxSimIp;
    uint16_t txSimPort = 0, rxSimPort = 0;
    bool tx, rx, ldm, help, safetyApps, bsm, wsa, cam, denm, preRecorded, txSim, rxSim;
    bool tunnelTx, tunnelRx;
    tx = rx = ldm = help = safetyApps = wsa = cam = denm = tunnelTx = tunnelRx = false;
    preRecorded = txSim = rxSim = false;
    // by default bsm is true
    bsm = true;
    if (argc <= 2)
    {
        printUse();
        return 0;
    }
    string configFile = string(argv[argc-1]);
    configFileCheck(configFile);
    string preRecordedFile;
    int idx = 1;
    //Get all options and file path
    for (; idx < (argc - 1); idx++) {
        getModes(argv[idx][1], idx, argc, argv, tx, rx, ldm, help,
            safetyApps, bsm, wsa, cam, denm, preRecorded, preRecordedFile, txSim,
            rxSim, tunnelTx, tunnelRx, csv, txSimIp, rxSimIp, txSimPort, rxSimPort);
    }

    // Let user know how QITS will be running
    printf("Enabled Settings Are: ");
    // communication settings
    if(txSim)
        printf("TX SIM ON; ");
    if(tx)
        printf("TX RADIO ON; ");
    if(rxSim)
        printf("RX SIM ON; ");
    if(rx)
        printf("RX RADIO ON; ");
    if(tunnelTx)
        printf("TUNNEL TX ON; ");
    if(tunnelRx)
        printf("TUNNEL RX ON; ");
    if(ldm)
        printf("LDM ON; ");
    if(wsa) {
        bsm = false;
        printf("WSA ON; ");
    }

    // Packet/protocol type information
    if(bsm)
        printf("BSM; ");
    if(cam)
        printf("CAM; ");
    if(denm)
        printf("DENM; ");
    // for wsa mode, should not have tx and rx at same time
    if(rx && tx && wsa){
        printf("Warning: Can only do either TX only or RX only when wsa is enabled.\n");
        printf("Setting to tx only by default\n");
        rx = false;
    }
    std::cout << "CONFIG_FILE: " <<  configFile << std::endl;

    // for wsa mode, should not have tx and rx at same time
    std::cout << "Checking if rx, tx, wsa are enabled\n";
    if(rx && tx && wsa){
        printf("Warning: Can only do either TX only or RX only when wsa is enabled.\n");
        printf("Setting to tx only by default\n");
        rx = false;
    }

    if (setup(tx, rx, ldm, help, safetyApps, bsm, wsa, cam, denm, preRecorded,
        preRecordedFile, txSim, rxSim, tunnelTx, tunnelRx, txSimIp, rxSimIp,
        txSimPort, rxSimPort, (char*)configFile.data()) < 0) {
        cout << "Failed to launch program" << endl;
    }

    joinThreads();

    if(!rxSim && !txSim && application){
        application->closeAllRadio();
    }
    return 0;
}
