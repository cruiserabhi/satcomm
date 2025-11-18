/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xTcpTest.cpp
 *
 * @brief: Simple application that demonstrates Tx/Rx TCP packets in Cv2x
 */

#include <arpa/inet.h>
#include <linux/tcp.h>
#include <net/if.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <ifaddrs.h>
#include <cstring>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <array>
#include <telux/cv2x/Cv2xRadio.hpp>

#include "../../common/utils/Utils.hpp"

using std::array;
using std::string;
using std::cerr;
using std::cout;
using std::endl;
using std::promise;
using std::shared_ptr;
using std::atomic;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::common::ServiceStatus;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xTxRxSocket;
using telux::cv2x::Periodicity;
using telux::cv2x::Priority;
using telux::cv2x::TrafficCategory;
using telux::cv2x::SocketInfo;
using telux::cv2x::EventFlowInfo;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xRadioListener;
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xRadioManager;

// In TCP_CLIENT mode, this tool connects to TCP server via V2X-IP iface,
// sends and recvs pkts from TCP server.
static constexpr uint8_t TCP_CLIENT = 0u;
// In TCP_SERVER mode, this tool listens on V2X-IP iface, accepts connection request
// from client, recvs pkt and echoes back.
// If proxy is enabled, it forwards pkts received from client to SCMS server and
// forwards echo pkt received from SCMS server to client.
static constexpr uint8_t TCP_SERVER = 1u;
// In TCP_TEST mode, this tool only setups flows on V2X-IP iface, not sends/recvs pkts,
// user can use other public tools like iperf or socat to do TCP testing.
static constexpr uint8_t TCP_TEST = 2u;
// In SCMS_SERVER mode, no telsdk API is invoked, it listens on the specified iface
// and port, accepts connection request from TCP server and echoes back each received pkt.
static constexpr uint8_t SCMS_SERVER = 3u;

static constexpr uint32_t SERVIC_ID = 1u;
static constexpr uint16_t DEFAULT_PORT = 5000u;
static constexpr int      PRIORITY = 5;
static constexpr uint32_t PACKET_LEN = 128u;
static constexpr uint32_t MAX_DUMMY_PACKET_LEN = 10000;
static constexpr uint16_t DEFAULT_PROXY_PORT = 9000u;

static constexpr char TEST_VERNO_MAGIC = 'Q';
static constexpr char CLIENT_UEID = 1;
static constexpr char SERVER_UEID = 2;

static shared_ptr<ICv2xRadioManager> gCv2xRadioMgr = nullptr;
static shared_ptr<ICv2xRadio> gCv2xRadio = nullptr;
static shared_ptr<ICv2xRadioListener> gRadioListener = nullptr;
static shared_ptr<ICv2xListener> gStatusListener = nullptr;
static shared_ptr<ICv2xTxRxSocket> gTcpSockInfo = nullptr;
static int32_t gTcpSocket = -1;
static int32_t gAcceptedSock = -1;
static Cv2xStatus gCv2xStatus;
static mutex gCv2xStatusMutex;
static condition_variable gStatusCv;
static promise<ErrorCode> gCallbackPromise;
static array<char, MAX_DUMMY_PACKET_LEN> gBuf;
static uint8_t gTcpMode = TCP_CLIENT;
static uint16_t gSrcPort = DEFAULT_PORT;
static uint16_t gDstPort = DEFAULT_PORT;
static string gDstAddr;
static uint32_t gServiceId = SERVIC_ID;
static uint32_t gPacketLen = PACKET_LEN;
static uint32_t gPacketNum = 0;
static uint32_t gTxCount = 0u;
static uint32_t gRxCount = 0u;
static atomic<bool> gTcpConnected{false};
static atomic<int> gTerminate{0};
static int gTerminatePipe[2];
static mutex gOperationMutex;

static bool gSetGlobalIp = false;
static string gGlobalIpPrefix("2600:8802:1507:c700");
static bool gClearGlobalIp = false;

static bool gEnableProxy = false;
static string gProxyAddr;
static uint16_t gProxyPort = DEFAULT_PROXY_PORT;
static string gRemoteAddr;
static uint16_t gRemotePort = DEFAULT_PROXY_PORT;
static int32_t gProxySock = -1;
static int32_t gProxyAcceptedSock = -1;
static int32_t gProxyFamily = AF_INET6;

