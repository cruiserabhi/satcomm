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
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       LocationUtils.cpp
 *
 * @brief      Location Utility class
 */

#include <iostream>

#include "LocationUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void LocationUtils::displayCapabilities(telux::loc::LocCapability capabilityMask) {
   PRINT_NOTIFICATION << "\n************* Capabilities Information *************"
                      << std::endl;
  if (capabilityMask & telux::loc::TIME_BASED_TRACKING) {
    std::cout << "Time based tracking" << std::endl;
  }
  if (capabilityMask & telux::loc::DISTANCE_BASED_TRACKING) {
    std::cout << "Distance based tracking" << std::endl;
  }
  if (capabilityMask & telux::loc::GNSS_MEASUREMENTS) {
    std::cout << "GNSS Measurement" << std::endl;
  }
  if (capabilityMask & telux::loc::CONSTELLATION_ENABLEMENT) {
    std::cout << "Constellation enablement" << std::endl;
  }
  if (capabilityMask & telux::loc::CARRIER_PHASE) {
    std::cout << "Carrier phase" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_GNSS_SINGLE_FREQUENCY) {
    std::cout << "QWES GNSS single frequency" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_GNSS_MULTI_FREQUENCY) {
    std::cout << "QWES GNSS multi frequency" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_VPE) {
    std::cout << "QWES VPE" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_CV2X_LOCATION_BASIC) {
    std::cout << "QWES CV2X location basic" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_CV2X_LOCATION_PREMIUM) {
    std::cout << "QWES CV2X location premium" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_PPE) {
    std::cout << "QWES PPE" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_QDR2) {
    std::cout << "QWES QDR2" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_QDR3) {
    std::cout << "QWES QDR3" << std::endl;
  }
  if (capabilityMask & telux::loc::TIME_BASED_BATCHING) {
    std::cout << "TIME BASED BATCHING" << std::endl;
  }
  if (capabilityMask & telux::loc::DISTANCE_BASED_BATCHING) {
    std::cout << "DISTANCE BASED BATCHING" << std::endl;
  }
  if (capabilityMask & telux::loc::GEOFENCE) {
    std::cout << "GEOFENCE" << std::endl;
  }
  if (capabilityMask & telux::loc::OUTDOOR_TRIP_BATCHING) {
    std::cout << "OUTDOOR TRIP BATCHING" << std::endl;
  }
  if (capabilityMask & telux::loc::SV_POLYNOMIAL) {
    std::cout << "SV POLYNOMIAL" << std::endl;
  }
  if (capabilityMask & telux::loc::NLOS_ML20) {
    std::cout << "NLOS ML20" << std::endl;
  }
  std::cout << "*****************************************" << std::endl;
}

void LocationUtils::displayXtraStatus(telux::loc::XtraStatus xtraStatus) {
    std::cout << "Xtra Data Status: ";
    switch(xtraStatus.xtraDataStatus) {
        case telux::loc::XtraDataStatus::STATUS_UNKNOWN :
            std::cout << "Unknown \n";
            break;
        case telux::loc::XtraDataStatus::STATUS_NOT_AVAIL :
            std::cout << "Not available \n";
            break;
        case telux::loc::XtraDataStatus::STATUS_NOT_VALID :
            std::cout << "Invalid \n";
            break;
        case telux::loc::XtraDataStatus::STATUS_VALID :
            std::cout << "Valid \n";
            break;
    }
}

