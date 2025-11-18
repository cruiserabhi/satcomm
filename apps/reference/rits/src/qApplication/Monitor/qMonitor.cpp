/*
// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.

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
 * @file: qMonitor.cpp
 *
 * @brief: qMonitor Class Definition
 */
#include "qMonitor.hpp"
#include "qMonitorJson.hpp"
#include <json-c/json.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <utility>
#include <iostream>
#include <ctime>

#define SOCK_ERROR -1
#define MONITOR_INIT_VAL 0

using namespace std;

// Initializing static variables
// Local variables
bool QMonitor::isMonitoring = false;
int QMonitor::serverSock = SOCK_ERROR;
thread *QMonitor::connHandlerThread = nullptr;
QMonitor::Configuration QMonitor::config;
map<int, QMClientData> QMonitor::clientData;
map<thread::id, QMonitorData> QMonitor::tData;

/**
 * @brief Construct a new QMonitor object
 *
 * @param conf object that holds several QMonitor attributes.
 */
QMonitor::QMonitor(const Configuration conf)
{
    auto ret = 0;
    isMonitoring = 1;
    config = conf;
    printConfig();

    // Creating Socket
    serverSock =
        socket(config.sockDomain, config.sockType, config.sockProtocol);
    ret = serverSock;
    if (errorCheck(AlertInfo<int>{"Error Creating qMonitor Server Socket", ret,
                                  SOCK_ERROR}) == SOCK_ERROR)
    {
        close(serverSock);
        return;
    }

    // Adding Options
    ret = setsockopt(serverSock, config.sockLevel, config.sockOptName,
                     &config.sockOpt, sizeof(config.sockOpt));

    if (errorCheck(AlertInfo<int>{"Error Setting Option for qMonitor Server Socket",
                                  ret, SOCK_ERROR}) == SOCK_ERROR)
    {
        close(serverSock);
        return;
    }

    // Binding to address and port
    // char buffer[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &config.sockAddress.sin_addr, buffer, sizeof(buffer));
    // cout << "Port net: " << config.sockAddress.sin_port << " address: " << buffer << endl;
    // cout << "Port: " << ntohs(config.sockAddress.sin_port) << endl;
    ret = bind(serverSock, (struct sockaddr *)&config.sockAddress,
               sizeof(config.sockAddress));
    // cout << "Return from bind with value: " << ret << endl;
    if (errorCheck(AlertInfo<int>{"Error Binding qMonitor Server Socket", ret,
                                  SOCK_ERROR}) == SOCK_ERROR)
    {
        close(serverSock);
        return;
    }

    ret = listen(serverSock, config.connBacklog);
    if (errorCheck(AlertInfo<int>{"Error Listening on qMonitor Server Socket", ret,
                                  SOCK_ERROR}))
    {
        close(serverSock);
        return;
    }

    if (config.blocking)
    {
        connectionHandler();
    }
    else
    {

        connHandlerThread = new thread(connectionHandler);
        connHandlerThread->detach();
    }
}

/**
 * @brief Destroy the QMonitor object
 *
 */
QMonitor::~QMonitor()
{
    delete connHandlerThread;
}

/**
 * @brief Thread function to catch incoming qimc(s) (Qits Monitor Client)
 *
 */
void QMonitor::connectionHandler()
{
    while (isMonitoring)
    {
        QMClientData cData = {};
        struct sockaddr_in clientAddress;
        unsigned int addrSize = sizeof(clientAddress);
        const int clientSock =
            accept(serverSock, (struct sockaddr *)&clientAddress, &addrSize);

        if (errorCheck(AlertInfo<int>{"Error accepting client with Address:",
                                      clientSock, SOCK_ERROR}) != SOCK_ERROR)
        {
            std::cout << "Accepting New Client" << std::endl;
            cData.sock = clientSock;
            cData.address = clientAddress;

            if (clientData.find(clientSock) == clientData.end())
            {
                // Lock client cData as needed
                clientData.insert(make_pair(clientSock, cData));
                // Unlock as needed
                addClient(clientSock);
            }
        }
        else
        {
            std::cout << "Error Accepting Client" << std::endl;
        }
    }
    stop();
}

/**
 * @brief Adds clients to a list and creates an exclusive thread for client.
 *
 * @param info QMClientInfo with socket and address of client
 */
