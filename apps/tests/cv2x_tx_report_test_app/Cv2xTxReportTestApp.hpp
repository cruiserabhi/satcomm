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
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
#ifndef CV2XTXREPORTTESTAPP_HPP
#define CV2XTXREPORTTESTAPP_HPP


#include <string>
#include "ConsoleApp.hpp"

#include <telux/cv2x/Cv2xTxFlow.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xTxStatusReportListener.hpp>

using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xTxFlow;
using telux::cv2x::ICv2xTxStatusReportListener;
using telux::cv2x::Cv2xStatus;

struct Options {
    bool isSPS; // SPS or event Tx flow type
    uint16_t port; // source port number of Tx flow
    uint16_t length; // packet payload lenght in bytes
    uint16_t interval; // packet transmission interval in miliseconds
    uint16_t serviceId; // service ID of Tx flow
    std::string file; // user specified csv file for saving Tx status reportfs
};

class Cv2xStatusListener : public ICv2xListener {
public:

    Cv2xStatusListener(Cv2xStatus status);

    Cv2xStatus getCv2xStatus();

    void onStatusChanged(Cv2xStatus status) override;

    void waitCv2xActive();

    void stopWaitCv2xActive();

private:
    std::mutex mtx_;
    Cv2xStatus cv2xStatus_;
    std::condition_variable cv_;
};

class Cv2xTxStatusReportApp : public ConsoleApp,
    public std::enable_shared_from_this<Cv2xTxStatusReportApp> {
public:

    static Cv2xTxStatusReportApp & getInstance();

    int init();

    void consoleInit();

    int deinit();

    void startTxAndListenToReportCommand();

    void stopTxAndListenToReportCommand();

    void startListenToReportCommand();

    void stopListenToReportCommand();

private:

    Cv2xTxStatusReportApp();

    void initOptions();

    void printOptions();

    int parseOptions();

    int registerTxFlow();

    int deregisterTxFlow();

    int fillTxBuffer(char* buf, uint16_t length);

    int sampleTx(int sock, char* buf, uint16_t length);

    void startTxPkts();

    void stopTxPkts();

    int createTxReportListener();

    int deleteTxReportListener();

    std::shared_ptr<ICv2xRadioManager> cv2xRadioManager_ = nullptr;
    std::shared_ptr<ICv2xRadio> radio_ = nullptr;
    std::shared_ptr<ICv2xTxStatusReportListener> txReportListener_ = nullptr;
    std::shared_ptr<Cv2xStatusListener> cv2xStatusListener_ = nullptr;
    std::shared_ptr<ICv2xTxFlow> txFlow_ = nullptr;
    bool txFlowValid_ = false;
    uint32_t txCount_ = 0;
    Options options_;
    char* buf_ = nullptr;
    std::future<void> txThread_;
    bool txThreadValid_ = false;
    std::mutex operationMtx_;
    bool exiting_ = false;
};
#endif  // CV2XTXREPORTTESTAPP_HPP
