/*
 *  Copyright (c) 2018-2020 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <limits>
#include "MySignalStrengthHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

using namespace telux::tel;
using namespace telux::common;

MySignalStrengthCallback::MySignalStrengthCallback() {
}

std::string MySignalStrengthCallback::signalLevelToString(telux::tel::SignalStrengthLevel level) {
    switch(level){
        case telux::tel::SignalStrengthLevel::LEVEL_1 : return "LEVEL_1";
        case telux::tel::SignalStrengthLevel::LEVEL_2 : return "LEVEL_2";
        case telux::tel::SignalStrengthLevel::LEVEL_3 : return "LEVEL_3";
        case telux::tel::SignalStrengthLevel::LEVEL_4 : return "LEVEL_4";
        case telux::tel::SignalStrengthLevel::LEVEL_5 : return "LEVEL_5";
        case telux::tel::SignalStrengthLevel::LEVEL_UNKNOWN : return "LEVEL_UNKNOWN";
        default:
            return "Invalid Signal Level";
    }
}

void MySignalStrengthCallback::signalStrengthResponse(
    std::shared_ptr<SignalStrength> signalStrength, ErrorCode error) {
    std::cout << std::endl << std::endl;
    PRINT_CB << "Received Signal Strength Callback with Error Code: "
             << Utils::getErrorCodeAsString(error) << std::endl;
    if (signalStrength->getGsmSignalStrength() != nullptr) {
        if(signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "GSM Signal Strength: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "GSM Signal Strength: "
                 << signalStrength->getGsmSignalStrength()->getGsmSignalStrength() << std::endl;
        }

        if(signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "GSM Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "GSM Bit Error Rate: "
                << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()<< std::endl;
        }

        if(signalStrength->getGsmSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "GSM Signal Strength(in dBm): " << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "GSM Signal Strength(in dBm): "
                << signalStrength->getGsmSignalStrength()->getDbm() << std::endl;
        }

        if(signalStrength->getGsmSignalStrength()->getTimingAdvance()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "GSM Timing Advance(in bit periods): " << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "GSM Timing Advance(in bit periods): "
                << signalStrength->getGsmSignalStrength()->getTimingAdvance() << std::endl;
        }

        PRINT_CB << "GSM Signal Level: "
            << signalLevelToString(signalStrength->getGsmSignalStrength()->getLevel())<< std::endl;
    }

    if (signalStrength->getLteSignalStrength() != nullptr) {
        if(signalStrength->getLteSignalStrength()->getLteSignalStrength()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "LTE Signal Strength: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "LTE Signal Strength: "
                 << signalStrength->getLteSignalStrength()->getLteSignalStrength() << std::endl;
        }

        if(signalStrength->getLteSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "LTE Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "LTE Signal Strength(in dBm): "
                 << signalStrength->getLteSignalStrength()->getDbm() << std::endl;
        }

        if(signalStrength->getLteSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "LTE Reference Signal Receive Power(in dBm): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "LTE Reference Signal Receive Power(in dBm): "
                 << signalStrength->getLteSignalStrength()->getDbm() << std::endl;
        }

        if(signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
             == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "LTE Reference Signal Receive Quality(in dB): "
                << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "LTE Reference Signal Receive Quality(in dB): "
                << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
                << std::endl;
        }

        if(signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "LTE Reference Signal SNR(in dB): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "LTE Reference Signal SNR(in dB): "
                 << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr() * 0.1
                 << std::endl;
        }

        PRINT_CB << "LTE Signal Level: "
            << signalLevelToString(signalStrength->getLteSignalStrength()->getLevel())
             << std::endl;
    }

    if (signalStrength->getWcdmaSignalStrength() != nullptr) {
        if (signalStrength->getWcdmaSignalStrength()->getSignalStrength()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "WCDMA Signal Strength: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "WCDMA Signal Strength: "
                 << signalStrength->getWcdmaSignalStrength()->getSignalStrength() << std::endl;
        }

        if(signalStrength->getWcdmaSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "WCDMA Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "WCDMA Signal Strength(in dBm): "
                 << signalStrength->getWcdmaSignalStrength()->getDbm() << std::endl;
        }

        if(signalStrength->getWcdmaSignalStrength()->getBitErrorRate()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "WCDMA Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "WCDMA Bit Error Rate: "
                 << signalStrength->getWcdmaSignalStrength()->getBitErrorRate() << std::endl;
        }

        if(signalStrength->getWcdmaSignalStrength()->getEcio()
             == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "WCDMA Energy per chip to Interference Power Ratio(in dB): "
                << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "WCDMA Energy per chip to Interference Power Ratio(in dB): "
                << signalStrength->getWcdmaSignalStrength()->getEcio() << std::endl;
        }

        if(signalStrength->getWcdmaSignalStrength()->getRscp()
             == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "WCDMA Reference Signal Code Power(in dBm): "
                << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "WCDMA Reference Signal Code Power(in dBm): "
                << signalStrength->getWcdmaSignalStrength()->getRscp() << std::endl;
        }

        PRINT_CB
            << "WCDMA Signal Level: "
            << signalLevelToString(signalStrength->getWcdmaSignalStrength()->getLevel())
            << std::endl;
    }


    if (signalStrength->getNr5gSignalStrength() != nullptr) {
        if(signalStrength->getNr5gSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "5G NR Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "5G NR Signal Strength(in dBm): "
                 << signalStrength->getNr5gSignalStrength()->getDbm() << std::endl;
        }

        if(signalStrength->getNr5gSignalStrength()->getReferenceSignalReceiveQuality()
                                                                == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "5G NR Receive Quality(in dB): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "5G NR Receive Quality(in dB): "
                 << signalStrength->getNr5gSignalStrength()->getReferenceSignalReceiveQuality()
                 << std::endl;
        }

        if(signalStrength->getNr5gSignalStrength()->getReferenceSignalSnr()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "5G Reference Signal SNR(in dB): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "5G Reference Signal SNR(in dB): "
                 << signalStrength->getNr5gSignalStrength()->getReferenceSignalSnr() * 0.1
                 << std::endl;
        }

        PRINT_CB << "5G Signal Level: "
            << signalLevelToString(signalStrength->getNr5gSignalStrength()->getLevel())
             << std::endl;
    }

    if (signalStrength->getNb1NtnSignalStrength() != nullptr) {
        if(signalStrength->getNb1NtnSignalStrength()->getSignalStrength()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "NB1 NTN Signal Strength: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "NB1 NTN Signal Strength: "
                << signalStrength->getNb1NtnSignalStrength()->getSignalStrength()
                << std::endl;
        }

        if (signalStrength->getNb1NtnSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "NB1 NTN Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            PRINT_CB << "NB1 NTN Reference Signal Receive Power(in dBm): " << "UNAVAILABLE"
                << std::endl;
        } else {
            PRINT_CB << "NB1 NTN Signal Strength(in dBm): "
                << signalStrength->getNb1NtnSignalStrength()->getDbm() << std::endl;
            PRINT_CB << "NB1 NTN Reference Signal Receive Power(in dBm): "
                << signalStrength->getNb1NtnSignalStrength()->getDbm() << std::endl;
        }

        if (signalStrength->getNb1NtnSignalStrength()->getRsrq()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "NB1 NTN Reference Signal Receive Quality(in dB): "
                << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "NB1 NTN Reference Signal Receive Quality(in dB): "
                << signalStrength->getNb1NtnSignalStrength()->getRsrq() << std::endl;
        }

        if (signalStrength->getNb1NtnSignalStrength()->getRssnr()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_CB << "NB1 NTN Reference Signal SNR(in dB): " << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_CB << "NB1 NTN Reference Signal SNR(in dB): "
                 << signalStrength->getNb1NtnSignalStrength()->getRssnr() * 0.1 << std::endl;
        }

        PRINT_CB << "NB1 NTN Signal Level: "
            << signalLevelToString(signalStrength->getNb1NtnSignalStrength()->getLevel())
             << std::endl;
    }
}
