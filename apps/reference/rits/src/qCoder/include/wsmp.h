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
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WSMP_H_
#define _WSMP_H_
#include "utils.h"

#ifdef __cplusplus
extern "C"
{
#endif
//struct msg_contents;
/**
 * There are at least three versions of WSMP protocols out in the wild
 * although going forward, the 2016 (Ver #3) is expected to be used as per J2945/1
 * but there are stacks and messages over the air from vehicles and RSE out in
 * the wild which still send the older formats
 */

typedef enum {
    WSMP_PROTO_VER1_PRE_2010_POC    = 1,
    WSMP_PROTO_VER2_2010            = 2,
    WSMP_PROTO_VER3_2016            = 3,
    WSMP_PROTO_MAX_SUPPORTED
} WSMP_PROTO_VER_e;

typedef enum {
    WSMP_SUBTYPE_NULL_NETWORK = 0,
    WSMP_SUBTYPE_ITS_STA_INTERNAL_FWD = 1,
    WSMP_SUBTYPE_N_HOP_FWD = 2,
    WSMP_SUBTYPE_GEONETWORKING = 3,
    WSMP_SUBTYPE_START_RESERVED = 4,
    WSMP_SUBTYPE_MAX_RESERVED = 15
} WsmpSubtype_e;
/**
 * IEEE1609 does not techncically limit how many bytes a PSID may extend into,
 * for now it is realistically capped at 4 bytes here, and in the real world.
 */
#define PSID_LEN_MAX                4
#define MAX_PSID            (0x1020407f)
#define WSMP_ABUF_DEFAULT_SIZE 8000
#define WSMP_ABUF_DEFAULT_HEADROOM 20

/**
 *  see IEEE 1609.3 annex E  -- or Annex F in the 2016 standard
 */
#define WAVE_ELEM_ID_PWR        4
#define WAVE_ELEM_ID_CHAN       15
#define WAVE_ELEM_ID_RATE       16
#define WAVE_ELEM_ID_LOAD       23

/**
 * WAVE_ELEM_ID_DATA is obsolete after 2010 version of IEEE1609, and WEID  #128
 * is now "Reserved"
 */
#define WAVE_ELEM_ID_DATA       128

/**
 * 129 is obsolete as of IEEE1609.3 standard, but it is still observed in
 * some older senders, so seen ocassionally OTA in the wild
 */
#define WAVE_ELEM_SAFETY_FLAG    129

/**
 * Identity Supplement  used in the 2010 version of 1609.3, but no longer.
 * seen in the wild, but 2016 marks WEID #129 as "Reserved"
 */
#define WAVE_IDENTITY_SUPPLEMENT    130

#define MAX_WAVE_ELEMENTS   6

typedef enum {
    EXT_FIELD_RATE = 0,
    EXT_FIELD_CHAN,
    EXT_FIELD_PWR,
    EXT_FIELD_LOAD,
    EXT_FIELD_MAX
} ext_field_e;

/* PSID: IEEE1609.12 only small portion that is needed are defined here */
typedef enum {
    PSID_SYSTEM = 0,
    PSID_EFC,
    /* 0x20 -0x22, 0x80 - 0x85 are defined for SAE J2735 */
    PSID_BSM = 0x20,
    PSID_SPAT = 0x82,
    PSID_MAP = 0x204097,
    PSID_WSA = 0x87,
} PSID_e;

/**
 * Transport TPID  -- an 8 bit field in IEEE 1609 (2016), but only 6 types
 * defined, #6-255=Reserved function is described in Tabled 23 of standard.
 */
typedef enum {
    TPID_PSID_NOEXT =       0,
    TPID_PSID_EXT =         1,
    TPID_ITS_PORTS_NOEXT =  2,
    TPID_ITS_PORTS_EXT  =   3,
    TPID_LPP_NOEXT  =       4,
    TPID_LPP_EXT    =       5,
    TPID_RESERVED_6 =       6,
    TPID_RESERVED_MAX =     255
} TPID_e;

typedef union {
    struct {
        TPID_e  id : 8;
    };
    uint8_t octet;
} tpid_ut;


/**
 * Binary flags, indicating which optional WEID extensions were present on a
 * recevied WSMP or should be included on building/encoding of a WSM.
 */
typedef struct {
    unsigned inc_rate_ext : 1;
    unsigned inc_chan_ext : 1;
    unsigned inc_pwr_ext  : 1;
    unsigned inc_load_ext : 1;
} wsmp_optional_weids_t;

/**
 * The WSMP_EXT_FIELD_SIZE() is a macro that is based on the typedef union/struct
 * "wsmp_ext_field_t" that selects the members, and adds up the length -- as of
 * now, the result is always 3 bytes, as each WSIE has a 1 byte ID, a 1 byte
 * length and a 1 byte payload -- at least for the ones in WSM headers the
 * extensions in IEEE 1609 WSA's are longer. This macro also will breakdown for
 * variable length WEID's like the "Channel Load" which come in with 2016 version
 * Macro's for unpacking a WSMP Extension field  , as per IEEE 1609. sec 8.1.1.
 */
#define WSMP_EXT_FIELD_SIZE(x) (sizeof((x)->wave_element_id) + sizeof((x)->length) +(x)->length)

/**
 * determine size of PSID based on the first octet of the PSID
 * 0xxx        1
 * 10xx        2
 * 110x        3
 * 1110        4
 */
#define WSMP_PSID_FIELD_SIZE(val) (((val) & 0xe0) == 0xe0 ? \
     4 : ((val) & 0xc0) == 0xc0 ? 3 : ((val) & 0x80) == 0x80 ? 2 : 1 )

