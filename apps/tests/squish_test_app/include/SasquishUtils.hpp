/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SASQUISHUTILS_HPP
#define SASQUISHUTILS_HPP

#define NANOSECONDS_IN_MILLISEC 1000000
#include <telux/cv2x/prop/CongestionControlDefines.hpp>
#include <telux/cv2x/prop/CongestionControlManager.hpp>
#include <telux/cv2x/prop/V2xPropFactory.hpp>
#include <telux/common/CommonDefines.hpp>
#include <inttypes.h>
#include <sys/timerfd.h>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <thread>
#include <stdio.h>      /* printf, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

using namespace std;
using namespace telux;
using namespace telux::cv2x::prop;
using namespace telux::common;

// definitions and commands

/**
 * Default channel busy percentage parameter values.
 */
#define DEFAULT_CBP_MEAS_INTERVAL 100 // ms
#define DEFAULT_CBP_WEIGHT_FACTOR 0.5

/**
 * Default packet error rate parameter values.
 */
#define DEFAULT_PER_INTERVAL 5000
#define DEFAULT_PER_SUBINTERVAL 1000
#define DEFAULT_PER_MAX 0.3
#define DEFAULT_MIN_CHAN_QUAL_IND 0.0
#define DEFAULT_MAX_CHAN_QUAL_IND 0.3

/**
 * Default density parameter values.
 */
#define DEFAULT_DENSITY_WEIGHT_FACTOR 0.05
#define DEFAULT_DENSITY_COEFFICIENT 25
#define DEFAULT_MIN_PER_RANGE 100

/**
 * Default tracking error and inter-transmit time parameter values.
 */
#define DEFAULT_TX_RATE_CTRL_INTERVAL 100
#define DEFAULT_HV_TE_MIN_TIME_DIFF 0
#define DEFAULT_HV_TE_MAX_TIME_DIFF 200
#define DEFAULT_RV_TE_MIN_TIME_DIFF 0
#define DEFAULT_RV_TE_MAX_TIME_DIFF 3000
#define DEFAULT_TE_ERR_SENSITIVITY 75
#define DEFAULT_TE_MIN_THRESH 200
#define DEFAULT_TE_MAX_THRESH 500
#define DEFAULT_MIN_ITT 100
#define DEFAULT_MAX_ITT 600
#define DEFAULT_TX_RAND 0
#define DEFAULT_TIME_ACC 1000
#define DEFAULT_RESCHED_THRESH 25

/**
 * Default power parameter values.
 */
#define DEFAULT_SUPRA_GAIN 0.5
#define DEFAULT_MIN_CHAN_UTIL 50
#define DEFAULT_MAX_CHAN_UTIL 80
#define DEFAULT_MIN_RADIATED_PWR 10
#define DEFAULT_MAX_RADIATED_PWR 20

/**
 * Default sps enhancements parameter values.
 */
#define DEFAULT_SPS_INTERVAL_ROUNDING 100
#define DEFAULT_SPS_INTERVAL_MIN_20 20
#define DEFAULT_SPS_INTERVAL_MIN_50 50
#define DEFAULT_SPS_INTERVAL_MAX 1000
#define DEFAULT_SPS_HYSTER_PERCENT 5
#define DEFAULT_SPS_DELAY_PERCENT 20

/**
 * Default miscellaneous parameter values.
 */
#define DEFAULT_MAX_MSG_CNT 128
#define MAX_CRIT_EVENT_TX 5
#define NANOSECONDS_IN_MILLISEC 1000000
#define NANOSECONDS_IN_SEC 1000000000
#define MAX_TIMESTAMP_BUFFER_SIZE 80

const unsigned int NUM_CSV_FIELDS = 30;
const int ROW_LIMIT = 50000;
const int MAX_THREADS = 8;
const uint16_t MAX_DELIMIT_VALUE = 65535;
const std::string appName_ = "CongestionControlTestApp";
const std::string MENU_DIVIDER = "------------------------------------------------";
const std::string CURSOR = "-> ";
const string setDensity = "Set_Density_Value";
const string setDensityConfig = "Set_Density_Config";
const string setDistanceThresh = "Set_Distance_Threshold";
const string setCongestionControlConfigFile = "Set_CongestionControl_Config_File";
const string setLoggingCsvFile = "Set_Logging_Csv_File";
const string setCbr = "Set_Cbr_Value";
const string setVehDataCsvFileName = "Set_Veh_Data_Csv_File_Name";
const string startCongestionControlCmd = "Start_Congestion_Control";
// testing options
const string setMsgCountBasedTest = "Set_Msg_Count_Based_Test";
const string startBasicUnitTests = "Start_Basic_Unit_Tests";
const string testSpecificFunction = "Test_Specific_Function";

