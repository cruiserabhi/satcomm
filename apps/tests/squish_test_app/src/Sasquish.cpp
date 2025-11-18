/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "Sasquish.hpp"
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <memory>
extern "C" {
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
}
#include <telux/common/Version.hpp>
#include "../../common/utils/Utils.hpp"

using namespace telux;
using namespace telux::cv2x::prop;
using namespace telux::common;

/**
* @file       Sasquish.cpp
* @brief      Sasquish is a primary test tool for CongestionControl related
*             functionality in the SQUISH library.
* This application demonstrates how to get the default subscription and listen to
* the subscription changes. The steps are as follows:
*
*  1. Get a CongestionControlFactory instance.
*  2. Get a ICongestionControlManager instance from CongestionControlFactory.
*  3. Register the listener which will receive updates whenever Congestion Control
*     changes are ready to be consumed.
*  4. Read data from an input CSV file.
*  5. Feed data to the CongestionControlManager. Each data representing data of a vehicle.
*  6. Finally, when all data is fed and logs (if any) are written.
*     When the use case is over, deregister the listener.
*/

// static and global variables
sem_t SasquishUtils::programSem;
sem_t SasquishUtils::testSem;
sem_t SasquishUtils::logSem;
static bool stopApp;
static uint64_t timeSinceStart;
int hvDataCnt = 0;
int rvDataCnt = 0;
// for logging
uint64_t lastPeriodicity_ = 100;
uint32_t rvsInRange = 0;
bsm_data hostBsmData = {0};
uint32_t l2SrcAddr = 0;
uint64_t txInterval = 0;
uint64_t lastTxTime = 0;
bool initMsgCount = false;
bool firstHvDataSeen = false;
    bool writeRxLogs_ = false;

std::vector<SasquishTestData>::iterator currHvData;
/**
 * signalHandler - handler function for signals
 * @param [in] signal
 * @returns void
 */
static void signalHandler(int signal) {
    stopApp = true;
    if(SasquishUtils::getSasquishVerbose()){
        std::cout << "Stopping test app\n";
    }
    exit(0);
}

/**
 * setupSignalHandler - sets up signal handler with specific signals
 * @returns void
 */
static void setupSignalHandler() {
    signal(SIGINT, signalHandler);
}

 /**
 * writeGeneralLog - writes a row into the log file based on vehicle and cong ctrl data
 * @param [in] tmpLogStr
 * @param [in] maxBufSize
 * @param [in] bs
 * @param [in] myfp
 * @param [in] timeStamp
 * @param [in] monotonicTime
 * @param [in] realworldTimeNow
 * @param [in] locPositionDop
 * @param [in] locNumSvUsed
 * @param [in] gnssTime
 * @param [in] cbr
 * @param [in] txInterval
 * @param [in] l2SrcAddr
 * @returns int - negative 1 if failure, otherwise success
 */
 static int writeGeneralLog(char* tmpLogStr, uint32_t maxBufSize, bsm_data* bs, FILE *myfp,
        bool isTx, uint64_t periodicityMs, bool validPkt, uint32_t RVsInRange,
        const char* timeStamp, uint64_t monotonicTime, uint64_t realworldTimeNow,
        float locPositionDop, uint16_t locNumSvUsed, uint64_t gnssTime, uint8_t cbr,
        uint64_t txInterval, uint32_t l2SrcAddr){
    if(!tmpLogStr){
        return -1;
    }
    if(!bs){
        return -1;
    }
    if(isTx){
        snprintf(tmpLogStr, maxBufSize,
        "%s,%" PRIu64 ",%" PRIu64 ",%s,,%d,%d,%" PRIu64 ",%d,%d,0,%d,%f,%f,%f,%f,%f,%f,%f",
            timeStamp, realworldTimeNow, monotonicTime,
            "Tx", cbr, 0,
            txInterval, bs->MsgCount, bs->id,
            bs->secMark_ms, (bs->Latitude / 10000000.0), (bs->Longitude / 10000000.0),
            (bs->SemiMajorAxisAccuracy / 20.0), ((bs->Speed / 50.0) * 3.6),
            ( bs->Heading_degrees * 0.0125), (bs->AccelLon_cm_per_sec_squared / 100.0),
             ( bs->AccelLat_cm_per_sec_squared / 100.0));
    }else{
        snprintf(tmpLogStr, maxBufSize,
        "%s,%" PRIu64 ",%" PRIu64 ",%s,%08x,0,0,0,%d,%d,0,%d,%f,%f,%f,%f,%f,%f,%f",
            timeStamp, realworldTimeNow, monotonicTime,
            "Rx", l2SrcAddr, bs->MsgCount, bs->id,
            bs->secMark_ms, (bs->Latitude / 10000000.0), (bs->Longitude / 10000000.0),
            (bs->SemiMajorAxisAccuracy / 20.0), ((bs->Speed / 50.0) * 3.6),
            ( bs->Heading_degrees * 0.0125), (bs->AccelLon_cm_per_sec_squared / 100.0),
             ( bs->AccelLat_cm_per_sec_squared / 100.0));
    }

    return 1;
}

 /**
 * updateHostBsmData
 * @param [in] id
 * @param [in] l2SrcAddr
 * @param [in] hvSasquishData
 * @returns void
 */
