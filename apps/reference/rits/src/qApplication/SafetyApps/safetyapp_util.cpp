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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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


 /**
  * @file: safetyapp_util.cpp
  *
  * @brief: utility for safety application
  *
  */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>
#include <stdbool.h>
#include "safetyapp_util.h"
#include <iostream>
/** Maximum number of vehicles including the host that the LDM can support */
double MAX_MAP_SIZE;

/** Maximum number of messages for a sinlg remote vehicle that the LDM can support*/
double MAX_MESSAGES_PER_RV;

/** Time in milli seconds. Messages with in this time from an RV are considered to be recent
 * from that RV*/
double RECENT_TIME_THR;

/** Time in milli seconds. If there is no message from an RV for this time, it will be
 * removed from the LDM */
double REMOVE_TIME_THR;

/** An RV moving in opposite direction with more than this speed (in m/sec) need not be in LDM*/
double REL_SPEED_THR;

/** Radius of the Earth in meters*/
double EARTH_RADIUS;

/** In degrees.If heading difference between two vehicles is greater than this,
 * conclude that they head opposite */
double SAME_DIR_ANG_THR;

/* Thresholds are read from a config file*/

/** Time in millisec. Warn the HV driver if time to crash is less than this */
double MIN_TTC_THR;

/** Time in millisec. Start running safety applications immediately if time to crash
 * is less than this */
double MIN_SAFE_TTC_THR;

/** Speed in m/sec. Consider the vehicle to be stopped if its speed is less than this */
double MOVING_VEH_SPEED_THR = 20;

/** Distance in meters. If perpendicular distance between HV and Rv is less than this,
 * consider they are in same lane */
double SAME_LANE_THR;

/** Distance in meters. If haversine distance between HV and RV is more than this,
 * consider RV is in out of zone of HV */
double IN_ZONE_DIST_THR;

/** Acceleration in meter/sec^2. If difference in acceleration between current and previous (recent)
 * accleration is less than this, consider it is rapidly decelerating */
double RAPID_DECL_THR = 2;

/** Speed in m/sec. If a vehicle is moving less than this speed, perceive that accident might
 * have happened ahead */
double ACC_MIN_SPEED_THR;

/** If these many numbner of vehicles have speed less than @ACC_MIN_SPEED_THR, confirm that an
 * accident happened*/
double ACC_NUM_VEH_THR;

/** Distance in meters. If perpendicular distance between HV and RV is less than this,
 * consider they are out of road */
double OUT_OF_ROAD_THR;

/** File. The file pointer to log the safety warnings */
FILE *warn_logs;

// Read from a config file and initializes thresholds. Each assignment in config file ends with a
// semicolon, comment after #
void init_safety_thr(FILE *fp)
{
    if (fp == NULL)
        return;
    char line[1000];
    int i = 0, j = 0, k = 0, y = 0;
    while (fgets(line, 1000, fp)) {
        i = 0;
        char str[100];
        char num[30];
        //char numstr[50];
        float val = 0;
        bool assign = false;
        j = 0;
        k = 0;
        y = 0;
        while (line[i] != '\0') {
            if (line[i] == '#') {
                break;
            } else if (line[i] == ';') {
                str[j] = '\0';
                num[k] = '\0';
                val = atof(num);
                if (strcmp(str, "MIN_TTC_THR") == 0)
                    MIN_TTC_THR = val;
                if (strcmp(str, "MIN_SAFE_TTC_THR") == 0)
                    MIN_SAFE_TTC_THR = val;
                if (strcmp(str, "MOVING_VEH_SPEED_THR") == 0)
                    MOVING_VEH_SPEED_THR = val;
                if (strcmp(str, "SAME_LANE_THRSAME_LANE_THR") == 0)
                    SAME_LANE_THR = val;
                if (strcmp(str, "IN_ZONE_DIST_THR") == 0)
                    IN_ZONE_DIST_THR = val;
                if (strcmp(str, "RAPID_DECL_THR") == 0)
                    RAPID_DECL_THR = val;
                if (strcmp(str, "ACC_MIN_SPEED_THR") == 0)
                    ACC_MIN_SPEED_THR = val;
                if (strcmp(str, "ACC_NUM_VEH_THR") == 0)
                    ACC_NUM_VEH_THR = val;

                if (strcmp(str, "LOG_WARNINGS") == 0) {
                    fprintf(warn_logs,
                        "Sys_time,hv_timestamp_ms,hv_Msgcnt,rv_timestamp_ms,rv_Msgcnt,Warning_type\n");
                }

                break;
            } else if (line[i] == ' ') {

            } else if (line[i] == '=') {
                assign = true;
            } else if ((isalpha(line[i]) || line[i] == '_') && !assign) {
                str[j] = line[i];
                j++;

            } else if (isdigit(line[i]) || line[i] == 'x' || line[i] == '.') {
                if (assign) {
                    num[k] = line[i];
                    k++;
                } else {
                    str[j] = line[i];
                    j++;
                }
            } else {

            }
            i++;
        }
    }
    fclose(fp);

}

