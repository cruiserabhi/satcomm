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
 * @file: qMonitor.hpp
 *
 * @brief: Monitoring Server Class for ITS Stack stats and info.
 */

#ifndef QMONITOR_HPP
#define QMONITOR_HPP

// System Includes
#include <json-c/json.h>
#include <map>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sstream>

// Local Includes

using std::map;
using std::thread;
using std::string;

// Local Defines

#define DEFAULT_PORT 6511
#define DEFAULT_ADDRESS "0.0.0.0"
#define DEFAULT_SOCK_OPT 1
#define MAX_BUFFER_SIZE 1500
#define BACKLOG_LENGTH 20
#define BILLION 1000000000
#define MILLION 1000000

// Version defines

#define JSON_VERSION "0.1"
#define QITS_VERSION "7.0" // Fix to right version
#define TELSDK_VERSION "46.66" // Change version based on telSDK
#define QMON_VERSION "1.0"

enum Alert
{
    NO_ALERT,
    LOW_ALERT,
    MED_ALERT,
    HIGH_ALERT
};

/* Client Request JSON

{
    //values
    totalRx: bool,
    totalTx: bool,
    decodeFails: bool,
    bsm: bool
    //options
    oneShot: bool
    rate: int //milliseconds
    interval:
}



*/

struct QMClientValOptions
{
    //** Monitor Variables
    bool totalRx;
    bool totalTx;
    bool decodeFails;
    bool securityFails;
    bool mbdAlerts; // Misbehavior detections alerts
    bool totalRVs;  // Total Remote Vehicles
    bool totalRSUs; // Total Road Side Units
    bool rxFails;
    //** Per Protocol
    // BSMs
    bool txBSMs;
    bool txSignedBSMs;
    bool rxBSMs;
    bool rxSignedBSMs;
    // Should we provide unsigned variables or can be inferred from total-signed?
    //** TODO others per protocol
};

struct QMClientMetaOptions
{
    uint8_t monitorRate; // in milliseconds
    int timeFrame;       // for json stream, delete if not.
    bool timestamp;
    bool jsonVersion;
    bool qitsVersion;
    bool telsdkVersion;
    bool qMonVersion;
    int blob; // Size of blob to reply back
    bool close;
    // Add more options here that don't involve data requests, for that parse jsonObject
};

struct QMClientOptions
{
    QMClientValOptions valueOptions;
    QMClientMetaOptions metaOptions;
};

struct QMClientData
{
    int sock;
    int handling;
    struct sockaddr_in address;
    thread *cThread;
    char *buffer; // Using only json_objects for now
    int bufferSize;
    QMClientOptions options;
    struct json_object *res, *req;
    // add mutex here for changes in client requests
};

struct QMonitorData
{
    //** Monitor Variables
    long long totalRx;
    long long totalTx;
    long long rxFails;
    long long decodeFails;
    long long secFails;
    long long mbdAlerts;
    long long totalRVs;
    long long totalRSUs;
    //* Per Protocol Variables
    // BSM
    long long txBSMs;
    long long rxBSMs;
    long long txSignedBSMs;
    long long rxSignedBSMs;
    long long txUnsignedBSMs;
    long long rxUnsignedBSMs;
    // SPAT
    long long txSPATs;
    long long rxSPATs;
    long long txSignedSPATs;
    long long rxSignedSPATs;
    long long txUnsignedSPATs;
    long long rxUnsignedSPATs;
    // MAP
    long long txMAPs;
    long long rxMAPs;
    long long txSignedMAPs;
    long long rxSignedMAPs;
    long long txUnsignedMAPs;
    long long rxUnsignedMAPs;
    // WSA
    // Add more data here, make sure qMonitorJson is updated with the necessary
    // keys.

    // Meta
    long long timestamp;
    string jsonVersion;
    string qitsVersion;
    string telsdkVersion;
    string qMonVersion;
    string blob;
};

