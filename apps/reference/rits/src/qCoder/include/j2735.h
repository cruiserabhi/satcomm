/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _J2735_H_
#define _J2735_H_

#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define PACKED __attribute__ ((packed))

typedef enum {V2X_False = 0, V2X_True = 1, V2X_MAX } v2x_bool_t;

// Creates a mask field for ASN.1 style counting from left to right, n bits starting at bit "startbit" (first param
#define GENMASK32(startbit, n) \
    (unsigned int)(( ((unsigned long long)1 << (32-startbit))  - 1 ) & \
            ~(((unsigned long long)1 << (32-n-startbit )) - 1))

#define GENMASK8(startbit, n) \
    (unsigned int)(( ((unsigned long long)1 << (8-startbit))  - 1 ) & \
            ~(((unsigned long long)1 << (8-n-startbit )) - 1))


//   From  ASN.1 sepc "DSRCmsgID ::= INTEGER (0..32767)"
typedef enum {
    //  DER forms, -- All DER forms are now retired and not to be used, but are seen in the wild from old equipment
    J2735_MSGID_RESERVED_DER                         =  0,   // --'00'H
    J2735_MSGID_ALA_CARTE_DER                        =  1,   // --'01'H ACM -- alaCarteMessage-D is Retired, not to be used
    J2735_MSGID_BASIC_SAFETY_MESSAGE_DER             =  2,   // --'02'H BSM, heartbeat msg
    J2735_MSGID_BASIC_SAFETY_MESSAGE_VERBOSE_DER     =  3,   // --'03'H For testing only
    J2735_MSGID_COMMON_SAFETY_REQUEST_DER            =  4,   // --'04'H CSR
    J2735_MSGID_EMERGENCY_VEHICLE_ALERT_DER          =  5,   // --'05'H EVA
    J2735_MSGID_INTERSECTION_COLLISION_DER           =  6,   // --'06'H ICA
    J2735_MSGID_MAP_DATA_DER                         =  7,   // --'07'H MAP, intersections
    J2735_MSGID_NMEA_CORRECTIONS_DER                 =  8,   // --'08'H NMEA
    J2735_MSGID_PROBE_DATA_MANAGEMENT_DER            =  9,   // --'09'H PDM
    J2735_MSGID_PROBE_VEHICLE_DATA_DER               = 10,   // --'0A'H PVD
    J2735_MSGID_ROAD_SIDE_ALERT_DER                  = 11,   // --'0B'H RSA
    J2735_MSGID_RTCM_CORRECTIONS_DER                 = 12,   // --'0C'H RTCM
    J2735_MSGID_SIGNAL_PHASE_AND_TIMING_DER          = 13,   // --'0D'H SPAT
    J2735_MSGID_SIGNAL_REQUEST_DER                   = 14,   // --'0E'H SRM
    J2735_MSGID_SIGNAL_STATUS_DER                    = 15,   // --'0F'H SSM
    J2735_MSGID_TRAVELER_INFORMATION_DER             = 16,   // --'10'H TIM
    J2735_MSGID_UPER_FRAME_DER                       = 17,   // --'11'H UPER frame

    // --
    // -- UPER forms
    // --

    J2735_MSGID_MAP_DATA                             =   18, // -- MAP, intersections
    J2735_MSGID_SIGNAl_PHASE_AND_TIMING              =   19, // -- SPAT
                                 // -- Above two entries were adopted in the 2015-04 edition
                                 // -- Message assignments added in 2015 follow below

    J2735_MSGID_BASIC_SAFETY                         =   20, // -- BSM, heartbeat msg
    J2735_MSGID_COMMON_SAFETY_REQUEST                =   21, // -- CSR
    J2735_MSGID_EMERGENCY_VEHICLE_ALERT              =   22, // -- EVA
    J2735_MSGID_INTERSECTION_COLLISION               =   23, // -- ICA
    J2735_MSGID_NMEA_CORRECTIONS                     =   24, // -- NMEA
    J2735_MSGID_PROBE_DATA_MANAGEMENT                =   25, // -- PDM
    J2735_MSGID_PROBE_VEHICLE_DATA                   =   26, // -- PVD
    J2735_MSGID_ROAD_SIDE_ALERT                      =   27, // -- RSA
    J2735_MSGID_RTCM_CORRECTIONS                     =   28, // -- RTCM
    J2735_MSGID_SIGNAL_REQUEST                       =   29, // -- SRM
    J2735_MSGID_SIGNAL_STATUS                        =   30, // -- SSM
    J2735_MSGID_TRAVELER_INFORMATION                 =   31, // -- TIM
    J2735_MSGID_PERSONAL_SAFETY                      =   32, // -- PSM

    //-- The Below values are reserved for local message testing use
    // --

    J2735_MSGID_TEST_00                     =  240,  // -- Hex 0xF0
    J2735_MSGID_TEST_01                     =  241,
    J2735_MSGID_TEST_02                     =  242,
    J2735_MSGID_TEST_03                     =  243,
    J2735_MSGID_TEST_04                     =  244,
    J2735_MSGID_TEST_05                     =  245,
    J2735_MSGID_TEST_06                     =  246,
    J2735_MSGID_TEST_07                     =  247,
    J2735_MSGID_TEST_08                     =  248,
    J2735_MSGID_TEST_09                     =  249,
    J2735_MSGID_TEST_10                     =  250,
    J2735_MSGID_TEST_11                     =  251,
    J2735_MSGID_TEST_12                     =  252,
    J2735_MSGID_TEST_13                     =  253,
    J2735_MSGID_TEST_14                     =  254,
    J2735_MSGID_TEST_15                     =  255,  // -- Hex 0xFF

    J2735_MSGID_MAX
} DSRCmsgID_et;



/* Bit dissection sizes that are UPER specific */
/* This MIN_BSM_CORE_OCTETS is just a number of bytes (octets) that we know
 * is the minimum functional BSM -- anything smaller than this is probably not a full/proper SAE J2735 2016 BSM
 * its used to save the effort of parsing, if we know it can't possibly be complete */
#define MIN_BSM_CORE_OCTETS         31
#define PART_II_ID_LEN_BITS         6
#define UPER_SEQUENCE_LEN_BITS      8
#define LATITUDE_LEN_BITS                   31

/* The follow items, are the J2735 ASN's encoding offsets, which are used to translate the integer value into an encoding
 * through arithmatic summation.  These "OFFSET's" are typically the minimum possible encoded value, which typically
 * gets encoded as a "0" in the bitstream */
#define BSM_ASN_LATITUDE_ENCODE_OFFSET      -900000000
#define BSM_ASN_LONGITUDE_ENCODE_OFFSET     -1799999999
#define BSM_ASN_ELEVATION_ENCODE_OFFSET     -4096
#define BSM_ASN_SWA_ENCODE_OFFSET           -126
#define BSM_ASN_ACCEL_LON_ENCODE_OFFSET     -2000
#define BSM_ASN_ACCEL_LAT_ENCODE_OFFSET     -2000

#define BSM_ASN_ACCEL_VERT_ENCODE_OFFSET    -127
#define BSM_ASN_ACCEL_YAW_ENCODE_OFFSET     -32767

#define LONGITUDE_LEN_BITS      (32)
#define HEADING_LEN_BITS        (15)
#define SPEED_LEN_BITS      (13)
#define SEMIMAJOR_ORIENTATION_LEN_BITS (16)
#define SEMIMAJOR_ACCURACY_LEN_BITS (8)
#define SEMIMINOR_ACCURACY_LEN_BITS  (8)
#define ELEVATION_CONFIDENCE_LEN_BITS   (4)