static void updateHostBsmData(uint32_t id, uint32_t l2SrcAddr,
        SasquishTestData* hvSasquishData){
    hostBsmData.id = id;
    hostBsmData.Speed = hvSasquishData->vehData.speed;
    hostBsmData.Latitude =
        (signed int)(hvSasquishData->vehData.pos.posLat * 10000000.0);
    hostBsmData.Longitude =
        (signed int)(hvSasquishData->vehData.pos.posLong * 10000000.0);
    hostBsmData.Heading_degrees =
        (unsigned int)(hvSasquishData->vehData.pos.heading / 0.0125);
    hostBsmData.timestamp_ms = hvSasquishData->vehData.rxTimeStamp;
    if(!initMsgCount){
        hostBsmData.MsgCount = hvSasquishData->vehData.currMsgCnt;
        initMsgCount = true;
    }
}

 /**
 * updateBsmData
 * @param [in] id
 * @param [in] l2SrcAddr
 * @param [in] sasquishData
 * @param [in] bsmData
 * @returns void
 */
static void updateBsmData(uint32_t id, uint32_t l2SrcAddr,
        SasquishTestData* sasquishData, bsm_data* bsmData){
    bsmData->id = id;
    bsmData->Speed = sasquishData->vehData.speed;
    bsmData->Latitude =
        (signed int)(sasquishData->vehData.pos.posLat * 10000000.0);
    bsmData->Longitude =
        (signed int)(sasquishData->vehData.pos.posLong * 10000000.0);
    bsmData->Heading_degrees =
        (unsigned int)(sasquishData->vehData.pos.heading / 0.0125);
    bsmData->timestamp_ms = sasquishData->vehData.rxTimeStamp;
    bsmData->MsgCount = sasquishData->vehData.currMsgCnt;
}

 /**
 * updateHostVehDataThr
 * @param [in] hostVehDataInt
 * @param [in] congestionControlManager
 * @returns void
 */
static void updateHostVehDataThr(long long hostVehDataInt,
    std::shared_ptr<ICongestionControlManager> congestionControlManager){
    // assumes that data is periodically updated by interval input
    // input is ms but converted to ns
    int timerMiss = 0;
    uint64_t expirations = 0;
    ssize_t s = sizeof(uint64_t);
    int txCtrlTimerFd = SasquishUtils::createTimer(hostVehDataInt*1000000);
    if (txCtrlTimerFd == -1){
        return;
    }

    while(!stopApp){
        congestionControlManager->updateHostVehicleData(
            currHvData->vehData.pos,
            currHvData->vehData.speed);
        s = read(txCtrlTimerFd, &expirations, sizeof(uint64_t));
        if (s == sizeof(uint64_t) && expirations > 1) {
            timerMiss += (expirations - 1);
        }
    }
}

 /**
 * writeRVDataToLog
 * @param [in] timeSinceStart
 * @param [in] sasquishRvData
 * @param [in] sasquishOutputHandler
 * @returns void
 */
static void writeRVDataToLog(uint64_t timeSinceStart,
    SasquishTestData* sasquishRvData,
    std::shared_ptr<SasquishOutputHandler> sasquishOutputHandler){
    // this function will attempt to log to a user provided file path
    if (!sasquishRvData || !sasquishOutputHandler) {
        return;
    }

    // create the string
    std::stringstream tmpSs;
    char tmpLogStr[500];
    FILE * myfp;
    uint64_t realWorldTime = 0;
    timeSinceStart = SasquishUtils::getTimeStampMs();
    bsm_data bsmData = {0};
    updateBsmData(sasquishRvData->id, sasquishRvData->l2SrcAddr,
        sasquishRvData, &bsmData);
    writeGeneralLog(tmpLogStr, 200, &bsmData, myfp, false, 0, true,
        rvsInRange, SasquishUtils::getCurrentTimestampStr().c_str(),
        timeSinceStart, realWorldTime, 0.0, 0, 0, 0, 0,
            sasquishRvData->l2SrcAddr);
    tmpSs << tmpLogStr << ",";
    tmpSs << ","; // track err
    tmpSs << ","; // veh dens
    tmpSs << ","; // cqi
    tmpSs << "1"; // bsm valid
    tmpSs << ",,,,,"; // max itt, gps time, events, rand time, hyster
    tmpSs << ",\n"; //total rvs, distance from rv
    // write the string to the end of the file
    sasquishOutputHandler->writeLineToFile(tmpSs.str());
}

 /**
 * loadCongCtrlDataFn
 * @param [in] congestionControlManager
 * @param [in] sasquishTestData
 * @param [in] sasquishOutputHandler
 * @returns void
 */