class RadioListener : public ICv2xRadioListener {
public:
    void onL2AddrChanged(uint32_t newL2Address) {
        cout << "source L2 address changed to:" << newL2Address << endl;
        // local-link address has changed after TCP connection establishment,
        // the TCP connection cannot be used now, need to exit
        if (newL2Address > 0 and gTcpConnected) {
            cerr << "v2x ip address has changed, need exit and re-start test!" << endl;
            gTerminate = 1;
            write(gTerminatePipe[1], &gTerminate, sizeof(int));
            gStatusCv.notify_all();
        }
    }
};

class Cv2xStatusListener : public ICv2xListener {
public:
    void onStatusChanged(Cv2xStatus status) override {
        lock_guard<mutex> lock(gCv2xStatusMutex);
        if (status.rxStatus != gCv2xStatus.rxStatus
            or status.txStatus != gCv2xStatus.txStatus) {
            cout << "cv2x status changed, Tx: " << static_cast<int>(status.txStatus);
            cout << ", Rx: " << static_cast<int>(status.rxStatus) << endl;
            gCv2xStatus = status;

            if (status.rxStatus == Cv2xStatusType::ACTIVE and
                status.txStatus == Cv2xStatusType::ACTIVE) {
                gStatusCv.notify_all();
            }
        }
    }
};

static bool isV2xReady() {
    lock_guard<mutex> lock(gCv2xStatusMutex);
    if (Cv2xStatusType::ACTIVE == gCv2xStatus.rxStatus and
        Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus) {
        return true;
    }

    cout << "cv2x Tx/Rx not active!" << endl;
    return false;
}

static void waitV2xStatusActive() {
    std::unique_lock<std::mutex> cvLock(gCv2xStatusMutex);
    while (!gTerminate and
           (Cv2xStatusType::ACTIVE != gCv2xStatus.rxStatus or
            Cv2xStatusType::ACTIVE != gCv2xStatus.txStatus)) {
        cout << "wait for Cv2x status active." << endl;
        gStatusCv.wait(cvLock);
    }
}

// Resets the global callback promise
static inline void resetCallbackPromise(void) {
    gCallbackPromise = promise<ErrorCode>();
}

// Callback function for ICv2xRadioManager->requestCv2xStatus()
static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gCv2xStatus = status;
    }
    gCallbackPromise.set_value(error);
}

// Callback function for ICv2xRadio->createCv2xTcpSocket()
static void createTcpSocketCallback(shared_ptr<ICv2xTxRxSocket> sock,
                                    ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gTcpSockInfo = sock;
    }
    gCallbackPromise.set_value(error);
}

// Callback function for ICv2xRadio->setGlobalIPInfo()
static void setGlobalIPInfoCallback(ErrorCode error) {
    gCallbackPromise.set_value(error);
}

// Returns current timestamp
static uint64_t getCurrentTimestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ull + tv.tv_usec;
}

// Fills buffer with dummy data
static void fillBuffer(void) {
    static uint16_t seq_num = 0u;
    auto timestamp = getCurrentTimestamp();

    // Very first payload is test Magic number, this is  where V2X Family ID would normally be.
    gBuf[0] = TEST_VERNO_MAGIC;

    // Next byte is the UE equipment ID
    if (gTcpMode == TCP_CLIENT) {
        gBuf[1] = CLIENT_UEID;
    } else {
        gBuf[1] = SERVER_UEID;
    }

    // Sequence number
    auto dataPtr = gBuf.data() + 2;
    uint16_t tmp = htons(seq_num++);
    memcpy(dataPtr, reinterpret_cast<char *>(&tmp), sizeof(uint16_t));
    dataPtr += sizeof(uint16_t);

    // Timestamp
    dataPtr += snprintf(dataPtr, gPacketLen - (2 + sizeof(uint16_t)),
                        "<%llu> ", static_cast<long long unsigned>(timestamp));

    // Dummy payload
    constexpr int NUM_LETTERS = 26;
    auto i = 2 + sizeof(uint16_t) + sizeof(long long unsigned);
    for (; i < gPacketLen; ++i) {
        gBuf[i] = 'a' + ((seq_num + i) % NUM_LETTERS);
    }
}

