/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021, 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file wsmp.c
 * @brief library for dissecting/encoding WSMP 2016 frames and dealing with
 * variabl lengh P-encoded PSID
 *
 * Things specifically related to the formating/decoding of a IEEE 1609 WSMP
 * frame are included here.
 *
 * @note WSA formatting is presently not supported.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <linux/wireless.h>
#include <assert.h>

#include "v2x_msg.h"
#include "wsmp.h"

// Global for printing more useful information
extern int gVerbosity;

/**
 * Decode the variable length PSID, and un p-code the field. The wsmp* whould
 * point to the beginning of the WSMP PSID field(after N-header,& T header) and
 * version number.
 * the order of bytes in the WSMP header:
 *
 * Version| PSID (variable length)| extension fields (optional)| Wave Element ID| length| data
 * PSID length is determined via the value of the PSID itself, see section 8.1.3 of 1609.3
 * Length is determined by 4 MSB's of the first value:
 * < 7f   = 1 byte
 * 80-BF  = 2 bytes
 * C0-DF  = 3 bytes
 * E0-EF  = 4 bytes
 * This is explained in IEEE1609.12, section 4.1.2 - -P-encoding of PSID's
 *
 * @param [in] wsmp the encoded buffer points to the start of PSID field.
 * @note that MAX_PSID is the largest value that can presently be encoded per 1609.3
 */
static __inline int wsmp_decode_psid(uint8_t *wsmp)
{
    int psid = 0;

    if (!wsmp) {
        fprintf(stderr, "wsmp_decode_psid called with null wsmp header.\n");
        return 0;
    }

    // Note, bytes are network order, AKA big-endian
    switch (WSMP_PSID_FIELD_SIZE(wsmp[0])) {
    case 1:
        psid = (int)(wsmp[0]);
        break;

    case 2:
        psid = ((wsmp[0] << 8) + (wsmp[1])) - 0x7f80;
        break;

    case 3:
        psid = ((wsmp[0]) << 16) + (wsmp[1] << 8) + wsmp[2] - 0xbfbf80;
        break;

    case 4:
        psid = ((wsmp[0]) << 24) + (wsmp[1] << 16) + (wsmp[2] << 8) + wsmp[3] - 0xdfdfbf80;
        break;

    default:
        fprintf(stderr, " Error decoding PSID length \n");
    }

    return psid;
}


/**
 * helper function to return number of bytes  the resulting P-encoded PSID will
 * need as per IEEE 1609.12 p-encoding rules
 *
 * @param [in] psid_v psid value.
 * @returns -1 if error
 */
static __inline int WSM_PSID_PCODED_LEN(int psid_v)
{

    if (psid_v > MAX_PSID && gVerbosity) {
        fprintf(stderr, "wsmp_psid_pcoded_len called with null.\n");
        return -1;
    }

    if (psid_v >= 0x204080) {
        return (4);
    } else if (psid_v >= 0x4080) {
        return (3);
    } else if (psid_v >= 0x80) {
        // 2 byte OTA PSID
        return (2);
    } else {
        // 1 byte OTA PSID
        return (1);
    }

    return (-2);
}


/**
 * Adds in the PSID in P-Encoded format as per IEEE 1609.12 sec 4.1.2
 *
 * @param [in] wsmp input buffer, should has at least 4 free bytes.
 * @param [in] psid_v PSID value to be encoded.
 * @param [in] upon return, it hold how many bytes this operation added.
 * @returns next free buffer after encoding the psid.
 */