/* Compared to the past acceleration, has the vehicle's accleration rapidly gone down?*/
bool rapid_decl(msg_contents *mc)
{
    bsm_value_t *bsm = (bsm_value_t *)mc->j2735_msg;
    if (bsm->AccelLon_cm_per_sec_squared * 0.01 <= RAPID_DECL_THR)
        return true;
    else {
        return false;
    }
}

// Haversine distance.
double calc_distance(double lat1, double lon1, double lat2, double lon2)
{
    double lb1, lb2, shi1, shi2, dshi, dlb;
    shi1 = ((lat1)*1 / 10000000) * M_PI / 180;
    shi2 = ((lat2)*1 / 10000000) * M_PI / 180;
    dshi = shi1 - shi2;
    dlb = ((lon1)-(lon2)) * 1 / 10000000 * M_PI / 180;
    double a = sin(dshi / 2) * sin(dshi / 2) + cos(shi1) * cos(shi2) * sin(dlb / 2) * sin(dlb / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return c * (EARTH_RADIUS);
}

/* Angle conversion based on the quadrant*/
double quadrant_based_angle(double x1, double y1, double x2, double y2, double theta)
{

    double sx = x2 - x1;
    double sy = y2 - y1;

    // Quadrant-1
    if (sx >= 0 && sy >= 0) {
        return theta;
    }
    // Quadrant-2
    else if (sx <= 0 && sy >= 0) {
        return 180 - theta;
    }
    // Quadrant-3
    else if (sx <= 0 && sy <= 0) {
        return 180 + theta;
    }
    // Quadrant-4
    else {
        return 360 - theta;
    }
}

/* 3-D Euclidean distance */
double calc_3D_distance(double lat1, double lon1, double alt1, double lat2, double lon2, double alt2)
{
    lat1 = lat1 * 1 / 10000000 * M_PI / 180;
    lon1 = lon1 * 1 / 10000000 * M_PI / 180;
    alt1 = alt1 / 10 + EARTH_RADIUS;
    lat2 = lat2 * 1 / 10000000 * M_PI / 180;
    lon2 = lon2 * 1 / 10000000 * M_PI / 180;
    alt2 = alt2 / 10 + EARTH_RADIUS;
    int x1 = alt1 * cos(lat1) * sin(lon1);
    int y1 = alt1 * sin(lat1);
    int z1 = alt1 * cos(lat1) * cos(lon1);
    int x2 = alt2 * cos(lat2) * sin(lon2);
    int y2 = alt2 * sin(lat2);
    int z2 = alt2 * cos(lat2) * cos(lon2);
    double dist = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
    return dist;
}

/* Is RV out of zone of HV ? */
bool out_of_zone(msg_contents *host, msg_contents *remote)
{
    bsm_value_t *host_bsm = (bsm_value_t *)host->j2735_msg;
    bsm_value_t *remote_bsm = (bsm_value_t *)remote->j2735_msg;
    int lat1 = host_bsm->Latitude;
    int lon1 = host_bsm->Longitude;
    int alt1 = host_bsm->Elevation;
    int lat2 = remote_bsm->Latitude;
    int lon2 = remote_bsm->Longitude;
    int alt2 = remote_bsm->Elevation;
    double dist = calc_3D_distance(lat1, lon1, alt1, lat2, lon2, alt2);
    if (dist > IN_ZONE_DIST_THR)
        return true;
    return false;
}

/* Classify the lane of RV and direction of motion of RV relative to HV .
 * Ref: https://ieeexplore.ieee.org/document/7850093/ */
lane_types classify_lane(msg_contents *host, msg_contents *remote)
{
    bsm_value_t *host_bsm = (bsm_value_t *)host->j2735_msg;
    bsm_value_t *remote_bsm = (bsm_value_t *)remote->j2735_msg;
    int qty_crumbs_hv = host_bsm->ph.qty_crumbs - 1;
    int qty_crumbs_rv = remote_bsm->ph.qty_crumbs - 1;
    double lat_h_t1 = 0;
    double lon_h_t1 = 0;
    double lat_r_t1 = 0;
    double lon_r_t1 = 0;
    if (qty_crumbs_hv >= 0 && qty_crumbs_rv >= 0) {
        lat_h_t1 = host_bsm->Latitude - (host_bsm->ph.ph_crumb[qty_crumbs_hv]).latOffset;
        lon_h_t1 = host_bsm->Longitude - (host_bsm->ph.ph_crumb[qty_crumbs_hv]).lonOffset;
        lat_r_t1 = remote_bsm->Latitude - (remote_bsm->ph.ph_crumb[qty_crumbs_rv]).latOffset;
        lon_r_t1 = remote_bsm->Longitude - (remote_bsm->ph.ph_crumb[qty_crumbs_rv]).lonOffset;
    }
    //printf("%f\t%f\t%f\t%f\n", lat_h_t1, lon_h_t1, lat_r_t1, lon_r_t1);

    double lat_h_t2 = host_bsm->Latitude;
    double lon_h_t2 = host_bsm->Longitude;
    double lat_r_t2 = remote_bsm->Latitude;
    double lon_r_t2 = remote_bsm->Longitude;

    //printf("%f\t%f\t%f\t%f\n", lat_h_t2, lon_h_t2, lat_r_t2, lon_r_t2);
    double eps = 0.00001;
    double del_H = calc_distance(lat_h_t2, lon_h_t2, lat_h_t1, lon_h_t1);
    double del_H_adj = calc_distance(lat_h_t1, lon_h_t1, lat_h_t1, lon_h_t2);
    double theta_H_s = acos((del_H_adj + eps) / (del_H + eps)) * 180 / M_PI;
    double theta_H = quadrant_based_angle(lon_h_t1, lat_h_t1, lon_h_t2, lat_h_t2, theta_H_s);

    //printf("%f\t%f\t%f\t%f\n", del_H, del_H_adj, theta_H_s, theta_H);

    double del_R = calc_distance(lat_r_t2, lon_r_t2, lat_r_t1, lon_r_t1);
    double del_R_adj = calc_distance(lat_r_t1, lon_r_t1, lat_r_t1, lon_r_t2);
    double theta_R_s = acos((del_R_adj + eps) / (del_R + eps)) * 180 / M_PI;
    double theta_R = quadrant_based_angle(lon_r_t1, lat_r_t1, lon_r_t2, lat_r_t2, theta_R_s);

    //printf("%f\t%f\t%f\t%f\n", del_R, del_R_adj, theta_R_s, theta_R);

    double d_RH = calc_distance(lat_h_t1, lon_h_t1, lat_r_t1, lon_r_t1);
    double d_RH_adj = calc_distance(lat_r_t1, lon_r_t1, lat_r_t1, lon_h_t1);
    double beta_s = acos((d_RH_adj + eps) / (d_RH + eps)) * 180 / M_PI;
    double beta = quadrant_based_angle(lon_r_t1, lat_r_t1, lon_h_t1, lat_h_t1, beta_s);

    //printf("%f\t%f\t%f\t%f\n", d_RH, d_RH_adj, beta_s, beta);

    double alpha_H = theta_H - beta;
    double alpha_R = theta_R - beta;
    double d_H = d_RH * abs(sin(alpha_R));

    //printf("%f\t%f\t%f\n", alpha_H, alpha_R, d_H);

    if (d_H > OUT_OF_ROAD_THR) {
        return OUT_OF_ROAD;
    }

    double heading_diff = abs((int)(host_bsm->Heading_degrees) - (int)(remote_bsm->Heading_degrees)) * 0.0125;
    if (heading_diff > 180)
        heading_diff = 360 - heading_diff;
    //printf("host_bsm->Heading_degrees: %d\nremote_bsm->Heading_degrees: %d\nheading_diff: %f\n");
    //printf("SAME_DIR_ANG_THR:%f\n", host_bsm->Heading_degrees,remote_bsm->Heading_degrees,
    //    heading_diff, SAME_DIR_ANG_THR);
    if (alpha_H < 0)
        alpha_H = 360 + alpha_H;
    //printf("%f\n", alpha_H);
    if (270 <= alpha_H ||  alpha_H <= 90) {
        // Indicates RV is behind HV
        if (heading_diff <= SAME_DIR_ANG_THR) {
            // Indicates same dir
            if (d_H <= SAME_LANE_THR)
                return SAME_LANE_BACK_SAMEDIR;
            else if (180 >= alpha_H && alpha_H >= 0)
                return ADJRIGHT_LANE_BACK_SAMEDIR;
            else
                return ADJLEFT_LANE_BACK_SAMEDIR;
        } else {
            // Indicates opp dir
            if (d_H <= SAME_LANE_THR)
                return SAME_LANE_BACK_OPPDIR;
            else if (180 >= alpha_H && alpha_H >= 0)
                return ADJRIGHT_LANE_BACK_OPPDIR;
            else
                return ADJLEFT_LANE_BACK_OPPDIR;
        }

    } else {

        // Indicates RV is ahead of HV
        if (heading_diff <= SAME_DIR_ANG_THR) {
            // Indicates same dir
            if (d_H <= SAME_LANE_THR)
                return SAME_LANE_AHEAD_SAMEDIR;
            else if (180 >= alpha_H && alpha_H >= 0)
                return ADJRIGHT_LANE_AHEAD_SAMEDIR;
            else
                return ADJLEFT_LANE_AHEAD_SAMEDIR;
        } else {
            // Indicates opp dir
            if (d_H <= SAME_LANE_THR)
                return SAME_LANE_AHEAD_OPPDIR;
            else if (180 >= alpha_H && alpha_H >= 0)
                return ADJRIGHT_LANE_AHEAD_OPPDIR;
            else
                return ADJLEFT_LANE_AHEAD_OPPDIR;
        }

    }
    return static_cast<lane_types>(13);
}

/*
 * Differs from basic verison. Also checks if the host would approximately travel in the
 * same dir of RV
 */
double time_to_crash_adv(msg_contents *host, msg_contents *remote, double distFromRV)
{
    bsm_value_t *host_bsm = (bsm_value_t *)host->j2735_msg;
    bsm_value_t *remote_bsm = (bsm_value_t *)remote->j2735_msg;
    double lat_hv = host_bsm->Latitude;
    double lat_rv = remote_bsm->Longitude;
    double lon_hv = host_bsm->Longitude;
    double lon_rv = remote_bsm->Longitude;
    double dlon = lon_rv - lon_hv;
    double dlat = lat_rv - lat_hv;
    double heading = host_bsm->Heading_degrees * 0.0125;
    bool match = false;
    if (heading >= 0 && heading <= 90) {
        //First quadrant
        if (dlat >= 0 && dlon >= 0)
            match = true;
    } else if (heading > 90 && heading <= 180) {
        if (dlat <= 0 && dlon >= 0)
            match = true;
    } else if (heading > 180 && heading <= 270) {
        if (dlat <= 0 && dlon <= 0)
            match = true;
    } else if (heading > 270 && heading <= 360) {
        if (dlat >= 0 && dlon <= 0)
            match = true;
    }
    if (!match || host_bsm->Speed < remote_bsm->Speed) {
        return 10002;
    }

    // use provided distance (distance from RV should not be zero)
    double speed_diff = (host_bsm->Speed - remote_bsm->Speed) * 0.02;
    if (speed_diff == 0)
        return 10000;
    return distFromRV / (speed_diff);
}


// host in the back and remote in the front
double time_to_crash(msg_contents *host, msg_contents *remote, double distFromRV)
{
    bsm_value_t *host_bsm = (bsm_value_t *)host->j2735_msg;
    bsm_value_t *remote_bsm = (bsm_value_t *)remote->j2735_msg;
    if (host_bsm->Heading_degrees != 28800)
        return time_to_crash_adv(host, remote, distFromRV);
    else {

        if (host_bsm->Speed < remote_bsm->Speed) {
            return 10000;
        }
        double speed_diff = (host_bsm->Speed - remote_bsm->Speed) * 0.02;
        if (speed_diff == 0)
            return 0;
        return distFromRV / (speed_diff);
    }
}


/*******************************************************************************
 * return current time stamp in microseconds
 * @return long long
 ******************************************************************************/
static __inline uint64_t ntimestamp_now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000LL + (uint64_t)tv.tv_usec;
}