// each thread will read the data from the input log file and feed into SQUISH
static void loadCongCtrlDataFn(
        std::shared_ptr<ICongestionControlManager> congestionControlManager,
        std::vector<SasquishTestData> sasquishTestData,
        std::shared_ptr<SasquishOutputHandler> sasquishOutputHandler){
    std::vector<SasquishTestData>::iterator dataIter = sasquishTestData.begin();
    long unsigned int i = 0;
    uint64_t lastTimeStamp = 0;
    uint64_t nextTimeStamp = 0;
    Position pos = {0};
    unsigned int rvTmpId = 0;
    pos.posLat = 0.0;
    pos.posLong = 0.0;
    pos.heading = 0.0;
    pos.elev = 0.0;
    congestionControlManager->updateHostVehicleData(pos,0);
    while(i < sasquishTestData.size() && !stopApp && dataIter !=
        sasquishTestData.end()){
        if (i == (sasquishTestData.size() / 2)) {
            std::cout << "Halfway through the provided data...\n";
        }
        // even though this is rx time stamp
        // we fill it for both hv and rv data from input log
        nextTimeStamp = dataIter->vehData.rxTimeStamp;
        if(lastTimeStamp && (nextTimeStamp > lastTimeStamp)){
            usleep(((unsigned int)(nextTimeStamp - lastTimeStamp)) * 1000);
        }
        switch (dataIter->dataType){
            case hostVehicleData:
                pos.posLat = dataIter->vehData.pos.posLat;
                pos.posLong = dataIter->vehData.pos.posLong;
                pos.heading = dataIter->vehData.pos.heading;
                pos.elev = dataIter->vehData.pos.elev;
                if(SasquishUtils::getSasquishVerbose()){
                    std::cout << " Host vehicle data \n";
                    std::cout << " hv data\n";
                    std::cout << "rx time stamp of data is: " <<
                        dataIter->vehData.rxTimeStamp << "\n";
                    std::cout << "msg cnt of data is: " <<
                        dataIter->vehData.currMsgCnt  << "\n";
                    std::cout << "id of data is: " << dataIter->id  << "\n";
                    std::cout << "pos lat of data is: " <<
                        dataIter->vehData.pos.posLat << "\n";
                    std::cout << "pos long of data is: " <<
                        dataIter->vehData.pos.posLong  << "\n";
                    std::cout << "speed of data is: " <<
                        dataIter->vehData.speed << "\n";
                    std::cout << "heading of data is: " <<
                        dataIter->vehData.pos.heading << "\n";
                }
                // update host data to be latest each time we see HV data
                updateHostBsmData(dataIter->id, dataIter->l2SrcAddr,
                    &(*dataIter));
                currHvData = dataIter;
                congestionControlManager->updateHostVehicleData(
                    dataIter->vehData.pos,
                    dataIter->vehData.speed);
                 if(!firstHvDataSeen){
                     firstHvDataSeen = true;
                     std::thread hvDataFeedThrd
                        (updateHostVehDataThr, 100, congestionControlManager);
                    hvDataFeedThrd.detach();
                 }
                break;

            case remoteVehicleData:
                rvTmpId = dataIter->id;
                if(SasquishUtils::getSasquishVerbose()){
                    std::cout << "Remote vehicle data for ID: " << rvTmpId << "\n";
                    std::cout << "rx time stamp of data is: " <<
                        dataIter->vehData.rxTimeStamp << "\n";
                    std::cout << "msg cnt of data is: " <<
                        dataIter->vehData.currMsgCnt  << "\n";
                    std::cout << "id of data is: " << dataIter->id  << "\n";
                    std::cout << "pos lat of data is: " <<
                        dataIter->vehData.pos.posLat << "\n";
                    std::cout << "pos long of data is: " <<
                        dataIter->vehData.pos.posLong  << "\n";
                    std::cout << "speed of data is: " <<
                        dataIter->vehData.speed << "\n";
                    std::cout << "heading of data is: " <<
                        dataIter->vehData.pos.heading << "\n";
                }
                congestionControlManager->addCongestionControlData(
                    rvTmpId,dataIter->vehData.pos.posLat,
                    dataIter->vehData.pos.posLong,
                    dataIter->vehData.pos.heading,
                    dataIter->vehData.speed,
                    dataIter->vehData.rxTimeStamp,
                    dataIter->vehData.currMsgCnt);
                    if(writeRxLogs_){
                        writeRVDataToLog(timeSinceStart, &(*dataIter),
                            sasquishOutputHandler);
                    }
                break;

            case eventData:
                break;
        }
        lastTimeStamp = nextTimeStamp;
        dataIter++;
        i++;
    }
    sem_post(&SasquishUtils::programSem);
    return;
}

 /**
 * writeSquishOutputToLog - Write the congestion control results to the log file.
 *                          Format should be same as the input file format.
 * @param [in] timeSinceStart
 * @param [in] congestionControlOutput
 * @param [in] sasquishOutputHandler
 * @returns void
 */
static void writeSquishOutputToLog(uint64_t timeSinceStart,
    CongestionControlCalculations* congestionControlOutput,
    std::shared_ptr<SasquishOutputHandler> sasquishOutputHandler){
    // this function will attempt to log to a user provided file path
    if (!congestionControlOutput || !sasquishOutputHandler) {
        return;
    }

    // create the string
    std::stringstream tmpSs;
    char tmpLogStr[500];
    FILE * myfp;
    uint64_t realWorldTime = 0;
    uint64_t currTxTime = SasquishUtils::getTimeStampMs();
    txInterval = currTxTime - lastTxTime;
    lastTxTime = currTxTime;
    if(initMsgCount){
        hostBsmData.MsgCount++;
        hostBsmData.MsgCount = hostBsmData.MsgCount % 128;
    }
    writeGeneralLog(tmpLogStr, 200, &hostBsmData, myfp, true, lastPeriodicity_, true,
        rvsInRange, SasquishUtils::getCurrentTimestampStr().c_str(),
        timeSinceStart, realWorldTime, 0.0, 0, 0, 0, txInterval, l2SrcAddr);
    tmpSs << tmpLogStr << ",";
    if(congestionControlOutput->trackingError > 0.0){
        tmpSs << congestionControlOutput->trackingError << ",";
    }else{
        tmpSs << ",";
    }
    if(congestionControlOutput->smoothDens > 0.0){
        tmpSs << congestionControlOutput->smoothDens << ",";
    }else{
        tmpSs << ",";
    }
    if(congestionControlOutput->channData != nullptr){
        if(congestionControlOutput->channData->channQualInd  > 0.0){
            tmpSs << congestionControlOutput->channData->channQualInd << ",";
        }else{
            tmpSs << ",";
        }
    }else{
        tmpSs << ",";
    }
    tmpSs << "1," << congestionControlOutput->maxITT;
    tmpSs << ",,,,";
    tmpSs << "," << congestionControlOutput->totalRvsInRange;
    tmpSs << ",\n";
    // write the string to the end of the file
    sasquishOutputHandler->writeLineToFile(tmpSs.str());
}

 /**
 * outputLoggerFn - Thread function that listens for signal to write a
 *                  log entry when cong ctrl tells ITS stack to send a packet
 * @param [in] sasquishOutputHandler
 * @param [in] congestionControlOutput
 * @returns void
 */
