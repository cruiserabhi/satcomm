/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <map>
#include "telux/tel/SignalStrength.hpp"
#include "common/Logger.hpp"

// Constants used for Signal Strength
#define SIGNAL_STRENGTH_UNKNOWN 99
#define SIGNAL_STRENGTH_DEFAULT -1
// GSM constants
#define MAX_GSM_LEVEL 31
#define MIN_GSM_LEVEL 0
#define GSM_MAX_BIT_ERROR_RATE 7
#define GSM_MIN_BIT_ERROR_RATE 0
#define GSM_MAX_TIMING_ADVANCE 219
#define GSM_MIN_TIMING_ADVANCE 0
#define GSM_DBM_CONVERSION_FACTOR -113
#define GSM_DBM_MULTIPLICATION_FACTOR 2

// CDMA & EVDO constants
#define MAX_CDMA_DBM 0
#define MIN_CDMA_DBM -120
#define MAX_CDMA_ECIO 0
#define MIN_CDMA_ECIO -160
#define MAX_EVDO_DBM 0
#define MIN_EVDO_DBM -120
#define MIN_EVDO_ECIO -160
#define MAX_EVDO_ECIO 0
#define MIN_EVDO_SNR 0
#define MAX_EVDO_SNR 8

// LTE constants
#define MAX_LTE_RSSNR_LEVEL 300
#define MIN_LTE_RSSNR_LEVEL -200
#define MIN_LTE_SIGNAL_STRENGTH 0
#define MAX_LTE_SIGNAL_STRENGTH 31
#define MIN_LTE_RSRP -140
#define MAX_LTE_RSRP -44
#define MIN_LTE_RSRQ -20
#define MAX_LTE_RSRQ -3
#define MIN_LTE_CQI 0
#define MAX_LTE_CQI 15
#define MIN_LTE_TIMING_ADVANCE 0
#define MAX_LTE_TIMING_ADVANCE 2147483646

// wcdma constants
#define MAX_WCDMA_LEVEL 31
#define MIN_WCDMA_LEVEL 0
#define MAX_WCDMA_BIT_ERROR_RATE 7
#define MIN_WCDMA_BIT_ERROR_RATE 0
#define MIN_WCDMA_ECIO -20
#define MAX_WCDMA_ECIO 0
#define MIN_WCDMA_RSCP -120
#define MAX_WCDMA_RSCP -24
#define WCDMA_DBM_CONVERSION_FACTOR -113
#define WCDMA_DBM_MULTIPLICATION_FACTOR 2

// TDSCDMA constants
#define MIN_TDSCDMA_RSCP -120
#define MAX_TDSCDMA_RSCP -25

// NR5G constants
#define MIN_NR5G_RSRP -140
#define MAX_NR5G_RSRP -44
#define MIN_NR5G_RSRQ -43
#define MAX_NR5G_RSRQ 20
#define MIN_NR5G_RSSNR_LEVEL -230
#define MAX_NR5G_RSSNR_LEVEL 400

// NB1 NTN constants
#define MAX_NB1_NTN_RSSNR_LEVEL 300
#define MIN_NB1_NTN_RSSNR_LEVEL -200
#define MIN_NB1_NTN_SIGNAL_STRENGTH 0
#define MAX_NB1_NTN_SIGNAL_STRENGTH 31
#define MIN_NB1_NTN_RSRP -140
#define MAX_NB1_NTN_RSRP -44
#define MIN_NB1_NTN_RSRQ -20
#define MAX_NB1_NTN_RSRQ -3