/* From the received secmark, calculate the timestamp*/
void calc_rv_timestamp(uint64_t time_now, msg_contents *rv)
{
    bsm_value_t *rv_bsm = (bsm_value_t *)rv->j2735_msg;

    int rem = time_now % 60000;
    if (rv_bsm->secMark_ms < rem) {
        rv_bsm->timestamp_ms = time_now / 60000 + rv_bsm->secMark_ms;
    } else {
        rv_bsm->timestamp_ms = (time_now / 60000 - 1) + rv_bsm->secMark_ms;
    }
}

/* Extrapolate rv contents to the current time*/
void extrapolate(msg_contents *hv, msg_contents *rv)
{
    bsm_value_t *hv_bsm = (bsm_value_t *)hv->j2735_msg;
    bsm_value_t *rv_bsm = (bsm_value_t *)rv->j2735_msg;

    if (rv_bsm->timestamp_ms == 0)
        return;
    uint64_t time_now = hv_bsm->timestamp_ms; //ntimestamp_now()/1000;
    calc_rv_timestamp(time_now, rv);
    double time_gap = time_now - rv_bsm->timestamp_ms;
    time_gap = time_gap / 1000;
    //printf("%f\n", time_gap);
    if (rv_bsm->Speed == 8191)
        return;
    double u = rv_bsm->Speed;
    u = u * 0.02;

    double longaccl = rv_bsm->AccelLon_cm_per_sec_squared;
    if (rv_bsm->AccelLon_cm_per_sec_squared == 2001)
        longaccl = 0;
    longaccl = longaccl * 0.01;

    double dist = (u * time_gap) + (0.5) * (longaccl)*(time_gap)*(time_gap);
    double coef = dist * 0.0000089;

    double lat = (rv_bsm->Latitude);
    lat = lat * 1 / 10000000;
    double lon = (rv_bsm->Longitude);
    lon = lon * 1 / 10000000;
    //printf("%"PRIu64"\t%d\t%d\t%d\n", rv_bsm->timestamp_ms,rv_bsm->Speed,rv_bsm->Latitude,
    //    rv_bsm->Longitude);
    rv_bsm->Speed = (u + (longaccl)*time_gap) * 50;
    rv_bsm->Latitude = (lat - (coef)) * 10000000;
    rv_bsm->Longitude = (lon + (coef) / cos(lat * M_PI / 180)) * 10000000;
    //printf("%"PRIu64"\t%d\t%d\t%d\n", rv_bsm->timestamp_ms,rv_bsm->Speed,rv_bsm->Latitude,
    //    rv_bsm->Longitude);

}

