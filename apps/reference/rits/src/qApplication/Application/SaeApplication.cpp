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
 *  Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
  * @file: SaeApplication.cpp
  *
  * @brief: class for ITS stack application - SAE
  */
#include <sys/time.h>
#include "SaeApplication.hpp"
#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <fstream>
#include <sstream>
#include <sys/time.h>
#include "asnbuf.h"
#include "wsmp.h"
#include "qUtils.hpp"

// Each thread that is receiving and verifying will use this for logging purposes
thread_local int verif_fails = 0;
thread_local std::vector<MisbehaviorStats> misbehaviorStats;
thread_local std::vector<VerifStats> verifStats;
thread_local int rxFail = 0;
thread_local int txFail = 0;
thread_local int encFail = 0;
thread_local int txSuccess = 0;
thread_local int syncVerifFail = 0;
thread_local int syncVerifSuccess = 0;
thread_local int totalSimLossPkts = 0;
thread_local std::shared_ptr<msg_contents> threadMc = nullptr;
thread_local std::shared_ptr<msg_contents> hostMc = nullptr;
static int64_t async_index = SHARED_BUFFER_MAX_SIZE;
static bool overridePsidCheck = false;
static bool enableCongCtrl = false;
static bool enableMbd = false;
static int secVerbosity = 0;
static shared_ptr<QMonitor> qMonPtr;
static RadioReceive* radioReceivePtr;
static bsm_data bs;
static std::mutex AsyncMtx;
static SecurityService* asyncSecService;
static std::condition_variable* writeMutexCvSae;
static int shared_index = 0;
static int start_index = 0;
static std::atomic<int> rxSuccess{0};
static std::atomic<int> asyncVerifFail{0};
static std::atomic<int> asyncVerifSuccess{0};
static std::atomic<int> asyncMbdUndetected{0};
static std::atomic<int> asyncMbdDetected{0};
static std::atomic<int> decFail{0};
static int asyncCallbackVerifSuccess = 0;
static int asyncCallbackVerifFail = 0;
static int prevVerifSuccess = 0;
static int prevVerifFail = 0;
int PostProcessingCbData[PP_BUFFER_MAX_SIZE] ={0};
static int begin_flag = true;
static int buffer_full = false;
static struct timeval currTime;
static double prevTimeStamp, prevBatchTimeStamp;
static double LogstartTime, avgRate, minBatchTime, avgBatchTime, maxBatchTime;
std::vector<asyncCbData_t> SaeApplication::asyncCbData;
std::vector<std::thread> asyncThreads;
sem_t verificationSem;
bool SaeApplication::exitAsync = false;
bool* writeLogFinishSae;
static VerifStats* asyncVerifStat;
static MisbehaviorStats* asyncMbdStat;
static ResultLoggingStats* asyncLogStat ;
static bool resFileLogging = false;

SaeApplication::SaeApplication(char *fileConfiguration,  MessageType msgType,
    bool enableCsvLog, bool enableDiagLog):
    ApplicationBase(fileConfiguration, msgType, enableCsvLog, enableDiagLog) {
    if (not configuration.isValid) {
        return;
    }
    sem_init(&verificationSem, 0, 0);
    gettimeofday(&currTime, NULL);
    LogstartTime = currTime.tv_sec*1000.0 + currTime.tv_usec/1000;
    prevBatchTimeStamp = LogstartTime;
    wraInterval = std::chrono::milliseconds::zero();

    writeMutexCvSae = &writeMutexCv;
    writeLogFinish = true;
    writeLogFinishSae = &writeLogFinish;
    resFileLogging = configuration.enableVerifResLog;
}

SaeApplication::SaeApplication(const string txIpv4, const uint16_t txPort,
        const string rxIpv4, const uint16_t rxPort,
        char* fileConfiguration, MessageType msgType, bool enableCsvLog, bool enableDiagLog) :
        ApplicationBase(txIpv4, txPort, rxIpv4, rxPort, fileConfiguration,
        enableCsvLog, enableDiagLog) {
    SaeApplication(fileConfiguration, msgType, enableCsvLog, enableDiagLog);
}

bool SaeApplication::init() {
    if (not configuration.isValid) {
        printf("SaeApplication invalid configuration\n");
        return false;
    }

    if (false == ApplicationBase::init()) {
        printf("SaeApplication initialization failed\n");
        return false;
    }

    if(configuration.enableAsync){
        // enabled when doing RX, no need for TX
        asyncSecService = SecService.get();
        overridePsidCheck = configuration.overridePsidCheck;
        enableCongCtrl = configuration.enableCongCtrl;
        enableMbd = configuration.enableMbd;
        secVerbosity = configuration.secVerbosity;
        qMonPtr = qMon;
        if(radioReceives.size()){
            radioReceivePtr = &radioReceives[0];
        }
        for (int i = 0; i < SHARED_BUFFER_MAX_SIZE; i++) {
            asyncCbData_t tmpData;
            #if AEROLINK
            tmpData.msgParseContext = (SecuredMessageParserC*)calloc(1, sizeof(SecuredMessageParserC));
            AerolinkSecurity* tmpAeroSecurity =
                static_cast<AerolinkSecurity*>(asyncSecService);
            if(tmpAeroSecurity){
                    if(tmpAeroSecurity->createNewSmp(
                        (SecuredMessageParserC*)(tmpData.msgParseContext)) <= -1){
                        printf("Error creating secure message generator for this packet.\n");
                        return false;
                    }
            }
            #endif
            asyncCbData.push_back(tmpData);
        }
    }

     return true;
}

SaeApplication::~SaeApplication() {
    printf("Total number of transmitted packets: %d\n",totalTxSuccess);
    printf("Total number of received packets: %d\n",totalRxSuccess);
    exit_ = true;
    {
        if(enableCsvLog_ && writeMutexCvSae && ApplicationBase::csvfp){
            std::unique_lock<std::mutex> csvLk(csvMutex);
            if(writeLogFinishSae != nullptr){
                writeMutexCvSae->wait(csvLk, [&]{ return *writeLogFinishSae; });
            }
            fclose(ApplicationBase::csvfp);
            ApplicationBase::csvfp = nullptr;
        }
    }
    // notify the wraThread to exit
    #ifdef WITH_WSA
    if(MsgType == MessageType::WSA){
        {
            std::unique_lock<std::mutex> lk(wraMutex);
            wraCv.notify_all();
        }
        if (wraThread.joinable() == true) {
            wraThread.join();
        }
        // delete default route in OBU if previously set
        deleteDefaultRouteInObu();
    }
    #endif
    if (isTxSim) {
        freeMsg(txSimMsg);
    }
    for (auto mc : eventContents) {
        freeMsg(mc);
    }
    for (auto mc : spsContents) {
        freeMsg(mc);
    }
    if (isRxSim) {
        freeMsg(rxSimMsg);
    }
    for (auto mc : receivedContents) {
        freeMsg(mc);
    }
}

std::string SAEgetCurrentTimestamp()
{
    using std::chrono::system_clock;
    auto currentTime = std::chrono::system_clock::now();
    char buffer[MAX_TIMESTAMP_BUFFER_SIZE];
    auto sinceEpoch = currentTime.time_since_epoch().count() / 1000000;
    auto millis = sinceEpoch % 1000;
    std::time_t tt = system_clock::to_time_t ( currentTime );
    auto timeinfo = localtime (&tt);
    int ret = strftime (buffer,80,"%F-%H:%M:%S.",timeinfo);
    snprintf(&buffer[ret], MAX_TIMESTAMP_BUFFER_SIZE, "%03d", (int)millis);
    return std::string(buffer);
}

void SaeApplication::printRxStats() {
    if(configuration.enableAsync)
    {
        exitAsync = true;
        sem_post(&verificationSem);

        for (auto &th : asyncThreads) {
            if(th.joinable()){
                if (configuration.appVerbosity) {
                    std::cout<< "Waiting for async threads to join....\n";
                }
                th.join();
            }
        }
        if (configuration.appVerbosity) {
            std::cout << "Async threads all joined\n";
        }

        std::stringstream ss;
        ss << std::this_thread::get_id();
        int tid = (int)std::stoul(ss.str());
        printf("Thread (%08x) rx fails is: %d\n", tid, rxFail);
        printf("Thread (%08x) decode fails is: %d\n", tid, decFail.load());
        printf("Thread (%08x) rx successes is: %d\n", tid, rxSuccess.load());
        if (configuration.enableSecurity){
            printf("note: verification results may include consistency and relevancy checks\n");
            printf("Thread (%08x) verif fails is: %d\n", tid, asyncVerifFail.load());
            printf("Thread (%08x) verif success is: %d\n", tid, asyncVerifSuccess.load());
            printf("Thread (%08x) mbd detected is: %d\n", tid, asyncMbdDetected.load());
            printf("Thread (%08x) mbd undetected is: %d\n", tid, asyncMbdUndetected.load());
        }
        totalRxSuccess=rxSuccess;
    }
    else
    {
        sem_wait(&this->log_sem);
        std::stringstream ss;
        ss << std::this_thread::get_id();
        int tid = (int)std::stoul(ss.str());
        printf("Thread (%08x) rx fails is: %d\n", tid, rxFail);
        printf("Thread (%08x) rx successes is: %d\n", tid, rxSuccess.load());
        printf("Thread (%08x) decode fails is: %d\n", tid, decFail.load());
        if (configuration.enableSecurity){
            printf("note: verification results may include consistency and relevancy checks\n");
            printf("Thread (%08x) verif fails is: %d\n", tid, syncVerifFail);
            printf("Thread (%08x) verif success is: %d\n", tid, syncVerifSuccess);
        }
        totalRxSuccess+=rxSuccess;
        sem_post(&this->log_sem);
    }
}

void SaeApplication::printTxStats() {
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    int tid = (int)std::stoul(ss.str());
    printf("Thread (%08x) tx fails is: %d\n", tid, txFail);
    printf("Thread (%08x) tx successes is: %d\n", tid, txSuccess);
    if (configuration.enableSecurity){
        printf("Thread (%08x) sign fails is: %d\n", tid, ApplicationBase::signFail);
        printf("Thread (%08x) sign success is: %d\n", tid, ApplicationBase::signSuccess);
    }
    totalTxSuccess+=txSuccess;
    sem_post(&this->log_sem);
}


// fill data for logging related to received bsms
void SaeApplication::fillLoggingData(bsm_value_t* bsm, bsm_data* bs){
    bs->id = bsm->id;
    bs->timestamp_ms = bsm->timestamp_ms;
    bs->secMark_ms = bsm->secMark_ms;
    bs->MsgCount = bsm->MsgCount;
    bs->Latitude = bsm->Latitude;
    bs->Longitude = bsm->Longitude;
    bs->Elevation = bsm->Elevation;
    bs->SemiMajorAxisAccuracy = bsm->SemiMajorAxisAccuracy;
    bs->SemiMinorAxisAccuracy = bsm->SemiMinorAxisAccuracy;
    bs->SemiMajorAxisOrientation = bsm->SemiMajorAxisOrientation;
    bs->TransmissionState = bsm->TransmissionState;
    bs->Speed = bsm->Speed;
    bs->Heading_degrees = bsm->Heading_degrees;
    bs->SteeringWheelAngle = bsm->SteeringWheelAngle;
    bs->AccelLon_cm_per_sec_squared = bsm->AccelLon_cm_per_sec_squared;
    bs->AccelLat_cm_per_sec_squared = bsm->AccelLat_cm_per_sec_squared;
    bs->AccelVert_two_centi_gs = bsm->AccelVert_two_centi_gs;
    bs->AccelYaw_centi_degrees_per_sec = bsm->AccelYaw_centi_degrees_per_sec;
    bs->brakes = bsm->brakes;
    bs->VehicleWidth_cm = bsm->VehicleWidth_cm;
    bs->VehicleLength_cm = bsm->VehicleLength_cm;
    bs->events = bsm->events;
}

