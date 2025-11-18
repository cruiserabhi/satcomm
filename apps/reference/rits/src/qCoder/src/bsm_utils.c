/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * @file bsm_utils.c
 * @purpose some BSM utilities.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "v2x_msg.h"
#include "v2x_codec.h"

#define STR_MAX_LEN 50
static char *event_str[] = {"No", "Yes"};
static char *brake_str0[] = {"Unavailable", "Off", "On", "Reserved"};
static char *brake_str1[] = {"Unavailable", "Off", "On", ""};
static char *brake_str2[] = {"Unavailable", "Off", "On", "Engaged"};

const char* get_wall_time(char* result)
{
    time_t now;
    struct tm * time_info;
    char *token;

    if (!result) {
        return NULL;
    }

    time(&now);
    time_info = localtime(&now);
    g_strlcpy(result, asctime(time_info), 100);
    /*If it is end with  \n character, find and remove it*/
    token = strchr(result, '\n');
    if (token) {
        token[0] = '\0';
    }
    return result;
}

double get_CPU_percentage(uint64_t monotonicTime)
{
 #if 0
    static uint64_t last_monotonicTime = 0;
    static double percent = 0.0;
    FILE* file;

    /*calculating this cost resource, do not perform the calcluation if within 3 secs*/
    if (monotonicTime - last_monotonicTime >= 3000000) {
        file = fopen("/proc/loadavg", "r");
        if(!file){
            printf("%s file pointer may be null",__FUNCTION__);
            return percent;
        }
        fscanf(file, "%lf", &percent);
        fclose(file);
        last_monotonicTime = monotonicTime;
    }
#endif
    static double percent = 0.0;
    return percent;
}

// Fills the bsm with all unvailable codes.
void bsm_init(bsm_value_t *bsm)
{
    bsm->timestamp_ms = 0;
    bsm->id = 3234398499;
    bsm->MsgCount = 127;
    bsm->secMark_ms = 65535;
    bsm->Latitude = 900000001;
    bsm->Longitude = 1800000001;
    bsm->Elevation = -4096;
    bsm->SemiMajorAxisAccuracy = 255;
    bsm->SemiMinorAxisAccuracy = 255;
    bsm->SemiMajorAxisOrientation = 65535;
    bsm->Speed = 8191;
    bsm->Heading_degrees = 28800;
    bsm->TransmissionState = 7;
    bsm->SteeringWheelAngle = 127;
    bsm->AccelLon_cm_per_sec_squared = 2001;
    bsm->AccelLat_cm_per_sec_squared = 2001;
    bsm->AccelVert_two_centi_gs = -127;
    bsm->AccelYaw_centi_degrees_per_sec = 32767;
    bsm->brakes.word = (1 << 15);
    bsm->VehicleWidth_cm = 0;
    bsm->VehicleLength_cm = 0;
}
// Print the summary of the bsm contents of the remote vehicle.
void print_bsm_summary_RV(msg_contents *mc)
{
    printf("\n---------\n");
    bsm_value_t *bs = mc->j2735_msg;
    printf("\nsender - %04x msg: %d speed: %f m/sec (lat:%f,lon:%f) width: %d, length: %d\n",
        bs->id, bs->MsgCount, bs->Speed/50.0, bs->Latitude / 10000000.0, bs->Longitude / 10000000.0,
        bs->VehicleWidth_cm, bs->VehicleLength_cm);

    if (bs->brakes.bits.antilock_brake_status)
        printf("Antilock brakes failed\n");
    if (bs->brakes.bits.brake_boost_applied)
        printf("brake boost applied\n");
    if (bs->brakes.bits.stability_control_status)
        printf("Stability control lost\n");
    if (bs->brakes.bits.traction_control_status)
        printf("Traction control lost\n");

    if (bs->events.bits.eventHazardLights) {
        printf("Event Hazard Lights ON\n");
    }

    if (bs->events.bits.eventABSactivated) {
        printf("Event ABS activated\n");
    }

    if (bs->events.bits.eventTractionControlLoss) {
        printf("Event traction control loss\n");
    }

    if (bs->events.bits.eventStabilityControlactivated) {
        printf("Event stability control loss\n");
    }

    if (bs->events.bits.eventHardBraking) {
        printf("Event Hard Barking\n");
    }

    if (bs->events.bits.eventWipersChanged) {
        printf("Event Front Wipers\n");
    }

    if (bs->events.bits.eventAirBagDeployment) {
        printf("Event Airbag Deployment\n");
    }

    //Exterior Lights

    if (bs->lights_in_use.bits.lowBeamHeadlightsOn) {
        printf("Low beam status on\n");
    }

    if (bs->lights_in_use.bits.highBeamHeadlightsOn) {
        printf("High beam status on\n");
    }

    if (bs->lights_in_use.bits.leftTurnSignalOn) {
        printf("Left turn signal On\n");
    }

    if (bs->lights_in_use.bits.rightTurnSignalOn) {
        printf("Right turn signal on\n");
    }

    if (bs->lights_in_use.bits.hazardSignalOn) {
        printf("hazard lights on\n");
    }

    if (bs->lights_in_use.bits.automaticLightControlOn) {
        printf("Auto light control on\n");
    }

    if (bs->lights_in_use.bits.daytimeRunningLightsOn) {
        printf("Day time lights on\n");
    }

    if (bs->lights_in_use.bits.fogLightOn) {
        printf("Fog Lights on\n");
    }

    if (bs->lights_in_use.bits.parkingLightsOn) {
        printf("PkgLghtsOn\n");
    }
    printf("\n---------\n");

}
// Public api. Print the summary of the bsm contents of the remote vehicle.
void print_summary_RV(msg_contents *mc)
{
    if (mc->msgId == J2735_MSGID_BASIC_SAFETY)
        print_bsm_summary_RV(mc);
}

int writeGeneralLog(char* tmpLogStr, uint32_t maxBufSize, bsm_data* bs, FILE *myfp,
    bool isTx, uint64_t periodicityMs, bool validPkt, uint32_t RVsInRange,
    const char* timeStamp, uint64_t monotonicTime, uint64_t realworldTimeNow,
    float locPositionDop, uint16_t locNumSvUsed, uint64_t gnssTime, uint8_t cbr,
    uint64_t txInterval, uint32_t l2SrcAddr){

    if(!tmpLogStr){
        return -1;
    }

    if(!bs){
        return -1;
    }

    char wall_time[100];
    get_wall_time(wall_time);

    if(isTx){
        snprintf(tmpLogStr, maxBufSize,
        "%s,%"PRIu64",%"PRIu64",%s,,%d,%lf,%"PRIu64",%d,%d,0,%d,%f,%f,%f,%f,%f,%f,%f,",
            timeStamp, realworldTimeNow, monotonicTime,
            "Tx", cbr, get_CPU_percentage(monotonicTime),
            txInterval, bs->MsgCount, bs->id,
            bs->secMark_ms, (bs->Latitude / 10000000.0), (bs->Longitude / 10000000.0),
            (bs->SemiMajorAxisAccuracy / 20.0), ((bs->Speed / 50.0) * 3.6),
            ( bs->Heading_degrees * 0.0125), (bs->AccelLon_cm_per_sec_squared / 100.0),
             ( bs->AccelLat_cm_per_sec_squared / 100.0));
    }else{
        snprintf(tmpLogStr, maxBufSize,
        "%s,%"PRIu64",%"PRIu64",%s,%08x,0,0,0,%d,%d,0,%d,%f,%f,%f,%f,%f,%f,%f,",
            timeStamp, realworldTimeNow, monotonicTime,
            "Rx", l2SrcAddr, bs->MsgCount, bs->id,
            bs->secMark_ms, (bs->Latitude / 10000000.0), (bs->Longitude / 10000000.0),
            (bs->SemiMajorAxisAccuracy / 20.0), ((bs->Speed / 50.0) * 3.6),
            ( bs->Heading_degrees * 0.0125), (bs->AccelLon_cm_per_sec_squared / 100.0),
             ( bs->AccelLat_cm_per_sec_squared / 100.0));
    }

    return 1;
}

