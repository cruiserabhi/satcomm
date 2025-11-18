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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file etsi.c
 * @brief top level ASN.1 encode/decode APIs for ETSI stack.
 */
#include "v2x_msg.h"
#include "CAM.h"
#include "DENM.h"

extern int gVerbosity;

static asn_codec_ctx_t *codec_ctx = 0;

/**
 * decode_as_cam decode the CAM message.
 *
 * @param[in] mc the message content, mc->abuf contains the buffer to be decoded
 * @return 0 on success or -1 on failure.
 */
static int decode_as_cam(msg_contents *mc) {
    int retVal;
    asn_dec_rval_t rval;

    if (!mc || !mc->abuf.data) {
        fprintf(stderr, "%s: invalid input\n", __func__);
        return -1;
    }

    // memory will be allocated by uper_decode_compelete if input cam is null.
    rval = uper_decode_complete(codec_ctx, &asn_DEF_CAM, (void **)&mc->cam,
            mc->abuf.data, mc->abuf.tail - mc->abuf.data);
    if (rval.code != RC_OK || !mc->cam) {
        fprintf(stderr, "failed to decode CAM\n");
        return -1;
    }

    abuf_pull(&mc->abuf, rval.consumed);

    if (gVerbosity > 3) {
        print_cam(mc->cam);
    }
    return 0;
}

/**
 * decode_as_denm decode the DENM message.
 *
 * @param[in] mc the message content, mc->abuf contains the buffer to be decoded
 * @return 0 on success or -1 on failure.
 */
static int decode_as_denm(msg_contents *mc) {
    int retVal;
    asn_dec_rval_t rval;

    if (!mc || !mc->abuf.data) {
        fprintf(stderr, "%s: invalid input\n", __func__);
        return -1;
    }

    rval = uper_decode_complete(codec_ctx, &asn_DEF_DENM, (void **)&mc->denm,
            mc->abuf.data, mc->abuf.tail - mc->abuf.data);
    if (rval.code != RC_OK) {
        fprintf(stderr, "failed to decode CAM\n");
        return -1;
    }

    abuf_pull(&mc->abuf, rval.consumed);

    if (gVerbosity > 3) {
        print_denm(mc->denm);
    }
    return 0;
}

/**
 * decode_as_etsi Decode etsi message, CAM or DENM.
 *
 * @param [in] mc the message content, mc->abuf contains the buffer to be
 * decoded.
 *
 * @return 0 on success or -1 on failure.
 *
 */
int decode_as_etsi(msg_contents *mc) {
    asn_dec_rval_t rval;
    int retVal;
    ItsPduHeader_t *header = NULL;
    
    if (!mc || !mc->abuf.data) {
        fprintf(stderr, "%s: invalid input\n", __func__);
        return -1;
    }
    rval = uper_decode_complete(codec_ctx, &asn_DEF_ItsPduHeader,
            (void **)&header, mc->abuf.data, mc->abuf.tail - mc->abuf.data + 1);

    if (rval.code != RC_OK) {
        fprintf(stderr, "failed to decode ItsPduHeader\n");
        return -1;
    }

    switch(header->messageID) {
        case ItsPduHeader__messageID_cam:
            retVal = decode_as_cam(mc);
            break;
        case ItsPduHeader__messageID_denm:
            retVal = decode_as_denm(mc);
        default:
            fprintf(stderr, "messageID: %lu is not supported", header->messageID);
            retVal = -1;
    }
    ASN_STRUCT_FREE(asn_DEF_ItsPduHeader, header);
    return retVal;
}
/**
 * encode the CAM message.
 *
 * @param [in] mc the message content, mc->cam points to cam data to be encoded.
 * @return encoded length on success, or -1 on failure.
 */
static int encode_as_cam(msg_contents *mc) {
    asn_enc_rval_t rval;
    int ret;

    if (!mc || !mc->cam || !mc->abuf.data) {
        fprintf(stderr, "%s: invalid input parameters\n", __func__);
        return -1;
    }
    rval = uper_encode_to_buffer(&asn_DEF_CAM, mc->cam, mc->abuf.data,
            mc->abuf.end - mc->abuf.data);
    if (rval.encoded < 0) {
        fprintf(stderr, "%s: failed to encode CAM %d\n", __func__, rval.encoded);
        return -1;
    } else {
        if (rval.encoded % 8 == 0) {
            abuf_put(&mc->abuf, rval.encoded/8);
            ret = rval.encoded/8;
        } else {
            abuf_put(&mc->abuf, rval.encoded/8 + 1);
            mc->abuf.tail_bits_left = 8 - (rval.encoded % 8);
            ret = rval.encoded/8 + 1;
        }
    }
    return ret;
}

/**
 * encode the DENM message.
 *
 * @param [in] mc the message content, mc->denm points to denm data to be encoded.
 * @return encoded length on success, or -1 on failure.
 */
static int encode_as_denm(msg_contents *mc) {
    asn_enc_rval_t rval;
    int ret;

    if (!mc || mc->denm || mc->abuf.data) {
        fprintf(stderr, "%s: invalid input parameters\n", __func__);
        return -1;
    }
    rval = uper_encode_to_buffer(&asn_DEF_DENM, mc->denm, mc->abuf.data,
            mc->abuf.tail - mc->abuf.data);
    if (rval.encoded < 0) {
        fprintf(stderr, "%s: failed to encode CAM\n", __func__);
        return -1;
    } else {
        if (rval.encoded % 8 == 0) {
            abuf_put(&mc->abuf, rval.encoded/8);
            ret = rval.encoded/8;
        } else {
            abuf_put(&mc->abuf, rval.encoded/8 + 1);
            mc->abuf.tail_bits_left = 8 - (rval.encoded % 8);
            ret = rval.encoded/8 + 1;
        }
    }
    return 0;
}
/**
 * encode the etsi messages, CAM/DENM so far.
 * @param mc the message content, in which the mc->etsi_msg_id specify what
 * message to encode.
 * @return encoded length on success or -1 on failure.
 */
int encode_as_etsi(msg_contents *mc) {
    switch(mc->etsi_msg_id) {
        case ItsPduHeader__messageID_cam:
            return encode_as_cam(mc);
        case ItsPduHeader__messageID_denm:
            return encode_as_denm(mc);
        default:
            fprintf(stderr, "message id : %d is not supported\n", mc->etsi_msg_id);
    }
    return -1;
}
void print_cam(void *cam) {
    asn_fprint(stdout, &asn_DEF_CAM, cam);
}
void print_denm(void *denm) {
    asn_fprint(stdout, &asn_DEF_DENM, denm);
}
// call ASN1 function to free cam struct
void free_cam(void* cam) {
    if (cam) {
        ASN_STRUCT_FREE(asn_DEF_CAM, cam);
    }
}
// call ASN1 function to free denm struct
void free_denm(void* denm) {
    if (denm) {
        ASN_STRUCT_FREE(asn_DEF_DENM, denm);
    }
}