template <typename T>
struct AlertInfo
{
    const char *data;
    T value;
    T errorValue;
};

class QMonitor
{

public:
    /**
     * @brief Basic configuration class for QMonitor.
     *
     */
    class Configuration
    {
    public:
        // TCP IP4 Server Options
        int sockDomain = AF_INET;
        int sockType = SOCK_STREAM;
        int sockProtocol = IPPROTO_TCP;
        int sockLevel = SOL_SOCKET;
        int sockOptName = SO_REUSEADDR | SO_REUSEPORT;
        int sockOpt = DEFAULT_SOCK_OPT;
        struct sockaddr_in sockAddress;
        int bufferSize = MAX_BUFFER_SIZE;
        int connBacklog = BACKLOG_LENGTH;
        int blocking = false; // Detaches connection handler if false.
        Alert debugLevel = NO_ALERT;
        Alert logLevel = NO_ALERT;

        Configuration(const char charAddr[] = DEFAULT_ADDRESS,
                      const int port = DEFAULT_PORT)
        {
            this->sockAddress.sin_family = this->sockDomain;
            this->sockAddress.sin_port = htons(port);
            inet_aton(charAddr, &sockAddress.sin_addr);
        }

        ~Configuration()
        {
        }
    };

    /**
     * @brief
     *
     * @param argc
     * @param argv
     * @return int
     */
    static Configuration loadArgs(int argc, const char **argv);
    /**
     * @brief Construct a new QMonitor object
     *
     * @param conf object that holds several QMonitor attributes.
     */
    QMonitor(const Configuration conf);

    /**
     * @brief Destroy the QMonitor object
     *
     */
    ~QMonitor();

    static void stop();
    static map<thread::id, QMonitorData> tData;

private:
    // Local variables
    static bool isMonitoring;
    static thread *connHandlerThread;
    static int serverSock;
    static Configuration config;
    //* Client Variables
    static map<int, QMClientData> clientData;

    /**
     * @brief Thread function to catch incoming qimc(s) (Qits Monitor Client)
     *
     */
    static void connectionHandler();

    /**
     * @brief Handles clients once connection is accepted
     *
     */
    static void clientHandler(int clientSock);

    /**
     * @brief Adds clients to a list and creates an exclusive thread for client.
     *
     * @param info QMClientInfo with socket and address of client
     */
    static void addClient(int clientSock);

    /**
     * @brief Does all the socket work to create the TCP/IP for the server
     *
     */
    static void initServerSocket();

    /**
     * @brief
     *
     * @param client
     * @param key
     * @param obj
     * @return 0 if successful
     * @return -1 if error
     */
    static int changeOption(int client, const char *key, json_object *obj);

    /**
     * @brief Processes client request
     *
     * @param client int which represents the client unique sock and ID.
     * @return 0 if successfull, else error
     */
    static int processReq(int client);

    /**
     * @brief process json req
     *
     * @param client client identifier
     * @return int 0 if success, else fail
     */
    static int processJsonReq(int client);

    static void continousResponse(int client);

    static void oneshotResponse(int client);

    /**
     * @brief Create a Response object
     *
     * @param client int which represents the client struct in the server.
     * @return int 0 if success, else error.
     */
    static int createResponse(int client, json_object *res);

    /**
     * @brief Adds per thread data of tDat into total mData (Monitor Data)
     *
     */
    static void addThreadData(QMonitorData *data);

    /**
     * @brief
     *
     * @tparam T
     * @param info
     * @return int
     */
    template <typename T>
    static int alert(AlertInfo<T> info);

    /**
     * @brief
     *
     * @tparam T template for different data types to check.
     * @param info struct that holds meta info about possible error.
     * @return int 0 if success, else fails
     */
    template <typename T>
    static int errorCheck(AlertInfo<T> info);

    /**
     * @brief
     *
     */
    static void printUsage();

    /**
     * @brief
     *
     */
    static void printConfig();
};

#endif