void QMonitor::addClient(int clientSock)
{
    QMClientData *cData = &clientData[clientSock];
    cData->bufferSize = config.bufferSize; // json_object but buffer for read.
    cData->buffer = new char[cData->bufferSize];
    // Initialize JSON object as needed.
    cData->cThread = new thread(clientHandler, clientSock);
    cData->cThread->detach();
}

/**
 * @brief Handles clients once connection is accepted
 *
 */
void QMonitor::clientHandler(int clientSock)
{
    QMClientData *cData = &clientData[clientSock];
    cData->handling = 1;
    int ret = SOCK_ERROR;
    cData->res = json_object_new_object();
    string info;
    size_t resSize = 0;
    auto count = 0;

    // In the future, make to threads, one for this while loop, and one for the
    // response

    while (cData->handling)
    {
        count++;
        ret = read(cData->sock, cData->buffer, cData->bufferSize);
        info = "Error reading data from client " + to_string(clientSock);
        if (errorCheck(AlertInfo<int>{info.c_str(), ret, SOCK_ERROR}))
        {
            info = "Closing Client " + to_string(clientSock);
            alert(AlertInfo<int>{info.c_str(), ret, SOCK_ERROR});
            cData->handling = 0;
            close(clientSock);
            clientData.erase(clientSock);
            return;
        }
        ret = processReq(clientSock);
        if (ret != SOCK_ERROR)
        {
            ret = createResponse(clientSock, cData->res);
        }
        info = "Error creating response";
        if (errorCheck(AlertInfo<int>{info.c_str(), ret, SOCK_ERROR}))
        {
            info = "Closing Client " + to_string(clientSock);
            alert(AlertInfo<int>{info.c_str(), ret, SOCK_ERROR});
            cData->handling = 0;
            close(clientSock);
            clientData.erase(clientSock);
            return;
        }
        ret = write(cData->sock,
                    json_object_to_json_string_length(cData->res, 0, &resSize),
                    resSize);
        info = "Error writing data to client " + to_string(clientSock);
        if (errorCheck(AlertInfo<int>{info.c_str(), ret, SOCK_ERROR}) == SOCK_ERROR)
        {
            info = "Closing Client " + to_string(clientSock);
            alert(AlertInfo<int>{info.c_str(), ret, SOCK_ERROR});
            cData->handling = 0;
            close(clientSock);
            clientData.erase(clientSock);
            return;
        }
        else
        {
            // Reset requests options and res
            json_object *jsonTmp = json_object_new_boolean(false);
            using namespace QMonitorJson;
            for (auto key : kStr)
            {
                if (changeOption(clientSock, key, jsonTmp))
                {
                    std::cout << "Error reseting key " << key << std::endl;
                }
                json_object_object_del(cData->res, key);
                json_object_object_del(cData->req, key);
            }
            // For debug purposes
            QMClientOptions *options = &cData->options;
            QMClientMetaOptions *metaOpts = &options->metaOptions;
            QMClientValOptions *valOpts = &options->valueOptions;
        }
    }
    close(clientSock);
    clientData.erase(clientSock);
    return;
}

/**
 * @brief Create a Response object
 *
 * @param client int which represents the client struct in the server.
 * @return true if response was created successful.
 * @return false if response fail to be created.
 */
