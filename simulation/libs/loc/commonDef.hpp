/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef COMMONDEF_HPP
#define COMMONDEF_HPP
#include "telux/loc/LocationDefines.hpp"

#define  UTC_FIX_TIME_VAL 0
#define  FIX_MODE_VAL 1
#define  ALTITUDE_VAL 832.57983318
#define  LATITUDE_VAL 12.97218703
#define  LONGITUDE_VAL 77.72091665
#define  QTY_SV_IN_VIEW_VAL 10
#define  QTY_SV_USED_VAL 10
#define  GNSS_STATUS_UNAVAILABLE_VAL 1
#define  GNSS_STATUS_APDOPUNDER5_VAL 1
#define  GNSS_STATUS_INVIEWOFUNDER5_VAL 1
#define  GNSS_STATUS_LOCALCORRECTION_PRESENT_VAL 1
#define  GNSS_STATUS_NETWORKCORRECTION_PRESENT_VAL 1
#define  SEMI_MAJOR_AXIS_ACCURACY_VAL 0
#define  SEMI_MINOR_AXIS_ACCURACY_VAL 0
#define  SEMI_MAJOR_AXIS_ORIENTATION_VAL 0
#define  HEADING_VAL 0
#define  VELOCITY_VAL 0
#define  CLIMB_VAL 0
#define  LATERAL_ACCELERATION_VAL 1.008f
#define  LONGITUDINAL_ACCELERATION_VAL 1.007
#define  VEHICLE_VERTICAL_ACCELERATION_VAL 1.004
#define  YAW_RATE_DEGREE_PER_SECOND_VAL 1.003
#define  YAW_RATE_95_PCT_CONFIDENCE_VAL 1.002
#define  LANE_POSITION_NUMBER_VAL 10

#define  LANE_POSITION_95_PCT_CONFIDENCE_VAL 20
#define  TIME_CONFIDENCE_VAL 0.001
#define  HORIZONTAL_CONFIDENCE_VAL 4.3563041
#define  VERTICAL_CONFIDENCE_VAL 3.3582332
#define  HEADING_CONFIDENCE_VAL 0
#define  VELOCITY_CONFIDENCE_VAL 0.4661544
#define  ELEVATION_CONFIDENCE_VAL 0.00
#define  LEAP_SECONDS_VAL 0



#define  ALTITUDE_MEAN_SEA_VAL 908.563964
#define  POSITION_DOP 0.8000000119209
#define  HORIZON_DOP 0.400000000596
#define  VERTICAL_DOP 0.6999999880
#define  GEOMETRIC_DOP 0.899999976
#define  TIME_DOP 0.5
#define  MAGNETIC_DEVIATION -1.1000000238
#define  HORIZONAL_UNCERTAINTY_SEMI_MAJOR 2.8274827
#define  HORIZONAL_UNCERTAINTY_SEMI_MINOR 2.8274827
#define  HORIZONAL_UNCERTAINTY_AZIMUTH 45
#define  EAST_STANDARD_DEVIATION 2.8274827
#define  NORTH_STANDARD_DEVIATION 2.8274827
#define  SV_USED 10
#define  HORIZONTAL_RELIABILITY LocationReliability::MEDIUM
#define  VERTICAL_RELIABILITY LocationReliability::MEDIUM
#define  POSITION_TECHNOLOGY GnssPositionTechType::GNSS_SATELLITE
#define  TIME_UNC 0.005675511
#define  CALIBRATION_CONFIDENCE_PERCENT 25
#define  CALIBRATION_STATUS 0x0f
#define  CONFORMITY_INDEX 0.567523

struct ReportSeqNos {
    uint32_t br = 0;
    uint32_t dr = 0;
    uint32_t der = 0;
};

struct NMEAVals {
    uint64_t nmeaTimestamp;
    std::string nmeaString;
};

#endif
