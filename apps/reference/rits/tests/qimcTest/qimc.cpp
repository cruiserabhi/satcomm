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
 * @file: qimc.cpp
 *
 * @brief: Implementation of qimc (qMonitor) client Class
 *
 */

// System Includes
#include <json.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <utility>
#include <iostream>
#include <stdio.h>
#include <time.h>
// Local Includes
#include "qimc.hpp"
#include "qMonitorJson.hpp"

#define CLIENT_ERROR -1
using namespace std;

Qimc::Qimc(const Configuration conf)
{
    struct json_object *jsonTmp = nullptr;
    auto ret = 0;
    config = conf;
    config.isClose = false;
    if (config.isHelp)
    {
        return;
    }
    if (config.isReqPath)
    {
        std::cout << "Getting request from " << config.jsonReqPath << std::endl;
        req = json_object_from_file(config.jsonReqPath.c_str());
    }
    else if (!config.periodicReport)
    {
        std::cout << "Getting request from default file" << REQ_FILE << std::endl;
        req = json_object_from_file(REQ_FILE);
    }

    if (connectServer())
    {
        ret = CLIENT_ERROR;
    }
    else{

        if(!config.periodicReport){
            // a single shot command
            if (json_object_object_get_ex(req, "close", &jsonTmp))
            {
                config.isClose = true; // For when continues request happens
            }
            res = sendReq(req);
            if (config.debugLevel > LOW_ALERT || config.printReq)
            {
                cout << "Raw Response: " << json_object_to_json_string(res) << endl;
            }
            parseRes(res);
            if (config.saveRes)
            {
                ret = 0;
                if (config.isResPath)
                {
                    json_object_to_file(config.jsonResPath.c_str(), res);
                }
                else
                {
                    json_object_to_file(RES_FILE, res);
                }
                if (ret == CLIENT_ERROR)
                {
                    cout << "Error writing response\n";
                }
            }
            close(clientSock);
        }
    }
    return;
}


/**
 * @brief Parses server response and prints it human readable
 *
 * @param res from server
 * @return int 0 if success, else fails
 */
int Qimc::parseRes(json_object *res)
{
    struct json_object *jsonTmp = nullptr;
    using namespace QMonitorJson;

    // Parse Keys
    for (auto key : kStr)
    {
        if (json_object_object_get_ex(res, key, &jsonTmp))
        {
            parseKey(key, jsonTmp);
        }
    }

    // Print Values
    for (auto key : kStr)
    {
        if (json_object_object_get_ex(res, key, &jsonTmp))
        {
            printKey(key, jsonTmp);
        }
    }
    return 0;
}