static __inline uint8_t*  wsmp_add_psid(uint8_t *wsmp, int psid_v, int *added)
{
    int P; // the P-encoded value of psid_v

    if ((!wsmp) || (psid_v > MAX_PSID) || !added) {
        fprintf(stderr, "wsmp_add_psid called with null or invalid psid.\n");
        fprintf(stderr, "Input psid value is: %d\n", psid_v);
        if (added) {
            *added = 0;
        }
        return NULL;
    }

    if (psid_v >= 0x204080) {
        // 4 byte OTA PSID
        P = psid_v + 0xdfdfbf80;
        wsmp[0] = (P & 0xff000000) >> 24;
        wsmp[1] = (P & 0x00ff0000) >> 16;
        wsmp[2] = (P & 0x0000ff00) >> 8;
        wsmp[3] = (P & 0x000000ff);
        *added = 4;
    } else if (psid_v >= 0x4080) {
        // 3 byte OTA PSID
        P = psid_v + 0xbfbf80;
        wsmp[0] = (P & 0xff0000) >> 16;
        wsmp[1] = (P & 0x00ff00) >> 8;
        wsmp[2] = (P & 0x0000ff);
        *added = 3;
    } else if (psid_v >= 0x80) {
        // 2 byte OTA PSID
        P = psid_v + 0x7f80;
        wsmp[0] = (P & 0xff00) >> 8;
        wsmp[1] = (P & 0x00ff);
        *added = 2;
    } else {
        // 1 byte OTA PSID
        // P=psid_v ;
        wsmp[0] = psid_v;
        *added = 1;
    }

#if 0
    {
        // extra debug
        int n;
        printf("[");
        for (n = 0; n < *added; n++) {
            printf("%02x", wsmp[n]);
        }
        printf("]");
    }
#endif

    return (wsmp + *added);

}

static void inline abuf_add_PSID(abuf_t *abp, uint32_t psid)
{
    int added;
    int len = WSM_PSID_PCODED_LEN(psid);
    uint8_t *cp;

    // adjust the asnbuf to allocate space at the tail for 1-4 bytes
    // note:  PSID's beyond 4 bytes not really supported.
    cp = abuf_put(abp, len);
    if (cp) {
        wsmp_add_psid(cp, psid, &added);
        if (gVerbosity > 6) {
            if (added != len) {
                printf("** WARN: P-Encoded PSID should have been %d bytes but only grew buf %d\n",
                    len, added);
            } else {
                printf("added %d byte PSID octets PSID=0x%0x (%d)\n", len, psid, psid);
            }
        }
    } else {
        fprintf(stderr, " ERROR adding PSID into WSM header\n");
    }
}


