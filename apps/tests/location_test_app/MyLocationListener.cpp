/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <bitset>
#include <iostream>
#include <sstream>
#include <memory>
#include <iomanip>
#include <cstdint>
#include <chrono>

#include <telux/loc/LocationDefines.hpp>
#include "LocationUtils.hpp"
#include "MyLocationListener.hpp"

using namespace telux::loc;

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define DETAILED_RECORDING \
    std::cout << "###" \
              << (std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000 \
              << ","

void MyLocationListener::printSbasCorrectionEx(
   std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
   telux::loc::SbasCorrection correction = locationInfo->getSbasCorrection();
   if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_IONO]) {
      std::cout << "SBAS ionospheric correction is used" << std::endl;
   }

   if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_FAST]) {
      std::cout << "SBAS fast correction is used" << std::endl;
   }

   if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_LONG]) {
      std::cout << "SBAS long correction is used" << std::endl;
   }

   if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_INTEGRITY]) {
      std::cout << "SBAS integrity information is used" << std::endl;
   }
   if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_DGNSS]) {
      std::cout << "SBAS DGNSS correction information is used" << std::endl;
   }
   if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_RTK]) {
      std::cout << "SBAS RTK correction information is used" << std::endl;
   }
   if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_PPP]) {
      std::cout << "SBAS PPP correction information is used" << std::endl;
   }
   if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_RTK_FIXED]) {
      std::cout << "SBAS RTK fixed correction information is used" << std::endl;
   }
   if(correction[(telux::loc::SbasCorrectionType)
       telux::loc::SBAS_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_]) {
      std::cout << "SBAS PPP correction information is used" << std::endl;
   }
}

void MyLocationListener::printNavigationSolutionEx(
   std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
   telux::loc::NavigationSolution solution = locationInfo->getNavigationSolution();
   if(solution[(telux::loc::NavigationSolutionType)telux::loc::NAV_SBAS_SOLUTION_IONO]) {
      std::cout << "SBAS ionospheric correction is used" << std::endl;
   }

   if(solution[(telux::loc::NavigationSolutionType)telux::loc::NAV_SBAS_SOLUTION_FAST]) {
      std::cout << "SBAS fast correction is used" << std::endl;
   }

   if(solution[(telux::loc::NavigationSolutionType)telux::loc::NAV_SBAS_SOLUTION_LONG]) {
      std::cout << "SBAS long correction is used" << std::endl;
   }

   if(solution[(telux::loc::NavigationSolutionType)telux::loc::NAV_SBAS_INTEGRITY]) {
      std::cout << "SBAS integrity information is used" << std::endl;
   }
   if(solution[(telux::loc::NavigationSolutionType)telux::loc::NAV_DGNSS_SOLUTION]) {
      std::cout << "DGNSS information is used" << std::endl;
   }
   if(solution[(telux::loc::NavigationSolutionType)telux::loc::NAV_RTK_SOLUTION]) {
      std::cout << "RTK information is used" << std::endl;
   }
   if(solution[(telux::loc::NavigationSolutionType)telux::loc::NAV_PPP_SOLUTION]) {
      std::cout << "PPP information is used" << std::endl;
   }
   if(solution[(telux::loc::NavigationSolutionType)telux::loc::NAV_RTK_FIXED_SOLUTION]) {
      std::cout << "RTK fixed information is used" << std::endl;
   }
   if(solution[(telux::loc::NavigationSolutionType)
      telux::loc::NAV_ONLY_SBAS_CORRECTED_SV_USED]) {
      std::cout << "Only SBAS corrected SV information is used" << std::endl;
   }
}

void MyLocationListener::printLocationExValidity(
      telux::loc::LocationInfoExValidity validityMask) {
    std::cout << "Location Ex Validity :" << std::endl;
    if((validityMask & telux::loc::HAS_ALTITUDE_MEAN_SEA_LEVEL)) {
      std::cout << "valid altitude mean sea level" << std::endl;
    }
    if((validityMask & telux::loc::HAS_DOP)) {
      std::cout << "valid pdop, hdop, vdop" << std::endl;
    }
    if((validityMask & telux::loc::HAS_MAGNETIC_DEVIATION)) {
      std::cout << "valid magnetic deviation" << std::endl;
    }
    if((validityMask & telux::loc::HAS_HOR_RELIABILITY)) {
      std::cout << "valid horizontal reliability" << std::endl;
    }
    if((validityMask & telux::loc::HAS_VER_RELIABILITY)) {
      std::cout << "valid vertical reliability" << std::endl;
    }
    if((validityMask & telux::loc::HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR)) {
      std::cout << "valid elipsode semi major" << std::endl;
    }
    if((validityMask & telux::loc::HAS_HOR_ACCURACY_ELIP_SEMI_MINOR)) {
      std::cout << "valid elipsode semi minor" << std::endl;
    }
    if((validityMask & telux::loc::HAS_HOR_ACCURACY_ELIP_AZIMUTH)) {
      std::cout << "valid accuracy elipsode azimuth" << std::endl;
    }
    if((validityMask & telux::loc::HAS_GNSS_SV_USED_DATA)) {
      std::cout << "valid gnss sv used in pos data" << std::endl;
    }
    if((validityMask & telux::loc::HAS_NAV_SOLUTION_MASK)) {
      std::cout << "valid navSolutionMask" << std::endl;
    }
    if((validityMask & telux::loc::HAS_POS_TECH_MASK)) {
      std::cout << "valid LocPosTechMask" << std::endl;
    }
    if((validityMask & telux::loc::HAS_SV_SOURCE_INFO)) {
      std::cout << "valid LocSvInfoSource" << std::endl;
    }
    if((validityMask & telux::loc::HAS_POS_DYNAMICS_DATA)) {
      std::cout << "valid position dynamics data" << std::endl;
    }
    if((validityMask & telux::loc::HAS_EXT_DOP)) {
      std::cout << "valid gdop, tdop" << std::endl;
    }
    if((validityMask & telux::loc::HAS_NORTH_STD_DEV)) {
      std::cout << "valid North standard deviation" << std::endl;
    }
    if((validityMask & telux::loc::HAS_EAST_STD_DEV)) {
      std::cout << "valid East standard deviation" << std::endl;
    }
    if((validityMask & telux::loc::HAS_NORTH_VEL)) {
      std::cout << "valid North Velocity" << std::endl;
    }
    if((validityMask & telux::loc::HAS_EAST_VEL)) {
      std::cout << "valid East Velocity" << std::endl;
    }
    if((validityMask & telux::loc::HAS_UP_VEL)) {
      std::cout << "valid Up Velocity" << std::endl;
    }
    if((validityMask & telux::loc::HAS_NORTH_VEL_UNC)) {
      std::cout << "valid North Velocity Uncertainty" << std::endl;
    }
    if((validityMask & telux::loc::HAS_EAST_VEL_UNC)) {
      std::cout << "valid East Velocity Uncertainty" << std::endl;
    }
    if((validityMask & telux::loc::HAS_UP_VEL_UNC)) {
      std::cout << "valid Up Velocity Uncertainty" << std::endl;
    }
    if((validityMask & telux::loc::HAS_LEAP_SECONDS)) {
      std::cout << "valid leap_seconds" << std::endl;
    }
    if((validityMask & telux::loc::HAS_TIME_UNC)) {
      std::cout << "valid timeUncMs" << std::endl;
    }
    if((validityMask & telux::loc::HAS_NUM_SV_USED_IN_POSITION)) {
      std::cout << "valid number of sv used" << std::endl;
    }
    if((validityMask & telux::loc::HAS_CALIBRATION_CONFIDENCE_PERCENT)) {
      std::cout << "valid sensor calibrationConfidencePercent" << std::endl;
    }
    if((validityMask & telux::loc::HAS_CALIBRATION_STATUS)) {
      std::cout << "valid sensor calibrationConfidence" << std::endl;
    }
    if((validityMask & telux::loc::HAS_OUTPUT_ENG_TYPE)) {
      std::cout << "valid output engine type" << std::endl;
    }
    if((validityMask & telux::loc::HAS_OUTPUT_ENG_MASK)) {
      std::cout << "valid output engine mask" << std::endl;
    }
    if((validityMask & telux::loc::HAS_CONFORMITY_INDEX_FIX)) {
      std::cout << "valid conformity index" << std::endl;
    }
    if((validityMask & telux::loc::HAS_LLA_VRP_BASED)) {
      std::cout << "valid lla vrp based" << std::endl;
    }
    if((validityMask & telux::loc::HAS_ENU_VELOCITY_VRP_BASED)) {
      std::cout << "valid enu velocity vrp based" << std::endl;
    }
    if((validityMask & telux::loc::HAS_SOLUTION_STATUS)) {
      std::cout << "valid DR solution status" << std::endl;
    }
    if((validityMask & telux::loc::HAS_ALTITUDE_TYPE)) {
      std::cout << "valid altitude type" << std::endl;
    }
    if((validityMask & telux::loc::HAS_REPORT_STATUS)) {
      std::cout << "valid report status" << std::endl;
    }
    if((validityMask & telux::loc::HAS_INTEGRITY_RISK_USED)) {
      std::cout << "valid integrity risk" << std::endl;
    }
    if((validityMask & telux::loc::HAS_PROTECT_LEVEL_ALONG_TRACK)) {
      std::cout << "valid protect along track" << std::endl;
    }
    if((validityMask & telux::loc::HAS_PROTECT_LEVEL_CROSS_TRACK)) {
      std::cout << "valid protect cross track" << std::endl;
    }
    if((validityMask & telux::loc::HAS_PROTECT_LEVEL_VERTICAL)) {
      std::cout << "valid protect vertical" << std::endl;
    }
    if((validityMask & telux::loc::HAS_DGNSS_STATION_ID)) {
      std::cout << "valid dgnss station id" << std::endl;
    }
    if((validityMask & telux::loc::HAS_BASE_LINE_LENGTH)) {
      std::cout << "valid base station distance" << std::endl;
    }
    if((validityMask & telux::loc::HAS_AGE_OF_CORRECTION)) {
      std::cout << "valid age of correction" << std::endl;
    }
    if((validityMask & telux::loc::HAS_LEAP_SECONDS_UNC)) {
      std::cout << "valid leap seconds uncertainty" << std::endl;
    }
}

void MyLocationListener::printLocationValidity(telux::loc::LocationInfoValidity validityMask) {
   std::cout << "Location Basic Validity :" << std::endl;
   if((validityMask & telux::loc::HAS_LAT_LONG_BIT)) {
      std::cout << "valid latitude longitude" << std::endl;
   }
   if((validityMask & telux::loc::HAS_ALTITUDE_BIT)) {
      std::cout << "valid altitude" << std::endl;
   }
   if((validityMask & telux::loc::HAS_SPEED_BIT)) {
      std::cout << "valid speed" << std::endl;
   }
   if((validityMask & telux::loc::HAS_HEADING_BIT)) {
      std::cout << "valid heading" << std::endl;
   }
   if((validityMask & telux::loc::HAS_HORIZONTAL_ACCURACY_BIT)) {
      std::cout << "valid horizontal accuracy" << std::endl;
   }
   if((validityMask & telux::loc::HAS_VERTICAL_ACCURACY_BIT)) {
      std::cout << "valid vertical accuracy" << std::endl;
   }
   if((validityMask & telux::loc::HAS_SPEED_ACCURACY_BIT)) {
      std::cout << "valid speed accuracy" << std::endl;
   }
   if((validityMask & telux::loc::HAS_HEADING_ACCURACY_BIT)) {
      std::cout << "valid heading accuracy " << std::endl;
   }
   if((validityMask & telux::loc::HAS_TIMESTAMP_BIT)) {
      std::cout << "valid timestamp" << std::endl;
   }
   if((validityMask & telux::loc::HAS_ELAPSED_REAL_TIME_BIT)) {
      std::cout << "valid elapsed real time" << std::endl;
   }
   if((validityMask & telux::loc::HAS_ELAPSED_REAL_TIME_UNC_BIT)) {
      std::cout << "valid elapsed real time uncertainty" << std::endl;
   }
   if((validityMask & telux::loc::HAS_TIME_UNC_BIT)) {
      std::cout << "valid timeUncMs" << std::endl;
   }
   if((validityMask & telux::loc::HAS_GPTP_TIME_BIT)) {
      std::cout << "valid elapsed gPTP time" << std::endl;
   }
   if((validityMask & telux::loc::HAS_GPTP_TIME_UNC_BIT)) {
      std::cout << "valid elapsed gPTP time uncertainty" << std::endl;
   }
}

void MyLocationListener::printLocationTech(telux::loc::LocationTechnology techMask) {
   std::cout << "Position Technology used :" << std::endl;
   if((techMask & telux::loc::LOC_GNSS)) {
      std::cout << "location calculated using GNSS" << std::endl;
   }
   if((techMask & telux::loc::LOC_CELL)) {
      std::cout << "location calculated using CELL" << std::endl;
   }
   if((techMask & telux::loc::LOC_WIFI)) {
      std::cout << "location calculated using WIFI" << std::endl;
   }
   if((techMask & telux::loc::LOC_SENSORS)) {
      std::cout << "location calculated using SENSORS" << std::endl;
   }
   if((techMask & telux::loc::LOC_REFERENCE_LOCATION)) {
      std::cout << "location calculated using Reference location" << std::endl;
   }
   if((techMask & telux::loc::LOC_INJECTED_COARSE_POSITION)) {
      std::cout << "location calculated using Coarse position injected into the location engine"
                << std::endl;
   }
   if((techMask & telux::loc::LOC_AFLT)) {
      std::cout << "location calculated using AFLT" << std::endl;
   }
   if((techMask & telux::loc::LOC_HYBRID)) {
      std::cout << "location calculated using GNSS and network-provided measurements"
                << std::endl;
   }
   if((techMask & telux::loc::LOC_PPE)) {
      std::cout << "location calculated using Precise position engine" << std::endl;
   }
   if((techMask & telux::loc::LOC_VEH)) {
      std::cout << "location calculated using Vehicular data" << std::endl;
   }
   if((techMask & telux::loc::LOC_VIS)) {
      std::cout << "location calculated using Visual data" << std::endl;
   }
   if((techMask & telux::loc::LOC_PROPAGATED)) {
      std::cout << "location calculated using Propagation logic" << std::endl;
   }
}