/* These "FAST" mask & shifts  for the J2735 parsing are made to speed up the  parsing and packing of the J2735 messages
 * they are faster to just mask and shift than to treat the bitstream other ways to work with the bit-stream
 * HOWEVER, this only works as long as the word is aligned properly, and no surprise ASN extensions have yet been applied.
 *
 * In theory, on a 64 bit CPU -- this could go even faster with 64 bit registers for masking & Shifting.
 * The GENMASK32() macro is all preprocessor (verified) its just a little cleaner to define this way than to have big
 * hex strings like 0xc0000000, etc.
 *
 * As soon as an extension is detected, you'll have to drop back into parsing the more expensive way, using the asnbuf "get next n bits"
 * methods, and for that you can still use the _BIT_LEN fields enumerated below, as they are still the correct ASN encoded lengths of
 * each field
 */

// Create a 32 bit mask for "n" bits starting from bitn (numbered 0-32 from LEFT to right!)
#define BSM_FAST32_0_EXTENSION_MASK             GENMASK32(0,1)
#define BSM_FAST32_1_OPTIONAL_PART2_MASK        GENMASK32(1,1)
#define BSM_FAST32_2_OPTIONAL_REGIONAL_MASK     GENMASK32(2,1)

// bit #3-10, 7 bit msgCount
#define BSM_FAST32_3_MSG_COUNT_BIT_NUM          3
#define BSM_FAST32_3_MSG_COUNT_BIT_LEN          7
#define BSM_FAST32_3_MSG_COUNT_BIT_MASK         GENMASK32(BSM_FAST32_3_MSG_COUNT_BIT_NUM,BSM_FAST32_3_MSG_COUNT_BIT_LEN)
#define BSM_FAST32_3_MSG_COUNT_BIT_SHIFT        (32- BSM_FAST32_3_MSG_COUNT_BIT_LEN - BSM_FAST32_3_MSG_COUNT_BIT_NUM)

#define BSM_FAST32_10_MSG_ID_BIT_NUM            10
#define BSM_FAST32_10_MSG_ID_BIT_LEN            32
#define BSM_FAST32_10_MSG_ID_SEG1_LEN           (32-BSM_FAST32_10_MSG_ID_BIT_NUM)
#define BSM_FAST32_10_MSG_ID_SEG2_LEN           (BSM_FAST32_10_MSG_ID_BIT_LEN - BSM_FAST32_10_MSG_ID_SEG1_LEN)
#define BSM_FAST32_10_MSG_ID_SEG1_MASK          GENMASK32(BSM_FAST32_10_MSG_ID_BIT_NUM,BSM_FAST32_10_MSG_ID_SEG1_LEN)
#define BSM_FAST32_10_MSG_ID_SEG2_MASK          GENMASK32(0,BSM_FAST32_10_MSG_ID_SEG2_LEN)
#define BSM_FAST32_10_MSG_ID_SEG2_SHIFT         (32-BSM_FAST32_10_MSG_ID_SEG2_LEN)

#define BSM_FAST32_42_SECMARK_BIT_NUM           BSM_FAST32_10_MSG_ID_SEG2_LEN
#define BSM_FAST32_42_SECMARK_BIT_LEN           16
#define BSM_FAST32_42_SECMARK_BIT_MASK          GENMASK32(BSM_FAST32_42_SECMARK_BIT_NUM,BSM_FAST32_42_SECMARK_BIT_LEN)
#define BSM_FAST32_42_SECMARK_BIT_SHIFT         (32- BSM_FAST32_42_SECMARK_BIT_LEN - BSM_FAST32_42_SECMARK_BIT_NUM)


#define BSM_FAST32_58_LAT_BIT_NUM           (32-BSM_FAST32_42_SECMARK_BIT_SHIFT)
#define BSM_FAST32_58_LAT_BIT_LEN           (LATITUDE_LEN_BITS)
#define BSM_FAST32_58_LAT_SEG1_LEN          (32-BSM_FAST32_58_LAT_BIT_NUM)
#define BSM_FAST32_58_LAT_SEG2_LEN          (BSM_FAST32_58_LAT_BIT_LEN - BSM_FAST32_58_LAT_SEG1_LEN)
#define BSM_FAST32_58_LAT_SEG1_MASK         GENMASK32(BSM_FAST32_58_LAT_BIT_NUM,BSM_FAST32_58_LAT_SEG1_LEN)
#define BSM_FAST32_58_LAT_SEG2_MASK         GENMASK32(0,BSM_FAST32_58_LAT_SEG2_LEN)
#define BSM_FAST32_58_LAT_SEG2_SHIFT        (32-BSM_FAST32_58_LAT_SEG2_LEN)

#define BSM_FAST32_89_LON_BIT_NUM           (32-BSM_FAST32_58_LAT_SEG2_SHIFT)
#define BSM_FAST32_89_LON_BIT_LEN           (LONGITUDE_LEN_BITS)
#define BSM_FAST32_89_LON_SEG1_LEN          (32-BSM_FAST32_89_LON_BIT_NUM)
#define BSM_FAST32_89_LON_SEG2_LEN          (BSM_FAST32_89_LON_BIT_LEN - BSM_FAST32_89_LON_SEG1_LEN)
#define BSM_FAST32_89_LON_SEG1_MASK         GENMASK32(BSM_FAST32_89_LON_BIT_NUM,BSM_FAST32_89_LON_SEG1_LEN)
#define BSM_FAST32_89_LON_SEG2_MASK         GENMASK32(0,BSM_FAST32_89_LON_SEG2_LEN)
#define BSM_FAST32_89_LON_SEG2_SHIFT        (32-BSM_FAST32_89_LON_SEG2_LEN)

#define BSM_FAST32_121_ELE_BIT_NUM          ( BSM_FAST32_89_LON_SEG2_LEN )
#define BSM_FAST32_121_ELE_BIT_LEN          (ELEVATION_LEN_BITS)
#define BSM_FAST32_121_ELE_SEG1_LEN         (32-BSM_FAST32_121_ELE_BIT_NUM)
#define BSM_FAST32_121_ELE_SEG2_LEN         (BSM_FAST32_121_ELE_BIT_LEN - BSM_FAST32_121_ELE_SEG1_LEN)
#define BSM_FAST32_121_ELE_SEG1_MASK        GENMASK32(BSM_FAST32_121_ELE_BIT_NUM,BSM_FAST32_121_ELE_SEG1_LEN)
#define BSM_FAST32_121_ELE_SEG2_MASK        GENMASK32(0,BSM_FAST32_121_ELE_SEG2_LEN)
#define BSM_FAST32_121_ELE_SEG2_SHIFT       (32-BSM_FAST32_121_ELE_SEG2_LEN)

#define BSM_FAST32_137_SEMIMAJOR_BIT_NUM            (32- BSM_FAST32_121_ELE_SEG2_SHIFT)
#define BSM_FAST32_137_SEMIMAJOR_BIT_LEN            8
#define BSM_FAST32_137_SEMIMAJOR_BIT_MASK           GENMASK32(BSM_FAST32_137_SEMIMAJOR_BIT_NUM,BSM_FAST32_137_SEMIMAJOR_BIT_LEN)
#define BSM_FAST32_137_SEMIMAJOR_BIT_SHIFT          (32- BSM_FAST32_137_SEMIMAJOR_BIT_LEN - BSM_FAST32_137_SEMIMAJOR_BIT_NUM)


#define BSM_FAST32_145_SEMIMINOR_BIT_NUM            (32 - BSM_FAST32_137_SEMIMAJOR_BIT_SHIFT )
#define BSM_FAST32_145_SEMIMINOR_BIT_LEN            8
#define BSM_FAST32_145_SEMIMINOR_BIT_MASK           GENMASK32(BSM_FAST32_145_SEMIMINOR_BIT_NUM,BSM_FAST32_145_SEMIMINOR_BIT_LEN)
#define BSM_FAST32_145_SEMIMINOR_BIT_SHIFT          (32- BSM_FAST32_145_SEMIMINOR_BIT_LEN - BSM_FAST32_145_SEMIMINOR_BIT_NUM)

