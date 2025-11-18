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
 * @file: qMonitorJson.hpp
 *
 * @brief: Monitoring Json and other  helper structs and functions.
 *  */

#pragma once
#include <map>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

namespace QMonitorJson
{

    enum JsonKeys
    {
        TOTAL_TX,
        TOTAL_RX,
        TOTAL_RSUS,
        TOTAL_RVS,
        RX_FAILS,
        DECODE_FAILS,
        SEC_FAILS,
        MBD_ALERTS,
        TX_BSMS,
        TX_SIGNED_BSMS,
        RX_BSMS,
        RX_SIGNED_BSMS,
        MONITOR_RATE,
        TIMEFRAME,
        TIMESTAMP, // nano since epoch at send time
        JSON_VER,
        QITS_VER,
        TELSDK_VER,
        QMON_VER,
        BLOB,
        CLOSE
    };

    static const char *kStr[] = { //MUST be in the same order as JsonKeys enum.
        "totalTx",
        "totalRx",
        "totalRSUs",
        "totalRVs",
        "rxFails",
        "decodeFails",
        "securityFails",
        "mbdAlerts",
        "txBSMs",
        "txSignedBSMs",
        "rxBSMs",
        "rxSignedBSMs",
        "monitorRate",
        "timeframe",
        "timestamp",
        "jsonVersion",
        "qitsVersion",
        "telsdkVersion",
        "qMonVersion",
        "blob",
        "close"};

    static const map<const char *, int> kMap = []()
    {
        map<const char *, int> m;
        int i = 0;
        for (auto k : kStr)
        {
            m[k] = i;
            i++;
        }
        return m;
    }();

}