// Encode specifically the 2016 variant of WSMP headers
// this method will load into the dsrc_buf @ db the wsmp message header and message
// with the payload copied in at the end of the abuf.
// We must be given a clean blank dsrc_buf that has an allocated asnbuffer in it.
//  WSMP headers will be populated with the contents of the pktbuf_t information fields
//  and optional WEID's added as specified in that structure
//  When successful, this returns the number of bytes of the resulting payload message
//
static int encode_wsm2016(wsmp_data_t *wbp, abuf_t *wsm_payload)
{
    int result = -1;
    //asn_ncat_bits(wbp->abp, 1, 8); // Family Id
    if (wbp && wbp->abp) {
        int ElementExtensionPresent;  // a boolean bit flag based on TPID request

        // OTA, the number of WEID's is required in second byte. add them up.
        wbp->weid_qty = wbp->weid_opts.inc_rate_ext +\
            wbp->weid_opts.inc_chan_ext +\
            wbp->weid_opts.inc_pwr_ext +\
            wbp->weid_opts.inc_load_ext;

        if (wbp->weid_qty) {
            wbp->n_header.option_indicator = 1;
        }


        asn_ncat_bits(wbp->abp, wbp->n_header.data, 8);

        if (wbp->weid_qty) {

            // Encode the count first, as per IEEE 1609 8.1.3 & 8.1.3  but since the number
            // extensions will always be way less than 127 -- we don't need to consider a two byte
            // count field.  one octet it is.

            asn_ncat_bits(wbp->abp, wbp->weid_qty, 8);

            if (wbp->weid_opts.inc_rate_ext) {
                asn_ncat_bits(wbp->abp, WAVE_ELEM_ID_RATE, 8);
                asn_ncat_bits(wbp->abp, sizeof(uint8_t), 8); // WSIE length field = 1
                asn_ncat_bits(wbp->abp, wbp->rate, 8);
            }

            if (wbp->weid_opts.inc_pwr_ext) {
                asn_ncat_bits(wbp->abp, WAVE_ELEM_ID_PWR, 8);
                asn_ncat_bits(wbp->abp, sizeof(uint8_t), 8); // WSIE length field = 1
                asn_ncat_bits(wbp->abp, wbp->pwr, 8);
            }

            if (wbp->weid_opts.inc_chan_ext) {
                asn_ncat_bits(wbp->abp, WAVE_ELEM_ID_CHAN, 8);
                asn_ncat_bits(wbp->abp, sizeof(uint8_t), 8); // WSIE length field = 1
                asn_ncat_bits(wbp->abp, wbp->chan, 8);
            }

            if (wbp->weid_opts.inc_load_ext) {
                asn_ncat_bits(wbp->abp, WAVE_ELEM_ID_LOAD, 8);
                asn_put_encoded_len(wbp->abp, wbp->chan_load_len); // Variable length WEID!,
                asn_ncat(wbp->abp, wbp->chan_load_ptr, wbp->chan_load_len);
            }

        } // end of adding 0..4 optional WSIE

        // .. next byte is TPID, followed by var-len  PSID, then WSM Length
        // Really TPID is only supporting 0 at this point
        asn_ncat_bits(wbp->abp, wbp->tpid.octet, 8);

        // This may be removced some day, but for now, we do not support the addition of any WSIE
        // into the T-header as these are not yet defined.  What elements?  none proposed in IEEE
        // 1609, as of 2016 version
        ElementExtensionPresent = (wbp->tpid.octet % 2);
        if (ElementExtensionPresent) {
            //TPID 1,3,5 = optional Elements present -- but we don't actually have any code
            //to do this
            if (gVerbosity) {
                printf("overriding(clearing) request to populate a T-header Optional WSIE\n");
            }

            ElementExtensionPresent = 0;
            wbp->tpid.octet &= 0xfe;  // Zero out the LSB to clear the Element present bit
        }

        // Basically TPID #0 and #0 are the standard PSID, other types are the ISO port# flavor
        // as per IEEE 1609 8.3.3.3.3
        if (wbp->tpid.octet < TPID_ITS_PORTS_NOEXT) {
            // So... if one of the two PSID flavors
            abuf_add_PSID(wbp->abp, wbp->psid);
        } else {
            asn_ncat_bits(wbp->abp, htons(wbp->ports.src_port), 8);
            asn_ncat_bits(wbp->abp, htons(wbp->ports.dst_port), 8);

            if (wbp->tpid.octet > TPID_ITS_PORTS_EXT) {
                printf("reuqested unsupported Transport ID=%d\n", wbp->tpid.octet);
                result = -2;
                goto err;
            }

        }

        if (ElementExtensionPresent) {
            printf("T-header requested optional WSIE when building WSM header. Not supported.\n");
            result = -3;
            goto err;
        }

        // One last word to add variable length UPER encoding of the WSM Payload (param #2)

        if (wsm_payload) {
            abuf_trim(wsm_payload); // clean-up any hanging, but unpopulated bytes


            abuf_trim(wbp->abp); // clean-up any hanging, but unpopulated bytes
            if (gVerbosity > 7) {
                printf("WSM HEADER: ");
                abuf_dump(wbp->abp);
                printf("\n");
            }

            // Really -- the Payload must all be square and starting on an even byte boundry:
            // The WSMP header is designed, even though UPER to ensure this.
            asn_push_len(wsm_payload); // ASN encoded length will work

            if (gVerbosity > 7) {
                printf("WSMP payload\n");
                abuf_dump(wsm_payload);
                printf("\n");
            }

            if (!abuf_isaligned(wsm_payload)) {
                printf("WARNING:  J2735 payload not byte aligned. ");
                printf("This wont work for building a good WSM\n");
            }

            // Copy WSMP header into front of the payload and return the merged buffer len
            abuf_merge_second(wbp->abp, wsm_payload);
            result = abuf_byte_len(wsm_payload);
        } else {
            if (gVerbosity) {
                printf(" No WSMP Payload nor payload length added \n");
            }
        }
    }
err:
    return result;
}