#define BSM_FAST32_153_ACC_ORIENT_BIT_NUM           ( 32 - BSM_FAST32_145_SEMIMINOR_BIT_SHIFT)
#define BSM_FAST32_153_ACC_ORIENT_BIT_LEN           16
#define BSM_FAST32_153_ACC_ORIENT_SEG1_LEN          (32-BSM_FAST32_153_ACC_ORIENT_BIT_NUM)
#define BSM_FAST32_153_ACC_ORIENT_SEG2_LEN          (BSM_FAST32_153_ACC_ORIENT_BIT_LEN - BSM_FAST32_153_ACC_ORIENT_SEG1_LEN)
#define BSM_FAST32_153_ACC_ORIENT_SEG1_MASK     GENMASK32(BSM_FAST32_153_ACC_ORIENT_BIT_NUM,BSM_FAST32_153_ACC_ORIENT_SEG1_LEN)
#define BSM_FAST32_153_ACC_ORIENT_SEG2_MASK     GENMASK32(0,BSM_FAST32_153_ACC_ORIENT_SEG2_LEN)
#define BSM_FAST32_153_ACC_ORIENT_SEG2_SHIFT        (32-BSM_FAST32_153_ACC_ORIENT_SEG2_LEN)

#define BSM_FAST32_169_TRANNY_BIT_NUM           (32 - BSM_FAST32_153_ACC_ORIENT_SEG2_SHIFT)
#define BSM_FAST32_169_TRANNY_BIT_LEN           3
#define BSM_FAST32_169_TRANNY_BIT_MASK          GENMASK32(BSM_FAST32_169_TRANNY_BIT_NUM,BSM_FAST32_169_TRANNY_BIT_LEN)
#define BSM_FAST32_169_TRANNY_BIT_SHIFT         (32- BSM_FAST32_169_TRANNY_BIT_LEN - BSM_FAST32_169_TRANNY_BIT_NUM)

#define BSM_FAST32_172_SPEED_BIT_NUM            (32 - BSM_FAST32_169_TRANNY_BIT_SHIFT)
#define BSM_FAST32_172_SPEED_BIT_LEN            13
#define BSM_FAST32_172_SPEED_BIT_MASK           GENMASK32(BSM_FAST32_172_SPEED_BIT_NUM,BSM_FAST32_172_SPEED_BIT_LEN)
#define BSM_FAST32_172_SPEED_BIT_SHIFT          (32- BSM_FAST32_172_SPEED_BIT_LEN - BSM_FAST32_172_SPEED_BIT_NUM)

#define BSM_FAST32_185_HEADING_BIT_NUM          ( 32 - BSM_FAST32_172_SPEED_BIT_SHIFT)
#define BSM_FAST32_185_HEADING_BIT_LEN          15
#define BSM_FAST32_185_HEADING_SEG1_LEN         (32-BSM_FAST32_185_HEADING_BIT_NUM)
#define BSM_FAST32_185_HEADING_SEG2_LEN         (BSM_FAST32_185_HEADING_BIT_LEN - BSM_FAST32_185_HEADING_SEG1_LEN)
#define BSM_FAST32_185_HEADING_SEG1_MASK        GENMASK32(BSM_FAST32_185_HEADING_BIT_NUM,BSM_FAST32_185_HEADING_SEG1_LEN)
#define BSM_FAST32_185_HEADING_SEG2_MASK        GENMASK32(0,BSM_FAST32_185_HEADING_SEG2_LEN)
#define BSM_FAST32_185_HEADING_SEG2_SHIFT       (32-BSM_FAST32_185_HEADING_SEG2_LEN)

#define BSM_FAST32_200_SWA_BIT_NUM              (32 - BSM_FAST32_185_HEADING_SEG2_SHIFT)
#define BSM_FAST32_200_SWA_BIT_LEN              8
#define BSM_FAST32_200_SWA_BIT_MASK             GENMASK32(BSM_FAST32_200_SWA_BIT_NUM,BSM_FAST32_200_SWA_BIT_LEN)
#define BSM_FAST32_200_SWA_BIT_SHIFT            (32- BSM_FAST32_200_SWA_BIT_LEN - BSM_FAST32_200_SWA_BIT_NUM)

#define BSM_FAST32_208_ACC_LON_BIT_NUM          (32 - BSM_FAST32_200_SWA_BIT_SHIFT)
#define BSM_FAST32_208_ACC_LON_BIT_LEN          12
#define BSM_FAST32_208_ACC_LON_BIT_MASK         GENMASK32(BSM_FAST32_208_ACC_LON_BIT_NUM,BSM_FAST32_208_ACC_LON_BIT_LEN)
#define BSM_FAST32_208_ACC_LON_BIT_SHIFT        (32- BSM_FAST32_208_ACC_LON_BIT_LEN - BSM_FAST32_208_ACC_LON_BIT_NUM)

#define BSM_FAST32_220_ACC_LAT_BIT_NUM          (32 - BSM_FAST32_208_ACC_LON_BIT_SHIFT)
#define BSM_FAST32_220_ACC_LAT_BIT_LEN          12
#define BSM_FAST32_220_ACC_LAT_SEG1_LEN     (32-BSM_FAST32_220_ACC_LAT_BIT_NUM)
#define BSM_FAST32_220_ACC_LAT_SEG2_LEN     (BSM_FAST32_220_ACC_LAT_BIT_LEN - BSM_FAST32_220_ACC_LAT_SEG1_LEN)
#define BSM_FAST32_220_ACC_LAT_SEG1_MASK        GENMASK32(BSM_FAST32_220_ACC_LAT_BIT_NUM,BSM_FAST32_220_ACC_LAT_SEG1_LEN)
#define BSM_FAST32_220_ACC_LAT_SEG2_MASK        GENMASK32(0,BSM_FAST32_220_ACC_LAT_SEG2_LEN)
#define BSM_FAST32_220_ACC_LAT_SEG2_SHIFT       (32-BSM_FAST32_220_ACC_LAT_SEG2_LEN)


#define BSM_FAST32_232_ACC_VERT_BIT_NUM     (32 - BSM_FAST32_220_ACC_LAT_SEG2_SHIFT)
#define BSM_FAST32_232_ACC_VERT_BIT_LEN     8
#define BSM_FAST32_232_ACC_VERT_BIT_MASK        GENMASK32(BSM_FAST32_232_ACC_VERT_BIT_NUM, BSM_FAST32_232_ACC_VERT_BIT_LEN)
#define BSM_FAST32_232_ACC_VERT_BIT_SHIFT       (32- BSM_FAST32_232_ACC_VERT_BIT_LEN - BSM_FAST32_232_ACC_VERT_BIT_NUM)

#define BSM_FAST32_240_ACC_YAW_NUM              (32 - BSM_FAST32_232_ACC_VERT_BIT_SHIFT)
#define BSM_FAST32_240_ACC_YAW_LEN              16
#define BSM_FAST32_240_ACC_YAW_MASK         GENMASK32(BSM_FAST32_240_ACC_YAW_NUM,BSM_FAST32_240_ACC_YAW_LEN)
#define BSM_FAST32_240_ACC_YAW_SHIFT            (32- BSM_FAST32_240_ACC_YAW_LEN - BSM_FAST32_240_ACC_YAW_NUM)