void SaeApplication::basicFilterAndSafetyChecks(int l2SrcAddr, double distFromRV){

    // if desired, qits can perform some filtering of packets
    // based on distance, TTC, direction of RV, etc.
    // One goal is to achieve some target RX rate
    if (configuration.enableL2Filtering && !isRxSim) {
        if (appVerbosity >= 5) {
            std::cout << "L2 ID is " << l2SrcAddr << std::endl;
        }
        auto remote_bsm = reinterpret_cast<bsm_value_t *>(threadMc->j2735_msg);
        if (hostMc == nullptr) {
            try {
                hostMc = std::make_shared<msg_contents>();
            } catch (std::bad_alloc & e) {
                cerr << "Error: Create Host bsm failed!" << endl;
                return;
            }
        }
        if (hostMc->abuf.head == NULL || hostMc->abuf.size == 0) {
            abuf_alloc(&hostMc->abuf, ABUF_LEN, ABUF_HEADROOM);
            initMsg(hostMc);
        } else {
            abuf_reset(&hostMc->abuf, ABUF_HEADROOM);
        }

        fillBsm(reinterpret_cast<bsm_value_t *>(hostMc->j2735_msg));
        std::shared_ptr<rv_specs> rvsp = std::make_shared<rv_specs>(this->l2RvMap[l2SrcAddr]) ;
        if(rvsp == nullptr){
            try {
                rvsp = std::make_shared<rv_specs>();
                rvsp->distFromRV = distFromRV;
            } catch (std::bad_alloc & e) {
                cerr << "Error: Create rv specs failed!" << endl;
                return;
            }
        }
        fill_RV_specs(hostMc.get(), threadMc.get(), rvsp.get());
        if (appVerbosity > 5) {
            print_rvspecs(rvsp.get());
        }
        this->updateL2RvMap(l2SrcAddr,rvsp.get());
    }
}

/*
 * This function will perform various steps from the reception of the packet
 * to filtering/safety checks, to decoding, to consistency, relevancy, and
 * verification. Depending on QITS configuration settings, QITS will accept
 * certain types of packets (e.g., bsms, wsas, signed or unsigned, etc.).
 * QITS may be configured to verify certain packets under certain configurations as well.
 */
int SaeApplication::receive(const uint8_t index, const uint16_t bufLen) {
    const auto i = index;
    int ret = 0;
    int packet_len = 0;
    wsmp_data_t *wsmpp;
    uint8_t sourceMacAddr[CV2X_MAC_ADDR_LEN];
    int macAddrLen = CV2X_MAC_ADDR_LEN;
    /* Congestion Control Parameters */
    CongestionControlData congestionControlData_;
    logData writelog_data = {};
    uint32_t l2SrcAddr = 0;
    uint64_t timestamp = 0;
    std::thread::id tid = std::this_thread::get_id();
    std::stringstream ss;
    ss << std::this_thread::get_id();
    int thrId = (int)std::stoul(ss.str());
    uint32_t psid = 0;
    bool signedPacket = false;
    double distFromRV = 0.0;

    // make sure that the thread's msg content struct is initialized
    if (threadMc == nullptr) {
        if (ldm == nullptr) {
            // allocate a new one since ldm is not active
            threadMc = std::make_shared<msg_contents>();
        } else {
            // use a ldm-provided msg contents struct for rx and decoding
            uint32_t ldmIndex = this->ldm->getFreeBsmSlotIdx();
            threadMc = this->ldm->bsmContents[ldmIndex];
        }
    }
    if (threadMc->abuf.head == NULL || threadMc->abuf.size == 0) {
        abuf_alloc(&threadMc->abuf, bufLen, ABUF_HEADROOM);
        initMsg(threadMc, true);
    } else {
        abuf_reset(&threadMc->abuf, ABUF_HEADROOM);
        if(threadMc->j2735_msg){
            memset(threadMc->j2735_msg, 0, sizeof(bsm_value_t));
        }
    }

    // receive packet and log reception time if desired
    if (isRxSim)
    {
        sem_wait(&rx_sem);
        ret = simReceive->receive(threadMc->abuf.data, bufLen-ABUF_HEADROOM);
        sem_post(&rx_sem);
        packet_len = ret;
    }
    else {
        sem_wait(&rx_sem);
        ret = radioReceives[index].receive(threadMc->abuf.data, bufLen-ABUF_HEADROOM,
                            sourceMacAddr, macAddrLen);

        if(configuration.appVerbosity > 3){
            rxCount++;
            gettimeofday(&endRxIntervalTime, NULL);
            time_t startTime = startRxIntervalTime.tv_sec;
            if (rxCount > 0 ){
                gettimeofday(&endRxIntervalTime, NULL);
                if((endRxIntervalTime.tv_sec-startTime) == 1){
                    cout << "Dur(ms): " << (endRxIntervalTime.tv_sec-startTime) ;
                    cout << ", messages in duration and msg/sec is: " << rxCount << "\n";
                    rxCount = 0;
                    startRxIntervalTime = endRxIntervalTime;
                }
            }
        }
        sem_post(&rx_sem);
    }

    // actual moment that a packet has been received
    timestamp = timestamp_now();
    // Make sure packet is successfully received
    if (ret < MIN_PACKET_LEN || ret > MAX_PACKET_LEN || threadMc == nullptr) {
        if (appVerbosity > 4) {
            if (ret < 0) {
                printf("Receive returned with error.\n");
            } else if (ret > 0 && ret < MIN_PACKET_LEN) {
                printf(
                "Dropping packet with %d bytes. Needs to be at least %d bytes.\n",
                        ret, MIN_PACKET_LEN);
            } else if (ret > 0 && ret >= MAX_PACKET_LEN) {
                printf(
                "Dropping packet with %d bytes. Needs to be less than %d bytes.\n",
                        ret, MAX_PACKET_LEN);
            }
            // if ret is 0, then polling timed out
        }
        if (ret != 0) {
            rxFail++;
            if (qMon){
                qMon->tData[tid].rxFails++;
            }
        }
        return -1;
    }else{
        if(configuration.RVTransmitLossSimulation){
            if((rxFail+rxSuccess) % 50 == 0){
                std::cout << "Lost " << totalSimLossPkts << " packets out of " << (rxFail+rxSuccess) <<
                    " pkts \n";
                std::cout << "Should be about " << configuration.RVTransmitLossSimulation <<"\n";
            }
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            srand(ts.tv_sec * 1000000000LL + ts.tv_nsec);
            if(rand()%100 <= configuration.RVTransmitLossSimulation){
                totalSimLossPkts++;
                rxFail++;
                if (qMon){
                    qMon->tData[tid].rxFails++;
                }
                return -1;
            }
        }
        rxSuccess++;
        if (qMon)
        {
            qMon->tData[tid].totalRx++;
        }

    }
    if(!isRxSim){
        l2SrcAddr = radioReceives[index].msgL2SrcAdrr;
    }

    // needs to be done for data pointer to not override tail pointer
    threadMc->abuf.tail = threadMc->abuf.data+ret;

    if (appVerbosity > 7) {
        struct timeval currTime;
        gettimeofday(&currTime, NULL);
        std::cout << "L2 ID is " << l2SrcAddr << std::endl;
        std::cout << "RX Time is: " << currTime.tv_sec << "s and ";
        std::cout << " " << currTime.tv_usec << " microsec\n";
        printf("\n 2) Full rx packet with length %d\n", ret);
        print_buffer((uint8_t*)threadMc->abuf.data, ret);
        printf("\n");
    }

    // Decode packet as WSMP Packet and IEEE 1609.2 Header
    // If the packet is unsigned, this will go through completely
    // Otherwise, it will return after processing IEEE 1609.2 header
    ret = decode_msg(threadMc.get());

    if(threadMc.get()->wsmp)
    {
        // check psid if wsmp header was decoded properly
        wsmpp = (wsmp_data_t*)(threadMc.get()->wsmp);
        psid = wsmpp->psid;
    }
    // Determine if we are expecting signed packet or not and process accordingly
    if (this->configuration.enableSecurity) {
        // check if the message is signed/encrypted IEEE1609.2 content.
        if (ret == DECODE_SIGNED) { // message is secured, so additional steps will happen
            signedPacket = true;

#ifdef AEROLINK
            ret = decodeAndVerify(threadMc.get(), l2SrcAddr, index, timestamp);
#else
            ret = DECODE_FAIL;
            if(appVerbosity > 3){
                printf("Cannot decode and verify this signed packet\n");
            }
            decFail++;
#endif
        } else if (ret == DECODE_SUCCESS) { // This packet is an unsigned packet and decoded properly
            // here we need to check option for processing both unsigned/signed packets
            if (!configuration.acceptAll) {
                if (appVerbosity > 3)
                    printf("Error in decoding unsigned packet - security enabled.\n");
                decFail++;
                ret = -1;
            } else {
                if (appVerbosity > 3)
                    printf("Decoded unsigned packet successfully.\n");
                // process WSA and other WSMP packets
                wsmpp = (wsmp_data_t *)threadMc->wsmp;
                if(wsmpp){
                    if (MsgType == MessageType::WSA && wsmpp->psid == PSID_WSA) {
#ifdef WITH_WSA
                    ret = decode_as_wsa(threadMc.get());
                    if (!ret && threadMc->wra) {
                        ret = onReceiveWra(
                                static_cast<RoutingAdvertisement_t*>(threadMc->wra),
                                sourceMacAddr, macAddrLen);
                    }
#endif
                    }else { // attempt to decode the rest of the packet as a j2735 msg
                        ret = decode_as_j2735(threadMc.get());
                    }
                }
            }
        } else {
            if (appVerbosity > 3)
                printf("Unexpected error in decoding packet\n");
            decFail++;
            ret = DECODE_FAIL;
        }
    } else {
        // determine if unsigned packet decoded properly
        switch (ret) {
            case DECODE_SUCCESS:
                if (appVerbosity > 3)
                    printf("Successful unsigned packet decode\n");
                ret = DECODE_SUCCESS;
                wsmpp = (wsmp_data_t *)threadMc->wsmp;
                if(wsmpp){
#ifdef WITH_WSA
                    // handle WSA message
                    if (MsgType == MessageType::WSA && wsmpp->psid == PSID_WSA) {
                        if (threadMc->wra) {
                            ret = onReceiveWra(
                                static_cast<RoutingAdvertisement_t*>(threadMc->wra),
                                sourceMacAddr, macAddrLen);
                        }
                    }
#endif

                    // calculate distance from RV for logging, filter, safety checks
                    if(threadMc->j2735_msg != nullptr && (wsmpp->psid == PSID_BSM ||
                                configuration.overridePsidCheck)){
                        bsm_value_t *bsm = (bsm_value_t*)(threadMc->j2735_msg);
                        double rvLat = bsm->Latitude / 10000000.0;   // in degrees
                        double rvLon = bsm->Longitude / 10000000.0;  // in degrees
                        double hvLatitude = 0.0;
                        double hvLongitude = 0.0;
                        if(kinematicsReceive && appLocListener_ && hvLocationInfo){
                            if(ApplicationBase::positionOverride){
                                hvLatitude = configuration.overrideLat;
                                hvLongitude = configuration.overrideLong;
                            }
                            else
                            {
                                {
                                    lock_guard<mutex> lk(hvLocUpdateMtx);
                                    hvLatitude = hvLocationInfo->getLatitude();
                                    hvLongitude = hvLocationInfo->getLongitude();
                                }
                            }
                        }
                        distFromRV = bsmCompute2dDistance(hvLatitude, hvLongitude, rvLat, rvLon);
                        if(this->configuration.enableDistanceLogs){
                            if(hvLatitude != 0.0 && hvLongitude != 0.0)
                            {
                               writelog_data.distFromRV = distFromRV;
                            }
                        }

                        if(configuration.fakeRVTempIds){
                           fakeTmpId = fakeTmpId % configuration.totalFakeRVTempIds;
                           bsm->id = fakeTmpId;
                           fakeTmpId++;
                        }
                        // perform operations on the message if it is an unsigned bsm
                        basicFilterAndSafetyChecks(l2SrcAddr, distFromRV);
                        fillLoggingData(bsm, &writelog_data.bs);
                    }
                }
                break;
            case DECODE_SIGNED:
                if (appVerbosity > 3)
                    printf("Error in decoding packet. Expecting unsigned packet.\n");
                decFail++;
                ret = DECODE_FAIL;
                break;
            case DECODE_FAIL:
            default:
                if (appVerbosity > 3)
                    printf("Error in decoding unsigned packet\n");
                decFail++;
                ret = DECODE_FAIL;
                break;
        }
    }
    // synchronous post processing steps including logging and congestion control
    if(!(this->configuration.enableAsync))
    {
        if (ret == DECODE_SUCCESS)
        {
            sem_wait(&this->log_sem);
            totalRxSuccessPerSecond++;
            sem_post(&this->log_sem);
            if (MsgType == MessageType::BSM &&
                 (psid == PSID_BSM || configuration.overridePsidCheck)){
                if(this->configuration.enableCongCtrl &&
                    this->congCtrlInitialized && !(this->configuration.enableAsync) )
                {
                    /* If congestion control is enabled, we will pass the contents
                    of the decoded/verified BSM to the cong ctrl library */
                    bsm_value_t* rvBsm = (bsm_value_t*)(threadMc.get()->j2735_msg);
                    if(configuration.fakeRVTempIds){
                       fakeTmpId = fakeTmpId % configuration.totalFakeRVTempIds;
                       rvBsm->id = fakeTmpId;
                       fakeTmpId++;
                    }
                    unsigned int rvTmpId = rvBsm->id;
                    congestionControlManager->addCongestionControlData(
                        rvTmpId,rvBsm->Latitude/10000000.0,
                        rvBsm->Longitude / 10000000.0, rvBsm->Heading_degrees * 0.0125,
                        rvBsm->Speed / (250/18), rvBsm->timestamp_ms,
                        rvBsm->MsgCount);
                }
                if (appVerbosity > 2)
                {
                    printf("Decoded BSM Summary: \n");
                    print_summary_RV(threadMc.get());
                }
            }

            if (qMon)
            {
                // log number of received packets per msg type
                if(configuration.overridePsidCheck){
                    // we override the PSID and treat this packet as a BSM
                    qMon->tData[tid].rxBSMs++;
                }else{
                    switch(psid){
                        case PSID_BSM:
                            qMon->tData[tid].rxBSMs++;
                            if(!signedPacket){
                                qMon->tData[tid].rxUnsignedBSMs++;
                            }else{
                                qMon->tData[tid].rxSignedBSMs++;
                            }
                            break;
                        case PSID_SPAT:
                            qMon->tData[tid].rxSPATs++;
                            if(!signedPacket){
                                qMon->tData[tid].rxUnsignedSPATs++;
                            }else{
                                qMon->tData[tid].rxSignedSPATs++;
                            }
                            break;
                        case PSID_MAP:
                            qMon->tData[tid].rxMAPs++;
                            if(!signedPacket){
                            qMon->tData[tid].rxUnsignedMAPs++;
                            }else{
                                qMon->tData[tid].rxSignedMAPs++;
                            }
                            break;
                    }
                }
            }
        }

        // check if valid msg contents pointer and if BSM for additional logging
        if (!isRxSim && threadMc){
            if ((psid == PSID_BSM || configuration.overridePsidCheck) &&
               threadMc.get()->j2735_msg && radioReceives.size() > index) {
                uint64_t monotonicTime = radioReceives[index].latestTxRxTimeMonotonic();
                uint8_t cbr = radioReceives[index].getCBRValue();
                // write the log here for this tx now. using tx timestamp made before sendto
                ApplicationBase::writeLog(index, l2SrcAddr,
                        false, TransmitType::SPS,
                        (ret >= 0) ? true : false, timestamp, psid,
                        monotonicTime, 0.0, 0, 0, cbr, &writelog_data.bs,
                        writelog_data.distFromRV, 0, txInterval,
                        configuration.enableCongCtrl, congCtrlInitialized,
                        writeMutexCvSae);
                if (enableDiagLog_) {
                    DiagLogData logData = {0};
                    logData.validPkt = (ret >= 0) ? true : false;
                    logData.currTime = timestamp;
                    logData.cbr = cbr;
                    logData.monotonicTime = monotonicTime;
                    logData.txInterval = txInterval;
                    logData.enableCongCtrl = configuration.enableCongCtrl;
                    logData.monotonicTime = congCtrlInitialized;
                    diagLogPktTxRx(false, TransmitType::SPS, &logData, &writelog_data.bs);
                }
            }
        }
    }
    return ret;
}

