/*
// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the
// disclaimer below) provided that the following conditions are met:

//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.

//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.

//     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.

// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/**
 * @file: qimcTest.cpp
 *
 * @brief: Implements Qimc Class as a command line tool.
 *
 */

#include "qimc.hpp"
#include "json.h"

Qimc* qimc = nullptr;
bool stopReport = false;
bool isResPath = false;

/**
 * @brief Feature for periodic reports; run by a thread
 *
 */
void startPeriodicReport(uint32_t interval_ms, string resPath){
    if(!qimc){
        std::cerr << "invalid qimc\n";
        return;
    }
     FILE* reportFile;
    // open file one time to clear it
    if (isResPath)
    {
        reportFile = std::fopen(resPath.c_str(), "w");
        if(reportFile != NULL){
            std::fclose(reportFile);
        }
        reportFile = std::fopen(resPath.c_str(), "a");
    }
    else
    {
        reportFile = std::fopen(RES_FILE, "w");
        if(reportFile != NULL){
            std::fclose(reportFile);
        }
        reportFile = std::fopen(RES_FILE, "a");
    }
    if(reportFile == NULL){
        std::cerr << "Periodic report log file not opened successfully. Returning.\n";
        return;
    }

    int timer_misses = 0;
    uint64_t exp;
    ssize_t s;
    int timerfd;
    struct itimerspec its = {0};
    std::cout << "Starting periodic report at " << interval_ms << "\n";
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd < 0) {
        return;
    }

    /* Start the timer */
    its.it_value.tv_sec = interval_ms / 1000;
    its.it_value.tv_nsec = (interval_ms%1000) * 1000000;
    its.it_interval = its.it_value;

    if (timerfd_settime(timerfd, 0, &its, NULL) < 0) {
        close(timerfd);
        return;
    }
    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    time_t startTime = currTime.tv_sec;
    timespec ts;
    // build default command
    string command = "{\"blob\":5, \"timestamp\": true}";
    struct json_object *jsonTmp = nullptr;
    auto ret = 0;
    json_object *reqPeriodic = nullptr;
    json_object *resPeriodic = nullptr;
    string path = "./req.json";
    reqPeriodic = json_object_from_file(path.c_str());
    // get report
    while (!stopReport){
        // should be based on an existing provided command but maybe we can just do blob for now
        // also document timestamp here and provide both timestamp off client and server to user
        resPeriodic = qimc->sendAndGetResponse(reqPeriodic);
        if(resPeriodic){
            // add the timestamp we received the response before writing to log file
            timespec_get(&ts, TIME_UTC);
            const int64_t nanoTime = (ts.tv_sec * BILLION) + ts.tv_nsec;
            json_object_object_add(resPeriodic, "timestamp_client",
                json_object_new_int64(nanoTime));
            int64_t servNanoTime = 0;
            // get the timestamp from the original response
            for (auto key : QMonitorJson::kStr)
            {
                if (json_object_object_get_ex(resPeriodic, key, &jsonTmp))
                {
                    if(strcmp(key, "timestamp") == 0){
                        servNanoTime = std::stoull(string(json_object_to_json_string(jsonTmp)));
                    }
                }
            }
            json_object_object_add(resPeriodic, "timestamp_diff",
                json_object_new_int64(nanoTime-servNanoTime));
            ret = 0;
            //convert json object to json string
            const char * jsonResStr = json_object_to_json_string(resPeriodic);
            // append to open log file
            fprintf(reportFile, "%s\n", jsonResStr);
        }
        s = read(timerfd, &exp, sizeof(exp));
        if (s == sizeof(uint64_t) && exp > 1) {
            timer_misses += (exp-1);
        }
    }
    std::fclose(reportFile);
}

int main(int argc, const char **argv)
{
    std::thread reportThr;
    string jsonResPath;
    qimc = new Qimc(Qimc::loadArgs(argc, argv));
    bool periodicReport = false;
    uint32_t reportInterval = 100;
    for (int i = 1; i < argc; i++)
    {
        const char c = argv[i][1];
        switch (c){
            case 'm':
                periodicReport = true;
                reportInterval = atoi(argv[++i]);
                break;
            case 'r':
                isResPath = true;
                jsonResPath = string(argv[++i]);
                break;
        }
    }

    if(periodicReport){
        reportThr = std::thread(startPeriodicReport, reportInterval, jsonResPath);
        // wait for input here and close correspondingly
        std::cout << "Input q or Q character and then ENTER to quit qimcTest: \n";
        char ch[2];
        while( fgets(ch, 2, stdin) != NULL ){
            if(ch[0] == 'q' || ch[0] == 'Q'){
                std::cout << "Quit key was input\n";
                break;
            }
            std::cout << "Input q or Q character and then ENTER to quit qimcTest: \n";
        }
        std::cout << "Quitting qimc\n";
        stopReport = true;
        if(reportThr.joinable()){
            reportThr.join();
        }
    }
    return 0;
}