void MyLocationListener::printGnssSignalType(telux::loc::GnssSignal signalTypeMask) {
   std::cout << "Gnss Signal Type :" << std::endl;
   if (signalTypeMask & telux::loc::GnssSignalType::GPS_L1CA) {
     std::cout << "GPS L1CA signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GPS_L1C) {
     std::cout << "GPS L1C signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GPS_L2) {
     std::cout << "GPS L2 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GPS_L5) {
     std::cout << "GPS L5 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GLONASS_G1) {
     std::cout << "Glonass G1 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GLONASS_G2) {
     std::cout << "Glonass G2 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GALILEO_E1) {
     std::cout << "Galileo E1 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GALILEO_E5A) {
     std::cout << "Galileo E5A signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GALILIEO_E5B) {
     std::cout << "Galileo E5B signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B1) {
     std::cout << "Beidou B1 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2) {
     std::cout << "Beidou B2 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::QZSS_L1CA) {
     std::cout << "QZSS L1CA signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::QZSS_L1S) {
     std::cout << "QZSS L1S signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::QZSS_L2) {
     std::cout << "QZSS L2 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::QZSS_L5) {
     std::cout << "QZSS L5 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::SBAS_L1) {
     std::cout << "SBAS L1 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B1I) {
     std::cout << "Beidou B1I signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B1C) {
     std::cout << "Beidou B1C signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2I) {
     std::cout << "Beidou B2I signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2AI) {
     std::cout << "Beidou B2AI signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::NAVIC_L5) {
     std::cout << "Navic L5 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2AQ) {
     std::cout << "Beidou B2AQ signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2BI) {
     std::cout << "Beidou B2BI signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2BQ) {
     std::cout << "Beidou B2BQ signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::NAVIC_L1) {
     std::cout << "Navic L1 signal is present" << std::endl;
   }
   if (signalTypeMask == telux::loc::UNKNOWN_SIGNAL_MASK) {
     std::cout << " No signal present" << std::endl;
   }
}

void MyLocationListener::printGnssMeasurementInfo(
   std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
   std::vector<telux::loc::GnssMeasurementInfo> measInfo = locationInfo->getmeasUsageInfo();
   std::cout << "GNSS Measurement Info:" << std::endl;
   for(uint16_t i = 0; i < measInfo.size(); i++) {
      telux::loc::GnssSignal signalType = measInfo[i].gnssSignalType;
      printGnssSignalType(signalType);

      telux::loc::GnssSystem system = measInfo[i].gnssConstellation;
      if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS) {
         std::cout << "GPS satellite" << std::endl;
      }
      else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GALILEO) {
         std::cout << "GALILEO satellite" << std::endl;
      }
      else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_SBAS) {
         std::cout << "SBAS satellite" << std::endl;
      }
      else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GLONASS) {
         std::cout << "GLONASS satellite" << std::endl;
      }
      else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_BDS) {
         std::cout << "BDS satellite" << std::endl;
      }
      else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_QZSS) {
         std::cout << "QZSS satellite" << std::endl;
      }
      else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_NAVIC) {
         std::cout << "NAVIC satellite" << std::endl;
      }
      else {
         std::cout << "UNKNOWN satellite" << std::endl;
      }

      std::cout << "Gnss sv id : " << measInfo[i].gnssSvId << std::endl;
   }
}

void MyLocationListener::printSvUsedInPosition(
      telux::loc::SvUsedInPosition svUsedInPosition) {
   std::cout << "SV used in position :" << std::endl;
   std::cout << "SVs from GPS constellation " << svUsedInPosition.gps << std::endl;
   std::cout << "SVs from GLONASS constellation " << svUsedInPosition.glo
     << std::endl;
   std::cout << "SVs from GALILEO constellation " << svUsedInPosition.gal
     << std::endl;
   std::cout << "SVs from BEIDOU constellation " << svUsedInPosition.bds
     << std::endl;
   std::cout << "SVs from QZSS constellation " << svUsedInPosition.qzss << std::endl;
   std::cout << "SVs from NAVIC constellation " << svUsedInPosition.navic << std::endl;
}

void MyLocationListener::printGnssSystemTime(
   std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
   telux::loc::SystemTime sysTime = locationInfo->getGnssSystemTime();
   std::cout << " GNSS System Time : " << std::endl;

   telux::loc::GnssSystem system = sysTime.gnssSystemTimeSrc;
   telux::loc::SystemTimeInfo sysTimeInfo = sysTime.time;
   if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS) {
      std::cout << "GPS satellite is valid" << std::endl;
      telux::loc::TimeInfo timeInfo = sysTimeInfo.gps;
      std::cout << "Validity mask: " << timeInfo.validityMask;
      std::cout << " System time week: " << timeInfo.systemWeek;
      std::cout << " System time week ms: " << timeInfo.systemMsec;
      std::cout << " System clk time: " << timeInfo.systemClkTimeBias;
      std::cout << " System clk time uncertainty valid: " << timeInfo.systemClkTimeUncMs;
      std::cout << " System reference valid: " << timeInfo.refFCount;
      std::cout << " System num clock reset valid: " << timeInfo.numClockResets << std::endl;
   } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GALILEO) {
      std::cout << "GALILEO satellite is valid" << std::endl;
      telux::loc::TimeInfo timeInfo = sysTimeInfo.gal;
      std::cout << "Validity mask: " << timeInfo.validityMask;
      std::cout << " System time week: " << timeInfo.systemWeek;
      std::cout << " System time week ms: " << timeInfo.systemMsec;
      std::cout << " System clk time: " << timeInfo.systemClkTimeBias;
      std::cout << " System clk time uncertainty valid: " << timeInfo.systemClkTimeUncMs;
      std::cout << " System reference valid: " << timeInfo.refFCount;
      std::cout << " System num clock reset valid: " << timeInfo.numClockResets << std::endl;
   } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_SBAS) {
      std::cout << "SBAS satellite is valid" << std::endl;
   } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GLONASS) {
      std::cout << "GLONASS satellite is valid" << std::endl;
      telux::loc::GlonassTimeInfo info = sysTimeInfo.glo;
      std::cout << "Validity mask: " << info.validityMask;
      std::cout << " GLONASS day number: " << info.gloDays;
      std::cout << " GLONASS time of day: " << info.gloMsec;
      std::cout << " GLONASS clock time bias: " << info.gloClkTimeBias;
      std::cout << " Single sided maximum time bias uncertainty: " << info.gloClkTimeUncMs;
      std::cout << " FCount (free running HW timer) value: " << info.refFCount;
      std::cout << " Number of clock resets/discontinuities detected: " << info.numClockResets;
      std::cout << " GLONASS four year number: " << info.gloFourYear << std::endl;
   } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_BDS) {
      std::cout << "BDS satellite is valid" << std::endl;
      telux::loc::TimeInfo timeInfo = sysTimeInfo.bds;
      std::cout << "Validity mask: " << timeInfo.validityMask;
      std::cout << " System time week: " << timeInfo.systemWeek;
      std::cout << " System time week ms: " << timeInfo.systemMsec;
      std::cout << " System clk time: " << timeInfo.systemClkTimeBias;
      std::cout << " System clk time uncertainty valid: " << timeInfo.systemClkTimeUncMs;
      std::cout << " System reference valid: " << timeInfo.refFCount;
      std::cout << " System num clock reset valid: " << timeInfo.numClockResets << std::endl;
   } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_QZSS) {
      std::cout << "QZSS satellite is valid" << std::endl;
      telux::loc::TimeInfo timeInfo = sysTimeInfo.qzss;
      std::cout << "Validity mask: " << timeInfo.validityMask;
      std::cout << " System time week: " << timeInfo.systemWeek;
      std::cout << " System time week ms: " << timeInfo.systemMsec;
      std::cout << " System clk time: " << timeInfo.systemClkTimeBias;
      std::cout << " System clk time uncertainty valid: " << timeInfo.systemClkTimeUncMs;
      std::cout << " System reference valid: " << timeInfo.refFCount;
      std::cout << " System num clock reset valid: " << timeInfo.numClockResets << std::endl;
   } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_NAVIC) {
      std::cout << "NAVIC satellite is valid" << std::endl;
      telux::loc::TimeInfo timeInfo = sysTimeInfo.navic;
      std::cout << "Validity mask: " << timeInfo.validityMask;
      std::cout << " System time week: " << timeInfo.systemWeek;
      std::cout << " System time week ms: " << timeInfo.systemMsec;
      std::cout << " System clk time: " << timeInfo.systemClkTimeBias;
      std::cout << " System clk time uncertainty valid: " << timeInfo.systemClkTimeUncMs;
      std::cout << " System reference valid: " << timeInfo.refFCount;
      std::cout << " System num clock reset valid: " << timeInfo.numClockResets << std::endl;
   } else {
      std::cout << "UNKNOWN satellite" << std::endl;
   }
}

void MyLocationListener::printLocationPositionDynamics(
   std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
   telux::loc::GnssKinematicsData posDynamics_ = locationInfo->getBodyFrameData();
   std::cout << "Location Position Dynamics: " << std::endl;
   telux::loc::KinematicDataValidity kinematicDataValidity = posDynamics_.bodyFrameDataMask;
   if((kinematicDataValidity & telux::loc::HAS_LONG_ACCEL)) {
      std::cout << "Navigation data has Forward Acceleration" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_LAT_ACCEL)) {
      std::cout << "Navigation data has Sideward Acceleration" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_VERT_ACCEL)) {
      std::cout << "Navigation data has Vertical Acceleration" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_YAW_RATE)) {
      std::cout << "Navigation data has Heading Rate" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_PITCH)) {
      std::cout << "Navigation data has Body pitch" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_LONG_ACCEL_UNC)) {
      std::cout << "Navigation data has Forward Acceleration Uncertainty" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_LAT_ACCEL_UNC)) {
      std::cout << "Navigation data has Sideward Acceleration Uncertainty" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_VERT_ACCEL_UNC)) {
      std::cout << "Navigation data has Vertical Acceleration Uncertainty" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_YAW_RATE_UNC)) {
      std::cout << "Navigation data has Heading Rate Uncertainty" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_PITCH_UNC)) {
      std::cout << "Navigation data has Body pitch Uncertainty" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_PITCH_RATE_BIT)) {
      std::cout << "Navigation data has pitch rate" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_PITCH_RATE_UNC_BIT)) {
      std::cout << "Navigation data has pitch rate uncertainty" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_ROLL_BIT)) {
      std::cout << "Navigation data has roll" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_ROLL_UNC_BIT)) {
      std::cout << "Navigation data has roll Uncertainty" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_ROLL_RATE_BIT)) {
      std::cout << "Navigation data has roll rate" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_ROLL_RATE_UNC_BIT)) {
      std::cout << "Navigation data has roll rate Uncertainty" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_YAW_BIT)) {
      std::cout << "Navigation data has yaw" << std::endl;
   }
   if((kinematicDataValidity & telux::loc::HAS_YAW_UNC_BIT)) {
      std::cout << "Navigation data has yaw Uncertainty" << std::endl;
   }
   std::cout << "Forward Acceleration in body frame (m/s2): " << posDynamics_.longAccel;
   std::cout << " Sideward Acceleration in body frame (m/s2): " << posDynamics_.latAccel;
   std::cout << " Vertical Acceleration in body frame (m/s2): " << posDynamics_.vertAccel
             << std::endl;
   std::cout << "Heading Rate (Radians/second): " << posDynamics_.yawRate;
   std::cout << " Body pitch (Radians): " << posDynamics_.pitch;
   std::cout << " Uncertainty of Forward Acceleration in body frame: " << posDynamics_.longAccelUnc
             << std::endl;
   std::cout << "Uncertainty of Side-ward Acceleration in body frame: " << posDynamics_.latAccelUnc;
   std::cout << " Uncertainty of Vertical Acceleration in body frame: " <<
             posDynamics_.vertAccelUnc;
   std::cout << " Uncertainty of Heading Rate: " << posDynamics_.yawRateUnc;
   std::cout << " Uncertainty of Body pitch: " << posDynamics_.pitchUnc;
   std::cout << " Body pitch rate: " << posDynamics_.pitchRate;
   std::cout << " Uncertainty of pitch rate: " << posDynamics_.pitchRateUnc;
   std::cout << " Roll of body frame, clockwise is positive: " << posDynamics_.roll;
   std::cout << " Uncertainty of roll, 68% confidence level: " << posDynamics_.rollUnc;
   std::cout << " Roll rate of body frame, clockwise is positive: " << posDynamics_.rollRate;
   std::cout << " Uncertainty of roll rate, 68% confidence level: " << posDynamics_.rollRateUnc;
   std::cout << " Yaw of body frame, clockwise is positive: " << posDynamics_.yaw;
   std::cout << " Uncertainty of yaw, 68% confidence level: " << posDynamics_.yawUnc << std::endl;
}

void MyLocationListener::printLocationPositionTech(
   std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
   telux::loc::GnssPositionTech gnssPositionTech = locationInfo->getPositionTechnology();
   std::cout << "Location position technology used : " << std::endl;
   if((gnssPositionTech & telux::loc::GNSS_SATELLITE)) {
      std::cout << "SATELLITE" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_CELLID)) {
      std::cout << "CELL" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_WIFI)) {
      std::cout << "WIFI" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_SENSORS)) {
      std::cout << "SENSORS" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_REFERENCE_LOCATION)) {
      std::cout << "REFERENCE LOCATION" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_INJECTED_COARSE_POSITION)) {
      std::cout << "INJECTED COARSE POSITION" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_AFLT)) {
      std::cout << "AFLT" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_HYBRID)) {
      std::cout << "HYBRID" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_PPE)) {
      std::cout << "PPE" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_VEHICLE)) {
      std::cout << "VEHICLE" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_VISUAL)) {
      std::cout << "VISUAL" << std::endl;
   }
   if((gnssPositionTech & telux::loc::GNSS_PROPAGATED)) {
      std::cout << "PROPAGATED" << std::endl;
   }
   if((gnssPositionTech == telux::loc::GNSS_DEFAULT)) {
      std::cout << "DEFAULT" << std::endl;
   }
}

