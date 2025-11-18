/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
/**
 * @file: gn_interna.h
 *
 * @brief: Internal data structure for GeoNetworking.
 */


#ifndef __GN_INTERNAL_H__
#define __GN_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__ ((packed))
#endif
/**
 *  GeoNetwork address struct.
 */
typedef struct PACKED gn_addr {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint16_t m:1,       /*! <1 if address is manually configured, 0 otherwise */
             st:5,      /*! <ITS station type */
             res:10;    /*! <reserved */
#else
    uint16_t res:10,
             st:5,
             m:1;
#endif
    uint8_t  mid[GN_MID_LEN];    /*! <MAC ID */
} gn_addr_t;
/**
 * long position vector(lpv)
 */
typedef struct PACKED gn_lpv {
    gn_addr_t gn_addr;  /*! <GeoNetwork address */
    uint32_t tst;       /*! <time in milliseconds at which
                            the latitude and longitude of the ITS-S were
                            acquired by the GeoAdhoc router.
                        */

    int32_t latitude;   /*! <Latitude of the GeoAdhoc router reference
                            position expressed in 1/10 degree */

    int32_t longitude;  /*! <Longitude of the GeoAdhoc router reference
                           position expressed in 1/10 degree */

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint16_t pai:1,     /*! <Position Accuracy Indicator */
             s:15;      /*! <Speed of the GeoAdhoc router expressed in
                            signed units of 0,01 metre per second */
#else
    uint16_t s:15,
             pai: 1;
#endif
    uint16_t h;         /*! <Heading of the GeoAdhoc router, expressed in
                            unsigned units of 0,1 degree from North */
} gn_lpv_t;

/**
 * short position vector struct
 */
typedef struct PACKED gn_spv {
    gn_addr_t gn_addr;  /*! <GeoNetwork address */
    uint32_t tst;       /*! <time in milliseconds at which
                            the latitude and longitude of the ITS-S were
                            acquired by the GeoAdhoc router.
                        */

    int32_t latitude;   /*! <Latitude of the GeoAdhoc router reference
                            position expressed in 1/10 degree */

    int32_t longitude;  /*! <Longitude of the GeoAdhoc router reference
                           position expressed in 1/10 degree */
} gn_spv_t;

/**
 * Next header type enum contained in basic header.
 */
typedef enum gn_basic_nh_type {
    BASIC_NH_TYPE_ANY = 0,
    BASIC_NH_TYPE_COMMON_HDR,
    BASIC_NH_TYPE_SECURED_PACKET
} gn_basic_nh_type_e;

/**
 * GeoNetwork Basic header struct.
 */
typedef struct PACKED gn_bhdr {

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint8_t version: 4,
            nh :4;      /*! <type of header immediately following the
                           GeoNetworking Basic Header. */
#else
    uint8_t nh: 4,
            version: 4;
#endif
    uint8_t reserved;   /*! <reserved field */
    uint8_t lt;         /*! <Lifetime field. Indicates the maximum tolerable
                           time a packet may be buffered until it reaches
                           its destination. */
    uint8_t rhl;        /*! <Decremented by 1 by each GeoAdhoc router
                           that forwards the packet. */
} gn_bhdr_t;

/**
 * Next header type enum contained in common header.
 */
typedef enum gn_common_nh_type {
    GN_COMMON_NH_TYPE_ANY = 0,
    GN_COMMON_NH_TYPE_BTP_A,
    GN_COMMON_NH_TYPE_BTP_B,
    GN_COMMON_NH_TYPE_IPV6
} gn_common_nh_type_e;

/**
 * common header type enum
 */
typedef enum gn_common_header_type {
    GN_COMMON_HEADER_TYPE_ANY = 0,
    GN_COMMON_HEADER_TYPE_BEACON,
    GN_COMMON_HEADER_TYPE_GEOUNICAST,
    GN_COMMON_HEADER_TYPE_GEOANYCAST,
    GN_COMMON_HEADER_TYPE_GEOBROADCAST,
    GN_COMMON_HEADER_TYPE_TSB,
    GN_COMMON_HEADER_TYPE_LS
}gn_common_header_type_e;

/**
 * Header subtype type depends on header type
 */
typedef enum gn_gac_hst {
	GAC_HST_CIRCLE = 0,
	GAC_HST_RECT,
	GAC_HST_ELIP,
} gn_gac_hst_e;

typedef enum gn_gbc_hst {
	GBC_HST_CIRCLE = 0,
	GBC_HST_RECT,
	GBC_HST_ELIP,
} gn_gbc_hst_e;

typedef enum gn_tsb_hst {
	TSB_HST_SINGLE_HOP = 0,
	TSB_HST_MULTI_HOP,
} gn_tsb_hst_e;