// Print the items in rv_specs
void print_rvspecs(rv_specs *rv)
{
    printf("Is it out of Zone : %d\n", rv->out_of_zone);
    printf("TTC : %f\n", rv->ttc);
    printf("Distance from RV : %f\n", rv->distFromRV);
    printf("Lane types : %d\n", rv->lt);
    printf("Rapid Decl : %d\n", rv->rapid_decl);
    printf("Stopped : %d\n", rv->stopped);
    printf("msg cnt: %d\n" ,rv->hv_msgcnt);
    printf("timestamp ms: %lu\n" ,rv->hv_timestamp_ms);
    printf("last msg cnt: %d\n" ,rv->lastCnt);
    printf("last timestamp ms: %lu\n" ,rv->lastTime);
    printf("total rx cnt: %d\n" ,rv->totalCnt);
    printf("last saved total rx cnt: %d\n" ,rv->lastTotalCnt);
    printf("count difference (between last and current): %d\n" ,rv->cntDiff);
    printf("estimated msg rate of this rv: %f\n", rv->msgRate);
}
void updateRVMsgCount(bsm_value_t *remote_bsm, rv_specs *rvsp){
    //std::cout << "Updating this rvsp with new msg count and time contents\n";
    rvsp->lastCnt = rvsp->rv_msgcnt;
    // first time seeing this L2 address, then save the timestamp for msg rate calculation
    if(rvsp->lastTime == 0){
        rvsp->lastTime = rvsp->rv_timestamp_ms;
    }
    if(remote_bsm->MsgCount < rvsp->lastCnt){ // assume it rolled over
        rvsp->cntDiff = (remote_bsm->MsgCount + 128) - rvsp->lastCnt;
    }else{
        rvsp->cntDiff = remote_bsm->MsgCount - rvsp->lastCnt;
    }
    rvsp->totalCnt++;

    rvsp->rv_msgcnt = remote_bsm->MsgCount;
    // calculate msg / sec rate for this node here?
    
}

