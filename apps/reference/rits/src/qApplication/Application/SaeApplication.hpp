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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /**
  * @file: SaeApplication.hpp
  *
  * @brief: class for ITS stack - SAE
  */
#include "ApplicationBase.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <climits>

class SaeApplication : public ApplicationBase {
public:
    SaeApplication(char *fileConfiguration, MessageType msgType, bool enableCsvLog = false,
        bool enableDiagLog = false);
    SaeApplication(const string txIpv4, const uint16_t txPort, const string rxIpv4,
        const uint16_t rxPort, char* fileConfiguration, MessageType msgType,
        bool enableCsvLog = false, bool enableDiagLog = false);
    ~SaeApplication();

    /* Initialization */
    bool init() override;

    /**
    * Method that decodes bsm from raw buffer to bsm contents data structure in ldm.
    * It also handles the Tunc. This method should be deprecated once Encoding and Decoding
    * libraries can take the Tunc in the j2735 dictionary and protocol.
    * @param index - An uint8_t that gives which receive buffer to access.
    * @param bufLen -The length of given buffer.
    * @param ldmIndex -The index of the ldm bsm content to store decoded value.
    */
    void receiveTuncBsm(const uint8_t index, const uint16_t bufLen, const uint32_t ldmIndex);

    /**
    * Method to illustrate Tunnel Timing by adding Tunc.
    * This method should be deprecated once Decoding and Encoding libraries
    * for BSMs can actually take Tunc as part of the J2735 dictionary and protocol.
    * @param index - An uint8_t that shows all elements used form vectors.
    * @param type - The type of flow (event, sps) in which bsm will transmit.
    * reports in milliseconds. It can be interval or more.
    */
    void sendTuncBsm(uint8_t index, TransmitType txType);

    /**
    * Overall method to fill msg_contents struct based on SAE packet contents.
    * @param msg - A shared pointer to the msg_contents struct
    */
    void fillMsg(std::shared_ptr<msg_contents> msg);

    /**
    * Method to print reception related statistics.
    */
    void printRxStats();

    /**
    * Method to print transmission related statistics.
    */
    void printTxStats();

    int setGlobalIPv6Prefix(void);
    int clearGlobalIPv6Prefix(void);
    static std::vector<asyncCbData_t> asyncCbData;
    static bool exitAsync;
    void AsyncPostProcessing(bool overridePsidCheck, bool enableCongCtrl,
        bool enableMisbehavior, void* asyncSecService,
        shared_ptr<ICongestionControlManager> congestionControlManager,
        shared_ptr<QMonitor> qMon,
        int secVerbosity, RadioReceive* radioReceive);
    static void postprocessing_cleanup();
    void PostProcessingThread();
private:
    uint32_t fakeTmpId = 0;
    bool exit_ = false;
    uint8_t prevSourceMac[CV2X_MAC_ADDR_LEN];
    std::atomic<bool>  GlobalIpSessionActive{false};
    std::chrono::milliseconds wraInterval;
    std::thread wraThread;
    std::mutex wraMutex;
    std::condition_variable wraCv;
    std::chrono::time_point<std::chrono::high_resolution_clock> now;
    void wraThreadFunc(int routerLifetime);
    bool initialized = false;   // used to initialize temp id randomly
    unsigned int msgCount = 0;      // Ranges from 1 - 127 in cyclic fashion.
    unsigned int tempId = 0;        // 32 bit identifier
    string rsuGateway_;    // used to store the RSU gateway parsed from received wsa
    string rsuPrimaryDns_; // used to store the RSU primray DNS parsed from received wsa
    std::atomic<bool> obuRouteSet_ {false}; // indicate whether the default route is set in OBU
    static void printStats(std::thread::id thrId, int secVerbosity);
    void basicFilterAndSafetyChecks(int l2SrcAddr, double distFromRV);
    void fillLoggingData(bsm_value_t* bsm, bsm_data* bs);
    void prepareForSecurityChecks(bsm_value_t* bsm, SecurityOpt_t* sopt);
    /**
    * Method to setup and perform transmission for SAE packets.
    * @param index - An uint8_t that is used for which buffer to access
    * @param mc - A shared pointer to a v2x message contents struct
    * @param bufLen - Length of given buffer or message
    * @param txType - Specifies the type of message for decoding
    */
    int transmit(uint8_t index, std::shared_ptr<msg_contents>mc, int16_t bufLen,
            TransmitType txType);
    /**
    * Method to setup and perform reception for SAE packets.
    * @param index - An uint8_t that is used for which buffer to access
    * @param bufLen - Length of given buffer or message
    */
    int receive(const uint8_t index, const uint16_t bufLen);