static void outputLoggerFn(std::shared_ptr<SasquishOutputHandler> sasquishOutputHandler,
                            CongestionControlCalculations* congestionControlOutput){
    lastTxTime = SasquishUtils::getTimeStampMs();
    while(!stopApp){
        sem_wait(&SasquishUtils::logSem);
        // when cong ctrl cb semaphore unblocked, we will write an entry to the csv file
        timeSinceStart = SasquishUtils::getTimeStampMs();
        if(SasquishUtils::getSasquishVerbose()){
            std::cout << "Received notification from SQUISH\n";
            std::cout << "HV TX timestamp: " << timeSinceStart << "\n";
        }
        writeSquishOutputToLog(timeSinceStart, congestionControlOutput,
                                sasquishOutputHandler);
    }
}

/**
 * Sasquish -  Sasquish ctor
 * @returns void
 */
Sasquish::Sasquish(){
    init();
}

/**
 * ~Sasquish - Sasquish dtor
 * @returns void
 */
Sasquish::~Sasquish(){
    stopApp = true;
    sem_post(&SasquishUtils::logSem);
}

/**
 * printUsage - prints usage of Sasquish
 * @returns void
 */
void Sasquish::printUsage(){
    std::cout <<
    "Mandatory Arguments:\n"
    "\t-i input log file path, -o output log file path, -r write RX logs to the log file\n"
    "Optional Arguments: \n"
    "\t-c Config file path, -n rows to read from input log file (default: all),"
    " -t number of threads (default: 1)\n"
    "\t -v Verbose Mode\n"
    "Note: Please refer to /etc/ObeConfig.conf and its SQUISH (Congestion Control) section for "
    "an example of the config file items. \n"
    "Examples:\n"
    "\tsasquish –c config_file_path –i input_log_file_path –o output_log_file_path "
    "–n 30000 –t 4\n"
    "\tFull congestion control test using input log file where it will output the results "
    "in –o argument.\n"
    "\tThere will be 4 threads acting as receiving threads of an ITS stack that will be "
    "feeding SQUISH.\n"
    "\tOnly 30000 rows will be read from –i file. Squish will be configured with "
    "‘config_file_path’ parameters.\n";
}

/**
 * init - Initializes Sasquish
 * @returns bool
 */
bool Sasquish::init(){
    auto &v2xPropFactory = V2xPropFactory::getInstance();
    congestionControlManager_ = v2xPropFactory.getCongestionControlManager();
    squishClient_ = std::make_shared<SquishClient>();
    congestionControlManager_->registerListener(squishClient_);
    return true;
}

/**
 * loadCongestionControlData
 * Called to load congestion control data from a row of vehicle data in string format.
 *
 * @param [in] iter
 * @param [in] sasquishTestData
 * @returns void
 */
void Sasquish::loadCongestionControlData(std::vector<string>::const_iterator &iter,
    SasquishTestData* sasquishTestData) {
    //SquishDataType dataType = rvData.first;
    CongestionControlData* congCtrlData = &sasquishTestData->vehData;
    sasquishTestData->vehData = {0};
    iter++;
    // should take the difference between this time stamp and previous entry later
    congCtrlData->rxTimeStamp = atoi(iter->c_str());
    iter+=3;
    // l2 src addr
    sasquishTestData->l2SrcAddr = atoi(iter->c_str());
    iter+=4;
    // msg cnt
    congCtrlData->currMsgCnt = atoi(iter++->c_str());

    //msg ID
    if(!fakeRVTempIds_){
        sasquishTestData->id = atol(iter->c_str());
    }else{
        // here we will replace with the fake id that we are currently one
        sasquishTestData->id = currFakeRVTempId;
        currFakeRVTempId++;
        currFakeRVTempId %= totalFakeRVTempIds;
        // get the last curr msg cnt of this RV
        // then increment by the gap
        if(fakeMsgCntMap.find(currFakeRVTempId) != fakeMsgCntMap.end()){
            int newFakeMsgCnt = fakeMsgCntMap[currFakeRVTempId];
            newFakeMsgCnt += msgCntGap;
            newFakeMsgCnt %= 128;

            fakeMsgCntMap.insert(std::pair<int,int>(currFakeRVTempId,
                newFakeMsgCnt));
        }else{
            fakeMsgCntMap.insert(std::pair<int,int>(currFakeRVTempId, congCtrlData->currMsgCnt));
        }
    }

    iter+=3;
    // latitude
    congCtrlData->pos.posLat = atof(iter++->c_str());
    // longitude
    congCtrlData->pos.posLong = atof(iter->c_str());
    iter+=2;
    // speed
    congCtrlData->speed = atof(iter++->c_str());
    // heading
    congCtrlData->pos.heading = atof(iter++->c_str());
        // std::cout << "msg cnt of data is: " <<congCtrlData->currMsgCnt  << "\n";
        // std::cout << "id of data is: " << sasquishTestData->id  << "\n";
    if(SasquishUtils::getSasquishVerbose()){
        std::cout << "rx time stamp of data is: " << congCtrlData->rxTimeStamp << "\n";
        std::cout << "msg cnt of data is: " <<congCtrlData->currMsgCnt  << "\n";
        std::cout << "id of data is: " << sasquishTestData->id  << "\n";
        std::cout << "l2 src addr is: " << sasquishTestData->l2SrcAddr << "\n";
        std::cout << "pos lat of data is: " << congCtrlData->pos.posLat << "\n";
        std::cout << "pos long of data is: " << congCtrlData->pos.posLong  << "\n";
        std::cout << "speed of data is: " << congCtrlData->speed << "\n";
        std::cout << "heading of data is: " << congCtrlData->pos.heading << "\n";
    }
}

