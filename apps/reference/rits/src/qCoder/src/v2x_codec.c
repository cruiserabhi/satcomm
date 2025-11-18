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
 *  Copyright (c) 2021, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file v2x_codec.c
 * @brief top-level ASN.1 encode/decode APIs
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "v2x_codec.h"

int gVerbosity = 0;
void set_codec_verbosity(int value) {
    if(value)
        printf("Codec verbosity will be set to: %d\n", value);
    gVerbosity = value;
}
/**
 * decode_msg top-level codec API, decode message stored in mc->abuf and
 * populate the corresponding data structure in mc.
 *
 * @param[in] mc the message content, mc->abuf contains the buffer to be
 * decoded.
 * @return 0 success, whole message is decoded succesfully.
 *         1 The message contains Signed/Encrypted PDU, need security service
 *           for verification before continuing decoding.
 *         -1 failure.
 */
int decode_msg(msg_contents *mc)
{
    int ret = 0;
    wsmp_data_t *wsmpp;
    if (!mc || !mc->abuf.data) {
        if(gVerbosity)
            fprintf(stderr, "%s: invalid input\n", __func__);
        return -1;
    }
    if (mc->stackId == STACK_ID_SAE) {
        // skip one byte C-V2X family ID
        abuf_pull(&mc->abuf, 1);

        if ((ret = wsmp_decode(mc)) < 0) {
            if(gVerbosity)
                fprintf(stderr, "WSMP decode failure\n");
            return ret;
        }
        wsmpp = (wsmp_data_t *)mc->wsmp;
        if ((ret = ieee1609_2_decode_unsecured(mc)) < 0) {
            if(gVerbosity)
                fprintf(stderr, "IEEE1609.2 decode failure\n");
            return ret;
        } else {
            ieee1609_2_data *ie = mc->ieee1609_2data;
            if (ie->content != unsecuredData){
                if(gVerbosity)
                    fprintf(stderr, "IEEE1609.2 contains signed data\n");
                return 1;
            }
        }
        if(gVerbosity)
            printf("PSID of received message is: %02x\n", wsmpp->psid);
        if (wsmpp->psid == PSID_WSA && mc->msgId == ((int)WSA_MSG_ID)) {
#ifdef WITH_WSA
            if ((ret = decode_as_wsa(mc)) < 0) {
                if(gVerbosity)
                    fprintf(stderr, "WSA decode failure\n");
                return -1;
            } else {
                if(gVerbosity > 3)
                    print_wsa(mc->wsa);
                ret = 0;
            }
#else
            if(gVerbosity)
                fprintf(stderr, "WSA not supprted\n");
#endif
        } else {
            if(mc->msgId != ((int)WSA_MSG_ID)){
                if ((ret = decode_as_j2735(mc)) < 0) {
                    if(gVerbosity)
                        fprintf(stderr, "J2735 decode failure\n");
                    return -1;
                }else{
                    ret = 0;
                }
            }
        }
    } else {
#ifdef ETSI
        // family ID is removed by GeoNetwork router.
        if ((ret = btp_decode(mc)) < 0) {
            if(gVerbosity)
                fprintf(stderr, "BTP decode failure\n");
            return ret;
        }
        if ((ret = decode_as_etsi(mc)) < 0) {
            if(gVerbosity)
                fprintf(stderr, "ETSI decode failure\n");
        }
#endif
    }
    return ret;
}
/**
 * decode_msg_continue continue decode message after previous decode_msg()
 * returned 1(signed/encrypted message detected) and the message has been
 * verified/decrypted.
 */

int decode_msg_continue(msg_contents *mc) {
    int ret = 0;

    if (mc->stackId == STACK_ID_SAE) {
        if ((ret = decode_as_j2735(mc)) < 0 ) {
            if(gVerbosity)
                fprintf(stderr, "J2735 decode failure\n");
        } else {
            ret = 0;
        }
    } else {
    }

    return ret;
}
/**
 * encode_msg top-level codec API, encode data stored in mc into mc->abuf
 *
 * @param[in] mc the message content
 * @return > 1 whole message is succesfully encoded, returned encoded length.
 *         = 1 requested to sign/encrypt the message, need security service to
 *         perform operation before continuing encode the rest of the message.
 *         < 0 failure.
 *
 * NOTE: if mc->stackId is ETSI, then mc->etsi_msg_id shall be set by the caller
 * and correspdong data structure(mc->cam or mc->denm) shall be intialized by
 * the caller before calling this function.
 */
int encode_msg(msg_contents *mc)
{
    int ret = 0;
    wsmp_data_t *wsmpp;
    if (!mc || !mc->abuf.data) {
        if(gVerbosity)
            fprintf(stderr, "%s invalid input\n", __func__);
        return -1;
    }
    if (mc->stackId == STACK_ID_SAE) {
        wsmpp = (wsmp_data_t *)mc->wsmp;
        if (wsmpp->psid == PSID_WSA) {
#ifdef WITH_WSA
            mc->msgId = (int)WSA_MSG_ID;
            if ((ret = encode_as_wsa(mc)) < 0) {
                if(gVerbosity)
                    fprintf(stderr, "WSA encode failure\n");
                return ret;
            }
            if(gVerbosity > 3) {
                print_wsa(mc->wsa);
            }
#else
            fprintf(stderr, "WSA not supported\n");
#endif
        } else {
            mc->j2735_msg_id = J2735_MSGID_BASIC_SAFETY;
            if ((ret = encode_as_j2735(mc)) < 0) {
                if(gVerbosity)
                    fprintf(stderr, "J2735 encode failure\n");
                return ret;
            }
        }
        if ((ret = ieee1609_2_encode_unsecured(mc)) < 0) {
            if(gVerbosity)
                fprintf(stderr, "IEEE1609.2 encode failure\n");
            return ret;
        } else if (ret == 1) {
            return ret;
        }
        if ((ret = wsmp_encode(mc)) < 0) {
            if(gVerbosity)
                fprintf(stderr, "WSMP encode failure\n");
            return ret;
        }
    } else {
#ifdef ETSI
        if ((ret = encode_as_etsi(mc)) < 0) {
            if(gVerbosity)
                fprintf(stderr, "ETSI encode failure\n");
            return ret;
        }
        if ((ret = btp_encode(mc)) < 0) {
            if(gVerbosity)
                fprintf(stderr, "BTP encode failure\n");
            return ret;
        }
#endif
    }
    if (mc->abuf.tail_bits_left != 8)
        ret = mc->abuf.tail - mc->abuf.data + 1;
    else
        ret = mc->abuf.tail - mc->abuf.data;

    return ret;
}
int encode_msg_continue(msg_contents *mc)
{
    int ret = 0;
    if (!mc || !mc->abuf.data) {
        if(gVerbosity)
            fprintf(stderr, "%s invalid input\n", __func__);
        return -1;
    }
    if (mc->stackId == STACK_ID_SAE) {
        if ((ret = wsmp_encode(mc)) < 0) {
            if(gVerbosity)
                fprintf(stderr,"WSMP encode failure\n");
            return ret;
        }
    } else {
    }
    if (mc->abuf.tail_bits_left != 8)
        ret = mc->abuf.tail - mc->abuf.data + 1;
    else
        ret = mc->abuf.tail - mc->abuf.data;

    return ret;
}