/**
 * In the 2016 version, the qty of extension is always included at the beginning
 * of a WIEE. However, in older versions, we'll pass in a param # which might be
 * the Max expected (4) because no count field is included
 */

static int wsmp_decode_wave_element_extension(abuf_t *bp, wsmp_data_t *wsmpp, int qty_WEIDs_expected)
{
    int bits_left = 8; // for parsing of more complex UPER bitstreams
    struct wsmp_ext_field *ext_field;

    // If we have an optional wave element extnesion, find out how many WEID's
    // to iterate over In reality, standard allows this to be a two byte count,
    // (Variable length 1 or 2)  but no more than a few WEID's are presently
    // allowed bt standard in a "WAVE Information Element Extension"
    if (!qty_WEIDs_expected) {
        qty_WEIDs_expected = parse_asn_variable_length_enc(
                (unsigned char **)&bp->data, &bits_left);
        // NOTE: not calling the pktbuf_advance() here because
        // parse_asn_variable_length_enc advances.We'll just decrementy the length
    }

    if (gVerbosity > 1) {
        printf("decode WIEE: qty WEID's expected= %d\n", qty_WEIDs_expected);
    }

    // loop over all the extension fields first, each one found decerements, for
    // 2016+ vers of standard that includes a count
    while (qty_WEIDs_expected > 0) {
        qty_WEIDs_expected--;

        ext_field = (struct wsmp_ext_field *)bp->data; // Note ptr not advanced here, leave
                                   //where it is.

        /* We leave it pointing to the WEID exension ID, because  for older pre 2016 protocols
         * this could be the beginning of the WSMP  element, which has a length item which follows
         * this falls into the "default case below
         */

        switch (ext_field->wave_element_id) {
        case WAVE_ELEM_ID_RATE:
            wsmpp->rate = ext_field->data.data_rate;

            if (gVerbosity > 1) {
                printf("optional rate WEID found, rate=%d\n", wsmpp->rate);
            }
            abuf_pull(bp, WSMP_EXT_FIELD_SIZE(ext_field));
            continue;

        case WAVE_ELEM_ID_PWR:
            wsmpp->pwr = ext_field->data.tx_power;

            if (gVerbosity > 1) {
                printf("optional power WEID found, power=%d\n", ext_field->data.tx_power);
            }
            abuf_pull(bp, WSMP_EXT_FIELD_SIZE(ext_field));
            continue;

        case WAVE_ELEM_ID_CHAN:
            wsmpp->chan = ext_field->data.channel;
            if (gVerbosity > 1) {
                printf("optional channel WEID found, channel=%d\n", ext_field->data.channel);
            }
            abuf_pull(bp, WSMP_EXT_FIELD_SIZE(ext_field));

            continue;

        case WAVE_ELEM_ID_LOAD: /* "Channel Load" As per IEEE1609 8.3.4.5 */
            // element ID
            abuf_pull(bp, 1);

            // variable length
            wsmpp->chan_load_len = parse_asn_variable_length_enc((unsigned char **)&bp->data,
                                                                 &bits_left);
            wsmpp->chan_load_ptr = bp->data;

            // element payload
            abuf_pull(bp, wsmpp->chan_load_len);

            if (gVerbosity > 1) {
                printf("optional load WEID found, len=%d\n", wsmpp->chan_load_len);
            }
            continue;

        case WAVE_ELEM_ID_DATA:
        case WAVE_ELEM_SAFETY_FLAG:
            if (wsmpp->protoVersion < WSMP_PROTO_VER3_2016) {
                // If earlier/obsolete version, this is expected to signal the beginniing of WSMP
                if (gVerbosity > 1) {
                    printf("first non optional WEID encountered %d\n", ext_field->wave_element_id);
                }
            } else {
                printf("WARN:  unxpected WEID #%d not expected in 2016 and newer WSMP headers.\n",
                    ext_field->wave_element_id);
            }

            qty_WEIDs_expected = 0; // Causes us to exit, still pointing at the Data WEID

            continue;

        default:
            {
                // Just because we don't know this WEID,
                // We can still assume it adheres to figrue 15 of the 1609 spec,a
                //and read in the WEID's   variable length, and at least  print out the contents.
                //
                int unk_weid_len;
                abuf_pull(bp, sizeof(uint8_t));

                unk_weid_len = parse_asn_variable_length_enc((unsigned char **)&bp->data,
                    &bits_left);
                //print_buffer(bp->data, unk_weid_len);
                abuf_pull(bp, unk_weid_len);

            }
            break;
        }
    }
    return (0);
}