#define BSM_FAST32_271_VEH_WIDTH_NUM            15
#define BSM_FAST32_271_VEH_WIDTH_LEN                10
#define BSM_FAST32_271_VEH_WIDTH_MASK           GENMASK32(BSM_FAST32_271_VEH_WIDTH_NUM,BSM_FAST32_271_VEH_WIDTH_LEN)
#define BSM_FAST32_271_VEH_WIDTH_SHIFT          (32- BSM_FAST32_271_VEH_WIDTH_LEN - BSM_FAST32_271_VEH_WIDTH_NUM)


/*
   This one is odd, its actually first half at the LSB of a 32 bit word, but because of  possible length constraints,
   there might only be 8 bits left, so the Mask and SHIFT is based on 8 bits (SEG2 MASK/SHIFT)
   */
#define BSM_FAST32_281_VEH_LEN_BIT_NUM          (32 - BSM_FAST32_271_VEH_WIDTH_SHIFT)
#define BSM_FAST32_281_VEH_LEN_BIT_LEN          12
#define BSM_FAST32_281_VEH_LEN_SEG1_LEN         (32-BSM_FAST32_281_VEH_LEN_BIT_NUM)
#define BSM_FAST32_281_VEH_LEN_SEG2_LEN         (BSM_FAST32_281_VEH_LEN_BIT_LEN - BSM_FAST32_281_VEH_LEN_SEG1_LEN)
#define BSM_FAST32_281_VEH_LEN_SEG1_MASK        GENMASK32(BSM_FAST32_281_VEH_LEN_BIT_NUM,BSM_FAST32_281_VEH_LEN_SEG1_LEN)
#define BSM_FAST32_281_VEH_LEN_SEG2_MASK        GENMASK8(0,BSM_FAST32_281_VEH_LEN_SEG2_LEN)
#define BSM_FAST32_281_VEH_LEN_SEG2_SHIFT       (8-BSM_FAST32_281_VEH_LEN_SEG2_LEN)


#define GNSS_STATUS_LEN_BITS    (8)
#define LIGHTS_IN_USE_LEN_BITS  (9)


#define PartII_Id_vehicleSafetyExt          (0)
#define PartII_Id_specialVehicleExt         (1)
#define PartII_Id_supplementalVehicleExt    (2)

#define PART_II_SAFETY_EXT_OPTION_QTY (4)
#define PART_II_SAFETY_EXT_OPTION_EVENTS (0x8)
#define PART_II_SAFETY_EXT_OPTION_PATH_HISTORY (0x4)
#define PART_II_SAFETY_EXT_OPTION_PATH_PREDICTION (0x2)
#define PART_II_SAFETY_EXT_OPTION_LIGHTS (0x1)

#define PART_II_SAFETY_EXT_EVENTS_LEN_BITS (13)

#define PATH_HISTORY_OPTIONS_QTY             (2)
#define PATH_HISTORY_OPTION_INITALPOSITION  (2)
#define PATH_HISTORY_OPTION_GNSS_STATUS     (1)

#define PATH_HISTORY_CRUMB_SEQUENCE_SIZE_LEN_BITS  (5)
#define PATH_HISTORY_SEQUENCE_SIZE_LEN_BITS (5)

/* Three optional fields in each path history point:
   Speed
   PositionalAccuracy
   CoarseHeading
   */
#define PATH_HISTORY_POINT_OPTIONS_QTY  3
#define PATH_HISTORY_POINT_OPTION_SPEED     (1<<2)
#define PATH_HISTORY_POINT_OPTION_ACCURACY  (1<<1)
#define PATH_HISTORY_POINT_OPTION_HEADING   (1<<0)

/* Sequence of 1 to 23 cumb points */
#define MAX_PATH_HISTORY_POINTS_QTY     23

#define FULLPOSITIONVECTOR_OPTIONS_QTY  (8)
#define FULLPOSITIONVECTOR_OPTION_SPEED_CONFIDENCE  1
#define FULLPOSITIONVECTOR_OPTION_POS_CONFIDENCE    2
#define FULLPOSITIONVECTOR_OPTION_TIME_CONFIDENCE   4
#define FULLPOSITIONVECTOR_OPTION_POS_ACCURACY      8
#define FULLPOSITIONVECTOR_OPTION_SPEED         0x10
#define FULLPOSITIONVECTOR_OPTION_HEADING       0x20
#define FULLPOSITIONVECTOR_OPTION_ELEVATION     0x40
#define FULLPOSITIONVECTOR_OPTION_UTCTIME       0x80

#define DDATETIME_OPTIONS_QTY   7
#define DDATETIME_OPTION_OFFSET     (1<<0)
#define DDATETIME_OPTION_DSECOND    (1<<1)
#define DDATETIME_OPTION_DMINUTE    (1<<2)
#define DDATETIME_OPTION_DHOUR      (1<<3)
#define DDATETIME_OPTION_DDAY       (1<<4)
#define DDATETIME_OPTION_DMONTH     (1<<5)
#define DDATETIME_OPTION_DYEAR      (1<<6)

#define POSITION_CONFIDENCE_LEN_BITS    4
#define ELEVATION_LEN_BITS          16
#define HEADING_CONFIDENCE_LEN_BITS     3
#define SPEED_CONFIDENCE_LEN_BITS       3
#define THROTTLE_CONFIDENCE_LEN_BITS    2
#define TIME_CONFIDENCE_LEN_BITS        6

// length of fields in the Path history Crumbs (Point list of offset)
#define LAT_OFFSET_LEN_BITS         18
#define LON_OFFSET_LEN_BITS         18
#define ELEVATION_OFFSET_LEN_BITS   12
#define TIME_OFFSET_LEN_BITS        16
#define PATH_CRUMB_SPEED_LEN_BITS   13

#define PATH_PREDICTION_RADIUS_LEN_BITS             16
#define PATH_PREDICTION_CONFIDENCE_LEN_BITS             8

/* items for the SpecialVehicleExtensions */
#define SPECIAL_VEH_EXT_OPTIONS_QTY  3
#define SPECIAL_VEH_EXT_OPTION_EMERGENCY_DETAILS      (1<<2)
#define SPECIAL_VEH_EXT_OPTION_EVENT_DESC             (1<<1)
#define SPECIAL_VEH_EXT_OPTION_TRAILER_DATA     (1<<0)

/* items for the Part- II Special Vehicle Emergency Details option  */

#define SPECIAL_VEH_EMERGENCY_DATA_OPTIONS_QTY (2)
#define SPECIAL_VEH_SSP_LEN_BITS    5
#define SPECIAL_VEH_SIREN_LEN_BITS  2
#define SPECIAL_VEH_LIGHTS_USE_LEN_BITS 3
#define SPECIAL_VEH_MULTI_LEN_BITS  2
#define SPECIAL_VEH_EVENT_LEN_BITS  16
#define SPECIAL_VEH_REPONSE_TYPE_LEN_BITS  3
#define EMERGENCY_DATA_OPTION_PRIVILEGED_EVENT      (1<<1)
#define EMERGENCY_DATA_OPTION_RESPONSE_TYPE     (1<<0)

#define SPECIAL_VEH_EVENT_OPTIONS_QTY (5)
#define SPECIAL_VEH_EVENT_OPTION_DESC          (1<<4)
#define SPECIAL_VEH_EVENT_OPTION_PRIOIRTY     (1<<3)
#define SPECIAL_VEH_EVENT_OPTION_HEADINGSLICE     (1<<2)
#define SPECIAL_VEH_EVENT_OPTION_EXTENT              (1<<1)
#define SPECIAL_VEH_EVENT_OPTION_REGIONAL_EXT      (1<<0)
#define SPECIAL_VEH_EVENT_DESC_LEN_BITS             16
#define SPECIAL_VEH_EVENT_PRIORITY_LEN_BITS         8
#define SPECIAL_VEH_EVENT_EXTENT_LEN_BITS           4
#define SPECIAL_VEH_EVENT_OPTION_DESC_COUNT_BITS    3

