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
 *Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without
 *modification, are permitted (subject to the limitations in the
 *disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /**
  * @file: ApplicationBase.hpp
  *
  * @brief: Base class for ITS stack
  */

#ifndef __APPLICATION_BASE_HPP__
#define __APPLICATION_BASE_HPP__
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <map>
#include <unistd.h>
#include <csignal>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <sys/resource.h>
#include <telux/cv2x/prop/CongestionControlManager.hpp>
#include <telux/cv2x/prop/V2xPropFactory.hpp>
#include "v2x_msg.h"
#include "v2x_codec.h"
#include "KinematicsReceive.h"
#include "RadioReceive.h"
#include "RadioTransmit.h"
#include "VehicleReceive.h"
#include "Ldm.h"
#include "ThrottleManager.h"
#include "safetyapp_util.h"
#include "qMonitor.hpp"
#include "qUtils.hpp"
#include <telux/sec/CryptoAcceleratorManager.hpp>
#include <telux/sec/SecurityFactory.hpp>
#include <telux/sec/CAControlManager.hpp>
#ifdef AEROLINK
#include "AerolinkSecurity.hpp"
#else
#include "NullSecurity.hpp"
#endif

#define ABUF_LEN            8448
#define ABUF_HEADROOM       256
#define MIN_PACKET_LEN      20
#define MAX_PACKET_LEN      8192
#define DEFAULT_BSM_PSID    32
#define MAX_PADDING_LEN     1000
#define MAX_TIMESTAMP_BUFFER_SIZE 80
#define PP_BUFFER_MAX_SIZE 4096
#define SHARED_BUFFER_MAX_SIZE 2048
#define ASYNC_BATCH_SIZE 500
#define VERIF_STAT_BATCH_SIZE 2500
#define DEFAULT_PROCESS_PRIORITY -20
#define MIN_NICE -20
#define MAX_NICE 19
#define DECODE_SUCCESS 0
#define DECODE_FAIL -1
#define DECODE_SIGNED 1

#define LOG_HEADER "TimeStamp,TimeStamp_ms,Time_monotonic,LogRecType,L2 ID,"\
                   "CBR Percent,CPU_Util,TXInterval,msgCnt,TempId,GPGSAMode,"\
                   "secMark,lat,long,semi_major_dev,speed,heading,longAccel,"\
                   "latAccel,Tracking_Error,vehicleDensityInRange,ChannelQualityIndication,"\
                   "BSMValid,max_ITT,GPS-Time,Events,DCC random time,Hysterisis,"\
                   "TotalRVs,DistanceFromRV"

using telux::cv2x::Priority;
using namespace std;
using namespace telux::cv2x::prop;
using telux::sec::ICAControlManager;
using telux::sec::SecurityFactory;
using telux::sec::ICAControlManagerListener;
using telux::sec::LoadConfig;
using telux::sec::CACapacity;
using telux::sec::CALoad;
enum class TransmitType {
    SPS,
    EVENT
};

enum class MessageType {
    BSM,
    CAM,
    DENM,
    SPAT,
    WSA
};

enum class TxRxType {
    TX,
    RX
};

typedef struct {
    uint64_t timestamp = 0;
    uint8_t index = 0;
    double distFromRV = 0.0;
    bsm_data bs = {0};
} logData;

typedef enum {FREE, VERIF_DONE, PP_DONE} AsyncCbState;

typedef struct {
    int indexToData;
    bool verifSuccess;
    AsyncCbState AsyncState=FREE;
    bsm_data asyncBs;
    uint32_t psid;
    uint8_t msg_index;
    uint64_t timestamp;
    uint32_t l2SrcAddr;
    double distFromRV;
    uint32_t RVsInRange;
    uint64_t txInterval;
    double startLatencyTime;
    double endLatencyTime;
    Kinematics rvKine;
    MisbehaviorStats* misbehaviorStat;
    VerifStats* asyncVerifStat;
    void* msgParseContext;
} asyncCbData_t;

struct Config {
    int procPriority = DEFAULT_PROCESS_PRIORITY;
    bool isValid = false;
    int codecVerbosity = 0;
    int ldmVerbosity = 0;
    vector<uint16_t> receivePorts;
    vector<uint32_t> receiveSubIds;
    uint32_t receiveSubId;
    vector<uint16_t> eventPorts;
    vector<uint16_t> spsPorts;
    vector<uint32_t> spsServiceIDs;
    vector<uint32_t> eventServiceIDs;
    vector<string> spsDestAddrs;
    vector<uint16_t> spsDestPorts;
    vector<uint16_t> eventDestPorts;
    vector<string> eventDestAddrs;
    uint32_t padding = 0; // length of dummy data added to BSM, unit in Bytes
    Priority spsPriority = Priority::PRIORITY_5; // priority setting for sps flow
    Priority eventPriority = Priority::PRIORITY_2; // priority setting for event
    //vector<uint32_t> spsReservationSizes;
    uint32_t spsReservationSize;
    bool wildcardRx = false;
    bool enablePreRecorded = false;
    string preRecordedFile;
    bool preRecordedBsmLog = false;
    bool enableTxAlways = true;
    uint16_t ldmGbTime = 3;
    uint8_t ldmGbTimeThreshold= 5;
    uint16_t ldmSize = 1;
    uint16_t transmitRate = 100;
    uint16_t spsPeriodicity = 100;
    uint16_t locationInterval = 100;
    unsigned int msgId = 0;
    uint16_t bsmJitter = 0;
    bool enableVehicleExt = false;
    uint8_t pathHistoryPoints = 15;
    uint8_t vehicleWidth = 0;
    uint8_t vehicleLength = 0;
    uint8_t vehicleHeight = 0;
    uint8_t frontBumperHeight = 0;
    uint8_t rearBumperHeight = 0;
    uint8_t vehicleMass = 0;
    uint8_t vehicleClass = 0;
    uint8_t sirenUse = 0;
    uint8_t lightBarUse = 0;
    uint16_t specialVehicleTypeEvent = 0;
    uint8_t vehicleType = 0;
    uint32_t tunc = 0;
    uint32_t age = 0;
    uint32_t uncertainty3D = 0;
    uint32_t distance3D = 0;
    bool qMonEnabled = false;
    uint32_t packetError = 0;
    uint8_t leapSeconds = 18;
    /** Simulation config */
    string ipv4_src;
    string ipv4_dest;
    uint16_t tx_port = 0;
    /** GeoNetwork config data */
    uint8_t MacAddr[6];
    int StationType = 0;
    uint16_t CAMDestinationPort = 0;
    /** Security Config Data */
    bool enableSecurity = false;
    string securityContextName;
    uint16_t securityCountryCode;
    uint32_t psid;
    uint8_t ssp[31];
    uint32_t sspLength = 0;
    uint8_t sspMask[31];
    uint32_t sspMaskLength = 0;
    bool enableSsp = false;
    bool enableSspMask = false;
    vector<string> sspValueVect;
    vector<string> sspMaskVect;
    bool enableAsync = false;
    bool enableEncrypt = false;
    bool setGenLocation = true;
    bool enableConsistency = true;
    bool enableRelevance = true;
    bool overridePsidCheck = false;
    bool emergencyVehicleEventTX = false;
    vector<string> expectedSspValueVect;
    uint8_t expectedSsp[31];
    uint32_t expectedSspLength = 0;
    uint8_t externalDataHash[32];
    uint32_t hashLength = 0;
    bool acceptAll = false;
    bool overrideVerifResult = false;
    int overrideVerifValue = -1;
    bool fakeRVTempIds = false;
    uint32_t totalFakeRVTempIds = 500;
    int RVTransmitLossSimulation = 0; //percentage of transmit loss of RVs
    /** Sec Driver Options **/
    uint8_t driverVerbosity = 0;
    uint8_t secVerbosity = 0;
    uint8_t appVerbosity = 0;
    /** Sec Driver Multi Threading Options **/
    uint8_t numRxThreadsEth = 1;
    uint8_t numRxThreadsRadio = 1;
    uint8_t numTxThreads = 0; // TODO
    /** Verification Stats Parameters */
    bool enableVerifStatLog = false;
    uint32_t verifStatsSize = 10000;
    string verifStatLogFile = "/tmp/verif_stats.log";
    /** Verification Results Parameters */
    bool enableVerifResLog = false;
    uint32_t verifResLogSize = 10000;
    string verifResLogFile = "/tmp/verif_results.log";
    /** Signing Stats Parameters */
    bool enableSignStatLog = false;
    uint32_t signStatsSize = 10000;
    string signStatLogFile = "/tmp/sign_stats.log";
    bool enableLocationFixes = true;
    bool enableDistanceLogs = false;
    bool enableVehicleDataCallbacks = true;
    /* config data for Ieee1609.3 Wsa */
    long routerLifetime;
    string ipPrefix;
    int ipPrefixLength;
    string defaultGateway;
    string primaryDns;
    uint32_t wraServiceId = 4;
    string wsaInfoFile;
    uint32_t wsaInterval = 1000; // WSA Tx interval, 1s by default
    /** Pseudonym/ID Change */
    string lcmName;
    unsigned int idChangeInterval = 0;
    /** Misbehavior Stats Parameters */
    bool enableMbd = false;
    bool enableMbdStatLog = false;
    uint32_t mbdStatLogListSize = 10000;
    string mbdStatLogFile = "/tmp/misbehavior_stats.log";
    unsigned int filterInterval = 1000; // 1 s by default
    unsigned int deltaInRxRate = 200; // 200 pkts by default
    bool enableL2Filtering = false;
    unsigned int l2FilteringTime = 1; //1 s by default
    uint8_t l2IdTimeThreshold = 5;
    /* Flooding attack detection and mitigation */
    bool enableL2FloodingDetect = false; // enable flooding attack detection and mitigation
    uint32_t floodDetectVerbosity = 0; // 0-8; console logging level for flooding detection
    uint32_t commandInterval = 100; //ms ; basic interval in flooding attack detection
    uint32_t nCommandInterval_0 = 10; // num of command intervals in eval interval in state 0
    uint32_t nCommandInterval_1 = 5; // num of command intervals in eval interval in state 1
    uint32_t floodAttackThreshTotal = 1000; // msg per sec
    uint32_t floodAttackThreshSingle = 100; // msg per sec
    uint32_t tShiftInterval = 20; // ms
    uint32_t loadUpdateInterval = 100; // interval for how often mvm load is updated
    double mvmUtilThreshold = 0.5; // utilization threshold to determine if should change state
    bool mvmCapacityOverride = false; // flag to override actual mvm capacity for testing
    uint32_t mvmCapacity = 20; // mvm capacity value to use if override flag enabled

    string congestionControlConfigFileName = "CongestionControlConfig.conf";
    bool enableCongCtrl = false; // flag to override actual mvm capacity for testing purposes
    bool positionOverride = false;
    double overrideLat = 0.0;
    double overrideLong = 0.0;
    double overrideHead = 0.0;
    double overrideElev = 0.0;
    double overrideSpeed = 0.0;
};

struct DiagLogData {
    bool validPkt;
    uint64_t currTime;
    uint8_t cbr;
    uint64_t monotonicTime;
    uint64_t txInterval;
    bool enableCongCtrl;
    bool congCtrlInitialized;
};

/* Congestion Control CongestionControl Data */
struct CongCtrlConfig {
    /*
        SQUISH CONFIGURATION ITEMS
    */
    uint8_t congCtrlType = 1; // for now just SAE j3161/1
    uint32_t enableCongCtrlLogging = 0;
    /*
        # j2945/1 ; j3161 ; SAE
    */
    uint32_t cbpMeasInterval = 100;
    double cbpWeightFactor = 0.5;
    /*
        packet error rate related
    */
    uint32_t perInterval = 5000;
    uint32_t perSubInterval = 100;
    double perMax = 0.3;

    // ITT related
    uint32_t vMax_ITT = 600;
    uint32_t vRescheduleTh = 25;
    uint8_t cv2xMaxITTRounding = 0; // 0, 2
    /*
        channel quality indication related
    */
    double minChanQualInd = 0.0;
    double maxChanQualInd = 0.3;
    /*
        density related
    */
    double vDensityWeightFactor = 0.05;
    double vDensityCoefficient = 25; // lambda
    uint32_t vDensityWindow = 1;
    uint32_t vDensityMinPerRange = 100;
    uint32_t vMaxSuccessiveFail = 3; //0, 10000
    uint8_t UseStaticVDensity = 0;// 0,1 # Will cause vDensity to be used instead of calculated density
    uint32_t vDensity = 10; //0, 100 # Only used if UseStaticDensity is set

    /*
        Tracking Error related
    */
    uint32_t txCtrlInterval = 100;
    uint32_t hvTEMinTimeDiff = 0;
    uint32_t hvTEMaxTimeDiff = 200;
    uint32_t rvTEMinTimeDiff = 0;
    uint32_t rvTEMaxTimeDiff = 3000;
    uint8_t teErrSensitivity = 75;
    double teMinThresh = 0.2;
    double teMaxThresh = 0.5;
    /*
        Inter Transmit Time and Tx Decision related
    */
    uint32_t minItt = 100;
    uint32_t maxItt = 600;
    uint8_t txRand = 5;
    uint32_t timeAccuracy = 1000;
    uint8_t reschedThresh = 25;

    /*
        power related
    */

    uint8_t supraGain = 0.5;
    uint8_t minChanUtil = 50;
    uint8_t maxChanUtil = 80;
    uint8_t minRadiPwr = 10;
    uint8_t maxRadiPwr = 20;

    /*
        sps enhancements parameters
    */
    bool enableSpsEnhancements = false;
    uint8_t spsEnhIntervalRound = 100; //#CV2XPeriodicityHz     = 100; 20, 1000
    uint8_t spsEnhHysterPerc = 5; //#cv2xSPSHysterisis = 5;
    uint8_t spsEnhDelayPerc = 20; //#cv2xMaxITTChangeFreq = 5;
};

class CaControlManagerListener : public ICAControlManagerListener {
public:
    struct CALoad currLoad = {0};
    void onCapacityUpdate(struct CACapacity newCapacity){}
    void onLoadUpdate(struct CALoad currentLoad){
        memcpy(&currLoad, &currentLoad, sizeof(struct CALoad));
    }
};

// listener for congestion control updates
class QitsCongCtrlListener :public ICongestionControlListener {
public:
    static RadioTransmit* spsTransmit_;
    uint64_t lastPeriodicity = 100;
    // need to provide pointer to sps transmit
    // need to provide pointer to cong control user data
    void updateSpsTransmitFlow(
        std::shared_ptr<CongestionControlUserData> congestionControlUserData) {
        // once the user data is updated, the thread in qits
        // can now schedule a transmission
        // cast void pointer
        // if sps enhancements enabled, we should make sure that the sps flow reservation is redone
        if (spsTransmit_ != nullptr && congestionControlUserData->spsEnhancementsEnabled
            && congestionControlUserData->congestionControlCalculations->maxITT != lastPeriodicity) {
            lastPeriodicity = congestionControlUserData->congestionControlCalculations->maxITT;
            // update the sps flow with the rounded max ITT that congestionControl calculates
            shared_ptr<SpsFlowInfo> spsInfoSharedPtr = spsTransmit_->getSpsFlowInfo();
            if (spsInfoSharedPtr == nullptr) {
                std::cerr << "Invalid sps info. Not updating. \n";
                return;
            }
            SpsFlowInfo *spsInfo = spsInfoSharedPtr.get();
            // congestionControl rounds it already to valid values for sps periodicity
            spsInfo->periodicityMs =
                (congestionControlUserData->congestionControlCalculations->maxITT);
            // set sps priority to same value
            spsInfo->priority = spsTransmit_->getSpsPriority();

            // set sps size to same value
            spsInfo->nbytesReserved = spsTransmit_->getSpsResSize();
            // catch future error here
            try{
                Status ret = spsTransmit_->updateSpsFlow(*spsInfo);
                if (ret == Status::FAILED) {
                    std::cerr << "sps transmit flow update failed\n";
                    std::cerr << "Max itt was: "
                              << congestionControlUserData->congestionControlCalculations->maxITT
                              << "\n";
                }
            } catch (const std::future_error &e) {
                std::cout << "Caught future error when updating sps flow\n";
                std::cout << "Error log is: " << e.what() << "\n";
            }
        }
    }
    void onCongestionControlDataReady (
        std::shared_ptr<CongestionControlUserData> congestionControlUserData,
            bool critEvent) override;
};

class ApplicationBase
{
public:
    sem_t rx_sem;
    sem_t log_sem;
    static sem_t triggerIdChangeSem;
    int appVerbosity = 0;
    int totalTxSuccess = 0;
    int totalRxSuccess = 0;
    int totalThrds = 0;
    int tempId = 0;
    int totalRxSuccessPerSecond = 0;
    int load = 0;
    int prevArrivalRate=0;
    int prevFilterRate= 0;
    int filterRate=0;
    int rxCount = 0;
    struct timeval startRxIntervalTime;
    struct timeval endRxIntervalTime;
    std::shared_ptr<QMonitor> qMon = nullptr;
    std::shared_ptr<QMonitor::Configuration> qMonConfig = nullptr;

    /* For multi-threaded msg verification */
    std::map<std::thread::id, int> verifStatIdx;
    std::map<std::thread::id, int> signStatIdx;
    std::map<std::thread::id, int> misbehaviorStatIdx;
    std::map<std::thread::id, long int> resultLoggingIdx;
    std::map<std::thread::id, std::vector<VerifStats>> thrVerifLatencies;
    std::map<std::thread::id, std::vector<SignStats>> thrSignLatencies;
    std::map<std::thread::id, std::vector<MisbehaviorStats>> thrMisbehaviorLatencies;
    std::map<std::thread::id, std::vector<ResultLoggingStats>> thrResLoggingValues;

    virtual ~ApplicationBase();

    /* Initialization */
    virtual bool init();

    /* Method to update the local stored V2X IP rmnet addr */
    int updateCachedV2xIpIfaceAddr();

    /* Method to get the local stored V2X IP rmnet addr */
    int getV2xIpIfaceAddr(string& addr);

    /* Identity Change Related Functions and Variables */
    void changeIdentity(sem_t* idChangeCbSem);
    IDChangeData idChangeData;
    uint64_t lastIdChangeTime = 0;
    sem_t idChangeCbSem;

    /* Function to permit different levels of verbosity */
    void setAppVerbosity(int value) {
        appVerbosity = value;
    }
    void detectFloodAndMitigate(bool& stateOn, std::vector<L2FilterInfo>* rvListToFilter);
    /**
    * Constructor of  Application instance with all the
    * specifications of a configuration file.
    * @param fileConfiguration a char* that contains the file path of the
    * @param msgType application message type .
    */
    ApplicationBase(char* fileConfiguration, MessageType msgType, bool enableCsvLog = false,
        bool enableDiagLog = false);

    /**
    * Constructs Application with all the specifications of a
    * configuration file and transmits in simulated TCP/IP
    * instead of Snaptel SDK radio. Ports can't be zero or won't be taken in
    * account.
    * @param txIpv4 a const string object that holds the transmit IP address.
    * @param txPort a const uint16_t that contains the transmit port.
    * @param rxIpv4 a const string object that holds the receive IP address.
    * @param rxPort a const uint16_t that contains the receive port.
    * @param fileConfiguration a char* that contains the file path of the
    */
    ApplicationBase(const string txIpv4, const uint16_t txPort, const string rxIpv4,
        const uint16_t rxPort, char* fileConfiguration, bool enableCsvLog = false,
        bool enableDiagLog = false);

    /**
    * send  send V2X message.
    * @param index - message content index.
    * @param type - The type of flow (event, sps) in which bsm will transmit.
    */

    int send(uint8_t index, TransmitType txType);

    /**
     * Function which encodes and signs message when security is enabled.
     */
    int encodeAndSignMsg(std::shared_ptr<msg_contents> mc,
                         SecurityService::SignType type = SecurityService::SignType::ST_AUTO);

    /**
     * receive process received contents.
     * @param index message content index.
     * @param bufLen received buffer length.
     */
    virtual int receive(const uint8_t index, const uint16_t bufLen);

    /**
     * receive process received contents and store in LDM.
     * @param index message content index.
     * @param bufLen received buffer length.
     * @param ldmIndex the LDM index
     */
    virtual int receive(const uint8_t index, const uint16_t bufLen,
                        const uint32_t ldmIndex);

    /**
     * Overloaded function to fill the message with stack specific data.(BSM/CAM/DENM) for transmission
     */
    virtual void fillMsg(std::shared_ptr<msg_contents> mc) = 0;

    /**
    * Closes all tx and rx flows from Snaptel SDK.
    */
    void closeAllRadio();

    /**
    *   Sets up the verification statistics vector based on the exisitng threads
    */
    void initVerifLogging();

    /**
     * Write verification statistics to file
     */
    void writeVerifLogging();

    /**
    *   Sets up the verification results vector based on the exisitng threads
    */
    void initResultsLogging();

    /**
     * Write verification results to file
     */
    void writeResultsLogging();

    /**
    *   Sets up the signing statistics vector based on the exisitng threads
    */
    void initSignLogging();

    /**
     * Write signing statistics to file
     */
    void writeSignLogging();

    /**
    *   Sets up the misbehavior statistics vector based on the exisitng threads
    */
    void initMisbehaviorLogging();

    /**
     * Write misbehavior statistics to file
     */
    void writeMisbehaviorLogging();

    void printRxStats();
    void printTxStats();
    int setup(MessageType msgType, bool reSetup = false);
    void setupLdm();
    virtual bool pendingTillEmergency();
    virtual bool pendingTillNoEmergency();
    virtual void prepareForExit();
    std::shared_ptr<ICongestionControlListener> congCtrlListener;
    std::shared_ptr<CaControlManagerListener> cacMgrListr;
    std::shared_ptr<ICAControlManager> caControlMgr;
    double currUtil = 0.0;
    struct CACapacity currCapacity = {0};
    bool openBsmLogFile(const std::string& fullPathName); // one dedicated to bsm
    bool openLogFile(const std::string& fullPathName);
    void writeLogHeader(FILE *fp);
    /**
     * Function that calculates incoming RX rate(the number of received packets per second) and
     * updates verifcation load to TM
     */
    void tmCommunication();

    /*********************************************************************************
     * data members.
     ********************************************************************************/
    Config configuration;
    CongCtrlConfig congCtrlConfig;

    /**
    * Instance of RadioReceive for simulations. Only allocated for simulation options.
    */
    unique_ptr<RadioReceive> simReceive;
    /**
    * Instance of RadioTransmit for simulations. Only allocated for simulation options.
    */
    unique_ptr<RadioTransmit> simTransmit;

    /**
     * Message used for simulation
     */
    shared_ptr<msg_contents> rxSimMsg;
    shared_ptr<msg_contents> txSimMsg;

    /**
    * BSMs STL vector of messages that have been decoded. This method gets replaced
    * with the LDM so the acpplication is no longer memoryless.
    */
    vector<shared_ptr<msg_contents>> receivedContents;

    /**
    * BSMs STL vector of host vehicle where data gets populated and
    * sent in an event flow. Each index matches the index of the event flow.
    * Meaning that there is one BSM already allocated for flows that have been opened.
    */
    vector<shared_ptr<msg_contents>> eventContents;

    /**
    * BSMs STL vector of host vehicle where data gets populated and
    * sent in a sps flow. Each index matches the index of the sps flow.
    * Meaning that there is one BSM already allocated for flows that
    * have been opened.
    */
    vector<shared_ptr<msg_contents>> spsContents;

    /**
    * Vector of RadioReceive instances that holds all data and meta
    * data of a Rx Subscription from the Snaptel SDK Radio Interface.
    */
    vector<RadioReceive> radioReceives;
    /**
    * Vector of RadioTransmit instances that holds all data and meta
    * data of an Event Flow from the Snaptel SDK Radio Interface.
    */
    vector<RadioTransmit> eventTransmits;
    /**
    * Vector of RadioTransmit instances that holds all data and meta
    * data of a Sps Flow from the Snaptel SDK Radio Interface.
    */
    vector<RadioTransmit> spsTransmits;
    /**
    * Object that represents the LDM were BSMs get stored.
    */
    Ldm* ldm = nullptr;

    /**
     * @brief Function that filters RV based on the filter rate produced by Throttle Manager.
     * @param rate
     */
    void setL2RvFilteringList(int rate);

    /**
     * @brief Function that checks whether the L2 src addr is present in the map, if the l2 addr is
     * present it updates the RV specs otherwise adds the L2 addr to the map.
     * @param l2_id
     * @param rvSpec
     */
    void updateL2RvMap(uint32_t l2_id ,rv_specs* rvSpec);

    /**
     * Update the relevant host vehicle data for SQUISH to perform its calculations
     */
    void updateCongCtrlHvData();

    /**
     * Object that registers a listener to throttle manager
     * and allows to set load and get filter rate.
     */
    shared_ptr<Cv2xTmListener> cv2xTmListener;

    // congestion variables that we'd want the driver program to access
    static shared_ptr<CongestionControlUserData> congCtrlCbDataPtr;
    static CongestionControlCalculations congCtrlCbData;
    static shared_ptr<ICongestionControlManager> congestionControlManager;
    static bool cbSuccess;
    static bool congCtrlEnabled;
    static bool securityEnabled;
    static bool positionOverride;
    static double overrideLat;
    static double overrideLong;
    static double overrideHead;
    static double overrideElev;
    static double overrideSpeed;
    static void setHvLocation(shared_ptr<ILocationInfoEx>& hvLocationInfoIn);
    static bool securityInitialized;
    static unsigned int idChangeDistance;
    static uint64_t scheduledIdChangeTime;
    static int signFail;
    static int signSuccess;
    static bool exitApp;
    static bool writeLogFinish;
    static void writeLog(const uint8_t index,
    uint32_t l2SrcAddr, bool isTx, TransmitType txType, bool validPkt,
    uint64_t timestamp, uint32_t psid, uint64_t monotonicTime,
    float locPositionDop, uint16_t locNumSvUsed, uint64_t locTimeMs, uint8_t cbr,
    bsm_data* bs, double distFromRV, uint32_t RVsInRange,
    uint64_t txInterval, bool enableCongCtrl, bool congCtrlInitialized,
    std::condition_variable* writeMutexCv);
    void diagLogPktGenericInfo();
protected:
    static std::mutex hvLocUpdateMtx;
    static shared_ptr<ILocationInfoEx> hvLocationInfo;
    static shared_ptr<ILocationInfoEx> lastLocationInfoIdChange;
    bool isTx = false;
    bool isRx = false;
    bool isTxSim = false;
    bool isRxSim = false;
    MessageType MsgType;
    uint8_t msgCount;
    uint64_t txInterval;
    uint64_t lastTxTime;
    uint64_t locTimeMs_ = 0;
    float locPositionDop_ = 0.0;
    uint16_t locNumSvUsed_ = 0;
    bool enableCsvLog_ = false;
    bool enableDiagLog_ = false;
    // congestionControl cong ctrl
    static CongestionControlData congestionControlOut;
    CongestionControlCalculations qitsCongControlCalculations;
    static sem_t congCtrlCbSem;
    bool congCtrlInitialized = false;
    bool finishProgram = false;
    sem_t programSem;
    current_dynamic_vehicle_state_t* currVehState;
    unordered_map <uint32_t,rv_specs> l2RvMap;
    std::mutex l2MapMtx;
    std::condition_variable writeMutexCv;
    std::shared_ptr<QUtils> utility_ = nullptr;
    /**
     * Adjust the specified transmit interval to cv2x supported reservation period.
     * @param intervalMs user specified transmit interval in milliseconds
     */
    int adjustSpsPeriodicity(int intervalMs);

    void fillSecurity(ieee1609_2_data* secData);
    /**
     * Overloaded function to initialize the message content for transmition.
     */
    virtual bool initMsg(std::shared_ptr<msg_contents> mc, bool isRx = false) = 0;
    /**
     * Overloaded function to free the message content, counter-part of initMsg.
     */
    virtual void freeMsg(std::shared_ptr<msg_contents> mc) = 0;

    /**
     * Call radio transmit function, maybe overloaded by child class to perform
     * additional operation before calling radio tx.
     */
    virtual int transmit(uint8_t index, std::shared_ptr<msg_contents>mc,
                            int16_t bufLen, TransmitType txType);

    virtual void vehicleEventReport(bool emergent,
        const current_dynamic_vehicle_state_t* const vehicle_state = nullptr);
    /**
     * Object that holds all data and meta data of the LocationSDK
     * and allows incoming fixes from such service.
     */
    shared_ptr<KinematicsReceive> kinematicsReceive;
    std::vector<shared_ptr<ILocationListener>> locListeners;
    std::shared_ptr<LocListener> appLocListener_;

    /**
     * Security service object.
     */
    unique_ptr<SecurityService> SecService;

    /**
     * Vehicle Receive object.
     */
    VehicleReceive VehRec;
    std::atomic<bool> criticalState{false};

    static FILE *csvfp;
    static std::mutex csvMutex;
    static v2x_diag_qits_general_data generalInfo;
    static unsigned short getEventsData(const vehicleeventflags_ut *events);
    static void fillEventsData(v2x_diag_event_bit_t *eventBit, const vehicleeventflags_ut *events);
    void diagLogPktTxRx(bool isTx, TransmitType txType, const DiagLogData *logData, const bsm_data *bs);
private:
    VehicleReceive::VehicleEventsCallback cb;
    std::mutex stateMtx;
    std::condition_variable stateCv;
    std::atomic<bool> newEvent{false};
    /* For local stored v2x IP rmnet address */
    std::mutex v2xIpAddrMtx_;
    string v2xIpAddr_;


    /* method to retrieve V2X IP rmnet address from the system */
    int getSysV2xIpIfaceAddr(string& ipAddr);

    void simTxSetup(const string ipv4, const uint16_t port);
    void simRxSetup(const string ipv4, const uint16_t port);
    static uint16_t delimiterPos(string line, vector<string> delimiters);
    int loadConfiguration(char* file);
    void saveConfiguration(map<string, string> configs);
    uint32_t vehiclesInRange();

    /* congestion control functions and variables */
    void saveCongCtrlConfig (map<string, string> configs);
    int loadCongCtrlConfig (const char* file);
    void startCongCtrl();
    static void congCtrlCb(CongestionControlUserData* congestionControlUserData, bool success);
    // function to write congestion control data to file
    static void writeCongCtrlLog(char* tmpLogStr, uint32_t maxBufSize, FILE *myfp,
    CongestionControlCalculations* congestionControlCalculations, bool validPkt,
        uint16_t eventsData);
    // function to write security related data to file
    void writeSecurityLog(char* tmpLogStr, uint32_t maxBufSize, FILE *myfp);
};
#endif