typedef enum gn_ls_hst {
	LS_HST_REQUEST = 0,
	LS_HST_REPLY,
} gn_ls_hst_e;

#define TC_SCF(tc) (tc &0x01)    /* Obtain TC Store-carry forward bit */
#define TC_CO(tc)  ((tc >> 1) & 0x1) /* Obtain TC Channel offload bit */
#define TC_ID(tc)  (tc >> 2)     /* Obtain TC ID */

/**
 *  GeoNetwork comman header struct
 */
typedef struct PACKED gn_chdr{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint8_t nh:       4,    /*!< next header type */
            reserved: 4;    /*!< reserved field */
    uint8_t ht:       4,    /*!< header type */
            hst:      4;    /*!< header sub type */
#else
    uint8_t reserved: 4,
            nh:       4;
    uint8_t hst:      4,
            ht:       4;
#endif
    uint8_t tc;             /*!< traffic class */
    uint8_t flags;          /*!< flags, only bit 0 is used for stationary/mobile indication */
    uint16_t pl;            /*!< payload length */
    uint8_t mhl;            /*!< Maximum hop limit */
    uint8_t res;            /*!< reserved octet */
} gn_chdr_t;

/**
 *  GeoNetwork GUC(GeoUnicast) header struct
 */
typedef struct PACKED gn_guc_hdr {
    gn_bhdr_t bh;   /*!< basic header */
    gn_chdr_t ch;   /*!< common header */
    uint16_t sn;    /*!< sequence number */
    uint16_t reserved;  /*!< reserved */
    gn_lpv_t so_pv;   /*!< source long position vector */
    gn_spv_t de_pv;   /*!< destination long position vector */
} gn_guc_hdr_t;

/**
 *  Topologically-scoped broadcast (TSB) header struct..
 */
typedef struct PACKED gn_tsb_hdr {
    gn_bhdr_t bh;   /*!< basic header */
    gn_chdr_t ch;   /*!< common header */
    uint16_t sn;    /*!< sequence number */
    uint16_t reserved;  /*!< reserved */
    gn_lpv_t so_pv;   /*!< source long position vector */
} gn_tsb_hdr_t;

/**
 *  Single Hop Broadcast (SHB) header struct.
 */
typedef struct PACKED gn_shb_hdr {
    gn_bhdr_t bh;   /*!< basic header */
    gn_chdr_t ch;   /*!< common header */
    gn_lpv_t so_pv; /*!< long position vector of the source */
    uint32_t mdd;   /*!< media dependent data */
} gn_shb_hdr_t;

/**
 * GeoBroadcast or GeoAnycast header struct
 */
typedef struct PACKED gn_gbc_gac_hdr {
    gn_bhdr_t bh;   /*!< basic header */
    gn_chdr_t ch;   /*!< common header */
    uint16_t sn;    /*!< sequence number */
    uint16_t res1;  /*!< reserved */
    gn_lpv_t so_pv; /*!< long position vector of the source */
    int32_t gp_latitude;
    int32_t gp_longitude;
    uint16_t dist_a;
    uint16_t dist_b;
    uint16_t angle;
    uint16_t res2;
} gn_gbc_gac_hdr_t;

/**
 * Beacon header struct
 */
typedef struct PACKED gn_beacon_hdr {
    gn_bhdr_t bh;   /*!< basic header */
    gn_chdr_t ch;   /*!< common header */
    gn_lpv_t so_pv;   /*!< source long position vector */
} gn_beacon_hdr_t;

/**
 * Location Service (LS) REQUEST header struct
 */
typedef struct PACKED gn_lsreq_hdr {
    gn_bhdr_t bh;   /*!< basic header */
    gn_chdr_t ch;   /*!< common header */
    uint16_t sn;    /*!< sequence number */
    uint16_t reserved;  /*!< reserved */
    gn_lpv_t so_pv;     /*!< source long position vector */
    gn_addr_t req_addr; /*!< the requester's gn address */
} gn_lsreq_hdr_t;

/**
 *  Location Service (LS) REPLY header has the same format as GUC header.
 */
typedef gn_guc_hdr_t gn_lsreply_hdr_t;

/*
 * Geo Position Lat/long unit
 */
typedef enum geo_pos_unit
{
	GEO_POS_UNIT_DEGREE = 1,
	GEO_POS_UNIT_MICRO_DEGREE = 1000000,
	GEO_POS_UNIT_TENTH_MICRO_DEGREE = 10000000
}geo_pos_unit_e;

#ifdef __cplusplus
 }
#endif
#endif  /* __GN_INTERNAL_H__ */
