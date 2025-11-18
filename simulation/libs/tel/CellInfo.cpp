/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * CellInfo  implementation
 */
#include "telux/tel/CellInfo.hpp"
#include "common/Logger.hpp"

#define INVALID_VALUE -1
namespace telux {
namespace tel {

bool CellInfo::isRegistered() {
   if(registered_ != 0) {
      LOG(DEBUG, " Current cell is registered: ", registered_);
      return true;
   } else {
      LOG(DEBUG, " Cell is not registered");
      return false;
   }
}

CellType CellInfo::getType() {
   return type_;
}

/**
 * GSM CellInfo  implementation
 */
GsmCellInfo::GsmCellInfo(int registered, GsmCellIdentity id, GsmSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
   registered_ = registered;
   type_ = CellType::GSM;
}

GsmCellIdentity GsmCellInfo::getCellIdentity() {
   return id_;
}

GsmSignalStrengthInfo GsmCellInfo::getSignalStrengthInfo() {
   return ssInfo_;
}

GsmCellIdentity::GsmCellIdentity(std::string mcc, std::string mnc, int lac, int cid, int arfcn, int bsic)
   : mcc_(mcc)
   , mnc_(mnc)
   , lac_(lac)
   , cid_(cid)
   , arfcn_(arfcn)
   , bsic_(bsic) {
}

// GSM cell info
const int GsmCellIdentity::getMcc() {
   if (mcc_.empty()) {
      return INVALID_VALUE;
   }
   return stoi(mcc_);
}

const int GsmCellIdentity::getMnc() {
   if (mnc_.empty()) {
      return INVALID_VALUE;
   }
   return stoi(mnc_);
}

const std::string GsmCellIdentity::getMobileCountryCode() {
   return mcc_;
}

const std::string GsmCellIdentity::getMobileNetworkCode() {
   return mnc_;
}

const int GsmCellIdentity::getLac() {
   return lac_;
}

const int GsmCellIdentity::getIdentity() {
   return cid_;
}

const int GsmCellIdentity::getArfcn() {
   return arfcn_;
}

const int GsmCellIdentity::getBaseStationIdentityCode() {
   return bsic_;
}

/**
 * CDMA CellInfo  implementation
 */
CdmaCellInfo::CdmaCellInfo(int registered, CdmaCellIdentity id, CdmaSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
   registered_ = registered;
   type_ = CellType::CDMA;
}

CdmaCellIdentity CdmaCellInfo::getCellIdentity() {
   return id_;
}

CdmaSignalStrengthInfo CdmaCellInfo::getSignalStrengthInfo() {
   return ssInfo_;
}

CdmaCellIdentity::CdmaCellIdentity(int nid, int sid, int stationId, int longitude,
                                   int latitude)
   : nid_(nid)
   , sid_(sid)
   , stationId_(stationId)
   , longitude_(longitude)
   , latitude_(latitude) {
}
// CDMA cell info
const int CdmaCellIdentity::getNid() {
   return nid_;
}

const int CdmaCellIdentity::getSid() {
   return sid_;
}

const int CdmaCellIdentity::getBaseStationId() {
   return stationId_;
}

const int CdmaCellIdentity::getLongitude() {
   return longitude_;
}

const int CdmaCellIdentity::getLatitude() {
   return latitude_;
}

/**
 * LTE CellInfo  implementation
 */
LteCellInfo::LteCellInfo(int registered, LteCellIdentity id, LteSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
   registered_ = registered;
   type_ = CellType::LTE;
}

LteCellIdentity LteCellInfo::getCellIdentity() {
   return id_;
}

LteSignalStrengthInfo LteCellInfo::getSignalStrengthInfo() {
   return ssInfo_;
}

LteCellIdentity::LteCellIdentity(std::string mcc, std::string mnc, int ci, int pci, int tac, int earfcn)
   : mcc_(mcc)
   , mnc_(mnc)
   , ci_(ci)
   , pci_(pci)
   , tac_(tac)
   , earfcn_(earfcn) {
}
// LTE cell info
const int LteCellIdentity::getMcc() {
   if (mcc_.empty()) {
      return INVALID_VALUE;
   }
   return stoi(mcc_);
}

const int LteCellIdentity::getMnc() {
   if (mnc_.empty()) {
      return INVALID_VALUE;
   }
   return stoi(mnc_);
}

const std::string LteCellIdentity::getMobileCountryCode() {
   return mcc_;
}

const std::string LteCellIdentity::getMobileNetworkCode() {
   return mnc_;
}

const int LteCellIdentity::getIdentity() {
   return ci_;
}

const int LteCellIdentity::getPhysicalCellId() {
   return pci_;
}

const int LteCellIdentity::getTrackingAreaCode() {
   return tac_;
}

const int LteCellIdentity::getEarfcn() {
   return earfcn_;
}

/**
 * WCDMA CellInfo  implementation
 */

WcdmaCellInfo::WcdmaCellInfo(int registered, WcdmaCellIdentity id, WcdmaSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
   registered_ = registered;
   type_ = CellType::WCDMA;
}

WcdmaCellIdentity WcdmaCellInfo::getCellIdentity() {
   return id_;
}

WcdmaSignalStrengthInfo WcdmaCellInfo::getSignalStrengthInfo() {
   return ssInfo_;
}

WcdmaCellIdentity::WcdmaCellIdentity(std::string mcc, std::string mnc, int lac, int cid, int psc, int uarfcn)
   : mcc_(mcc)
   , mnc_(mnc)
   , lac_(lac)
   , cid_(cid)
   , psc_(psc)
   , uarfcn_(uarfcn) {
}

// WCDMA cell info
const int WcdmaCellIdentity::getMcc() {
   if (mcc_.empty()) {
      return INVALID_VALUE;
   }
   return stoi(mcc_);
}

const int WcdmaCellIdentity::getMnc() {
   if (mnc_.empty()) {
      return INVALID_VALUE;
   }
   return stoi(mnc_);
}

const std::string WcdmaCellIdentity::getMobileCountryCode() {
   return mcc_;
}

const std::string WcdmaCellIdentity::getMobileNetworkCode() {
   return mnc_;
}

const int WcdmaCellIdentity::getLac() {
   return lac_;
}

const int WcdmaCellIdentity::getIdentity() {
   return cid_;
}

const int WcdmaCellIdentity::getPrimaryScramblingCode() {
   return psc_;
}

const int WcdmaCellIdentity::getUarfcn() {
   return uarfcn_;
}

/**
 * TDSCDMA CellInfo  implementation
 */
TdscdmaCellInfo::TdscdmaCellInfo(int registered, TdscdmaCellIdentity id,
   TdscdmaSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
   registered_ = registered;
   type_ = CellType::TDSCDMA;
}

TdscdmaCellIdentity TdscdmaCellInfo::getCellIdentity() {
   return id_;
}

TdscdmaSignalStrengthInfo TdscdmaCellInfo::getSignalStrengthInfo() {
   return ssInfo_;
}

TdscdmaCellIdentity::TdscdmaCellIdentity(std::string mcc, std::string mnc, int lac, int cid, int cpid)
   : mcc_(mcc)
   , mnc_(mnc)
   , lac_(lac)
   , cid_(cid)
   , cpid_(cpid) {
}

// TDSCDMA cell info
const int TdscdmaCellIdentity::getMcc() {
   if (mcc_.empty()) {
      return INVALID_VALUE;
   }
   return stoi(mcc_);
}

const int TdscdmaCellIdentity::getMnc() {
   if (mnc_.empty()) {
      return INVALID_VALUE;
   }
   return stoi(mnc_);
}

const std::string TdscdmaCellIdentity::getMobileCountryCode() {
   return mcc_;
}

const std::string TdscdmaCellIdentity::getMobileNetworkCode() {
   return mnc_;
}

const int TdscdmaCellIdentity::getLac() {
   return lac_;
}

const int TdscdmaCellIdentity::getIdentity() {
   return cid_;
}

const int TdscdmaCellIdentity::getParametersId() {
   return cpid_;
}

/**
 * NR5G CellInfo  implementation
 */
Nr5gCellInfo::Nr5gCellInfo(int registered, Nr5gCellIdentity id, Nr5gSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
   registered_ = registered;
   type_ = CellType::NR5G;
}

Nr5gCellIdentity Nr5gCellInfo::getCellIdentity() {
   return id_;
}

Nr5gSignalStrengthInfo Nr5gCellInfo::getSignalStrengthInfo() {
   return ssInfo_;
}

Nr5gCellIdentity::Nr5gCellIdentity(std::string mcc, std::string mnc, uint64_t ci, uint32_t pci,
   int32_t tac, int32_t arfcn)
   : mcc_(mcc)
   , mnc_(mnc)
   , ci_(ci)
   , pci_(pci)
   , tac_(tac)
   , arfcn_(arfcn) {
}
// NR5G cell info
const std::string Nr5gCellIdentity::getMobileCountryCode() {
   return mcc_;
}

const std::string Nr5gCellIdentity::getMobileNetworkCode() {
   return mnc_;
}

const uint64_t Nr5gCellIdentity::getIdentity() {
   return ci_;
}

const uint32_t Nr5gCellIdentity::getPhysicalCellId() {
   return pci_;
}

const int32_t Nr5gCellIdentity::getTrackingAreaCode() {
   return tac_;
}

const int32_t Nr5gCellIdentity::getArfcn() {
   return arfcn_;
}

/**
 * NB1 NTN CellInfo implementation
 */
Nb1NtnCellInfo::Nb1NtnCellInfo(
    int registered, Nb1NtnCellIdentity id, Nb1NtnSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
    registered_ = registered;
    type_       = CellType::NB1_NTN;
}

Nb1NtnCellIdentity Nb1NtnCellInfo::getCellIdentity() {
    return id_;
}

Nb1NtnSignalStrengthInfo Nb1NtnCellInfo::getSignalStrengthInfo() {
    return ssInfo_;
}

Nb1NtnCellIdentity::Nb1NtnCellIdentity(
    std::string mcc, std::string mnc, int ci, int tac, int earfcn)
   : mcc_(mcc)
   , mnc_(mnc)
   , ci_(ci)
   , tac_(tac)
   , earfcn_(earfcn) {
}

// NB1 NTN cell info
const std::string Nb1NtnCellIdentity::getMobileCountryCode() {
    return mcc_;
}

const std::string Nb1NtnCellIdentity::getMobileNetworkCode() {
    return mnc_;
}

const int Nb1NtnCellIdentity::getIdentity() {
    return ci_;
}

const int Nb1NtnCellIdentity::getTrackingAreaCode() {
    return tac_;
}

const int Nb1NtnCellIdentity::getEarfcn() {
    return earfcn_;
}

}  // end of namespace tel

}  // end namespace telux
