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
 * @file ieee1609.2.c
 * @brief encode/decode IEEE1609.2 header (unsecured packet only).
 */
#include "v2x_msg.h"
#include "ieee1609.2.h"
extern int gVerbosity;
/**
 * encode IEEE1609.2 header (unsecured packet only).
 * @param [in] mc message content to be encoded.
 * @return 0 success.
 *         1 request to encode secured packet, which is not handled by this
 *         function.
 *         -1 failure ,invalid input.
 */
int ieee1609_2_encode_unsecured(msg_contents *mc)
{
    if (!mc || !mc->ieee1609_2data) {
        fprintf(stderr, "%s: invalid input\n", __func__);
        return -1;
    }
    ieee1609_2_data *ie = mc->ieee1609_2data;
    if (ie->content == signedData ||
        ie->content == encryptedData)
        return 1;

    asn_push_ieee1609_2_len(&mc->abuf);
    asn_push_bits(&mc->abuf, ie->content, 6);
    asn_push_bits(&mc->abuf, ie->tagclass, 2);
    asn_push_bits(&mc->abuf, ie->protocolVersion, 8);

    return 0;
}

int ieee1609_2_decode_unsecured(msg_contents *mc)
{
    //uint8_t *ptr;
    int bits_left = 8;
    if (!mc->ieee1609_2data) {
        mc->ieee1609_2data = calloc(sizeof(ieee1609_2_data), 1);
        if (!mc->ieee1609_2data) {
            printf("ieee1609_2_decode_unsecured calloc error!\n");
            return -1;
        }
    }

    ieee1609_2_data *ie = mc->ieee1609_2data;

    ie->protocolVersion = *((uint8_t*)mc->abuf.data);
    abuf_pull(&mc->abuf, sizeof(uint8_t));
    //mc->abuf.tail = mc->abuf.data+1;
    ie->tagclass = get_next_n_bits((uint8_t **)&mc->abuf.data, 2, &bits_left);
    ie->content = get_next_n_bits((uint8_t **)&mc->abuf.data, 6, &bits_left);
    // anthony: to check if fixes any decode problems related to unsecured?
    bits_left = 8;
    mc->payload_len = parse_asn_CER_len_enc((uint8_t **)&mc->abuf.data, &bits_left);

    if (gVerbosity > 4) {
        printf("\nIEEE 1609.2 Security Header Version: %d\ttagclass:%d\tcontent:%d\tlength:%"PRIu64"\t\n",
            ie->protocolVersion,ie->tagclass,ie->content, mc->payload_len);
        printf("In Hex: %02x:%02x:%02x\n", ie->protocolVersion,
                ie->tagclass, ie->content);
    }
    return 0;
}
