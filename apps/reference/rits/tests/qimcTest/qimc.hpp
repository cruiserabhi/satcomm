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
 * @file: qimc.hpp
 *
 * @brief: Monitoring Server Client for ITS Stack stats and info.
 */

#ifndef QIMC_HPP
#define QIMC_HPP

// System Includes
#include <map>
#include <list>
#include <thread>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <json.h>
#include <cstring>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
// Local Includes

#include "qimc.hpp"
#include "qMonitor.hpp"
#include "qMonitorJson.hpp"

// Local Defines

#define REQ_FILE "./req.json"
#define RES_FILE "./res.json"
#define SOCK_ERROR -1

using std::list;
using std::map;
using std::string;
using std::thread;

class Qimc
{

public:
    class Configuration
    {
    public:
        struct sockaddr_in sAddress; // Server ADdress
        bool saveRes, printRes, printReq, isResPath, isReqPath, isHelp, isClose, periodicReport;
        int sockDomain = AF_INET;
        int sockType = SOCK_STREAM;
        int sockProtocol = IPPROTO_TCP;
        string jsonReqPath, jsonResPath;
        Alert debugLevel = NO_ALERT;
        Alert logLevel = NO_ALERT;
        uint32_t reportInterval = 100; // default is 100 ms

        Configuration(const char charAddr[] = DEFAULT_ADDRESS,
                      const int port = DEFAULT_PORT)
        {

            this->sAddress.sin_family = this->sockDomain;
            this->sAddress.sin_port = htons(port);
            inet_aton(charAddr, &sAddress.sin_addr);
        }

        ~Configuration()
        {
        }
    };
    /**
     * @brief Construct a new qimc object
     *
     * @param conf object that holds several qimc attributes.
     */
    Qimc(const Configuration conf);

    /**
     * @brief Destroy the qimc object
     *
     */
    ~Qimc();

    /**
     * @brief Loads arguments from command lines.
     *
     * @param argc int, number of arguments.
     * @param argv char**, array of char* of argc size.
     * @return Configuration Qimc configuration object.
     */
    static Configuration loadArgs(int argc, const char **argv);

    /**
     * @brief send json request to qmonitor and get response
     *
     * @param json_object*, json command to send
     * @return json_object*, json response.
     */
    json_object* sendAndGetResponse(json_object* command);
private:
    Configuration config;
    json_object *req = nullptr;
    json_object *res = nullptr;
    int clientSock = SOCK_ERROR;
    char buffer[MAX_BUFFER_SIZE];
    QMonitorData resData = {0};

    /**
     * @brief Sends json request to qMonitor Server.
     *
     * @param req json object with request
     * @return json_object* json response from request
     */
     json_object *sendReq(json_object *req);

    /**
     * @brief Parses server response and prints it human readable
     *
     * @param res from server
     * @return int 0 if success, else fails
     */
     int parseRes(json_object *res);

    /**
     * @brief
     *
     * @param key
     * @param obj
     * @return int 0 if success, else fails
     */
    int parseKey(const char *key, json_object *obj);

    int printKey(const char *key, json_object *obj);
    /**
     * @brief Connects to qMonitor server
     *
     * @return int 0 if success, else fails.
     */
    int connectServer();

    /**
     * @brief Changes configuration options in Qimc
     *
     * @return int 0 if success, else error.
     */
    int changeConfig();

    /**
     * @brief Prints Qimc client usage
     *
     */
    static void printUsage();
};

#endif