/* Fill the rv-specs object to be used by the safety applications*/
void fill_RV_specs(msg_contents *host, msg_contents *remote, rv_specs *rvsp)
{
    if (remote == NULL || host == NULL)
        return;
    bsm_value_t *host_bsm = (bsm_value_t *)host->j2735_msg;
    bsm_value_t *remote_bsm = (bsm_value_t *)remote->j2735_msg;

    rvsp->rapid_decl = rapid_decl(remote);
    // use provided distance unless not provided (distance from RV should not be zero)
    if(rvsp->distFromRV == 0.0){
        rvsp->distFromRV = calc_distance(host_bsm->Latitude, host_bsm->Longitude,
        remote_bsm->Latitude, remote_bsm->Longitude);
    }
    rvsp->ttc = time_to_crash(host, remote, rvsp->distFromRV);
    rvsp->out_of_zone = out_of_zone(host, remote);
    rvsp->lt = classify_lane(host, remote);
    if (remote_bsm->Speed < MOVING_VEH_SPEED_THR)
        rvsp->stopped = true;
    else
        rvsp->stopped = false;

    if (remote_bsm->events.bits.eventAirBagDeployment)
        rvsp->airbag = true;
    else
        rvsp->airbag = false;
    rvsp->hv_timestamp_ms = host_bsm->timestamp_ms;
    rvsp->hv_msgcnt = remote_bsm->MsgCount;
    updateRVMsgCount(remote_bsm, rvsp);
}