// Function for transmitting data
static int sampleTx(int32_t sock) {
    cout << "sampleTx(" << sock << ")" << endl;

    if (sock < 0) {
        return EXIT_FAILURE;
    }

    struct msghdr message = {0};
    struct iovec iov[1] = {0};
    struct cmsghdr * cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];

    // Send data using sendmsg to provide IPV6_TCLASS per packet
    iov[0].iov_base = gBuf.data();
    iov[0].iov_len = gPacketLen;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    // Fill ancillary data
    int priority = PRIORITY;
    cmsghp = CMSG_FIRSTHDR(&message);
    cmsghp->cmsg_level = IPPROTO_IPV6;
    cmsghp->cmsg_type = IPV6_TCLASS;
    cmsghp->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsghp), &priority, sizeof(int));

    // Send data
    auto sendBytes = sendmsg(sock, &message, 0);

    // Check bytes sent
    if (sendBytes <= 0) {
        cerr << "Error occurred sending to sock:" << sock << " err:" << strerror(errno) << endl;
    } else {
        uint16_t seq = 0;
        memcpy(reinterpret_cast<char *>(&seq), gBuf.data() + 2, sizeof(uint16_t));
        seq = ntohs(seq);
        ++gTxCount;
        cout << "TX count: " << gTxCount << " bytes:" << sendBytes;
        cout << " UEID:" << static_cast<int>(gBuf[1]);
        cout << " SEQ:" << seq << endl;
    }

    return sendBytes;
}

// Function for reading from Rx socket
static int sampleRx(int32_t sock) {
    cout << "sampleRx(" << sock << ")" << endl;

    if (sock < 0) {
        return EXIT_FAILURE;
    }

    // Attempt to read from socket
    int recvBytes = recv(sock, gBuf.data(), gBuf.max_size(), 0);

    if (recvBytes <= 0) {
        cerr << "Error occurred reading from sock:" << sock << " err:" << strerror(errno) << endl;
    } else {
        ++gRxCount;
        uint16_t seq = 0;
        memcpy(reinterpret_cast<char *>(&seq), gBuf.data() + 2, sizeof(uint16_t));
        seq = ntohs(seq);
        cout << "RX count: " << gRxCount << " bytes:" << recvBytes;
        cout << " UEID:" << static_cast<int>(gBuf[1]);
        cout << " SEQ:" << seq << endl;
    }

    return recvBytes;
}

// Callback for ICv2xRadio->closeCv2xTcpSocket()
static void closeTcpSocketCallback(shared_ptr<ICv2xTxRxSocket> chan, ErrorCode error) {
    gCallbackPromise.set_value(error);
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << endl;
    cout << "client example: " << Opt << " -m 0 -d <server addr>" << endl;
    cout << "server example: " << Opt << " -m 1" << endl;
    cout << "test mode example: " << Opt << " -m 2 -s 0" << endl;
    cout << "server proxy example: " << Opt << " -m 1 -x <proxy addr> -y <remote addr>" << endl;
    cout << "scms server example: " << Opt << " -m 3 -x <proxy addr>" << endl;
    cout << "-m <tcpMode>       0-TCP_CLIENT, 1-TCP_SERVER, 2-TEST_MODE, 3-SCMS_SERVER" << endl;
    cout << "-d <dstAddr>       Destination IPV6 address used for connecting" << endl;
    cout << "-s <srcPort>       Source port used for binding, default is 5000" << endl;
    cout << "-t <dstPort>       Destination port used for connecting, default is 5000" << endl;
    cout << "-p <service ID>    Service ID used for Tx and Rx flows, default is ";
    cout << gServiceId << endl;
    cout << "-l <packet length> Tx Packet length, default is " << gPacketLen <<endl;
    cout << "-n <packet number> Tx Packet number" <<endl;
    cout << "-g<global IP prefix> Set global IP prefix, default is " << gGlobalIpPrefix << endl;
    cout << "-x <proxy_addr> Proxy addr for TCP_SERVER or local addr for SCMS_SERVER" << endl;
    cout << "-X <proxy_port> Proxy port, default is " << gProxyPort << endl;
    cout << "-y <remote_addr> Proxy remote addr for TCP_SERVER" << endl;
    cout << "-Y <remote_port> Proxy remote port, default is " << gRemotePort << endl;
    cout << "-F Use IPV4 addr for proxy, default is IPV6" << endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int rc = 0;
    int c;
    while ((c = getopt(argc, argv, "?d:m:s:t:p:l:n:g::x:X:y:Y:F")) != -1) {
        switch (c) {
        case 'd':
            if (optarg) {
                gDstAddr = optarg;
                cout << "dstAddr: " << gDstAddr << endl;
            }
            break;
        case 'm':
            if (optarg) {
                gTcpMode = atoi(optarg);
                cout << "tcpMode: " << +gTcpMode << endl;
            }
            break;
        case 's':
            if (optarg) {
                gSrcPort = atoi(optarg);
                cout << "srcPort: " << gSrcPort << endl;
            }
            break;
        case 't':
            if (optarg) {
                gDstPort = atoi(optarg);
                cout << "dstPort: " << gDstPort << endl;
            }
            break;
        case 'p':
            if (optarg) {
                gServiceId = atoi(optarg);
                cout << "service ID: " << gServiceId << endl;
            }
            break;
        case 'l':
            if (optarg) {
                gPacketLen = atoi(optarg);
                cout << "packet length: " << gPacketLen << endl;
            }
            break;
        case 'n':
            if (optarg) {
                gPacketNum = atoi(optarg);
                cout << "packet number: " << gPacketNum << endl;
            }
            break;
        case 'g':
            gSetGlobalIp = true;
            if (optarg) {
                gGlobalIpPrefix = optarg;
            }
            cout << "global IP prefix: " << gGlobalIpPrefix << endl;
            break;
        case 'x':
            if (optarg) {
                gEnableProxy = true;
                gProxyAddr = optarg;
                cout << "Set proxy addr:" << gProxyAddr << endl;
            }
            break;
        case 'X':
            if (optarg) {
                gProxyPort = atoi(optarg);
                cout << "Set proxy port:" << gProxyPort << endl;
            }
            break;
        case 'y':
            if (optarg) {
                gRemoteAddr = optarg;
                cout << "Set proxy remote addr:" << gRemoteAddr << endl;
            }
            break;
        case 'Y':
            if (optarg) {
                gRemotePort = atoi(optarg);
                cout << "Set proxy remote port:" << gRemotePort << endl;
            }
            break;
        case 'F':
            gProxyFamily = AF_INET;
            cout << "Use IPV4 addr for proxy" << endl;
            break;
        case '?':
        default:
            rc = -1;
            printUsage(argv[0]);
            return rc;
        }
    }

    if (gTcpMode == TCP_CLIENT && gDstAddr.empty()) {
        cout << "error Destination IP Addr." << endl;
        rc = -1;
    }

    if (gEnableProxy and
        (gProxyAddr.empty() or (gTcpMode == TCP_SERVER and gRemoteAddr.empty()))) {
       cerr << "Error proxy parameters!" << endl;
       rc = -1;
    }

    return rc;
}