void MyLocationListener::printHorizontalReliability(telux::loc::LocationReliability locReliability) {
   switch(locReliability) {
      case telux::loc::LocationReliability::NOT_SET:
         std::cout << "Horizontal reliability: NOT_SET" << std::endl;
         break;
      case telux::loc::LocationReliability::VERY_LOW:
         std::cout << "Horizontal reliability: VERY_LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::LOW:
         std::cout << "Horizontal reliability: LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::MEDIUM:
         std::cout << "Horizontal reliability: MEDIUM" << std::endl;
         break;
      case telux::loc::LocationReliability::HIGH:
         std::cout << "Horizontal reliability: HIGH" << std::endl;
         break;
      default:
         std::cout << "Horizontal reliability: UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printVerticalReliability(telux::loc::LocationReliability locReliability) {
   switch(locReliability) {
      case telux::loc::LocationReliability::NOT_SET:
         std::cout << "Vertical reliability: NOT_SET" << std::endl;
         break;
      case telux::loc::LocationReliability::VERY_LOW:
         std::cout << "Vertical reliability: VERY_LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::LOW:
         std::cout << "Vertical reliability: LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::MEDIUM:
         std::cout << "Vertical reliability: MEDIUM" << std::endl;
         break;
      case telux::loc::LocationReliability::HIGH:
         std::cout << "Vertical reliability: HIGH" << std::endl;
         break;
      default:
         std::cout << "Vertical reliability: UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printConstellationType(telux::loc::GnssConstellationType constellation) {
   switch(constellation) {
      case telux::loc::GnssConstellationType::GPS:
         std::cout << "Constellation type: GPS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::GALILEO:
         std::cout << "Constellation type: GALILEO" << std::endl;
         break;
      case telux::loc::GnssConstellationType::SBAS:
         std::cout << "Constellation type: SBAS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::GLONASS:
         std::cout << "Constellation type: GLONASS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::BDS:
         std::cout << "Constellation type: BDS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::QZSS:
         std::cout << "Constellation type: QZSS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::NAVIC:
         std::cout << "Constellation type: NAVIC" << std::endl;
         break;
      default:
         std::cout << "Constellation type: UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printEphimerisAvailability(telux::loc::SVInfoAvailability availability) {
   switch(availability) {
      case telux::loc::SVInfoAvailability::YES:
         std::cout << "Ephemeris availability: YES" << std::endl;
         break;
      case telux::loc::SVInfoAvailability::NO:
         std::cout << "Ephemeris availability: NO" << std::endl;
         break;
      default:
         std::cout << "Ephemeris availability: UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printAlmanacAvailability(telux::loc::SVInfoAvailability availability) {
   switch(availability) {
      case telux::loc::SVInfoAvailability::YES:
         std::cout << "Almanac availability: YES" << std::endl;
         break;
      case telux::loc::SVInfoAvailability::NO:
         std::cout << "Almanac availability: NO" << std::endl;
         break;
      default:
         std::cout << "Almanac availability: UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printFixAvailability(telux::loc::SVInfoAvailability availability) {
   switch(availability) {
      case telux::loc::SVInfoAvailability::YES:
         std::cout << "Fix availability: YES" << std::endl;
         break;
      case telux::loc::SVInfoAvailability::NO:
         std::cout << "Fix availability: NO" << std::endl;
         break;
      default:
         std::cout << "Fix availability: UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printCalibrationStatus(
    std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
   telux::loc::DrCalibrationStatus calibrationStatus = locationInfo->getCalibrationStatus();
   std::cout << "Calibration status : " << std::endl;
   if((calibrationStatus & telux::loc::DR_ROLL_CALIBRATION_NEEDED)) {
      std::cout << "Roll calibration is needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_PITCH_CALIBRATION_NEEDED)) {
      std::cout << "Pitch calibration is needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_YAW_CALIBRATION_NEEDED)) {
      std::cout << "Yaw calibration is needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_ODO_CALIBRATION_NEEDED)) {
      std::cout << "Odo calibration is needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_GYRO_CALIBRATION_NEEDED)) {
      std::cout << "Gyro calibration is needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_TURN_CALIBRATION_LOW)) {
      std::cout << "Lot more turns on level ground needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_TURN_CALIBRATION_MEDIUM)) {
      std::cout << "Some more turns on level ground needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_TURN_CALIBRATION_HIGH)) {
      std::cout << "Sufficient turns on level ground observed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_LINEAR_ACCEL_CALIBRATION_LOW)) {
      std::cout << "Lot more accelerations in straight line needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_LINEAR_ACCEL_CALIBRATION_MEDIUM)) {
      std::cout << "Some more accelerations in straight line needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_LINEAR_ACCEL_CALIBRATION_HIGH)) {
      std::cout << "Sufficient acceleration events in straight line observed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_LINEAR_MOTION_CALIBRATION_LOW)) {
      std::cout << "Lot more motion in straight line needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_LINEAR_MOTION_CALIBRATION_MEDIUM)) {
      std::cout << "Some more motion in straight line needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_LINEAR_MOTION_CALIBRATION_HIGH)) {
      std::cout << "Sufficient motion events in straight line observed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_STATIC_CALIBRATION_LOW)) {
      std::cout << "Lot more stationary events on level ground needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_STATIC_CALIBRATION_MEDIUM)) {
      std::cout << "Some more stationary events on level ground needed" << std::endl;
   }
   if((calibrationStatus & telux::loc::DR_STATIC_CALIBRATION_HIGH)) {
      std::cout << "Sufficient stationary events on level ground observed" << std::endl;
   }
}

void MyLocationListener::printSolutionStatus(
   std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
   telux::loc::DrSolutionStatus solutionStatus = locationInfo->getSolutionStatus();
   std::cout << "Solution status : " << std::endl;
   if((solutionStatus & telux::loc::VEHICLE_SENSOR_SPEED_INPUT_DETECTED)) {
      std::cout << "Vehicle sensor speed input was detected by the DR position engine."<< std::endl;
   }
   if((solutionStatus & telux::loc::VEHICLE_SENSOR_SPEED_INPUT_USED)) {
      std::cout << "Vehicle sensor speed input was used by the DR position engine. "<< std::endl;
   }
   if((solutionStatus & telux::loc::WARNING_UNCALIBRATED)) {
      std::cout << "DRE solution disengaged due to insufficient calibration. "<< std::endl;
   }
   if((solutionStatus & telux::loc::WARNING_GNSS_QUALITY_INSUFFICIENT)) {
      std::cout << "DRE solution disengaged due to bad GNSS quality. "<< std::endl;
   }
   if((solutionStatus & telux::loc::WARNING_FERRY_DETECTED)) {
      std::cout << "DRE solution disengaged as ferry condition detected. "<< std::endl;
   }
   if((solutionStatus & telux::loc::ERROR_6DOF_SENSOR_UNAVAILABLE)) {
      std::cout << "DRE solution disengaged as 6DOF sensor inputs not available. "<< std::endl;
   }
   if((solutionStatus & telux::loc::ERROR_VEHICLE_SPEED_UNAVAILABLE)) {
      std::cout << "DRE solution disengaged as vehicle speed inputs not available. "<< std::endl;
   }
   if((solutionStatus & telux::loc::ERROR_GNSS_EPH_UNAVAILABLE)) {
      std::cout << "DRE solution disengaged as Ephemeris info not available. "<< std::endl;
   }
   if((solutionStatus & telux::loc::ERROR_GNSS_MEAS_UNAVAILABLE)) {
      std::cout << "DRE solution disengaged as GNSS measurement info not available. "<< std::endl;
   }
   if((solutionStatus & telux::loc::WARNING_INIT_POSITION_INVALID)) {
      std::cout << "DRE solution disengaged due non-availability of stored position from"
         "previous session. "<< std::endl;
   }
   if((solutionStatus & telux::loc::WARNING_INIT_POSITION_UNRELIABLE)) {
      std::cout << "DRE solution dis-engaged due to vehicle motion detected at session"
         " start. "<< std::endl;
   }
   if((solutionStatus & telux::loc::WARNING_POSITON_UNRELIABLE)) {
      std::cout << "DRE solution dis-engaged due to unreliable position. "<< std::endl;
   }
   if((solutionStatus & telux::loc::ERROR_GENERIC)) {
      std::cout << "DRE solution dis-engaged due to a generic error. "<< std::endl;
   }
   if((solutionStatus & telux::loc::WARNING_SENSOR_TEMP_OUT_OF_RANGE)) {
      std::cout << "DRE solution dis-engaged due to Sensor Temperature "
         "being out of range. "<< std::endl;
   }
   if((solutionStatus & telux::loc::WARNING_USER_DYNAMICS_INSUFFICIENT)) {
      std::cout << "DRE solution dis-engaged due to insufficient user dynamics. "<< std::endl;
   }
   if((solutionStatus & telux::loc::WARNING_FACTORY_DATA_INCONSISTENT)) {
      std::cout << "DRE solution dis-engaged due to inconsistent factory data. "<< std::endl;
   }
}

void MyLocationListener::printLocOutputEngineType(
    std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
  telux::loc::LocationAggregationType locEngineType = locationInfo->getLocOutputEngType();
  if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_FUSED) {
    std::cout << " This is FUSED engine reports" << std::endl;
  }
  if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_SPE) {
    std::cout << " This is SPE engine reports" << std::endl;
  }
  if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_PPE) {
    std::cout << " This is PPE engine reports" << std::endl;
  }
  if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_VPE) {
    std::cout << " This is VPE engine reports" << std::endl;
  }
}

void MyLocationListener::printLocOutputEngineMask(
    std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo) {
  telux::loc::PositioningEngine posEngineBits = locationInfo->getLocOutputEngMask();
  if(posEngineBits & telux::loc::STANDARD_POSITIONING_ENGINE) {
    std::cout << " SPE used in the reports" << std::endl;
  }
  if(posEngineBits & telux::loc::DEAD_RECKONING_ENGINE) {
    std::cout << " DRE used in the reports" << std::endl;
  }
  if(posEngineBits & telux::loc::PRECISE_POSITIONING_ENGINE) {
    std::cout << " PPE used in the reports" << std::endl;
  }
  if(posEngineBits & telux::loc::VP_POSITIONING_ENGINE) {
    std::cout << " VPE used in the reports" << std::endl;
  }
}

void MyLocationListener::printMeasurementsClockValidity(
    telux::loc::GnssMeasurementsClockValidity flags) {
  if(flags & telux::loc::LEAP_SECOND_BIT) {
    std::cout << " Valid leap seconds" << std::endl;
  }
  if(flags & telux::loc::TIME_BIT) {
    std::cout << " Valid time" << std::endl;
  }
  if(flags & telux::loc::TIME_UNCERTAINTY_BIT) {
    std::cout << " Valid time uncertainty" << std::endl;
  }
  if(flags & telux::loc::FULL_BIAS_BIT) {
    std::cout << " Valid full bias" << std::endl;
  }
  if(flags & telux::loc::BIAS_BIT) {
    std::cout << " Valid bias" << std::endl;
  }
  if(flags & telux::loc::BIAS_UNCERTAINTY_BIT) {
    std::cout << " Valid bias uncertainty" << std::endl;
  }
  if(flags & telux::loc::DRIFT_BIT) {
    std::cout << " Valid drift" << std::endl;
  }
  if(flags & telux::loc::DRIFT_UNCERTAINTY_BIT) {
    std::cout << " Valid drift uncertainty" << std::endl;
  }
  if(flags & telux::loc::HW_CLOCK_DISCONTINUITY_COUNT_BIT) {
    std::cout << " Valid hw clock discontinuity count" << std::endl;
  }
  if(flags & telux::loc::ELAPSED_REAL_TIME_BIT) {
    std::cout << " Valid elapsed real time" << std::endl;
  }
  if(flags & telux::loc::ELAPSED_REAL_TIME_UNC_BIT) {
    std::cout << " Valid elapsed real time uncertainity" << std::endl;
  }
  if(flags & telux::loc::ELAPSED_GPTP_TIME_BIT) {
    std::cout << " Valid elapsed gPTP time" << std::endl;
  }
  if(flags & telux::loc::ELAPSED_GPTP_TIME_UNC_BIT) {
    std::cout << " Valid elapsed gPTP time uncertainity" << std::endl;
  }
}

void MyLocationListener::printMeasurementsDataValidity(
    telux::loc::GnssMeasurementsDataValidity flags) {
  if(flags & telux::loc::SV_ID_BIT) {
    std::cout << " valid sv id" << std::endl;
  }
  if(flags & telux::loc::SV_TYPE_BIT) {
    std::cout << " valid svType" << std::endl;
  }
  if(flags & telux::loc::STATE_BIT) {
    std::cout << " valid stateMask" << std::endl;
  }
  if(flags & telux::loc::RECEIVED_SV_TIME_BIT) {
    std::cout << " valid receivedSvTimeNs" << std::endl;
  }
  if(flags & telux::loc::RECEIVED_SV_TIME_UNCERTAINTY_BIT) {
    std::cout << " valid receivedSvTimeUncertaintyNs" << std::endl;
  }
  if(flags & telux::loc::CARRIER_TO_NOISE_BIT) {
    std::cout << " valid carrierToNoiseDbHz" << std::endl;
  }
  if(flags & telux::loc::PSEUDORANGE_RATE_BIT) {
    std::cout << " valid pseudorangeRateMps" << std::endl;
  }
  if(flags & telux::loc::PSEUDORANGE_RATE_UNCERTAINTY_BIT) {
    std::cout << " valid pseudorangeRateUncertaintyMps" << std::endl;
  }
  if(flags & telux::loc::ADR_STATE_BIT) {
    std::cout << " valid adrStateMask" << std::endl;
  }
  if(flags & telux::loc::ADR_BIT) {
    std::cout << " valid adrMeters" << std::endl;
  }
  if(flags & telux::loc::ADR_UNCERTAINTY_BIT) {
    std::cout << " valid adrUncertaintyMeters" << std::endl;
  }
  if(flags & telux::loc::CARRIER_FREQUENCY_BIT) {
    std::cout << " valid carrierFrequencyHz" << std::endl;
  }
  if(flags & telux::loc::CARRIER_CYCLES_BIT) {
    std::cout << " valid carrierCycles" << std::endl;
  }
  if(flags & telux::loc::CARRIER_PHASE_BIT) {
    std::cout << " valid carrierPhase" << std::endl;
  }
  if(flags & telux::loc::CARRIER_PHASE_UNCERTAINTY_BIT) {
    std::cout << " valid carrierPhaseUncertainty" << std::endl;
  }
  if(flags & telux::loc::MULTIPATH_INDICATOR_BIT) {
    std::cout << " valid multipathIndicator" << std::endl;
  }
  if(flags & telux::loc::SIGNAL_TO_NOISE_RATIO_BIT) {
    std::cout << " valid signalToNoiseRatioDb" << std::endl;
  }
  if(flags & telux::loc::AUTOMATIC_GAIN_CONTROL_BIT) {
    std::cout << " valid agcLevelDb" << std::endl;
  }
  if(flags & telux::loc::GNSS_SIGNAL_TYPE) {
    std::cout << " valid signal type" << std::endl;
  }
  if(flags & telux::loc::BASEBAND_CARRIER_TO_NOISE) {
    std::cout << " valid basebandCarrierToNoise" << std::endl;
  }
  if(flags & telux::loc::FULL_ISB) {
    std::cout << " valid fullInterSignalBias" << std::endl;
  }
  if(flags & telux::loc::FULL_ISB_UNCERTAINTY) {
    std::cout << " valid fullInterSignalBiasUncertainty" << std::endl;
  }
}

void MyLocationListener::printMeasurementState(telux::loc::GnssMeasurementsStateValidity mask) {
  if(mask & telux::loc::UNKNOWN_BIT) {
    std::cout << " State is unknown" << std::endl;
  }
  if(mask & telux::loc::CODE_LOCK_BIT) {
    std::cout << " State is code lock" << std::endl;
  }
  if(mask & telux::loc::BIT_SYNC_BIT) {
    std::cout << " State is bit sync" << std::endl;
  }
  if(mask & telux::loc::SUBFRAME_SYNC_BIT) {
    std::cout << " State is subframe sync" << std::endl;
  }
  if(mask & telux::loc::TOW_DECODED_BIT) {
    std::cout << " State is tow decoded" << std::endl;
  }
  if(mask & telux::loc::MSEC_AMBIGUOUS_BIT) {
    std::cout << " State is msec ambiguous" << std::endl;
  }
  if(mask & telux::loc::SYMBOL_SYNC_BIT) {
    std::cout << " State is symbol sync" << std::endl;
  }
  if(mask & telux::loc::GLO_STRING_SYNC_BIT) {
    std::cout << " State is GLONASS string sync" << std::endl;
  }
  if(mask & telux::loc::GLO_TOD_DECODED_BIT) {
    std::cout << " State is GLONASS TOD decoded" << std::endl;
  }
  if(mask & telux::loc::BDS_D2_BIT_SYNC_BIT) {
    std::cout << " State is BDS D2 bit sync" << std::endl;
  }
  if(mask & telux::loc::BDS_D2_SUBFRAME_SYNC_BIT) {
    std::cout << " State is BDS D2 subframe sync" << std::endl;
  }
  if(mask & telux::loc::GAL_E1BC_CODE_LOCK_BIT) {
    std::cout << " State is Galileo E1BC code lock" << std::endl;
  }
  if(mask & telux::loc::GAL_E1C_2ND_CODE_LOCK_BIT) {
    std::cout << " State is Galileo E1C second code lock" << std::endl;
  }
  if(mask & telux::loc::GAL_E1B_PAGE_SYNC_BIT) {
    std::cout << " State is Galileo E1B page sync" << std::endl;
  }
  if(mask & telux::loc::SBAS_SYNC_BIT) {
    std::cout << " State is SBAS sync" << std::endl;
  }
}

void MyLocationListener::printMeasurementAdrState(
    telux::loc::GnssMeasurementsAdrStateValidity mask) {
  if(mask & telux::loc::UNKNOWN_STATE) {
    std::cout << " State is unknown" << std::endl;
  }
  if(mask & telux::loc::VALID_BIT) {
    std::cout << " State is valid" << std::endl;
  }
  if(mask & telux::loc::RESET_BIT) {
    std::cout << " State is reset" << std::endl;
  }
  if(mask & telux::loc::CYCLE_SLIP_BIT) {
    std::cout << " State is cycle slip" << std::endl;
  }
}

void MyLocationListener::printMeasurementsMultipathIndicator(
    telux::loc::GnssMeasurementsMultipathIndicator indicator) {
  if(indicator == telux::loc::UNKNOWN_INDICATOR) {
    std::cout << " Multipath indicator is unknown" << std::endl;
  }
  if(indicator == telux::loc::PRESENT) {
    std::cout << " Multipath indicator is present" << std::endl;
  }
  if(indicator == telux::loc::NOT_PRESENT) {
    std::cout << " Multipath indicator is not present" << std::endl;
  }
}

void MyLocationListener::printLLAVRPBasedInfo(telux::loc::LLAInfo llaInfo) {
  std::cout << "LLAVRPBased Information :" << std::endl;
  std::cout << " Latitude : " << llaInfo.latitude << std::endl;
  std::cout << " Longitude : " << llaInfo.longitude << std::endl;
  std::cout << " Altitude : " << llaInfo.altitude << std::endl;
}

void MyLocationListener::printENUVelocityVRPBased(std::vector<float> enuVelocityVRPBased) {
  std::cout << "East, North, Up Velocity VRP based :" << std::endl;
  std::cout << " East velocity : " << enuVelocityVRPBased[0] << std::endl;
  std::cout << " North velocity : " << enuVelocityVRPBased[1] << std::endl;
  std::cout << " Up velocity : " << enuVelocityVRPBased[2] << std::endl;
}

void MyLocationListener::printAltitudeType(telux::loc::AltitudeType type) {
  std::cout << "Altitude Type is :" << std::endl;
  if (type == telux::loc::AltitudeType::UNKNOWN) {
    std::cout << "UNKNOWN" << std::endl;
  }
  if (type == telux::loc::AltitudeType::CALCULATED) {
    std::cout << "CALCULATED" << std::endl;
  }
  if (type == telux::loc::AltitudeType::ASSUMED) {
    std::cout << "ASSUMED" << std::endl;
  }
}

void MyLocationListener::printReportStatus(telux::loc::ReportStatus status) {
  std::cout << "Report Status is :" << std::endl;
  if (status == telux::loc::ReportStatus::UNKNOWN) {
    std::cout << "UNKNOWN" << std::endl;
  }
  if (status == telux::loc::ReportStatus::SUCCESS) {
    std::cout << "SUCCESS" << std::endl;
  }
  if (status == telux::loc::ReportStatus::INTERMEDIATE) {
    std::cout << "INTERMEDIATE" << std::endl;
  }
  if (status == telux::loc::ReportStatus::FAILURE) {
    std::cout << "FAILURE" << std::endl;
  }
}

void MyLocationListener::onCapabilitiesInfo(const telux::loc::LocCapability capabilityMask) {
  LocationUtils::displayCapabilities(capabilityMask);
}

void MyLocationListener::printDgnssStationIds(std::vector<uint16_t> dgnssStationIds) {
    if(!dgnssStationIds.empty()) {
        std::cout << "Dgnss Station IDs : ";
        for(auto id: dgnssStationIds) {
            std::cout << id << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "No Dgnss Station Id is present\n";
    }
}

void MyLocationListener::onBasicLocationUpdate(
   const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) {
   if(!isBasicReportFlagEnabled_) {
      return;
   }
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n*********************** Basic Location Report *********************"
                      << std::endl;
   printLocationValidity(locationInfo->getLocationInfoValidity());
   printLocationTech(locationInfo->getTechMask());

   if(locationInfo->getTimeStamp() != telux::loc::UNKNOWN_TIMESTAMP) {
     time_t realtime;
     realtime = (time_t)((locationInfo->getTimeStamp() / 1000));
     std::cout << "Time stamp: " << locationInfo->getTimeStamp() << " mSec" << std::endl;
     std::cout << "GMT Time stamp: " << ctime(&realtime);
   } else {
     std::cout << "Time stamp Not Valid" << std::endl;
   }
   std::cout << "Latitude: " << std::setprecision(15) << locationInfo->getLatitude() << std::endl
             << "Longitude: " << std::setprecision(15) << locationInfo->getLongitude() << std::endl
             << "Altitude: " << std::setprecision(15) << locationInfo->getAltitude() << std::endl
             << "Speed: " << locationInfo->getSpeed() << std::endl
             << "Heading: " << locationInfo->getHeading() << std::endl
             << "Horizontal uncertainty: " << locationInfo->getHorizontalUncertainty() << std::endl
             << "Vertical uncertainty: " << locationInfo->getVerticalUncertainty() << std::endl
             << "Speed uncertainty: " << locationInfo->getSpeedUncertainty() << std::endl
             << "Heading uncertainty: " << locationInfo->getHeadingUncertainty() << std::endl
             << "Elapsed real time: " << locationInfo->getElapsedRealTime() << std::endl
             << "Elapsed real time uncertainty: " << locationInfo->getElapsedRealTimeUncertainty()
             << std::endl
             << "Time uncertainty: " << locationInfo->getTimeUncMs() << std::endl
             << "gPTP time: " << locationInfo->getElapsedGptpTime() << std::endl
             << "gPTP time uncertainty: " << locationInfo->getElapsedGptpTimeUnc() << std::endl;

   std::cout << "*************************************************************" << std::endl;
}

void MyLocationListener::recordLocationInfo(
    const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) {
    //Recording Data.
    std::ostringstream recordStream;
    recordStream << LOCATION << ",";

    telux::loc::SystemTime sysTime = locationInfo->getGnssSystemTime();
    telux::loc::SystemTimeInfo sysTimeInfo = sysTime.time;
    telux::loc::GnssSystem system = sysTime.gnssSystemTimeSrc;

    uint8_t leapSeconds = 0;
    locationInfo->getLeapSeconds(leapSeconds);
    std::vector<float> enuVelocityVRPBased = locationInfo->getVRPBasedENUVelocity();
    std::vector<uint16_t> SVIds;
    locationInfo->getSVIds(SVIds);
    std::vector<float> velocityEastNorthUp;
    std::vector<float> velocityUncertaintyEastNorthUp;
    locationInfo->getVelocityUncertaintyEastNorthUp(velocityUncertaintyEastNorthUp);
    locationInfo->getVelocityEastNorthUp(velocityEastNorthUp);
    recordStream << locationInfo->getTimeStamp() << "," <<
    static_cast<int>(locationInfo->getLocOutputEngType()) << "," <<
    locationInfo->getTechMask() << "," <<
    locationInfo->getLatitude() << "," << locationInfo->getLongitude() << "," <<
    locationInfo->getAltitude() << "," << locationInfo->getHeading() << "," <<
    locationInfo->getSpeed() << "," << locationInfo->getHeadingUncertainty() << "," <<
    locationInfo->getSpeedUncertainty() << "," << locationInfo->getHorizontalUncertainty() <<
    "," << locationInfo->getVerticalUncertainty() << "," <<
    locationInfo->getLocationInfoValidity() << "," << locationInfo->getElapsedRealTime() << ","
    << locationInfo->getElapsedRealTimeUncertainty() << "," <<
    locationInfo->getLocationInfoExValidity() << "," <<
    locationInfo->getAltitudeMeanSeaLevel() << "," <<
    locationInfo->getPositionDop() << "," <<
    locationInfo->getHorizontalDop() << "," <<
    locationInfo->getVerticalDop() << "," <<
    locationInfo->getGeometricDop() << "," <<
    locationInfo->getTimeDop() << "," <<
    locationInfo->getMagneticDeviation() << "," <<
    static_cast<int>(locationInfo->getHorizontalReliability()) << "," <<
    static_cast<int>(locationInfo->getVerticalReliability()) << "," <<
    locationInfo->getHorizontalUncertaintySemiMajor() << "," <<
    locationInfo->getHorizontalUncertaintySemiMinor() << "," <<
    locationInfo->getHorizontalUncertaintyAzimuth() << "," <<
    locationInfo->getEastStandardDeviation() << "," <<
    locationInfo->getNorthStandardDeviation() << "," <<
    locationInfo->getNumSvUsed() << "," <<
    locationInfo->getSvUsedInPosition().gps << "," <<
    locationInfo->getSvUsedInPosition().glo << "," <<
    locationInfo->getSvUsedInPosition().gal << "," <<
    locationInfo->getSvUsedInPosition().bds << "," <<
    locationInfo->getSvUsedInPosition().qzss << "," <<
    locationInfo->getSvUsedInPosition().navic << "," <<
    locationInfo->getSbasCorrection() << "," <<
    locationInfo->getPositionTechnology() << "," <<
    locationInfo->getBodyFrameData().latAccel << "," <<
    locationInfo->getBodyFrameData().longAccel << "," <<
    locationInfo->getBodyFrameData().vertAccel << "," <<
    locationInfo->getBodyFrameData().yawRate << "," <<
    locationInfo->getBodyFrameData().pitch << "," <<
    locationInfo->getBodyFrameData().latAccelUnc << "," <<
    locationInfo->getBodyFrameData().longAccelUnc << "," <<
    locationInfo->getBodyFrameData().vertAccelUnc << "," <<
    locationInfo->getBodyFrameData().yawRateUnc << "," <<
    locationInfo->getBodyFrameData().pitchUnc << "," <<
    locationInfo->getBodyFrameData().pitchRate << "," <<
    locationInfo->getBodyFrameData().pitchRateUnc << "," <<
    locationInfo->getBodyFrameData().roll << "," <<
    locationInfo->getBodyFrameData().rollUnc << "," <<
    locationInfo->getBodyFrameData().rollRate << "," <<
    locationInfo->getBodyFrameData().rollRateUnc << "," <<
    locationInfo->getBodyFrameData().yaw << "," <<
    locationInfo->getBodyFrameData().yawUnc << "," <<
    locationInfo->getBodyFrameData().bodyFrameDataMask << "," <<
    locationInfo->getTimeUncMs() << ",";
    recordStream << static_cast<int>(leapSeconds) << "," <<
    unsigned(locationInfo->getCalibrationConfidencePercent()) << "," <<
    locationInfo->getCalibrationStatus() << "," <<
    locationInfo->getConformityIndex() << "," <<
    locationInfo->getVRPBasedLLA().latitude << "," <<
    locationInfo->getVRPBasedLLA().longitude << "," <<
    locationInfo->getVRPBasedLLA().altitude << "," <<
    enuVelocityVRPBased[0] << "," <<
    enuVelocityVRPBased[1] << "," <<
    enuVelocityVRPBased[2] << "," <<
    static_cast<int>(locationInfo->getAltitudeType()) << "," <<
    static_cast<int>(locationInfo->getReportStatus()) << "," <<
    locationInfo->getIntegrityRiskUsed() << "," <<
    locationInfo->getProtectionLevelAlongTrack() << "," <<
    locationInfo->getProtectionLevelCrossTrack() << "," <<
    locationInfo->getProtectionLevelVertical() << "," <<
    locationInfo->getSolutionStatus() << ",";

    auto measInfo = locationInfo->getmeasUsageInfo();
    recordStream << measInfo.size() << ",";

    for (unsigned i = 0; i < measInfo.size(); ++i ) {
        recordStream << measInfo[i].gnssSignalType << ","
                     << static_cast<int>(measInfo[i].gnssConstellation)
                     << "," << measInfo[i].gnssSvId << ",";
    }

    recordStream << velocityEastNorthUp.size() << ",";
    for (auto vel : velocityEastNorthUp) {
        recordStream << vel << ",";
    }

    recordStream << velocityUncertaintyEastNorthUp.size() << ",";
    for (auto velUncert : velocityUncertaintyEastNorthUp) {
        recordStream << velUncert << ",";
    }

    recordStream << SVIds.size() << ",";
    for (auto id : SVIds) {
        recordStream << id << ",";
    }
    recordStream << static_cast<int>(system) << ",";
    if (system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS ||
        system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GALILEO ||
        system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_BDS ||
        system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_QZSS ||
        system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_NAVIC) {
        telux::loc::TimeInfo timeInfo = sysTimeInfo.bds;
        recordStream << timeInfo.validityMask << "," <<
                        timeInfo.numClockResets << "," <<
                        timeInfo.refFCount << "," <<
                        timeInfo.systemClkTimeUncMs << "," <<
                        timeInfo.systemClkTimeBias << "," <<
                        timeInfo.systemMsec << "," <<
                        timeInfo.systemWeek << ",";
    } else if (system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GLONASS) {
        telux::loc::GlonassTimeInfo info = sysTimeInfo.glo;
        recordStream << info.validityMask << ","
                     << info.gloDays << ","
                     << info.gloMsec << ","
                     << info.gloClkTimeBias << ","
                     << info.gloClkTimeUncMs << ","
                     << info.refFCount << ","
                     << info.numClockResets << ","
                     << info.gloFourYear << ",";
    } // GNSS_LOC_SV_SYSTEM_SBAS, no timeInfo

    recordStream << locationInfo->getNavigationSolution() << "," <<
    locationInfo->getElapsedGptpTime() << "," <<
    locationInfo->getElapsedGptpTimeUnc() << ",";

    std::vector<uint16_t> dgnssStationIds = locationInfo->getDgnssStationIds();
    recordStream << dgnssStationIds.size() << ",";
    for (auto id : dgnssStationIds) {
        recordStream << id << ",";
    }

    recordStream << locationInfo->getBaselineLength() << "," <<
    locationInfo->getAgeOfCorrections() << ",";

    recordStream << locationInfo->getLeapSecondsUncertainty() << ",";

    DETAILED_RECORDING << recordStream.str() << std::endl;
}

void MyLocationListener::onDetailedLocationUpdate(
   const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) {
   if(!isDetailedReportFlagEnabled_) {
      return;
   }
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n*********************** Detailed Location Report "
                         "*********************"
                      << std::endl;
   printLocationValidity(locationInfo->getLocationInfoValidity());
   printLocationExValidity(locationInfo->getLocationInfoExValidity());
   printLocationTech(locationInfo->getTechMask());
   if(locationInfo->getTimeStamp() != telux::loc::UNKNOWN_TIMESTAMP) {
     time_t realtime;
     realtime = (time_t)((locationInfo->getTimeStamp() / 1000));
     std::cout << "Time stamp: " << locationInfo->getTimeStamp() << " mSec" << std::endl;
     std::cout << "GMT Time stamp: " << ctime(&realtime);
   } else {
     std::cout << "Time stamp Not Valid" << std::endl;
   }
   std::cout
      << "Speed: " << locationInfo->getSpeed() << std::endl
      << "Latitude: " << std::setprecision(15) << locationInfo->getLatitude() << std::endl
      << "Longitude: " << std::setprecision(15) << locationInfo->getLongitude() << std::endl
      << "Altitude: " << std::setprecision(15) << locationInfo->getAltitude() << std::endl
      << "Heading: " << locationInfo->getHeading() << std::endl
      << "Horizontal uncertainty: " << locationInfo->getHorizontalUncertainty() << std::endl
      << "Vertical uncertainty: " << locationInfo->getVerticalUncertainty() << std::endl
      << std::endl
      << "Altitude with respect to mean sea level: " << locationInfo->getAltitudeMeanSeaLevel()
      << std::endl
      << "Position DOP: " << locationInfo->getPositionDop() << std::endl
      << "Horizontal DOP: " << locationInfo->getHorizontalDop() << std::endl
      << "Vertical DOP: " << locationInfo->getVerticalDop() << std::endl
      << "Geometric DOP: " << locationInfo->getGeometricDop() << std::endl
      << "Time DOP: " << locationInfo->getTimeDop() << std::endl
      << "Magnetic deviation: " << locationInfo->getMagneticDeviation() << std::endl
      << "Speed uncertainty: " << locationInfo->getSpeedUncertainty() << std::endl
      << "Heading uncertainty: " << locationInfo->getHeadingUncertainty() << std::endl
      << "Elapsed real time: " << locationInfo->getElapsedRealTime() << std::endl
      << "Elapsed real time uncertainty: " << locationInfo->getElapsedRealTimeUncertainty()
      << std::endl
      << "Time uncertainty: " << locationInfo->getTimeUncMs() << std::endl
      << "elapsed gPTP time: " << locationInfo->getElapsedGptpTime() << std::endl
      << "elapsed gPTP time uncertainty: " << locationInfo->getElapsedGptpTimeUnc() << std::endl
      << "HorizontalUncertainty\nSemiMajor: " << locationInfo->getHorizontalUncertaintySemiMajor()
      << ", SemiMinor: " << locationInfo->getHorizontalUncertaintySemiMinor()
      << ", Azimuth: " << locationInfo->getHorizontalUncertaintyAzimuth() << std::endl
      << ", East standard deviation: " << locationInfo->getEastStandardDeviation() << std::endl
      << ", North standard deviation: " << locationInfo->getNorthStandardDeviation() << std::endl
      << ", Number of satellite vehicle used: " << locationInfo->getNumSvUsed() << std::endl;
   printSvUsedInPosition(locationInfo->getSvUsedInPosition());
   printHorizontalReliability(locationInfo->getHorizontalReliability());
   printVerticalReliability(locationInfo->getVerticalReliability());
   std::vector<uint16_t> SVIds;
   locationInfo->getSVIds(SVIds);
   if(SVIds.size() > 0) {
      std::cout << "Ids of used SVs : " << std::endl;
      for(auto i = 0; (unsigned)i < SVIds.size() - 1; ++i) {
         std::cout << SVIds[i] << ", ";
      }
      if(SVIds.size() > 0) {
         std::cout << SVIds[SVIds.size() - 1] << std::endl;
      }
   }
   printSbasCorrectionEx(locationInfo);
   printNavigationSolutionEx(locationInfo);
   printLocationPositionTech(locationInfo);
   printLocationPositionDynamics(locationInfo);
   printGnssMeasurementInfo(locationInfo);
   printGnssSystemTime(locationInfo);
   std::cout << " Time Uncertainty : " << locationInfo->getTimeUncMs() << std::endl;
   uint8_t leapSeconds = 0;

   if(locationInfo->getLeapSeconds(leapSeconds) == telux::common::Status::SUCCESS) {
      std::cout << "Leap seconds: " << static_cast<int>(leapSeconds) << std::endl;
   } else {
      std::cout << "No Leap seconds Provided" << std::endl;
   }

   std::vector<float> velocityEastNorthUp;
   if(locationInfo->getVelocityEastNorthUp(velocityEastNorthUp) == telux::common::Status::SUCCESS) {
      std::cout << "East, North, Up velocity: ";
      for(auto i = 0; (unsigned)i < velocityEastNorthUp.size() - 1; ++i) {
         std::cout << velocityEastNorthUp[i] << ", ";
      }
      if(velocityEastNorthUp.size() > 0) {
         std::cout << velocityEastNorthUp[velocityEastNorthUp.size() - 1];
      }
      std::cout << std::endl;
   } else {
      std::cout << "East, North, Up velocity Not Provided" << std::endl;
   }

   std::vector<float> velocityUncertaintyEastNorthUp;
   if(locationInfo->getVelocityUncertaintyEastNorthUp(velocityUncertaintyEastNorthUp)
      == telux::common::Status::SUCCESS) {
      std::cout << "East, North, Up velocity uncertainty: " << std::endl;
      for(auto i = 0; (unsigned)i < velocityEastNorthUp.size() - 1; ++i) {
         std::cout << velocityUncertaintyEastNorthUp[i] << ", ";
      }
      if(velocityEastNorthUp.size() > 0) {
         std::cout << velocityUncertaintyEastNorthUp[velocityEastNorthUp.size() - 1];
      }
      std::cout << std::endl;
   } else {
      std::cout << "East, North, Up velocity uncertainty Not Provided" << std::endl;
   }
   std::cout << "Calibration confidence percent : " <<
       unsigned(locationInfo->getCalibrationConfidencePercent()) << std::endl;
   printCalibrationStatus(locationInfo);
   printSolutionStatus(locationInfo);
   printLocOutputEngineType(locationInfo);
   printLocOutputEngineMask(locationInfo);
   std::cout << "Conformity index : " << locationInfo->getConformityIndex() << std::endl;
   printLLAVRPBasedInfo(locationInfo->getVRPBasedLLA());
   printENUVelocityVRPBased(locationInfo->getVRPBasedENUVelocity());
   printAltitudeType(locationInfo->getAltitudeType());
   printReportStatus(locationInfo->getReportStatus());
   std::cout << "Integrity risk used : " << locationInfo->getIntegrityRiskUsed() << std::endl;
   std::cout << "Protection level along track : " <<
       locationInfo->getProtectionLevelAlongTrack() << std::endl;
   std::cout << "Protection level cross track : " <<
       locationInfo->getProtectionLevelCrossTrack() << std::endl;
   std::cout << "Protection level vertical : " <<
       locationInfo->getProtectionLevelVertical() << std::endl;
   printDgnssStationIds(locationInfo->getDgnssStationIds());
   std::cout << "Baseline length : " <<
       locationInfo->getBaselineLength() << std::endl;
   std::cout << "Age of corrections : " <<
       locationInfo->getAgeOfCorrections() << std::endl;
   std::cout << "Leap seconds uncertainty : " <<
       static_cast<unsigned>(locationInfo->getLeapSecondsUncertainty()) << std::endl;
   std::cout << "*************************************************************" << std::endl;
   if(isRecordingEnabled_) {
       recordLocationInfo(locationInfo);
   }
}

void MyLocationListener::onDetailedEngineLocationUpdate(
      const std::vector<std::shared_ptr<telux::loc::ILocationInfoEx> > &locationEngineInfo) {
    if(!isDetailedEngineReportFlagEnabled_) {
      return;
    }
    uint32_t engReportCount = 0;

    std::cout << std::endl;
    PRINT_NOTIFICATION << "\n*********************** Detailed Engine Location Report "
                         "*********************"
                      << std::endl;
    std::cout << std::endl;
    for (auto locationInfo : locationEngineInfo) {
      std::cout << "For Engine[ " << ++engReportCount << " ]" << std::endl;
      printLocationValidity(locationInfo->getLocationInfoValidity());
      printLocationExValidity(locationInfo->getLocationInfoExValidity());
      if(locationInfo->getTimeStamp() != telux::loc::UNKNOWN_TIMESTAMP) {
        time_t realtime;
        realtime = (time_t)((locationInfo->getTimeStamp() / 1000));
        std::cout << "Time stamp: " << locationInfo->getTimeStamp() << " mSec" << std::endl;
        std::cout << "GMT Time stamp: " << ctime(&realtime);
      } else {
        std::cout << "Time stamp Not Valid" << std::endl;
      }
      std::cout
        << "Speed: " << locationInfo->getSpeed() << std::endl
        << "Latitude: " << std::setprecision(15) << locationInfo->getLatitude() << std::endl
        << "Longitude: " << std::setprecision(15) << locationInfo->getLongitude() << std::endl
        << "Altitude: " << std::setprecision(15) << locationInfo->getAltitude() << std::endl
        << "Heading: " << locationInfo->getHeading() << std::endl
        << "Horizontal uncertainty: " << locationInfo->getHorizontalUncertainty() << std::endl
        << "Vertical uncertainty: " << locationInfo->getVerticalUncertainty() << std::endl
        << std::endl
        << "Altitude with respect to mean sea level: " << locationInfo->getAltitudeMeanSeaLevel()
        << std::endl
        << "Position DOP: " << locationInfo->getPositionDop() << std::endl
        << "Horizontal DOP: " << locationInfo->getHorizontalDop() << std::endl
        << "Vertical DOP: " << locationInfo->getVerticalDop() << std::endl
        << "Geometric DOP: " << locationInfo->getGeometricDop() << std::endl
        << "Time DOP: " << locationInfo->getTimeDop() << std::endl
        << "Magnetic deviation: " << locationInfo->getMagneticDeviation() << std::endl
        << "Speed uncertainty: " << locationInfo->getSpeedUncertainty() << std::endl
        << "Heading uncertainty: " << locationInfo->getHeadingUncertainty() << std::endl
        << "Elapsed real time: " << locationInfo->getElapsedRealTime() << std::endl
        << "Elapsed real time uncertainty: " << locationInfo->getElapsedRealTimeUncertainty()
        << std::endl
        << ", Time uncertainty: " << locationInfo->getTimeUncMs() << std::endl
        << ", elapsed gPTP time: " << locationInfo->getElapsedGptpTime() << std::endl
        << ", elapsed gPTP time uncertainty: " << locationInfo->getElapsedGptpTimeUnc() << std::endl
        << "HorizontalUncertainty\nSemiMajor: " <<
            locationInfo->getHorizontalUncertaintySemiMajor()
        << ", SemiMinor: " << locationInfo->getHorizontalUncertaintySemiMinor()
        << ", Azimuth: " << locationInfo->getHorizontalUncertaintyAzimuth() << std::endl
        << ", East standard deviation: " << locationInfo->getEastStandardDeviation() << std::endl
        << ", North standard deviation: " << locationInfo->getNorthStandardDeviation() <<
           std::endl;
     printHorizontalReliability(locationInfo->getHorizontalReliability());
     printVerticalReliability(locationInfo->getVerticalReliability());
     std::vector<uint16_t> SVIds;
     locationInfo->getSVIds(SVIds);
     if(SVIds.size() > 0) {
      std::cout << "Ids of used SVs : " << std::endl;
      for(auto i = 0; (unsigned)i < SVIds.size() - 1; ++i) {
         std::cout << SVIds[i] << ", ";
      }
      if(SVIds.size() > 0) {
         std::cout << SVIds[SVIds.size() - 1] << std::endl;
      }
     }
     printSbasCorrectionEx(locationInfo);
     printNavigationSolutionEx(locationInfo);
     printLocationPositionTech(locationInfo);
     printLocationPositionDynamics(locationInfo);
     printGnssMeasurementInfo(locationInfo);
     printGnssSystemTime(locationInfo);
     std::cout << " Time Uncertainty : " << locationInfo->getTimeUncMs() << std::endl;
     uint8_t leapSeconds = 0;

     if(locationInfo->getLeapSeconds(leapSeconds) == telux::common::Status::SUCCESS) {
      std::cout << "Leap seconds: " << static_cast<int>(leapSeconds) << std::endl;
     }

     std::vector<float> velocityEastNorthUp;
     if(locationInfo->getVelocityEastNorthUp(velocityEastNorthUp) ==
         telux::common::Status::SUCCESS) {
      std::cout << "East, North, Up velocity: ";
      for(auto i = 0; (unsigned)i < velocityEastNorthUp.size() - 1; ++i) {
         std::cout << velocityEastNorthUp[i] << ", ";
      }
      if(velocityEastNorthUp.size() > 0) {
         std::cout << velocityEastNorthUp[velocityEastNorthUp.size() - 1];
      }
      std::cout << std::endl;
     }

     std::vector<float> velocityUncertaintyEastNorthUp;
     if(locationInfo->getVelocityUncertaintyEastNorthUp(velocityUncertaintyEastNorthUp)
       == telux::common::Status::SUCCESS) {
      std::cout << "East, North, Up velocity uncertainty: " << std::endl;
      for(auto i = 0; (unsigned)i < velocityEastNorthUp.size() - 1; ++i) {
         std::cout << velocityUncertaintyEastNorthUp[i] << ", ";
      }
      if(velocityEastNorthUp.size() > 0) {
         std::cout << velocityUncertaintyEastNorthUp[velocityEastNorthUp.size() - 1];
      }
      std::cout << std::endl;
     }
     std::cout << "Calibration confidence percent : " <<
       unsigned(locationInfo->getCalibrationConfidencePercent()) << std::endl;
     printCalibrationStatus(locationInfo);
     printSolutionStatus(locationInfo);
     printLocOutputEngineType(locationInfo);
     printLocOutputEngineMask(locationInfo);
     std::cout << "Conformity index : " << locationInfo->getConformityIndex() << std::endl;
     printLLAVRPBasedInfo(locationInfo->getVRPBasedLLA());
     printENUVelocityVRPBased(locationInfo->getVRPBasedENUVelocity());
     printAltitudeType(locationInfo->getAltitudeType());
     printReportStatus(locationInfo->getReportStatus());
     std::cout << "Integrity risk used : " << locationInfo->getIntegrityRiskUsed() << std::endl;
     std::cout << "Protection level along track : " <<
         locationInfo->getProtectionLevelAlongTrack() << std::endl;
     std::cout << "Protection level cross track : " <<
         locationInfo->getProtectionLevelCrossTrack() << std::endl;
     std::cout << "Protection level vertical : " <<
         locationInfo->getProtectionLevelVertical() << std::endl;
     printDgnssStationIds(locationInfo->getDgnssStationIds());
     std::cout << "Baseline length : " <<
         locationInfo->getBaselineLength() << std::endl;
     std::cout << "Age of corrections : " <<
         locationInfo->getAgeOfCorrections() << std::endl;
     std::cout << "Leap seconds uncertainty : " <<
       static_cast<unsigned>(locationInfo->getLeapSecondsUncertainty()) << std::endl;
     std::cout << "*************************************************************" << std::endl;

     if(isRecordingEnabled_) {
         recordLocationInfo(locationInfo);
     }
    }
}

void MyLocationListener::onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) {
   if(!isSvInfoFlagEnabled_) {
      return;
   }
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n**************** Satellite Vehicle Information ***************"
                      << std::endl;
   for(auto svInfo : gnssSVInfo->getSVInfoList()) {
      std::cout << "**** GNSS SV Id : " << svInfo->getId() << " ****" << std::endl;
      printConstellationType(svInfo->getConstellation());
      printEphimerisAvailability(svInfo->getHasEphemeris());
      printAlmanacAvailability(svInfo->getHasAlmanac());
      printFixAvailability(svInfo->getHasFix());
      std::cout << "Elevation: " << svInfo->getElevation() << ", Azimuth: " << svInfo->getAzimuth()
                << ", Signal Strength: " << svInfo->getSnr() << std::endl;
      std::cout << std::setprecision(15) << std::showpoint;
      std::cout << "Carrier frequency: " << svInfo->getCarrierFrequency() << std::endl;
      printGnssSignalType(svInfo->getSignalType());
      std::cout << "Glonass FCN: " << svInfo->getGlonassFcn() << std::endl;
      std::cout << "Baseband Carrier To Noise Ratio: " << svInfo->getBasebandCnr()
                << std::endl;
   }
   std::cout << "*************************************************************" << std::endl;

   if (isRecordingEnabled_) {
       //Recording Data.
       std::ostringstream recordStream;
       //Format: type, SV number, each SV info
       recordStream << SATELLITE_VEHICLE << ",";
       for (auto svInfo : gnssSVInfo->getSVInfoList()) {
           recordStream << svInfo->getId() << ","
                        << static_cast<int>(svInfo->getConstellation()) << ","
                        << static_cast<int>(svInfo->getHasEphemeris()) << ","
                        << static_cast<int>(svInfo->getHasAlmanac()) << ","
                        << static_cast<int>(svInfo->getHasFix()) << ","
                        << svInfo->getElevation() << ","
                        << svInfo->getAzimuth() << ","
                        << svInfo->getSnr() << ","
                        << std::setprecision(15) << std::showpoint
                        << svInfo->getCarrierFrequency() << ","
                        << svInfo->getSignalType() << ","
                        << svInfo->getGlonassFcn() << ","
                        << svInfo->getBasebandCnr() << ",";
       }

       DETAILED_RECORDING << recordStream.str() << std::endl;
   }
}

void MyLocationListener::onGnssSignalInfo(
   const std::shared_ptr<telux::loc::IGnssSignalInfo> &gnssDatainfo) {

   if(!isDataInfoFlagEnabled_) {
      return;
   }
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n**************** Gnss Signal Information ***************" << std::endl;
   std::cout << "<<< onGnssDataCb\n" << std::endl;

   //Recording Data.
   std::ostringstream recordStream;

   for(int sig = 0; sig < static_cast<int>(
                             telux::loc::GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES);
       sig++) {
      std::cout << "Signal Type : " << sig << std::endl;

      recordStream << gnssDatainfo->getGnssData().gnssDataMask[sig] << ",";
      if(telux::loc::GnssDataValidityType::HAS_JAMMER
         == ((gnssDatainfo->getGnssData().gnssDataMask[sig])
             & (telux::loc::GnssDataValidityType::HAS_JAMMER))) {
         std::cout << " gnssDataMask: " << gnssDatainfo->getGnssData().gnssDataMask[sig]
                   << std::endl;
         std::cout << " jammerInd: " << gnssDatainfo->getGnssData().jammerInd[sig] << std::endl;
         recordStream << gnssDatainfo->getGnssData().jammerInd[sig] << ",";
      } else {
         std::cout << "JAMMER Ind Not Present  " << std::endl;
         recordStream << 0 << ",";
      }
      if(telux::loc::GnssDataValidityType::HAS_AGC
         == ((gnssDatainfo->getGnssData().gnssDataMask[sig])
             & (telux::loc::GnssDataValidityType::HAS_AGC))) {
         std::cout << " gnssDataMask: " << gnssDatainfo->getGnssData().gnssDataMask[sig]
                   << std::endl;
         std::cout << " agc: " << gnssDatainfo->getGnssData().agc[sig]
                   << std::endl;
         recordStream << gnssDatainfo->getGnssData().agc[sig] << ",";
      } else {
         std::cout << "AGC Not Present  " << std::endl;
         recordStream << 0 << ",";
      }
      std::cout << std::endl;
   }
   std::cout << "AGC L1 Status: " << gnssDatainfo->getGnssData().agcStatusL1
             << std::endl;
   recordStream << gnssDatainfo->getGnssData().agcStatusL1 << ",";
   std::cout << "AGC L2 Status: " << gnssDatainfo->getGnssData().agcStatusL2
             << std::endl;
   recordStream << gnssDatainfo->getGnssData().agcStatusL2 << ",";
   std::cout << "AGC L5 Status: " << gnssDatainfo->getGnssData().agcStatusL5
             << std::endl;
   recordStream << gnssDatainfo->getGnssData().agcStatusL5 << ",";
   std::cout << "*************************************************************" << std::endl;
   if (isRecordingEnabled_) {
       DETAILED_RECORDING << DATA << "," << recordStream.str() << std::endl;
   }
}

void MyLocationListener::onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) {
   if(!isNmeaInfoFlagEnabled_) {
      return;
   }
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n**************** Gnss Nmea Information ***************" << std::endl;
   std::cout << "<<< onGnssNmeaCb\n" << std::endl;
   std::cout << " Timestamp : " << timestamp << std::endl;
   std::cout << " Nmea String : " << nmea << std::endl;

   //Recording Data.
   std::ostringstream recordStream;
   //Format: type, timestamp, nmea
   recordStream << NMEA << "," << timestamp << "," << nmea;

   if (isRecordingEnabled_) {
       DETAILED_RECORDING << recordStream.str() << std::endl;
   }
}

void MyLocationListener::onEngineNmeaInfo(telux::loc::LocationAggregationType engineType,
    uint64_t timestamp, const std::string &nmea) {
    if(!isEngineNmeaInfoFlagEnabled_) {
        return;
    }
    PRINT_NOTIFICATION << "\n**************** Engine Nmea Information ***************" << std::endl;
    LocationUtils::displayLocEngineType(engineType);
    std::cout << " Timestamp : " << timestamp << std::endl;
    std::cout << " Nmea String : " << nmea << std::endl;
}

void MyLocationListener::onGnssExtendedDataInfo(const std::vector<uint8_t>& payload) {
    PRINT_NOTIFICATION << "\n************ Gnss Extended Information ***********" << std::endl;
    size_t length = payload.size();
    std::cout << " Payload len : " << length << std::endl;
    std::cout << " Payload byte information: ";
    if(!isExtendedInfoFlagEnabled_) {
        std::cout << static_cast<unsigned>(payload[0]) << " "
                  << static_cast<unsigned>(payload[1]) << " "
                  << static_cast<unsigned>(payload[length - 2]) << " "
                  << static_cast<unsigned>(payload[length - 1]) << std::endl;
        return;
    }
    for(size_t i = 0; i < length; i++) {
        std::cout << static_cast<unsigned>(payload[i]) << " ";
    }
    std::cout << std::endl;
}

void MyLocationListener::onGnssMeasurementsInfo(const telux::loc::
     GnssMeasurements &measurementInfo) {
   if(!isMeasurementsInfoFlagEnabled_) {
      return;
   }
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n**************** Gnss Measurements Information ***************"
       << std::endl;
   std::cout << "<<< onGnssMeasurementsCb\n" << std::endl;

   //Recording Data.
   std::ostringstream recordStream;
   //Format: type, timestamp, nmea
   recordStream << MEASUREMENT << ",";
   printMeasurementsClockValidity(measurementInfo.clock.valid);
   recordStream << measurementInfo.clock.valid << ",";
   std::cout
      << " Leap second, in unit of seconds " << measurementInfo.clock.leapSecond << std::endl
      << " Time, in unit of ns " << measurementInfo.clock.timeNs << std::endl
      << " Time uncertainty in unit of ns " << measurementInfo.clock.timeUncertaintyNs << std::endl
      << " Full bias, in unit of ns " << measurementInfo.clock.fullBiasNs << std::endl
      << " Sub-nanoseconds bias in unit of ns " << measurementInfo.clock.biasNs << std::endl
      << " Bias uncertainty in unit of ns " << measurementInfo.clock.biasUncertaintyNs << std::endl
      << " Clock drift " << measurementInfo.clock.driftNsps << std::endl
      << " Clock drift uncertainty " << measurementInfo.clock.driftUncertaintyNsps << std::endl
      << " HW clock discontinuity count " << measurementInfo.clock.hwClockDiscontinuityCount
      << std::endl
      << " elapsed real time " << measurementInfo.clock.elapsedRealTime << std::endl
      << " elapsed real time uncertainty " << measurementInfo.clock.elapsedRealTimeUnc << std::endl
      << " elapsed gPTP time " << measurementInfo.clock.elapsedgPTPTime << std::endl
      << " elapsed gPTP time uncertainty " << measurementInfo.clock.elapsedgPTPTimeUnc << std::endl;
   recordStream << measurementInfo.clock.leapSecond << ","
                << measurementInfo.clock.timeNs << ","
                << measurementInfo.clock.timeUncertaintyNs << ","
                << measurementInfo.clock.fullBiasNs << ","
                << measurementInfo.clock.biasNs << ","
                << measurementInfo.clock.biasUncertaintyNs << ","
                << measurementInfo.clock.driftNsps << ","
                << measurementInfo.clock.driftUncertaintyNsps << ","
                << measurementInfo.clock.hwClockDiscontinuityCount << ","
                << measurementInfo.clock.elapsedRealTime << ","
                << measurementInfo.clock.elapsedRealTimeUnc << ","
                << measurementInfo.clock.elapsedgPTPTime << ","
                << measurementInfo.clock.elapsedgPTPTimeUnc << ",";

   for( auto &measData : measurementInfo.measurements) {
     std::cout << "\n*************** Measurement Data ******************* " << std::endl;
     printMeasurementsDataValidity(measData.valid);
     recordStream << measData.valid << ",";
     std::cout << " Specify satellite vehicle ID number " << measData.svId << std::endl;
     recordStream << measData.svId << ",";
     printConstellationType(measData.svType);
     recordStream << static_cast<int>(measData.svType) << ",";
     std::cout << " Time offset when the measurement was taken, in ns " << measData.timeOffsetNs
         << std::endl;
     recordStream << measData.timeOffsetNs << ",";
     printMeasurementState(measData.stateMask);
     recordStream << measData.stateMask << ",";
     std::cout << " Received GNSS time of the week in nanoseconds " << measData.receivedSvTimeNs
         << std::endl;
     recordStream << measData.receivedSvTimeNs << ",";
     std::cout << " Sub-nanoseconds of GNSS time of the week " << measData.receivedSvTimeSubNs
         << std::endl
               << " Satellite time, in ns " << measData.receivedSvTimeUncertaintyNs << std::endl
               << " Signal strength, carrier to noise ratio " << measData.carrierToNoiseDbHz
         << std::endl
               << " Uncorrected pseudorange rate " << measData.pseudorangeRateMps
         << std::endl
               << " Uncorrected pseudorange rate uncertainty " <<
         measData.pseudorangeRateUncertaintyMps << std::endl;
     recordStream << measData.receivedSvTimeSubNs << ","
                  << measData.receivedSvTimeUncertaintyNs << ","
                  << measData.carrierToNoiseDbHz << ","
                  << measData.pseudorangeRateMps << ","
                  << measData.pseudorangeRateUncertaintyMps << ",";
     printMeasurementAdrState(measData.adrStateMask);
     recordStream << measData.adrStateMask << ",";
     std::cout << " Accumulated delta range " << measData.adrMeters << std::endl
               << " Accumulated delta range uncertainty " << measData.adrUncertaintyMeters
         << std::endl
               << " Carrier frequency of the tracked signal " << measData.carrierFrequencyHz
         << std::endl
               << " The number of full carrier cycles between the receiver and the satellite "
         << measData.carrierCycles << std::endl
               << " The RF carrier phase " << measData.carrierPhase <<std::endl
               << " RF carrier phase uncertainty " << measData.carrierPhaseUncertainty
         <<std::endl;
     recordStream << measData.adrMeters << "," << measData.adrUncertaintyMeters << ","
                  << measData.carrierFrequencyHz << "," << measData.carrierCycles << ","
                  << measData.carrierPhase << "," << measData.carrierPhaseUncertainty << ",";
     printMeasurementsMultipathIndicator(measData.multipathIndicator);
     recordStream << measData.multipathIndicator << ",";
     std::cout << " Signal to noise ratio " << measData.signalToNoiseRatioDb << std::endl
               << " Automatic gain control level " << measData.agcLevelDb << std::endl;
     recordStream << measData.signalToNoiseRatioDb << "," << measData.agcLevelDb << ",";
     printGnssSignalType(measData.gnssSignalType);
     recordStream << measData.gnssSignalType << ",";
     std::cout << " Carrier-to-noise ratio of the signal measured at baseband : "
               << measData.basebandCarrierToNoise << std::endl;
     recordStream << measData.basebandCarrierToNoise <<  ",";
     std::cout << " Full inter-signal bias : " << measData.fullInterSignalBias
               << std::endl;
     recordStream <<  measData.fullInterSignalBias <<  ",";
     std::cout << " Uncertainty associated with the full inter-signal bias : "
               << measData.fullInterSignalBiasUncertainty << std::endl;
     recordStream << measData.fullInterSignalBiasUncertainty <<  ",";
     std::cout << "\n********************** " << std::endl;
   }
   std::cout << "NHz measurements indicator: " << std::boolalpha << measurementInfo.isNHz
             << std::endl;
   std::cout << "AGC L1 Status: " << measurementInfo.agcStatusL1
             << std::endl;
   std::cout << "AGC L2 Status: " << measurementInfo.agcStatusL2
             << std::endl;
   std::cout << "AGC L5 Status: " << measurementInfo.agcStatusL5
             << std::endl;
   std::cout << "*************************************************************" << std::endl;

   recordStream << static_cast<int>(measurementInfo.isNHz) << ",";
   recordStream << measurementInfo.agcStatusL1 << ",";
   recordStream << measurementInfo.agcStatusL2 << ",";
   recordStream << measurementInfo.agcStatusL5 << ",";

   if (isRecordingEnabled_) {
       DETAILED_RECORDING << recordStream.str() << std::endl;
   }

   //Recording dummy payload for GNSS extended data
   std::ostringstream extendedDataRecordStream;
   extendedDataRecordStream << EXTENDED_DATA << "," << extendedDataPayload_;

   if (isRecordingEnabled_) {
       DETAILED_RECORDING << extendedDataRecordStream.str() << std::endl;
   }
}

void MyLocationListener::onGnssDisasterCrisisInfo(
    const telux::loc::GnssDisasterCrisisReport &dcReportInfo) {
    if(!isDisasterCrisisInfoFlagEnabled_) {
        return;
    }
    PRINT_NOTIFICATION << "\n************ Gnss Disaster-Crisis Information *************" << "\n";
    LocationUtils::displayDisasterCrisisReportType(dcReportInfo);
    std::cout << "Disaster-crisis Valid bits: " << dcReportInfo.numValidBits << "\n";
    if (dcReportInfo.prnValid) {
         std::cout << "Disaster-crisis prn valid " << "\n";
         std::cout << "Disaster-crisis prn: " << static_cast<unsigned>(dcReportInfo.prn) << "\n";
    } else {
         std::cout << "Disaster-crisis prn Invalid " << "\n";
    }
    std::cout << "Disaster-crisis Report data: \n";
    for(auto &itr: dcReportInfo.dcReportData) {
        std::cout << (int)itr << ": ";
        char hex_string[16];
        snprintf(hex_string, 16, "%x", itr);
        std::cout << "0x" << hex_string << "\n";
    }
    std::cout << "\n";
}

void MyLocationListener::printEphSrc(telux::loc::GnssEphSource ephSrc) {
    switch(ephSrc) {
        case telux::loc::EPH_SRC_OTA : std::cout << "OTA"; break;
        default: std::cout << "Unknown"; break;
    }
}

void MyLocationListener::printEphAct(telux::loc::GnssEphAction ephAct) {
    switch(ephAct) {
        case telux::loc::EPH_ACTION_UPDATE: std::cout << "Update"; break;
        case telux::loc::EPH_ACTION_DELETE: std::cout << "Delete"; break;
        default: std::cout << "Unknown"; break;
    }
}

void MyLocationListener::printGnssEphemerisCommonData(const telux::loc::GnssEphCommon &commonData) {
    std::cout   << "Common Data";
    std::cout   << "\nSVID       : " << commonData.gnssSvId;
    std::cout   << "\nephSource    : "; printEphSrc(commonData.ephSource);
    std::cout   << "\naction       : "; printEphAct(commonData.action);
    std::cout   << "\nIODE         : " << commonData.IODE
                << "\naSqrt        : " << std::fixed << std::setprecision(15) << commonData.aSqrt
                << "\ndeltaN       : " << std::fixed << std::setprecision(15) << commonData.deltaN
                << "\nm0           : " << std::fixed << std::setprecision(15) << commonData.m0
                << "\neccentricity : " << std::fixed << std::setprecision(15) << commonData.eccentricity
                << "\nomega0       : " << std::fixed << std::setprecision(15) << commonData.omega0
                << "\ni0           : " << std::fixed << std::setprecision(15) << commonData.i0
                << "\nomega        : " << std::fixed << std::setprecision(15) << commonData.omega
                << "\nomegaDot     : " << std::fixed << std::setprecision(15) << commonData.omegaDot
                << "\niDot         : " << std::fixed << std::setprecision(15) << commonData.iDot
                << "\ncUc          : " << std::fixed << std::setprecision(15) << commonData.cUc
                << "\ncUs          : " << std::fixed << std::setprecision(15) << commonData.cUs
                << "\ncRc          : " << std::fixed << std::setprecision(15) << commonData.cRc
                << "\ncRs          : " << std::fixed << std::setprecision(15) << commonData.cRs
                << "\ncIc          : " << std::fixed << std::setprecision(15) << commonData.cIc
                << "\ncIs          : " << std::fixed << std::setprecision(15) << commonData.cIs
                << "\ntoe          : " << commonData.toe
                << "\ntoc          : " << commonData.toc
                << "\naf0          : " << std::fixed << std::setprecision(15) << commonData.af0
                << "\naf1          : " << std::fixed << std::setprecision(15) << commonData.af1
                << "\naf2          : " << std::fixed << std::setprecision(15) << commonData.af2;
}

void MyLocationListener::printGpsQzssExtendedEphemerisData(
    const telux::loc::GpsQzssExtEphemeris &extEphData) {
    std::cout   << "GPS QZSS Extended Ephemeris Data";
    std::cout   << "\n svID                 : " << extEphData.gnssSvId;
    LocationUtils::displayGpsQzssExtEphValidity(extEphData.validityMask);
    std::cout   << "\n iscL1ca              : " <<
                    std::fixed << std::setprecision(15) << extEphData.iscL1ca
                << "\n iscL2c               : " <<
                    std::fixed << std::setprecision(15) << extEphData.iscL2c
                << "\n iscL5I5              : " <<
                    std::fixed << std::setprecision(15) << extEphData.iscL5I5
                << "\n iscL5Q5              : " <<
                    std::fixed << std::setprecision(15) << extEphData.iscL5Q5
                << "\n alert                : " << extEphData.alert
                << "\n uraNed0              : " << extEphData.uraNed0
                << "\n uraNed1              : " << extEphData.uraNed1
                << "\n uraNed2              : " << extEphData.uraNed2
                << "\n top                  : " <<
                    std::fixed << std::setprecision(15) << extEphData.top
                << "\n topClock             : " << extEphData.topClock
                << "\n validityPeriod       : " << extEphData.validityPeriod
                << "\n deltaNdot            : " <<
                    std::fixed << std::setprecision(15) << extEphData.deltaNdot
                << "\n deltaA               : " <<
                    std::fixed << std::setprecision(15) << extEphData.deltaA
                << "\n adot                 : " <<
                    std::fixed << std::setprecision(15) << extEphData.adot;
}

void MyLocationListener::printGpsQzssEphData(const telux::loc::GpsQzssEphemeris &ephData) {
    printGnssEphemerisCommonData(ephData.commonData);
    std::cout   << "\nSignal Health   : " << static_cast<unsigned>(ephData.signalHealth)
                << "\nURAI            : " << static_cast<unsigned>(ephData.URAI)
                << "\ncodeL2          : " << static_cast<unsigned>(ephData.codeL2)
                << "\ndataFlagL2P     : " << static_cast<unsigned>(ephData.dataFlagL2P)
                << "\ntgd             : " << std::fixed << std::setprecision(15) << ephData.tgd
                << "\nfitInterval     : " << static_cast<unsigned>(ephData.fitInterval)
                << "\nIODC            : " << ephData.IODC
                << "\nGpsQzss Extended Eph Validity: " << ephData.extendedEphDataValidity << "\n";
    if(ephData.extendedEphDataValidity) {
        printGpsQzssExtendedEphemerisData(ephData.gpsQzssExtEphData);
    }
}

void MyLocationListener::printBdsExtendedEphemerisData(
    const telux::loc::BdsExtEphemeris &extEphData) {
    std::cout   << "BDS Extended Ephemeris Data";
    std::cout   << "\n svID                 : " << extEphData.gnssSvId;
    LocationUtils::displayBdsExtEphValidity(extEphData.validityMask);
    LocationUtils::displayBdsSvType(extEphData.svType);
    std::cout   << "\n tgdB2a              :" <<
                    std::fixed << std::setprecision(15) << extEphData.tgdB2a
                << "\n iscB2a              :" <<
                    std::fixed << std::setprecision(15) << extEphData.iscB2a
                << "\n tgdB1c              :" <<
                    std::fixed << std::setprecision(15) << extEphData.tgdB1c
                << "\n iscB1c              :" <<
                    std::fixed << std::setprecision(15) << extEphData.iscB1c
                << "\n validityPeriod      :" << extEphData.validityPeriod
                << "\n integrityFlags      :" << static_cast<unsigned>(extEphData.integrityFlags)
                << "\n deltaNdot            : " <<
                    std::fixed << std::setprecision(15) << extEphData.deltaNdot
                << "\n deltaA               : " <<
                    std::fixed << std::setprecision(15) << extEphData.deltaA
                << "\n adot                 : " <<
                    std::fixed << std::setprecision(15) << extEphData.adot;
}

void MyLocationListener::onGnssEphemerisInfo(const telux::loc::GnssEphemeris &ephemerisInfo) {
    if(!isEphemerisInfoFlagEnabled_) {
        return;
    }
    PRINT_NOTIFICATION << "\n************ Gnss Ephemeris Information *************" << "\n";
    std::cout << "Is System time valid - " << ephemerisInfo.isSystemTimeValid << "\n";
    std::cout << "Gnss system time info - \n";
    telux::loc::TimeInfo timeInfo = ephemerisInfo.timeInfo;
    std::cout << "Validity mask: " << timeInfo.validityMask;
    std::cout << " System time week: " << timeInfo.systemWeek;
    std::cout << " System time week ms: " << timeInfo.systemMsec;
    std::cout << " System clk time: " << timeInfo.systemClkTimeBias;
    std::cout << " System clk time uncertainty valid: " << timeInfo.systemClkTimeUncMs;
    std::cout << " System reference valid: " << timeInfo.refFCount;
    std::cout << " System num clock reset valid: " << timeInfo.numClockResets << "\n";
    telux::loc::GnssSystem system = ephemerisInfo.constellationType;
    std::cout << "Constellation type: ";
    if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS) {
        std::cout << "GPS satellite" << "\n";
        for(size_t i = 0; i < ephemerisInfo.gpsEphemerisData.size(); i++) {
            printGpsQzssEphData(ephemerisInfo.gpsEphemerisData[i]);
        }
    } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GALILEO) {
        std::cout << "GALILEO satellite" << "\n";
        for(size_t i = 0; i < ephemerisInfo.galEphemerisData.size(); i++) {
            printGnssEphemerisCommonData(ephemerisInfo.galEphemerisData[i].commonData);
            switch(ephemerisInfo.galEphemerisData[i].dataSourceSignal) {
                case telux::loc::GAL_SIG_SRC_E1B : std::cout << "\nGal signal source: E1B"; break;
                case telux::loc::GAL_SIG_SRC_E5A : std::cout << "\nGal signal source: E5A"; break;
                case telux::loc::GAL_SIG_SRC_E5B : std::cout << "\nGal signal source: E5B"; break;
                default: std::cout << "\nGal signal source: Unknown"; break;
            }
            std::cout << "\nsisIndex    : "
                      << static_cast<unsigned>(ephemerisInfo.galEphemerisData[i].sisIndex)
                      << "\nbgdE1E5a    : " << ephemerisInfo.galEphemerisData[i].bgdE1E5a
                      << "\nbgdE1E5b    : " << ephemerisInfo.galEphemerisData[i].bgdE1E5b
                      << "\nsvHealth    : "
                      << static_cast<unsigned>(ephemerisInfo.galEphemerisData[i].svHealth) << "\n";
        }
    } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GLONASS) {
        std::cout << "GLONASS satellite" << "\n";
        for(size_t i = 0; i < ephemerisInfo.gloEphemerisData.size(); i++) {
            std::cout   << "\ngnssSvId      : " << ephemerisInfo.gloEphemerisData[i].gnssSvId;
            std::cout   << "\nephSource     : ";
            printEphSrc(ephemerisInfo.gloEphemerisData[i].ephSource);
            std::cout   << "\naction        : ";
            printEphAct(ephemerisInfo.gloEphemerisData[i].action);
            std::cout   << "\nbnHealth      : "
                        << static_cast<unsigned>(ephemerisInfo.gloEphemerisData[i].bnHealth)
                        << "\nlnHealth      : "
                        << static_cast<unsigned>(ephemerisInfo.gloEphemerisData[i].lnHealth)
                        << "\ntb            : "
                        << static_cast<unsigned>(ephemerisInfo.gloEphemerisData[i].tb)
                        << "\nft            : "
                        << static_cast<unsigned>(ephemerisInfo.gloEphemerisData[i].ft)
                        << "\ngloM          : "
                        << static_cast<unsigned>(ephemerisInfo.gloEphemerisData[i].gloM)
                        << "\nenAge         : "
                        << static_cast<unsigned>(ephemerisInfo.gloEphemerisData[i].enAge)
                        << "\ngloFrequency  : "
                        << static_cast<unsigned>(ephemerisInfo.gloEphemerisData[i].gloFrequency)
                        << "\np1            : "
                        << static_cast<unsigned>(ephemerisInfo.gloEphemerisData[i].p1)
                        << "\np2            : "
                        << static_cast<unsigned>(ephemerisInfo.gloEphemerisData[i].p2)
                        << "\ndeltaTau      : " << ephemerisInfo.gloEphemerisData[i].deltaTau
                        << "\ntauN          : " << ephemerisInfo.gloEphemerisData[i].tauN
                        << "\ngamma         : " << ephemerisInfo.gloEphemerisData[i].gamma
                        << "\ntoe           : " << ephemerisInfo.gloEphemerisData[i].toe
                        << "\nnt            : " << ephemerisInfo.gloEphemerisData[i].nt;
            std::cout   << "\nGlo position: ";
            for(size_t k = 0 ; k < 3; k++) {
                std::cout << ephemerisInfo.gloEphemerisData[i].position[k] << " ";
            }
            std::cout   << "\nGlo velocity: ";
            for(size_t k = 0 ; k < 3; k++) {
                std::cout << ephemerisInfo.gloEphemerisData[i].velocity[k] << " ";
            }
            std::cout   << "\nGlo acceleration: ";
            for(size_t k = 0 ; k < 3; k++) {
                std::cout << ephemerisInfo.gloEphemerisData[i].acceleration[k] << " ";
            }
        }
    } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_BDS) {
        std::cout << "BDS satellite" << "\n";
        for(size_t i = 0; i < ephemerisInfo.bdsEphemerisData.size(); i++) {
            printGnssEphemerisCommonData(ephemerisInfo.bdsEphemerisData[i].commonData);
            std::cout   << "\nsvHealth    : "
                        << static_cast<unsigned>(ephemerisInfo.bdsEphemerisData[i].svHealth)
                        << "\nAODC        : "
                        << static_cast<unsigned>(ephemerisInfo.bdsEphemerisData[i].AODC)
                        << "\ntgd1        : "
                        << std::fixed << std::setprecision(15) << ephemerisInfo.bdsEphemerisData[i].tgd1
                        << "\ntgd2        : "
                        << std::fixed << std::setprecision(15) << ephemerisInfo.bdsEphemerisData[i].tgd2
                        << "\nURAI        : "
                        << static_cast<unsigned>(ephemerisInfo.bdsEphemerisData[i].URAI)
                        << "\nBds Extended Eph Validity: " <<
                            ephemerisInfo.bdsEphemerisData[i].extendedEphDataValidity << "\n";
            if(ephemerisInfo.bdsEphemerisData[i].extendedEphDataValidity) {
                printBdsExtendedEphemerisData(ephemerisInfo.bdsEphemerisData[i].bdsExtEphData);
            }
        }
    } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_QZSS) {
        std::cout << "QZSS satellite" << "\n";
        for(size_t i = 0; i < ephemerisInfo.qzssEphemerisData.size(); i++) {
            printGpsQzssEphData(ephemerisInfo.qzssEphemerisData[i].qzssEphData);
        }
    } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_NAVIC) {
        std::cout << "NAVIC satellite" << "\n";
        for(size_t i = 0; i < ephemerisInfo.navicEphemerisData.size(); i++) {
            printGnssEphemerisCommonData(ephemerisInfo.navicEphemerisData[i].commonData);
            std::cout   << "\nweekNum               : "
                        << ephemerisInfo.navicEphemerisData[i].weekNum
                        << "\niodec                 : "
                        << ephemerisInfo.navicEphemerisData[i].iodec
                        << "\nl5Health              : "
                        << static_cast<unsigned>(ephemerisInfo.navicEphemerisData[i].l5Health)
                        << "\nsHealth               : "
                        << static_cast<unsigned>(ephemerisInfo.navicEphemerisData[i].sHealth)
                        << "\ninclinationAngleRad   : "
                        << ephemerisInfo.navicEphemerisData[i].inclinationAngleRad
                        << "\nurai                  : "
                        << static_cast<unsigned>(ephemerisInfo.navicEphemerisData[i].urai)
                        << "\ntgd                   : "
                        << ephemerisInfo.navicEphemerisData[i].tgd << "\n";
        }
    } else {
        std::cout << "UNKNOWN satellite" << "\n";
    }
    if(ephemerisInfo.validDataSourceSignal) {
        LocationUtils::displayGnssDataSignal(ephemerisInfo.dataSourceSignal);
    } else {
        std::cout << "Invalid data source signal \n";
    }
    std::cout << "\n";
}