namespace telux {

namespace tel {

// Signal strength level map for gsm, lte and cdma, if value is greater than specified range
// return the signal level
// clang-format off
std::map<SignalStrengthLevel, int> gsmLevelMap {
   {SignalStrengthLevel::LEVEL_1, 0},
   {SignalStrengthLevel::LEVEL_2, 3},
   {SignalStrengthLevel::LEVEL_3, 5},
   {SignalStrengthLevel::LEVEL_4, 8},
   {SignalStrengthLevel::LEVEL_5, 12},
};

std::map<SignalStrengthLevel, int> lteRsrpLevelMap {
   {SignalStrengthLevel::LEVEL_1, -140},
   {SignalStrengthLevel::LEVEL_2, -100},
   {SignalStrengthLevel::LEVEL_3, -90},
   {SignalStrengthLevel::LEVEL_4, -80},
   {SignalStrengthLevel::LEVEL_5, -70},
};

std::map<SignalStrengthLevel, int> lteRssnrLevelMap {
   {SignalStrengthLevel::LEVEL_1, -200},
   {SignalStrengthLevel::LEVEL_2, -30},
   {SignalStrengthLevel::LEVEL_3, 10},
   {SignalStrengthLevel::LEVEL_4, 45},
   {SignalStrengthLevel::LEVEL_5, 130},
};

std::map<SignalStrengthLevel, int> lteLevelMap {
   {SignalStrengthLevel::LEVEL_1, 0},
   {SignalStrengthLevel::LEVEL_2, 5},
   {SignalStrengthLevel::LEVEL_3, 7},
   {SignalStrengthLevel::LEVEL_4, 9},
   {SignalStrengthLevel::LEVEL_5, 12},
};

std::map<SignalStrengthLevel, int> cdmaDbmMap {
   {SignalStrengthLevel::LEVEL_1, -110},
   {SignalStrengthLevel::LEVEL_2, -100},
   {SignalStrengthLevel::LEVEL_3, -95},
   {SignalStrengthLevel::LEVEL_4, -85},
   {SignalStrengthLevel::LEVEL_5, -75},
};

std::map<SignalStrengthLevel, int> cdmaEcioMap {
   {SignalStrengthLevel::LEVEL_1, -160},
   {SignalStrengthLevel::LEVEL_2, -150},
   {SignalStrengthLevel::LEVEL_3, -130},
   {SignalStrengthLevel::LEVEL_4, -110},
   {SignalStrengthLevel::LEVEL_5, -90},
};

std::map<SignalStrengthLevel, int> evdoDbmMap {
   {SignalStrengthLevel::LEVEL_1, -115},
   {SignalStrengthLevel::LEVEL_2, -105},
   {SignalStrengthLevel::LEVEL_3, -90},
   {SignalStrengthLevel::LEVEL_4, -75},
   {SignalStrengthLevel::LEVEL_5, -65},
};

std::map<SignalStrengthLevel, int> evdoSnrMap {
   {SignalStrengthLevel::LEVEL_1, 0},
   {SignalStrengthLevel::LEVEL_2, 1},
   {SignalStrengthLevel::LEVEL_3, 3},
   {SignalStrengthLevel::LEVEL_4, 5},
   {SignalStrengthLevel::LEVEL_5, 7},
};

std::map<SignalStrengthLevel, int> wcdmaLevelMap {
   {SignalStrengthLevel::LEVEL_1, 0},
   {SignalStrengthLevel::LEVEL_2, 3},
   {SignalStrengthLevel::LEVEL_3, 5},
   {SignalStrengthLevel::LEVEL_4, 8},
   {SignalStrengthLevel::LEVEL_5, 12},
};

std::map<SignalStrengthLevel, int> nr5gRsrpLevelMap {
   {SignalStrengthLevel::LEVEL_1, -140},
   {SignalStrengthLevel::LEVEL_2, -110},
   {SignalStrengthLevel::LEVEL_3, -90},
   {SignalStrengthLevel::LEVEL_4, -80},
   {SignalStrengthLevel::LEVEL_5, -65},
};

std::map<SignalStrengthLevel, int> nr5gRssnrLevelMap {
   {SignalStrengthLevel::LEVEL_1, -230},
   {SignalStrengthLevel::LEVEL_2, -50},
   {SignalStrengthLevel::LEVEL_3, 50},
   {SignalStrengthLevel::LEVEL_4, 150},
   {SignalStrengthLevel::LEVEL_5, 300},
};

std::map<SignalStrengthLevel, int> nb1NtnRsrpLevelMap {
   {SignalStrengthLevel::LEVEL_1, -140},
   {SignalStrengthLevel::LEVEL_2, -100},
   {SignalStrengthLevel::LEVEL_3, -90},
   {SignalStrengthLevel::LEVEL_4, -80},
   {SignalStrengthLevel::LEVEL_5, -70},
};

std::map<SignalStrengthLevel, int> nb1NtnRssnrLevelMap {
   {SignalStrengthLevel::LEVEL_1, -200},
   {SignalStrengthLevel::LEVEL_2, -30},
   {SignalStrengthLevel::LEVEL_3, 10},
   {SignalStrengthLevel::LEVEL_4, 45},
   {SignalStrengthLevel::LEVEL_5, 130},
};

std::map<SignalStrengthLevel, int> nb1NtnLevelMap {
   {SignalStrengthLevel::LEVEL_1, 0},
   {SignalStrengthLevel::LEVEL_2, 5},
   {SignalStrengthLevel::LEVEL_3, 7},
   {SignalStrengthLevel::LEVEL_4, 9},
   {SignalStrengthLevel::LEVEL_5, 12},
};

// clang-format on

inline SignalStrengthLevel calculateLevel(int val, std::map<SignalStrengthLevel, int> levelMap) {
   SignalStrengthLevel level = SignalStrengthLevel::LEVEL_UNKNOWN;
   for(auto &it : levelMap) {
      if(val >= it.second) {
         level = it.first;
      } else {
         break;
      }
   }
   return level;
};

inline int inRange(int value, int min, int max) {
   if (value < min || value > max) {
      return INVALID_SIGNAL_STRENGTH_VALUE;
   }
   return value;
}

SignalStrength::SignalStrength(std::shared_ptr<LteSignalStrengthInfo> lteSignalStrengthInfo,
                               std::shared_ptr<GsmSignalStrengthInfo> gsmSignalStrengthInfo,
                               std::shared_ptr<CdmaSignalStrengthInfo> cdmaSignalStrengthInfo,
                               std::shared_ptr<WcdmaSignalStrengthInfo> wcdmaSignalStrengthInfo,
                               std::shared_ptr<TdscdmaSignalStrengthInfo> tdscdmaSignalStrengthInfo,
                               std::shared_ptr<Nr5gSignalStrengthInfo> nr5gSignalStrengthInfo,
                               std::shared_ptr<Nb1NtnSignalStrengthInfo> nb1NtnSignalStrengthInfo)
   : lteSS_(lteSignalStrengthInfo)
   , gsmSS_(gsmSignalStrengthInfo)
   , cdmaSS_(cdmaSignalStrengthInfo)
   , wcdmaSS_(wcdmaSignalStrengthInfo)
   , tdscdmaSS_(tdscdmaSignalStrengthInfo)
   , nr5gSS_(nr5gSignalStrengthInfo)
   , nb1NtnSS_(nb1NtnSignalStrengthInfo) {
   LOG(DEBUG, "Signal Strength Constructor");
}

std::shared_ptr<LteSignalStrengthInfo> SignalStrength::getLteSignalStrength() {
   return lteSS_;
}

std::shared_ptr<GsmSignalStrengthInfo> SignalStrength::getGsmSignalStrength() {
   return gsmSS_;
}

std::shared_ptr<CdmaSignalStrengthInfo> SignalStrength::getCdmaSignalStrength() {
   return cdmaSS_;
}

std::shared_ptr<WcdmaSignalStrengthInfo> SignalStrength::getWcdmaSignalStrength() {
   return wcdmaSS_;
}

std::shared_ptr<TdscdmaSignalStrengthInfo> SignalStrength::getTdscdmaSignalStrength() {
   return tdscdmaSS_;
}

std::shared_ptr<Nr5gSignalStrengthInfo> SignalStrength::getNr5gSignalStrength() {
   return nr5gSS_;
}

std::shared_ptr<Nb1NtnSignalStrengthInfo> SignalStrength::getNb1NtnSignalStrength() {
    return nb1NtnSS_;
}

LteSignalStrengthInfo::LteSignalStrengthInfo(int lteSignalStrength, int lteRsrp, int lteRsrq,
                                             int lteRssnr, int lteCqi, int timingAdvance) {

   LOG(DEBUG, __FUNCTION__, " Before range check, Signal Strength: ", lteSignalStrength,
        " RSRP: ", lteRsrp, " RSRQ: ", lteRsrq, " RSSNR: ", lteRssnr, " CQI: ", lteCqi,
        " Timing Advance: ", timingAdvance);
   lteSignalStrength_ = inRange(lteSignalStrength, MIN_LTE_SIGNAL_STRENGTH,
      MAX_LTE_SIGNAL_STRENGTH);
   lteRsrp_ = inRange(lteRsrp, MIN_LTE_RSRP, MAX_LTE_RSRP);
   lteRsrq_ = inRange(lteRsrq, MIN_LTE_RSRQ, MAX_LTE_RSRQ);
   lteRssnr_ = inRange(lteRssnr, MIN_LTE_RSSNR_LEVEL, MAX_LTE_RSSNR_LEVEL);
   lteCqi_ = inRange(lteCqi, MIN_LTE_CQI, MAX_LTE_CQI);
   timingAdvance_ = inRange(timingAdvance, MIN_LTE_TIMING_ADVANCE, MAX_LTE_TIMING_ADVANCE);
   LOG(DEBUG, __FUNCTION__, " After range check, Signal Strength: ", lteSignalStrength_,
        " RSRP: ", lteRsrp_, " RSRQ: ", lteRsrq_, " RSSNR: ", lteRssnr_, " CQI: ", lteCqi_,
        " Timing Advance: ", timingAdvance_);
}

const int LteSignalStrengthInfo::getLteSignalStrength() const {
   return lteSignalStrength_;
}

const int LteSignalStrengthInfo::getLteReferenceSignalReceiveQuality() const {
   return lteRsrq_;
}

const int LteSignalStrengthInfo::getLteReferenceSignalSnr() const {
   return lteRssnr_;
}

const int LteSignalStrengthInfo::getLteChannelQualityIndicator() const {
   return lteCqi_;
}

const int LteSignalStrengthInfo::getTimingAdvance() const {
   return timingAdvance_;
}

const int LteSignalStrengthInfo::getDbm() const {
   return lteRsrp_;
}

const SignalStrengthLevel LteSignalStrengthInfo::getLevel() const {

   SignalStrengthLevel rsrpLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
   if((lteRsrp_ >= MIN_LTE_RSRP) && (lteRsrp_ <= MAX_LTE_RSRP)) {
      rsrpLevel = calculateLevel(lteRsrp_, lteRsrpLevelMap);
   }

   SignalStrengthLevel rsnrLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
   if((lteRssnr_ >= MIN_LTE_RSSNR_LEVEL) && (lteRssnr_ <= MAX_LTE_RSSNR_LEVEL)) {
      rsnrLevel = calculateLevel(lteRssnr_, lteRssnrLevelMap);
   }

   // Valid values are (0-63, 99) as defined in TS 36.331
   SignalStrengthLevel sigStrengthLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
   if((lteSignalStrength_ >= MIN_LTE_SIGNAL_STRENGTH)
      && (lteSignalStrength_ <= MAX_LTE_SIGNAL_STRENGTH)) {
      sigStrengthLevel = calculateLevel(lteRssnr_, lteRssnrLevelMap);
   }

   // Give preference to rsrpLevel
   if(rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN
      && rsnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN) {
      return (rsrpLevel > rsnrLevel ? rsrpLevel : rsnrLevel);
   }

   if(rsnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
      return rsnrLevel;

   if(rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
      return rsrpLevel;

   // if we don't have valid rsrp and rsnr values use signal strength level
   return sigStrengthLevel;
}

GsmSignalStrengthInfo::GsmSignalStrengthInfo(int gsmSignalStrength, int gsmBitErrorRate,
                                             int timingAdvance) {

   LOG(DEBUG, __FUNCTION__, " Before range check, Signal Strength: ", gsmSignalStrength,
        " Error Rate: ", gsmBitErrorRate, " Timing Advance: ", timingAdvance);
   gsmSignalStrength_ = inRange(gsmSignalStrength, MIN_GSM_LEVEL, MAX_GSM_LEVEL);
   gsmBitErrorRate_ = inRange(gsmBitErrorRate, GSM_MIN_BIT_ERROR_RATE, GSM_MAX_BIT_ERROR_RATE);
   timingAdvance_ = inRange(timingAdvance, GSM_MIN_TIMING_ADVANCE, GSM_MAX_TIMING_ADVANCE);
   LOG(DEBUG, __FUNCTION__, " After range check, Signal Strength: ", gsmSignalStrength_,
        " Error Rate: ", gsmBitErrorRate_, " Timing Advance: ", timingAdvance_);
}

const int GsmSignalStrengthInfo::getGsmSignalStrength() const {
   return gsmSignalStrength_;
}

const int GsmSignalStrengthInfo::getGsmBitErrorRate() const {
   return gsmBitErrorRate_;
}

const int GsmSignalStrengthInfo::getTimingAdvance() {
   return timingAdvance_;
}

const int GsmSignalStrengthInfo::getDbm() const {
   int dBm = inRange(gsmSignalStrength_, MIN_GSM_LEVEL, MAX_GSM_LEVEL);
   if(dBm != INVALID_SIGNAL_STRENGTH_VALUE) {
      dBm = GSM_DBM_CONVERSION_FACTOR + (GSM_DBM_MULTIPLICATION_FACTOR * gsmSignalStrength_);
   }
   LOG(DEBUG, __FUNCTION__, " dBm = ", dBm);
   return dBm;
}

const SignalStrengthLevel GsmSignalStrengthInfo::getLevel() const {
   // Valid values are 0-31, 99 as defined in TS 27.007 8.5
   if(gsmSignalStrength_ >= MIN_GSM_LEVEL && gsmSignalStrength_ <= MAX_GSM_LEVEL) {
      return calculateLevel(gsmSignalStrength_, gsmLevelMap);
   }
   return SignalStrengthLevel::LEVEL_UNKNOWN;
}

CdmaSignalStrengthInfo::CdmaSignalStrengthInfo(int cdmaDbm, int cdmaEcio, int evdoDbm,
                                               int evdoEcio, int evdoSignalNoiseRatio) {

   LOG(DEBUG, __FUNCTION__);
   cdmaDbm_ = inRange(cdmaDbm, MIN_CDMA_DBM, MAX_CDMA_DBM);
   cdmaEcio_ = inRange(cdmaEcio, MIN_CDMA_ECIO, MAX_CDMA_ECIO);
   evdoDbm_ = inRange(evdoDbm, MIN_EVDO_DBM, MAX_EVDO_DBM);
   evdoEcio_ = inRange(evdoEcio, MIN_EVDO_ECIO, MAX_EVDO_ECIO);
   evdoSignalNoiseRatio_ = inRange(evdoSignalNoiseRatio, MIN_EVDO_SNR, MAX_EVDO_SNR);

}

const int CdmaSignalStrengthInfo::getCdmaEcio() const {
   return cdmaEcio_;
}

const int CdmaSignalStrengthInfo::getEvdoEcio() const {
   return evdoEcio_;
}

const int CdmaSignalStrengthInfo::getEvdoSignalNoiseRatio() const {
   return evdoSignalNoiseRatio_;
}

const int CdmaSignalStrengthInfo::getDbm() const {
   int dbm;
   int cdmaDbm = (cdmaDbm_ > MAX_CDMA_DBM) ? cdmaDbm_ : MIN_CDMA_DBM;
   int evdoDbm = (evdoDbm_ > MAX_EVDO_DBM) ? evdoDbm_ : MIN_EVDO_DBM;
   dbm = cdmaDbm < evdoDbm ? cdmaDbm : evdoDbm;
   LOG(DEBUG, __FUNCTION__, "Cdma/Evdo Dbm =", dbm);
   return dbm;
}

const int CdmaSignalStrengthInfo::getCdmaDbm() const {
   return cdmaDbm_;
}

const int CdmaSignalStrengthInfo::getEvdoDbm() const {
   return evdoDbm_;
}

const SignalStrengthLevel CdmaSignalStrengthInfo::getLevel() const {
   SignalStrengthLevel cdmaLevel = getCdmaLevel();
   SignalStrengthLevel evdoLevel = getEvdoLevel();

   // if evdo level is unknown return cdmaLevel
   if(evdoLevel == SignalStrengthLevel::LEVEL_UNKNOWN) {
      return cdmaLevel;
   }

   // if cdma level is unknown return evdoLevel
   if(cdmaLevel == SignalStrengthLevel::LEVEL_UNKNOWN) {
      return evdoLevel;
   }

   return (cdmaLevel < evdoLevel) ? cdmaLevel : evdoLevel;
}

const SignalStrengthLevel CdmaSignalStrengthInfo::getCdmaLevel() const {
   SignalStrengthLevel cdmaDbmLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
   if(cdmaDbm_ >= MIN_CDMA_DBM && cdmaDbm_ <= MAX_CDMA_DBM) {
      cdmaDbmLevel = calculateLevel(cdmaDbm_, cdmaDbmMap);
   }

   SignalStrengthLevel cdmaEcioLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
   if(cdmaEcio_ >= MIN_CDMA_ECIO && cdmaEcio_ <= MAX_CDMA_ECIO) {
      cdmaEcioLevel = calculateLevel(cdmaEcio_, cdmaEcioMap);
   }

   return (cdmaDbmLevel < cdmaEcioLevel) ? cdmaDbmLevel : cdmaEcioLevel;
}

const SignalStrengthLevel CdmaSignalStrengthInfo::getEvdoLevel() const {
   SignalStrengthLevel evdoDbmLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
   if(evdoDbm_ >= MIN_EVDO_DBM && evdoDbm_ <= MAX_EVDO_DBM) {
      evdoDbmLevel = calculateLevel(evdoDbm_, evdoDbmMap);
   }

   SignalStrengthLevel evdoSnrLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
   if((evdoSignalNoiseRatio_ > MIN_EVDO_SNR) && (evdoSignalNoiseRatio_ <= MAX_EVDO_SNR)) {
      evdoSnrLevel = calculateLevel(evdoSignalNoiseRatio_, evdoSnrMap);
   }

   return (evdoDbmLevel < evdoSnrLevel) ? evdoDbmLevel : evdoSnrLevel;
}

WcdmaSignalStrengthInfo::WcdmaSignalStrengthInfo(int signalStrength, int bitErrorRate) {
   LOG(DEBUG, __FUNCTION__, " Before range check, Signal Strength: ", signalStrength,
       " Error Rate: ", bitErrorRate);
   signalStrength_ = inRange(signalStrength, MIN_WCDMA_LEVEL, MAX_WCDMA_LEVEL);
   bitErrorRate_ = inRange(bitErrorRate, MIN_WCDMA_BIT_ERROR_RATE, MAX_WCDMA_BIT_ERROR_RATE);
   LOG(DEBUG, __FUNCTION__, " Before range check, Signal Strength: ", signalStrength,
        " Error Rate: ", bitErrorRate);
}

WcdmaSignalStrengthInfo::WcdmaSignalStrengthInfo(int signalStrength, int bitErrorRate,
   int ecio, int rscp) {
    LOG(DEBUG, __FUNCTION__, " Before range check, Signal Strength: ", signalStrength,
        " Error Rate: ", bitErrorRate, " ECIO: ", ecio, " RSCP: ", rscp);
   signalStrength_ = inRange(signalStrength, MIN_WCDMA_LEVEL, MAX_WCDMA_LEVEL);
   bitErrorRate_ = inRange(bitErrorRate, MIN_WCDMA_BIT_ERROR_RATE, MAX_WCDMA_BIT_ERROR_RATE);
   ecio_ = inRange(ecio, MIN_WCDMA_ECIO, MAX_WCDMA_ECIO);
   rscp_ = inRange(rscp, MIN_WCDMA_RSCP, MAX_WCDMA_RSCP);
   LOG(DEBUG, __FUNCTION__, " After range check, Signal Strength: ", signalStrength_,
        " Error Rate: ", bitErrorRate_, " ECIO: ", ecio_, " RSCP: ", rscp_);
}

const SignalStrengthLevel WcdmaSignalStrengthInfo::getLevel() const {
   // Valid values are (0-31, 99) as defined in TS 27.007 8.5
   if(signalStrength_ >= MIN_WCDMA_LEVEL && signalStrength_ <= MAX_WCDMA_LEVEL) {
      return calculateLevel(signalStrength_, wcdmaLevelMap);
   }
   return SignalStrengthLevel::LEVEL_UNKNOWN;
}

const int WcdmaSignalStrengthInfo::getDbm() const {

   int dBm = inRange(signalStrength_, MIN_WCDMA_LEVEL, MAX_WCDMA_LEVEL);
   if(dBm != INVALID_SIGNAL_STRENGTH_VALUE) {
      dBm = WCDMA_DBM_CONVERSION_FACTOR + (WCDMA_DBM_MULTIPLICATION_FACTOR * signalStrength_);
   }
   LOG(DEBUG, __FUNCTION__, " dBm = ", dBm);
   return dBm;
}

const int WcdmaSignalStrengthInfo::getSignalStrength() const {
   // Valid values are (0-31, 99) as defined in TS 27.007 8.5
   LOG(DEBUG, __FUNCTION__);
   return signalStrength_;
}

const int WcdmaSignalStrengthInfo::getBitErrorRate() const {
   // bit error rate (0-7, 99) as defined in TS 27.007 8.5
   LOG(DEBUG, __FUNCTION__);
   return bitErrorRate_;
}

const int WcdmaSignalStrengthInfo::getEcio() const {
   LOG(DEBUG, __FUNCTION__, " ECIO: ", ecio_);
   return ecio_;
}

const int WcdmaSignalStrengthInfo::getRscp() const {
   LOG(DEBUG, __FUNCTION__, " RSCP: ", rscp_);
   return rscp_;
}

TdscdmaSignalStrengthInfo::TdscdmaSignalStrengthInfo(int rscp) {
   LOG(DEBUG, __FUNCTION__);
   rscp_ = inRange(rscp, MIN_TDSCDMA_RSCP, MAX_TDSCDMA_RSCP);
}

const int TdscdmaSignalStrengthInfo::getRscp() const {
   return rscp_;
}

Nr5gSignalStrengthInfo::Nr5gSignalStrengthInfo(int rsrp, int rsrq, int rssnr) {
   LOG(DEBUG, __FUNCTION__, " Before range check, RSRP: ", rsrp, " RSRQ: ", rsrq,
        " RSSNR: ", rssnr);
   rsrp_ = inRange(rsrp, MIN_NR5G_RSRP, MAX_NR5G_RSRP);
   rsrq_ = inRange(rsrq, MIN_NR5G_RSRQ, MAX_NR5G_RSRQ);
   rssnr_ = inRange(rssnr, MIN_NR5G_RSSNR_LEVEL, MAX_NR5G_RSSNR_LEVEL);
   LOG(DEBUG, __FUNCTION__, " After range check, RSRP: ", rsrp_, " RSRQ: ", rsrq_,
        " RSSNR: ", rssnr_);
}

const int Nr5gSignalStrengthInfo::getDbm() const {
   return rsrp_;
}

const int Nr5gSignalStrengthInfo::getReferenceSignalReceiveQuality() const {
   return rsrq_;
}

const int Nr5gSignalStrengthInfo::getReferenceSignalSnr() const {
   return rssnr_;
}

const SignalStrengthLevel Nr5gSignalStrengthInfo::getLevel() const {

   SignalStrengthLevel rsrpLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
   if((rsrp_ >= MIN_NR5G_RSRP) && (rsrp_ <= MAX_NR5G_RSRP)) {
      rsrpLevel = calculateLevel(rsrp_, nr5gRsrpLevelMap);
   }

   SignalStrengthLevel rssnrLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
   if((rssnr_ >= MIN_NR5G_RSSNR_LEVEL) && (rssnr_ <= MAX_NR5G_RSSNR_LEVEL)) {
      rssnrLevel = calculateLevel(rssnr_, nr5gRssnrLevelMap);
   }

   // Give preference to rsrpLevel
   if(rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN
      && rssnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN) {
      return (rsrpLevel > rssnrLevel ? rsrpLevel : rssnrLevel);
   }

   if(rssnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
      return rssnrLevel;

   if(rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
      return rsrpLevel;

   return SignalStrengthLevel::LEVEL_UNKNOWN;
}

Nb1NtnSignalStrengthInfo::Nb1NtnSignalStrengthInfo(
    int nb1NtnSignalStrength, int nb1NtnRsrp, int nb1NtnRsrq, int nb1NtnRssnr) {

    LOG(DEBUG, __FUNCTION__, " Before range check, Signal Strength: ", nb1NtnSignalStrength,
        " RSRP: ", nb1NtnRsrp, " RSRQ: ", nb1NtnRsrq, " RSSNR: ", nb1NtnRssnr);
    signalStrength_
        = inRange(nb1NtnSignalStrength, MIN_NB1_NTN_SIGNAL_STRENGTH, MAX_NB1_NTN_SIGNAL_STRENGTH);
    rsrp_  = inRange(nb1NtnRsrp, MIN_NB1_NTN_RSRP, MAX_NB1_NTN_RSRP);
    rsrq_  = inRange(nb1NtnRsrq, MIN_NB1_NTN_RSRQ, MAX_NB1_NTN_RSRQ);
    rssnr_ = inRange(nb1NtnRssnr, MIN_NB1_NTN_RSSNR_LEVEL, MAX_NB1_NTN_RSSNR_LEVEL);
    LOG(DEBUG, __FUNCTION__, " After range check, Signal Strength: ", signalStrength_,
        " RSRP: ", rsrp_, " RSRQ: ", rsrq_, " RSSNR: ", rssnr_);
}

const int Nb1NtnSignalStrengthInfo::getSignalStrength() const {
    return signalStrength_;
}

const int Nb1NtnSignalStrengthInfo::getRsrq() const {
    return rsrq_;
}

const int Nb1NtnSignalStrengthInfo::getRssnr() const {
    return rssnr_;
}

const int Nb1NtnSignalStrengthInfo::getDbm() const {
    return rsrp_;
}

const SignalStrengthLevel Nb1NtnSignalStrengthInfo::getLevel() const {

    SignalStrengthLevel rsrpLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if ((rsrp_ >= MIN_NB1_NTN_RSRP) && (rsrp_ <= MAX_NB1_NTN_RSRP)) {
        rsrpLevel = calculateLevel(rsrp_, nb1NtnRsrpLevelMap);
    }

    SignalStrengthLevel rsnrLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if ((rssnr_ >= MIN_NB1_NTN_RSSNR_LEVEL) && (rssnr_ <= MAX_NB1_NTN_RSSNR_LEVEL)) {
        rsnrLevel = calculateLevel(rssnr_, nb1NtnRssnrLevelMap);
    }

    // Valid values are (0-63, 99) as defined in TS 36.331
    SignalStrengthLevel sigStrengthLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if ((signalStrength_ >= MIN_NB1_NTN_SIGNAL_STRENGTH)
        && (signalStrength_ <= MAX_NB1_NTN_SIGNAL_STRENGTH)) {
        sigStrengthLevel = calculateLevel(rssnr_, nb1NtnRssnrLevelMap);
    }

    // Give preference to rsrpLevel
    if (rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN
        && rsnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN) {
        return (rsrpLevel > rsnrLevel ? rsrpLevel : rsnrLevel);
    }

    if (rsnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
        return rsnrLevel;

    if (rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
        return rsrpLevel;

    // if we don't have valid rsrp and rssnr values use signal strength level
    return sigStrengthLevel;
}

}  // end of namespace tel

}  // end of namespace telux