int QMonitor::createResponse(int client, json_object *res)
{
    QMClientData *cData = &clientData[client];
    QMClientOptions *options = &cData->options;
    QMClientMetaOptions *metaOpts = &options->metaOptions;
    QMClientValOptions *valOpts = &options->valueOptions;
    QMonitorData tempData = {0};
    int ret = SOCK_ERROR;
    addThreadData(&tempData);
    using namespace QMonitorJson;
    if (valOpts->totalTx)
    {
        json_object_object_add(res, kStr[TOTAL_TX],
                               json_object_new_int64(tempData.totalTx));
    }
    if (valOpts->totalRx)
    {
        json_object_object_add(res, kStr[TOTAL_RX],
                               json_object_new_int64(tempData.totalRx));
    }
    if (valOpts->totalRSUs)
    {
        json_object_object_add(res, kStr[TOTAL_RSUS],
                               json_object_new_int64(tempData.totalRSUs));
    }
    if (valOpts->totalRVs)
    {
        json_object_object_add(res, kStr[TOTAL_RVS],
                               json_object_new_int64(tempData.totalRVs));
    }
    if (valOpts->rxFails)
    {
        json_object_object_add(res, kStr[RX_FAILS],
                               json_object_new_int64(tempData.rxFails));
    }
    if (valOpts->decodeFails)
    {
        json_object_object_add(res, kStr[DECODE_FAILS],
                               json_object_new_int64(tempData.decodeFails));
    }
    if (valOpts->securityFails)
    {
        json_object_object_add(res, kStr[SEC_FAILS],
                               json_object_new_int64(tempData.secFails));
    }
    if (valOpts->mbdAlerts)
    {
        json_object_object_add(res, kStr[MBD_ALERTS],
                               json_object_new_int64(tempData.mbdAlerts));
    }
    if (valOpts->txBSMs)
    {
        json_object_object_add(res, kStr[TX_BSMS],
                               json_object_new_int64(tempData.txBSMs));
    }
    if (valOpts->txSignedBSMs)
    {
        json_object_object_add(res, kStr[TX_SIGNED_BSMS],
                               json_object_new_int64(tempData.txSignedBSMs));
    }
    if (valOpts->rxBSMs)
    {
        json_object_object_add(res, kStr[RX_BSMS],
                               json_object_new_int64(tempData.rxBSMs));
    }
    if (valOpts->rxSignedBSMs)
    {
        json_object_object_add(res, kStr[RX_SIGNED_BSMS],
                               json_object_new_int64(tempData.rxSignedBSMs));
    }
    if (metaOpts->timeFrame)
    {
        // TODO only for future json stream
    }

    if (metaOpts->monitorRate)
    {
        // TODO only for future json stream
    }
    if (metaOpts->timestamp)
    {
        timespec ts;
        timespec_get(&ts, TIME_UTC);
        const int64_t nanoTime = (ts.tv_sec * BILLION) + ts.tv_nsec;
        json_object_object_add(res, kStr[TIMESTAMP],
                               json_object_new_int64(nanoTime));
    }
    if (metaOpts->jsonVersion)
    {
        json_object_object_add(res, kStr[JSON_VER],
                               json_object_new_string(JSON_VERSION));
    }

    if (metaOpts->qitsVersion)
    {
        json_object_object_add(res, kStr[QITS_VER],
                               json_object_new_string(QITS_VERSION));
    }

    if (metaOpts->telsdkVersion)
    {
        json_object_object_add(res, kStr[TELSDK_VER],
                               json_object_new_string(TELSDK_VERSION));
    }
    if (metaOpts->qMonVersion)
    {
        json_object_object_add(res, kStr[QMON_VER],
                               json_object_new_string(QMON_VERSION));
    }

    if (metaOpts->blob)
    {
        string tempStr = string("");
        for (int i = 0; i < metaOpts->blob; i++)
        {
            tempStr += string("A");
        }
        json_object_object_add(res, kStr[BLOB],
                               json_object_new_string(tempStr.c_str()));
    }

    if (metaOpts->close)
    {
        cData->handling = 0;
    }

    return 0;
}

void QMonitor::addThreadData(QMonitorData *data)
{
    // Add per thread Data to overall data
    for (const auto &t : tData)
    {
        data->totalRx += t.second.totalRx;
        data->totalTx += t.second.totalTx;
        data->decodeFails += t.second.decodeFails;
        data->rxFails += t.second.rxFails;
        data->secFails += t.second.secFails;
        data->mbdAlerts += t.second.mbdAlerts;
        data->totalRVs += t.second.totalRVs;
        data->totalRSUs += t.second.totalRSUs;
        data->txBSMs += t.second.txBSMs;
        data->txSignedBSMs += t.second.txSignedBSMs;
        data->rxBSMs += t.second.rxBSMs;
        data->rxSignedBSMs += t.second.rxSignedBSMs;
        // TODO add the rest of the QMonitorData here.
    }
}

/**
 * @brief Processes client request
 *
 * @param client int which represents the client unique sock and ID.
 * @return 0 if successfull parsing of JSON object options
 * @return -1 if parsing is not successful or unknown options.
 */