/**
 * readCsvLine - reads line from csv file and separates data based on comma
 * @param [in] line
 * @param [in] sasquishTestData
 * @returns void
 */
bool Sasquish::readCsvLine(string& line, SasquishTestData* sasquishTestData) {
    char delim = ',';
    std::vector<string> tokens;
    tokens.reserve(NUM_CSV_FIELDS);

    std::size_t current, previous = 0;
    current = line.find(delim);

    while (current != string::npos) {
        tokens.emplace_back(line.substr(previous, current - previous));
        previous = current + 1;
        current = line.find(delim, previous);
    }
    tokens.emplace_back(line.substr(previous));
    vector<string>::const_iterator iter;
    iter = tokens.begin();

    // check data type
    iter+=3; // move to the record data type elememt
    string dataTypeStr = *iter++;
    if (dataTypeStr.compare("Tx") == 0 || dataTypeStr.compare("TX") == 0) {
        sasquishTestData->dataType = SquishDataType::hostVehicleData;
        hvDataCnt++;
    }
    else if (dataTypeStr.compare("Rx") == 0 || dataTypeStr.compare("RX") == 0) {
        sasquishTestData->dataType = SquishDataType::remoteVehicleData;
        rvDataCnt++;
    }
    currRow_++;
    // load data based on data type
    iter = tokens.begin();

    if(rvTransmitLossSimulation > 0){
        if((rxFail+rxSuccess) % 50 == 0){
            std::cout << "Lost " << totalSimLossPkts <<
                " packets out of " << (rxFail+rxSuccess) << " pkts \n";
            std::cout << "Should be about " << rvTransmitLossSimulation <<"\n";
            std::cout << "Current loss rate is: " <<
                ((double)totalSimLossPkts / (double)(rxFail+rxSuccess)) << "\n";
        }
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        srand(ts.tv_sec * 1000000000LL + ts.tv_nsec);
        if(rand()%100 <= rvTransmitLossSimulation){
            totalSimLossPkts++;
            rxFail++;
        }
    }else{
        loadCongestionControlData(iter, sasquishTestData);
        rxSuccess++;
    }

    return true;
}

 /**
 * loadSquishInputData
 * Read congestion control data from input log file and store in internal data struct.
 * Format should be same as the output file format.
 * @param [in] sasquishInputHandler
 * @returns void
 */
void Sasquish::loadSquishInputData(
    std::shared_ptr<SasquishInputHandler> sasquishInputHandler)
{
    int logs_read = 0;
    int threadId = 0;
    string line;
    // set random seed for choosing which thread gets which data
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand(ts.tv_sec * 1000000000LL + ts.tv_nsec);
    SasquishTestData sasquishTestData = {0};
    if (sasquishInputHandler && !stopApp) {
        // read each line and store it into a vehData instant
        string line;
        currRow_ = 0;
        // skip the first line (the headers)
        sasquishInputHandler->readLineFromFile(line);
        while (sasquishInputHandler->readLineFromFile(line)) {
            // check the first entry, then parse accordingly
            sasquishTestData = {0};
            if (not readCsvLine(line, &sasquishTestData)) {
                if(SasquishUtils::getSasquishVerbose()){
                    cerr << "Error occurred when parsing the following csv line : "
                        << line << endl;
                }
            }
            else {
                if(multithreaded_){
                   threadId = (rand() % numTestThreads_);
                   sasquishTestDataAll_[threadId].push_back(sasquishTestData);
                }else{
                    sasquishTestDataAll_[0].push_back(sasquishTestData);
                }
            }
            ++logs_read;
            if(logs_read == rowsToRead_){
                break;
            }
        }
        if(SasquishUtils::getSasquishVerbose()){
            cout << logs_read <<
                " congestionControl data entries read from file." << endl;
            cout << hvDataCnt << " hv data entries read " << endl;
            cout << rvDataCnt << " rv data entries read " << endl;
        }
    }
}

/**
 * setRowsToRead - sets number of rows to read from csv file
 * @param [in] rows
 * @returns bool
 */
bool Sasquish::setRowsToRead(int rows){
    if(rows <= 0 || rows > ROW_LIMIT){
        return false;
    }
    rowsToRead_ = rows;
    return true;
}

/**
 * setProcessingThreads - sets number of threads to provide data to congestion control lib
 * @param [in] numThreads
 * @returns bool
 */
bool Sasquish::setProcessingThreads(int numThreads){
    if(numThreads <= 0){
        return false;
    }
    // allocated vectors for each thread
    for(int i = 0; i<numTestThreads_; i++){
        std::vector<SasquishTestData> tmpSasquishTestData;
        sasquishTestDataAll_.push_back(tmpSasquishTestData);
    }
    return true;
}

/**
 * initOutputCsv - initializes the output csv format
 * @returns void
 */
