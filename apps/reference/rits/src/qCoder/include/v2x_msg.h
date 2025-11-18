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

/**
 * @file v2x_msg.h
 * @brief v2x_msg data structure for encoding/decoding the messages
 * across different protocol layers.
 */

#ifndef __V2X_MSG_H__
#define __V2X_MSG_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdbool.h>
#include "asnbuf.h"

typedef enum {
    STACK_ID_SAE = 0,
    STACK_ID_ETSI
} stack_id_e;

typedef enum {
    V2X_PROTO_UNK = 0,
    V2X_PROTO_WAVE2010 = 1,
    V2X_PROTO_WAVE2016 = 2,
    V2X_PROTO_GEONETWORK = 3,
    V2X_PROTO_IPV6 = 4,
    V2X_PROTO_MAX
} v2x_proto_e;

typedef enum {
   BSM_MSG_ID = 0,
   WSA_MSG_ID = 1,
   CAM_MSG_ID = 2,
   DENM_MSG_ID = 3
} msg_id_e;

/** \struct msg_contents
 *  This structure holds all information about the message for encoding/decoding
 */

typedef struct {
    bool decoded;       /**< a message is received and succesfully decoded*/
    stack_id_e stackId; /**< are we running SAE or ETSI stack */
    int msgId;          /**< msgId holds an integer that indicates the message type BSM/TIM/WRA etc*/
    abuf_t abuf;        /**< asn buffer for encoding/decoding */
    int payload_len;    /**< The length of the message payload, changing when it pass through layers */

    /* Layer 2 datalink(not used for C-V2X */
    uint8_t src_mac[6]; /**< source MAC address */
    uint8_t dsg_mac[6]; /**< Destination MAC address */
    uint16_t ethertype; /** host ordered short with full ethertype (WSMP, ETSI GN, etc) */

    /* Layer 3 network (WSMP covers both layer3 and layer 4 */
    void *wsmp;         /** decoded wsmp data, or wsmp to be encoded */
    void *gn;           /** decoded GeoNetwork data, or GeoNetwork data to be encoded */
    char *l3_payload;   /** layer 3 payload start location, for SAE, this is wsmp payload,
                            for ETSI, this is the content after GeoNetwork header. */
    int l3_payload_len;

    /* Layer 4 transport */
    void *btp;          /** decoded(or to be encoded) Basic Transport Protocol data */
    int btp_pkt_type;

    /* No layer 5 and 6 */
    /* Layer 7, application layer */
    void *j2735_msg;       /** decoded(or to be encoded)SAE data, e.g. BSM/TIM, etc */
    uint16_t j2735_msg_id; /** J2735 Message ID (TIM, BSM, BIM, SPAT, MAP, etc) */

    int etsi_msg_id;
    void *cam;             /** decoded(or to be encoded) CAM data. */
    void *denm;            /** decoded(or to be encoded) DENM data */
    void *wsa;             /** decoded(or to be encoded) 1609.3 WSA data */
    void *wra;             /** decoded(or to be encoded) 1609.3 WRA data that is in WSA */

    /* Security */
    void *ieee1609_2data;  /** decoded(or to be encoded) IEEE1609.2 data */
} msg_contents;

#ifdef __cplusplus
}
#endif

#endif