int QMonitor::processReq(int client)
{
    QMClientData *cData = &clientData[client];
    //std::cout << "Starting json parsing" << std::endl;
    cData->req = json_tokener_parse(cData->buffer);
    struct json_object *jsonTmp = nullptr;
    string info;
    if (cData->req == NULL ||
        json_object_get_type(cData->req) != json_type_object)
    {
        if (config.debugLevel > 1)
        {
            cout << "Malformed json with buffer: " << cData->buffer << endl;
        }
        return SOCK_ERROR;
    }
    if (config.debugLevel > 1)
    {
        cout << "Processing the following json: "
             << json_object_to_json_string(cData->req) << endl;
    }
    //std::cout << "Processing json " << json_object_to_json_string(cData->req) << std::endl;
    //  set mutex here if two threads per client
    //   Get Keys in request
    using namespace QMonitorJson;
    for (auto key : kStr)
    {
        if (json_object_object_get_ex(cData->req, key, &jsonTmp))
        {
            info = string("Error adding ") + key + " to options from client " +
                   to_string(client);
            errorCheck(AlertInfo<int>{info.c_str(),
                                      changeOption(client, key, jsonTmp), 0});
        }
        else
        {
            info = string("No key ") + key + " from client " + to_string(client);
            alert(AlertInfo<bool>{info.c_str(), false, false});
        }
    }
    // unlock mutex here if two threads per request
    return 0;
}

/**
 * @brief
 *
 * @param client
 * @param key
 * @param obj
 * @return true
 * @return false
 */