void Sasquish::initOutputCsv(){
    // open output log handler
    sasquishOutputHandler_  = std::make_shared<SasquishOutputHandler>(outputCsv_);
    // write the headers at top
    // Writes log header to the csv file pointed by fp.
    std::string strHeader =
    "TimeStamp,TimeStamp_ms,Time_monotonic,"
    "LogType,L2 ID,CBR,CPU Util,"
    "TXInterval,msgCnt,TempId,GPGSAMode,"
    "secMark,lat,long,semiMajorDev,speed,"
    "heading,longAccel,latAccel,TrackingError,"
    "SmoothedVehicleDensity,CQI,"
    "BSMValid,MaxITT,GPSTime,Events,CongCtrlRandTime,SPSHysterisis,"
    "TotalRVs,DistanceFromRV"
     "\n";
    sasquishOutputHandler_->writeLineToFile(strHeader);
    if(SasquishUtils::getSasquishVerbose()){
        std::cout << "Writing to log file: " << outputCsv_ << "\n";
    }
}

 /**
 * readConfigFile - Reads and stores config items from file
 * @returns void
 */
void Sasquish::readConfigFile(){

    map <string, string> configs;
    string line;
    vector<string> delimiters = { " ", "\t", "#", "=" };
    ifstream configFile(congestionControlConfigFileName_);
    if(SasquishUtils::getSasquishVerbose()){
        std::cout << "Now opening config file and loading parameters\n";
    }
    if (configFile.is_open())
    {
        while (getline(configFile, line))
        {
            if (line[0] != '#' && !line.empty())
            {
                uint8_t pos = 0;
                uint8_t end = SasquishUtils::delimiterPos(line, delimiters);
                string key = line.substr(pos, end);
                line.erase(0, end);
                while (line[0] == ' ' || line[0] == '=' || line[0] == '\t') {
                    line.erase(0, 1);
                }
                end = SasquishUtils::delimiterPos(line, delimiters);
                string value = line.substr(pos, end);
                configs.insert(pair<string, string>(key, value));
            }
        }
        if(SasquishUtils::getSasquishVerbose()){
            std::cout << "Saving parameters into local config struct\n";
        }
        configFile.close();
    }else{
        return;
    }
    if(SasquishUtils::getSasquishVerbose()){
        std::cout << "Proceeding to find and save cong ctrl config parameters\n";
    }
    if(configs.find("congCtrlLoggingLvl") != configs.end()){
       congCtrlLoggingLvl_ = stoi(configs["congCtrlLoggingLvl"]);
        if(SasquishUtils::getSasquishVerbose()){
            std::cout << "Logging level will be: " << congCtrlLoggingLvl_ << "\n";
        }
    }
    if(configs.find("cbpMeasInterval") != configs.end()){
        congCtrlConfig_.cbpConfig.cbpInterval = stoi(configs["cbpMeasInterval"]);
    }
    if(configs.find("cbpWeightFactor") != configs.end()){
        congCtrlConfig_.cbpConfig.cbpWeightFactor = stod(configs["cbpWeightFactor"]);
    }
    if(configs.find("perInterval") != configs.end()){
        congCtrlConfig_.perConfig.packetErrorInterval =
            stoi(configs["perInterval"]);
    }
    if(configs.find("perSubInterval") != configs.end()){
        congCtrlConfig_.perConfig.packetErrorSubInterval =
            stoi(configs["perSubInterval"]);
    }
    if(configs.find("perMax") != configs.end()){
        congCtrlConfig_.perConfig.maxPacketErrorRate = stod(configs["perMax"]);
    }
    if(configs.find("maxChanQualInd") != configs.end()){
        congCtrlConfig_.cqiConfig.threshold = stod(configs["maxChanQualInd"]);
    }
    if(configs.find("vDensityWeightFactor") != configs.end()){
        congCtrlConfig_.densConfig.densWeightFactor =
            stod(configs["vDensityWeightFactor"]);
    }
    if(configs.find("vDensityCoefficient") != configs.end()){
        congCtrlConfig_.densConfig.densCoeff = stod(configs["vDensityCoefficient"]);
    }
    if(configs.find("vDensityMinPerRange") != configs.end()){
        congCtrlConfig_.densConfig.distThresh = stoi(configs["vDensityMinPerRange"]);
    }
    if(configs.find("txCtrlInterval") != configs.end()){
        congCtrlConfig_.teConfig.txCtrlInterval = stoi(configs["txCtrlInterval"]);
    }
    if(configs.find("hvTEMinTimeDiff") != configs.end()){
        congCtrlConfig_.teConfig.hvMinTimeDiff = stoi(configs["hvTEMinTimeDiff"]);
    }
    if(configs.find("hvTEMaxTimeDiff") != configs.end()){
        congCtrlConfig_.teConfig.hvMaxTimeDiff = stoi(configs["hvTEMaxTimeDiff"]);
    }
    if(configs.find("rvTEMinTimeDiff") != configs.end()){
        congCtrlConfig_.teConfig.rvMinTimeDiff = stoi(configs["rvTEMinTimeDiff"]);
    }
    if(configs.find("rvTEMaxTimeDiff") != configs.end()){
        congCtrlConfig_.teConfig.rvMaxTimeDiff = stoi(configs["rvTEMaxTimeDiff"]);
    }
    if(configs.find("teErrSensitivity") != configs.end()){
        congCtrlConfig_.teConfig.errSensitivity = stoi(configs["teErrSensitivity"]);
    }
    if(configs.find("teMinThresh") != configs.end()){
        congCtrlConfig_.teConfig.teLowerThresh = stod(configs["teMinThresh"]);
    }
    if(configs.find("teMaxThresh") != configs.end()){
        congCtrlConfig_.teConfig.teUpperThresh = stod(configs["teMaxThresh"]);
    }
    if(configs.find("txRand") != configs.end()){
        congCtrlConfig_.ittConfig.txRand = stoi(configs["txRand"]);
    }
    if(configs.find("timeAccuracy") != configs.end()){
        congCtrlConfig_.ittConfig.timeAccuracy = stoi(configs["timeAccuracy"]);
    }
    if(configs.find("minItt") != configs.end()){
        congCtrlConfig_.ittConfig.minIttThresh = stoi(configs["minItt"]);
    }
    if(configs.find("vMax_ITT") != configs.end()){
        congCtrlConfig_.ittConfig.maxIttThresh = stoi(configs["vMax_ITT"]);
    }
    if(configs.find("vRescheduleTh") != configs.end()){
        congCtrlConfig_.ittConfig.reschedThresh = stoi(configs["vRescheduleTh"]);
    }

    if (configs.find("enableSpsEnhancements") != configs.end()) {
        istringstream is(configs["enableSpsEnhancements"]);
        is >> boolalpha >> congCtrlConfig_.enableSpsEnhance;
        if (SasquishUtils::getSasquishVerbose() && congCtrlConfig_.enableSpsEnhance) {
            std::cout << "SPS Enhancements Enabled\n";
        }

        if (configs.find("spsEnhIntervalRound") != configs.end()) {
            congCtrlConfig_.spsEnhanceConfig.spsPeriodicity =
                stoi(configs["spsEnhIntervalRound"]);
        }
        if (configs.find("spsEnhDelayPerc") != configs.end()) {
            congCtrlConfig_.spsEnhanceConfig.changeFrequency =
                stoi(configs["spsEnhDelayPerc"]);
        }
        if (configs.find("spsEnhHysterPerc") != configs.end()) {
            congCtrlConfig_.spsEnhanceConfig.hysterPercent =
                stoi(configs["spsEnhHysterPerc"]);
        }
    }
    /*
     * These config items will enable faking the RV BSM Temp Ids to test different
     * congestion control scenarios.
     */
    if (configs.find("fakeRVTempIds") != configs.end()) {
        istringstream is(configs["fakeRVTempIds"]);
        is >> boolalpha >> fakeRVTempIds_;
    }
    if (configs.find("totalFakeRVTempIds") != configs.end()) {
        totalFakeRVTempIds =
            stoi(configs["totalFakeRVTempIds"]);
    }
    if (configs.find("msgCntGap") != configs.end()) {
        msgCntGap =
            stoi(configs["msgCntGap"]);
    }

    if (configs.find("rvTransmitLossSimulation") != configs.end()) {
        rvTransmitLossSimulation =
            stoi(configs["rvTransmitLossSimulation"]);
    }

    if(SasquishUtils::getSasquishVerbose()){
        std::cout << "Finished saving config parameters to local config structure\n";
    }

}

 /**
 * initSquishConfigs - initializes the config parameters of congestion control library
 * @returns bool
 */