void LocationUtils::printGnssSignalType(telux::loc::GnssSignal signalTypeMask) {
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

void LocationUtils::displayDisasterCrisisReportType(telux::loc::GnssDisasterCrisisReport
    dcReportInfo) {
    std::cout << "Disaster Crisis Report type: ";
    switch(dcReportInfo.dcReportType) {
        case telux::loc::GnssReportDCType::QZSS_JMA_DISASTER_PREVENTION_INFO :
            std::cout << "QZSS_JMA_DISASTER_PREVENTION_INFO \n";
            break;
        case telux::loc::GnssReportDCType::QZSS_NON_JMA_DISASTER_PREVENTION_INFO :
            std::cout << "QZSS_NON_JMA_DISASTER_PREVENTION_INFO \n";
            break;
    }
}

void LocationUtils::displayLocEngineType(telux::loc::LocationAggregationType locEngineType) {
    std::cout << "Location Engine Type: ";
    switch(locEngineType) {
        case telux::loc::LOC_OUTPUT_ENGINE_FUSED : std::cout << "FUSED engine " << std::endl;
                                                   break;
        case telux::loc::LOC_OUTPUT_ENGINE_SPE : std::cout << "SPE engine " << std::endl;
                                                   break;
        case telux::loc::LOC_OUTPUT_ENGINE_PPE : std::cout << "PPE engine " << std::endl;
                                                   break;
        case telux::loc::LOC_OUTPUT_ENGINE_VPE : std::cout << "VPE engine " << std::endl;
                                                   break;
    }
}

void LocationUtils::displayGnssDataSignal(telux::loc::GnssDataSignalTypes dataSourceSignal) {
    std::cout << "\nGnss Data Signal Type: ";
    switch(dataSourceSignal) {
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_INVALID : std::cout << "INVALID\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_GPS_L1CA : std::cout << "GPS_L1CA\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_GPS_L1C : std::cout << "GPS_L1C\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_GPS_L2C_L : std::cout << "GPS_L2C\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_GPS_L5_Q : std::cout << "GPS_L5_Q\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_GLONASS_G1 : std::cout << "GLONASS_G1\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_GLONASS_G2 : std::cout << "GLONASS_G2\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_GALILEO_E1_C : std::cout << "GALILEO_E1_C\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_GALILEO_E5A_Q :
                                                             std::cout << "GALILEO_E5A_Q\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_GALILEO_E5B_Q :
                                                             std::cout << "GALILEO_E5B_Q\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_BEIDOU_B1_I : std::cout << "BEIDOU_B1_I\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_BEIDOU_B1C : std::cout << "BEIDOU_B1C\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2_I : std::cout << "BEIDOU_B2_I\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2A_I : std::cout << "BEIDOU_B2A_I\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_QZSS_L1CA : std::cout << "QZSS_L1CA\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_QZSS_L1S : std::cout << "QZSS_L1S\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_QZSS_L2C_L : std::cout << "QZSS_L2C\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_QZSS_L5_Q : std::cout << "QZSS_L5_Q\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_SBAS_L1_CA : std::cout << "SBAS_L1_CA\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_NAVIC_L5 : std::cout << "NAVIC_L5\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2A_Q : std::cout << "BEIDOU_B2A_Q\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2BI : std::cout << "BEIDOU_B2BI\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2BQ : std::cout << "BEIDOU_B2BQ\n"; break;
        case telux::loc::GNSS_DATA_SIGNAL_TYPE_NAVIC_L1 : std::cout << "NAVIC_L1\n"; break;
        default: std::cout << "Other\n"; break;
    }
}

void LocationUtils::displayBdsSvType(telux::loc::BdsSvType svType) {
    std::cout << "BDS SV Type: ";
    switch(svType) {
        case telux::loc::BDS_SV_TYPE_UNKNOWN: std::cout << "BDS_SV_TYPE_UNKNOWN \n";
                                              break;
        case telux::loc::BDS_SV_TYPE_GEO: std::cout << "BDS_SV_TYPE_GEO \n";
                                              break;
        case telux::loc::BDS_SV_TYPE_IGSO: std::cout << "BDS_SV_TYPE_IGSO \n";
                                              break;
        case telux::loc::BDS_SV_TYPE_MEO: std::cout << "BDS_SV_TYPE_MEO \n";
                                              break;
    }
}

void LocationUtils::displayGpsQzssExtEphValidity(telux::loc::GpsQzssExtEphValidity validityMask) {
    std::cout << "\nGps Qzss Ext Eph Validity fields\n";
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_ISC_L1CA_VALID) {
        std::cout << "Valid ISC_L1CA\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_ISC_L2C_VALID) {
        std::cout << "Valid ISC_L2C\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_ISC_L5I5_VALID) {
        std::cout << "Valid ISC_L5I5\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_ISC_L5Q5_VALID) {
        std::cout << "Valid ISC_L5Q5\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_ALERT_VALID) {
        std::cout << "Valid ALERT\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_URANED0_VALID) {
        std::cout << "Valid URANED0\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_URANED1_VALID) {
        std::cout << "Valid URANED1\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_URANED2_VALID) {
        std::cout << "Valid URANED2\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_TOP_VALID) {
        std::cout << "Valid TOP\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_TOP_CLOCK_VALID) {
        std::cout << "Valid TOP_CLOCK\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_VALIDITY_PERIOD_VALID) {
        std::cout << "Valid VALIDITY_PERIOD\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_DELTA_NDOT_VALID) {
        std::cout << "Valid DELTA_NDOT\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_DELTAA_VALID) {
        std::cout << "Valid DELTAA\n";
    }
    if(validityMask & telux::loc::GPS_QZSS_EXT_EPH_ADOT_VALID) {
        std::cout << "Valid ADOT\n";
    }
}

void LocationUtils::displayBdsExtEphValidity(telux::loc::BdsExtEphValidity validityMask) {
    std::cout << "\nBds Ext Eph Validity fields\n";
    if(validityMask & telux::loc::BDS_EXT_EPH_ISC_B2A_VALID) {
        std::cout << "Valid ISC_B2A\n";
    }
    if(validityMask & telux::loc::BDS_EXT_EPH_ISC_B1C_VALID) {
        std::cout << "Valid ISC_B1C\n";
    }
    if(validityMask & telux::loc::BDS_EXT_EPH_TGD_B2A_VALID) {
        std::cout << "Valid TGD_B2A\n";
    }
    if(validityMask & telux::loc::BDS_EXT_EPH_TGD_B1C_VALID) {
        std::cout << "Valid TGD_B1C\n";
    }
    if(validityMask & telux::loc::BDS_EXT_EPH_SV_TYPE_VALID) {
        std::cout << "Valid SV_TYPE\n";
    }
    if(validityMask & telux::loc::BDS_EXT_EPH_VALIDITY_PERIOD) {
        std::cout << "Valid VALIDITY_PERIOD\n";
    }
    if(validityMask & telux::loc::BDS_EXT_EPH_INTEGRITY_FLAGS) {
        std::cout << "Valid INTEGRITY_FLAGS\n";
    }
    if(validityMask & telux::loc::BDS_EXT_EPH_DELTA_NDOT_VALID) {
        std::cout << "Valid DELTA_NDOT\n";
    }
    if(validityMask & telux::loc::BDS_EXT_EPH_DELTAA_VALID) {
        std::cout << "Valid DELTAA\n";
    }
    if(validityMask & telux::loc::BDS_EXT_EPH_ADOT_VALID) {
        std::cout << "Valid ADOT\n";
    }
}