//Function to write bsm contents to a csv file
void write_bsm_to_csv(msg_contents *mc, FILE *myfp, bool isTx, uint64_t periodicityMs,
    bool validPkt, uint32_t RVsInRange, uint64_t monotonicTime, uint64_t realworldTimeNow,
    float locPositionDop, uint16_t locNumSvUsed, uint64_t gnssTime, uint8_t cbr)
{
    //Writing Core Data
    int i = 1;
    bsm_value_t *bs = mc->j2735_msg;
    int tracking_error = 0;
    char wall_time[100];

    get_wall_time(wall_time);

    if (isTx) {
        if (bs->timestamp_ms < gnssTime) {
            /*If timestamp from GNSS is earlier than timestamp in tx message*/
            tracking_error = 1;
        } else if (bs->timestamp_ms - gnssTime >= 100) {
            /*if we are sending outdated location info, take it as tracking error*/
            tracking_error = 2;
        }
    } else {
        /*Rx scenario*/
        if (realworldTimeNow - bs->timestamp_ms >= 100) {
            /* if receives messge with timestamp too late, take it as trakcing error*/
            tracking_error = 3;
        } else if (bs->timestamp_ms < realworldTimeNow) {
            /*If received RV's timestamp is earlier than HV' now timestamp, take it as error*/
            tracking_error = 4;
        }
    }

    fprintf(myfp, "%s,%"PRIu64",%"PRIu64",%s,%d,%.2f,", wall_time,
        bs->timestamp_ms, monotonicTime, isTx ? "Tx" : "Rx",
        cbr, get_CPU_percentage(monotonicTime));

    fprintf(myfp, "%d,%04x,%d,%f,%f,%f,%f,%f,%f,%s,%f,%f,%f,%f,%f,%f,%f,%d,%d,%d,%d,",
        bs->MsgCount,
        bs->id, bs->secMark_ms, (bs->Latitude / 10000000.0), (bs->Longitude / 10000000.0),
        (bs->Elevation / 10.0), (bs->SemiMajorAxisAccuracy / 20.0),
        (bs->SemiMinorAxisAccuracy / 20.0), (bs->SemiMajorAxisOrientation * 0.0054932479), "",
        ((bs->Speed / 50.0) * 3.6), (bs->Heading_degrees * 0.0125),
        (bs->SteeringWheelAngle * 1.5), (bs->AccelLon_cm_per_sec_squared / 100.0),
        (bs->AccelLat_cm_per_sec_squared / 100.0), (bs->AccelVert_two_centi_gs / 50.0),
        (bs->AccelYaw_centi_degrees_per_sec / 100.0),
        bs->brakes.bits.antilock_brake_status, bs->brakes.bits.brake_boost_applied,
        bs->brakes.bits.stability_control_status, bs->brakes.bits.traction_control_status);

    //Writing part-2 extensions
    fprintf(myfp, "%d,%d,%d,",
        tracking_error, RVsInRange, validPkt);

    if (isTx) {
        fprintf(myfp,"%f %d,%"PRIu64",", locPositionDop, locNumSvUsed, gnssTime);
    } else {
        fprintf(myfp,",,");
    }

    if (periodicityMs != 0) {
        fprintf(myfp,"%"PRIu64",", periodicityMs);
    } else {
        fprintf(myfp,",");
    }

    // Event Flags
    if (bs->events.bits.eventHazardLights) {
        fprintf(myfp, "1,");
    } else {
        fprintf(myfp, ",");
    }

    if (bs->events.bits.eventABSactivated) {
        fprintf(myfp, "1,");
    } else {
        fprintf(myfp, ",");
    }

    if (bs->events.bits.eventTractionControlLoss) {
        fprintf(myfp, "1,");
    } else {
        fprintf(myfp, ",");
    }

    if (bs->events.bits.eventStabilityControlactivated) {
        fprintf(myfp, "1,");
    } else {
        fprintf(myfp, ",");
    }

    if (bs->events.bits.eventHardBraking) {
        fprintf(myfp, "1,");
    } else {
        fprintf(myfp, ",");
    }

    if (bs->events.bits.eventWipersChanged) {
        fprintf(myfp, "1,");
    } else {
        fprintf(myfp, ",");
    }

    if (bs->events.bits.eventAirBagDeployment) {
        fprintf(myfp, "1,");
    } else {
        fprintf(myfp, ",");
    }
    fprintf(myfp, "\n");
}

long double deg2rad(double deg){
    return deg * (M_PI / 180.0);
}

 double rad2deg(long double rad){
    return tan (rad * M_PI / 180.0);
}

double bsmCompute2dDistance(double hvLat, double hvLon, double rvLat, double rvLon){

    double rLat1 = deg2rad(hvLat); // deg2rad(lat1);
    double rLat2 = deg2rad(rvLat); // deg2rad(lat2);
    double rLon1 =  deg2rad(hvLon); // deg2rad(lon1);
    double rLon2 = deg2rad(rvLon); // deg2rad(lon2);
    double alpha = 6378137; // mean radius of earth in meters
    double f = 0.003353; // earth's flattening
    double f1 = pow((f * (2.0 - f)), (0.5));
    double f1_sqd = pow(f1, 2.0);
    double numer = (alpha * (1.0-f1_sqd));
    double denom = pow(1.0 - (f1_sqd * pow(sin(rvLat),2)), 1.5);
    double meridRad = (numer / denom);
    double distance = meridRad *
        ( acos(sin(rLat1) * sin(rLat2) + cos(rLat1) * cos(rLat2) * cos(rLon1 - rLon2)) ); // cos-1
    return distance;
    return distance;
}

// Writes bsm header to the csv file pointed by fp.
void write_bsm_header(FILE *fp)
{
    fprintf(fp, "WallTime,TimeStamp_ms,monotonicTime,TxRxType,CBR,CPU_busy_percent,");
    fprintf(fp, "msgCnt,TempId,secMark,lat,long,elev,semi_major_dev,");
    fprintf(fp, "semi_minor_dev,semi_major_orient,PRNDL,speed,heading,angle,longAccel,latAccel,");
    fprintf(fp, "vertAccel,yawRate,ABSAct,BrkAct,StbCtrlAct,TrcCtrlAct,");
    fprintf(fp, "Tracking_Error,vehicleDensityInRange,validMessage,GpsDOP Satellites,GPS-Time,max_ITT");
    fprintf(fp, "eventHazardLights,eventABSactivated,eventTractionControlLoss,");
    fprintf(fp, "eventStabilityControlactivated,eventHardBraking,eventFrontWipers,eventAirBagDeployment,");
#if 0
    fprintf(fp, "LowBeamStatus,HighBeamStatus,LftTurnSig,RtTurnSig,HzdLgts,");
    fprintf(fp, "AutoLghtCntrlOn,DtimeRunLghtsOn,FogLghtsOn,PkgLghtsOn,WiperSwFnt,WiperRtFnt,WiperSwRear,");
#endif
    fprintf(fp, "\n");
}