int SaeApplication::receive(const uint8_t index, const uint16_t bufLen,
                     const uint32_t ldmIndex) {
    // use the ldm-provide message contents struct for rx and decoding
    threadMc = this->ldm->bsmContents[ldmIndex];
    int ret = receive(index, bufLen);
    if (ret >= 0) {
        if (threadMc->j2735_msg != nullptr) {
            auto bsm = reinterpret_cast<bsm_value_t *>(threadMc->j2735_msg);
            this->ldm->setIndex((uint32_t)bsm->id, ldmIndex, nullptr);
        }
    }
    return ret;
}

// fill parameters to prepare for consistency, relevancy, misbehavior checks
void SaeApplication::prepareForSecurityChecks(bsm_value_t* bsm, SecurityOpt_t* sopt){
    // set the hv kinematics for consistency, relevancy, mbd checks
    if(hvLocationInfo){
        {
            lock_guard<mutex> lk(hvLocUpdateMtx);
            sopt->hvKine.latitude = (hvLocationInfo->getLatitude() * 10000000);
            sopt->hvKine.longitude = (hvLocationInfo->getLongitude() * 10000000);
            sopt->hvKine.elevation = (hvLocationInfo->getAltitude() * 10);
        }
    }
    // set the rv kinematics for consistency, relevancy, mbd checks
    sopt->rvKine.latitude = bsm->Latitude;
    sopt->rvKine.longitude = bsm->Longitude;
    sopt->rvKine.elevation = bsm->Elevation;
    if (appVerbosity > 7) {
        std::cout << "HV Latitude, longitude, elevation from packet: " <<
              sopt->hvKine.latitude << ", " << sopt->hvKine.longitude << ", " <<
              sopt->hvKine.elevation << "\n";
        std::cout << "RV Latitude, longitude, elevation from packet: " <<
              bsm->Latitude << ", " << bsm->Longitude << ", " <<  bsm->Elevation << "\n";
        std::cout << "Sopt: Latitude, longitude, elevation from packet: " <<
               sopt->rvKine.latitude << ", " << sopt->rvKine.longitude  << ", "
               << sopt->rvKine.elevation  << "\n";
    }
    // misbehavior detection parameters
        asyncMbdStat = nullptr;
    if (configuration.enableMbd) {
        sopt->enableMbd = true;
        enableMbd = true;
        sopt->rvKine.id = bsm->id;
        sopt->rvKine.dataType = PSID_BSM;
        sopt->rvKine.msgCount = bsm->MsgCount;
        sopt->rvKine.speed = bsm->Speed;
        sopt->rvKine.heading = bsm->Heading_degrees;
        sopt->rvKine.longitudeAcceleration = bsm->AccelLon_cm_per_sec_squared;
        sopt->rvKine.latitudeAcceleration = bsm->AccelLat_cm_per_sec_squared;
        sopt->rvKine.yawAcceleration = bsm->AccelYaw_centi_degrees_per_sec;
        sopt->rvKine.brakes = (uint16_t)bsm->brakes.word;
    }
}


