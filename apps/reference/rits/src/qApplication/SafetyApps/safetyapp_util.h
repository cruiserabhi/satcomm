/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
    @file safetyapp_util.h
    This file contains the function declarations required for implementing safety applications.
    To add a safety application, it is as simple as just adding a new function here.
 */

#ifndef _SAFETYAPP_UTIL_H_
#define _SAFETYAPP_UTIL_H_


#include <stdio.h>
#include <string.h>
#include "v2x_codec.h"
#ifdef __cplusplus
extern "C"{
#endif
/** This is the lane types enum */
typedef enum {
    SAME_LANE_AHEAD_SAMEDIR = 1,    /**< Remote Vehicle is in Same lane ahead of HV and moving in the same direction. */
    ADJLEFT_LANE_AHEAD_SAMEDIR = 2, /**< Remote Vehicle is in Adjacent Left lane ahead of HV and moving in the same direction. */
    ADJRIGHT_LANE_AHEAD_SAMEDIR = 3, /**< Remote Vehicle is in Adjacent right lane ahead of HV and moving in the same direction.*/
    SAME_LANE_BACK_SAMEDIR = 4,     /**< Remote Vehicle is in Same lane back of HV and moving in the same direction. */
    ADJLEFT_LANE_BACK_SAMEDIR = 5,  /**< Remote Vehicle is in Adjacent left lane back of HV and moving in the same direction. */
    ADJRIGHT_LANE_BACK_SAMEDIR = 6, /**< Remote Vehicle is in Adjacent Right lane back of HV and moving in the same direction. */
    SAME_LANE_AHEAD_OPPDIR = 7,     /**< Remote Vehicle is in Same lane ahead of HV and moving in the opposite direction. */
    ADJLEFT_LANE_AHEAD_OPPDIR = 8,  /**< Remote Vehicle is in Adjacent Left lane ahead of HV and moving in the opposite direction. */
    ADJRIGHT_LANE_AHEAD_OPPDIR = 9, /**< Remote Vehicle is in Adjacent Right lane ahead of HV and moving in the opposite direction. */
    SAME_LANE_BACK_OPPDIR = 10,     /**< Remote Vehicle is in same lane back of HV and moving in the opposite direction. */
    ADJLEFT_LANE_BACK_OPPDIR = 11,  /**< Remote Vehicle is in Adjacent Left lane back of HV and moving in the opposite direction. */
    ADJRIGHT_LANE_BACK_OPPDIR = 12, /**< Remote Vehicle is in Adjacent Right lane back of HV and moving in the opposite direction. */
    OUT_OF_ROAD = 13,               /**< Remote vehicle is not near by host. */
} lane_types;

/** \struct rv_specs
 * A structure that will be passed to the safety applications.
 * The main aim of this structure is to eliminate the need for computing the same information again by each safety app.
 * This structure is filled once per RV before running any safety app and all safety apps use that information.
 */
typedef struct {
    bool out_of_zone;   /**< Is RV out of zone of HV? */
    double ttc;         /**< HV Time to crash RV*/
    double distFromRV;  /**< Distance between HV and RV*/
    lane_types lt;      /**< @lane_types Lane of RV relative to HV */
    bool rapid_decl;    /**< Is RV rapidly decelerating ?*/
    bool stopped;       /**< Is RV stopped ?*/
    bool airbag;        /**< Are airbags open in RV */
    uint64_t hv_timestamp_ms;   /**< Recent timestamp on bsm of the host, Useful for logging warnings*/
    int hv_msgcnt;     /**< Message count of the recent bsm of the host, Useful for logging warnings*/
    uint64_t rv_timestamp_ms;   /**< Recent timestamp on bsm of the rv*/
    int rv_msgcnt;     /**< Message count of the recent bsm of the rv*/
    int totalCnt;      /**< Total number of messsages with different msg cnts received by the host*/
    int lastTotalCnt;  /**< Used to calculate msg rate*/
    uint64_t lastTime; /**< Used to calculate msg rate*/
    int cntDiff;       /**< Total difference of messsages count between current and last msg received by the host*/
    double msgRate;    /**< Message rate based on totalCnt over last interval of 1000ms*/
    int lastCnt;       /**< Last Message count of the recent bsm of the host, Useful for logging warnings*/
} rv_specs;

/** @brief Initializes all the thresholds defined earlier by reading from the config file provided.
 *  Must do as a first step before executing Safety applications.
 *  @param[in] fp , File pointer to the config file.
 */
void init_safety_thr(FILE *fp);

/** @brief Calculates the time to crash for HV and RV based on distance and relative velocity between them.
 *  @param[in] Host pointer to msg_contents of the host. Contains all the information about the host.
 *  @param[in] Remote pointer to msg_contents of the remote. Contains all the information about the remote.
 *  @param[in] Distance from RV.
 *  @return double , time to crash in sec
 */
double time_to_crash(msg_contents *host, msg_contents *remote, double distFromRV);

/** @brief Extrapolates the RV coordinates and speed at its timestamp to the timestamp of the host.
 *  @param[in] Host pointer to msg_contents of the host. Contains all the information about the host.
 *  @param[in,out] Remote pointer to msg_contents of the remote. This will be modified with extrapolated contents.
 *  @return void
 */
void extrapolate(msg_contents *hv, msg_contents *rv);

void updateRVMsgCount(bsm_value_t *remote_bsm, rv_specs *rvsp);

/** @brief Extrapolates the RV coordinates and speed at its timestamp to the timestamp of the host.
 *  @param[in] Host pointer to msg_contents of the host. Contains all the information about the host.
 *  @param[in] Remote pointer to msg_contents of the remote. Contains all the information about the remote.
 *  @param[in] Map The pointer to the array of @ldm structs.
 *  @param[in] Ldm_size The size of the LDM meaning the number of Rvs it was initialized to.
 *  @param[out] rvsp pointer to rv_specs structure. This gets populated.
 *  @return void
 */
void fill_RV_specs(msg_contents *host, msg_contents *remote, rv_specs *rvsp);

/** @brief Alerts the HV driver if there is an accident that happened to a vehicle ahead.
 *  Prints the warning on the console and also writes to the log file sepcified in the config file.
 *  @param[in] Remote pointer to msg_contents of the remote. Contains all the information about the remote.
 *  @param[in] rvsp pointer to rv_specs structure. The populated pointer will be passed.
 *  @return void.
 */

void accident_ahead_warning(msg_contents *remote, rv_specs *rvsp);

/** @brief alerts the HV driver if there is a congestion ahead.
 *  If number of vehicles with a very low speed is greater than the @ACC_NUM_VEH_THR, then prints the warning on the console.
 *  @return void
 */

void congestion_ahead_warning();

/** @brief alerts the HV driver if there is an EEBl event that happened to a vehicle ahead.
 *  Prints the warning on the console and also writes to the log file sepcified in the config file.
 *  @param[in] remote pointer to msg_contents of the remote. Contains all the information about the remote.
 *  @param[in] rvsp pointer to rv_specs structure. The populated pointer will be passed.
 *  @return void
 */

void EEBL_warning(msg_contents *remote, rv_specs *rvsp);

/** @brief alerts the HV driver if there is a predicted forward crash event that might occur with a vehicle ahead.
 *  Prints the warning on the console and also writes to the log file sepcified in the config file.
 *  @param[in] remote pointer to msg_contents of the remote. Contains all the information about the remote.
 *  @param[in] rvsp pointer to rv_specs structure. The populated pointer will be passed.
 *  @return void
 */

void forward_collision_warning(msg_contents *remote, rv_specs *rvsp);
void print_rvspecs(rv_specs* rv);;

#ifdef __cplusplus
}
#endif
#endif // #ifndef _SAFETYAPP_UTIL_H_