/**
 * Decode WSMP packet header, setting up payload pointers.
 *
 * @param [in] buf input buffer pointer contains the data to be decoded.
 * @return 0 on success.
 */
static int  wsmp_decode_header(msg_contents *mc)
{

    int retcode = 0;
    struct wave_element_field *wsmp_data_hdr_p = NULL; // Only used in pre 2016 formats
    int qty_WEIDs_expected = 4; // 4 for the older legacy pre 2016 optional WEIDS, assume max

    uint8_t next_weid;
    WSMP_N_HEADER_t wsmp_n_header;

    int ElementExtensionPresent; // Flag used when parsing 2016 TPID
    int PortsPresent; // Flag used when parsing 2016 TPID

    int bits_left = 8; // for parsing of more complex UPER bitstreams, should move into pktbuf_t
    char ver_octet;
    uint8_t *p;

    wsmp_data_t *wsmpp = (wsmp_data_t *)mc->wsmp;
    abuf_t *bp = &mc->abuf;

    if ((bp == NULL) || (NULL == wsmpp)) {
        fprintf(stderr, "One or more nulls\n");
        retcode = -1;
        goto wsmp_decode_err;
    }

    /**
     * in all versions of IEEE 1609 so far, the 3 LSB of first byte after 88dc
     * ethertype is the WSMP version #
     */
    if(gVerbosity > 4)
        printf("Pulling octet of WSMP version\n");
    //ver_octet = (*(uint8_t *)abuf_pull(bp, 1));
    uint8_t* byte = (uint8_t*)abuf_pull(bp, 1);
    if(byte == NULL)
        goto wsmp_decode_err;
    ver_octet = (*byte);
    wsmpp->protoVersion = ver_octet & 0x7;  // 3 least significant bits

    if (gVerbosity > 7) {
        printf("IEEE 1609 Proto version=%d. (first octet=%02x)\n",
                wsmpp->protoVersion, ver_octet);
    }

    switch (wsmpp->protoVersion) {

    case WSMP_PROTO_VER1_PRE_2010_POC:
    case WSMP_PROTO_VER2_2010:
        p = bp->data;
        wsmpp->psid = wsmp_decode_psid(p);


        // start the dbp->payload just past the PSID and version fields.  This is either
        // the start of the extension fields (if they are present), or the start of the
        // special WSMP data payload element ID data (what we call a wsmp_data_hdr)
        //  history way of doing this: dbp->payload = WSMP_EXT_FIELD_START(dbp->payload);
        //  which will not work here, since buf already advanced past the Ver
        if (gVerbosity > 7)
            printf(" PSID length=%d PSID=%d (0x%x)\n",
                WSMP_PSID_FIELD_SIZE(*p), wsmpp->psid, wsmpp->psid);

        p = abuf_pull(bp, WSMP_PSID_FIELD_SIZE(*p));

        break;


    case WSMP_PROTO_VER3_2016:
        // 2016 Version of the 1609.3 WSMP frame
        // Looking at the same byte that had Proto version in LSB,
        wsmp_n_header.data = ver_octet;
        wsmpp->n_header.data = ver_octet;
        qty_WEIDs_expected = wsmp_n_header.option_indicator;
        if (gVerbosity > 7) {
            printf("WSMP-N header  subtype=%d option=%d ver=%d\n",
                wsmp_n_header.subtype,
                wsmp_n_header.option_indicator,
                wsmp_n_header.version);
        }

        // We really only expect to see subtype zero, so far
        if (wsmp_n_header.subtype) {
            fprintf(stderr, "unexpected WSMP-transport  Subtype,  subtype=%d option=%d ver=%d\n",
                wsmp_n_header.subtype,
                wsmp_n_header.option_indicator,
                wsmp_n_header.version
                );
            retcode = -1;
            goto exit;
        }

        // If we have an optional wave element extnesion, find out how many WEID's to iterate over
        // In reality, standard allows this to be a two byte count, (Variable length 1 or 2)  but
        // no more than a few WEID's are presently allowed bt standard in a "WAVE Information
        // Element Extension"
        if (qty_WEIDs_expected) {
            if(gVerbosity > 2)
                printf("WAVE Extension IDs are present\n");
            if (wsmp_decode_wave_element_extension(bp, wsmpp, 0)) {
                retcode = -1;
                goto exit;
            }
            qty_WEIDs_expected = 0;

        }

        break;

    default:
        if(gVerbosity > 2)
            fprintf(stderr,
                "not a supported WSMP version/type,  version byte=0x%02x, ver=%d\n",
                ver_octet, wsmpp->protoVersion);
        retcode = -1;
        goto exit;
    }

    if (qty_WEIDs_expected) {
        if(gVerbosity > 2)
                printf("WAVE Extension IDs are present\n");
        if (wsmp_decode_wave_element_extension(bp, wsmpp, qty_WEIDs_expected)) {
            retcode = -1;
            goto exit;
        }
    }

    // case WAVE_SAFETY_FLAGS: in  Pre-2016 versions, this flag adds an extra "control field" byte
    // after the length

    // after the extension fields comes the payload.   For WSMP, it'll be a
    // element ID, length, followed by payload

    if( NULL == bp ) {
        printf(" bp is NULL \n ");
        retcode = -1;
        goto exit;
    }

    if( NULL == bp->data ) {
        printf(" data is NULL \n ");
        retcode = -1;
        goto exit;
    }

    next_weid = *(uint8_t *)bp->data;

    // If earlier/obsolete version, optional WEID's come now.
    if (wsmpp->protoVersion < WSMP_PROTO_VER3_2016) {

        if (gVerbosity > 2) {
            printf("** next_weid=%0d payload assumed**\n", next_weid);
        }

        if ((next_weid == WAVE_ELEM_ID_DATA) || next_weid == WAVE_ELEM_SAFETY_FLAG) {

            if (next_weid == WAVE_ELEM_SAFETY_FLAG) {
                if (gVerbosity) {
                    printf("** Warning: obsolete WSMP-S bit set. This adds a control byte**");
                }
            }

            wsmp_data_hdr_p = (struct wave_element_field *)bp->data;
            if (wsmp_data_hdr_p) {

                //dbp->payload_len = ntohs(wsmp_data_hdr_p->data_len.two_octet);
                mc->payload_len = ntohs(wsmp_data_hdr_p->data_len.two_octet);

                // advance data frame dbp->payload 2 bytes into data
                if (gVerbosity > 1) {
                    printf("** TWO octet length=%d bytes\n", mc->payload_len);
                }
                // advance past the WEID and length field.
                abuf_pull(bp, sizeof(uint8_t) + sizeof(short));


            }
            if (next_weid == WAVE_ELEM_SAFETY_FLAG) {
                abuf_pull(bp, sizeof(uint8_t));
            }

        } else {
            fprintf(stderr, "** WSMP WEID Parse error , unrecognized WEID %d\n", next_weid);
            retcode = -1;
            goto exit;
        }

    } else {
        // 2016 Version or newer.. next byte is TPID, followed by var-len  PSID, then WSM Length
        if(gVerbosity > 2)
                printf("Getting TPID Octet\n");
        //wsmpp->tpid.octet = *(uint8_t *)abuf_pull(bp, sizeof(uint8_t));
        uint8_t* ptr = abuf_pull(bp, 1);
        if (ptr == NULL) {
            fprintf(stderr, "** WSMP TPID Parse error\n");
            retcode = -1;
            goto exit;
        }
        wsmpp->tpid.octet = *ptr;
        /* now according to IEEE1609 2016, there could be an optional
           WEID Extension field (variable length) , before the WSMp payload length/data
           See enum definition in dsrc_util.h called TPUD_te
        */
        switch (wsmpp->tpid.id) {
        case TPID_PSID_NOEXT:
        case TPID_PSID_EXT:
            PortsPresent = 0;
            break;
            /* TPID 2 & 3 are with SRC/DEST ports */
        case TPID_ITS_PORTS_NOEXT:
        case TPID_ITS_PORTS_EXT:
            PortsPresent = 1;
            break;
        case TPID_LPP_NOEXT:
        case TPID_LPP_EXT:
            PortsPresent = 1;
            break;
        default:
            if (gVerbosity) {
                printf("unknown TPID %d\n", wsmpp->tpid.octet);
            }
            retcode = -1;
            goto exit;
        }

        // Odd (LSB=1) TPID indicate presence of optional WAVE Inofrmation Element Extension
        ElementExtensionPresent = (wsmpp->tpid.id % 2); //TPID 1,3,5 = Eelments present
        if(gVerbosity > 2)
                printf("Checking if ports are present or PSID\n");
        // There are either Ports or PSID -- never both
        if (PortsPresent) {
            if(gVerbosity > 2)
                printf("Ports are present\n");

            uint16_t* srcPort = (uint16_t *)abuf_pull(bp, sizeof(uint16_t));
            uint16_t* dstPort = (uint16_t *)abuf_pull(bp, sizeof(uint16_t));
            if (!srcPort || !dstPort) {
                fprintf(stderr, "** Src/Dst Port Parse error\n");
                retcode = -1;
                goto exit;
            }
            wsmpp->ports.src_port = ntohs(*srcPort);
            wsmpp->ports.dst_port = ntohs(*dstPort);

            if (gVerbosity > 2) {
                printf("TPID source port=%d, dest port=%d\n", wsmpp->ports.src_port,
                    wsmpp->ports.dst_port);
            }
        } else {
            p = bp->data;
            if (gVerbosity > 2) {
                printf("Ports are not present, PSID present\n");
                printf("PSID LEN=%d ", WSMP_PSID_FIELD_SIZE(*(uint8_t *)p));
            }

            wsmpp->psid = wsmp_decode_psid(p);

            // advance the ptr through the variable length PSID, length determined by looking
            // at first byte only
            p = abuf_pull(bp, WSMP_PSID_FIELD_SIZE(*p));

            if (gVerbosity > 2) {
                printf("TPID=%08x PSID=%d (0x%02x)\n", wsmpp->tpid.id, wsmpp->psid, wsmpp->psid);
            }
        }

        // Get datalength from ASN UPER
        if(gVerbosity>2)
            printf("Getting payload length from wsmp header\n");
        mc->payload_len = parse_asn_variable_length_enc((unsigned char **)&bp->data, &bits_left);

        if (ElementExtensionPresent) {
            // Zero param flags that it is a full WSEE and length is next byte in stream
            if (wsmp_decode_wave_element_extension(bp, wsmpp, 0)) {
                retcode = -1;
                goto exit;
            }
        }


        if (next_weid == WAVE_ELEM_SAFETY_FLAG) {
            // could read-in the obsolete control fields from the WSMP-S ext
            abuf_pull(bp, sizeof(uint8_t));
        }
    }

    if (gVerbosity > 7) {
        printf("decoded WSMP payload len=%d\n", mc->payload_len);
    }

    if (gVerbosity > 7) {
        printf("PSID=%d (0x%02x)\n", wsmpp->psid, wsmpp->psid);
    }


wsmp_decode_err:


exit:

    if (gVerbosity > 5) {
        printf("WSMP header dissection complete returncode=%d\n", retcode);
    }

    return (retcode);
}