#ifdef AEROLINK
static void AsyncCallbackFunction (AEROLINK_RESULT returnCode,
    void *userData)
{
    // need to make sure this function is completed before closing qits

    std::unique_lock<std::mutex> lock(AsyncMtx);
    asyncCbData_t* cb_data = (asyncCbData_t*) userData;
    if(cb_data == nullptr){
        printf("cb_data is a null pointer\n");
        return;
    }
    if(returnCode != WS_SUCCESS){
        cb_data->verifSuccess = false;
        cb_data->AsyncState = VERIF_DONE;
        asyncCallbackVerifFail++;
    }
    else
    {
        cb_data->verifSuccess = true;
        cb_data->AsyncState = VERIF_DONE;
        gettimeofday(&currTime, NULL);
        cb_data->endLatencyTime = (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
        if(cb_data->asyncVerifStat != nullptr ){
            cb_data->asyncVerifStat->timestamp =
                (cb_data->endLatencyTime)-(LogstartTime);
            cb_data->asyncVerifStat->verifLatency =
                (cb_data->endLatencyTime)-(cb_data->startLatencyTime);
        }
        asyncCallbackVerifSuccess++;
    }
    if(shared_index < PP_BUFFER_MAX_SIZE)
    {
        if(begin_flag)
        {
            begin_flag = false;
            start_index = shared_index;
        }
        // this part may need to be protected
        PostProcessingCbData[shared_index] = cb_data->indexToData;
        shared_index++;
    }
    else
    {
        // Post processing cleanup function will update the index and this
        // index will update the start index also based on this flag
        buffer_full = true;
        shared_index = 0;
        begin_flag = true;
    }
    // wake up post processing thread
    sem_post(&verificationSem);
}
#endif


//Function to print out running verification stats
void SaeApplication::printStats(std::thread::id thrId, int secVerbosity){
    //sem_wait(&ApplicationBase::log_sem);
    gettimeofday(&currTime, NULL);
    double currTimeStamp =
            (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);

    if(asyncVerifFail % VERIF_STAT_BATCH_SIZE >= 0 &&
        asyncVerifFail % VERIF_STAT_BATCH_SIZE <= ASYNC_BATCH_SIZE &&
        asyncVerifFail > 0 &&
        asyncVerifFail > (prevVerifFail + ASYNC_BATCH_SIZE)){
        if(secVerbosity > 4)
            fprintf(stdout, "VerifSuccess: %d; VerifFail: %d\n",
                    asyncVerifSuccess.load(), asyncVerifFail.load());
        prevVerifFail = asyncVerifFail;
    }
    // batch stats reporting (per 2500 successful verifications)
    if(asyncVerifSuccess % VERIF_STAT_BATCH_SIZE >= 0 &&
        asyncVerifSuccess % VERIF_STAT_BATCH_SIZE <= ASYNC_BATCH_SIZE &&
        asyncVerifSuccess > 0 &&
        asyncVerifSuccess > (prevVerifSuccess + ASYNC_BATCH_SIZE)){
            double dur = currTimeStamp-prevBatchTimeStamp;
            double delta = asyncVerifSuccess-prevVerifSuccess;
            double rate = delta/dur;
            // minimum time so far per 2500*N verifications
            minBatchTime = std::min(minBatchTime, dur);
            // running time per 2500*N verifications
            maxBatchTime = std::max(maxBatchTime, dur);
            // average time per 2500*N verifications
            avgBatchTime = (avgBatchTime+dur)/2.0;

            // overall average batch rate
            // dealing with initializing variables for first verification
            if(minBatchTime <= 0.0){
                minBatchTime = dur;
                avgBatchTime = dur;
            }

            std::stringstream ss;
            ss << thrId;
            int tid = (int)std::stoul(ss.str());
            if (resFileLogging){
                asyncLogStat->tid = tid;
                asyncLogStat->asyncVerifSuccess = asyncVerifSuccess;
                asyncLogStat->currTimeStamp = currTimeStamp;
                asyncLogStat->rate = rate;
                asyncLogStat->dur = dur;
            }
            else{
                // logging for batch verif stats
                fprintf(stdout, "ThreadID: 0x%08x; ", tid);
                fprintf(stdout, "TotalSuccessfulVerifs: %d;\n", asyncVerifSuccess.load());
                fprintf(stdout, "BatchVerifRate: %fk VHz; ", rate);
                fprintf(stdout, "BatchTimeStep: %fms;\n", dur);
                fprintf(stdout, "MinBatchTime: %fms; ", minBatchTime);
                fprintf(stdout, "MaxBatchTime: %fms; ", maxBatchTime);
                fprintf(stdout, "AvgBatchTime: %fms;\n", avgBatchTime);
                // logging for individual verif stats - includes ITS overhead
                if(secVerbosity > 1){
                    fprintf(stdout, "CurrTime: %fms; ", currTimeStamp);
                    fprintf(stdout, "PrevBatchTime: %fms;\n", prevBatchTimeStamp);
                }
                fprintf(stdout, "\n");
            }
            prevVerifSuccess = asyncVerifSuccess;
            // get latest time stamp because print statements cause delay
            gettimeofday(&currTime, NULL);
            prevBatchTimeStamp =
                        (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);

    }
    // get latest time stamp because print statements cause delay
    gettimeofday(&currTime, NULL);
    prevTimeStamp =
                    (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
    //sem_post(&ApplicationBase::log_sem);
}

void SaeApplication::AsyncPostProcessing(bool overridePsidCheck, bool enableCongCtrl,
    bool enableMisbehavior, void* asyncSecService,
    shared_ptr<ICongestionControlManager> congestionControlManager,
    shared_ptr<QMonitor> qMon, int secVerbosity, RadioReceive* radioReceive)
{
    uint64_t monotonicTime = 0;
    uint8_t cbr = 0;
    bool congCtrlInitialized = false;
    if(congestionControlManager && enableCongCtrl){
        congCtrlInitialized = true;
    }
    thread::id thrId = std::this_thread::get_id();
    while((!exitAsync)){
        sem_wait(&verificationSem);

        int i = 0 ;
        for(i = start_index; PostProcessingCbData[i]!=0;++i)
        {
            if(exitAsync){
                return;
            }
            if(asyncCbData[PostProcessingCbData[i]].AsyncState != PP_DONE){
                if((asyncCbData[PostProcessingCbData[i]].verifSuccess))
                {
                    asyncVerifSuccess++;
                    asyncCbData[PostProcessingCbData[i]].AsyncState = PP_DONE;
                    if(asyncCbData[PostProcessingCbData[i]].psid == PSID_BSM){
                        if(enableCongCtrl &&
                            (congestionControlManager != NULL))
                        {
                            congestionControlManager->addCongestionControlData(
                                asyncCbData[PostProcessingCbData[i]].asyncBs.id,
                                (asyncCbData[PostProcessingCbData[i]].asyncBs.Latitude)/10000000.0,
                                (asyncCbData[PostProcessingCbData[i]].asyncBs.Longitude)/10000000.0,
                                asyncCbData[PostProcessingCbData[i]].asyncBs.Heading_degrees * 0.0125,
                                asyncCbData[PostProcessingCbData[i]].asyncBs.Speed * (250/18),
                                asyncCbData[PostProcessingCbData[i]].asyncBs.timestamp_ms,
                                asyncCbData[PostProcessingCbData[i]].asyncBs.MsgCount);
                        }
                        #ifdef AEROLINK
                        if(enableMisbehavior){
                            if(asyncSecService){
                                // start time for mbd here
                                AerolinkSecurity* tmpAeroSecurity =
                                    static_cast<AerolinkSecurity*>(asyncSecService);
                                    AEROLINK_RESULT ret = tmpAeroSecurity->mbdCheck(
                                        &asyncCbData[PostProcessingCbData[i]].rvKine,
                                        asyncCbData[PostProcessingCbData[i]].misbehaviorStat,
                                        (SecuredMessageParserC*)
                                            asyncCbData[PostProcessingCbData[i]].msgParseContext);
                                if (ret == WS_ERR_MISBEHAVIOR_DETECTED){
                                    asyncMbdDetected++;
                                }else{
                                    asyncMbdUndetected++;
                                }
                            }
                        }
                        #endif
                    }
                    // log number of received packets per msg
                    if(qMon){
                        if(overridePsidCheck){
                            // we override the PSID and treat this packet as a BSM
                            qMon->tData[thrId].rxBSMs++;
                        }else{
                            switch(asyncCbData[PostProcessingCbData[i]].psid){
                                case PSID_BSM:
                                    qMon->tData[thrId].rxBSMs++;
                                    qMon->tData[thrId].rxSignedBSMs++;
                                    break;
                                case PSID_SPAT:
                                    qMon->tData[thrId].rxSPATs++;
                                    qMon->tData[thrId].rxSignedSPATs++;
                                    break;
                                case PSID_MAP:
                                    qMon->tData[thrId].rxMAPs++;
                                    qMon->tData[thrId].rxSignedMAPs++;
                                    break;
                            }
                        }
                    }
                }
                else if(!(asyncCbData[PostProcessingCbData[i]].verifSuccess))
                {
                    asyncVerifFail++;
                    asyncCbData[PostProcessingCbData[i]].AsyncState = PP_DONE;
                }
                if (asyncCbData[PostProcessingCbData[i]].psid == PSID_BSM ||
                    overridePsidCheck)
                {
                    if(radioReceive){
                        monotonicTime = radioReceive->latestTxRxTimeMonotonic();
                        cbr = radioReceive->getCBRValue();
                    }
                    // write the log here for this tx now. using tx timestamp made before sendto
                    ApplicationBase::writeLog(asyncCbData[PostProcessingCbData[i]].msg_index,
                        asyncCbData[PostProcessingCbData[i]].l2SrcAddr, false, TransmitType::SPS,
                        asyncCbData[PostProcessingCbData[i]].verifSuccess,
                        asyncCbData[PostProcessingCbData[i]].timestamp, PSID_BSM,
                        monotonicTime, 0.0, 0, 0, cbr,
                        &(asyncCbData[PostProcessingCbData[i]].asyncBs),
                        asyncCbData[PostProcessingCbData[i]].distFromRV,
                        asyncCbData[PostProcessingCbData[i]].RVsInRange,
                        asyncCbData[PostProcessingCbData[i]].txInterval,
                        enableCongCtrl, congCtrlInitialized, writeMutexCvSae);
                    if (enableDiagLog_) {
                        DiagLogData logData = {0};
                        logData.validPkt = asyncCbData[PostProcessingCbData[i]].verifSuccess;
                        logData.currTime = asyncCbData[PostProcessingCbData[i]].timestamp;
                        logData.cbr = cbr;
                        logData.monotonicTime = monotonicTime;
                        logData.txInterval = asyncCbData[PostProcessingCbData[i]].txInterval;
                        logData.enableCongCtrl = enableCongCtrl;
                        logData.monotonicTime = congCtrlInitialized;
                        diagLogPktTxRx(false, TransmitType::SPS, &logData,
                            &(asyncCbData[PostProcessingCbData[i]].asyncBs));
                    }
                }
            }
        }

        // Since the thread is running in while loop updating the
        // loop index to the last successful value
        start_index = i;
        if(buffer_full)
        {
            postprocessing_cleanup();
        }
        if(secVerbosity > 0 ){
            printStats(thrId, secVerbosity);
        }
    }
}

void SaeApplication::postprocessing_cleanup()
{
    std::unique_lock<std::mutex> lock(AsyncMtx);
    for(int j =0;j<PP_BUFFER_MAX_SIZE;j++)
    {
        //If there is any pending elements to be post processed which is
        // currently in VERIFY State , find that entry
        if((asyncCbData[PostProcessingCbData[j]].AsyncState != PP_DONE) ||
        ( asyncCbData[PostProcessingCbData[j]].AsyncState != FREE))
        {
            shared_index = j ;
            // This will make the post processing loop to
            //run from this particular index
        }
        else
        {
            asyncCbData[PostProcessingCbData[j]].AsyncState = FREE;
        }
    }
}

// thread function to PostProcessingThread
void SaeApplication::PostProcessingThread()
{
    asyncThreads.push_back(std::thread(&SaeApplication::AsyncPostProcessing, this,
        overridePsidCheck, enableCongCtrl,
        enableMbd, asyncSecService,
        congestionControlManager, qMonPtr, secVerbosity,
        radioReceivePtr));
}

#ifdef AEROLINK
// when the packet is signed, we need to perform extra steps to extract the ieee 1609.2 header.
// Afterward, fields needed for consistency, relevancy, misbehavior detection
// and verification are extracted from the decoded payload and provided to Aerolink.
// Then security checks and verification will occur.
int SaeApplication::decodeAndVerify(msg_contents* mc, int l2SrcAddr,
        uint8_t index, uint64_t timestamp) {
    int ret = DECODE_FAIL;
    std::thread::id tid = std::this_thread::get_id();
    wsmp_data_t *wsmpp;
    uint8_t sourceMacAddr[CV2X_MAC_ADDR_LEN];
    int macAddrLen = CV2X_MAC_ADDR_LEN;
    double distFromRV = 0.0;
    // Prepare for verification
    SecurityOpt sopt;
    sopt.enableAsync = this->configuration.enableAsync;
    sopt.setGenLocation = this->configuration.setGenLocation;
    sopt.enableConsistency = this->configuration.enableConsistency;
    sopt.enableRelevance = this->configuration.enableRelevance;
    sopt.enableEnc  = this->configuration.enableEncrypt;
    sopt.secVerbosity = this->configuration.secVerbosity;
    if(!isRxSim){
        sopt.priority= (uint8_t)radioReceives[0].priority;
    }
    else{
        sopt.priority = 1;
    }
    uint32_t dot2HdrLen;
    uint8_t const *payload = NULL;
    uint32_t       payloadLen = 0;
    SecuredMessageParserC* smp = nullptr;
    if(SecService){
        SecurityService* tmpSecService = SecService.get();
        if(sopt.enableAsync){
            if(tmpSecService){
                AerolinkSecurity* tmpAeroSecurity =
                    static_cast<AerolinkSecurity*>(tmpSecService);
                if(async_index < (SHARED_BUFFER_MAX_SIZE / 5)){
                    async_index = SHARED_BUFFER_MAX_SIZE-1;
                    begin_flag = true;
                }
                smp = (SecuredMessageParserC*)(asyncCbData[async_index].msgParseContext);
            }
        }
        // extract the PDU from the secured packet
        ret = SecService->ExtractMsg(
            smp,
            sopt,
            (uint8_t*)mc->l3_payload,mc->l3_payload_len,
            payload, payloadLen,
            dot2HdrLen);
    }else{
        // security service no longer valid
        ret = DECODE_FAIL;
    }
    if (ret == DECODE_FAIL) {
        printf("Error in extracting security header from signed packet.\n");
        asyncVerifFail++;
        if (qMon)
        {
            qMon->tData[tid].secFails++;
        }
        return ret;
    }

    // ieee header is 3 bytes long typically
    abuf_pull(&mc->abuf, dot2HdrLen - IEEE_1609_2_HDR_LEN);
    mc->l3_payload=mc->l3_payload+dot2HdrLen;
    mc->payload_len=payloadLen;
    wsmpp = (wsmp_data_t *)mc->wsmp;
    if (appVerbosity > 4) {
        printf("Total security header length is: %d bytes\n",
                dot2HdrLen);
        printf("payload length is %d bytes\n", ret);
        if(wsmpp){
            printf("wsmpp psid is %d\n", wsmpp->psid);
        }
    }
    // decode the j2735 packet and access the lat/long if required for security actions
    if(MsgType != MessageType::WSA){
        ret = decode_as_j2735(mc);
        // here the secure header was extracted properly, packet decoded incorrectly
        if (ret == DECODE_FAIL) {
            if (appVerbosity > 3)
                printf("Error in decoding unsigned packet - security enabled.\n");
            decFail++;
            if (qMon)
            {
                qMon->tData[tid].decodeFails++;
            }
            return -1;
        }

        // as an example, bsms require passing lat and long to
        // the aerolink library for consistency and relevancy checks
        // since they are not included in the ieee 1609.2 header according to spec
        if(wsmpp){
            if(mc->j2735_msg != nullptr && (wsmpp->psid == PSID_BSM ||
                        configuration.overridePsidCheck)){
                bsm_value_t *bsm = (bsm_value_t*)(mc->j2735_msg);
                if(bsm != nullptr){
                    // calculate distance from RV for logging, filter, safety checks
                    double rvLat = bsm->Latitude / 10000000.0;   // in degrees
                    double rvLon = bsm->Longitude / 10000000.0;  // in degrees
                    double hvLatitude = 0.0;
                    double hvLongitude = 0.0;
                    if(kinematicsReceive && appLocListener_ && hvLocationInfo){
                        if(ApplicationBase::positionOverride){
                            hvLatitude = configuration.overrideLat;
                            hvLongitude = configuration.overrideLong;
                        }
                        else
                        {
                            {
                                lock_guard<mutex> lk(hvLocUpdateMtx);
                                hvLatitude = hvLocationInfo->getLatitude();
                                hvLongitude = hvLocationInfo->getLongitude();
                            }
                        }
                    }
                    distFromRV = bsmCompute2dDistance(hvLatitude, hvLongitude, rvLat, rvLon);
                    if(this->configuration.enableDistanceLogs){
                        if(hvLatitude != 0.0 && hvLongitude != 0.0)
                        {
                            bs.distFromRV = distFromRV;
                        }
                    }

                    // perform operations on the message if it is an unsigned bsm
                    basicFilterAndSafetyChecks(l2SrcAddr, distFromRV);
                    fillLoggingData(bsm, &bs);
                    prepareForSecurityChecks(bsm,&sopt);

                    // SSP Check if the BSM is from a public vehicle with emergency event
                    if((bsm->has_special_extension) && (bsm->vehicleAlerts.lightsUse) &&
                       (bsm->vehicleAlerts.sirenUse) && (bsm->vehicleAlerts.multi) &&
                       (configuration.expectedSspLength))
                    {
                        uint8_t const* ssp = nullptr;
                        ret = SecService->sspCheck(smp,ssp);
                        if (ret == DECODE_FAIL) {
                            if (appVerbosity > 4)
                                printf("Error decoding SSP \n ");
                            return -1;
                        }
                        // First byte of the ssp should be 0x01 as per the specification J2945/J3161
                        // Bits 1-8: Version number. Set to one for this version
                        // of the specification (00000001).
                        if(ssp != nullptr){
                            if(sizeof(ssp) > 1){
                                if(ssp[0] != configuration.expectedSsp[0])
                                {
                                    fprintf(stderr,"Invalid SSP Version Present \n");
                                    return -1;
                                }
                                // Second byte of the ssp should be 0x80 as per the specification J2945/J3161
                                // Bits 9-15: SSP activity bits with bit 9 set to 1
                                // (bit 16 is reserved for future use)(10000000)
                                if(ssp[1] != configuration.expectedSsp[1])
                                {
                                    fprintf(stderr,"Invalid Entity Activity Detected \n");
                                    return -1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // prepare verification statistics logging
    if (configuration.enableVerifStatLog) {
        if(!configuration.enableAsync) {
            if (thrVerifLatencies[tid].size() >= verifStatIdx[tid]) {
                sopt.verifStat = &thrVerifLatencies[tid].at(verifStatIdx[tid]);
            } else {
                verifStatIdx[tid] = 0;
                sopt.verifStat = &thrVerifLatencies[tid].at(verifStatIdx[tid]);
            }
            verifStatIdx[tid]++;
            verifStatIdx[tid]%=thrVerifLatencies[tid].size();
        }
    }else {
        sopt.verifStat = nullptr;
        asyncVerifStat = nullptr;
    }
    // this will need to be measured in post processing thread
    if (configuration.enableMbdStatLog) {
        if(!configuration.enableAsync) {
            if (thrMisbehaviorLatencies[tid].size() >= misbehaviorStatIdx[tid]) {
                sopt.misbehaviorStat =
                    &thrMisbehaviorLatencies[tid].at(misbehaviorStatIdx[tid]);
            } else {
                misbehaviorStatIdx[tid] = 0;
                sopt.misbehaviorStat =
                    &thrMisbehaviorLatencies[tid].at(misbehaviorStatIdx[tid]);
            }
            misbehaviorStatIdx[tid]++;
            misbehaviorStatIdx[tid]%=thrMisbehaviorLatencies[tid].size();
        }
    } else {
        sopt.misbehaviorStat = nullptr;
        asyncMbdStat = nullptr;
    }
    if(configuration.enableVerifResLog){
        if(thrResLoggingValues.find(tid) == thrResLoggingValues.end()){
            std::vector<ResultLoggingStats> tmp_test;
            thrResLoggingValues.insert(std::pair<std::thread::id,
            std::vector<ResultLoggingStats>>(tid, tmp_test));
            for(int i = 0 ; i < configuration.verifResLogSize; i++){
                    ResultLoggingStats tmpVerifResStat = {0};
                    thrResLoggingValues[tid].push_back(tmpVerifResStat);
            }
        }
    }
    // this will need to be measured in post processing thread
    if (configuration.enableVerifResLog) {
        if (thrResLoggingValues[tid].size() >= resultLoggingIdx[tid]) {
            asyncLogStat =
                &thrResLoggingValues[tid].at(resultLoggingIdx[tid]);
        } else {
            resultLoggingIdx[tid] = 0;
            asyncLogStat =
                &thrResLoggingValues[tid].at(resultLoggingIdx[tid]);
        }
        resultLoggingIdx[tid]++;
        resultLoggingIdx[tid]%=thrResLoggingValues[tid].size();
    } else {
        asyncLogStat = nullptr;
    }
    // Verify packet signature ; providing lat/lon from the rx message
    if(!(sopt.enableAsync))
    {
        // returns nonzero value if success, otherwise -1
        ret = SecService->VerifyMsg(sopt);
    }
    else
    {
        if(async_index >= 0)
        {
            asyncCbData[async_index].indexToData = async_index;
            memcpy(&asyncCbData[async_index].asyncBs, &bs, sizeof(bsm_data));
            if(configuration.fakeRVTempIds){
               fakeTmpId = fakeTmpId % configuration.totalFakeRVTempIds;
               asyncCbData[async_index].asyncBs.id = fakeTmpId;
               fakeTmpId++;
            }
            asyncCbData[async_index].msg_index = index;
            asyncCbData[async_index].l2SrcAddr = l2SrcAddr;
            asyncCbData[async_index].timestamp = timestamp;
            asyncCbData[async_index].distFromRV = distFromRV;
            asyncCbData[async_index].psid = PSID_BSM;

            //RVsInRange
            //txInterval
            //call to AsyncVerify
            if((asyncCbData[async_index].AsyncState != VERIF_DONE)){
                SecurityService* tmpSecService = SecService.get();
                if(tmpSecService){
                    AerolinkSecurity* tmpAeroSecurity =
                        static_cast<AerolinkSecurity*>(tmpSecService);
                    ret = tmpAeroSecurity->checkConsistencyandRelevancy(
                        (SecuredMessageParserC*)(asyncCbData[async_index].msgParseContext), sopt);
                    if(ret != DECODE_FAIL && tmpAeroSecurity){
                        asyncCbData[async_index].asyncVerifStat = nullptr;
                        if(configuration.enableVerifStatLog){
                            if(thrVerifLatencies.find(tid) == thrVerifLatencies.end()){
                                std::vector<VerifStats> tmp;
                                thrVerifLatencies.insert(std::pair<std::thread::id,
                                    std::vector<VerifStats>>(tid, tmp));
                                for(int i = 0 ; i < configuration.verifStatsSize; i++){
                                    VerifStats tmpVerifStat = {0};
                                    thrVerifLatencies[tid].push_back(tmpVerifStat);
                                }
                            }
                            if (thrVerifLatencies[tid].size() > verifStatIdx[tid]){
                                asyncCbData[async_index].asyncVerifStat =
                                    &thrVerifLatencies[tid].at(verifStatIdx[tid]);
                            }else{
                                verifStatIdx[tid] = 0;
                                asyncCbData[async_index].asyncVerifStat =
                                    &thrVerifLatencies[tid].at(verifStatIdx[tid]);
                            }
                            verifStatIdx[tid]++;
                            verifStatIdx[tid]%=thrVerifLatencies[tid].size();
                        }
                        asyncCbData[async_index].misbehaviorStat = nullptr;
                        if(configuration.enableMbdStatLog){
                            if(thrMisbehaviorLatencies.find(tid) == thrMisbehaviorLatencies.end()){
                                std::vector<MisbehaviorStats> tmp;
                                thrMisbehaviorLatencies.insert(std::pair<std::thread::id,
                                std::vector<MisbehaviorStats>>(tid, tmp));
                                for(int i = 0 ; i < configuration.mbdStatLogListSize; i++){
                                    MisbehaviorStats tmpMbdStat;
                                    thrMisbehaviorLatencies[tid].push_back(tmpMbdStat);
                                }
                            }
                            if (thrMisbehaviorLatencies[tid].size() > misbehaviorStatIdx[tid]){
                                asyncCbData[async_index].misbehaviorStat =
                                    &thrMisbehaviorLatencies[tid].at(misbehaviorStatIdx[tid]);
                            }else{
                                misbehaviorStatIdx[tid] = 0;
                                asyncCbData[async_index].misbehaviorStat =
                                    &thrMisbehaviorLatencies[tid].at(misbehaviorStatIdx[tid]);
                            }
                            misbehaviorStatIdx[tid]++;
                            misbehaviorStatIdx[tid]%=thrMisbehaviorLatencies[tid].size();
                        }
                        gettimeofday(&currTime, NULL);
                        asyncCbData[async_index].startLatencyTime =
                               (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
                        memcpy(&asyncCbData[async_index].rvKine,
                            &sopt.rvKine, sizeof(Kinematics));
                        // returns nonzero value if success, otherwise -1
                        ret = tmpAeroSecurity->asyncVerify(
                                sopt.rvKine, sopt.misbehaviorStat,
                                (void *)&(asyncCbData[async_index]), sopt.priority,
                                AsyncCallbackFunction,
                                (SecuredMessageParserC*)
                                    asyncCbData[async_index].msgParseContext);
                    }
                }
                async_index--;
            }else{
               async_index--;
            }
        }

    }

    // this will override the actual result for testing purposes
    if(configuration.overrideVerifResult){
        ret = configuration.overrideVerifValue;
    }
    if (ret == DECODE_FAIL) {
        if(!(configuration.enableAsync))
        {
            syncVerifFail++;
        }else{
            asyncVerifFail++;
        }
        if (qMon)
        {
            qMon->tData[tid].secFails++;
        }
        if (appVerbosity > 3)
            printf("Error in verifying secured packet.\n");
        // failure in security ; if flooding detect and mitigate enabled,
        // add this l2 address to rv map
        if (configuration.enableL2FloodingDetect && !isRxSim) {
            if (configuration.floodDetectVerbosity >= 3) {
                std::cout << "L2 ID is " << l2SrcAddr << std::endl;
            }
            auto remote_bsm = reinterpret_cast<bsm_value_t *>(mc->j2735_msg);
            if (hostMc == nullptr) {
                try {
                    hostMc = std::make_shared<msg_contents>();
                } catch (std::bad_alloc & e) {
                    cerr << "Error: Create Host bsm failed!" << endl;
                    return ret;
                }
            }
            if (hostMc->abuf.head == NULL || hostMc->abuf.size == 0) {
                abuf_alloc(&hostMc->abuf, ABUF_LEN, ABUF_HEADROOM);
                initMsg(hostMc);
            } else {
                abuf_reset(&hostMc->abuf, ABUF_HEADROOM);
            }

            fillBsm(reinterpret_cast<bsm_value_t *>(hostMc->j2735_msg));
            std::shared_ptr<rv_specs> rvsp;
            if(l2RvMap.find(l2SrcAddr) == l2RvMap.end()){
                try {
                    rvsp = std::make_shared<rv_specs>();
                } catch (std::bad_alloc & e) {
                    cerr << "Error: Create rv specs failed!" << endl;
                    return ret;
                }
            }else{
                rvsp = std::make_shared<rv_specs>(l2RvMap.at(l2SrcAddr));
            }
            fill_RV_specs(hostMc.get(), mc, rvsp.get());
            if (configuration.floodDetectVerbosity > 5) {
                print_rvspecs(rvsp.get());
            }
            this->updateL2RvMap(l2SrcAddr,rvsp.get());
        }
    } else {
        if(!(configuration.enableAsync))
        {
            syncVerifSuccess++;
        }
        // process WSA and other WSMP packets after verification
        wsmpp = (wsmp_data_t *)mc->wsmp;
        if(wsmpp){
            if (MsgType == MessageType::WSA && wsmpp->psid == PSID_WSA) {
    #ifdef WITH_WSA
                ret = decode_as_wsa(mc);
                if (!ret && mc->wra) {
                    ret = onReceiveWra(
                            static_cast<RoutingAdvertisement_t*>(mc->wra),
                            sourceMacAddr, macAddrLen);
                }
    #endif
            }
        }
    }
    return ret;
}
#endif


/*
 * Initialize the wsmp header, ieee 1609.2 header, and j2735 contents unless WRA/WSA
 */
bool SaeApplication::initMsg(std::shared_ptr<msg_contents> mc, bool isRx) {
    mc->stackId = STACK_ID_SAE;

    if (isRx) {
        // not allocate memory for Rx
        mc->wsmp = nullptr;
        mc->ieee1609_2data = nullptr;
        mc->j2735_msg = nullptr;
        mc->wsa = nullptr;
        if (MsgType == MessageType::BSM) {
            mc->msgId = J2735_MSGID_BASIC_SAFETY;
        } else {
            mc->msgId = (int)WSA_MSG_ID;
        }
    } else {
        mc->wsmp = (wsmp_data_t*) calloc(1, sizeof(wsmp_data_t));
        if (!mc->wsmp) {
            std::cerr << "calloc wsmp failed" << endl;
            return false;
        }

        ((wsmp_data_t*)mc->wsmp)->abp = (abuf_t*) calloc(1, sizeof(abuf_t));

        if (!((wsmp_data_t*)mc->wsmp)->abp)
        {
            free(mc->wsmp);
            mc->wsmp = nullptr;
            std::cerr << "calloc wsmp asnbuf structure failed" << endl;
            return false;
        }

        const auto abuf_ret = abuf_alloc(((wsmp_data_t*)mc->wsmp)->abp,
                        WSMP_ABUF_DEFAULT_SIZE, WSMP_ABUF_DEFAULT_HEADROOM);

        if (abuf_ret != WSMP_ABUF_DEFAULT_SIZE)
        {
            freeMsg(mc);
            std::cerr << "alloc wsmp asn buffer failed" << endl;
            return false;
        }

        mc->ieee1609_2data = malloc(sizeof(ieee1609_2_data));
        if (!mc->ieee1609_2data) {
            freeMsg(mc);
            std::cerr << "alloc ieee1609_2data failed" << endl;
            return false;
        }
        memset(mc->ieee1609_2data, 0, sizeof(ieee1609_2_data));

        if (MsgType == MessageType::BSM) {
            mc->j2735_msg = malloc(sizeof(bsm_value_t));
            if (!mc->j2735_msg) {
                freeMsg(mc);
                std::cerr << "alloc j2735_msg failed" << endl;
                return false;
            }
            memset(mc->j2735_msg, 0, sizeof(bsm_value_t));
            mc->wsa = 0;
            mc->msgId = J2735_MSGID_BASIC_SAFETY;
        } else {
#ifdef WITH_WSA
            mc->wsa = malloc(sizeof(SrvAdvMsg_t));
            if (!mc->wsa) {
                freeMsg(mc);
                std::cerr << "alloc wsa failed" << endl;
                return false;
            }
            memset(mc->wsa, 0, sizeof(SrvAdvMsg_t));

            mc->wra = malloc(sizeof(RoutingAdvertisement_t));
            if (!mc->wra) {
                freeMsg(mc);
                std::cerr << "alloc wra failed" << endl;
                return false;
            }
            memset(mc->wra, 0, sizeof(RoutingAdvertisement_t));

            // set wra into wsa struct, wra be freed along with wsa
            SrvAdvMsg_t* wsa = (SrvAdvMsg_t*)(mc->wsa);
            wsa->body.routingAdvertisement = (RoutingAdvertisement_t*)(mc->wra);
            mc->j2735_msg = 0;
            mc->msgId = (int)WSA_MSG_ID;
#endif
        }
    }

    return true;
}

/*
 * Free the allocated message contents
 */
void SaeApplication::freeMsg(std::shared_ptr<msg_contents> mc) {
    if (mc->wsmp) {
        auto wsmp = (wsmp_data_t*)mc->wsmp;

        if (wsmp->chan_load_ptr) {
            free(wsmp->chan_load_ptr);
            wsmp->chan_load_ptr = nullptr;
        }

        abuf_free(wsmp->abp);
        if (wsmp->abp) {
            free(wsmp->abp);
        }
        free(mc->wsmp);
        mc->wsmp = nullptr;
    }

    if (mc->j2735_msg) {
        free(mc->j2735_msg);
        mc->j2735_msg = nullptr;
    }

    if (mc->ieee1609_2data) {
        free(mc->ieee1609_2data);
        mc->ieee1609_2data = nullptr;
    }
#ifdef WITH_WSA
    // free the wsa struct, wra will be freed if it exists
    if (mc->wsa) {
        free_wsa(mc->wsa);
        mc->wsa = nullptr;
    }
#endif

    abuf_free(&(mc->abuf));
}

/*
 * Fill the contents of an initialized message structure based on msg type
 * Currently, only BSMs or WRA/WSA are supported
 */
void SaeApplication::fillMsg(std::shared_ptr<msg_contents> mc) {
    fillWsmp(static_cast<wsmp_data_t *>(mc->wsmp));
    fillSecurity(static_cast<ieee1609_2_data *>(mc->ieee1609_2data));

    if (MsgType == MessageType::BSM) {
        fillBsm(static_cast<bsm_value_t *>(mc->j2735_msg));
    }

#ifdef WITH_WSA
    if (MsgType == MessageType::WSA) {
        fillWsa(static_cast<SrvAdvMsg_t *>(mc->wsa),
                static_cast<RoutingAdvertisement_t *>(mc->wra));
        wsmp_data_t *wsmpp = (wsmp_data_t *)mc->wsmp;
        wsmpp->psid = PSID_WSA;
    }
#endif
}

/*
 * Fill the wsmp header
 */
void SaeApplication::fillWsmp(wsmp_data_t *wsmp) {
    wsmp->n_header.data = 3;
    wsmp->tpid.octet = 0;
    // use the same service id as the sps flow unless none provided
    if (this->configuration.spsServiceIDs.size()) {
        wsmp->psid = this->configuration.spsServiceIDs[0];
    } else {
        wsmp->psid = PSID_BSM; // default 0x20
    }

    // The content of channel load is not standardized yet, use this IE for padding
    if (this->configuration.padding > 0) {
        if (!wsmp->chan_load_ptr) {
            wsmp->chan_load_ptr = (uint8_t *)malloc(this->configuration.padding);
            if (!wsmp->chan_load_ptr) {
                cerr << "alloc padding failed!" << endl;
            } else {
                wsmp->weid_opts.inc_load_ext = 1;
                wsmp->chan_load_len = this->configuration.padding;
                // fill 0xFF for padding
                memset(wsmp->chan_load_ptr, 0xFF, wsmp->chan_load_len);
            }
        }
    } else {
        wsmp->chan_load_ptr = nullptr;
        wsmp->chan_load_len = 0;
    }
}

int SaeApplication::parseIPv6Addr(const string& str, char *buf, int& bufLen) {
    int i = 0;
    auto pos = 0, prev = 0;

    if (str.empty() or bufLen < 0) {
        cerr << "Input error for parseIPv6Addr!" << endl;
        return -1;
    }

    string ipAddr = str + ":"; /* help parsing */
    do {
        if (i >= bufLen) {
            cerr << "Input IPv6 Address too long!" << endl;
            return -1;
        }
        pos = ipAddr.find(":", prev);
        if (pos == std::string::npos or pos == prev) {
            break;
        }
        string sub = ipAddr.substr(prev, pos - prev);
        if (sub.size() > 4) {
            cerr << "sub string " << sub << " too long!" << endl;
            return -1;
        }
        uint16_t val = stoi(sub, 0, 16);
        buf[i] = (val >> 8);
        buf[i + 1] = (val & 0xFF);
        prev = pos + 1;
        i += 2;
    } while (prev < ipAddr.size());

    bufLen = i;

    return 0;
}

int SaeApplication::getDefaultGWAddrInRsu(char *buf, int& len) {
    if (!buf or len <= 0 or len > CV2X_IPV6_ADDR_ARRAY_LEN) {
        cerr << "Input error for getDefaultGWAddrInRsu!"<< endl;
        return -1;
    }

    string strAddr;
    if (configuration.defaultGateway.empty()) {
        // If the static defaultGateway is not configured, get V2X IP rmnet addr
        if (0 != getV2xIpIfaceAddr(strAddr)) {
            std::cerr << "retrieve V2X IP addr error!" << endl;
            return -1;
        }
    } else {
        strAddr = configuration.defaultGateway;
    }

    if (appVerbosity > 3) {
        cout << "GW:" << strAddr << endl;
    }

    // parse the addr
    return parseIPv6Addr(strAddr, buf, len);
}

int SaeApplication::convertIpv6Addr2Str(char* buf, int bufLen, string& addr) {
    if (not buf or bufLen <= 0 or bufLen > CV2X_IPV6_ADDR_ARRAY_LEN) {
        cerr << "Input error for convertIpv6Addr2Str!" << endl;
        return -1;
    }

    std::ostringstream ss;
    for (int i = 0; i + 1 < bufLen; i += 2) {
        // add colon in front if it's not the first element
        if (0 != i) {
            ss << ':';
        }

        uint16_t val = buf[i] << 8 | buf[i + 1];
        ss << std::hex << val;
    }
    addr = ss.str();
    return 0;
}

int SaeApplication::setDefaultRouteInObu(string addr) {
    // delelte default route if already exist
    deleteDefaultRouteInObu();

    // set default route of OBU to the defaultGateway received from RSU
    string cmd = "ip -6 route add default via " + addr;
    FILE *fp;
    fp = popen(cmd.c_str(), "r");
    if (not fp) {
        cerr << "popen failed when set default route!" << endl;
        return -1;
    }
    if (pclose(fp) < 0) {
        cerr << "Set default route failed!" << endl;
        return -1;
    }
    obuRouteSet_ = true;
    if (appVerbosity > 3) {
        cout << "Set default route " << addr << endl;
    }
    return 0;
}

int SaeApplication::deleteDefaultRouteInObu() {
    if (not obuRouteSet_) {
        return 0;
    }

    FILE *fp;
    fp = popen("ip -6 route del default", "r");
    if (not fp) {
        cerr << "popen failed when delete default route!" << endl;
        return -1;
    }
    if (pclose(fp) < 0) {
        cerr << "Delete default route failed!" << endl;
        return -1;
    }

    if (appVerbosity > 3) {
        cout << "Delete default route" << endl;
    }
    return 0;
}

#ifdef WITH_WSA
int SaeApplication::storeWraInfoInObu(RoutingAdvertisement_t* wra) {
    if (not wra) {
        cerr << "Input error for storeWraInfoInObu" << endl;
        return -1;
    }

    // convert defaultGateway and primaryDns to readable format
    string addr1, addr2;
    if (convertIpv6Addr2Str((char *)wra->defaultGateway.buf, wra->defaultGateway.size, addr1)
        or convertIpv6Addr2Str((char *)wra->primaryDns.buf, wra->primaryDns.size, addr2)) {
        cerr << "convert gateway or DNS error" << endl;
        return -1;
    }

    if (appVerbosity > 3) {
        cout << "defaultGateway = " << addr1 << " primaryDns = " << addr2 << endl;
    }

    // check if the RSU gateway or primary DNS addr has changed
    if (not rsuGateway_.empty() and 0 == addr1.compare(rsuGateway_)
        and not rsuPrimaryDns_.empty() and 0 == addr2.compare(rsuPrimaryDns_)) {
        if (appVerbosity > 3) {
            cout << "RSU address not changed." << endl;
        }
        return 0;
    }

    // store the updated addr to the configured file
    ofstream file(configuration.wsaInfoFile);
    if (!file) {
        cerr << "Failed to create wsa file!" << endl;
        return -1;
    }

    file << "defaultGateway = " << addr1 << endl;
    file << "primaryDns = " << addr2;
    rsuGateway_ = addr1;
    rsuPrimaryDns_ = addr2;

    // set default route to the RSU rmnet addr
    // The DNS address is not actually used for now
    return setDefaultRouteInObu(rsuGateway_);
}

void SaeApplication::fillWsa(SrvAdvMsg_t *wsa, RoutingAdvertisement_t *wra) {
    memset(wsa, 0, sizeof(SrvAdvMsg_t));
    wsa->version = 3;  /* 1609.3 2016 */
    wsa->body.routingAdvertisement = wra;
    memset(wra, 0, sizeof(RoutingAdvertisement_t));

    wra->lifetime = configuration.routerLifetime;

    /* Fill in the IPv6 prefix */
    char ipAddr[CV2X_IPV6_ADDR_ARRAY_LEN] = {0};
    int addrLen = CV2X_IPV6_ADDR_ARRAY_LEN;
    if (parseIPv6Addr(configuration.ipPrefix, ipAddr, addrLen) < 0) {
        cerr << " Parse IPv6 prefix error" << endl;
        return;
    }
    if (OCTET_STRING_fromBuf(&wra->ipPrefix, ipAddr, CV2X_IPV6_ADDR_ARRAY_LEN) < 0) {
        if (appVerbosity > 3)
            std::cerr << "wra conversion failure for ipPrefix" << std::endl;
    }
    wra->ipPrefixLength = configuration.ipPrefixLength;

    /* Fill in the defaultGateway, it's either from the configuration or the dynamic rmnet addr */
    addrLen = CV2X_IPV6_ADDR_ARRAY_LEN;
    memset(ipAddr, 0, CV2X_IPV6_ADDR_ARRAY_LEN);
    if (0 == getDefaultGWAddrInRsu(ipAddr, addrLen)) {
        if (OCTET_STRING_fromBuf(&wra->defaultGateway, ipAddr, addrLen) < 0) {
            if (appVerbosity > 3)
                std::cerr << "wra conversion failure for defaultGateway" << std::endl;
        }
    }

    /* Fill in the primary DNS */
    addrLen = CV2X_IPV6_ADDR_ARRAY_LEN;
    memset(ipAddr, 0, CV2X_IPV6_ADDR_ARRAY_LEN);
    if (parseIPv6Addr(configuration.primaryDns, ipAddr, addrLen) < 0) {
        cerr << " Parse primary DNS error" << endl;
        return;
    }
    if (OCTET_STRING_fromBuf(&wra->primaryDns, ipAddr, CV2X_IPV6_ADDR_ARRAY_LEN) < 0) {
        if (appVerbosity > 3)
            std::cerr << "wra conversion failure for primaryDns" << std::endl;
    }
}
#endif

/*
 * This function will demonstrate how to fill the BSM contents according to spec.
 * QITS use vehicle data from an available CAN interface and provided values
 * from the configuration file.
 * Message count and id are randomized and incremented according to specification.
 */
void SaeApplication::fillBsm(bsm_value_t *bsm) {

    uint32_t randNumMsgId = 0;
    uint32_t randNumMsgCount = 0;
    int rng_ret = -1 ;
    bool idChangeEnabled = false;
    memset(bsm, 0, sizeof(bsm_value_t));

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand(ts.tv_sec * 1000000000LL + ts.tv_nsec);
    fillBsmCan(bsm);
    fillBsmLocation(bsm);
    bsm->timestamp_ms = timestamp_now();
    bsm->VehicleLength_cm = configuration.vehicleLength;
    bsm->VehicleWidth_cm = configuration.vehicleWidth;
    bsm->secMark_ms = bsm->timestamp_ms % 60000;
    // needs to be randomized along with l2 address and msg id and pseudonym cert

    // check if msg count has been randomized and we haven't updated this yet
    // if so, keep adding and modding 128
    if (!this->configuration.lcmName.empty() &&
        this->configuration.idChangeInterval) {
        idChangeEnabled = true;
    }

    if (idChangeEnabled) {
        sem_wait(&idChangeData.idSem);
    }
    // for synchronization between Application and Aerolink sides
    // Using the HW TME Random number Generator as the TRNG Source
    if (!utility_) {
        utility_ = std::make_shared<QUtils>();
    }
    rng_ret = utility_->hwTRNGInt(randNumMsgCount);
    if(rng_ret){
        printf("Failure in Randon Number Generation for Message Count \n");
    }
    rng_ret = utility_->hwTRNGInt(randNumMsgId);
    if(rng_ret){
        printf("Failure in Randon Number Generation for Message Id \n");
    }
    if (!initialized) {
        bsm->MsgCount = (randNumMsgCount % 128);
        bsm->id = randNumMsgId;
        initialized = true;
        if (appVerbosity > 1) {
            printf("Msg count: %d, id: %u\n", bsm->MsgCount, bsm->id);
        }
    }
    else if (idChangeData.idChanged) {
        // randomize msg count
        bsm->MsgCount = (randNumMsgCount % 128);
        // update the temp id
        bsm->id = (uint32_t)idChangeData.tempId[0] << 24 |
        (uint32_t)idChangeData.tempId[1] << 16 |
        (uint32_t)idChangeData.tempId[2] << 8  |
        (uint32_t)idChangeData.tempId[3];
        idChangeData.idChanged = false;
        if (appVerbosity > 1)
            printf("Id changed, new msgcount is: %d, and new temp id is: %u\n",
                                    bsm->MsgCount, bsm->id);
        this->tempId = bsm->id;
    }
    else{
        bsm->MsgCount = (msgCount + 1) % 128;
        bsm->id = tempId;
    }
    if (idChangeEnabled) {
        sem_post(&idChangeData.idSem);
    }

    msgCount = bsm->MsgCount;
    tempId = bsm->id;
}


void SaeApplication::fillBsmCan(bsm_value_t *bsm)
{
    //fill data with values that may not make sense, these data should come from vehicle
    //CAN network
    //reset these variables
    bsm->events.data = 0;
    bsm->vehsafeopts = 0;
    if(criticalState){
        bsm->has_partII = (v2x_bool_t)1;
        bsm->qty_partII_extensions = (int)1;
        bsm->has_safety_extension = (v2x_bool_t)1;
        bsm->has_special_extension = (v2x_bool_t)0;
        bsm->has_supplemental_extension = (v2x_bool_t)0;
        bsm->TransmissionState = J2735_TRANNY_REVERSE_GEARS;
        bsm->vehsafeopts |= PART_II_SAFETY_EXT_OPTION_EVENTS;
    }else{
        bsm->has_partII = (v2x_bool_t)0;
        bsm->has_safety_extension = (v2x_bool_t)0;
        bsm->qty_partII_extensions = (int)0;
        bsm->has_special_extension = (v2x_bool_t)0;
        bsm->has_supplemental_extension = (v2x_bool_t)0;
        bsm->TransmissionState = J2735_TRANNY_FORWARD_GEARS;
        bsm->vehsafeopts = 0;

        if((configuration.emergencyVehicleEventTX))
        {
            /* Setting the BSM fields for simulating the Public Vehicle Emergency Event from
             * TX Device
             ASN.1 Representation:
             EmergencyDetails ::= SEQUENCE {
              notUsed SSPindex,
                -- always set to 0 and carries no meaning;
                -- legacy field maintained for backward compatibility
              sirenUse SirenInUse,
              lightsUse LightbarInUse,
              multi MultiVehicleResponse,
              events PrivilegedEvents OPTIONAL,
              responseType ResponseType OPTIONAL,
              ...
             }
             *
             */
            bsm->has_partII = (v2x_bool_t)1;
            bsm->qty_partII_extensions = (int)1;
            bsm->has_special_extension = (v2x_bool_t)1;
            bsm->vehicleAlerts.sspRights = 0 ;
            bsm->vehicleAlerts.sirenUse = J2735_SIREN_IN_USE;
            bsm->vehicleAlerts.lightsUse = J2735_LIGHTS_IN_USE;
            bsm->vehicleAlerts.multi = J2735_MULTIVEHICLE_AVAILABLE;
            bsm->specvehopts |= SPECIAL_VEH_EXT_OPTION_EMERGENCY_DETAILS;
        }
    }
    if(this->currVehState != NULL){
        // for each flag, set the corresponding one in the bsm
        bsm->events.bits.eventAirBagDeployment =
            this->currVehState->events.bits.eventAirBagDeployment;
        bsm->events.bits.eventDisabledVehicle =
            this->currVehState->events.bits.eventDisabledVehicle;
        bsm->events.bits.eventFlatTire =
            this->currVehState->events.bits.eventFlatTire;
        bsm->events.bits.eventWipersChanged =
            this->currVehState->events.bits.eventWipersChanged;
        bsm->events.bits.eventLightsChanged =
            this->currVehState->events.bits.eventLightsChanged;
        bsm->events.bits.eventHardBraking =
            this->currVehState->events.bits.eventHardBraking;
        bsm->events.bits.eventHazardousMaterials =
            this->currVehState->events.bits.eventHazardousMaterials;
        bsm->events.bits.eventStabilityControlactivated =
            this->currVehState->events.bits.eventStabilityControlactivated;
        bsm->events.bits.eventTractionControlLoss =
            this->currVehState->events.bits.eventTractionControlLoss;
        bsm->events.bits.eventABSactivated =
            this->currVehState->events.bits.eventABSactivated;
        bsm->events.bits.eventStopLineViolation =
            this->currVehState->events.bits.eventStopLineViolation;
        bsm->events.bits.eventHazardLights =
            this->currVehState->events.bits.eventHazardLights;
        bsm->events.bits.unused = 0;
        bsm->events.bits.eventReserved1 = 0;
    }
}

void SaeApplication::fillBsmLocation(bsm_value_t *bsm) {
    if(!configuration.enableLocationFixes || !kinematicsReceive ||
        !appLocListener_ || !hvLocationInfo){
        return;
    }
    lock_guard<mutex> lk(ApplicationBase::hvLocUpdateMtx);
    //ref_app code with the new telSDK Location
    bsm->Latitude = (hvLocationInfo->getLatitude() * 10000000);
    bsm->Longitude = (hvLocationInfo->getLongitude() * 10000000);
    bsm->Elevation = (hvLocationInfo->getAltitude() * 10);

    bsm->SemiMajorAxisAccuracy = (hvLocationInfo->getHorizontalUncertaintySemiMajor() * 20);

    bsm->SemiMinorAxisAccuracy = (hvLocationInfo->getHorizontalUncertaintySemiMinor() * 20);

    bsm->SemiMajorAxisOrientation = (hvLocationInfo->getHorizontalUncertaintyAzimuth() / 0.0054932479);

    bsm->Heading_degrees = (hvLocationInfo->getHeading() / 0.0125);

    bsm->Speed = (50 * hvLocationInfo->getSpeed());

    bsm->AccelLat_cm_per_sec_squared = (100 * hvLocationInfo->getBodyFrameData().latAccel);

    bsm->AccelLon_cm_per_sec_squared = (100 * hvLocationInfo->getBodyFrameData().longAccel);

    bsm->AccelVert_two_centi_gs = (hvLocationInfo->getBodyFrameData().latAccel * 50); // / 0.1962);

    bsm->AccelYaw_centi_degrees_per_sec = (hvLocationInfo->getBodyFrameData().yawRate * 100);
}
void SaeApplication::initRecordedBsm(bsm_value_t* bsm) {
    bsm->timestamp_ms = (uint64_t ) 0;
    bsm->MsgCount = (unsigned int ) 0;
    bsm->id = (unsigned int ) 0;
    bsm->secMark_ms = (unsigned int) 0;
    bsm->Latitude = (signed int) 0;
    bsm->Longitude = (signed int) 0;
    bsm->Elevation = (signed int) 0;
    bsm->SemiMajorAxisAccuracy = (unsigned int) 0;
    bsm->SemiMinorAxisAccuracy = (unsigned int) 0;
    bsm->SemiMajorAxisOrientation = (unsigned int) 0;
    bsm->TransmissionState = (j2735_transmission_state_e) 0;
    bsm->Speed = (unsigned int) 0;
    bsm->Heading_degrees = (unsigned int) 0;
    bsm->SteeringWheelAngle = (signed int) 0;
    bsm->AccelLon_cm_per_sec_squared = (signed int) 0;
    bsm->AccelLat_cm_per_sec_squared = (signed int) 0;
    bsm->AccelVert_two_centi_gs = (signed int) 0;
    bsm->AccelYaw_centi_degrees_per_sec = (signed int) 0;
    bsm->brakes.word = (uint16_t) 0;
    bsm->brakes.bits.unused_padding = (unsigned) 0;
    bsm->brakes.bits.aux_brake_status = (j2735_AuxBrakeStatus_e) 0;
    bsm->brakes.bits.brake_boost_applied = (j2735_BrakeBoostApplied_e) 0;
    bsm->brakes.bits.stability_control_status = (j2735_StabilityControlStatus_e)0;
    bsm->brakes.bits.antilock_brake_status = (j2735_AntiLockBrakeStatus_e) 0;
    bsm->brakes.bits.traction_control_status = (j2735_TractionControlStatus_e) 0;
    bsm->brakes.bits.rightRear = (unsigned) 0;
    bsm->brakes.bits.rightFront = (unsigned) 0;
    bsm->brakes.bits.leftRear = (unsigned) 0;
    bsm->brakes.bits.leftFront = (unsigned) 0;
    bsm->brakes.bits.unavailable = (unsigned) 0;
    bsm->VehicleWidth_cm = (unsigned int) 0;
    bsm->VehicleLength_cm = (unsigned int) 0;
    bsm->has_partII = (v2x_bool_t) 0;
    bsm->qty_partII_extensions = (int) 0;
    bsm->has_safety_extension = (v2x_bool_t) 0;
    bsm->has_special_extension = (v2x_bool_t) 0;
    bsm->has_supplemental_extension = (v2x_bool_t) 0;
    bsm->vehsafeopts = (int) 0;
    bsm->phopts = (int) 0;
    bsm->ph = (path_history_t) { 0 };
    bsm->pp.is_straight = (v2x_bool_t) 0;
    bsm->pp.radius = (signed int) 0;  // Radius of Curve in unis of 10cm
    bsm->pp.confidence = (uint8_t) 0;
    bsm->events.data = (uint16_t) 0;
    bsm->events.bits.eventAirBagDeployment = (unsigned) 0;
    bsm->events.bits.eventDisabledVehicle = (unsigned) 0;
    bsm->events.bits.eventFlatTire = (unsigned) 0;
    bsm->events.bits.eventWipersChanged = (unsigned) 0;
    bsm->events.bits.eventLightsChanged = (unsigned) 0;
    bsm->events.bits.eventHardBraking = (unsigned) 0;
    bsm->events.bits.eventReserved1 = (unsigned) 0;
    bsm->events.bits.eventHazardousMaterials = (unsigned) 0;
    bsm->events.bits.eventStabilityControlactivated = (unsigned) 0;
    bsm->events.bits.eventTractionControlLoss = (unsigned) 0;
    bsm->events.bits.eventABSactivated = (unsigned) 0;
    bsm->events.bits.eventStopLineViolation = (unsigned) 0;
    bsm->events.bits.eventHazardLights = (unsigned) 0;
    bsm->events.bits.unused = (unsigned) 0;
    bsm->lights_in_use.data = (uint16_t) 0;
    bsm->lights_in_use.bits.parkingLightsOn = (unsigned) 0;
    bsm->lights_in_use.bits.fogLightOn = (unsigned) 0;
    bsm->lights_in_use.bits.daytimeRunningLightsOn = (unsigned) 0;
    bsm->lights_in_use.bits.automaticLightControlOn = (unsigned) 0;
    bsm->lights_in_use.bits.hazardSignalOn = (unsigned) 0;
    bsm->lights_in_use.bits.rightTurnSignalOn = (unsigned) 0;
    bsm->lights_in_use.bits.leftTurnSignalOn = (unsigned) 0;
    bsm->lights_in_use.bits.highBeamHeadlightsOn = (unsigned) 0;
    bsm->lights_in_use.bits.lowBeamHeadlightsOn = (unsigned) 0;
    bsm->lights_in_use.bits.unused = (unsigned) 0;
    bsm->specvehopts = (int) 0;
    bsm->edopts = (uint32_t) 0;
    bsm->eventopts = (uint32_t) 0;
    bsm->vehicleAlerts.sspRights = (int) 0;
    bsm->vehicleAlerts.sirenUse = (int) 0;
    bsm->vehicleAlerts.lightsUse = (int) 0;
    bsm->vehicleAlerts.multi = (int) 0;
    bsm->vehicleAlerts.events.sspRights = (int) 0;
    bsm->vehicleAlerts.events.event = (int) 0;
    bsm->vehicleAlerts.responseType = (int) 0;
    bsm->description.typeEvent = (int) 0;
    bsm->description.size_desc = (int) 0;
    //bsm->description.desc = { 0 };//array of 8 ints so it is already initialized
    bsm->description.priority = (int) 0;
    bsm->description.heading = (int) 0;
    bsm->description.extent = (int) 0;
    bsm->description.size_reg = (int) 0;
    //bsm->description.regional = {0}//array of 4 ints so it is already initialized
    bsm->trailers.sspRights = (int) 0;  // 5 bits
    bsm->trailers.pivotOffset = (int) 0; // 11 bits
    bsm->trailers.pivotAngle = (int) 0;  // 8 bits
    bsm->trailers.pivots = (int) 0; // 1 bit. boolean
    bsm->trailers.size_trailer = (int) 0;
    //bsm->trailers.units = {0}// 8 tr_unit_dest array so it is already initialized
    bsm->suppvehopts = (int) 0;
    bsm->VehicleClass = (int) 0;
    bsm->veh.height_cm = (uint32_t) 0;
    bsm->veh.front_bumper_height_cm = (uint32_t) 0;
    bsm->veh.rear_bumper_height_cm = (uint32_t) 0;
    bsm->veh.mass_kg = (uint32_t) 0;
    bsm->veh.trailer_weight = (uint32_t) 0;
    bsm->veh.supplemental_veh_data_options.word = (uint8_t) 0;
    bsm->veh.supplemental_veh_data_options.bits.has_trailer_weight = (unsigned) 0;
    bsm->veh.supplemental_veh_data_options.bits.has_mass = (unsigned) 0;
    bsm->veh.supplemental_veh_data_options.bits.has_bumpers_heights = (unsigned) 0;
    bsm->veh.supplemental_veh_data_options.bits.has_height = (unsigned) 0;
    bsm->veh.supplemental_veh_data_options.bits.unused_padding = (unsigned) 0;
    bsm->weatheropts = (uint32_t) 0;
    bsm->wiperopts = (uint32_t) 0;
    bsm->airTemp = (int) 0;
    bsm->airPressure = (int) 0;
    bsm->rateFront = (int) 0;
    bsm->rateRear = (int) 0;
    bsm->statusFront = (int) 0;
    bsm->statusRear = (int) 0;
}

int SaeApplication::transmit(uint8_t index, std::shared_ptr<msg_contents>mc_,
                                int16_t bufLen, TransmitType txType) {
    std::thread::id tid = std::this_thread::get_id();
    int encLength = bufLen;
    int ret = -1;
    // let the transmit function handle the actual transmission
    if (encLength) {
        // insert family ID of 0x01
        char *p = abuf_push(&mc_->abuf, 1);
        if (p != NULL) {
            *p = 0x01;
            ret = ApplicationBase::transmit(index, mc_, encLength+1, txType);
        }
    }
    if (ret > 0){
        txSuccess++;
        if (qMon)
        {
            qMon->tData[tid].totalTx++;
        }
    }
    else{
        txFail++;
    }
    return ret;
}

void SaeApplication::receiveTuncBsm(const uint8_t index, const uint16_t bufLen, const uint32_t ldmIndex) {
    const auto i = index;
    float tunc = -1;
    threadMc = this->ldm->bsmContents[ldmIndex];
    if (isRxSim) {
        threadMc = rxSimMsg;
    }
    decode_msg(threadMc.get());

    const bsm_value_t *bsm = reinterpret_cast<bsm_value_t *>(threadMc->j2735_msg);
    if (this->ldm->tuncs.find(bsm->id) == this->ldm->tuncs.end()) {
        this->ldm->tuncs.insert(pair<uint32_t, float>(bsm->id, tunc));
    }
    else {
        this->ldm->tuncs[bsm->id] += tunc;
    }

}

void SaeApplication::sendTuncBsm(uint8_t index, TransmitType txType) {
    const auto i = index;
    auto encLength = 0;
    msg_contents* msg = NULL;
    float tunc = -1.0;
    auto debugVar = 0;
    std::shared_ptr<msg_contents> mc;
    switch (txType)
    {
    case TransmitType::SPS:
        mc = this->spsContents[i];
        fillMsg(mc);
        encLength = encode_msg(mc.get());
        this->spsTransmits[i].statusCheck(RadioType::TX); //This will give us the most up to date time uncertainty
        debugVar = this->spsTransmits[i].gCv2xStatus.timeUncertainty;
        tunc = ntohl(this->spsTransmits[i].gCv2xStatus.timeUncertainty);
        memcpy(mc->abuf.tail, &tunc, sizeof(float));
        abuf_put(&mc->abuf, sizeof(float));
        encLength += sizeof(float);
        // SPS priority is set when creating the flow
        this->spsTransmits[i].transmit(mc->abuf.data, encLength, Priority::PRIORITY_UNKNOWN);
        break;
    case TransmitType::EVENT:
        mc = this->eventContents[i];
        fillMsg(mc);
        encLength = encode_msg(mc.get());
        this->spsTransmits[i].statusCheck(RadioType::TX); //This will give us the most up to date time uncertainty
        debugVar = this->spsTransmits[i].gCv2xStatus.timeUncertainty;
        tunc = ntohl(this->spsTransmits[i].gCv2xStatus.timeUncertainty);
        memcpy(mc->abuf.tail, &tunc, sizeof(float));
        abuf_put(&mc->abuf, sizeof(float));
        encLength += sizeof(float);
        // event priority is set per packet using traffic class
        this->eventTransmits[i].transmit(mc->abuf.data, encLength, configuration.eventPriority);
        break;
    default:
        break;
    }
}

#ifdef WITH_WSA
int SaeApplication::onReceiveWra(RoutingAdvertisement_t *wra, uint8_t *sourceMacAddr,
        int& macAdrLen) {
    std::lock_guard<std::mutex> lock(wramutex);
    telux::cv2x::IPv6AddrType IpPrefix;
    telux::cv2x::GlobalIPUnicastRoutingInfo RoutingInfo;
    int ret = 0;
    auto func = [this](int routerLifetime) {
        wraThreadFunc(routerLifetime);
    };

    if (GlobalIpSessionActive == true) {
        if (wraInterval == std::chrono::milliseconds::zero()) {
            //received the second WRA message, need to determine the period of the WRA,
            //so that if within expected internal we didn't receive next WRA, we deem the
            //OBU went out of range of the associated RSU.
            auto diff = std::chrono::high_resolution_clock::now() - now;
            wraInterval = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
            if (appVerbosity > 3)
                cout << "wraInterval=" << wraInterval.count() << endl;
        }
        wraCv.notify_all();
        if (memcmp(sourceMacAddr, prevSourceMac, CV2X_MAC_ADDR_LEN)) {
            memcpy(RoutingInfo.destMacAddr, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            memcpy(prevSourceMac, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            if (appVerbosity > 3)
                std::cout << "Updating routing info" << endl;
            ret = radioReceives[0].setRoutingInfo(RoutingInfo);
        }
    } else {
        if (wra->ipPrefix.size > CV2X_IPV6_ADDR_ARRAY_LEN) {
            if (appVerbosity > 3)
                std::cerr << "Invalid ip prefix length received: " <<
                        wra->ipPrefix.size << endl;
            ret = -1;
        } else {
            //Received first valid WRA.
            now = std::chrono::high_resolution_clock::now();
            memcpy(IpPrefix.ipv6Addr, wra->ipPrefix.buf, wra->ipPrefix.size);
            IpPrefix.prefixLen = wra->ipPrefixLength;
            if (appVerbosity > 3)
                cout << "Setting Global IP address" << endl;
            memcpy(prevSourceMac, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            ret = radioReceives[0].setGlobalIPInfo(IpPrefix, configuration.wraServiceId);
            if (!ret) {
                memcpy(RoutingInfo.destMacAddr, sourceMacAddr, CV2X_MAC_ADDR_LEN);
                ret = radioReceives[0].setRoutingInfo(RoutingInfo);
                if (ret) {
                    return ret;
                }
                //Launch Wra thread to monitor WRA timeout.
                if (wraThread.joinable() == false) {
                    wraThread = std::thread(func, wra->lifetime);
                    GlobalIpSessionActive = true;
                } else {
                    if (GlobalIpSessionActive == false) {
                        wraThread.join();
                        wraThread = std::thread(func, wra->lifetime);
                        GlobalIpSessionActive = true;
                    } else {
                        //Notify Wra thread we got new WRA message
                        wraCv.notify_all();
                    }
                }
            }
        }
    }
    // store the wra info from RSU if wsaInfoFile is configured in ObeConfig.conf
    if (0 == ret and not configuration.wsaInfoFile.empty()) {
        return storeWraInfoInObu(wra);
    }
    return ret;
}
#endif
void SaeApplication::wraThreadFunc(int routerLifetime)
{
    std::unique_lock<std::mutex> lk(wraMutex);
    std::cv_status status;

    lk.unlock();
    while (not exit_) {
        lk.lock();
        if (wraInterval == std::chrono::milliseconds::zero()) {
            // we didn't receive 2nd WRA yet, no idea about the WRA interval.
            // Use routerlifetime as wait time.
            std::chrono::milliseconds wt(routerLifetime*1000);
            status = wraCv.wait_for(lk, wt);
        } else {
            //if no WRA within 3*interval, deem it as out of range of RSU
            status = wraCv.wait_for(lk, 3*wraInterval);
        }
        lk.unlock();
        if (status == std::cv_status::timeout) {
            radioReceives[0].onWraTimedout();
            GlobalIpSessionActive = false;
            if (appVerbosity > 3)
                std::cout << "WRA timeout, global IP session stopped" << endl;
            return;
        }
    }
}

/* For RSU use case only */
int SaeApplication::setGlobalIPv6Prefix(void)
{
    char ipPrefix[CV2X_IPV6_ADDR_ARRAY_LEN];
    int prefixLen = CV2X_IPV6_ADDR_ARRAY_LEN;
    telux::cv2x::IPv6AddrType IpPrefix;
    int ret = 0;

    if (GlobalIpSessionActive == false) {
        memset(ipPrefix, 0, CV2X_IPV6_ADDR_ARRAY_LEN);
        if (!parseIPv6Addr(configuration.ipPrefix, ipPrefix, prefixLen)) {
            IpPrefix.prefixLen = configuration.ipPrefixLength;
            memcpy(IpPrefix.ipv6Addr, ipPrefix, prefixLen);
            if(radioReceives.size()){
                ret = radioReceives[0].setGlobalIPInfo(IpPrefix, configuration.wraServiceId);
            }else if(this->spsTransmits.size()){
                ret = this->spsTransmits[0].setGlobalIPInfo(IpPrefix, configuration.wraServiceId);
            }
        }
        GlobalIpSessionActive = true;
    }

    return ret;
}

int SaeApplication::clearGlobalIPv6Prefix(void)
{
    int ret = 0;
    GlobalIpSessionActive = false;
    if(radioReceives.size()){
        ret = radioReceives[0].clearGlobalIPInfo();
    }else if(this->spsTransmits.size()){
        ret = this->spsTransmits[0].clearGlobalIPInfo();
    }
    return ret;
}
