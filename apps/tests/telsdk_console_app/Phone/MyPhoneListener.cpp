/*
 *  Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021, 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <memory>
#include <sstream>
#include <string>

extern "C" {
#include <sys/time.h>
}

#include "MyPhoneListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

// Notify PhoneManager subsystem status
void MyPhoneListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::string stat = "";
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }
    PRINT_NOTIFICATION << " Phone onServiceStatusChange" << stat << "\n";
}

void MyPhoneListener::onServiceStateChanged(int phoneId, telux::tel::ServiceState state) {
    std::cout << "\n";
    PRINT_NOTIFICATION << "OnServiceStateChanged for PhoneId = " << phoneId
                       << " ,ServiceState = " << serviceStateToString(state) << std::endl;
}

std::string MyPhoneListener::serviceStateToString(telux::tel::ServiceState serviceState) {
    std::string state = "";
    switch (serviceState) {
    case telux::tel::ServiceState::EMERGENCY_ONLY:
        state = "Emergency Only";
        break;
    case telux::tel::ServiceState::IN_SERVICE:
        state = "In Service";
        break;
    case telux::tel::ServiceState::OUT_OF_SERVICE:
        state = "Out Of Service";
        break;
    case telux::tel::ServiceState::RADIO_OFF:
        state = "Radio Off";
        break;
    default:
        state = "Unknown";
        break;
    }
    return state;
}

void MyPhoneListener::onSignalStrengthChanged(
    int phoneId, std::shared_ptr<telux::tel::SignalStrength> signalStrength) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << "OnSignalStrengthChanged for PhoneId = " << phoneId << std::endl;
    if (signalStrength->getGsmSignalStrength() != nullptr) {
        if(signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "GSM Signal Strength: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "GSM Signal Strength: "
                 << signalStrength->getGsmSignalStrength()->getGsmSignalStrength() << std::endl;
        }

        if(signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "GSM Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "GSM Bit Error Rate: "
                << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()<< std::endl;
        }

        if(signalStrength->getGsmSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "GSM Signal Strength(in dBm): " << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "GSM Signal Strength(in dBm): "
                << signalStrength->getGsmSignalStrength()->getDbm() << std::endl;
        }

        if(signalStrength->getGsmSignalStrength()->getTimingAdvance()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "GSM Timing Advance(in bit periods): " << "UNAVAILABLE"
            << std::endl;
        } else {
            PRINT_NOTIFICATION << "GSM Timing Advance(in bit periods): "
                << signalStrength->getGsmSignalStrength()->getTimingAdvance() << std::endl;
        }

        PRINT_NOTIFICATION << "GSM Signal Level: "
            << MyPhoneHelper::signalLevelToString(
                signalStrength->getGsmSignalStrength()->getLevel())<< std::endl;
    }

    if (signalStrength->getLteSignalStrength() != nullptr) {
        if(signalStrength->getLteSignalStrength()->getLteSignalStrength()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "LTE Signal Strength: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "LTE Signal Strength: "
                 << signalStrength->getLteSignalStrength()->getLteSignalStrength() << std::endl;
        }

        if(signalStrength->getLteSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "LTE Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "LTE Signal Strength(in dBm): "
                 << signalStrength->getLteSignalStrength()->getDbm() << std::endl;
        }

        if(signalStrength->getLteSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "LTE Reference Signal Receive Power(in dBm): "<< "UNAVAILABLE"
            << std::endl;
        } else {
            PRINT_NOTIFICATION << "LTE Reference Signal Receive Power(in dBm): "
                 << signalStrength->getLteSignalStrength()->getDbm() << std::endl;
        }

        if(signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
             == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "LTE Reference Signal Receive Quality(in dB): "
                << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "LTE Reference Signal Receive Quality(in dB): "
                << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
                << std::endl;
        }

        if(signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "LTE Reference Signal SNR(in dB): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "LTE Reference Signal SNR(in dB): "
                 << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr() * 0.1
                 << std::endl;
        }

        PRINT_NOTIFICATION << "LTE Signal Level: "
            << MyPhoneHelper::signalLevelToString(
            signalStrength->getLteSignalStrength()->getLevel()) << std::endl;
    }

    if (signalStrength->getWcdmaSignalStrength() != nullptr) {
        if(signalStrength->getWcdmaSignalStrength()->getSignalStrength()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "WCDMA Signal Strength: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "WCDMA Signal Strength: "
                 << signalStrength->getWcdmaSignalStrength()->getSignalStrength() << std::endl;
        }

        if(signalStrength->getWcdmaSignalStrength()->getDbm()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "WCDMA Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "WCDMA Signal Strength(in dBm): "
                 << signalStrength->getWcdmaSignalStrength()->getDbm() << std::endl;
        }

        if(signalStrength->getWcdmaSignalStrength()->getBitErrorRate()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "WCDMA Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "WCDMA Bit Error Rate: "
                 << signalStrength->getWcdmaSignalStrength()->getBitErrorRate() << std::endl;
        }

        if(signalStrength->getWcdmaSignalStrength()->getEcio()
             == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "WCDMA Energy per chip to Interference Power Ratio(in dB): "
                << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "WCDMA Energy per chip to Interference Power Ratio(in dB): "
                << signalStrength->getWcdmaSignalStrength()->getEcio() << std::endl;
        }

        if(signalStrength->getWcdmaSignalStrength()->getRscp()
             == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "WCDMA Reference Signal Code Power(in dBm): "
                << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "WCDMA Reference Signal Code Power(in dBm): "
                << signalStrength->getWcdmaSignalStrength()->getRscp() << std::endl;
        }

        PRINT_NOTIFICATION
            << "WCDMA Signal Level: "
            << MyPhoneHelper::signalLevelToString(
            signalStrength->getWcdmaSignalStrength()->getLevel()) << std::endl;
    }

    if (signalStrength->getNr5gSignalStrength() != nullptr) {
        std::shared_ptr<telux::tel::Nr5gSignalStrengthInfo> nr5GSigInfo =
            signalStrength->getNr5gSignalStrength();
        if(nr5GSigInfo->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "5G NR Signal Strength(in dBm): " << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "5G NR Signal Strength(in dBm): "
                 << nr5GSigInfo->getDbm() << std::endl;
        }
        if(nr5GSigInfo->getReferenceSignalReceiveQuality() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "5G NR Receive Quality(in dB): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "5G NR Receive Quality(in dB): "
                 << nr5GSigInfo->getReferenceSignalReceiveQuality() << std::endl;
        }
        if(nr5GSigInfo->getReferenceSignalSnr() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "5G Reference Signal SNR(in dB): "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "5G Reference Signal SNR(in dB): "
                 << nr5GSigInfo->getReferenceSignalSnr() * 0.1 << std::endl;
        }
        PRINT_NOTIFICATION << "5G Signal Level: "
            << MyPhoneHelper::signalLevelToString(nr5GSigInfo->getLevel()) << std::endl;
    }

    if (signalStrength->getNb1NtnSignalStrength() != nullptr) {
        if (signalStrength->getNb1NtnSignalStrength()->getSignalStrength()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "NB1 NTN Signal Strength: "<< "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "NB1 NTN Signal Strength: "
                << signalStrength->getNb1NtnSignalStrength()->getSignalStrength()
                << std::endl;
        }

        if (signalStrength->getNb1NtnSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "NB1 NTN Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            PRINT_NOTIFICATION << "NB1 NTN Reference Signal Receive Power(in dBm): "
                << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "NB1 NTN Signal Strength(in dBm): "
                << signalStrength->getNb1NtnSignalStrength()->getDbm() << std::endl;
            PRINT_NOTIFICATION << "NB1 NTN Reference Signal Receive Power(in dBm): "
                 << signalStrength->getNb1NtnSignalStrength()->getDbm() << std::endl;
        }

        if (signalStrength->getNb1NtnSignalStrength()->getRsrq()
             == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "NB1 NTN Reference Signal Receive Quality(in dB): "
                << "UNAVAILABLE" << std::endl;
        } else {
            PRINT_NOTIFICATION << "NB1 NTN Reference Signal Receive Quality(in dB): "
                << signalStrength->getNb1NtnSignalStrength()->getRsrq() << std::endl;
        }

        if (signalStrength->getNb1NtnSignalStrength()->getRssnr()
            == INVALID_SIGNAL_STRENGTH_VALUE) {
            PRINT_NOTIFICATION << "NB1 NTN Reference Signal SNR(in dB): " << "UNAVAILABLE"
                << std::endl;
        } else {
            PRINT_NOTIFICATION << "NB1 NTN Reference Signal SNR(in dB): "
                << signalStrength->getNb1NtnSignalStrength()->getRssnr() * 0.1 << std::endl;
        }

        PRINT_NOTIFICATION << "NB1 NTN Signal Level: "
            << MyPhoneHelper::signalLevelToString(
            signalStrength->getNb1NtnSignalStrength()->getLevel()) << std::endl;
    }
}

std::string MyPhoneHelper::signalLevelToString(telux::tel::SignalStrengthLevel level) {
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

std::string MyPhoneListener::getCurrentTime() {
    timeval tod;
    gettimeofday(&tod, NULL);
    std::stringstream ss;
    time_t tt = tod.tv_sec;
    char buffer[100];
    std::strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&tt));
    char currTime[120];
    snprintf(currTime, 120, "%s.%ld", buffer, tod.tv_usec / 1000);
    return std::string(currTime);
}

std::string MyPhoneListener::radioStateToString(telux::tel::RadioState radioState) {
    std::string state = "";
    switch (radioState) {
    case telux::tel::RadioState::RADIO_STATE_OFF:
        state = "Off";
        break;
    case telux::tel::RadioState::RADIO_STATE_UNAVAILABLE:
        state = "Unavailable";
        break;
    case telux::tel::RadioState::RADIO_STATE_ON:
        state = "On";
        break;
    default:
        state = "Unknown";
        break;
    }
    return state;
}

std::string MyPhoneHelper::radioTechToString(
    telux::tel::RadioTechnology radioTech) {
    std::string rtString = "";
    switch (radioTech) {
    case telux::tel::RadioTechnology::RADIO_TECH_GPRS:
        rtString = "GPRS";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_EDGE:
        rtString = "EDGE";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_UMTS:
        rtString = "UMTS";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_IS95A:
        rtString = "IS95A";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_IS95B:
        rtString = "IS95B";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_1xRTT:
        rtString = "1xRTT";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_EVDO_0:
        rtString = "EVDO_0";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_EVDO_A:
        rtString = "EVDO_A";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_HSUPA:
        rtString = "HSUPA";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_HSPA:
        rtString = "HSPA";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_EVDO_B:
        rtString = "EVDO_B";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_EHRPD:
        rtString = "EHRPD";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_LTE:
        rtString = "LTE";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_HSPAP:
        rtString = "HSPA+";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_GSM:
        rtString = "GSM";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_TD_SCDMA:
        rtString = "TD_SCDMA";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_IWLAN:
        rtString = "IWLAN";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_LTE_CA:
        rtString = "LTE_CA";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_NR5G:
        rtString = "NR5G";
        break;
    case telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN:
        rtString = "NB1_NTN";
        break;
    default:
        rtString = "Unknown";
        break;
    }
    return rtString;
}

std::string MyPhoneHelper::operatorInfoIsHomeToString(telux::common::BoolValue isHome) {
    std::string state = "";
    switch (isHome) {
    case telux::common::BoolValue::STATE_FALSE:
        state = "FALSE";
        break;
    case telux::common::BoolValue::STATE_TRUE:
        state = "TRUE";
        break;
    default:
        state = "UNKNOWN";
        break;
    }
    return state;
}

void MyVoiceServiceStateCallback::voiceServiceStateResponse(
    const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo,
    telux::common::ErrorCode error) {
    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "requestVoiceServiceState successful, Service State: "
                 << MyPhoneHelper::voiceServiceStateToString(serviceInfo->getVoiceServiceState())
                 << " , Radio Technology: "
                 << MyPhoneHelper::radioTechToString(serviceInfo->getRadioTechnology())
                 << std::endl;
    } else {
        PRINT_CB << "requestVoiceServiceState is failed, errorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

std::string MyPhoneHelper::voiceServiceStateToString(telux::tel::VoiceServiceState vocSrvState) {
    std::string state = "";
    switch (vocSrvState) {
    case telux::tel::VoiceServiceState::NOT_REG_AND_NOT_SEARCHING:
        state = "NOT_REG_AND_NOT_SEARCHING";
        break;
    case telux::tel::VoiceServiceState::REG_HOME:
        state = "REG_HOME";
        break;
    case telux::tel::VoiceServiceState::NOT_REG_AND_SEARCHING:
        state = "NOT_REG_AND_SEARCHING";
        break;
    case telux::tel::VoiceServiceState::REG_DENIED:
        state = "REG_DENIED";
        break;
    case telux::tel::VoiceServiceState::UNKNOWN:
        state = "UNKNOWN";
        break;
    case telux::tel::VoiceServiceState::REG_ROAMING:
        state = "REG_ROAMING";
        break;
    case telux::tel::VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_NOT_SEARCHING:
        state = "NOT_REG_AND_EMERGENCY_AVAILABLE_AND_NOT_SEARCHING";
        break;
    case telux::tel::VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_SEARCHING:
        state = "NOT_REG_AND_EMERGENCY_AVAILABLE_AND_SEARCHING";
        break;
    case telux::tel::VoiceServiceState::REG_DENIED_AND_EMERGENCY_AVAILABLE:
        state = "REG_DENIED_AND_EMERGENCY_AVAILABLE";
        break;
    case telux::tel::VoiceServiceState::UNKNOWN_AND_EMERGENCY_AVAILABLE:
        state = "UNKNOWN_AND_EMERGENCY_AVAILABLE";
        break;
    default:
        state = "Unknown";
        break;
    }
    return state;
}

void MyPhoneListener::onVoiceRadioTechnologyChanged(
    int phoneId, telux::tel::RadioTechnology radioTechnology) {
    std::cout << "\n";
    PRINT_NOTIFICATION << "Received unsol response, PhoneId " << phoneId << std::endl;
    PRINT_NOTIFICATION << "Changed Radio technology " << static_cast<int>(radioTechnology)
                       << std::endl;
}

void MyPhoneListener::onVoiceServiceStateChanged(
    int phoneId, const std::shared_ptr<telux::tel::VoiceServiceInfo> &srvInfo) {
    if (srvInfo) {
        auto voiceSrvState = srvInfo->getVoiceServiceState();
        if (voiceSrvcState_[phoneId-1] != voiceSrvState) {
            voiceSrvcState_[phoneId-1] = voiceSrvState;
            std::cout << "\n";
            PRINT_NOTIFICATION << "onVoiceServiceStateChanged: PhoneId = " << phoneId
                << ", State: " << MyPhoneHelper::voiceServiceStateToString(voiceSrvState);
            if (srvInfo->isEmergency()) {
                std::cout << ", Phone is in EMERGENCY_ONLY mode" <<
                ", Radio Technology: " <<
                MyPhoneHelper::radioTechToString(srvInfo->getRadioTechnology()) << std::endl;
            }
            if (srvInfo->isInService()) {
                std::cout << ", Phone is in HOME network mode" <<
                ", Radio Technology: " <<
                MyPhoneHelper::radioTechToString(srvInfo->getRadioTechnology()) << std::endl;
            }
            if (srvInfo->isOutOfService()) {
                std::cout << ", Phone is in OUT_OF_SERVICE mode" <<
                ", Radio Technology: " <<
                MyPhoneHelper::radioTechToString(srvInfo->getRadioTechnology()) << std::endl;
            }
        }
    }
}

std::string MyCellularCapabilityCallback::voiceServiceTechnologiesMaskToString(
    telux::tel::VoiceServiceTechnologiesMask vstMask) {
    std::string vocSrvTechStr = "";
    if (vstMask[static_cast<int>(telux::tel::VoiceServiceTechnology::VOICE_TECH_GW_CSFB)]) {
        vocSrvTechStr = "GW_CSFB ";
    }
    if (vstMask[static_cast<int>(telux::tel::VoiceServiceTechnology::VOICE_TECH_1x_CSFB)]) {
        vocSrvTechStr += "1x_CSFB ";
    }
    if (vstMask[static_cast<int>(telux::tel::VoiceServiceTechnology::VOICE_TECH_VOLTE)]) {
        vocSrvTechStr += "VOLTE";
    }
    if (vocSrvTechStr.empty()) {
        vocSrvTechStr = "Unknown";
    }
    return vocSrvTechStr;
}

std::string MyCellularCapabilityCallback::ratCapabilitiesMaskToString(
    telux::tel::RATCapabilitiesMask ratCapabilitiesMask) {
    std::string ratCapStr = "";
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::AMPS)]) {
        ratCapStr = "AMPS ";
    }
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::CDMA)]) {
        ratCapStr += "CDMA ";
    }
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::HDR)]) {
        ratCapStr += "HDR ";
    }
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::GSM)]) {
        ratCapStr += "GSM ";
    }
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::WCDMA)]) {
        ratCapStr += "WCDMA ";
    }
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::LTE)]) {
        ratCapStr += "LTE ";
    }
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::TDS)]) {
        ratCapStr += "TDS ";
    }
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::NR5G)]) {
        ratCapStr += "NR5G(NSA)";
    }
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::NR5GSA)]) {
        ratCapStr += "NR5G(SA)";
    }
    if (ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::NB1_NTN)]) {
        ratCapStr += "NB1_NTN";
    }
    if (ratCapStr.empty()) {
        ratCapStr = "Unknown";
    }
    return ratCapStr;
}

void MyCellularCapabilityCallback::cellularCapabilityResponse(
    telux::tel::CellularCapabilityInfo capabilityInfo, telux::common::ErrorCode error) {
    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "requestCellularCapability response successful" << std::endl;
        PRINT_CB << "VoiceServiceTechnologiesMask: "
                 << voiceServiceTechnologiesMaskToString(capabilityInfo.voiceServiceTechs)
                 << std::endl;

        for (auto &simRatCap : capabilityInfo.simRatCapabilities) {
            PRINT_CB << "Sim RAT capabilities for Slot Id: " << simRatCap.slotId << "\n";
            PRINT_CB
                << "RATCapabilitiesMask: " << ratCapabilitiesMaskToString(simRatCap.capabilities)
                << std::endl;
        }
        for (auto &deviceRatCap : capabilityInfo.deviceRatCapability) {
            PRINT_CB << "Device static capabilities for Slot Id: " << deviceRatCap.slotId << "\n";
            PRINT_CB << "Device static capabilities Mask: "
                     << ratCapabilitiesMaskToString(deviceRatCap.capabilities) << "\n";
        }

        PRINT_CB << "SIM Count : " << capabilityInfo.simCount << std::endl;
        PRINT_CB << "Max Active SIMs : " << capabilityInfo.maxActiveSims << std::endl;
    } else {
        PRINT_CB << "requestCellularCapability is failed, errorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

std::string MyPhoneHelper::operatingModeToString(telux::tel::OperatingMode operatingMode) {
    std::string mode = "";
    switch (operatingMode) {
    case telux::tel::OperatingMode::ONLINE:
        mode = "ONLINE";
        break;
    case telux::tel::OperatingMode::AIRPLANE:
        mode = "AIRPLANE";
        break;
    case telux::tel::OperatingMode::FACTORY_TEST:
        mode = "FACTORY_TEST";
        break;
    case telux::tel::OperatingMode::OFFLINE:
        mode = "OFFLINE";
        break;
    case telux::tel::OperatingMode::RESETTING:
        mode = "RESETTING";
        break;
    case telux::tel::OperatingMode::SHUTTING_DOWN:
        mode = "SHUTTING_DOWN";
        break;
    case telux::tel::OperatingMode::PERSISTENT_LOW_POWER:
        mode = "PERSISTENT_LOW_POWER";
        break;
    default:
        mode = "Unknown";
        break;
    }
    return mode;
}

void MyGetOperatingModeCallback::operatingModeResponse(
    telux::tel::OperatingMode operatingMode, telux::common::ErrorCode error) {
    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "requestOperatingMode response successful" << std::endl;
        PRINT_CB << "Operating Mode: " << MyPhoneHelper::operatingModeToString(operatingMode)
                 << std::endl;
    } else {
        PRINT_CB << "requestOperatingMode is failed, errorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void MyPhoneListener::onOperatingModeChanged(telux::tel::OperatingMode mode) {
    std::cout << "\n";
    PRINT_NOTIFICATION << "Received Operating Mode Change " << std::endl;
    PRINT_NOTIFICATION << "Operating Mode: " << MyPhoneHelper::operatingModeToString(mode)
                       << std::endl;
}

void MySetOperatingModeCallback::setOperatingModeResponse(telux::common::ErrorCode error) {
    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Set operating mode request successful" << std::endl;
    } else {
        PRINT_CB << "Set operating mode request failed" << std::endl;
    }
    PRINT_CB << "SetOperatingModeRequest error: " << static_cast<int>(error)
             << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}

void MyPhoneListener::onCellInfoListChanged(
    int phoneId, std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList) {
    PRINT_NOTIFICATION << "Received unsol response for PhoneId " << phoneId << std::endl;
    MyPhoneHelper::printCellInfoDetails(cellInfoList);
}

void MyPhoneHelper::printCellInfoDetails(
    std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList) {
    for (auto cellinfo : cellInfoList) {
        PRINT_NOTIFICATION << "CellInfo Type: " << (int)cellinfo->getType() << std::endl;
        if (cellinfo->getType() == telux::tel::CellType::GSM) {
            auto gsmCellInfo = std::static_pointer_cast<telux::tel::GsmCellInfo>(cellinfo);
            PRINT_NOTIFICATION << "GSM isRegistered: " << gsmCellInfo->isRegistered() << std::endl;
            PRINT_NOTIFICATION << "GSM mcc: "
                << gsmCellInfo->getCellIdentity().getMobileCountryCode() << std::endl;
            PRINT_NOTIFICATION << "GSM mnc: "
                << gsmCellInfo->getCellIdentity().getMobileNetworkCode() << std::endl;
            if(gsmCellInfo->getCellIdentity().getLac() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "GSM lac: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_NOTIFICATION << "GSM lac: " << gsmCellInfo->getCellIdentity().getLac()
                               << std::endl;
            }
            if(gsmCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "GSM cid: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_NOTIFICATION << "GSM cid: " << gsmCellInfo->getCellIdentity().getIdentity()
                               << std::endl;
            }
            PRINT_NOTIFICATION << "GSM arfcn: " << gsmCellInfo->getCellIdentity().getArfcn()
                               << std::endl;
            // GSM signal strength
            if(gsmCellInfo->getSignalStrengthInfo().getGsmSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "GSM Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "GSM Signal Strength: "
                    << gsmCellInfo->getSignalStrengthInfo().getGsmSignalStrength() << std::endl;
            }

            if(gsmCellInfo->getSignalStrengthInfo().getGsmBitErrorRate()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "GSM Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "GSM Bit Error Rate: "
                   << gsmCellInfo->getSignalStrengthInfo().getGsmBitErrorRate()<< std::endl;
            }

            if(gsmCellInfo->getSignalStrengthInfo().getDbm()== INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "GSM Signal Strength(in dBm): " << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "GSM Signal Strength(in dBm): "
                   << gsmCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(gsmCellInfo->getSignalStrengthInfo().getTimingAdvance()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "GSM Timing Advance(in bit periods): " << "UNAVAILABLE"
                << std::endl;
            } else {
               PRINT_NOTIFICATION << "GSM Timing Advance(in bit periods): "
                   << gsmCellInfo->getSignalStrengthInfo().getTimingAdvance() << std::endl;
            }

            PRINT_NOTIFICATION << "GSM Signal Level: "
               << signalLevelToString(gsmCellInfo->getSignalStrengthInfo().getLevel())<< std::endl;
        } else if (cellinfo->getType() == telux::tel::CellType::LTE) {
            auto lteCellInfo = std::static_pointer_cast<telux::tel::LteCellInfo>(cellinfo);
            PRINT_NOTIFICATION << "LTE isRegistered: " << lteCellInfo->isRegistered() << std::endl;
            PRINT_NOTIFICATION << "LTE mcc: "
                << lteCellInfo->getCellIdentity().getMobileCountryCode() << std::endl;
            PRINT_NOTIFICATION << "LTE mnc: "
                << lteCellInfo->getCellIdentity().getMobileNetworkCode() << std::endl;
            if(lteCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "LTE cid: " << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "LTE cid: " << lteCellInfo->getCellIdentity().getIdentity()
                               << std::endl;
            }
            PRINT_NOTIFICATION << "LTE pid: " << lteCellInfo->getCellIdentity().getPhysicalCellId()
                               << std::endl;
            if(lteCellInfo->getCellIdentity().getTrackingAreaCode()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "LTE tac: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_NOTIFICATION
                    << "LTE tac: " << lteCellInfo->getCellIdentity().getTrackingAreaCode()
                    << std::endl;
            }
            PRINT_NOTIFICATION << "LTE arfcn: " << lteCellInfo->getCellIdentity().getEarfcn()
                               << std::endl;
            // LTE Signal Strength
            if(lteCellInfo->getSignalStrengthInfo().getLteSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "LTE Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "LTE Signal Strength: "
                    << lteCellInfo->getSignalStrengthInfo().getLteSignalStrength() << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "LTE Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "LTE Signal Strength(in dBm): "
                    << lteCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "LTE Reference Signal Receive Power(in dBm): "<< "UNAVAILABLE"
               << std::endl;
            } else {
               PRINT_NOTIFICATION << "LTE Reference Signal Receive Power(in dBm): "
                    << lteCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalReceiveQuality()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "LTE Reference Signal Receive Quality(in dB): "
                   << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "LTE Reference Signal Receive Quality(in dB): "
                   << lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalReceiveQuality()
                   << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getTimingAdvance() ==
                INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "LTE Timing Advance: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "LTE Timing Advance: "
                    << lteCellInfo->getSignalStrengthInfo().getTimingAdvance()
                    << std::endl;
            }
            PRINT_NOTIFICATION << "LTE Signal Level: "
            << signalLevelToString(lteCellInfo->getSignalStrengthInfo().getLevel())
             << std::endl;
        } else if (cellinfo->getType() == telux::tel::CellType::WCDMA) {
            auto wcdmaCellInfo = std::static_pointer_cast<telux::tel::WcdmaCellInfo>(cellinfo);
            PRINT_NOTIFICATION << "WCDMA isRegistered: " << wcdmaCellInfo->isRegistered()
                               << std::endl;
            PRINT_NOTIFICATION << "WCDMA mcc: "
                << wcdmaCellInfo->getCellIdentity().getMobileCountryCode() << std::endl;
            PRINT_NOTIFICATION << "WCDMA mnc: "
                << wcdmaCellInfo->getCellIdentity().getMobileNetworkCode() << std::endl;
            if(wcdmaCellInfo->getCellIdentity().getLac() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "WCDMA lac: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_NOTIFICATION << "WCDMA lac: " << wcdmaCellInfo->getCellIdentity().getLac()
                               << std::endl;
            }
            if(wcdmaCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "WCDMA cid: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_NOTIFICATION << "WCDMA cid: "
                    << wcdmaCellInfo->getCellIdentity().getIdentity() << std::endl;
            }
            PRINT_NOTIFICATION
                << "WCDMA psc: " << wcdmaCellInfo->getCellIdentity().getPrimaryScramblingCode()
                << std::endl;
            PRINT_NOTIFICATION << "WCDMA arfcn: " << wcdmaCellInfo->getCellIdentity().getUarfcn()
                               << std::endl;
            // WCDMA Signal Strength
            if(wcdmaCellInfo->getSignalStrengthInfo().getSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "WCDMA Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "WCDMA Signal Strength: "
                    << wcdmaCellInfo->getSignalStrengthInfo().getSignalStrength() << std::endl;
            }

            if(wcdmaCellInfo->getSignalStrengthInfo().getDbm()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "WCDMA Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "WCDMA Signal Strength(in dBm): "
                    << wcdmaCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(wcdmaCellInfo->getSignalStrengthInfo().getBitErrorRate()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "WCDMA Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "WCDMA Bit Error Rate: "
                    << wcdmaCellInfo->getSignalStrengthInfo().getBitErrorRate() << std::endl;
            }
            PRINT_NOTIFICATION
               << "WCDMA Signal Level: "
               << signalLevelToString(wcdmaCellInfo->getSignalStrengthInfo().getLevel())
               << std::endl;
        } else if(cellinfo->getType() == telux::tel::CellType::NR5G) {
            auto nr5gCellInfo = std::static_pointer_cast<telux::tel::Nr5gCellInfo>(cellinfo);
            PRINT_NOTIFICATION << "NR5G isRegistered: " << nr5gCellInfo->isRegistered()
                << std::endl;
            PRINT_NOTIFICATION << "NR5G mcc: "
                << nr5gCellInfo->getCellIdentity().getMobileCountryCode() << std::endl;
            PRINT_NOTIFICATION << "NR5G mnc: "
                << nr5gCellInfo->getCellIdentity().getMobileNetworkCode() << std::endl;
            if(nr5gCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "NR5G cid: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_NOTIFICATION << "NR5G cid: " << nr5gCellInfo->getCellIdentity().getIdentity()
                << std::endl;
            }
            PRINT_NOTIFICATION << "NR5G pid: "
                << nr5gCellInfo->getCellIdentity().getPhysicalCellId() << std::endl;
            if(nr5gCellInfo->getCellIdentity().getTrackingAreaCode()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "NR5G tac: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_NOTIFICATION << "NR5G tac: "
                    << nr5gCellInfo->getCellIdentity().getTrackingAreaCode() << std::endl;
            }
            PRINT_NOTIFICATION << "NR5G arfcn: "
                << nr5gCellInfo->getCellIdentity().getArfcn() << std::endl;
            // NR5G Signal Strength
            if(nr5gCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "NR5G Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "NR5G Signal Strength(in dBm): "
                    << nr5gCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(nr5gCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "NR5G Reference Signal Receive Power(in dBm): "<< "UNAVAILABLE"
               << std::endl;
            } else {
               PRINT_NOTIFICATION << "NR5G Reference Signal Receive Power(in dBm): "
                    << nr5gCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(nr5gCellInfo->getSignalStrengthInfo().getReferenceSignalReceiveQuality()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "NR5G Reference Signal Receive Quality(in dB): "
                   << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "NR5G Reference Signal Receive Quality(in dB): "
                   << nr5gCellInfo->getSignalStrengthInfo().getReferenceSignalReceiveQuality()
                   << std::endl;
            }

            if(nr5gCellInfo->getSignalStrengthInfo().getReferenceSignalSnr()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "NR5G Reference Signal SNR(in dB): "<< "UNAVAILABLE"
                    << std::endl;
            } else {
               PRINT_NOTIFICATION << "NR5G Reference Signal SNR(in dB): "
                    << nr5gCellInfo->getSignalStrengthInfo().getReferenceSignalSnr() * 0.1
                    << std::endl;
            }

            PRINT_NOTIFICATION << "NR5G Signal Level: "
            << signalLevelToString(nr5gCellInfo->getSignalStrengthInfo().getLevel())
             << std::endl;
         } else if (cellinfo->getType() == telux::tel::CellType::NB1_NTN) {
            auto nb1NtnCellInfo = std::static_pointer_cast<telux::tel::Nb1NtnCellInfo>(cellinfo);
            PRINT_NOTIFICATION << "NB1 NTN isRegistered: " << nb1NtnCellInfo->isRegistered()
                << std::endl;
            PRINT_NOTIFICATION << "NB1 NTN mcc: "
                << nb1NtnCellInfo->getCellIdentity().getMobileCountryCode() << std::endl;
            PRINT_NOTIFICATION << "NB1 NTN mnc: "
                << nb1NtnCellInfo->getCellIdentity().getMobileNetworkCode() << std::endl;
            if (nb1NtnCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "NB1 NTN cid: " << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "NB1 NTN cid: "
                   << nb1NtnCellInfo->getCellIdentity().getIdentity() << std::endl;
            }
            if (nb1NtnCellInfo->getCellIdentity().getTrackingAreaCode()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "NB1 NTN tac: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_NOTIFICATION
                    << "NB1 NTN tac: " << nb1NtnCellInfo->getCellIdentity().getTrackingAreaCode()
                    << std::endl;
            }
            PRINT_NOTIFICATION << "NB1 NTN arfcn: " << nb1NtnCellInfo->getCellIdentity().getEarfcn()
                << std::endl;

            // NB1 NTN Signal Strength
            if (nb1NtnCellInfo->getSignalStrengthInfo().getSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_NOTIFICATION << "NB1 NTN Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "NB1 NTN Signal Strength: "
                   << nb1NtnCellInfo->getSignalStrengthInfo().getSignalStrength()
                   << std::endl;
            }

            if (nb1NtnCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE)
            {
               PRINT_NOTIFICATION << "NB1 NTN Signal Strength(in dBm): " << "UNAVAILABLE"
                   << std::endl;
               PRINT_NOTIFICATION << "NB1 NTN Reference Signal Receive Power(in dBm): "
                   << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_NOTIFICATION << "NB1 NTN Signal Strength(in dBm): "
                    << nb1NtnCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
               PRINT_NOTIFICATION << "NB1 NTN Reference Signal Receive Power(in dBm): "
                   << nb1NtnCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if (nb1NtnCellInfo->getSignalStrengthInfo().getRsrq() == INVALID_SIGNAL_STRENGTH_VALUE)
            {
                PRINT_NOTIFICATION << "NB1 NTN Reference Signal Receive Quality(in dB): "
                    << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_NOTIFICATION << "NB1 NTN Reference Signal Receive Quality(in dB): "
                    << nb1NtnCellInfo->getSignalStrengthInfo().getRsrq() << std::endl;
            }

            if (nb1NtnCellInfo->getSignalStrengthInfo().getRssnr()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_NOTIFICATION << "NB1 NTN Reference Signal SNR(in dB): " << "UNAVAILABLE"
                    << std::endl;
            } else {
               PRINT_NOTIFICATION << "NB1 NTN Reference Signal SNR(in dB): "
                    << nb1NtnCellInfo->getSignalStrengthInfo().getRssnr() * 0.1 << std::endl;
            }

            PRINT_NOTIFICATION << "NB1 NTN Signal Level: "
                << signalLevelToString(nb1NtnCellInfo->getSignalStrengthInfo().getLevel())
                << std::endl;
        }
    }
}

void MyPhoneListener::onECallOperatingModeChange(int phoneId, telux::tel::ECallModeInfo modeInfo) {

    PRINT_NOTIFICATION << "onECallOperatingModeChange for PhoneId = " << phoneId
                       << " , mode = " << MyPhoneHelper::eCallOperatingModeToString(modeInfo.mode)
                       << " , reason = " << eCallModeReasonToString(modeInfo.reason) << std::endl;
}

std::string MyPhoneHelper::eCallOperatingModeToString(telux::tel::ECallMode mode) {
    std::string eCallOprtModeString = "UNKNOWN";
    switch (mode) {
    case telux::tel::ECallMode::NORMAL:
        eCallOprtModeString = "NORMAL";
        break;
    case telux::tel::ECallMode::ECALL_ONLY:
        eCallOprtModeString = "ECALL_ONLY";
        break;
    case telux::tel::ECallMode::NONE:
        eCallOprtModeString = "NONE";
        break;
    default:
        break;
    }
    return eCallOprtModeString;
}

std::string MyPhoneListener::eCallModeReasonToString(telux::tel::ECallModeReason modeReason) {
    std::string reason = "";
    switch (modeReason) {
    case telux::tel::ECallModeReason::NORMAL:
        reason = "NORMAL";
        break;
    case telux::tel::ECallModeReason::ERA_GLONASS:
        reason = "ERA_GLONASS";
        break;
    default:
        reason = "UNKNOWN";
        break;
    }
    return reason;
}

void MyPhoneListener::onOperatorInfoChange(int phoneId, telux::tel::PlmnInfo info) {

    PRINT_NOTIFICATION << "Operator information changes for PhoneId = " << phoneId
                       << " , short name = " << info.shortName
                       << " , long name = " << info.longName
                       << " , plmn = " << info.plmn
                       << " , is from home network = "
                       <<  MyPhoneHelper::operatorInfoIsHomeToString(info.isHome)
                       << std::endl;
}

void MySetECallOperatingModeCallback::setECallOperatingModeResponse(
    telux::common::ErrorCode error) {
    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Set eCall operating mode request executed successfully" << std::endl;
    } else {
        PRINT_CB << "Set eCall operating mode request failed" << std::endl;
    }
    PRINT_CB << "SetECallOperatingModeRequest error: " << Utils::getErrorCodeAsString(error)
             << std::endl;
}

void MyGetECallOperatingModeCallback::getECallOperatingModeResponse(
    telux::tel::ECallMode eCallMode, telux::common::ErrorCode error) {

    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "eCall operating mode request executed successfully" << std::endl;
        PRINT_CB << "eCall Operating Mode: " << MyPhoneHelper::eCallOperatingModeToString(eCallMode)
                 << std::endl;
    } else {
        PRINT_CB << "Request eCall Operating Mode failed, errorCode: "
                 << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void MyOperatorInfoCallback::requestOperatorInfoCb(telux::tel::PlmnInfo info,
    telux::common::ErrorCode error) {
   std::cout << "\n";
   if (error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Operator long name: " << info.longName
          << ", short name: " << info.shortName
          << ", plmn: " << info.plmn
          << " , is from home network = "
          << MyPhoneHelper::operatorInfoIsHomeToString(info.isHome)
          << "\n";
   } else {
      PRINT_CB << "Operator name request failed with errorCode: "
          << Utils::getErrorCodeAsString(error) << "\n";
   }
}

void MyResetWwanCallback::resetWwanResponse(telux::common::ErrorCode error) {

    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "reset WWAN request executed successfully" << std::endl;
    } else {
        PRINT_CB << "Request reset WWAN failed, errorCode: "
                 << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void MyConfigureSignalStrengthCallback::configureSignalStrengthResponse(
        telux::common::ErrorCode error) {

    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Configure SignalStrength request executed successfully" << std::endl;
    } else {
        PRINT_CB << "Request Configure SignalStrength failed, errorCode: "
                 << Utils::getErrorCodeAsString(error) << std::endl;
    }
}