/**
 * decode wsmp packet.
 *
 * @param [in] mc pointer to the msg contents data that contains the incoming packet
 * @return 0 on success.
 *
 * @note upon succesful decoding, the mc->wsmp will contain the decoded wsmp
 * contennt
 */
int wsmp_decode(msg_contents *mc)
{
    int retcode = 0;

    if (!mc || !mc->abuf.data) {
        fprintf(stderr, "Invalid pointer to WSMP packet\n");
        retcode = -1;
        goto wsmp_pkt_err;
    }

    if (!mc->wsmp) {
        mc->wsmp = calloc(sizeof(wsmp_data_t), 1);
        if (!mc->wsmp) {
            retcode = -2;
            goto wsmp_pkt_err;
        }
    }

    /* extract the wsmp data payload in addition to the rate, channel
       and power info give it pointer to data and size (&payload_len)
       from the dsrc_buf..unencoded content loaded with decode results
       this decode can handle 2010 or 2016 formats*/
    if ( wsmp_decode_header(mc) < 0) {
        retcode = -3;
        goto wsmp_pkt_err;
    }
    mc->l3_payload = mc->abuf.data;
    mc->l3_payload_len = mc->payload_len;

    if (gVerbosity > 2) {
        printf("WSMP Payload: ");
        print_buffer(mc->abuf.data, mc->payload_len);
    }

    // A WSMP payload does not have to be J2735 or a BSM.., Could actually be a special WSA
    // but those the most commmon.  However, payload can contain any number of private uses,
    // depnding on PSID For example, it could be IEEE 1609.11 tolling frames.

    // decode returns what msg_id was found, but we do not keep track of
    // whether the J2945/J2735 was decoded correctly.
    retcode = 0;

wsmp_pkt_err:

    return retcode;
}

