/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SASQUISH_HPP
#define SASQUISH_HPP

#include <telux/cv2x/prop/CongestionControlDefines.hpp>
#include <telux/cv2x/prop/CongestionControlManager.hpp>
#include <telux/cv2x/prop/V2xPropFactory.hpp>
#include <telux/common/CommonDefines.hpp>
#include "SquishClient.hpp"
#include "SquishControlMenu.hpp"
#include <sys/timerfd.h>
#include <vector>
#include <map>
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
#include <unistd.h>

using namespace telux;
using namespace telux::cv2x::prop;
using namespace telux::common;

/**
* @file       Sasquish.hpp
* @brief      Sasquish is a primary test tool for CongestionControl related
*             functionality in the SQUISH library.
*
*/
class Sasquish {
public:
    int currRow_ = 0;
    int rowsToRead_ = ROW_LIMIT;
    bool multithreaded_ = false;
    int numTestThreads_ = 1;
    bool setRowsToRead(int rows);
    bool setProcessingThreads(int numThreads);
    Sasquish();
    ~Sasquish();
    void printUsage();
    bool init();
    bool parseArgs(int argc, char **argv);
    void nonInteractiveLaunch();
    void cleanup();
    void loadCongestionControlData(
        std::vector<string>::const_iterator &iter, SasquishTestData* sasquishTestData);
    bool readCsvLine(string& line, SasquishTestData* sasquishTestData);
    void loadSquishInputData(std::shared_ptr<SasquishInputHandler> sasquishInputHandler);
    bool initSquishConfigs();
    void testCongCtrl();
private:
    void initConsole();
    void initOutputCsv();
    void readConfigFile();
    // Instance of all menu created are stored to maintain parallel running streams
    std::shared_ptr<SquishControlMenu> squishControlMenu_;
    std::shared_ptr<SquishControlMenu> squishFeatureControlMenu_;
    std::shared_ptr<SquishClient> squishClient_;
    std::shared_ptr<ICongestionControlManager> congestionControlManager_;
    std::shared_ptr<SasquishOutputHandler> sasquishOutputHandler_;
    std::shared_ptr<SasquishInputHandler> sasquishInputHandler_;
    std::vector<std::vector<SasquishTestData>> sasquishTestDataAll_;
    int congCtrlLoggingLvl_ = 0;
    bool fakeRVTempIds_ = false;
    int totalFakeRVTempIds = 500;
    int msgCntGap = 1;
    int currFakeRVTempId = 0;
    int rvTransmitLossSimulation = 0;
    int totalSimLossPkts = 0;
    int rxFail = 0;
    int rxSuccess = 0;
    std::map <int, int> fakeMsgCntMap; // id and msg cnt
    // Structure instance to store the command line args passed
    SasquishArguments commandlineArgs_;
    CongestionControlConfig congCtrlConfig_;
    string inputCsv_;
    string outputCsv_;
    string congestionControlConfigFileName_;
};

#endif  // SASQUISH_HPP