int QMonitor::changeOption(int client, const char *key, json_object *obj)
{
    QMClientData *cData = &clientData[client];
    QMClientOptions *options = &cData->options;
    QMClientMetaOptions *metaOpts = &options->metaOptions;
    QMClientValOptions *valOpts = &options->valueOptions;
    int ret = SOCK_ERROR;
    string info;
    using namespace QMonitorJson;
    if (config.debugLevel > LOW_ALERT)
    {
        std::cout << "Changing the following option " << key << " to: "
                  << json_object_get_int(obj) << std::endl;
    }
    try
    {
        switch (kMap.at(key))
        {
        case TOTAL_RX:
            // std::cout << "got total rx req" << std::endl;
            valOpts->totalRx = json_object_get_boolean(obj);
            break;
        case TOTAL_TX:
            // std::cout << "got total tx req" << std::endl;
            valOpts->totalTx = json_object_get_boolean(obj);
            break;
        case RX_FAILS:
            valOpts->rxFails = json_object_get_boolean(obj);
            break;
        case DECODE_FAILS:
            valOpts->decodeFails = json_object_get_boolean(obj);
            break;
        case SEC_FAILS:
            valOpts->securityFails = json_object_get_boolean(obj);
            break;
        case MBD_ALERTS:
            valOpts->mbdAlerts = json_object_get_boolean(obj);
            break;
        case TOTAL_RVS:
            valOpts->totalRVs = json_object_get_boolean(obj);
            break;
        case TOTAL_RSUS:
            valOpts->totalRSUs = json_object_get_boolean(obj);
            break;
        case TX_BSMS:
            valOpts->txBSMs = json_object_get_boolean(obj);
            break;
        case TX_SIGNED_BSMS:
            valOpts->txSignedBSMs = json_object_get_boolean(obj);
            break;
        case RX_BSMS:
            valOpts->rxBSMs = json_object_get_boolean(obj);
            break;
        case RX_SIGNED_BSMS:
            valOpts->rxSignedBSMs = json_object_get_boolean(obj);
            break;
        case MONITOR_RATE:
            metaOpts->monitorRate = json_object_get_int(obj);
            break;
        case TIMEFRAME:
            metaOpts->timeFrame = json_object_get_int(obj);
            break;
        case TIMESTAMP:
            // std::cout << "got timestamp req" << std::endl;
            metaOpts->timestamp = json_object_get_int(obj);
            break;
        case JSON_VER:
            metaOpts->jsonVersion = json_object_get_boolean(obj);
            break;
        case QITS_VER:
            metaOpts->qitsVersion = json_object_get_boolean(obj);
            break;
        case TELSDK_VER:
            metaOpts->telsdkVersion = json_object_get_boolean(obj);
            break;
        case QMON_VER:
            metaOpts->qMonVersion = json_object_get_boolean(obj);
        case BLOB:
            metaOpts->blob = json_object_get_int(obj);
            break;
        case CLOSE:
            // std::cout << "got close req" << std::endl;
            metaOpts->close = json_object_get_boolean(obj);
            break;
        default:
            ret = false;
            info = string("Key ") + key + " has no option handler";
            alert(AlertInfo<bool>{info.c_str(), false, false});
            break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        std::cout << "Key " << key << " not found in server." << std::endl;
        return ret;
    }
    ret = 0; // SUCCESS
    return ret;
}

/**
 * @brief Helper function that helps showing logging or debugging.
 *
 * @param info struct that has info about the error
 * @return int 0 if log or debug was successful, else fails.
 */
template <typename T>
int QMonitor::alert(AlertInfo<T> info)
{
    auto ret = MONITOR_INIT_VAL;
    // Logging Options
    switch (config.logLevel)
    {
    case NO_ALERT:
        break;
    case LOW_ALERT:
        break;
    case MED_ALERT:
        break;
    case HIGH_ALERT:
        break;
    default:
        break;
    }

    // Debuggin Options
    switch (config.debugLevel)
    {
    case LOW_ALERT:
        cout << info.data;
        break;
    case MED_ALERT:
        cout << info.data << " Value: " << info.value << " Error: "
             << info.errorValue << endl;
        break;
    case HIGH_ALERT:
        cout << info.data << " Value: " << info.value << " Error: "
             << info.errorValue << " Errno: " << errno << " Errno Info: "
             << strerror(errno) << endl;
        break;
    case NO_ALERT:
    default:
        break;
    }
    return ret;
}
template <typename T>
int QMonitor::errorCheck(AlertInfo<T> info)
{
    auto ret = 0;
    if (info.value == info.errorValue)
    {
        ret = info.errorValue;
        alert(info);
    }
    return ret;
}

QMonitor::Configuration QMonitor::loadArgs(int argc, const char **argv)
{
    Configuration c = Configuration();
    for (int i = 1; i < argc; i++)
    {
        const char confName = argv[i][1];
        cout << "Loading arguments \n";
        switch (confName)
        {
        case 'p': // Port
            cout << "Modifying port to " << argv[i + 1] << endl;
            c.sockAddress.sin_port = htons(atoi(argv[++i]));
            break;
        case 'a': // Address
            std::cout << "Modifying address" << std::endl;
            inet_aton(argv[++i], &c.sockAddress.sin_addr);
            break;
        case 'l': // log level
            std::cout << "Changing log level" << std::endl;
            c.logLevel = Alert(atoi(argv[++i]));
            break;
        case 'd': // debug level
            std::cout << "Changing debug level " << std::endl;
            c.debugLevel = Alert(atoi(argv[++i]));
            break;
        case 'b':
            cout << "Changing blocking to true\n";
            c.blocking = true;
            break;
        case 'h':
            printUsage();
            break;
        }
    }

    return c;
}

void QMonitor::printUsage()
{
    std::cout << "-h \t\tprints usage" << std::endl;
    std::cout << "-p <port> \t\tSets port server." << std::endl;
    std::cout << "-a <address> \t\tSets IPV4 server address" << std::endl;
    std::cout << "-l <0,1,2,3>  \t\tSets log level from 0 to 3" << std::endl;
    std::cout << "-d <0,1,2,3> \t\tSets debug level from 0 to 3" << std::endl;
    std::cout << "-b \t\t For testing mode, allows to block connection ";
    std::cout << "thread and prevents program from exiting" << std::endl;
}

void QMonitor::printConfig()
{
    if (config.debugLevel)
    {
        cout << "Port net: " << config.sockAddress.sin_port << " port host:"
             << ntohs(config.sockAddress.sin_port) << endl;
        std::cout << "Blocking: " << config.blocking << std::endl;
        std::cout << "Debug level" << config.debugLevel << std::endl;
        std::cout << "Log level" << config.logLevel << std::endl;
    }
}

void QMonitor::stop()
{
    isMonitoring = 0;
    close(serverSock);
    // shutdown(serverSock, SHUT_RDWR);
}