void MyLocationListener::onLocationSystemInfo(const telux::loc::LocationSystemInfo
     &locationSystemInfo) {
   if(!isLocSysInfoFlagEnabled_) {
      return;
   }
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n**************** Location System Information ***************" << std::endl;
   std::cout << "<<< onLocationSystemInfoCb\n" << std::endl;
   std::cout << " LocationSystemInfoValidity : " << std::endl;
   telux::loc::LocationSystemInfoValidity locationSystemInfoMask = locationSystemInfo.valid;
   if(locationSystemInfoMask & telux::loc::LOCATION_SYS_INFO_LEAP_SECOND) {
       std::cout << " Contains current leap second or leap second change info" << std::endl;
   }
   std::cout << " LeapSecondInfoValidity : " << std::endl;
   telux::loc::LeapSecondInfoValidity leapSecondSysInfoMask = locationSystemInfo.info.
       valid;
   if(leapSecondSysInfoMask & telux::loc::LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT) {
       std::cout << " Current leap second info is available." << std::endl;
   }
   if(leapSecondSysInfoMask & telux::loc::LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT) {
       std::cout << " The last known leap change event is available." << std::endl;
   }
   std::cout << " leapSecondCurrent : " << unsigned(locationSystemInfo.info.current) << std::endl;
   telux::loc::TimeInfo timeInfo = locationSystemInfo.info.info.
       timeInfo;
   std::cout << "TimeInfo : " << std::endl;

   std::cout << "System time week: " << timeInfo.systemWeek << std::endl;
   std::cout << "System time week ms: " << timeInfo.systemMsec << std::endl;
   std::cout << "System clk time: " << timeInfo.systemClkTimeBias << std::endl;
   std::cout << "System clk time uncertainty valid: " << timeInfo.systemClkTimeUncMs << std::endl;
   std::cout << "System reference valid: " << timeInfo.refFCount << std::endl;
   std::cout << "System num clock reset valid: " << timeInfo.numClockResets << std::endl;

   std::cout << " leapSecondsBeforeChange" << unsigned(locationSystemInfo.info.
       info.leapSecondsBeforeChange) << std::endl;
   std::cout << " leapSecondsAfterChange" << unsigned(locationSystemInfo.info.
       info.leapSecondsAfterChange) << std::endl;
}

