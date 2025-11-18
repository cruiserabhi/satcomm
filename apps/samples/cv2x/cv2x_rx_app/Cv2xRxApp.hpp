/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#ifndef CV2XRXAPP_HPP
#define CV2XRXAPP_HPP

#include <string>
#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xUtil.hpp>

using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xTxFlow;
using telux::cv2x::ICv2xRxSubscription;

class Cv2xRxApp {
public:

    int init();

    void deinit();

    int parseOptions(int argc, char *argv[]);

    int registerFlow();

    void deregisterFlow();

    int sampleRx(int& len);

    int sampleTx(int len);

private:

    // RX mode types supported for CV2X non-IP traiffc
    // WILDCARD: Receive all packets on a single port, no SID filtering.
    //           Register Rx flow with none specific SIDs.
    // CATCHALL: Receive packets of a specified list of SIDs on a single port,
    //           packets of SIDs other than in the SID list will be filtered.
    //           Register Rx flow with a list of SIDs, SIDs in the list are not supposed
    //           to be used for transmission, so do not register Tx flow using any of them.
    // SPECIFIC_SID: Transmit and Receive packets of different SIDs on different port numbers.
    //               Register a Tx flow and then register an Rx flow for each SID, specify
    //               an unused port for each pair of flows.
    // Limits:
    // 1. WILDCARD cannot work along with CATCHALL/SPECIFIC_SID, it will break other Rx methods.
    // 2. Only one port can be enabled for WILDCARD or CATCHALL in the whole system.
    // 3. CATCHALL can work along with SPECIFIC_SID, but the two methods must use different
    //    SIDs and different port numbers.
    enum class RxModeType {
        WILDCARD,
        CATCHALL,
        SPECIFIC_SID,
    };

    static constexpr uint16_t RX_PORT_NUM = 9000u;

    void printUsage();
    int parseSidList(char* param);
    int registerTxFlow();
    int registerRxFlow();
    int deregisterTxFlow();
    int deregisterRxFlow();

    std::shared_ptr<ICv2xRadio> cv2xRadio_ = nullptr;
    std::shared_ptr<ICv2xTxFlow> txFlow_ = nullptr;
    std::shared_ptr<ICv2xRxSubscription> rxFlow_ = nullptr;
    RxModeType rxMode_ = RxModeType::WILDCARD;
    uint16_t port_ = RX_PORT_NUM;
    std::vector<uint32_t> idVector_;
    char* buf_ = nullptr;
    uint32_t rxCount_ = 0;
    uint32_t txCount_ = 0;
};
#endif  // CV2XRXAPP_HPP