    /**
    * Method to setup and perform reception with LDM for SAE packets.
    * @param index - An uint8_t that is used for which buffer to access
    * @param bufLen - Length of given buffer or message
    * @param ldmIndex - Index of the LDM BSM content to store decoded value
    */
    int receive(const uint8_t index, const uint16_t bufLen,
                     const uint32_t ldmIndex);

#ifdef AEROLINK
    /**
    * Method to setup and perform reception with LDM for SAE packets.
    * @param mc - A shared pointer to a v2x message contents struct
    * @param l2SrcAddr - the l2 src address of the RV; needed for flooding detection
    */
    int decodeAndVerify(msg_contents* mc, int l2SrcAddr,
        uint8_t index, uint64_t timestamp);
#endif
#ifdef WITH_WSA
    int onReceiveWra(RoutingAdvertisement_t *wra, uint8_t *sourceMacAddr, int& MacAdrLen);
    /**
     * Method to setup and fill WSA related information.
     * @param wsa - A pointer to the SrvAdvMsg_t struct
     * @param wra - A pointer to RoutingAdvertisement_t struct
     */
    void fillWsa(SrvAdvMsg_t *wsa, RoutingAdvertisement_t *wra);

    /**
    * Method for OBU to store the WRA information retrieved from RSU.
    * @param wra  - A pointer to WRA message
    */
    int storeWraInfoInObu(RoutingAdvertisement_t* wra);
#endif

    /**
    * Method for RSU to get the static application server IPv6 address from
    * configuration or the dynamic address of C-V2X IP rmnet interface.
    * @param buf     - A pointer to the buffer for storing the IPv6 address
    * @param len     - The length of the ipAddr in bytes
    */
    int getDefaultGWAddrInRsu(char *buf, int& len);

    /**
    * Method for OBU to set default route in OBU on receivng default gateway from the RSU.
    * @param addr  - route to set.
    */
    int setDefaultRouteInObu(string addr);

    /**
    * Method for OBU to delete the default route set previously.
    */
    int deleteDefaultRouteInObu();

    /**
    * Method to initialize SAE packets in msg_contents struct.
    * @param mc - A shared pointer to the msg_contents struct
    * @param isRx - A flag to indicate if the packet is used for Rx
    */
    bool initMsg(std::shared_ptr<msg_contents> mc, bool isRx = false) override;

    /**
    * Method to delete and free SAE packet memory in msg_contents struct.
    * @param mc - A shared pointer to the msg_contents struct
    */
    void freeMsg(std::shared_ptr<msg_contents> mc) override;

    /**
    * Method to setup and fill BSM related information.
    * @param bsm - A pointer to the BSM struct
    */
    void fillBsm(bsm_value_t *bsm);

    /**
    * Method to setup and fill BSM CAN related information.
    * @param bsm - A pointer to the BSM struct
    */
    void fillBsmCan(bsm_value_t *bsm);

    /**
    * Method to setup and fill BSM Location related information.
    * @param bsm - A pointer to the BSM struct
    */
    void fillBsmLocation(bsm_value_t *bsm);

    /**
    * Method to setup and fill WSMP related information.
    * @param bsm - A pointer to the WSMP struct
    */
    void fillWsmp(wsmp_data_t *wsmp);

    //Pre-recorded file methods
    /**
    * Method to setup and initialize a pre-recorded BSM.
    * @param bsm - A pointer to the BSM struct
    */
    void initRecordedBsm(bsm_value_t* bsm);

    /**
    * Method to parse IPv6 Address from input string.
    * @param str    - A string that includes the IPv6 address
    * @param buf    - A pointer to the decoded IPv6 address
    * @param bufLen - Length of decoded IPv6 address
    */
    int parseIPv6Addr(const string& str, char *buf, int& bufLen);

    /**
    * Method to convert IPv6 Address from input buffer to string.
    * @param buf    - A pointer to the decoded IPv6 address
    * @param bufLen - Length of decoded IPv6 address
    * @param str    - A string that includes the IPv6 address
    */
    int convertIpv6Addr2Str(char* buf, int bufLen, string& addr);
    std::mutex wramutex;
};