void MyLocationListener::setDetailedLocationReportFlag(bool enable) {
   isDetailedReportFlagEnabled_ = enable;
}

void MyLocationListener::setBasicLocationReportFlag(bool enable) {
   isBasicReportFlagEnabled_ = enable;
}

void MyLocationListener::setSvInfoFlag(bool enable) {
   isSvInfoFlagEnabled_ = enable;
}

void MyLocationListener::setDataInfoFlag(bool enable) {
   isDataInfoFlagEnabled_ = enable;
}

void MyLocationListener::setNmeaInfoFlag(bool enable) {
   isNmeaInfoFlagEnabled_ = enable;
}

void MyLocationListener::setEngineNmeaInfoFlag(bool enable) {
   isEngineNmeaInfoFlagEnabled_ = enable;
}

void MyLocationListener::setDetailedEngineLocReportFlag(bool enable) {
   isDetailedEngineReportFlagEnabled_ = enable;
}

void MyLocationListener::setMeasurementsInfoFlag(bool enable) {
   isMeasurementsInfoFlagEnabled_ = enable;
}

void MyLocationListener::setDisasterCrisisInfoFlag(bool enable) {
    isDisasterCrisisInfoFlagEnabled_ = enable;
}

void MyLocationListener::setEphemerisInfoFlag(bool enable) {
    isEphemerisInfoFlagEnabled_ = enable;
}

void MyLocationListener::setLocSystemInfoFlag(bool enable) {
   isLocSysInfoFlagEnabled_ = enable;
}

void MyLocationConfigListener::onXtraStatusUpdate(const telux::loc::XtraStatus xtraStatus) {
    PRINT_NOTIFICATION << "\n********** Xtra Status Info **********" << std::endl;
    std::cout << "Xtra Feature Enabled: " << xtraStatus.featureEnabled << "\n";
    std::cout << "Xtra Feature Validity: " << xtraStatus.xtraValidForHours << "\n";
    LocationUtils::displayXtraStatus(xtraStatus);
    std::cout << "Xtra Feature Consent: " << xtraStatus.userConsent << "\n";
}

void MyLocationConfigListener::onGnssSignalUpdate(const telux::loc::GnssSignal gnssSignalMask){
    PRINT_NOTIFICATION << "\n********** GnssSignalMask Info **********" << std::endl;
    LocationUtils::printGnssSignalType(gnssSignalMask);
}

void MyLocationListener::setRecordingFlag(bool enable) {
   isRecordingEnabled_ = enable;
}

void MyLocationListener::setExtendedInfoFlag(bool enable) {
    isExtendedInfoFlagEnabled_ = enable;
}