/* items for the SupplementalVehicleExtensions */
#define SUPPLEMENT_VEH_EXT_OPTIONS_QTY  10
#define SUPPLEMENT_VEH_EXT_OPTION_CLASSIFICATION    (1<<9)
#define SUPPLEMENT_VEH_EXT_OPTION_CLASS_DETAILS     (1<<8)
#define SUPPLEMENT_VEH_EXT_OPTION_VEHICLE_DATA      (1<<7)
#define SUPPLEMENT_VEH_EXT_OPTION_WEATHER_REPORT    (1<<6)
#define SUPPLEMENT_VEH_EXT_OPTION_WEATHER_PROBE     (1<<5)
#define SUPPLEMENT_VEH_EXT_OPTION_OBSTACLE          (1<<4)
#define SUPPLEMENT_VEH_EXT_OPTION_DISABLED_VEH      (1<<3)
#define SUPPLEMENT_VEH_EXT_OPTION_SPEED_PROFILE     (1<<2)
#define SUPPLEMENT_VEH_EXT_OPTION_RTCM              (1<<1)
#define SUPPLEMENT_VEH_EXT_OPTION_REGIONAL_EXT      (1<<0)


#define SUPPLEMENT_VEH_CLASS_LEN_BITS  8

/* items for the Part- II Supplemental VehicleData option  */
#define SUPPLEMENT_VEH_DATA_OPTIONS_QTY (4)
#define SUPPLEMENT_VEH_DATA_OPTION_HEIGHT           (1<<3)
#define SUPPLEMENT_VEH_DATA_OPTION_BUMPERS          (1<<2)
#define SUPPLEMENT_VEH_DATA_OPTION_MASS             (1<<1)
#define SUPPLEMENT_VEH_DATA_OPTION_TRAILER_WEIGHT   (1<<0)

#define SUPPLEMENT_WEATHER_OPTIONS_QTY (3)
#define SUPPLEMENT_WEATHER_AIRTEMP         (1<<2)
#define SUPPLEMENT_WEATHER_AIRTEMP_LEN_BITS     8
#define SUPPLEMENT_WEATHER_AIRPRESSURE              (1<<1)
#define SUPPLEMENT_WEATHER_AIRPRESSURE_LEN_BITS     8
#define SUPPLEMENT_WEATHER_WIPERS   (1<<0)
#define SUPPLEMENT_WEATHER_WIPERS_REAR_STATUS   (1<<1)
#define SUPPLEMENT_WEATHER_WIPERS_REAR_RATE   (1<<0)
#define SUPPLEMENT_WEATHER_WIPEROPT_LEN_BITS   2
#define SUPPLEMENT_WEATHER_WIPER_STATUS_LEN_BITS   4
#define SUPPLEMENT_WEATHER_WIPER_RATE_LEN_BITS   7

/* This is the dissection of the 2016 J2735 as required by J2945/1 (2016)  UPER encoding
   For the coreData component only -- after the message ID and length field
   As soon as one of the "extension " flags indicate the message has beem extended, an alternate set of calculated
   offsets must be used

   Because this code is intended to be run many times per second, on a CPU and resource contsrained system, efficient processing is
   imperative.

NOTE:  we are not using c bit-fields to encode/decode, for several reasons.. one, lack of  protabiltiy, endianess and two,
because the way UPER fills in bits from the MSB, the alignment of larger fields across byte boundries is not traditional

For future expansion, additional sets could use a pre-processor macro for each flavor of future protocols.
*/

#define BSM_FLAG_IS_EXTENDED_OFFSET

/* this enum must match the J2735 2016 version, or whatever you are working with */
typedef enum {
    J2735_TRANNY_NEUTRAL = 0,
    J2735_TRANNY_PARK = 1,
    J2735_TRANNY_FORWARD_GEARS = 2,
    J2735_TRANNY_REVERSE_GEARS = 3,
    J2735_TRANNY_RESERVED1 = 4,
    J2735_TRANNY_RESERVED2 = 5,
    J2735_TRANNY_RESERVED3 = 6,
    J2735_TRANNY_UNAVAILABLE = 7,

    J2735_TRANNY_MAX
} j2735_transmission_state_e;

/* this struct must match the J2735 2016 version, or whatever you are working with */
typedef union {
    struct {
        unsigned unused_padding: 3;
        unsigned rightRear: 1;
        unsigned rightFront: 1;
        unsigned leftRear: 1;
        unsigned leftFront: 1;
        unsigned unavialable: 1;
    } bits;
    uint8_t  data;
} BrakeAppliedStatus_t;

/* this enum must match the J2735 2016 version, or whatever you are working with */
typedef enum {
    J2735_BRAKEBOOST_UNAVAIL = 0,
    J2735_BRAKEBOOST_OFF = 1,      //   Brake boost is not applied
    J2735_BRAKEBOOST_ON = 2,       //   Brakes actively beeing boosted

    J2735_BRAKEBOOST_MAX
} j2735_BrakeBoostApplied_e;

/* this enum must match the J2735 2016 version, or whatever you are working with */
typedef enum {
    J2735_TCS_UNAVAIL = 0,
    J2735_TCS_OFF = 1,         //   TCS is not applied
    J2735_TCS_ON = 2,      //   TCS is on, but not presently engaged
    J2735_TCS_ENGAGED = 3      //   TCS actively beeing engaged
                   // TCS_MAX guard check value can't be part of the enum, since this is used in a 2 bit bitfield later
#define J2735_TCS_MAX 4
} j2735_TractionControlStatus_e;


/* this enum must match the J2735 2016 version, or whatever you are working with */
typedef enum {
    J2735_ABS_Unavailable = 0, // ABS not equipped or status unavailable
    J2735_ABS_Off = 1,         //   ABS is not applied
    J2735_ABS_On = 2,           //   ABS is on, but not presently engaged
    J2735_ABS_Engaged = 3      //   ABS actively beeing engaged on one or more wheels
                   // ABS_MAX guard check value can't be part of the enum, since this is used in a 2 bit bitfield later
#ifndef J2735_ABS_MAX
#define J2735_ABS_MAX   4
#endif
} j2735_AntiLockBrakeStatus_e;

/* this enum must match the J2735 2016 version, or whatever you are working with */
typedef enum {
    J2735_STABILITY_CONTROL_UNAVAILBLE = 0,
    J2735_STABILITY_CONTROL_OFF = 1,       //   Stability Control is not applied
    J2735_STABILITY_CONTROL_ON = 2,        //   Stability Control is on, but not presently engaged
    J2735_STABILITY_CONTROL_ENGAGED = 3        //   Stability Control actively beeing engaged
                           // STABILITY_CONTROL _MAX guard check value can't be part of the enum, since this is used in a 2 bit bitfield later
#define J2735_STABILITY_CONTROL_MAX 4
} j2735_StabilityControlStatus_e;

/* this enum must match the J2735 2016 version, or whatever you are working with */
typedef enum {
    J2735_AUX_BRAKE_UNAVAILBLE = 0, // Vehicle has no Aux Brake equipment, or status unavialable
    J2735_AUX_BRAKE_OFF = 1,        //   Aux is not applied [OFF]
    J2735_AUX_BRAKE_ON = 2,         //   Auxillary braking is engaged  [ON]
    J2735_AUX_BRAKE_RESERVED = 3
#define J2735_AUX_BRAKE_MAX 4
} j2735_AuxBrakeStatus_e;