static int cv2xInit() {
    lock_guard<mutex> lock(gOperationMutex);

    // Get handle to Cv2xRadioManager
    bool cv2xRadioManagerStatusUpdated = false;
    telux::common::ServiceStatus cv2xRadioManagerStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xRadioManagerStatusUpdated = true;
        cv2xRadioManagerStatus = status;
        cv.notify_all();
    };

    auto & cv2xFactory = Cv2xFactory::getInstance();
    gCv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!gCv2xRadioMgr) {
        cout << "Error: failed to get Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioManagerStatus) {
            cerr << "C-V2X Radio Manager initialization failed, exiting" << endl;
            return EXIT_FAILURE;
        }
    }

    // Get C-V2X status and make sure Tx/Rx is active
    resetCallbackPromise();
    if (Status::SUCCESS != gCv2xRadioMgr->requestCv2xStatus(cv2xStatusCallback)
        or ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "Failed to get cv2x radio status"<< endl;
        return EXIT_FAILURE;
    }

    if (Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus) {
        cout << "C-V2X TX/RX status is active" << endl;
    } else {
        cerr << "C-V2X TX/RX is inactive" << endl;
        return EXIT_FAILURE;
    }

    bool cv2x_radio_status_updated = false;
    telux::common::ServiceStatus cv2xRadioStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

    auto cb = [&](ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2x_radio_status_updated = true;
        cv2xRadioStatus = status;
        cv.notify_all();
    };

    gCv2xRadio = gCv2xRadioMgr->getCv2xRadio(TrafficCategory::SAFETY_TYPE, cb);
    if (not gCv2xRadio) {
        cerr << "C-V2X Radio creation failed." << endl;
        return EXIT_FAILURE;
    }

    // Wait for radio to complete initialization
    {
        std::unique_lock<std::mutex> lc(mtx);
        cv.wait(lc, [&cv2x_radio_status_updated]() { return cv2x_radio_status_updated; });

        if (cv2xRadioStatus != ServiceStatus::SERVICE_AVAILABLE) {
            cerr << "C-V2X Radio initialization failed." << endl;
            return EXIT_FAILURE;
        } else {
            cout << "C-V2X Radio is ready" << endl;
        }
    }

    // Register for Src L2 Id Update callbacks
    gRadioListener = std::make_shared<RadioListener>();
    if (Status::SUCCESS != gCv2xRadio->registerListener(gRadioListener)) {
        cerr << "Radio listener registration failed." << endl;
        return EXIT_FAILURE;
    }

    // Register for cv2x status update
    gStatusListener = std::make_shared<Cv2xStatusListener>();
    if (Status::SUCCESS != gCv2xRadioMgr->registerListener(gStatusListener)) {
        cerr << "Status listener registration failed." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


static int connectTcpSocketClient(int sock, string dstAddr, uint16_t dstPort, int32_t family) {
    // For TCP client, establish connection with the created sock
    if (family == AF_INET6) {
        // dest addr is IPV6 type
        struct sockaddr_in6 dstSockAddr = {0}; //must reset the sockaddr
        dstSockAddr.sin6_port = htons(dstPort);
        inet_pton(AF_INET6, dstAddr.c_str(), (void *)&dstSockAddr.sin6_addr);
        dstSockAddr.sin6_family = AF_INET6;

        cout << "connecting sock:" << sock << endl;
        if (connect(sock, (struct sockaddr *)&dstSockAddr, sizeof(struct sockaddr_in6))) {
            cout << "connect err:" << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
    } else {
        // dest addr is IPV4 type
        struct sockaddr_in dstSockAddr = {0}; //must reset the sockaddr
        dstSockAddr.sin_port = htons(dstPort);
        inet_pton(AF_INET, dstAddr.c_str(), (void *)&dstSockAddr.sin_addr);
        dstSockAddr.sin_family = AF_INET;

        cout << "connecting sock:" << sock << endl;
        if (connect(sock, (struct sockaddr *)&dstSockAddr, sizeof(struct sockaddr_in))) {
            cout << "connect err:" << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int acceptTcpSocketServer(int listenSock, int32_t& acceptSock) {
    // mark the created socket as listening sock
    cout << "listening sock" << listenSock << endl;
    if (listen(listenSock, 5) < 0) {
        cout << "listen err:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    // accept connection request
    cout << "accepting connection..." << endl;
    struct sockaddr_in6 tmpAddr = {0};
    socklen_t socklen = sizeof(tmpAddr);
    acceptSock = accept(listenSock, (struct sockaddr *)&tmpAddr, &socklen);
    if (acceptSock < 0) {
        cout << "accept err:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    cout << "accepted sock:" << acceptSock << endl;
    return EXIT_SUCCESS;
}

static int createTcpSocket() {
    lock_guard<mutex> lock(gOperationMutex);

    cout << "creating TCP socket" << endl;
    SocketInfo tcpInfo;
    tcpInfo.serviceId = gServiceId;
    tcpInfo.localPort = gSrcPort;
    EventFlowInfo eventInfo;
    // set unicast flag if testing with global IP prefix
    if (gSetGlobalIp) {
        eventInfo.isUnicast = true;
    }

    resetCallbackPromise();
    if (Status::SUCCESS != gCv2xRadio->createCv2xTcpSocket(eventInfo, tcpInfo,
                                                           createTcpSocketCallback) ||
        ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cout << "Tcp Socket creation failed." << endl;
        return EXIT_FAILURE;
    }

    //get created TCP socket
    gTcpSocket = gTcpSockInfo->getSocket();

    cout << "create TCP socket successfully, port: "
        << static_cast<int>(ntohs(gTcpSockInfo->getSocketAddr().sin6_port)) << endl;
    // add 1s Tx/Rx timeout to remove the possibility for indefinite wait
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(gTcpSocket, SOL_SOCKET, SO_RCVTIMEO|SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        cout << "set sock timeout err:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int parseIPv6Prefix(char *ipPrefix) {
    int i = 0;
    std::string::size_type pos = 0, prev = 0;
    string prefixStr = gGlobalIpPrefix + ":";
    do {
        if (i >= CV2X_IPV6_ADDR_ARRAY_LEN) {
            cout << "ipPrefix " << i << " too long" << std::endl;
            return EXIT_FAILURE;
        }
        pos = prefixStr.find(":", prev);
        if (pos != std::string::npos) {
            uint16_t val = stoi(prefixStr.substr(prev, pos), 0, 16);
            ipPrefix[i] = (val >> 8);
            ipPrefix[i + 1] = (val & 0xFF);
        }
        prev = pos + 1;
        i += 2;
    } while(pos != std::string::npos);

    return EXIT_SUCCESS;
}

static int setGlobalIpPrefix() {
    cout << "setting global ip prefix" << endl;

    // parse global IP prefix
    char ipPrefix[CV2X_IPV6_ADDR_ARRAY_LEN] = {0};
    if (EXIT_FAILURE == parseIPv6Prefix(ipPrefix)) {
        cerr << "parse global IP prefix err!"<< endl;
        return EXIT_FAILURE;
    }

    // set global IP prefix to modem
    telux::cv2x::IPv6AddrType prefix;
    prefix.prefixLen = 64;
    memcpy(prefix.ipv6Addr, ipPrefix, CV2X_IPV6_ADDR_ARRAY_LEN);
    resetCallbackPromise();
    if (Status::SUCCESS != gCv2xRadio->setGlobalIPInfo(prefix, setGlobalIPInfoCallback)
        or ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "set global IP prefix fails!" << endl;
        return EXIT_FAILURE;
    }

    // set global IP prefix succeeded, need to clear global IP prefix when exit
    gClearGlobalIp = true;
    return EXIT_SUCCESS;
}

static int clearGlobalIpPrefix() {
    cout << "clearing global ip prefix" << endl;

    // set global IP prefix 0 to modem
    telux::cv2x::IPv6AddrType prefix;
    prefix.prefixLen = 64;
    memset(prefix.ipv6Addr, 0, CV2X_IPV6_ADDR_ARRAY_LEN);
    resetCallbackPromise();
    if (Status::SUCCESS != gCv2xRadio->setGlobalIPInfo(prefix, setGlobalIPInfoCallback)
        or ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "clear global IP prefix fails!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int setupTcpConnection() {
    // set global IP prefix to IP data call before creating TCP socket
    if (gSetGlobalIp) {
        if (EXIT_FAILURE == setGlobalIpPrefix()) {
            return EXIT_FAILURE;
        }
    }

    // create TCP socket
    if (createTcpSocket()) {
        return EXIT_FAILURE;
    }

    if (gTcpMode == TCP_CLIENT) {
        // For TCP client, connect to the configured dst addr
        if (connectTcpSocketClient(gTcpSocket, gDstAddr, gDstPort, AF_INET6)) {
            return EXIT_FAILURE;
        }
        gTcpConnected = true;
    } else if (gTcpMode == TCP_SERVER) {
        // For TCP server, accept incoming connection request
        if (acceptTcpSocketServer(gTcpSocket, gAcceptedSock)) {
            return EXIT_FAILURE;
        }
        gTcpConnected = true;
    } else {
        // do nothing in test mode
    }

    return EXIT_SUCCESS;
}

// close accepted socket in server mode
static void closeAcceptedSocket() {
    if (gAcceptedSock < 0) {
        return;
    }
    cout << "closing client socket:"<< gAcceptedSock << endl;
    //call shutdown to send out FIN
    shutdown(gAcceptedSock, SHUT_WR|SHUT_RD);
    close(gAcceptedSock);
    gAcceptedSock = -1;
    usleep(500000);
}

static void closeTcpSocket() {
    if (!gTcpSockInfo) {
        return;
    }

    cout << "closing Tcp socket, fd:" << gTcpSockInfo->getSocket() << endl;
    resetCallbackPromise();
    if(Status::SUCCESS != gCv2xRadio->closeCv2xTcpSocket(gTcpSockInfo, closeTcpSocketCallback) ||
       ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cout << "close Tcp socket err" << endl;
    }
    gTcpSocket = -1;
    gTcpSockInfo = nullptr;
}

static void releaseTcpConnection() {
    gTcpConnected = false;

    // For TCP server, close the accepted socket before closing the listening socket
    closeAcceptedSocket();

    // close TCP socket and deregister flows
    closeTcpSocket();

    // reset global IP prefix
    if (gClearGlobalIp) {
        clearGlobalIpPrefix();
    }
}

void releaseProxyConnection() {
    if (gProxyAcceptedSock > -1) {
        cout << "closing accepted proxy sock:" << gProxyAcceptedSock << endl;
        close(gProxyAcceptedSock);
        gProxyAcceptedSock = -1;
    }

    if (gProxySock > -1) {
        cout << "closing proxy sock:" << gProxySock << endl;
        close(gProxySock);
        gProxySock = -1;
    }
}

static void terminationCleanup() {
    lock_guard<mutex> lock(gOperationMutex);

    cout << "Terminating" << endl;

    // release proxy connection
    releaseProxyConnection();

    // Release resources of TCP connection
    releaseTcpConnection();

    if (gCv2xRadio and gRadioListener) {
        gCv2xRadio->deregisterListener(gRadioListener);
    }

    if (gCv2xRadioMgr and gStatusListener) {
        gCv2xRadioMgr->deregisterListener(gStatusListener);
    }

    cout << "TCP Tx count:" << gTxCount << endl;
    cout << "TCP Rx count:" << gRxCount << endl;
}

static void terminationHandler(int signum) {
    gTerminate = 1;
    write(gTerminatePipe[1], &gTerminate, sizeof(int));

    // notify threads waiting for active status
    gStatusCv.notify_all();
}

static void installSignalHandler() {
    struct sigaction sig_action;

    sig_action.sa_handler = terminationHandler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    sigaction(SIGINT, &sig_action, NULL);
    sigaction(SIGHUP, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
}

static int startTcpClientMode(int32_t sock) {
    // send out pkt reaches configured number
    if (gPacketNum > 0 and gTxCount >= gPacketNum) {
        cout << "Tx pkt count reached!" << endl;
        return EXIT_FAILURE;
    }

    if (sock < 0) {
        cerr << "Error sock for TCP client!" << endl;
        return EXIT_FAILURE;
    }

    // send pkt to client
    fillBuffer();
    if (sampleTx(sock) <= 0) {
        return EXIT_FAILURE;
    }

    // wait for echo from client
    if (sampleRx(sock) <= 0) {
        // EAGAIN and EWOULDBLOCK are possible error when
        // socket read timeout, not bail out in this case
        if (errno != EAGAIN and errno != EWOULDBLOCK) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int createProxySock() {
    if ((gProxySock = socket(gProxyFamily, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Create proxy sock failed, errno:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    // allow multiple clients to bind to the same IP address with different port,
    // and allow binding a socket in TIME_WAIT state
    int option = 1;
    if (setsockopt(gProxySock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<void *>(&option),
                   sizeof(option)) < 0) {
        cerr << "Set SO_REUSEADDR to proxy sock failed, errno:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    // bind to proxy iface and port
    if (gProxyFamily == AF_INET6) {
        // proxy addr is IPV6 type
        struct sockaddr_in6 proxySockAddr = {0}; //must reset the sockaddr
        proxySockAddr.sin6_port = htons(gProxyPort);
        inet_pton(AF_INET6, gProxyAddr.c_str(), (void *)&proxySockAddr.sin6_addr);
        proxySockAddr.sin6_family = AF_INET6;
        if (bind(gProxySock, reinterpret_cast<struct sockaddr *>(&proxySockAddr),
                 sizeof(struct sockaddr_in6)) < 0) {
            cerr << "Bind proxy sock failed, errno:" << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
    } else {
        // proxy addr is IPV4 type
        struct sockaddr_in proxySockAddr = {0}; //must reset the sockaddr
        proxySockAddr.sin_port = htons(gProxyPort);
        inet_pton(AF_INET, gProxyAddr.c_str(), (void *)&proxySockAddr.sin_addr);
        proxySockAddr.sin_family = AF_INET;
        if (bind(gProxySock, reinterpret_cast<struct sockaddr *>(&proxySockAddr),
                 sizeof(struct sockaddr_in)) < 0) {
            cerr << "Bind proxy sock failed, errno:" << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int setupProxy() {
    // create TCP socket that binds to the proxy iface
    if (createProxySock()) {
        cout << "Create proxy socket failed!" << endl;
        return EXIT_FAILURE;
    }

    if (gTcpMode == TCP_SERVER) {
        // connect to remote SCMS addr and port
        if (connectTcpSocketClient(gProxySock, gRemoteAddr, gRemotePort, gProxyFamily)) {
            cout << "Connect to SCMS server err:" << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
    } else if (gTcpMode == SCMS_SERVER) {
        // listen on specified port
        if (acceptTcpSocketServer(gProxySock, gProxyAcceptedSock)) {
            cout << "Accept RSU connection err:" << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
    } else {
        cerr << "Error mode for proxy:" << +gTcpMode << endl;
        return EXIT_FAILURE;
    }

    cout << "Setup proxy mode succesfully!" << endl;
    return EXIT_SUCCESS;
}

static int startTcpServerMode(int32_t sock, int32_t proxySock) {
    if (sock < 0) {
        cerr << "Error socket for TCP server!" << endl;
        return EXIT_FAILURE;
    }

    // recv pkt from client
    if ((gPacketLen = sampleRx(sock)) <= 0) {
        // EAGAIN and EWOULDBLOCK are possible error when
        // socket read timeout, not bail out in this case
        if (errno != EAGAIN and errno != EWOULDBLOCK) {
            cerr << "Recv from client sock:" << sock << " failed, errno:";
            cerr << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
    } else {
        // If proxy is enabled for TCP server, forward pkts btw TCP client and SCMS server
        if (proxySock > -1) {
            // forward pkt received from client to remote network
            if (sampleTx(proxySock) <= 0) {
                cerr << "Send pkt to proxy sock:" << sock << " failed, errno:";
                cerr << strerror(errno) << endl;
                return EXIT_FAILURE;
            }

            // recv echo pkt from remote network
            if ((gPacketLen = sampleRx(proxySock)) <= 0) {
                // EAGAIN and EWOULDBLOCK are possible error when
                // socket read timeout, not bail out in this case
                if (errno != EAGAIN and errno != EWOULDBLOCK) {
                    cerr << "Recv from proxy sock:" << sock << " failed, errno:";
                    cerr << strerror(errno) << endl;
                    return EXIT_FAILURE;
                }
            }
        }

        // send echo to client
        if (sampleTx(sock) <= 0) {
            cerr << "Send pkt to sock:" << sock << " failed, errno:";
            cerr << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int init() {
    // setup proxy btw device and remote network
    if (gEnableProxy) {
        if (setupProxy()) {
            cerr << "Failed to setup proxy mode!" << endl;
            return EXIT_FAILURE;
        }

        // no telsdk API invoked for SCMS_SERVER
        if (gTcpMode == SCMS_SERVER) {
            return EXIT_SUCCESS;
        }
    }

    // do cv2x telsdk related initialization
    if (cv2xInit()) {
        cerr << "Cv2x init failed!" << endl;
        return EXIT_FAILURE;
    }

    // setup TCP connection via PC5
    if (setupTcpConnection()) {
        cerr << "Setup PC5 TCP connection error!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    cout << "Running C-V2X TCP Test" << endl;

    if (pipe(gTerminatePipe) == -1) {
        cout << "Pipe error" << endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> groups{"system", "diag", "radio", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(groups);
    if (rc == -1){
        cout << "Adding supplementary group failed!" << std::endl;
    }

    installSignalHandler();

    auto f = std::async(std::launch::async, []() {
        int terminate = 0;
        read(gTerminatePipe[0], &terminate, sizeof(int));
        cout << "Read terminate:" << terminate << endl;
        terminationCleanup();
    });

    // Parse parameters, get cv2x handles, create TCP flow and establish the connection
    if (parseOpts(argc, argv) or init()) {
        goto bail;
    }

    // main operation loop
    while (!gTerminate) {
        if (gTcpMode == SCMS_SERVER) {
            // echo each msg received from remote network
            if (startTcpServerMode(gProxyAcceptedSock, -1)) {
                goto bail;
            }
        } else if (gTcpMode == TCP_TEST) {
            cout << "Entering TCP_TEST mode, use CTRL+C to exit" << endl;
            goto waitExit;
        } else {
            // wait for V2X active status before Tx/Rx via PC5
            waitV2xStatusActive();
            if (!isV2xReady()) {
                continue;
            }

            if (gTcpMode == TCP_CLIENT) {
                // send msg to server via PC5 and wait for echo
                if (startTcpClientMode(gTcpSocket)) {
                    goto bail;
                } else {
                    // wait 100ms to send the next pkt
                    usleep(100000u);
                }
            } else if (gTcpMode == TCP_SERVER) {
                // echo each msg received from client via PC5
                if (startTcpServerMode(gAcceptedSock, gProxySock)) {
                    goto bail;
                }
            }
        }
    }

bail:
    // teminate
    gTerminate = 1;
    write(gTerminatePipe[1], &gTerminate, sizeof(int));
waitExit:
    f.get();

    if (gCv2xRadio) {
        gCv2xRadio = nullptr;
    }
    if (gCv2xRadioMgr) {
        gCv2xRadioMgr = nullptr;
    }

    cout << "Done." << endl;

    return EXIT_SUCCESS;
}