extern int wsmp_data_decode(wsmp_data_t* wsmp){
    return 0;
}

int wsmp_encode(msg_contents *mc)
{
    wsmp_data_t *wsmp = (wsmp_data_t*) (mc->wsmp);
    if (!mc->wsmp) {
        if(gVerbosity > 2)
            fprintf(stderr, "%s: invalid input\n", __func__);
        return -1;
    } else if (!mc->abuf.data) {
        if(gVerbosity > 2)
            fprintf(stderr, "%s: no input to encode\n", __func__);
        return -1;
    }

    if (wsmp->abp == NULL)
    {
        if(gVerbosity > 1)
        {
            fprintf(stderr, "%s: passing wsmp data with no asnbuf \n",
             __func__);
        }
        return -1;
    }

    if (wsmp->abp->size <= 0)
    {
        if(gVerbosity > 1)
        {
            fprintf(stderr, "%s: passing wsmp data with no allocated asnbuf\n",
                 __func__);
        }
        return -1;
    }
    //Resetting buffer before using it for encodeing.
    abuf_reset(wsmp->abp, WSMP_ABUF_DEFAULT_HEADROOM);
    int ret = encode_wsm2016(wsmp, &(mc->abuf));
    return ret;
}

int wsmp_data_encode(wsmp_data_t* wsmp)
{
    if (!wsmp) {
        if(gVerbosity > 2)
            fprintf(stderr, "%s: wsmp input is null\n", __func__);
        return -1;
    }

    if (wsmp->abp == NULL)
    {
        if(gVerbosity > 1)
        {
            fprintf(stderr, "%s: passing wsmp data with no asnbuf \n",
             __func__);
        }
        return -1;
    }

    if (wsmp->abp->size <= 0)
    {
        if(gVerbosity > 1)
        {
            fprintf(stderr, "%s: passing wsmp data with no allocated asnbuf \n",
                __func__);
        }
        return -1;
    }

    int ret = encode_wsm2016(wsmp, wsmp->abp);
    return ret;
}