typedef union {
    struct {

        unsigned unused_padding                                 : 1; // used to align to 16 bits, critical

        j2735_AuxBrakeStatus_e aux_brake_status                 : 2;
        j2735_BrakeBoostApplied_e brake_boost_applied           : 2;
        j2735_StabilityControlStatus_e stability_control_status : 2;
        j2735_AntiLockBrakeStatus_e antilock_brake_status       : 2;
        j2735_TractionControlStatus_e traction_control_status   : 2;

        //BrakeAppliedStatus_t brakes_applied                       : 5 ;   // This is a 5 bit bitfield defined in J2735 dictionary
        unsigned rightRear                                      : 1;
        unsigned rightFront                                     : 1;
        unsigned leftRear                                       : 1;
        unsigned leftFront                                      : 1;
        unsigned unavailable                                    : 1;

    } bits;
    uint16_t word;
} brakeStatus_ut;

/* this enum is for bit shift to unused bit in vehicleeventflags_ut */
typedef enum {
    SHIFT_HAZARDLIGHTS = 0,
    SHIFT_STOPLINEVIOLATION = 1,
    SHIFT_ABSACTIVATED = 2,
    SHIFT_TRACTIONCONTROLLOSS = 3,
    SHIFT_STABILITYCONTROLACTIVATED = 4,
    SHIFT_HAZARDOUSMATERIALS = 5,
    SHIFT_RESERVED = 6,
    SHIFT_HARDBRAKING = 7,
    SHIFT_LIGHTSCHANGED = 8,
    SHIFT_WIPERSCHANGED = 9,
    SHIFT_FLATTIRE = 10,
    SHIFT_DISABLEDVEHICLE = 11,
    SHIFT_AIRBAGDEPLOYMENT = 12
}event_bits_shift_et;

/*
 * this typedef match the J2735 2023 version, or whatever you are working with and
 * this enum is to specify the value of lightsUse in emergencyDetails field in BSM message */
typedef enum {
    J2735_LIGHTS_UNAVAILABLE = 0,
    J2735_LIGHTS_NOT_IN_USE = 1,
    J2735_LIGHTS_IN_USE = 2,
    J2735_YELLOW_LIGHTS= 3,
    J2735_SCHOOL_BUS_LIGHTS = 4,
    J2735_ARROW_SIGNS_ACTIVE = 5,
    J2735_SLOW_MOVING_VEHICLE = 6,
    J2735_FREQ_STOPS = 7
}j2735_LightbarStatus_e;

/*
 * this typedef match the J2735 2023 version, or whatever you are working with and
 * this enum is to specify the value of sirenUse in emergencyDetails field in BSM message */
typedef enum {
    J2735_SIREN_UNAVAILABLE = 0,
    J2735_SIREN_NOT_IN_USE = 1,
    J2735_SIREN_IN_USE = 2,
    J2735_SIREN_RESERVED =3
}j2735_SirenStatus_e;

/*
 * this typedef match the J2735 2023 version, or whatever you are working with and
 * this enum is to specify the value of MultiVehicleResponse in emergencyDetails field in
 * BSM message  */
typedef enum {
    J2735_MULTIVEHICLE_UNAVAILABLE = 0,
    J2735_SINGLE_VEHICLE = 1,
    J2735_MULTIVEHICLE_AVAILABLE = 2,
    J2735_MULTIVEHICLE_RESERVED = 3
}j2735_MultiVehicleResponse_e;

/* this typedef match the J2735 2016 version, or whatever you are working with */
typedef union {
    struct {
        unsigned eventAirBagDeployment : 1;       // (12)
        unsigned eventDisabledVehicle  : 1;       // (11), -- The DisabledVehicle DF may also be sent
        unsigned eventFlatTire         : 1;       // (10),
        unsigned eventWipersChanged    : 1;       // (9),
        unsigned eventLightsChanged    : 1;       // (8),
        unsigned eventHardBraking      : 1;       // (7),
        unsigned eventReserved1        : 1;       // (6),
        unsigned eventHazardousMaterials   : 1;    // (5),
        unsigned eventStabilityControlactivated : 1; // (4),
        unsigned eventTractionControlLoss      : 1;  // (3),
        unsigned eventABSactivated             : 1; // (2),
        unsigned eventStopLineViolation        : 1; // (1), -- Intersection Violation
        unsigned eventHazardLights             : 1; // (0),
        unsigned unused             : 3; // (0),
    }  bits;

    uint16_t  data;

} vehicleeventflags_ut;

typedef struct {
    int xy;
    int elevation;
} pos_confidence_set_t;

typedef struct {
    int heading_confidence;
    int speed_confidence;
    int throttle_confidence;
} motion_confidence_set_t;

typedef struct {
    int semi_major;
    int semi_minor;
    int orientation;

} pos_accuracy_t;

#define DDATETIME_DYEAR_LEN_BITS    12
#define DDATETIME_DMONTH_LEN_BITS   4
#define DDATETIME_DDAY_LEN_BITS     5
#define DDATETIME_DHOUR_LEN_BITS    5
#define DDATETIME_DMINUTE_LEN_BITS  6
#define DDATETIME_DSECOND_LEN_BITS  16
#define DDATETIME_DOFFSET_LEN_BITS  11


typedef uint16_t DYear_t;    // Integer between 0 and 4095
typedef uint8_t DMonth_t;   // Integer # 1 -12
typedef uint8_t DDay_t; // Integer 0 - 31
typedef uint8_t DHour_t;    // Integer 0 - 31
typedef uint8_t DMinute_t;  // Integer 0 - 60
typedef uint16_t DSecond_t; // Integer 0 - 65535 Milliseconds
typedef int DOffset_t;    // Integer from -840 to 840 indicunt minutes off UTC
#define J2735_DOFFSET_MIN (-840)
#define J2735_DOFFSET_MAX (+840)


/* 2016 J2735 ASN encodes the following as a sequnce of 7 optional members
 * This structure is essentially the contents of the J2735 "DDateTime" with
 a union  callsed "options" to
 */
typedef struct {
    union {
        uint8_t byte;
        struct {
            unsigned has_offset : 1;
            unsigned has_second : 1;
            unsigned has_minute : 1;
            unsigned has_hour : 1;
            unsigned has_day : 1;
            unsigned has_month : 1;
            unsigned has_year : 1;
            // Only 7 optional fields,  1 MSb should never be used
            unsigned unsused_padding : 1;
        } bits;
    } opts;  // 7 LSB bits inidcating which items are present

    DYear_t year;
    DMonth_t month;
    DDay_t  day;
    DHour_t hour;
    DMinute_t minute;
    DSecond_t second;   // Actually milliseconds, but J2735 calls it "DSecond"
    DOffset_t offset;   // time zone adjustment in +/- minutes from UTC

} DDateTime_t; // J2735 DDateTime element -- non extendable

/*
   From the 2016 J2735 ASN:
   FullPositionVector ::= SEQUENCE {
   utcTime             DDateTime OPTIONAL,   -- time with mSec precision
   long                Longitude,            -- 1/10th microdegree
   lat                 Latitude,             -- 1/10th microdegree
   elevation           Elevation  OPTIONAL,  -- units of 0.1 m
   heading             Heading OPTIONAL,
   speed               TransmissionAndSpeed OPTIONAL,
   posAccuracy         PositionalAccuracy OPTIONAL,
   timeConfidence      TimeConfidence OPTIONAL,
   posConfidence       PositionConfidenceSet OPTIONAL,
   speedConfidence     SpeedandHeadingandThrottleConfidence OPTIONAL,
   ...
   }
   */