/*
 * Forward collision warning safety app. Writes safety messages to log file defined in config,
 * and also prints to console.
 */
void forward_collision_warning(msg_contents *remote, rv_specs *rvsp)
{
    //print_rvspecs(rvsp);
    if (rvsp->out_of_zone) {
        return;
    }

    if (rvsp->stopped || rvsp->rapid_decl) {

        if (rvsp->lt == SAME_LANE_AHEAD_SAMEDIR) {
            double t = rvsp->ttc;
            if (t > 0 && t < MIN_SAFE_TTC_THR) {
                printf("FCW Warning\n");
                //assert(1==-1);
                uint64_t millis = ntimestamp_now() / 1000;
                //fprintf(warn_logs, "%"PRIu64",%"PRIu64",%d,%"PRIu64",%d,FCW\n",
                //    millis, rvsp->hv_timestamp_ms, rvsp->hv_msgcnt, remote_bsm->timestamp_ms,
                //    remote_bsm->MsgCount);
                //warn_hmi();
                //system("mosquitto_pub -h 10.71.104.47 -p 1883 -t test -f /home/root/fcw.txt");
            }
        }

    }

}

/*
 * EEBL warning safety app. Writes safety messages to log file defined in config,
 * and also prints to console.
 */
void EEBL_warning(msg_contents *remote, rv_specs *rvsp)
{
    bsm_value_t *remote_bsm = (bsm_value_t *)remote->j2735_msg;
    lane_types lt = rvsp->lt;
    //print_rvspecs(rvsp);
    double t = rvsp->ttc;

    if (rvsp->out_of_zone) {
        return;
    }

    if (lt == SAME_LANE_AHEAD_SAMEDIR || lt == ADJLEFT_LANE_AHEAD_SAMEDIR ||
            lt == ADJRIGHT_LANE_AHEAD_SAMEDIR) {

        if (rvsp->rapid_decl) {
            if (t > 0 && t < MIN_SAFE_TTC_THR) {
                printf("EEBL Warning\n");
                uint64_t millis = ntimestamp_now() / 1000;
                //fprintf(warn_logs, "%"PRIu64",%"PRIu64",%d,%"PRIu64",%d,EEBL\n",
                //    millis, rvsp->hv_timestamp_ms, rvsp->hv_msgcnt, remote_bsm->timestamp_ms,
                //    remote_bsm->MsgCount);
                //system("mosquitto_pub -h 10.71.104.47 -p 1883 -t test -f /home/root/eebl.txt");
            }
        }

        if (remote_bsm->brakes.word & 0x0018 == 2 || remote_bsm->events.bits.eventHardBraking ||
                remote_bsm->events.bits.eventABSactivated) {
            if (t > 0 && t < MIN_SAFE_TTC_THR) {
                uint64_t millis = ntimestamp_now() / 1000;
                printf("EEBL Warning\n");
                //fprintf(warn_logs, "%"PRIu64",%"PRIu64",%d,%"PRIu64",%d,EEBL\n",
                //    millis, rvsp->hv_timestamp_ms, rvsp->hv_msgcnt,
                //    remote_bsm->timestamp_ms, remote_bsm->MsgCount);
                //system("mosquitto_pub -h 10.71.104.47 -p 1883 -t test -f /home/root/fcw.txt");
            }
        }

    }


}

/*
 * Accident Ahead warning safety app. Writes safety messages to log file defined in config,
 * and also prints to console.
 */
void accident_ahead_warning(msg_contents *remote, rv_specs *rvsp)
{
    bsm_value_t *remote_bsm = (bsm_value_t *)remote->j2735_msg;
    if (rvsp->airbag || remote_bsm->events.bits.eventHazardLights ||
            remote_bsm->lights_in_use.bits.hazardSignalOn) {
        if (rvsp->ttc < MIN_SAFE_TTC_THR) {
            printf("Accident Ahead Warning\n");
            uint64_t millis = ntimestamp_now() / 1000;
            //fprintf(warn_logs, "%"PRIu64",%"PRIu64",%d,%"PRIu64",%d,AccidentAhead\n",
            //   millis, rvsp->hv_timestamp_ms, rvsp->hv_msgcnt, remote_bsm->timestamp_ms,
            //   remote_bsm->MsgCount);
        }
    }
}

/*
 * Has to be called from a reference app. If more no of vehicles with very low speed are observed,
 * call this
 */
void congestion_ahead_warning()
{
    printf("Congestion Ahead Warning\n");
}