bool Sasquish::initSquishConfigs(){
     if(!congestionControlManager_){
        std::cerr << "Congestion Control Mngr not yet created\n";
        return false;
     }

    if(congCtrlLoggingLvl_ > 0){
        CongestionControlUtility::setLoggingLevel(congCtrlLoggingLvl_);
    }
    // init configs with default values
    if(congestionControlConfigFileName_.empty()){
        congestionControlManager_->updateCongestionControlType
            (CongestionControlType::SAE);
        congestionControlManager_->updateCbpConfig(
                                DEFAULT_CBP_WEIGHT_FACTOR, DEFAULT_CBP_MEAS_INTERVAL);
        congestionControlManager_->updatePERConfig(DEFAULT_PER_MAX, DEFAULT_PER_INTERVAL,
                                DEFAULT_PER_SUBINTERVAL);
        congestionControlManager_->updateDensConfig(
                        DEFAULT_DENSITY_COEFFICIENT, DEFAULT_DENSITY_WEIGHT_FACTOR,
                        DEFAULT_MIN_PER_RANGE);
        congestionControlManager_->updateTeConfig(
                    DEFAULT_TX_RATE_CTRL_INTERVAL, DEFAULT_HV_TE_MIN_TIME_DIFF,
                    DEFAULT_HV_TE_MAX_TIME_DIFF, DEFAULT_RV_TE_MIN_TIME_DIFF,
                    DEFAULT_RV_TE_MAX_TIME_DIFF, DEFAULT_TE_MIN_THRESH,
                    DEFAULT_TE_MAX_THRESH, DEFAULT_TE_ERR_SENSITIVITY);
        congestionControlManager_->updateIttConfig(DEFAULT_RESCHED_THRESH,
                            DEFAULT_TIME_ACC, DEFAULT_MIN_ITT,
                            DEFAULT_MAX_ITT, DEFAULT_TX_RAND);
    }else{
        // read from the config file
        if(SasquishUtils::getSasquishVerbose()){
            std::cout << "Reading parameters from user-provided config file\n";
        }

        readConfigFile();
        std::shared_ptr<CongestionControlConfig> tmpConfigPtr =
            std::make_shared<CongestionControlConfig>(congCtrlConfig_);
        if(CCErrorCode::SUCCESS !=
            congestionControlManager_->updateCongestionControlConfig(tmpConfigPtr)){
                std::cerr << "Failed to update the SQUISH congestion control config params\n";
                return false;
        }
        congestionControlManager_->updateCongestionControlType
            (CongestionControlType::SAE);
    }
    return true;
}

 /**
 * parseArgs - parses arguments from command line when running sasquish
 * @param [in] argc
 * @param [in] argv
 * @returns bool
 */
