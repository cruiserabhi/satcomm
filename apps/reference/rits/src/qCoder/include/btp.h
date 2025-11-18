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
 * @file btp.h
 * @BTP(basic transport protocol) header file, Refer: ETSI EN 302 636-5-1 V2.2.1
 *
 */
#ifndef __BTP_H__
#define __BTP_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include "utils.h"
#include "v2x_msg.h"
/*! \enum
 *  \brief BTP packet type enum.
 */
typedef enum btp_packet_type {
    BTP_PACKET_TYPE_A = 1,
    BTP_PACKET_TYPE_B
} btp_packet_type_e;

struct _btp_hdr_A {
	uint16_t d_port;
	uint16_t s_port;
} PACKED;

struct _btp_hdr_B {
	uint16_t d_port;
	uint16_t dp_info;
} PACKED;

/*! \struct
 *  \brief BTP data service parameters.
 */
typedef struct btp_data {
    btp_packet_type_e pkt_type;     /*!< BTP packet type */
    uint16_t s_port;                /*!< Source port */
    uint16_t d_port;                /*!< Destinaiton port */
    uint16_t dp_info;               /*!< Destination port info */
} btp_data_t;

int btp_encode(msg_contents *mc);
int btp_decode(msg_contents *mc );
#ifdef __cplusplus
}
#endif

#endif
