/*
 *  Copyright (c) 2018, 2021 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "iostream"
#include "MyCellInfoHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

using namespace telux::tel;
using namespace telux::common;

std::string MyCellInfoCallback::signalLevelToString(telux::tel::SignalStrengthLevel level) {
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

void MyCellInfoCallback::cellInfoListResponse(
   std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList,
   telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Received call back for requestCellInfo in MyCellInfoCallback" << std::endl;
      for(auto cellinfo : cellInfoList) {
         PRINT_CB << "CellInfo Type: " << (int)cellinfo->getType() << std::endl;
         if(cellinfo->getType() == telux::tel::CellType::GSM) {
            PRINT_CB << "GSM Cellinfo " << std::endl;
            auto gsmCellInfo = std::static_pointer_cast<telux::tel::GsmCellInfo>(cellinfo);
            PRINT_CB << "GSM isRegistered: " << gsmCellInfo->isRegistered() << std::endl;
            PRINT_CB << "GSM mcc: " << gsmCellInfo->getCellIdentity().getMobileCountryCode()
                << std::endl;
            PRINT_CB << "GSM mnc: " << gsmCellInfo->getCellIdentity().getMobileNetworkCode()
                << std::endl;
            if(gsmCellInfo->getCellIdentity().getLac() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "GSM lac: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "GSM lac: " << gsmCellInfo->getCellIdentity().getLac() << std::endl;
            }
            if(gsmCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "GSM cid: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "GSM cid: " << gsmCellInfo->getCellIdentity().getIdentity()
                    << std::endl;
            }
            PRINT_CB << "GSM arfcn: " << gsmCellInfo->getCellIdentity().getArfcn() << std::endl;
            // GSM signal strength
            if(gsmCellInfo->getSignalStrengthInfo().getGsmSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "GSM Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "GSM Signal Strength: "
                    << gsmCellInfo->getSignalStrengthInfo().getGsmSignalStrength() << std::endl;
            }

            if(gsmCellInfo->getSignalStrengthInfo().getGsmBitErrorRate()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "GSM Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "GSM Bit Error Rate: "
                   << gsmCellInfo->getSignalStrengthInfo().getGsmBitErrorRate()<< std::endl;
            }

            if(gsmCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "GSM Signal Strength(in dBm): " << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "GSM Signal Strength(in dBm): "
                   << gsmCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(gsmCellInfo->getSignalStrengthInfo().getTimingAdvance()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "GSM Timing Advance(in bit periods): " << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "GSM Timing Advance(in bit periods): "
                   << gsmCellInfo->getSignalStrengthInfo().getTimingAdvance() << std::endl;
            }

            PRINT_CB << "GSM Signal Level: "
               << signalLevelToString(gsmCellInfo->getSignalStrengthInfo().getLevel())<< std::endl;

         } else if(cellinfo->getType() == telux::tel::CellType::LTE) {
            PRINT_CB << "LTE Cellinfo  " << std::endl;
            auto lteCellInfo = std::static_pointer_cast<telux::tel::LteCellInfo>(cellinfo);
            PRINT_CB << "LTE isRegistered: " << lteCellInfo->isRegistered() << std::endl;
            PRINT_CB << "LTE mcc: " << lteCellInfo->getCellIdentity().getMobileCountryCode()
                << std::endl;
            PRINT_CB << "LTE mnc: " << lteCellInfo->getCellIdentity().getMobileNetworkCode()
                << std::endl;
            if(lteCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "LTE cid: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "LTE cid: " << lteCellInfo->getCellIdentity().getIdentity()
                    << std::endl;
            }
            PRINT_CB << "LTE pid: " << lteCellInfo->getCellIdentity().getPhysicalCellId()
                     << std::endl;
            if(lteCellInfo->getCellIdentity().getTrackingAreaCode()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE tac: " << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE tac: " << lteCellInfo->getCellIdentity().getTrackingAreaCode()
                     << std::endl;
            }
            PRINT_CB << "LTE arfcn: " << lteCellInfo->getCellIdentity().getEarfcn() << std::endl;
            // LTE Signal Strength

            if(lteCellInfo->getSignalStrengthInfo().getLteSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Signal Strength: "
                    << lteCellInfo->getSignalStrengthInfo().getLteSignalStrength() << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Signal Strength(in dBm): "
                    << lteCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Reference Signal Receive Power(in dBm): "<< "UNAVAILABLE"
               << std::endl;
            } else {
               PRINT_CB << "LTE Reference Signal Receive Power(in dBm): "
                    << lteCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalReceiveQuality()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Reference Signal Receive Quality(in dB): "
                   << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Reference Signal Receive Quality(in dB): "
                   << lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalReceiveQuality()
                   << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getTimingAdvance()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Timing Advance: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Timing Advance: "
                    << lteCellInfo->getSignalStrengthInfo().getTimingAdvance()
                    << std::endl;
            }
            PRINT_CB << "LTE Signal Level: "
            << signalLevelToString(lteCellInfo->getSignalStrengthInfo().getLevel())
             << std::endl;
         } else if(cellinfo->getType() == telux::tel::CellType::WCDMA) {
            PRINT_CB << "WCDMA Cellinfo " << std::endl;
            auto wcdmaCellInfo = std::static_pointer_cast<telux::tel::WcdmaCellInfo>(cellinfo);
            PRINT_CB << "WCDMA isRegistered: " << wcdmaCellInfo->isRegistered() << std::endl;
            PRINT_CB << "WCDMA mcc: " << wcdmaCellInfo->getCellIdentity().getMobileCountryCode()
                << std::endl;
            PRINT_CB << "WCDMA mnc: " << wcdmaCellInfo->getCellIdentity().getMobileNetworkCode()
                << std::endl;
            if(wcdmaCellInfo->getCellIdentity().getLac() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "WCDMA lac: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "WCDMA lac: " << wcdmaCellInfo->getCellIdentity().getLac()
                    << std::endl;
            }
            if(wcdmaCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "WCDMA cid: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "WCDMA cid: " << wcdmaCellInfo->getCellIdentity().getIdentity()
                     << std::endl;
            }
            PRINT_CB << "WCDMA psc: " << wcdmaCellInfo->getCellIdentity().getPrimaryScramblingCode()
                     << std::endl;
            PRINT_CB << "WCDMA arfcn: " << wcdmaCellInfo->getCellIdentity().getUarfcn()
                     << std::endl;
            // WCDMA Signal Strength
            if(wcdmaCellInfo->getSignalStrengthInfo().getSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "WCDMA Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "WCDMA Signal Strength: "
                    << wcdmaCellInfo->getSignalStrengthInfo().getSignalStrength() << std::endl;
            }

            if(wcdmaCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "WCDMA Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "WCDMA Signal Strength(in dBm): "
                    << wcdmaCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(wcdmaCellInfo->getSignalStrengthInfo().getBitErrorRate()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "WCDMA Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "WCDMA Bit Error Rate: "
                    << wcdmaCellInfo->getSignalStrengthInfo().getBitErrorRate() << std::endl;
            }
            if (wcdmaCellInfo->getSignalStrengthInfo().getEcio()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "WCDMA Energy per chip to Interference Power Ratio(in dB): "
                    << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "WCDMA Energy per chip to Interference Power Ratio(in dB): "
                    << wcdmaCellInfo->getSignalStrengthInfo().getEcio() << std::endl;
            }

            if (wcdmaCellInfo->getSignalStrengthInfo().getRscp()
                 == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "WCDMA Reference Signal Code Power(in dBm): "
                    << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "WCDMA Reference Signal Code Power(in dBm): "
                    << wcdmaCellInfo->getSignalStrengthInfo().getRscp() << std::endl;
            }

            PRINT_CB
               << "WCDMA Signal Level: "
               << signalLevelToString(wcdmaCellInfo->getSignalStrengthInfo().getLevel())
               << std::endl;
         } else if(cellinfo->getType() == telux::tel::CellType::NR5G) {
            PRINT_CB << "NR5G Cellinfo  " << std::endl;
            auto nr5gCellInfo = std::static_pointer_cast<telux::tel::Nr5gCellInfo>(cellinfo);
            PRINT_CB << "NR5G isRegistered: " << nr5gCellInfo->isRegistered() << std::endl;
            PRINT_CB << "NR5G mcc: " << nr5gCellInfo->getCellIdentity().getMobileCountryCode()
                << std::endl;
            PRINT_CB << "NR5G mnc: " << nr5gCellInfo->getCellIdentity().getMobileNetworkCode()
                << std::endl;
            if(nr5gCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "NR5G cid: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "NR5G cid: " << nr5gCellInfo->getCellIdentity().getIdentity()
                    << std::endl;
            }
            PRINT_CB << "NR5G pid: " << nr5gCellInfo->getCellIdentity().getPhysicalCellId()
                     << std::endl;
            if(nr5gCellInfo->getCellIdentity().getTrackingAreaCode()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "NR5G tac: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "NR5G tac: " << nr5gCellInfo->getCellIdentity().getTrackingAreaCode()
                     << std::endl;
            }
            PRINT_CB << "NR5G arfcn: " << nr5gCellInfo->getCellIdentity().getArfcn() << std::endl;
            // NR5G Signal Strength

            if(nr5gCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "NR5G Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "NR5G Signal Strength(in dBm): "
                    << nr5gCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(nr5gCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "NR5G Reference Signal Receive Power(in dBm): "<< "UNAVAILABLE"
               << std::endl;
            } else {
               PRINT_CB << "NR5G Reference Signal Receive Power(in dBm): "
                    << nr5gCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(nr5gCellInfo->getSignalStrengthInfo().getReferenceSignalReceiveQuality()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "NR5G Reference Signal Receive Quality(in dB): "
                   << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "NR5G Reference Signal Receive Quality(in dB): "
                   << nr5gCellInfo->getSignalStrengthInfo().getReferenceSignalReceiveQuality()
                   << std::endl;
            }

            if(nr5gCellInfo->getSignalStrengthInfo().getReferenceSignalSnr()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "NR5G Reference Signal SNR(in dB): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "NR5G Reference Signal SNR(in dB): "
                    << nr5gCellInfo->getSignalStrengthInfo().getReferenceSignalSnr() * 0.1
                    << std::endl;
            }

            PRINT_CB << "NR5G Signal Level: "
            << signalLevelToString(nr5gCellInfo->getSignalStrengthInfo().getLevel())
             << std::endl;
         } else if(cellinfo->getType() == telux::tel::CellType::NB1_NTN) {
            PRINT_CB << "NB1 NTN Cellinfo  " << std::endl;
            auto nb1NtnCellInfo = std::static_pointer_cast<telux::tel::Nb1NtnCellInfo>(cellinfo);
            PRINT_CB << "NB1 NTN isRegistered: " << nb1NtnCellInfo->isRegistered() << std::endl;
            PRINT_CB << "NB1 NTN mcc: " << nb1NtnCellInfo->getCellIdentity().getMobileCountryCode()
                << std::endl;
            PRINT_CB << "NB1 NTN mnc: " << nb1NtnCellInfo->getCellIdentity().getMobileNetworkCode()
                << std::endl;
            if (nb1NtnCellInfo->getCellIdentity().getIdentity() == INVALID_SIGNAL_STRENGTH_VALUE) {
                PRINT_CB << "NB1 NTN cid: " << "UNAVAILABLE" << std::endl;
            } else {
                PRINT_CB << "NB1 NTN cid: " << nb1NtnCellInfo->getCellIdentity().getIdentity()
                    << std::endl;
            }
            if (nb1NtnCellInfo->getCellIdentity().getTrackingAreaCode()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "NB1 NTN tac: " << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "NB1 NTN tac: "
                   << nb1NtnCellInfo->getCellIdentity().getTrackingAreaCode() << std::endl;
            }
            PRINT_CB << "NB1 NTN arfcn: " << nb1NtnCellInfo->getCellIdentity().getEarfcn()
                << std::endl;

            // NB1 NTN Signal Strength
            if (nb1NtnCellInfo->getSignalStrengthInfo().getSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "NB1 NTN Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "NB1 NTN Signal Strength: "
                   << nb1NtnCellInfo->getSignalStrengthInfo().getSignalStrength()
                   << std::endl;
            }

            if (nb1NtnCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "NB1 NTN Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
               PRINT_CB << "NB1 NTN Reference Signal Receive Power(in dBm): "<< "UNAVAILABLE"
                   << std::endl;
            } else {
               PRINT_CB << "NB1 NTN Signal Strength(in dBm): "
                    << nb1NtnCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
               PRINT_CB << "NB1 NTN Reference Signal Receive Power(in dBm): "
                    << nb1NtnCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if (nb1NtnCellInfo->getSignalStrengthInfo().getRsrq() == INVALID_SIGNAL_STRENGTH_VALUE)
            {
               PRINT_CB << "NB1 NTN Reference Signal Receive Quality(in dB): "
                   << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "NB1 NTN Reference Signal Receive Quality(in dB): " <<
                   nb1NtnCellInfo->getSignalStrengthInfo().getRsrq() << std::endl;
            }

            if (nb1NtnCellInfo->getSignalStrengthInfo().getRssnr()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "NB1 NTN Reference Signal SNR(in dB): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "NB1 NTN Reference Signal SNR(in dB): "
                    << nb1NtnCellInfo->getSignalStrengthInfo().getRssnr() * 0.1 << std::endl;
            }

            PRINT_CB << "NB1 NTN Signal Level: "
                << signalLevelToString(nb1NtnCellInfo->getSignalStrengthInfo().getLevel())
                << std::endl;
         }
      }
   } else {
      PRINT_CB << "RequestCellInfo failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyCellInfoCallback::cellInfoListRateResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Set cell info list rate request executed successfully" << std::endl;
   } else {
      PRINT_CB << "Set cell info list rate request failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}
