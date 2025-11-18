/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file wsa.c
 * @brief top level ASN.1 encode/decode APIs for 1609.3 WSA
 */
#include "v2x_msg.h"
#include "SrvAdvMsg.h"

static asn_codec_ctx_t *codec_ctx = 0;

/**
 * decode_as_wsa decode the WSA message.
 *
 * @param[in] mc the message content, mc->abuf contains the buffer to be decoded
 * @return 0 on success or -1 on failure.
 */
int decode_as_wsa(msg_contents *mc) {
    int retVal;
    asn_dec_rval_t rval;
    SrvAdvMsg_t *wsa = NULL;
    if (!mc || !mc->abuf.data) {
        fprintf(stderr, "%s: invalid input\n", __func__);
        return -1;
    }

    // memory will be allocated by uper_decode_compelete if input wsa is null.
    rval = uper_decode_complete(codec_ctx, &asn_DEF_SrvAdvMsg, (void **)&mc->wsa,
            mc->abuf.data, mc->abuf.tail - mc->abuf.data);
    if (rval.code != RC_OK || !mc->wsa) {
        fprintf(stderr, "failed to decode WSA\n");
        return -1;
    }

    /* Only handles WRA */
    wsa = (SrvAdvMsg_t *)mc->wsa;
    if (wsa->body.routingAdvertisement) {
        mc->wra = wsa->body.routingAdvertisement;
    }
    abuf_pull(&mc->abuf, rval.consumed);
    return 0;
}
/**
 * encode the WSA message.
 *
 * @param [in] mc the message content, mc->wsa points to cam data to be encoded.
 * @return encoded length on success, or -1 on failure.
 */
int encode_as_wsa(msg_contents *mc) {
    asn_enc_rval_t rval;
    int ret;

    if (!mc || !mc->wsa || !mc->abuf.data) {
        fprintf(stderr, "%s: invalid input parameters\n", __func__);
        return -1;
    }
    rval = uper_encode_to_buffer(&asn_DEF_SrvAdvMsg, mc->wsa, mc->abuf.data,
            mc->abuf.end - mc->abuf.data);
    if (rval.encoded < 0) {
        fprintf(stderr, "%s: failed to encode WSA %d\n", __func__, rval.encoded);
        if (rval.failed_type) {
            if (rval.failed_type->name) {
                fprintf(stderr, "failed type:%s", rval.failed_type->name);
            }
            if (rval.failed_type->xml_tag) {
                fprintf(stderr, " tag:%s", rval.failed_type->xml_tag);
            }
        }
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

void print_wsa(void *wsa) {
    asn_fprint(stdout, &asn_DEF_SrvAdvMsg, wsa);
}

// call ASN1 function to free wsa struct
void free_wsa(void* wsa) {
    if (wsa) {
        ASN_STRUCT_FREE(asn_DEF_SrvAdvMsg, wsa);
    }
}