/*
 * To help differentiate what logging to do
 */

enum SasquishLogType{
  InputSasquishLog,
  OutputSasquishLog
};

/*
 * Enum for type of data log when parsing/writing logs.
 */
enum SquishDataType {
    hostVehicleData,
    remoteVehicleData,
    eventData
};

/*
 * Enum for various Sasquish arguments
 */
enum SasquishArguments {
    setCongestionControlConfigFile_t,
    setVehDataCsvFileName_t,
    setLoggingCsvFile_t,
    startCongestionControl_t,
    startBasicUnitTests_t,
    testSpecificFunction_t
};

/*
 * Struct for testing functionalities of SQUISH
 */
struct SasquishTestResultData {
    uint64_t rvMsgTotalCount;
    uint64_t expectHVTEInducedMsgs;
    uint64_t expectHVCritEventMsgs;
};


/*
 * Struct for testing functionalities of SQUISH
 */
struct SasquishTestData {
    uint64_t id;
    uint32_t l2SrcAddr;
    SquishDataType dataType;
    CongestionControlData vehData;
};

struct bsm_data{
    uint64_t timestamp_ms;      // UTC Timestamp in milliseconds when bsm was creatd. computed from secmark_ms
    unsigned int MsgCount;      // Ranges from 1 - 127 in cyclic fashion.
    unsigned int id;            // 32 bit identifier
    unsigned int secMark_ms;    // No of milliseconds in a minute
    signed int   Latitude;      // Degrees * 10^7
    signed int   Longitude;     // Degrees * 10^7
    signed int   Elevation;     // Meters * 10
    double distFromRV;
    unsigned int SemiMajorAxisAccuracy;         // val * 20
    unsigned int SemiMinorAxisAccuracy;         // val * 20
    unsigned int SemiMajorAxisOrientation;      // val/0.0054932479
    unsigned int Speed;                     // value (in kmph) * 250/18
    unsigned int Heading_degrees;           // value (in degrees) / 0.0125
    signed int   SteeringWheelAngle;        // value (in degree) / 1.5
    signed int   AccelLon_cm_per_sec_squared;       // value (in m/sec2) / 0.01
    signed int   AccelLat_cm_per_sec_squared;       // value (in m/sec2) / 0.01
    signed int   AccelVert_two_centi_gs;            // value in .02 G steps
    signed int   AccelYaw_centi_degrees_per_sec;        // value in degrees per second /0.01
    unsigned int VehicleWidth_cm;                   // units are 1 centimeter, at widest point, 0=unavailable
    unsigned int VehicleLength_cm;                  // units are 1 centimeter, 0=unavailable
} ;

/*
 * Class to manage any input file logging
 */
class SasquishInputHandler {
public:
    SasquishInputHandler();
    SasquishInputHandler(std::string fileName);
    ~SasquishInputHandler();
    bool openFile(std::string fileName);
    bool clearFile(std::string fileName);
    bool closeFile();
    bool readLineFromFile(std::string& line);
private:
    std::ifstream logFile;
    std::mutex logMutex;
};

/*
 * Class to handle output file logging
 */
class SasquishOutputHandler {
public:
    SasquishOutputHandler();
    SasquishOutputHandler(std::string fileName);
    ~SasquishOutputHandler();
    bool openFile(std::string fileName);
    bool clearFile(std::string fileName);
    bool closeFile();
    bool writeLineToFile(std::string line);
private:
    std::ofstream logFile;
    std::mutex logMutex;
};
/*
 * Class with generic utility functions related to
 * time, command line handling, and others.
 * Logging functionality (debug purposes) as well.
 */
class SasquishUtils {
public:
    static sem_t programSem;
    static sem_t testSem;
    static sem_t logSem;
    static bool sasquishVerbose;
    static void setSasquishVerbose(bool verbose);
    static bool getSasquishVerbose();
    static uint16_t delimiterPos(string line, vector<string> delimiters);
    static void getInput(std::string prompt, std::string &input);
    static uint64_t getTimeStampNs();
    static uint64_t getTimeStampMs();
    static int setTimerFd(int timerfd, long long interval_ns);
    static int createTimer(long long interval_ns);
    static std::string getCurrentTimestampStr();
};

#endif  // SASQUISHUTILS_HPP