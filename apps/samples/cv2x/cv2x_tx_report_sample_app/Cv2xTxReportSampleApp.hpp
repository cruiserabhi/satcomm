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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2XTXREPORTSAMPLEAPP_HPP
#define CV2XTXREPORTSAMPLEAPP_HPP


#include <string>

#include <telux/cv2x/Cv2xTxFlow.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xTxStatusReportListener.hpp>

using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xTxFlow;
using telux::cv2x::ICv2xTxStatusReportListener;

class Cv2xTxStatusReportApp {
public:
    static Cv2xTxStatusReportApp & getInstance();

    int init();

    int deinit();

    void startTxPkts();

private:

    Cv2xTxStatusReportApp();

    int registerTxFlow(std::shared_ptr<ICv2xRadio> &radio);

    int deregisterTxFlow(std::shared_ptr<ICv2xRadio> &radio);

    int fillTxBuffer(char* buf, uint16_t length);

    int sampleTx(int sock, char* buf, uint16_t length);

    int createTxReportListener(std::shared_ptr<ICv2xRadio> &radio);

    int deleteTxReportListener(std::shared_ptr<ICv2xRadio> &radio);

    std::shared_ptr<ICv2xRadio> radio_ = nullptr;
    std::shared_ptr<ICv2xTxStatusReportListener> txReportListener_ = nullptr;
    std::shared_ptr<ICv2xTxFlow> txFlow_ = nullptr;
    bool txFlowValid_ = false;
    uint32_t txCount_ = 0;
    char* buf_ = nullptr;
};
#endif  // CV2XTXREPORTSAMPLEAPP_HPP