int Qimc::parseKey(const char *key, json_object *obj)
{
    using namespace QMonitorJson;
    try
    {
        switch (kMap.at(key))
        {
        case TOTAL_RX:
            // std::cout << "got total rx req" << std::endl;
            resData.totalRx = json_object_get_int64(obj);
            break;
        case TOTAL_TX:
            // std::cout << "got total tx req" << std::endl;
            resData.totalTx = json_object_get_int64(obj);
            break;
        case DECODE_FAILS:
            resData.decodeFails = json_object_get_int64(obj);
            break;
        case SEC_FAILS:
            resData.secFails = json_object_get_int64(obj);
            break;
        case MBD_ALERTS:
            resData.mbdAlerts = json_object_get_int64(obj);
            break;
        case TOTAL_RVS:
            resData.totalRVs = json_object_get_int64(obj);
            break;
        case TOTAL_RSUS:
            resData.totalRSUs = json_object_get_int64(obj);
            break;
        case TX_BSMS:
            resData.txBSMs = json_object_get_int64(obj);
            break;
        case TX_SIGNED_BSMS:
            resData.txSignedBSMs = json_object_get_int64(obj);
            break;
        case RX_BSMS:
            resData.rxBSMs = json_object_get_int64(obj);
            break;
        case RX_SIGNED_BSMS:
            resData.rxSignedBSMs = json_object_get_int64(obj);
            break;
        case TIMESTAMP:
            resData.timestamp = json_object_get_int64(obj);
            break;
        case JSON_VER:
            resData.jsonVersion = string(json_object_get_string(obj));
            break;
        case QITS_VER:
            resData.qitsVersion = string(json_object_get_string(obj));
            break;
        case TELSDK_VER:
            resData.telsdkVersion = string(json_object_get_string(obj));
            break;
        case QMON_VER:
            resData.qMonVersion = string(json_object_get_string(obj));
        case BLOB:
            resData.blob = string(json_object_get_string(obj));
            break;
        default:
            std::cout << "Key doesn't exists" << std::endl;
            break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        std::cout << "Key " << key << " not found in server." << std::endl;
        return -1;
    }
    return 0;
}

int Qimc::printKey(const char *key, json_object *obj)
{
    using namespace QMonitorJson;
    long long secTime;
    long long nanoRemains;
    long long milliRemains;
    time_t t;
    try
    {
        switch (kMap.at(key))
        {
        case TOTAL_RX:
            //std::cout << key << ":\t\t" << resData.totalRx << std::endl;
            break;
        case TOTAL_TX:
            //std::cout << key << ":\t\t" << resData.totalTx << std::endl;
            break;
        case DECODE_FAILS:
            //std::cout << key << ":\t\t" << resData.decodeFails << std::endl;
            break;
        // case SEC_FAILS:
        //     resData.securityFails = json_object_get_int64(obj);
        //     break;
        // case MBD_ALERTS:
        //     resData.mbdAlerts = json_object_get_int64(obj);
        //     break;
        // case TOTAL_RVS:
        //     resData.totalRVs = json_object_get_int64(obj);
        //     break;
        // case TOTAL_RSUS:
        //     resData.totalRSUs = json_object_get_int64(obj);
        //     break;
        case TX_BSMS:
            //std::cout << key << ":\t\t" << resData.txBSMs << std::endl;
            break;
        // case TX_SIGNED_BSMS:
        //     resData.txSignedBSMs = json_object_get_int64(obj);
        //     break;
        case RX_BSMS:
            //std::cout << key << ":\t\t" << resData.rxBSMs << std::endl;
            break;
        // case RX_SIGNED_BSMS:
        //     resData.rxSignedBSMs = json_object_get_int64(obj);
        //     break;
        case TIMESTAMP:
            secTime = resData.timestamp / BILLION;
            nanoRemains = resData.timestamp - (secTime * BILLION);
            milliRemains = nanoRemains / MILLION;
            t = static_cast<time_t>(secTime);
            break;
        case JSON_VER:
            resData.jsonVersion = string(json_object_get_string(obj));
            break;
        case QITS_VER:
            resData.qitsVersion = string(json_object_get_string(obj));
            break;
        case TELSDK_VER:
            resData.telsdkVersion = string(json_object_get_string(obj));
            break;
        case QMON_VER:
            resData.qMonVersion = string(json_object_get_string(obj));
        case BLOB:
            resData.blob = string(json_object_get_string(obj));
            break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        std::cout << "Key " << key << " not found in server." << std::endl;
        return -1;
    }
    return 0;
}

Qimc::~Qimc()
{
}

/**
 * @brief Sends json request to qMonitor Server.
 *
 * @param req json object with request
 * @return json_object* json response from request
 */
json_object *Qimc::sendReq(json_object *req)
{
    auto ret = 0;
    size_t jsonLength;
    //std::cout << "Sending Request" << std::endl;
    ret = write(clientSock,
                json_object_to_json_string_length(req, 0, &jsonLength),
                jsonLength);
    if (ret == CLIENT_ERROR)
    {
        std::cout << "Error sending" << json_object_to_json_string(req) << std::endl;
    }

    ret = read(clientSock, buffer, MAX_BUFFER_SIZE);
    if (ret == CLIENT_ERROR)
    {
        if (config.debugLevel)
        {
            std::cout << "Error getting client response" << std::endl;
        }
    }
    else
    {
        // Transform buffer into json res
        res = json_tokener_parse(buffer);
    }

    return res;
}

/**
 * @brief Connects to qMonitor server
 *
 * @return int 0 if success, else fails.
 */
int Qimc::connectServer()
{
    auto ret = 0;
    clientSock =
        socket(config.sockDomain, config.sockType, config.sockProtocol);
    auto sAddrSize = sizeof(config.sAddress);
    if (clientSock == CLIENT_ERROR)
    {
        std::cout << "Error creating client socket " << std::endl;
        ret = CLIENT_ERROR;
    }
    else
    {
        auto ret = connect(clientSock, (struct sockaddr *)&config.sAddress, sAddrSize);
        if (ret == CLIENT_ERROR)
        {
            std::cout << "Error connecting to server " << std::endl;

            ret = CLIENT_ERROR;
        }
    }

    return ret;
}

/**
 * @brief Changes configuration options in Qimc
 *
 * @return int 0 if success, else error.
 */
int Qimc::changeConfig()
{
    return 0;
}

/**
 * @brief Loads arguments from command lines.
 *
 * @param argc int, number of arguments.
 * @param argv char**, array of char* of argc size.
 * @return Configuration Qimc configuration object.
 */
Qimc::Configuration Qimc::loadArgs(int argc, const char **argv)
{
    Configuration c = Configuration();
    c.isHelp = false;
    for (int i = 1; i < argc; i++)
    {
        const char confName = argv[i][1];
        //cout << "Loading arguments \n";
        switch (confName)
        {
        case 'p': // Port
            //cout << "Modifying port\n";
            c.sAddress.sin_port = htons(atoi(argv[++i]));
            break;
        case 'a': // Address
            //std::cout << "Modifying address" << std::endl;
            inet_aton(argv[++i], &c.sAddress.sin_addr);
            break;
        case 'l': // log level
            //std::cout << "Changing log level" << std::endl;
            c.logLevel = Alert(atoi(argv[++i]));
            break;
        case 'd': // debug level
            //std::cout << "Changing debug level" << std::endl;
            c.debugLevel = Alert(atoi(argv[++i]));
            break;
        case 'i':
            c.isReqPath = true;
            //std::cout << "Getting absolute path of request" << std::endl;
            c.jsonReqPath = string(argv[++i]);
            //std::cout << "Getting request from: " << c.jsonReqPath << std::endl;
        case 'o':
            c.saveRes = true;
            break;
        case 'r':
            c.isResPath = true;
            //std::cout << "Getting absolute path of response" << std::endl;
            c.jsonResPath = string(argv[++i]);
            //std::cout << "Getting request from: " << c.jsonResPath << std::endl;
            break;
        case 't':
            c.printRes = true;
            break;
        case 'u':
            c.printReq = true;
            break;
        case 'm':
            c.periodicReport = true;
            c.reportInterval = atoi(argv[++i]);
            break;
        case 'h':
            printUsage();
            c.isHelp = true;
            break;
        }
    }

    return c;
}

/**
 * @brief Prints Qimc client usage
 *
 */
void Qimc::printUsage()
{
    std::cout << "-h \t\tprints usage" << std::endl;
    std::cout << "-p <port> \t\tSets port server." << std::endl;
    std::cout << "-a <address> \t\tSets IPV4 server address" << std::endl;
    std::cout << "-l <0,1,2,3>  \t\tSets log level from 0 to 3" << std::endl;
    std::cout << "-d <0,1,2,3> \t\tSets debug level from 0 to 3" << std::endl;
    std::cout << "-i <path> \t\tAbsolute request path e.g. /usr/home/req.json\n";
    std::cout << "-o \t\tSaves response to ./res.json, change path with -p \n";
    std::cout << "-r <path> \t\tAbsolute response path e.g. /usr/home/res.json\n";
    std::cout << "-m <ms time interval) \t\tTime interval for periodic reports\n";
}

json_object* Qimc::sendAndGetResponse(json_object* reqPeriodic){
        int ret = 0;
        json_object *resPeriodic = nullptr;
        resPeriodic = sendReq(reqPeriodic);
        parseRes(resPeriodic);
        return resPeriodic;
}