bool Sasquish::parseArgs(int argc, char **argv){
    if (argc > 2){
        int idx = 1;
        //Get all options and file paths from command line instead of waiting for user input
        for (; idx < (argc - 1); idx++) {
            switch (argv[idx][1]){
                // input config file
                case 'c':
                    idx += 1;
                    congestionControlConfigFileName_ = string(argv[idx]);
                    std::cout << "Using path " << congestionControlConfigFileName_
                        << " for loading CongestionControl configuration parameters\n";

                    // check if config file provided and use that one
                    initSquishConfigs();
                    break;
                // input csv
                case 'i':
                    idx += 1;
                    inputCsv_ = string(argv[idx]);
                    std::cout << "Using input file: " << inputCsv_ << "\n";
                    break;
                // limited rows to read from csv
                case 'n':
                    idx += 1;
                    rowsToRead_ = stoi(argv[idx]);
                    std::cout << "Will read " << rowsToRead_ <<
                        " entries from the input CSV file\n";
                    break;
                // output csv
                case 'o':
                    idx += 1;
                    outputCsv_ = string(argv[idx]);
                    std::cout << "Using output file: " << outputCsv_ << "\n";
                    break;
                // write received log entries to csv
                case 'r':
                    writeRxLogs_ = true;
                    break;
                // multithreaded operation for random feeding of data
                case 't':
                    multithreaded_ = true;
                    idx += 1;
                    numTestThreads_ = stoi(argv[idx]);
                    std::cout << " using " << numTestThreads_ << " threads\n";
                    break;
                // verbose mode
                case 'v':
                    std::cout << "Setting verbose mode\n";
                    SasquishUtils::setSasquishVerbose(true);
                    break;
            }
        }
    }else {
        return false;
    }

    // init output csv file
    if(!outputCsv_.empty()){
        initOutputCsv();
    }

    // init data structs for multithreaded processing
    setProcessingThreads(numTestThreads_);

    // init the data structs with data from input csv
    if(!inputCsv_.empty()){
        sasquishInputHandler_ = std::make_shared<SasquishInputHandler>(inputCsv_);
        loadSquishInputData(sasquishInputHandler_);
    }

    return true;
}

 /**
 * testCongCtrl - command to test congestion control by providing data read from csv file
 * @returns void
 */
void Sasquish::testCongCtrl(){
    // if multithreaded, preallocate the vectors assigned to each thread
    sem_init(&SasquishUtils::programSem, 0, 0);
    sem_init(&SasquishUtils::logSem, 0, 0);
    squishClient_->setDataReadySemaphore(&SasquishUtils::logSem);
    stopApp = false;
    if(SasquishUtils::getSasquishVerbose()){
        std::cout << "Creating the threads and detaching them\n";
    }
    std::thread outputLoggerThrd(outputLoggerFn, sasquishOutputHandler_,
        congestionControlManager_->getCongestionControlUserData()->
            congestionControlCalculations.get());
    outputLoggerThrd.detach();
    if(SasquishUtils::getSasquishVerbose()){
        std::cout << "Starting congestion control in background\n";
    }
    if(!congestionControlManager_){
        std::cerr << "Congestion control manager not yet created\n";
        return;
    }
    congestionControlManager_->startCongestionControl();
    // make rx threads that will add data from the rx based on the offsets
    // it calculated for its rows
    if(multithreaded_){
        for(long unsigned int i = 0; i < sasquishTestDataAll_.size(); i++){
            if(SasquishUtils::getSasquishVerbose()){
                std::cout << "Detaching thread " << i << "\n";
                std::cout << "size of this threads vector is: "
                    << sasquishTestDataAll_[i].size() << "\n";
            }
            std::thread feedThrd
                (loadCongCtrlDataFn, congestionControlManager_,
                sasquishTestDataAll_[i], sasquishOutputHandler_);
            feedThrd.detach();
        }
    }else{
        // function that will feed the congestionControl threads
        // with the data it needs to calculate when to send next
        std::thread feedThrd(loadCongCtrlDataFn, congestionControlManager_,
            sasquishTestDataAll_[0], sasquishOutputHandler_);
        feedThrd.detach();
    }
    sleep(1);
    sem_wait(&SasquishUtils::programSem);
    stopApp = true;
    congestionControlManager_->stopCongestionControl();
    if(SasquishUtils::getSasquishVerbose()){
        std::cout << "Feeding from input file completed. "
                    "Now ending Sasquish Test App.\n";
        std::cout << "-----------------------------------------\n-";
    }
}

 /**
 * main - driver function which will parse command line arguments, create sasquish, and
 *      run sasquish for testing the Telsdk congestion control library
 * @param [in] argc
 * @param [in] argv
 * @returns int
 */
int main(int argc, char* argv[]) {
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::string appName = "SASQUISH Test App - SDK v" + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch) +"\n" +
                          "Release name: " + sdkReleaseName;
    setupSignalHandler();

    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "locclient", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cerr << "Adding supplementary groups failed!" << std::endl;
    }

    // create the Sasquish instance
    std::shared_ptr<Sasquish> sasquish;
    sasquish = std::make_shared<Sasquish>();

    // make sure it was successful
    if(!sasquish){
        std::cerr << "Something wrong happened with creation of Sasquish instance\n";
        return -1;
    }

    // read in parameters from command line
    if(argc < 2 || !sasquish->parseArgs(argc, argv)){
        sasquish->printUsage();
        return -1;
    }
    // now process the input log file
    // each thread will act on their own vector and have their own timeline
    // they will read through their vector and provide data to squish
    stopApp = false;
    sasquish->testCongCtrl();
    return 0;
}