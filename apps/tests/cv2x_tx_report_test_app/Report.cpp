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
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file: Report.cpp
 *
 * @brief: Handler of Tx status report.
 */

#include <iostream>
#include <string>
#include <stdlib.h>
#include <inttypes.h>

#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include "Report.hpp"
#include "../../common/utils/Utils.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::RFTxStatus;
using telux::cv2x::TxType;
using telux::cv2x::SegmentType;

static const char gTxReportHeader[] = "UTC(us),port,sfn,tx_type,"
                                      "tx_status_0,tx_pwr_0(dBm),tx_status_1,tx_pwr_1(dBm),"
                                      "rb_number,start_rb,mcs,"
                                      "segment_number,segment_type";

static constexpr uint32_t SFN_LIMIIT = 10240u; // valid SFN value is 0~10239

static constexpr uint32_t SPS_TIMING_CHANGE_NUM = 5u; // threshold for pkt jitter detection

Cv2xTxStatusReportListener::Cv2xTxStatusReportListener(string fileName, uint16_t port, int & ret) {
    ret = EXIT_SUCCESS;
    port_ = port;
    // create csv file if specified valid file name
    if (not fileName.empty()) {
        file_ = fopen(fileName.c_str(), "w");
        if (not file_) {
            cerr << "Failed to open log file " << fileName;
            cerr << ", store to " << DEFAULT_LOG_FILE << " instead!" << endl;
            file_ = fopen(DEFAULT_LOG_FILE, "w");
            if (not file_) {
                cerr << "Failed to open log file " << DEFAULT_LOG_FILE << endl;
                ret = EXIT_FAILURE;
            }
        }
    } else {
        ret = EXIT_FAILURE;
    }

    if (EXIT_SUCCESS == ret) {
       //print header to file
       fprintf(file_, "%s\n", gTxReportHeader);
    }
}

Cv2xTxStatusReportListener::~Cv2xTxStatusReportListener() {
    if (file_) {
        fclose(file_);
        file_ = nullptr;
    }
    cout << "newTx report count:" << newTxCount_;
    cout << ", reTx report count:" << reTxCount_;

    // only print SLSS counts when listener is associated with port 0,
    // listener with other port cannot receive SLSS reports
    if (0 == port_) {
        cout << ", slss Tx report count:" << slssTxCount_ << endl;
    } else {
        cout << endl;
    }
}

string Cv2xTxStatusReportListener::txType2String(TxType in) {
    string out;
    if (in == TxType::NEW_TX) {
        out = "newTx";
    } else if (in == TxType::RE_TX) {
        out = "reTx";
    } else {
        out = "slss";
    }

    return out;
}

string Cv2xTxStatusReportListener::rfStatus2String(RFTxStatus in) {
    string out;
    if (in == RFTxStatus::INACTIVE) {
        out = "NA";
    } else if (in == RFTxStatus::OPERATIONAL) {
        out = "good";
    } else {
        out = "bad";
    }

    return out;
}

string Cv2xTxStatusReportListener::segType2String(SegmentType in) {
    string out;
    if (in == SegmentType::FIRST) {
        out = "F";
    } else if (in == SegmentType::MIDDLE) {
        out = "M";
    } else if (in == SegmentType::LAST) {
        out = "L";
    }  else {
        out = "N";
    }

    return out;
}

void Cv2xTxStatusReportListener::writeReportToFile(const TxStatusReport & info) {
    if (not file_) {
        return;
    }
    fprintf(file_, "%" PRIu64 ", ", Utils::getCurrentTimestamp());
    fprintf(file_, "%u, %u, %s, ", info.port, info.otaTiming, txType2String(info.txType).c_str());
    fprintf(file_, "%s, %.1f, ", rfStatus2String(info.rfInfo[0].status).c_str(),
            static_cast<float>(info.rfInfo[0].power)/10);
    fprintf(file_, "%s, %.1f, ", rfStatus2String(info.rfInfo[1].status).c_str(),
            static_cast<float>(info.rfInfo[1].power)/10);
    fprintf(file_, "%u, %u, %u, ", info.numRb, info.startRb, info.mcs);
    fprintf(file_, "%u, %s\n", info.segNum, segType2String(info.segType).c_str());

    fflush(file_);
}

void Cv2xTxStatusReportListener::onTxStatusReport(const TxStatusReport & info) {
    // print report to file
    if (file_) {
        writeReportToFile(info);
    }

    checkPerPktStatus(info);
    checkTxChainStatus(info);
}

// update Tx pkt info, return true when recv the first report for a pkt
void Cv2xTxStatusReportListener::checkPerPktStatus(const TxStatusReport & info) {
    if (info.txType == TxType::NEW_TX) {
        newTxCount_++;
        if (info.segType == SegmentType::FIRST or
        info.segType == SegmentType::ONLY_ONE) {
            pktCount_++;
            cout << "Recv newTx(F/N seg) report#" << pktCount_;
            cout << " at ota:" << info.otaTiming;
            cout << ", segNum:" << +info.segNum << endl;
        }
    } else if (info.txType == TxType::RE_TX) {
        reTxCount_++;
    } else {
        slssTxCount_++;
    }
}

// check if one or two Tx chain has bad status
void Cv2xTxStatusReportListener::checkTxChainStatus(const TxStatusReport & info) {
    // check overall Tx chain status
    if (info.rfInfo[0].status == RFTxStatus::FAULT or
        info.rfInfo[1].status == RFTxStatus::FAULT) {
        cerr << "Warning: Tx chain bad status detected at ota:"<< info.otaTiming << endl;
    }
}