typedef struct {
    union {
        uint8_t byte;
        struct {
            unsigned has_motion_cofidence : 1;
            unsigned has_pos_cofidence : 1;
            unsigned has_time_cofidence : 1;
            unsigned has_pos_accuracy : 1;
            unsigned has_speed : 1;
            unsigned has_heading : 1;
            unsigned has_elevation : 1;
            unsigned has_utcTime : 1;
        } bits;
    } opts;

    DDateTime_t utcTime;
    int lon;        // Mandatory field -- Lat/Lon always in every FullPositionVector
    int lat;
    int elevation;
    int heading;
    int speed;
    pos_accuracy_t pos_accuracy;
    int time_confidence;
    pos_confidence_set_t pos_confidence;
    motion_confidence_set_t motion_confidence_set;

} full_position_vector_t;

typedef struct {
    int accuracy_placeholder; // KABOB
} PositionalAccuracy_t;


// GNSSstatus ::= BIT STRING {  8 bits of bifield flags }

typedef union {
    struct {

        unsigned unavailable               : 1; //(0), -- Not Equipped or unavailable
        unsigned isHealthy                 : 1; //(1),
        unsigned isMonitored               : 1; //(2),
        unsigned baseStationType           : 1; //(3), -- Set to zero if a moving base station,
                            //     -- or if a rover device (an OBU),
                            //     -- set to one if it is a fixed base station

        unsigned aPDOPofUnder5             : 1; //(4), -- A dilution of precision greater than 5
        unsigned inViewOfUnder5            : 1; //(5), -- Less than 5 satellites in view
        unsigned localCorrectionsPresent   : 1; //(6), -- DGPS type corrections used
        unsigned networkCorrectionsPresent : 1; //(7)  -- RTK type corrections used

    }  bits;

    uint8_t  data;

} GNSSstatus_ut;



// Special encoding value that encodes to mean heading is not available
#define COARSE_HEADING_LEN_BITS                 8
#define MICRO_DEGREES_PER_COARSE_HEADING_LSB    1500
#define COARSE_HEADING_UNAVAILABLE              240
#define LAT_OFFSET_MIN_VALUE                    -131072
#define LON_OFFSET_MIN_VALUE                    -131072
#define ELE_OFFSET_MIN_VALUE                    -2048
#define TIME_OFFSET_MIN_VALUE                   1

// There are 1-22 of these in each Path History section of a BSM
typedef struct {
    union {
        struct {
            unsigned has_heading : 1;
            unsigned has_pos_accuracy : 1;
            unsigned has_speed : 1;
            unsigned unused_padding: 5;
        }    bits;
        uint8_t byte;
    } opts_u;


    int latOffset; // 18 bit Latitude offset (from initialPosition)
    int lonOffset;  // 18 bit longitude offset (from initialPosition)
    int eleOffset; // 12 bit elevation offset
    int timeOffset_ms;
    // though encoded UPER=LSB in units of 10mSec, 65535 means time offset unavailable

    // The following 3 items are not expected, as per 2016 J2945/1
    int speed;      // OPTIONAL; encoded as 13 bits, from LSB 0.2 meters per sec
    pos_accuracy_t accy; // OPTIONAL; Positional Accuracy set

    uint32_t    heading_microdegrees;  // OPTIONAL

    v2x_bool_t heading_available;

} ph_point_t;

typedef struct {
    full_position_vector_t initialPosition;
    GNSSstatus_ut gnss_status;
    int qty_crumbs;     // From 1 to 23 points
    ph_point_t ph_crumb[MAX_PATH_HISTORY_POINTS_QTY];   // the PathHistoryPointList

} path_history_t;


#define VEHICLE_DATA_HEIGHT_LEN_BITS            7
#define VEHICLE_DATA_HEIGHT_CM_PER_LSB          5
#define VEHICLE_DATA_BUMPER_HEIGHT_LEN_BITS     7
#define VEHICLE_DATA_MASS_LEN_BITS              8
#define VEHICLE_DATA_TRAILER_WEIGHT_LEN_BITS    16

// From the 2016 ASN.1  , this is a specially encoded value
#define VEHICLE_MASS_UNKNOWN -1
#define VEHICLE_MASS_UNKNOWN_ASN_ENCODING    (255)

#define VEHICLE_MASS_OVER_170MT (170001)
#define VEHICLE_MASS_OVER_170MT_ASN_ENCODING (254)
/*
   returns the number of KG, Unkown, or VEHICLE_MASS_OVER_170MT
   from the ASN.1 encoded 8 bit value
   */
static inline int VehicleMassDecode(uint8_t  n)
{
    int kg = 0;

    if (n == VEHICLE_MASS_UNKNOWN_ASN_ENCODING) {
        kg = VEHICLE_MASS_UNKNOWN;
    } else if (n == VEHICLE_MASS_OVER_170MT_ASN_ENCODING) {
        kg = VEHICLE_MASS_OVER_170MT;
    } else {

        if (n > 200) {
            kg = 2000 *  (n - 200);
            n = 200;
        }

        if (n > 80) {
            kg += 500 *  (n - 80);
            n = 80;
        }

        kg += n * 50;

    }

    return (kg);
}

/* Inverse operation -- Given a weight in KG, or VEHICLE_MASS_UNKNOWN
   calculate the 8 bit ASN.1 encoded value
   returns the 8 bit encoding of the "Gross mass of Vehicle and contents, if known"
   */

static inline uint8_t  VehicleMassEncode(int mass_kg)
{
    uint8_t enc_val;

    if (mass_kg < 0) {
        enc_val = VEHICLE_MASS_UNKNOWN_ASN_ENCODING;
    } else if (mass_kg >= VEHICLE_MASS_OVER_170MT) {
        // Special case for over 170  Metric ton (170000 KG)
        enc_val = VEHICLE_MASS_OVER_170MT_ASN_ENCODING;
    } else {
        if (mass_kg <= (80 * 50)) {
            enc_val = mass_kg / 50;
        } else if (mass_kg <= ((80 * 50) + ((200 - 81) * 500))) {
            // Half Ton per step past 80, after subtracting the first 80x50 kg steps
            enc_val = 80 + ((mass_kg - (80 * 50)) / 500);
        } else {
            enc_val = 200 + ((mass_kg - ((80 * 50) + ((200 - 80) * 500))) / 2000);
        }
    }

    return (enc_val);
}



typedef struct {
    //define SUPPLEMENT_VEH_DATA_OPTIONS_QTY (4) optionally present fields
    union {
        struct {
            unsigned    has_trailer_weight  : 1;
            unsigned    has_mass            : 1;
            unsigned    has_bumpers_heights : 1;
            unsigned    has_height          : 1;
            unsigned    unused_padding :
            (8 - SUPPLEMENT_VEH_DATA_OPTIONS_QTY);
        } bits;
        uint8_t word;
    } supplemental_veh_data_options;


    uint32_t height_cm;  // Vehiclese Height
    uint32_t front_bumper_height_cm;
    uint32_t rear_bumper_height_cm;
    uint32_t mass_kg;   // Encoded/DEcoded to/from ASN special sliding scale
    uint32_t trailer_weight;     // 16 bit value between 0 and 64255 accordding to ASN.1

} vehicle_data_t;



//  ExteriorLights ::= BIT STRING {

typedef union {
    struct {
        // 9 bits, per 2016 J2735 standard
        //-- All lights off is indicated by no bits set

        unsigned parkingLightsOn           : 1;     // (8)
        unsigned fogLightOn                : 1;     // (7),
        unsigned daytimeRunningLightsOn    : 1;     // (6),
        unsigned automaticLightControlOn   : 1;     // (5),
        unsigned hazardSignalOn            : 1;     // (4),
        unsigned rightTurnSignalOn         : 1;     // (3),
        unsigned leftTurnSignalOn          : 1;     // (2),
        unsigned highBeamHeadlightsOn      : 1;     // (1),
        unsigned lowBeamHeadlightsOn       : 1;     // (0),
        unsigned unused             : 7; // (0),
    }  bits;

    uint16_t  data;

} exteriorlights_ut;