typedef union {
    struct {
        unsigned int version:           3;
        unsigned int option_indicator:  1;
        WsmpSubtype_e subtype:          4;
    };

    uint8_t data;

} WSMP_N_HEADER_t;

/**
 * Structure used for communicating src/dst port of ITS port extension to
 * IEEE 1609.3, these are shorts are in host order
 */
typedef struct wsmp_t_header_port_info {
    uint16_t src_port;
    uint16_t dst_port;
} wsmp_t_header_port_info_t;

/**
 * IEEE 1609.3, 8.3.4 extension fields are optional, and intended for the recipient
 * Valid fields are data-rate, channel, and tx-power, and are intended to be
 * specified on a per-packet basis.  These fields are optional and not required
 * but may be sent, and are valid for WSA and WSMP headers
 */
typedef struct wsmp_ext_field {
    uint8_t wave_element_id;
    uint8_t length;             // length of the data field
    union {
        uint8_t data[0];        // generic data, of length "length"
        uint8_t data_rate;      // these are predefined in Annex H
        uint8_t tx_power;
        uint8_t channel;
        uint8_t safety_control[1]; // obsolete field, yet seen OTA still
    } data;
} PACKED wsmp_ext_field_t;

/*  ** NOTE ** , this wave_element_field_t only has meaning in pre 2016 IEEE 1609.3
   In later standards, the fields are in a different order, and this structure cannot be used
*/
typedef struct wave_element_field {
    uint8_t wave_element_id;
    union {
        uint8_t  one_octet;     // SVM  -- see 1609.3 sec 8.1.3
        uint16_t two_octet;     // used if length over 128bytes
    } data_len;
    uint8_t  data[0];               //variable length data field
} PACKED wave_element_field_t;

/**
 * wsmp_data_t type is for storing the WSMP payload & header  when working with
 * the 2016 version.  it has at its core an asn abuf structure, but also
 * contains the fields for requesting the optional WSM WEID's, PSID, rate, etc.
 */

typedef struct {
    WSMP_PROTO_VER_e protoVersion;

    WSMP_N_HEADER_t n_header;
    wsmp_t_header_port_info_t ports;
    wsmp_optional_weids_t weid_opts;
    uint8_t weid_qty;   /* set based on number bits on in weid_opt,
                         also sent OTA in WSMP header frame */
    uint8_t rate;
    uint8_t chan;
    uint8_t pwr;

    tpid_ut tpid;
    uint32_t psid;

    uint8_t *chan_load_ptr;
    uint32_t chan_load_len;
    char *payload;      /* payload pointer after wsmp header */
    int payload_len;    /* payload length after wsmp header */

    abuf_t *abp;        /* The ASN buffer that contains the payload, used for encoding */
} wsmp_data_t;

/**
 * encode wsmp packet.
 *
 * @param [in] buf input buffer contains the wsmp payload(already encoded)
 * @param [in] db buffer contain packet data to be encoded.
 * @returns 0 on success.
 */
extern int wsmp_encode(msg_contents *mc);


/**
 * @brief Encodes wsm packet using wsmp data structure.
 *
 * @param wsmp data structure that contains wsmp data variables.
 * @return int 0 on success, else fail
 */
extern int wsmp_data_encode(wsmp_data_t* wsmp);

/**
 * @brief Decodes wsm packet using wsmp data structure.
 *
 * @param wsmp data structure that contains wsmp data variables.
 * @return int 0 on success, else fail
 */
extern int wsmp_data_decode(wsmp_data_t* wsmp);

/**
 * decode wsmp packet.
 *
 * @param [in] db pointer to the buffer that contains the incoming packet
 * @return 0 on success.
 *
 * @note upon succesful decoding, the bp->wsmp will contain the decoded wsmp
 * contennt
 */
extern int wsmp_decode(msg_contents *mc);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _WSMP_H_
