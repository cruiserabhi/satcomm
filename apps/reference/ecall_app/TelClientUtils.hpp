/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELCLIENTUTILS_HPP
#define TELCLIENTUTILS_HPP

#include <telux/tel/CallManager.hpp>

using namespace telux::common;
using namespace telux::tel;

/** TelClientUtils class provides helper functions to convert various parameters to strings */
class TelClientUtils {
public:
    /*
     * Get the call state in string format
     */
    static std::string callStateToString(CallState cs);
    /*
     * Get the call direction in string format
     */
    static std::string callDirectionToString(CallDirection cd);
    /**
     * Get the call end cause in string format from call end cause code
     */
    static std::string callEndCauseToString(CallEndCause callEndCause);
    /*
     * Get ECallMsdTransmissionStatus in string
     */
    static std::string eCallMsdTransmissionStatusToString(ECallMsdTransmissionStatus status);
    /*
     * Get eCall HLAP timer status in string
     */
    static std::string eCallHlapTimerStatusToString(HlapTimerStatus status);
    /*
     * Get eCall HLAP timer event in string
     */
    static std::string eCallHlapTimerEventToString(HlapTimerEvent event);

    /*
     * Print the eCall configuration to stdout
     */
    static void printEcallConfig(EcallConfig config);

    /*
     * Prints the encoded optional additional data content.
     */
    static void printEncodedOptionalAdditionalDataContent(std::string encodedString);

    /*
     * Prints the encoded eCall MSD payload.
     */
    static void printECallMsdPayload(std::string encodedPdu);

    /*
     * Prints the reason to redial eCall or not.
     */
    static std::string eCallRedialReasonToString(ReasonType reason);

    TelClientUtils();
    ~TelClientUtils();
};

#endif  // TELCLIENTUTILS_HPP