typedef struct {
    int sspRights;  // 5 bits
    int event; // 16 bits
} PrivilegedEvents;

typedef struct {
    int sspRights;  // 5 bits
    int sirenUse;   // 2 bits
    int lightsUse;  // 3 bits
    int multi;      // 2 bits
    PrivilegedEvents events;
    int responseType;    // 3 bits

} EmergencyDetails;

typedef struct {
    int typeEvent;
    int size_desc;
    int desc[8];
    int priority; // 8 bits
    int heading;    // 16 bits
    int extent;     // 4 bits
    int size_reg;
    int regional[4];

} EventDescription;

typedef struct {
    int isDolly; // 1 bit
    int width; // 10 bits
    int length; // 12 bits
    int height; // 7 bits
    int mass; // 8 bits
    int frontbumperht; // 7 bits
    int rearbumperht; // 7 bits
    int cog; // 7 bits

} tr_unit_desc;

typedef struct {
    int sspRights;  // 5 bits
    int pivotOffset; // 11 bits
    int pivotAngle;  // 8 bits
    int pivots; // 1 bit. boolean
    int size_trailer;
    tr_unit_desc units[8];

} TrailerData;


// The RADIUS_MIN_OFFSET is what a 0 value in the PER/UPER ASN encoding corresponds to, the lower value
#define PATH_RADIUS_MIN_OFFSET  -32767
#define PATH_RADIUS_STRAIGHT    32767
typedef struct {
    v2x_bool_t is_straight;
    signed int radius; // Radius of Curve in unis of 10cm
    uint8_t confidence;  // LSB units of 0.5 percent range 0 to 200
} path_prediction_t;

/* This structure is  for storing in fast access units the contents of a BSM, but not meant for over the air
   they are in only partially encoded/decoded states-- no scaling multiply divide operations are done on decode this form,
   nor before they go over the air.  only interger offests to convert to signed values.  This is to avoid having to scale
   through mutiply/divide more than once.. Presumably the Ego vehicle data from GNSS/ ADAS will to a single floating point operation
   to make that scale to the appropriate J2735 units

   On Transmit, the encoding/packing of this structure into a UPER BSM is done on the elements of this structure.
   this is BSM structure is not used to encode/decode exactly, not is it intended to be the exact order/size/payload
   of an encoded BSM, but rather just store the various attributes in a programmer efficient storage way. (Unpacked)

*/

typedef struct {

    uint64_t timestamp_ms;      // UTC Timestamp in milliseconds when bsm was creatd. computed from secmark_ms
    unsigned int MsgCount;      // Ranges from 1 - 127 in cyclic fashion.
    unsigned int id;            // 32 bit identifier
    unsigned int secMark_ms;    // No of milliseconds in a minute
    signed int   Latitude;      // Degrees * 10^7
    signed int   Longitude;     // Degrees * 10^7
    signed int   Elevation;     // Meters * 10

    unsigned int SemiMajorAxisAccuracy;         // val * 20
    unsigned int SemiMinorAxisAccuracy;         // val * 20
    unsigned int SemiMajorAxisOrientation;      // val/0.0054932479

    j2735_transmission_state_e TransmissionState;   // P,R,N,D,L (park etc..)
    unsigned int Speed;                     // value (in kmph) * 250/18
    unsigned int Heading_degrees;           // value (in degrees) / 0.0125
    signed int   SteeringWheelAngle;        // value (in degree) / 1.5
    signed int   AccelLon_cm_per_sec_squared;       // value (in m/sec2) / 0.01
    /*
       -- LSB units are 0.01 m/s^2
       -- the value 2000 shall be used for values greater than 2000
       -- the value -2000 shall be used for values less than -2000
       -- a value of 2001 shall be used for Unavailable
       */

    signed int   AccelLat_cm_per_sec_squared;       // value (in m/sec2) / 0.01
    signed int   AccelVert_two_centi_gs;            // value in .02 G steps
    signed int   AccelYaw_centi_degrees_per_sec;        // value in degrees per second /0.01

    brakeStatus_ut brakes;

    unsigned int VehicleWidth_cm;                   // units are 1 centimeter, at widest point, 0=unavailable
    unsigned int VehicleLength_cm;                  // units are 1 centimeter, 0=unavailable

    /* Vehicle Safety Extensions */
    v2x_bool_t  has_partII;
    int  qty_partII_extensions;             // From 1 to 7
    v2x_bool_t  has_regional_extensions;
    v2x_bool_t  has_safety_extension;
    v2x_bool_t  has_special_extension;
    v2x_bool_t  has_supplemental_extension;
    int vehsafeopts;
    int phopts;
    path_history_t ph;
    path_prediction_t pp;
    vehicleeventflags_ut events;
    exteriorlights_ut lights_in_use;

    /* Special Vehicle Extensions */
    int specvehopts;
    uint32_t edopts;
    uint32_t eventopts;
    EmergencyDetails vehicleAlerts;
    EventDescription description;
    TrailerData trailers;

    /* Supplemental Vehicle Extensions */
    int suppvehopts;
    int VehicleClass;
    vehicle_data_t veh;     //VehicleData carried by ASN Part II Supplement extension
    uint32_t weatheropts;
    uint32_t wiperopts;
    int airTemp;    // 8 bits
    int airPressure;    // 8 bits
    int rateFront;  // 7 bits
    int rateRear; // Optional 7 bits
    int statusFront;
    int statusRear; // Optional

} bsm_value_t;

#define ELEVATION_UNKOWN    -4096


/* Definitions used from J2735 ASN.1 to encode the range into the bit sizes available for each field
   In the interest of CPU efficiency, the encode/dcode between the strcut bsm_value_t and the UPER is fairly
   "hand-coded"  But all the magic numbers related to this process are collected below,
   in order that the data elements appear in the BSM


   In general:
   _MAX is the maxium value encodable
   _BITS is the length of the field encoding in bits
   _OFFSET_BYTE is the offset from some specified beginning into which byte, starting from 0, the field (named in prefix) starts
   _OFFSET_BITS, where in the OFFSET_BYTE, the filed starts, counting from 7 being MSB, 0 being LSB
   */

#define MSGCOUNT_MAX    127
#define MSGCOUNT_BITS   7
#define MSGCOUNT_OFFSET_BYTE    (0)
#define MSGCOUNT_OFFSET_BITS    (4)
#define MSGCOUNT_OFFSET_MASK_1_of_2 (0x1f)


#if 0
#pragma message "content of MSGCOUNT_OFFSET_MASK_1_OF_2: " STR(MSGCOUNT_OFFSET_MASK_1_OF_2)

static_assert(MSGCOUNT_OFFSET_MASK_1_OF_2 == 0x1f, err_msg(MSGCOUNT_OFFSET_MASK_1_OF_2));

#if MSGCOUNT_OFFSET_MASK_1_of_2 !=  (0x1f)
#warning " ERROR calculating MSGCOUNT_OFFSET_MASK 1 of 1"
#endif


#define MSGCOUNT_OFFSET_MASK_2_of_2  (0xFF ^ ((1<<(8-(MSG_COUNT_BITS-(1+MSGCOUNT_OFFSET_BITS))))-1))

#if MSGCOUNT_OFFSET_MASK_2_of_2 !=  (0xc0)
#warning " ERROR calculating MSGCOUNT_OFFSET_MASK 2"
#endif

#endif

extern int encode_as_j2735(msg_contents *mc);
extern int  decode_as_j2735(msg_contents *mc);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _J2735_H_