// public api to write to a csv file.
void writeToCsv(msg_contents *mc, FILE *fp, bool isTx, uint64_t periodicityMs,
    bool validPkt, uint32_t RVsInRange, uint64_t monotonicTime, uint64_t realworldTime,
    float locPositionDop, uint16_t locNumSvUsed, uint64_t gnssTime, uint8_t cbr)
{
    if (mc->msgId == J2735_MSGID_BASIC_SAFETY) {
        write_bsm_to_csv(mc, fp, isTx, periodicityMs, validPkt, RVsInRange, monotonicTime,
            realworldTime, locPositionDop, locNumSvUsed, gnssTime, cbr);
    }
}

// Writing to XML. This file could be opened in any browser.
void write_to_xml(msg_contents *mc, FILE *fp)
{
    int m = 0;
    char s1[30];
    //fprintf(fp,"<wsmp_length> %d </wsmp_length>\n", mc->payload_len) ;
    fprintf(fp, "<MessageFrame>\n");
    fprintf(fp, "\t<messageId>%d</messageId>\n", mc->msgId);
    fprintf(fp, "\t<value>\n");

    if (mc->msgId == J2735_MSGID_BASIC_SAFETY) {

        bsm_value_t *bsm = mc->j2735_msg;
        fprintf(fp, "\t\t<BasicSafetyMessage>\n");
        fprintf(fp, "\t\t\t<coreData>\n");
        fprintf(fp, "\t\t\t\t<msgCnt> %u </msgCnt>\n", bsm->MsgCount);
        fprintf(fp, "\t\t\t\t<id> %u </id>\n", bsm->id);
        fprintf(fp, "\t\t\t\t<secmark> %u </secmark>\n", bsm->secMark_ms);
        fprintf(fp, "\t\t\t\t<lat> %d </lat>\n", bsm->Latitude);
        fprintf(fp, "\t\t\t\t<lon> %d </lon>\n", bsm->Longitude);
        fprintf(fp, "\t\t\t\t<elev> %d </elev>\n", bsm->Elevation);
        fprintf(fp, "\t\t\t\t<accuracy>\n");
        fprintf(fp, "\t\t\t\t\t<semiMajor> %u </semiMajor>\n", bsm->SemiMajorAxisAccuracy);
        fprintf(fp, "\t\t\t\t\t<semiMinor> %u </semiMinor>\n", bsm->SemiMinorAxisAccuracy);
        fprintf(fp, "\t\t\t\t\t<orientation> %u </orientation>\n", bsm->SemiMajorAxisOrientation);
        fprintf(fp, "\t\t\t\t</accuracy>\n");
        fprintf(fp, "\t\t\t\t<transmission>\n");
        switch (bsm->TransmissionState) {
        case 0:
            fprintf(fp, "\t\t\t\t\tNeutral\n");
            break;
        case 1:
            fprintf(fp, "\t\t\t\t\tPark\n");
            break;
        case 2:
            fprintf(fp, "\t\t\t\t\tForward Gears\n");
            break;
        case 3:
            fprintf(fp, "\t\t\t\t\tReverse Gears\n");
            break;
        case 7:
            fprintf(fp, "\t\t\t\t\tUnavailable\n");
            break;
        default:
            fprintf(fp, "\t\t\t\t\tReserved\n");
        }
        fprintf(fp, "\t\t\t\t</transmission>\n");
        fprintf(fp, "\t\t\t\t<Speed> %u </Speed>\n", bsm->Speed);
        fprintf(fp, "\t\t\t\t<Heading> %u </Heading>\n", bsm->Heading_degrees);
        fprintf(fp, "\t\t\t\t<Angle> %d </Angle>\n", bsm->SteeringWheelAngle);
        fprintf(fp, "\t\t\t\t<Accleration>\n");
        fprintf(fp, "\t\t\t\t\t<Lon> %d </Lon>\n", bsm->AccelLon_cm_per_sec_squared);
        fprintf(fp, "\t\t\t\t\t<Lat> %d </Lat>\n", bsm->AccelLat_cm_per_sec_squared);
        fprintf(fp, "\t\t\t\t\t<Vert> %d </Vert>\n", bsm->AccelVert_two_centi_gs);
        fprintf(fp, "\t\t\t\t\t<Yaw> %d </Yaw>\n", bsm->AccelYaw_centi_degrees_per_sec);
        fprintf(fp, "\t\t\t\t</Accleration>\n");
        fprintf(fp, "\t\t\t\t<brakes>\n");
        fprintf(fp, "\t\t\t\t<brakes.data> %04x </brakes.data>\n", bsm->brakes.word);
        fprintf(fp, "\t\t\t\t\t<Aux_brakes>\n");
        fprintf(fp, "\t\t\t\t\t\t%s\n", brake_str0[bsm->brakes.word >> 1 & (3)]);
        fprintf(fp, "\t\t\t\t\t</Aux_brakes>\n");
        fprintf(fp, "\t\t\t\t\t<brake_boost>\n");
        fprintf(fp, "\t\t\t\t\t\t%s\n", brake_str1[bsm->brakes.word >> 3 & (3)]);
        fprintf(fp, "\t\t\t\t\t</brake_boost>\n");
        fprintf(fp, "\t\t\t\t\t<stability_control>\n");
        fprintf(fp, "\t\t\t\t\t\t%s\n", brake_str2[bsm->brakes.word >> 5 & (3)]);
        fprintf(fp, "\t\t\t\t\t</stability_control>\n");
        fprintf(fp, "\t\t\t\t\t<Antilock_brakes>\n");
        fprintf(fp, "\t\t\t\t\t\t%s\n", brake_str2[bsm->brakes.word >> 7 & (3)]);
        fprintf(fp, "\t\t\t\t\t</Antilock_brakes>\n");
        fprintf(fp, "\t\t\t\t\t<traction_control>\n");
        fprintf(fp, "\t\t\t\t\t\t%s\n", brake_str2[bsm->brakes.word >> 9 & (3)]);
        fprintf(fp, "\t\t\t\t\t</traction_control>\n");
        fprintf(fp, "\t\t\t\t</brakes>\n");
        fprintf(fp, "\t\t\t\t<VehicleWidth> %u </VehicleWidth>\n", bsm->VehicleWidth_cm);
        fprintf(fp, "\t\t\t\t<VehicleLength> %u </VehicleLength>\n", bsm->VehicleLength_cm);
        fprintf(fp, "\t\t\t</coreData>\n");
        if (bsm->has_partII) {
            fprintf(fp, "\t\t\t<part-II>\n");
            fprintf(fp, "\t\t\t\t<qty_partII_extensions> %d </qty_partII_extensions>\n",
                    bsm->qty_partII_extensions);
            if (bsm->has_safety_extension) {
                fprintf(fp, "\t\t\t\t<Veh_Safety_Ext>\n");
                fprintf(fp, "\t\t\t\t\t<Vehsafeopts> %04x </Vehsafeopts>\n", bsm->vehsafeopts);
                if (bsm->vehsafeopts & PART_II_SAFETY_EXT_OPTION_EVENTS) {
                    fprintf(fp, "\t\t\t\t\t<Events>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Airbag>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 0 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Airbag>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Disabled_Veh>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 1 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Disabled_Veh>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Flat_Tire>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 2 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Flat_Tire>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Wipers_Changed>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 3 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Wipers_Changed>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Lights_Changed>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 4 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Lights_Changed>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Hard_Braking>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 5 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Hard_Braking>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Reserved>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 6 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Reserved>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Hazardous Material>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 7 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Hazardous_Material>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Stability_Control>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 8 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Stability_Control>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Traction_Control>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 9 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Traction_Control>\n");
                    fprintf(fp, "\t\t\t\t\t\t<ABS_activated>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 10 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</ABS_activated>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Stopline_Violation>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 11 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</Stopline_Violation>\n");
                    fprintf(fp, "\t\t\t\t\t\t<HazardLights>\n");
                    fprintf(fp, "\t\t\t\t\t\t\t%s\n", event_str[bsm->events.data >> 12 & (1)]);
                    fprintf(fp, "\t\t\t\t\t\t</HazardLights>\n");
                    fprintf(fp, "\t\t\t\t\t</Events>\n");
                }

                if (bsm->vehsafeopts & PART_II_SAFETY_EXT_OPTION_PATH_HISTORY) {
                    fprintf(fp, "\t\t\t\t\t<Path_History>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Options>%02x</Options>\n", bsm->phopts);
                    if (bsm->phopts & PATH_HISTORY_OPTION_INITALPOSITION) {
                        fprintf(fp, "\t\t\t\t\t\t<Initial_Position>\n");
                        fprintf(fp, "\t\t\t\t\t\t<FPV_Options>%02x</FPV_Options\n",
                                bsm->ph.initialPosition.opts.byte);
                        fprintf(fp, "\t\t\t\t\t\t\t<Lat>%d</Lat>\n", bsm->ph.initialPosition.lat);
                        fprintf(fp, "\t\t\t\t\t\t\t<Lon>%d</Lon>\n", bsm->ph.initialPosition.lon);
                        if (bsm->ph.initialPosition.opts.byte & FULLPOSITIONVECTOR_OPTION_ELEVATION) {
                            fprintf(fp, "\t\t\t\t\t\t\t<elev>%d</elev>\n",
                                    bsm->ph.initialPosition.elevation);
                        }
                        if (bsm->ph.initialPosition.opts.byte & FULLPOSITIONVECTOR_OPTION_HEADING) {
                            fprintf(fp, "\t\t\t\t\t\t\t<heading>%d</heading>\n",
                                    bsm->ph.initialPosition.heading);
                        }
                        if (bsm->ph.initialPosition.opts.byte & FULLPOSITIONVECTOR_OPTION_SPEED) {
                            fprintf(fp, "\t\t\t\t\t\t\t<speed>%d</speed>\n",
                                    bsm->ph.initialPosition.speed);
                        }
                        if (bsm->ph.initialPosition.opts.byte &
                                FULLPOSITIONVECTOR_OPTION_POS_ACCURACY) {
                            fprintf(fp, "\t\t\t\t\t\t\t<accuracy>\n");
                            fprintf(fp, "\t\t\t\t\t\t\t\t<semiMajor> %u </semiMajor>\n",
                                    bsm->ph.initialPosition.pos_accuracy.semi_major);
                            fprintf(fp, "\t\t\t\t\t\t\t\t<semiMinor> %u </semiMinor>\n",
                                    bsm->ph.initialPosition.pos_accuracy.semi_minor);
                            fprintf(fp, "\t\t\t\t\t\t\t\t<orientation> %u </orientation>\n",
                                    bsm->ph.initialPosition.pos_accuracy.orientation);
                            fprintf(fp, "\t\t\t\t\t\t\t</accuracy>\n");
                        }
                        if (bsm->ph.initialPosition.opts.byte &
                                FULLPOSITIONVECTOR_OPTION_TIME_CONFIDENCE) {
                            fprintf(fp, "\t\t\t\t\t\t\t<confidence>%d</confidence>\n",
                                    bsm->ph.initialPosition.time_confidence);
                        }
                        if (bsm->ph.initialPosition.opts.byte &
                                FULLPOSITIONVECTOR_OPTION_POS_CONFIDENCE) {
                            fprintf(fp, "\t\t\t\t\t\t\t<pos_confidence>\n");
                            fprintf(fp, "\t\t\t\t\t\t\t\t<xy>%d</xy>\n",
                                    bsm->ph.initialPosition.pos_confidence.xy);
                            fprintf(fp, "\t\t\t\t\t\t\t\t<elevconf>%d</elevconf>\n",
                                    bsm->ph.initialPosition.pos_confidence.elevation);
                            fprintf(fp, "\t\t\t\t\t\t\t</pos_confidence>\n");
                        }
                        if (bsm->ph.initialPosition.opts.byte &
                                FULLPOSITIONVECTOR_OPTION_SPEED_CONFIDENCE) {
                            fprintf(fp, "\t\t\t\t\t\t\t<speed_confidence>\n");
                            fprintf(fp, "\t\t\t\t\t\t\t\t<heading_confidence>%d</heading_confidence>\n",
                                bsm->ph.initialPosition.motion_confidence_set.heading_confidence);
                            fprintf(fp, "\t\t\t\t\t\t\t\t<speed_confidence>%d</speed_confidence>\n",
                                bsm->ph.initialPosition.motion_confidence_set.speed_confidence);
                            fprintf(fp, "\t\t\t\t\t\t\t\t<throttle_confidence>%d</throttle_confidence>\n",
                                bsm->ph.initialPosition.motion_confidence_set.throttle_confidence);
                            fprintf(fp, "\t\t\t\t\t\t\t</speed_confidence>\n");
                        }

                        if (bsm->ph.initialPosition.opts.byte & FULLPOSITIONVECTOR_OPTION_UTCTIME) {
                            fprintf(fp, "\t\t\t\t\t\t\t<utcTime>\n");

                            if (bsm->ph.initialPosition.utcTime.opts.bits.has_year) {
                                fprintf(fp, "\t\t\t\t\t\t\t\t<year>%d</year>\n",
                                        bsm->ph.initialPosition.utcTime.year);
                            }

                            if (bsm->ph.initialPosition.utcTime.opts.bits.has_month) {
                                fprintf(fp, "\t\t\t\t\t\t\t\t<month>%d</month>\n",
                                        bsm->ph.initialPosition.utcTime.month);
                            }

                            if (bsm->ph.initialPosition.utcTime.opts.bits.has_day) {
                                fprintf(fp, "\t\t\t\t\t\t\t\t<day>%d</day>\n",
                                        bsm->ph.initialPosition.utcTime.day);
                            }

                            if (bsm->ph.initialPosition.utcTime.opts.bits.has_hour) {
                                fprintf(fp, "\t\t\t\t\t\t\t\t<hour>%d</hour>\n",
                                        bsm->ph.initialPosition.utcTime.hour);
                            }

                            if (bsm->ph.initialPosition.utcTime.opts.bits.has_minute) {
                                fprintf(fp, "\t\t\t\t\t\t\t\t<minute>%d</minute>\n",
                                        bsm->ph.initialPosition.utcTime.minute);
                            }

                            if (bsm->ph.initialPosition.utcTime.opts.bits.has_second) {
                                fprintf(fp, "\t\t\t\t\t\t\t\t<second>%d</second>\n",
                                        bsm->ph.initialPosition.utcTime.second);
                            }

                            if (bsm->ph.initialPosition.utcTime.opts.bits.has_offset) {
                                fprintf(fp, "\t\t\t\t\t\t\t\t<offset>%d</offset>\n",
                                        bsm->ph.initialPosition.utcTime.offset);
                            }

                            fprintf(fp, "\t\t\t\t\t\t\t</utcTime>\n");
                        }

                        fprintf(fp, "\t\t\t\t\t\t</Initial Position>\n");
                    }

                    if (bsm->phopts & PATH_HISTORY_OPTION_GNSS_STATUS) {
                        fprintf(fp, "\t\t\t\t\t\t\t<gnss_status>%d</gnss_status>\n",
                                bsm->ph.gnss_status.data);
                    }

                    fprintf(fp, "\t\t\t\t\t\t<qty_crumbs>%d</qty_crumbs>\n", bsm->ph.qty_crumbs);
                    for ( m = 0; m < bsm->ph.qty_crumbs; m++) {
                        fprintf(fp, "\t\t\t\t\t\t<PathHistoryPoint>\n");
                        fprintf(fp, "\t\t\t\t\t\t\t<latOffset>%d</latOffset>\n",
                                bsm->ph.ph_crumb[m].latOffset);
                        fprintf(fp, "\t\t\t\t\t\t\t<lonOffset>%d</lonOffset>\n",
                                bsm->ph.ph_crumb[m].lonOffset);
                        fprintf(fp, "\t\t\t\t\t\t\t<elevOffset>%d</elevOffset>\n",
                                bsm->ph.ph_crumb[m].eleOffset);
                        fprintf(fp, "\t\t\t\t\t\t\t<timeOffset>%d</timeOffset>\n",
                                bsm->ph.ph_crumb[m].timeOffset_ms);
                        fprintf(fp, "\t\t\t\t\t\t\t<crumb_opts>%d</crumb_opts>\n",
                                bsm->ph.ph_crumb[m].opts_u.byte);
                        if (bsm->ph.ph_crumb[m].opts_u.byte & PATH_HISTORY_POINT_OPTION_SPEED) {
                            fprintf(fp, "\t\t\t\t\t\t\t<speed>%d</speed>\n",
                                    bsm->ph.ph_crumb[m].speed);
                        }

                        if (bsm->ph.ph_crumb[m].opts_u.byte & PATH_HISTORY_POINT_OPTION_ACCURACY) {
                            fprintf(fp, "\t\t\t\t\t\t\t<accuracy>\n");
                            fprintf(fp, "\t\t\t\t\t\t\t\t<semiMajor> %u </semiMajor>\n",
                                    bsm->ph.ph_crumb[m].accy.semi_major);
                            fprintf(fp, "\t\t\t\t\t\t\t\t<semiMinor> %u </semiMinor>\n",
                                    bsm->ph.ph_crumb[m].accy.semi_minor);
                            fprintf(fp, "\t\t\t\t\t\t\t\t<orientation> %u </orientation>\n",
                                    bsm->ph.ph_crumb[m].accy.orientation);
                            fprintf(fp, "\t\t\t\t\t\t\t</accuracy>\n");
                        }

                        if (bsm->ph.ph_crumb[m].opts_u.byte & PATH_HISTORY_POINT_OPTION_HEADING) {
                            fprintf(fp, "\t\t\t\t\t\t\t<heading>%d</heading>\n",
                                    bsm->ph.ph_crumb[m].heading_microdegrees);
                        }
                        fprintf(fp, "\t\t\t\t\t\t</PathHistoryPoint>\n");
                    }

                    fprintf(fp, "\t\t\t\t\t</Path_History>\n");
                }

                if (bsm->vehsafeopts & PART_II_SAFETY_EXT_OPTION_PATH_PREDICTION) {

                    fprintf(fp, "\t\t\t\t\t<Path_Prediction>\n");

                    fprintf(fp, "\t\t\t\t\t\t<radius>%d</radius>\n", bsm->pp.radius);

                    fprintf(fp, "\t\t\t\t\t\t<is_straight>%d</is_straight>\n", bsm->pp.is_straight);

                    fprintf(fp, "\t\t\t\t\t\t<confidence>%d</confidence>\n", bsm->pp.confidence);

                    fprintf(fp, "\t\t\t\t\t</Path_Prediction>\n");

                }

                if (bsm->vehsafeopts & PART_II_SAFETY_EXT_OPTION_LIGHTS) {
                    fprintf(fp, "\t\t\t\t\t\t<parkingLightsOn>%s</parkingLightsOn>\n",
                            event_str[bsm->lights_in_use.data & 1]);
                    fprintf(fp, "\t\t\t\t\t\t<fogLightOn>%s</fogLightOn>\n",
                            event_str[(bsm->lights_in_use.data >> 1) & 1]);
                    fprintf(fp, "\t\t\t\t\t\t<daytimeRunningLightsOn>%s</daytimeRunningLightsOn>\n",
                            event_str[(bsm->lights_in_use.data >> 2)& 1]);
                    fprintf(fp, "\t\t\t\t\t\t<automaticLightControlOn>%s</automaticLightControlOn>\n",
                            event_str[(bsm->lights_in_use.data >> 3)& 1]);
                    fprintf(fp, "\t\t\t\t\t\t<hazardSignalOn>%s</hazardSignalOna>\n",
                            event_str[(bsm->lights_in_use.data >> 4)& 1]);
                    fprintf(fp, "\t\t\t\t\t\t<rightTurnSignalOn>%s</rightTurnSignalOn>\n",
                            event_str[(bsm->lights_in_use.data >> 5)& 1]);
                    fprintf(fp, "\t\t\t\t\t\t<leftTurnSignalOn>%s</leftTurnSignalOn>\n",
                            event_str[(bsm->lights_in_use.data >> 6)& 1]);
                    fprintf(fp, "\t\t\t\t\t\t<highBeamHeadlightsOn>%s</highBeamHeadlightsOn>\n",
                            event_str[(bsm->lights_in_use.data >> 7)& 1]);
                    fprintf(fp, "\t\t\t\t\t\t<lowBeamHeadlightsOn>%s</lowBeamHeadlightsOn>\n",
                            event_str[(bsm->lights_in_use.data >> 8)& 1]);
                }


                fprintf(fp, "\t\t\t\t</Veh_Safety_Ext>\n");
            }

            if (bsm->has_special_extension) {
                fprintf(fp, "\t\t\t\t<Special_Vehicle_Ext>\n");
                if (bsm->specvehopts & SPECIAL_VEH_EXT_OPTION_EMERGENCY_DETAILS) {
                    fprintf(fp, "\t\t\t\t\t<Emergency_Details>\n");
                    fprintf(fp, "\t\t\t\t\t<edopts>%d</edopts>\n", bsm->edopts);
                    fprintf(fp, "\t\t\t\t\t<sspRights>%d</sspRights>\n",
                            bsm->vehicleAlerts.sspRights);
                    fprintf(fp, "\t\t\t\t\t<sirenUse>%d</sirenUse>\n", bsm->vehicleAlerts.sirenUse);
                    fprintf(fp, "\t\t\t\t\t<lightsUse>%d</lightsUse>\n",
                            bsm->vehicleAlerts.lightsUse);
                    fprintf(fp, "\t\t\t\t\t<multi>%d</multi>\n", bsm->vehicleAlerts.multi);

                    if (bsm->edopts & EMERGENCY_DATA_OPTION_PRIVILEGED_EVENT) {
                        fprintf(fp, "\t\t\t\t\t<Privileged_Events>\n");
                        fprintf(fp, "\t\t\t\t\t\t<sspRights>%d</sspRights>\n",
                                bsm->vehicleAlerts.events.sspRights);
                        fprintf(fp, "\t\t\t\t\t\t<event>%d</event>\n",
                                bsm->vehicleAlerts.events.event);
                        fprintf(fp, "\t\t\t\t\t</Privileged_Events>\n");
                    }
                    if (bsm->edopts & EMERGENCY_DATA_OPTION_RESPONSE_TYPE) {
                        fprintf(fp, "\t\t\t\t\t<Response_Type>\n");
                        fprintf(fp, "\t\t\t\t\t\t%d\n", bsm->vehicleAlerts.responseType);
                        fprintf(fp, "\t\t\t\t\t</Response_Type>\n");
                    }
                    fprintf(fp, "\t\t\t\t\t</Emergency_Details>\n");
                }
                if (bsm->specvehopts & SPECIAL_VEH_EXT_OPTION_EVENT_DESC) {

                    fprintf(fp, "\t\t\t\t\t<Event_Description>\n");
                    fprintf(fp, "\t\t\t\t\t\t<Event_Opts>%d</Event_Opts>\n", bsm->eventopts);
                    fprintf(fp, "\t\t\t\t\t\t<typeEvent>%d</typeEvent>\n",
                            bsm->description.typeEvent);

                    if (bsm->eventopts & SPECIAL_VEH_EVENT_OPTION_DESC) {
                        fprintf(fp, "\t\t\t\t\t<Description_Seq>\n");
                        fprintf(fp, "\t\t\t\t\t\t<descsize>%d</descsize>\n",
                                bsm->description.size_desc);
                        fprintf(fp, "\t\t\t\t\t\t<desc>\n");
                        for ( m = 0; m < bsm->description.size_desc; m++) {
                            fprintf(fp, "\t\t\t\t\t\t\t%d\n", bsm->description.desc[m]);
                        }
                        fprintf(fp, "\t\t\t\t\t\t</desc>\n");
                        fprintf(fp, "\t\t\t\t\t</Description_Seq>\n");
                    }

                    if (bsm->eventopts & SPECIAL_VEH_EVENT_OPTION_PRIOIRTY) {
                        fprintf(fp, "\t\t\t\t\t<priority>\n");
                        fprintf(fp, "\t\t\t\t\t\t%d\n", bsm->description.priority);
                        fprintf(fp, "\t\t\t\t\t</priority>\n");
                    }

                    if (bsm->eventopts & SPECIAL_VEH_EVENT_OPTION_HEADINGSLICE) {
                        fprintf(fp, "\t\t\t\t\t<headingslice>\n");
                        fprintf(fp, "\t\t\t\t\t\t%d\n", bsm->description.heading);
                        fprintf(fp, "\t\t\t\t\t</headingslice>\n");
                    }

                    if (bsm->eventopts & SPECIAL_VEH_EVENT_OPTION_EXTENT) {
                        fprintf(fp, "\t\t\t\t\t<extent\n");
                        fprintf(fp, "\t\t\t\t\t\t%d\n", bsm->description.extent);
                        fprintf(fp, "\t\t\t\t\t</extent>\n");
                    }

                    if (bsm->eventopts & SPECIAL_VEH_EVENT_OPTION_REGIONAL_EXT) {
                        fprintf(fp, "\t\t\t\t\t<regional ext\n");
                        fprintf(fp, "\t\t\t\t\t\tYes\n");
                        fprintf(fp, "\t\t\t\t\t</regional ext>\n");
                    }
                    fprintf(fp, "\t\t\t\t\t</Event_Description>\n");
                }


                if (bsm->specvehopts & SPECIAL_VEH_EXT_OPTION_TRAILER_DATA) {
                    fprintf(fp, "\t\t\t\t\t<TrailerData>\n");
                    fprintf(fp, "\t\t\t\t\t\tYes\n");
                    fprintf(fp, "\t\t\t\t\t</TrailerData>\n");
                }

                fprintf(fp, "\t\t\t\t</Special_Vehicle_Ext>\n");
            }

            if (bsm->has_supplemental_extension) {
                fprintf(fp, "\t\t\t\t<Suppl_Vehicle_Ext>\n");

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_CLASSIFICATION) {
                    fprintf(fp, "\t\t\t\t\t<Basic_Vehicle_Class>\n");
                    fprintf(fp, "\t\t\t\t\t\t%d\n", bsm->VehicleClass);
                    fprintf(fp, "\t\t\t\t\t</Basic_Vehicle_Class>\n");
                }

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_CLASS_DETAILS) {

                    fprintf(fp, "\t\t\t\t\t<Classification>\n");
                    fprintf(fp, "\t\t\t\t\t</Classification>\n");
                }

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_VEHICLE_DATA) {
                    fprintf(fp, "\t\t\t\t\t<Vehicle_Data>\n");
                    fprintf(fp, "\t\t\t\t\t\t<VehData_opts>%d</VehData_opts>\n",
                            bsm->veh.supplemental_veh_data_options.word);

                    if (bsm->veh.supplemental_veh_data_options.word &
                            SUPPLEMENT_VEH_DATA_OPTION_HEIGHT) {
                        fprintf(fp, "\t\t\t\t\t\t<Vehicle_Height>\n");
                        fprintf(fp, "\t\t\t\t\t\t\t%d\n", bsm->veh.height_cm);
                        fprintf(fp, "\t\t\t\t\t\t</Vehicle_Height>\n");
                    }

                    if (bsm->veh.supplemental_veh_data_options.word &
                            SUPPLEMENT_VEH_DATA_OPTION_BUMPERS) {
                        fprintf(fp, "\t\t\t\t\t\t<Front_Bumper_Height>%d</Front_Bumper_Height>\n",
                                bsm->veh.front_bumper_height_cm);
                        fprintf(fp, "\t\t\t\t\t\t<Rear_Bumper_Height>%d</Rear_Bumper_Height>\n",
                                bsm->veh.rear_bumper_height_cm);
                    }

                    if (bsm->veh.supplemental_veh_data_options.word &
                            SUPPLEMENT_VEH_DATA_OPTION_MASS) {
                        fprintf(fp, "\t\t\t\t\t\t<Mass>%d</Mass>\n", bsm->veh.mass_kg);
                    }

                    if (bsm->veh.supplemental_veh_data_options.word &
                            SUPPLEMENT_VEH_DATA_OPTION_TRAILER_WEIGHT) {
                        fprintf(fp, "\t\t\t\t\t\t<trailer_weight>%d</trailer_weight>\n",
                                bsm->veh.trailer_weight);
                    }

                    fprintf(fp, "\t\t\t\t\t</Vehicle_Data>\n");

                }

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_WEATHER_REPORT) {
                    fprintf(fp, "\t\t\t\t\t<Weather_Report>\n");
                    fprintf(fp, "\t\t\t\t\t</Weather_Report>\n");
                }

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_WEATHER_PROBE) {
                    fprintf(fp, "\t\t\t\t\t<Weather_Probe>\n");

                    if (bsm->weatheropts & SUPPLEMENT_WEATHER_AIRTEMP) {
                        fprintf(fp, "\t\t\t\t\t\t<Air_Temp>%d</Air_Temp>\n", bsm->airTemp);
                    }

                    if (bsm->weatheropts & SUPPLEMENT_WEATHER_AIRPRESSURE) {
                        fprintf(fp, "\t\t\t\t\t\t<Air_Pressure>%d</Air_Pressure>\n",
                                bsm->airPressure);
                    }

                    if (bsm->weatheropts & SUPPLEMENT_WEATHER_WIPERS) {
                        fprintf(fp, "\t\t\t\t\t\t<Wipers>\n");

                        fprintf(fp, "\t\t\t\t\t\t\t<wiperopts>%d</wiperopts>\n", bsm->wiperopts);
                        fprintf(fp, "\t\t\t\t\t\t\t<statusFront>%d</statusFront>\n",
                                bsm->statusFront);
                        fprintf(fp, "\t\t\t\t\t\t\t<rateFront>%d</rateFront>\n", bsm->rateFront);

                        if (bsm->wiperopts & SUPPLEMENT_WEATHER_WIPERS_REAR_STATUS) {
                            fprintf(fp, "\t\t\t\t\t\t\t<statusRear>%d</statusRear>\n",
                                    bsm->statusRear);
                        }

                        if (bsm->wiperopts & SUPPLEMENT_WEATHER_WIPERS_REAR_RATE) {
                            fprintf(fp, "\t\t\t\t\t\t\t<rateRear>%d</rateRear>\n", bsm->rateRear);
                        }
                        fprintf(fp, "\t\t\t\t\t\t</Wipers>\n");
                    }

                    fprintf(fp, "\t\t\t\t\t</Weather_Probe>\n");

                }

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_OBSTACLE) {
                    fprintf(fp, "\t\t\t\t\t<Obstacle>\n");
                    fprintf(fp, "\t\t\t\t\t</Obstacle>\n");
                }

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_DISABLED_VEH) {
                    fprintf(fp, "\t\t\t\t\t<Disabled_Veh>\n");
                    fprintf(fp, "\t\t\t\t\t</Disabled_Veh>\n");
                }

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_SPEED_PROFILE) {
                    fprintf(fp, "\t\t\t\t\t<Speed_Profile>\n");
                    fprintf(fp, "\t\t\t\t\t</Speed_Profile>\n");
                }

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_RTCM) {
                    fprintf(fp, "\t\t\t\t\t<RTCM>\n");
                    fprintf(fp, "\t\t\t\t\t</RTCM>\n");
                }

                if (bsm->suppvehopts & SUPPLEMENT_VEH_EXT_OPTION_REGIONAL_EXT) {
                    fprintf(fp, "\t\t\t\t\t<Regional>\n");
                    fprintf(fp, "\t\t\t\t\t</Regional>\n");
                }
                fprintf(fp, "\t\t\t\t</Suppl_Vehicle_Ext>\n");
            }

            fprintf(fp, "\t\t\t</part-II>\n");
        }

        fprintf(fp, "\t\t</BasicSafetyMessage>\n");

    }

    fprintf(fp, "\t</value>\n");
    fprintf(fp, "</MessageFrame>\n");

}
//Permute the 32 bit identifier if necessary.
unsigned int id_shift(unsigned int a)
{
    char b[100], c[100];
    snprintf(b, 100, "%08x", a);
    int l = strlen(b);
    int i = l - 2;
    int j = 0;
    while (i >= 0) {
        c[j] = b[i];
        c[j + 1] = b[i + 1];
        i = i - 2;
        j = j + 2;
    }
    c[j] = '\0';
    unsigned int d  = (unsigned int)strtol(c, NULL, 16);

    return d;
}
// Take a line in CSV file and encode its contents into buf and return its length.
// len parameter just shows the size of buf created by caller.
int encode_singleline_fromCSV(char *line, msg_contents *mc, bool bsmLog)
{
    int i = 0, m = 0;
    wsmp_data_t *wsmpp = (wsmp_data_t*)(mc->wsmp);
    bsm_value_t *bsm = (bsm_value_t *)(mc->j2735_msg);
    ieee1609_2_data *ie = (ieee1609_2_data*)(mc->ieee1609_2data);

    ie->protocolVersion = 3;
    ie->content = 0;
    ie->tagclass = 2;
    wsmpp->n_header.data = 3;
    wsmpp->tpid.octet = 0;
    wsmpp->psid = 1;

    char *tmp = strdup(line);
    if (tmp == NULL){
        return 0;
    }
    const char *tok;
    char **tokens = (char **)calloc(sizeof(char *), 1000);
    if (tokens == NULL){
        free(tmp);
        return 0;
    }
    int j = 0;
    while (j < 1000){
        tokens[j] = NULL;
        j++;
    }
    while ((tok = strsep(&tmp, ",")) != NULL) {
        tokens[i] = strdup(tok);
        i++;
    }

    bsm->brakes.word = (1 << 15);
    bsm->suppvehopts = 0;
    bsm_init(bsm);

    if (!bsmLog) {
        bsm->timestamp_ms = strtoull(tokens[1], NULL, 0);
        bsm->MsgCount = strtoul(tokens[8], NULL, 0);
        bsm->id = id_shift(strtoul(tokens[9], NULL, 0));
        bsm->secMark_ms = strtoul(tokens[11], NULL, 0);
        bsm->Latitude = strtod(tokens[12], NULL) * 10000000.0;
        bsm->Longitude = strtod(tokens[13], NULL) * 10000000.0;
        bsm->SemiMajorAxisAccuracy = strtoul(tokens[14], NULL, 0) * 20;
        bsm->Speed = strtoul(tokens[15], NULL, 0) * 250 / 18;
        bsm->Heading_degrees = strtol(tokens[16], NULL, 0) / 0.0125;
        bsm->AccelLon_cm_per_sec_squared = strtol(tokens[17], NULL, 0) / 0.01;
    } else {
        int a = 0;
        while (a < i) {
            switch (a) {
            case 15:
                mc->msgId = J2735_MSGID_BASIC_SAFETY;
                break;
            case 16:
                bsm->MsgCount = atoi(tokens[16]);
                break;
            case 17:
                bsm->id = id_shift(atoi(tokens[17]));
                break;
            case 18:
                break;
            case 19:
                bsm->secMark_ms = atoi(tokens[19]);
                break;
            case 20:
                bsm->Latitude = (int)(atof(tokens[20]) * 10000000); // Check int - float
                break;
            case 21:
                bsm->Longitude = (int)(atof(tokens[21]) * 10000000);   // Check int - float
                break;
            case 22:
                bsm->Elevation = (int)(atof(tokens[22]) * 10);  // Check *2 or not
                break;
            case 23:
                bsm->SemiMajorAxisAccuracy = atof(tokens[23]) * 20; // int - float
                break;
            case 24:
                bsm->SemiMinorAxisAccuracy = atof(tokens[24]) * 20; // int - float
                break;
            case 25:
                bsm->SemiMajorAxisOrientation = (int)(atof(tokens[25]) / 0.0054932479); // int - float
                break;
            case 26:
                bsm->TransmissionState = atoi(tokens[26]);
                break;
            case 27:
                bsm->Speed = (atoi(tokens[27]) * 250 / 18);  // Verify kmph to conversion
                break;
            case 28:
                bsm->Heading_degrees = (atof(tokens[28]) / 0.0125); // taking value in degrees and encoding as an integer
                break;
            case 29:
                if (atoi(tokens[29]) > 188)
                    bsm->SteeringWheelAngle = 126;
                else if (atoi(tokens[29]) < -191)
                    bsm->SteeringWheelAngle = -128;
                else
                    bsm->SteeringWheelAngle = atoi(tokens[29]) / 1.5;
                break;
            case 30:
                if (atoi(tokens[30]) / 0.01 > 2000)
                    bsm->AccelLon_cm_per_sec_squared = 2000;
                else if (atoi(tokens[30]) / 0.01 < -2000)
                    bsm->AccelLon_cm_per_sec_squared = -2000;
                else
                    bsm->AccelLon_cm_per_sec_squared = atoi(tokens[30]) / 0.01;
                break;
            case 31:
                if (atoi(tokens[31]) / 0.01 > 2000)
                    bsm->AccelLat_cm_per_sec_squared = 2000;
                else if (atoi(tokens[31]) / 0.01 < -2000)
                    bsm->AccelLat_cm_per_sec_squared = -2000;
                else
                    bsm->AccelLat_cm_per_sec_squared = atoi(tokens[31]) / 0.01;
                break;
            case 32:
                bsm->AccelVert_two_centi_gs = atoi(tokens[32]) * 50; // / 0.1962 ; //in csv, give in g's
                break;
            case 33:
                bsm->AccelYaw_centi_degrees_per_sec = atoi(tokens[33]) / 0.01;
                break;
            case 34:
                bsm->brakes.word = bsm->brakes.word | (atoi(tokens[34]) << 7);
                break;
            case 35:
                bsm->brakes.word = bsm->brakes.word | (atoi(tokens[35]) << 3); //VERIFY
                break;
            case 36:
                bsm->brakes.word = bsm->brakes.word | (atoi(tokens[36]) << 5);
                break;
            case 37:
                bsm->brakes.word = bsm->brakes.word | (atoi(tokens[37]) << 9);
                break;
            case 38:
                bsm->VehicleWidth_cm = atoi(tokens[38]);
                break;
            case 39:
                bsm->VehicleLength_cm = atoi(tokens[39]);
                break;
            case 40:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 3);
                bsm->events.data = bsm->events.data | (atoi(tokens[40]) << 12);
                break;
            case 41:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 3);
                bsm->events.data = bsm->events.data | (atoi(tokens[41]) << 10);
                break;
            case 42:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 3);
                bsm->events.data = bsm->events.data | (atoi(tokens[42]) << 9);
                break;
            case 43:
                bsm->has_partII = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 3);
                bsm->events.data = bsm->events.data | (atoi(tokens[43]) << 8);
                break;
            case 44:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 3);
                bsm->events.data = bsm->events.data | (atoi(tokens[44]) << 5);
                break;
            case 45:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 3);
                bsm->events.data = bsm->events.data | (atoi(tokens[45]) << 3);
                break;
            case 46:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 3);
                bsm->events.data = bsm->events.data | (atoi(tokens[46]) << 0);
                break;
            case 47:
                if (atoi(tokens[47]) == 0)
                    break;
                else {
                    bsm->vehsafeopts = bsm->vehsafeopts | (1 << 2);
                    bsm->ph.qty_crumbs = atoi(tokens[47]);
                    bsm->has_partII = 1;
                    bsm->has_safety_extension = 1;
                    for (m = 0; m < bsm->ph.qty_crumbs; m++) {
                        bsm->ph.ph_crumb[m].latOffset = atoi(tokens[a + 1 + 5 * m]); //*10;
                        bsm->ph.ph_crumb[m].lonOffset = atoi(tokens[a + 1 + 5 * m + 1]); //*10;
                        bsm->ph.ph_crumb[m].eleOffset = atoi(tokens[a + 1 + 5 * m + 2]); //*10;
                        bsm->ph.ph_crumb[m].timeOffset_ms = atoi(tokens[a + 1 + 5 * m + 3]); //*10;
                        bsm->ph.ph_crumb[m].opts_u.byte = 1;
                        bsm->ph.ph_crumb[m].heading_available = 1;
                        bsm->ph.ph_crumb[m].heading_microdegrees = (atoi(tokens[a + 1 + 5 * m + 4]) / 1.5) * 1500;
                }
                a = a + 5 * (bsm->ph.qty_crumbs);
                }
                break;
            case 163:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 1);
                bsm->pp.radius = atof(tokens[163]) * 10;
                break;
            case 164:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 1);
                bsm->pp.confidence = atoi(tokens[164]) * 2;
                break;
            case 165:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
                bsm->lights_in_use.data = bsm->lights_in_use.data | (atoi(tokens[165]) << 8);
                break;
            case 166:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
                bsm->lights_in_use.data = bsm->lights_in_use.data | (atoi(tokens[166]) << 7);
                break;
            case 167:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
                bsm->lights_in_use.data = bsm->lights_in_use.data | (atoi(tokens[167]) << 6);
                break;
            case 168:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
                bsm->lights_in_use.data = bsm->lights_in_use.data | (atoi(tokens[168]) << 5);
                break;
            case 169:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
                bsm->lights_in_use.data = bsm->lights_in_use.data | (atoi(tokens[169]) << 4);
                break;
            case 170:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
                bsm->lights_in_use.data = bsm->lights_in_use.data | (atoi(tokens[170]) << 3);
                break;
            case 171:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
                bsm->lights_in_use.data = bsm->lights_in_use.data | (atoi(tokens[171]) << 2);
                break;
            case 172:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
                bsm->lights_in_use.data = bsm->lights_in_use.data | (atoi(tokens[172]) << 1);
                break;
            case 173:
                bsm->has_partII = 1;
                bsm->has_safety_extension = 1;
                bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
                bsm->lights_in_use.data = bsm->lights_in_use.data | (atoi(tokens[173]) << 0);
                break;
            case 174:
                bsm->has_partII = 1;
                bsm->has_supplemental_extension = 1;
                bsm->suppvehopts |= (SUPPLEMENT_VEH_EXT_OPTION_WEATHER_PROBE);
                bsm->weatheropts = bsm->weatheropts | (1 << 0);
                bsm->statusFront = atoi(tokens[174]);
                break;
            case 175:
                bsm->has_partII = 1;
                bsm->has_supplemental_extension = 1;
                bsm->suppvehopts |= (SUPPLEMENT_VEH_EXT_OPTION_WEATHER_PROBE);
                bsm->weatheropts = bsm->weatheropts | (1 << 0);
                bsm->rateFront = atoi(tokens[175]); // Always 1 per sxxxxi;
                break;
            case 176:
                bsm->has_partII = 1;
                bsm->has_supplemental_extension = 1;
                bsm->suppvehopts |= (SUPPLEMENT_VEH_EXT_OPTION_WEATHER_PROBE);
                bsm->weatheropts = bsm->weatheropts | (1 << 0);
                bsm->wiperopts = bsm->wiperopts | (1 << 1);
                bsm->statusRear = atoi(tokens[176]);
                break;
            case 177:
                printf("Unknown ThrottlePos\n");
                break;
            case 178:
                bsm->has_partII = 1;
                bsm->has_supplemental_extension = 1;
                bsm->suppvehopts = bsm->suppvehopts | (1 << 7);
                bsm->veh.supplemental_veh_data_options.word =
                    bsm->veh.supplemental_veh_data_options.word | (1 << 3);
                bsm->veh.height_cm = atof(tokens[178]) * 100;
                break;
            case 179:
                bsm->has_partII = 1;
                bsm->has_supplemental_extension = 1;
                bsm->suppvehopts = bsm->suppvehopts | (1 << 7);
                bsm->veh.supplemental_veh_data_options.word =
                    bsm->veh.supplemental_veh_data_options.word | (1 << 2);
                bsm->veh.front_bumper_height_cm = atof(tokens[179]) * 100;
                break;
            case 180:
                bsm->has_partII = 1;
                bsm->has_supplemental_extension = 1;
                bsm->suppvehopts = bsm->suppvehopts | (1 << 7);
                bsm->veh.supplemental_veh_data_options.word =
                    bsm->veh.supplemental_veh_data_options.word | (1 << 2);
                bsm->veh.rear_bumper_height_cm = atof(tokens[180]) * 100;
                break;
            case 181:
                bsm->has_partII = 1;
                bsm->has_supplemental_extension = 1;
                bsm->suppvehopts = bsm->suppvehopts | (1 << 7);
                bsm->veh.supplemental_veh_data_options.word =
                    bsm->veh.supplemental_veh_data_options.word | (1 << 1);
                bsm->veh.mass_kg = atoi(tokens[181]);
                break;
            case 182:
                /*
                bsm->has_partII = 1;
                bsm->has_supplemental_extension = 1;
                bsm->suppvehopts = bsm->suppvehopts | (1<<9);
                bsm->VehicleClass = 10;//atoi (tokens[182]) ; being set in conf file for sxxxxi
                */
                break;
            default:
                break;
            }
            a++;
        }
    }

    if (bsm->has_safety_extension == 1)
        bsm->qty_partII_extensions++;
    if (bsm->has_special_extension)
        bsm->qty_partII_extensions++;
    if (bsm->has_supplemental_extension)
        bsm->qty_partII_extensions++;

    if (bsm->events.data == 0) {
        bsm->vehsafeopts &= (1 << 3) - 1;
    }

    if (bsm->lights_in_use.data == 0) {
        bsm->vehsafeopts &= (1 << 0) - 1;
    }

    int size = encode_msg(mc);

    while ((--i) >= 0) {
        if (NULL != tokens[i]) {
            free(tokens[i]);
        }
    }

    free(tokens);
    free(tmp);
    return size;
}
