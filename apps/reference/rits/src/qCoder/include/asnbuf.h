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
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _ASNBUF_H_
#define _ASNBUF_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <assert.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define ASNBUF_DEBUG 0

// A Bit-wise nbuf type structure for encoding/building up ASN.1 messages
typedef struct {
    char *head;
    int size;
    uint8_t head_headspace_bits;
    uint8_t tail_bits_left; /* number (1-8) of bits left in the byte tail points to , filled in from MSB*/
    char *data;
    char *tail; // Pointer to last byte we are presently filling in, that has 1 to 8 tail_bits_left unused.
    char *end;  // pounter to the last byte, as configured in at alloc time, saves calculations later
} abuf_t;

// Forward declarations:
static inline int asn_ncat_bits(abuf_t *bp, uint32_t data, int bitlen);
static void abuf_dump(abuf_t *bp);

/* Allocate an ASN abuf.  Ideally this would not be done for every packet to be sent or received...but pre-allocated abufs
 * would be re-used.  Note.   Presently a more expensive "calloc" is used.  its possible, upon further thought and testing that
 * a pure malloc, with just proper initialization of a few parameters would suffice.
 */
static inline int abuf_alloc(abuf_t *abp, int size, int headroom)
{
    int result = -1;

    // Make sure we have non-null buf pointer, and reasonable params
    if (abp && size && (headroom < size)) {
        abp->head = (char*) calloc(size, 1); // REVISIT: malloc may suffice

        if (!abp->head) {
            goto alloc_fail;
        }

        abp->size = size;
        abp->end = abp->head + size; // saves calculations to have a pointer to last byte
        abp->data = abp->head + headroom;
        abp->tail = abp->data;

        abp->tail_bits_left = 8;
        abp->head_headspace_bits = 0;
        result = size;

    }

alloc_fail:
    return (result);

}

static uint32_t  get_next_n_bits(unsigned char **cpp, int n, int *bits_left_p);

/* Parse the ASN encoding of length, for at least the simple 1 and 2 byte cases
   which is long enough for all known BSM cases --   We presently do not handle messages over MTU,
   that might require OTA fragmentation and/or ASN.1 length encoding Fragmentation
   This advances the *cpp param by 1 or 2 bytes, or 3 if we started with less than 8 bits left
   */
static inline int parse_asn_variable_length_enc(unsigned char **cpp, int *bits_left_p)
{
    int length = -1;

    // Fast optimization is less work if octet aligned
    if (*bits_left_p > 7) {
        if (**cpp & 0x80) {
           // two octets are used for length if it's bigger than 127
            if (**cpp & 0x40) {
                printf("MTU fragmentation unsupported\n");
            } else {
                length = (**cpp & 0x7f) << 8 | *(*cpp + 1);
                *cpp = *cpp + 2;
            }
        } else {
            length = **cpp;
            *cpp = *cpp + 1;
        }
    } else {
        uint8_t byte1;
        // More expensive case, where the length field is not octet aligned...which of course could happen
        byte1 = get_next_n_bits(cpp, 8, bits_left_p);
        if (byte1 < 0x80) {
            length = byte1;
        } else {
            if (byte1 & 0x40) {
                printf("MTU fragmentation unsupported\n");
            } else {
                uint8_t byte2;
                byte2 = get_next_n_bits(cpp, 8, bits_left_p);
                length = (byte1 & 0x7f) << 8;
                length |= byte2;
            }
        }
    }

    return (length);
}

// Function parses the length that is in Canonical encoding

static inline uint64_t parse_asn_CER_len_enc(unsigned char **cpp, int *bits_left_p)
{
    uint64_t length = -1;

    if (**cpp & 0x80) {

        int noctets;
        uint8_t byte;
        *bits_left_p = 7;
        noctets = get_next_n_bits(cpp, 7, bits_left_p);
        //printf("number of octets: %d\n", noctets);
        if (noctets > 8)
            printf("unsupported: Length too big > 8 octets\n");

        while (noctets > 0) {

            byte = get_next_n_bits(cpp, 8, bits_left_p);
            if (length == -1)
                length = byte;
            else
                length = (length << 8) | byte;
            noctets--;

        }

    } else {

        length = **cpp;
        *cpp = *cpp + 1;

    }

    return (length);
}



static inline uint8_t* abuf_purge(abuf_t *abp, int headroom)
{
    char *result = NULL;

    // Make sure we have non-null buf pointer, and reasonable params
    if (abp && (headroom < abp->size) && abp->head) {
        bzero(abp->head, abp->size);

        abp->data = abp->head + headroom;
        abp->tail = abp->data;

        abp->tail_bits_left = 8;
        abp->head_headspace_bits = 0;
        result = abp->data;
    }

    return ((uint8_t*)result);
}


// abuf_isaligned returns 1 if front byte does not have any off bit partially populated bytes
static inline int abuf_isaligned(abuf_t *abp)
{
    return (abp->head_headspace_bits == 0 || abp->head_headspace_bits == 8);

}


static inline void abuf_free(abuf_t *abp)
{
    if (abp && abp->head) {
        free(abp->head);
    }
}

/* cleans-up any unfilled bytes on head and/tail and adjusts pointers */
static inline void abuf_trim(abuf_t *bp)
{
    if (bp && bp->tail && bp->data) {

        if (bp->head_headspace_bits == 8) {
            bp->data++;
            bp->head_headspace_bits = 0;
        }

        if (bp->tail_bits_left == 8) {
            bp->tail--;
            bp->tail_bits_left = 0;
        }
    }
}

/* return the count of set bits */
static inline int abuf_count_set_bits(abuf_t *bp)
{
    register int retval = 0;
    char *p = bp->data;

    while (p <= bp->tail) {
        uint8_t n = *((uint8_t*)p);
        while (n) {
            retval += n & 1;
            n >>= 1;
        }
        p++;
    }

    return (retval);
}



/* return payload length of the asn bitstring in bits */
static inline int abuf_bits_len(abuf_t *bp)
{
    register int retval = -1;

    if (bp && bp->tail && bp->data) {
        retval = (bp->tail - bp->data) + 1;
        retval *= 8; // convert to bitcount

        retval -= bp->head_headspace_bits;
        retval -= bp->tail_bits_left;

    }

    return (retval);
}


/* Return the length of the current bp, in bytes... taking into
    consideration a partially filled last byte  */
static inline int abuf_byte_len(abuf_t *bp)
{
    register int retval = -1;
    if (bp && bp->tail && bp->data) {
        retval = (bp->tail - bp->data);
        if (bp->tail_bits_left != 8) {
            retval++;
        }
    }

    return (retval);
}


// -1 if there is some kind of error or no tailroom
// otherwise returns the # of tailroom Bytes
static inline int abuf_tailroom(abuf_t *abp)
{
    if (abp && abp->tail && abp->end) {
        return (abp->end - abp->tail);
    } else {
        return (-1);
    }
}

static inline int abuf_headroom(abuf_t *abp)
{
    if (abp && abp->head && abp->data) {
        return (abp->data - abp->head);
    } else {
        return (-1);
    }
}

static inline int abuf_headroom_bits(abuf_t *abp)
{
    if (abp && abp->head && abp->data) {
        return (((abp->data - abp->head) * 8) + (abp->head_headspace_bits));
    } else {
        return (-1);
    }
}

// return the number of bytes needed to add bitlen to current abuf, -1 returned if it will not fit
static inline int abuf_check_put_tailroom(abuf_t *abp, int bitlen)
{
    int tailroom_B = 1;
    if (bitlen > 0 && abp && abp->head && abp->data) {
        tailroom_B = 0;
        bitlen -= abp->tail_bits_left;

        // If there is more bits than will fit in the last byte, a remainder bits bitlen
        if (bitlen > 0) {
            // calculate how many bytes it will take to accomodate this byte
            tailroom_B = ((bitlen + 7) / 8);
            //printf("<%d/%d bytes needed >",tailroom_B,abuf_tailroom(abp));

            // OK we need tailroom_B bytes... do we have enough left in our abuf?
            if (tailroom_B >  abuf_tailroom(abp)) {

                tailroom_B = -1;
            }
        }
    }
    return tailroom_B;
}

// Make sure buffer can be easily merged into main BSM later, by
// allocating a prescribed amount of headspace at front of buffer which will not
// be used, and not included in the length inquiries
static inline void abuf_reserve_headspace(abuf_t *bp, uint8_t reserved_bits)
{
    // fill 8 minus n of the high bits with 0 as these will utimately
    // end up being bitwise OR'ed in with an existing buffer that has those bits
    // occupied (which is why we are reserving the headspace in this abuf)
    assert(bp->head_headspace_bits == 0); // only works on full clean abufs at this point.

    asn_ncat_bits(bp, 0, reserved_bits);

    bp->head_headspace_bits = reserved_bits;

    //printf("reserved %d headspace bits in new buffer.", bp->head_headspace_bits);
}

/* asn_push_bits
    only adds up to 32 bits
    returns the number of bits added, or -1 if a bad parameter encountered
*/
static inline int asn_push_bits(abuf_t *bp, uint32_t data, int bits_to_add)
{
    int qty_lsb_bits_into_first_byte;
    int result = -1;
    char *tp = bp->data;
#if ASNBUF_DEBUG
    printf("pushing bits<0x%08x>[%d bits] ", data, bits_to_add);
    if (bp) {
        printf("{headroom=%d bits, head headspace_bits=%d,bp->data=%02x}\n", abuf_headroom_bits(bp), bp->head_headspace_bits, *tp);
    }
#endif


    if (bp && bits_to_add && (abuf_headroom_bits(bp) >= bits_to_add)) {
        result = bits_to_add;


        if (bits_to_add < bp->head_headspace_bits) {
            qty_lsb_bits_into_first_byte = bits_to_add; //( bits_to_add < bp->head_headspace_bits );
        } else {
            qty_lsb_bits_into_first_byte = bp->head_headspace_bits;
        }

#if ASNBUF_DEBUG
        printf("qty_lsb_bits_into_first_byte=%d\n", qty_lsb_bits_into_first_byte);
#endif

        // Get least bits,  least "first bits"
        *bp->data &=  ~(((1 << qty_lsb_bits_into_first_byte) - 1) << (8 - bp->head_headspace_bits));
        *bp->data |=  ((data & ((1 << qty_lsb_bits_into_first_byte) - 1)) << (8 - bp->head_headspace_bits));

        bp->head_headspace_bits -= qty_lsb_bits_into_first_byte;
        data >>= qty_lsb_bits_into_first_byte;
        bits_to_add -= qty_lsb_bits_into_first_byte;

        if (bp->head_headspace_bits < 1) {
            bp->data--;  // push data pointer into headspace 1 byte
            bp->head_headspace_bits = 8;
        }

        while (bits_to_add >= 8) {
            // printf("[insert to add in %0x.%d]", data, bits_to_add);
            *bp->data  = data & 0xff;
            data >>= 8;
            bp->data--;
            bp->head_headspace_bits = 8;
            bits_to_add -= 8;
        }

        if (bits_to_add) {
            *bp->data = (data & ((1 << bits_to_add) - 1));
            bp->head_headspace_bits = 8 - bits_to_add;
#if ASNBUF_DEBUG
            printf("{added last msb bits_to_add=%d, data=%0x}\n", bits_to_add, *bp->data);
#endif
        }

        if (bp->head_headspace_bits == 8) {
            // printf("trimming up head pointer\n");
            bp->data++;
            bp->head_headspace_bits = 0;
        }

    }

    return (result);
}

// Leaves the tailbuf with at least a byte to copy into, if
// something left the tail pointing at an already full byte
static inline void abuf_tail_ready(abuf_t *bp)
{
    // Leave the tail and tail_bits pointing to the next free data, not a zero bit full byte
    if (bp->tail_bits_left < 1) {
        // really should never be negative
        assert(bp->tail_bits_left >= 0);
        if (abuf_tailroom(bp)) {
            bp->tail++;
            bp->tail_bits_left = 8;
        }
    }
}
/*  ASN operation to round-up to even multiple of 8 length, which is
 *  required for certain open type sequences.  This is one of the very
 *  rare times that any padding is used in an UPPER encoded stream.
 *  need to align  is suprising, given that this is UPER, however we're
 *  we're not exactly aligning to a byte boundry, but padding with zeros
 *  to make the length of the UPER an interer multiple of *8 bits
 *     */

static inline void abuf_pad_to_even_octet(abuf_t *bp)
{
    register uint8_t  mod8, bit_len;

    bit_len = abuf_bits_len(bp);

    if (bit_len > 0) {

        mod8 = bit_len % 8;
        if (mod8) {
            //printf(" [[padding %d bits]]", 8 - mod8);
            asn_ncat_bits(bp, 0x00000, 8 - mod8);
        }
    }

}

/* abuf_put
 Advance the tail by "bytes"and return a pointer to previous to be filled tail
    this is byte based... if previous tail was (partially used, ie tail_bits_left,
    that is not accounted for here
    This adds n bytes, but returns a pointer to the current tail.,
    returns NULL if there was not enough room
*/
static inline char* abuf_put(abuf_t *bp, int bytes)
{
    char *curr_tail = NULL;
    if (bp) {
        curr_tail = bp->tail;
    }

    bp->tail += bytes; // advance the tail.

    if (bp->tail > bp->end) {
        bp->tail = curr_tail;
        curr_tail = NULL;

    }

    return (curr_tail);
}
static inline void abuf_reset(abuf_t *abp, int headroom)
{
    abp->data = abp->head + headroom;
    abp->tail = abp->data;
    abp->tail_bits_left = 8;
    abp->head_headspace_bits = 0;
    abp->end = abp->head + abp->size;
}
/*
 * advance the data by "bytes", and return previous data pointer, for decoding
 * operation
 */
static inline char *abuf_pull(abuf_t *bp, int bytes)
{
    char *curr_data = NULL;
    if (bp) {
        curr_data = bp->data;
    }
    if(bp->data){
        bp->data += bytes;
        //bp->end
        if ((bp->data) > bp->tail) {
            printf("Error: Pulling passed tail of abuf struct\n");
            curr_data = NULL;
            return NULL;
        }
    }
    return (curr_data);
}
/*
 * reduce the headroom by "bytes" from the front of the buffer.
 */
static inline char *abuf_push(abuf_t *bp, int bytes)
{
    if (bp) {
        if (bp->data - bp->head < bytes)
            return NULL;
        bp->data -= bytes;
    }
    return bp->data;
}
static inline char* abuf_tail(abuf_t *bp)
{
    char *curr_tail = NULL;
    if (bp) {
        curr_tail = bp->tail;

        // Special case... if out of bits... advance to next byte if any available.
        if (!bp->tail_bits_left) {
            curr_tail = abuf_put(bp, 1);
        }
    }

    return (curr_tail);
}

// Copy into tail (concatenante) an arbitary # of bytes/bits into the ASN buf
// NOTE: onyl copies in an even nuimber of bytes.  Use ASN merge if you have two ASN bitstreams to combine
static inline int asn_ncat(abuf_t *bp, char *data, int total_source_bytes)
{
    int result = 0;

    if (total_source_bytes > 0 && bp && bp->head && bp->data) {

        char *copy_to_here = NULL;

        abuf_tail_ready(bp);

        // First check for super clean/fast even byte aligned qty
        if (bp->tail_bits_left == 8) {
            copy_to_here = abuf_put(bp, total_source_bytes);
            if (copy_to_here) {
                memcpy(copy_to_here, data, total_source_bytes);
                // Warning: if you copied in a non integer number of bytes,
                // your data buffer's last byte better be already in ASN aligned style,
                // with the Most significant bits containing the partial stream.

            } else {
                result = -1;
                goto ncat_exit;
            }
        } else {
            int remaining_source_bits = total_source_bytes * 8; // counter to decrement as we copy into the bitstream
                                        // fractional tail bits to start with, makes a lot more work for CPU
                                        // This will be pretty expensive, so don't do this too often, or for large bit strings
                                        //
            while (remaining_source_bits > 0) {
                // Add the source bit stream word by word, until only a partial remnanat, then done.
                if (remaining_source_bits >= 32) {
                    asn_ncat_bits(bp, htonl(*(uint32_t *)data), 4);
                    remaining_source_bits -= 4;
                    data += 4; // advance data ptr
                } else {
                    asn_ncat_bits(bp, htonl(*(uint32_t *)data), remaining_source_bits);
                    remaining_source_bits = 0;
                }
            }
        }
    }

ncat_exit:

    return (result);
}

// Concatenate nbits onto abuf pounted to by first param
static inline int asn_ncat_bits(abuf_t *bp, uint32_t data, int bitlen)
{
    int result = 0;

    // printf("asn_ncat(bp,0x%0x,%d) [tailroom=%d]", data, bitlen, abuf_tailroom(bp));

    if ((bitlen <= 0) || !bp || !bp->head || !bp->data) {
        result = -1;
        goto done;
    }

    // If this is the first and only byte so far, setup the tail pointer
    if (!bp->tail) {
        bp->tail = bp->data;
        bp->head_headspace_bits = 0;
        bp->tail_bits_left = 8;
    }

    if(bp->tail_bits_left == 8){
        *bp->tail = 0;
    }

    if (bitlen <= bp->tail_bits_left) {

        // Mask off just the "bitlen" bits passed in on data and shift into place.
        if (bitlen == 8 && bp->tail_bits_left == 8) {
            *bp->tail = (uint8_t)data;    // perfect add 8 bits optimization
            bp->tail++;
            goto done;

        } else {
            *bp->tail |= ((uint8_t)data & ((1 << bitlen) - 1)) << (bp->tail_bits_left - bitlen);
            bp->tail_bits_left -= bitlen;
        }
    } else {
        // Not going to fit in the byte we had.
        int bits_to_add = bitlen;

        // First fill up any fragmented first byte
        *bp->tail |= ((data & (((1 << bp->tail_bits_left) - 1) << (bitlen - bp->tail_bits_left))) >>
            (bitlen - bp->tail_bits_left));
        bits_to_add -= bp->tail_bits_left;
        bp->tail_bits_left = 0;

        // printf("tailroom_needed(%d)=%d ", bits_to_add, abuf_check_put_tailroom(bp, bits_to_add ));
        // Check all at once that we have enough tailroom  for bitlen
        if (abuf_check_put_tailroom(bp, bits_to_add) >= 0) {
            // Advance into next full/free byte
            abuf_tail_ready(bp);

            while (bits_to_add >= 8) {
                // Mask and shift to LSB, the most significant 8 bits of the input "data" bitstream
                *bp->tail = (data & (0xff << (bits_to_add - 8))) >> (bits_to_add - 8);
                bp->tail++;
                bits_to_add -= 8;
            }

            // Add/Put the last, 7 or fewer partial-byte's bits to add to the tail
            if (bits_to_add) {

                *bp->tail = (data & ((1 << bits_to_add) - 1)) << (8 - bits_to_add);
                //printf("{bits_to_add=%d,%02x}\n",bits_to_add,*bp->tail);

                bp->tail_bits_left -= bits_to_add;
            }


        } else {
            printf("ERR: out of room in ASN buffer\n");
            result = -1;
        }
    }

    abuf_tail_ready(bp);


done:

    return result;
}


/* As per ISOIEC 8825-2 packed encoding rules, length values
    larger than 127 less than "16K" are encoded with 2 bytes,
    with the MSB set , and bit 7 of that first bit cleared (0xDFFF below)
*/
#define ASN_2BYTE_16K_MAX_LEN 0x4000
#define ASN_2BYTE_BIT_SET     0x8000
#define ASN_2BYTE_BIT_CLEAR   0xDFFF
#define ASN_1BYTE_MAX_LEN 0x0100
#define ASN_2BYTE_MAX_LEN 0x010000
#define ASN_3BYTE_MAX_LEN 0x01000000
#define ASN_4BYTE_MAX_LEN 0x0100000000


/*
    puts the length in the variable-length ASN UPER format *in front* (push)
    of current data.
    returns -1 if unable to add
*/
static inline int asn_push_encoded_len(abuf_t *bp, int len)
{
    int result = 0;
#if ASNBUF_DEBUG
    printf("pushing length=0x%0x(%d)\n", len, len);
#endif

    if (len > 0 && bp && bp->head && bp->data) {

        // If a an 8 bit length will suffice...
        if (len < 0x80) {
            if (bp->head_headspace_bits == 8) { // Room for the whole byte
                printf("was room at bp->data--- odd case");
                *bp->data = (uint8_t)len;
                bp->head_headspace_bits = 0;
                result = 8;
            } else if (bp->head_headspace_bits == 0) {
                if (bp->data > bp->head) { // If there as room for at least 1 byte more at head
                    bp->data--;
                    *bp->data = (uint8_t)len;
                    result = 8;
                    // bp->head_headspace_bits = 0; already the case, since we added 8 bits
                } else {
                    result = -1;
                    goto err;
                }

            } else {
                result = asn_push_bits(bp, len, 8);
            }


        } else {
            // We're going to be adding a 16 bit value, because length bigger than 127 and less than 16K
            if (len < ASN_2BYTE_16K_MAX_LEN) {
                // ASN Length encoding rules for lengths between 0x127 and 0x4000 ("16K")
                len |= ASN_2BYTE_BIT_SET;
                len &= ASN_2BYTE_BIT_CLEAR;
                result = asn_push_bits(bp, len, 16);
            } else {
                fprintf(stderr, "encountered a asn fragment len larger than reasonable.\n");
                result = -1;
                goto err;
            }

        }

    }

err:
    return result;

}

/*
    puts the length in the variable-length ASN CSER format *in front* (push)
    of current data.
    returns -1 if unable to add
*/

static inline int asn_push_CER_encoded_len(abuf_t *bp, int len)
{
    int result = 0;
#if ASNBUF_DEBUG
    printf("pushing length=0x%0x(%d)\n", len, len);
#endif

    if (len > 0 && bp && bp->head && bp->data) {

        // If a an 8 bit length will suffice...
        if (len < 0x80) {
            if (bp->head_headspace_bits == 8) { // Room for the whole byte
                printf("was room at bp->data--- odd case");
                *bp->data = (uint8_t)len;
                bp->head_headspace_bits = 0;
                result = 8;
            } else if (bp->head_headspace_bits == 0) {
                if (bp->data > bp->head) { // If there as room for at least 1 byte more at head
                    bp->data--;
                    *bp->data = (uint8_t)len;
                    result = 8;
                    // bp->head_headspace_bits = 0; already the case, since we added 8 bits
                } else {
                    result = -1;
                    goto err;
                }

            } else {
                result = asn_push_bits(bp, len, 8);
            }


        } else {
            int noctets = 0;
            if (len < ASN_1BYTE_MAX_LEN)
                noctets = 1;
            else if (len < ASN_2BYTE_MAX_LEN)
                noctets = 2;
            else if (len < ASN_3BYTE_MAX_LEN)
                noctets = 3;
            else if (len < ASN_4BYTE_MAX_LEN)
                noctets = 4;

            if (noctets == 0) {
                printf("Too big\n");
                goto err;
            }
            asn_push_bits(bp, len, 8 * noctets);
            asn_push_bits(bp, noctets, 7);
            asn_push_bits(bp, 1, 1);

        }

    }

err:
    return result;

}

static inline int asn_put_encoded_len(abuf_t *bp, int len)
{
    int result = 0;
#if ASNBUF_DEBUG
    printf("adding(putting) ASN encoded length=0x%0x(%d)\n", len, len);
#endif

    if (len > 0 && bp && bp->head && bp->data) {

        // If a an 8 bit length will suffice...
        if (len < 0x80) {
            asn_ncat_bits(bp, len, 8);
        } else {
            // We're going to be adding a 16 bit value, because length bigger than 127 and less than 16K
            if (len < ASN_2BYTE_16K_MAX_LEN) {
                // ASN Length encoding rules for lengths between 0x127 and 0x4000 ("16K")
                len |= ASN_2BYTE_BIT_SET;
                len &= ASN_2BYTE_BIT_CLEAR;
                // encode MSB and LSB
                asn_ncat_bits(bp, len >> 8, 8);
                asn_ncat_bits(bp, len & 0x00FF, 8);
            } else {
                fprintf(stderr, "encountered a ASN UPER len larger than reasonable. \n");
                result = 1;
                goto err;
            }

        }

    }

err:
    return result;

}


// Push the  actual length of the current abuf to the front, for ASN PER ecnoding rules
static inline int asn_push_len(abuf_t *bp)
{
    //return (asn_push_encoded_len(bp, abuf_byte_len(bp)));
    int len = abuf_bits_len(bp);
    int res = len / 8;
    if (len % 8 != 0)
        res +=  1;

    return (asn_push_encoded_len(bp, res));
}

// Push the  actual length of the current abuf to the front, for ASN CER ecnoding rules
static inline int asn_push_ieee1609_2_len(abuf_t *bp)
{
    //return (asn_push_encoded_len(bp, abuf_byte_len(bp)));
    int len = abuf_bits_len(bp);
    int res = len / 8;
    if (len % 8 != 0)
        res +=  1;

    return (asn_push_CER_encoded_len(bp, res));
}



/****************************************************************
 *
 * abuf_merge()
 *
 * Does not handle merging of fragments that are totally mis-aligned.
 * The "tail_unused" of the left fragment, and the "head_headspace" must be perfectly matched
 * it would be very expensive, if they were not, as all bytes would have to be shifted
 *
 *
 * returns the length of the new cobined abuf bitstream, or -1 if error encountered
 *
****************************************************************/
static inline int abuf_merge(abuf_t *first_and_combined_abuf_p, abuf_t *add_on_abuf_p)
{
    int retval = -1;
    char *cp;

    if (first_and_combined_abuf_p && add_on_abuf_p) {
        int add_on_len = abuf_byte_len(add_on_abuf_p);


        // Two basic flavors of ASN buffer merge -- one that combines the bits that are left over from one byte with the
        // bits of the first byte of the add_on_buf buffer... In theory, the head_head_space of the add on + bits_left
        // of second should add up to 8...   The second flavor is just  clean byte aligned concatenation.
        if (add_on_abuf_p->head_headspace_bits != 0 || first_and_combined_abuf_p->tail_bits_left != 0) {
#if ASNBUF_DEBUG
            printf("[[  merging in %d bytes, bitwise OR: 0x%0x | 0x%0x ]]", add_on_len,
                *first_and_combined_abuf_p->tail,
                *add_on_abuf_p->data);
#endif

            *add_on_abuf_p->data = *add_on_abuf_p->data | *first_and_combined_abuf_p->tail;
            // put enough room onto tail to accomodate all but one byte (the first byte which will be bitwise merged)
            cp = abuf_put(first_and_combined_abuf_p, add_on_len - 1);
        } else {
#if ASNBUF_DEBUG
            printf("[[  merging in %d bytes-- clean byte aligned ASN buffers ]]", add_on_len);
#endif
            cp = abuf_put(first_and_combined_abuf_p, add_on_len);

            cp++; // advance past the last full byte;

        }


        if (cp) {
            memcpy(cp, add_on_abuf_p->data, add_on_len);
            first_and_combined_abuf_p->tail_bits_left = add_on_abuf_p->tail_bits_left;
            //printf("[set tail_bits_left=%d]\n", first_and_combined_abuf_p->tail_bits_left);
            retval = abuf_byte_len(first_and_combined_abuf_p);
        } else {
            printf("abuf_put() error on merging two ASN UPER strings\n");
            printf("\nleft buf:");
            abuf_dump(first_and_combined_abuf_p);
            printf("\nadd on:");
            abuf_dump(add_on_abuf_p);
        }
    }
    return (retval);

}

/****************************************************************
 *
 * abuf_merge_second()
 *
 * Does not handle merging of fragments that are totally mis-aligned.
 * This function takes the first buffer and merges it into front of the second buffer
 * returns the length of the new cobined abuf bitstream, or -1 if error encountered
****************************************************************/
static inline int abuf_merge_second(abuf_t *first_abuf_p, abuf_t *combined_abuf_p)
{
    int retval = -1;
    char *cp;

    if (first_abuf_p && combined_abuf_p) {

        abuf_t *ab = first_abuf_p;
        cp = ab->tail;

        while (cp >= ab->data) {

            asn_push_bits(combined_abuf_p, *((uint8_t *)cp), 8);
            cp--;

        }

    }

    return (retval);

}



/*********************************************************************************************************
 * calculate how many additional bytes will be needed to get n bits out of what is "bits left" bits
 * of the current byte, plus  the n bits desired out of the bitstream
 * will return 0 if there are enough bits left in the current byte (bits_left>n)
 *
 * This is a critical function for range checking to prevent buffer overflows.
 *
 **********************************************************************************************************/

static inline uint8_t calc_bytes_needed_for_n_more_bits(int n, int bits_left)
{
    return (((n - bits_left) / 8) + 1);
}


/********************************************************************************
 *  Retrieve n=[1..32]  bits from the byte stream, starting at offset bits_left_p  [1-8]
 *   Caller must make sure memory pointer is good for at least n bits before calling
 ***************************************************************************************/
static inline uint32_t  get_next_n_bits(unsigned char **cpp, int n, int *bits_left_p)
{

    register unsigned char *cp = *cpp; // cpp is used to point to the current byte in the stream
                       // and return those
    register int bits_left = *bits_left_p;
    unsigned int result;
    int addtl_bytes_needed = 0;

    //printf("get_next_%d(0x%0x, bits_left=%d) ",n,*cp,bits_left);

    if (n > bits_left) {
        addtl_bytes_needed = calc_bytes_needed_for_n_more_bits(n, bits_left);
    }

    // printf("case %d.%d:%0x ",addtl_bytes_needed,bits_left,*cp);
    // So exactly how many bytes do we need to get this data element... 0? 1? 2? 3?
    switch (addtl_bytes_needed) {

        // Everything we need is in this one first byte.
    case 0:
        // the bits are all within this byte is the most efficient case.
        // printf ("mask=%02x on val=%0x ", ((1 << n)-1) << (bits_left-n),*cp);
        result = (*cp & (((1 << n) - 1) << (bits_left - n))) >> (bits_left - n);
        bits_left -= n;
        break;

        // Everything we need spans just 2 bytes
    case 1:
        {
            register int bits_from_second_byte = n - bits_left;
            register int second_byte_shift = 8 - bits_from_second_byte;

            // printf ("mask=%02x ", ((1 << bits_left)-1) );
            result  = (*cp++ & (((1 << bits_left) - 1))) << (bits_from_second_byte);

            result |= (*cp & (((1 << bits_from_second_byte) - 1) << second_byte_shift)) >> second_byte_shift;

            bits_left = second_byte_shift;
        }
        break;

        // bitstring wanted comes from 3 bytes, so between 10 and 24 bits
    case 2:
        {
            register int bits_from_third_byte = n - 8 - bits_left;
            register int third_byte_shift = 8 - bits_from_third_byte;

            // grab all available in 1st byte
            result  = (*cp++ & (((1 << bits_left) - 1))) << (8 + bits_from_third_byte);

            result |= (*cp++ << bits_from_third_byte);   // grab all of second byte
            result |= (*cp & (((1 << bits_from_third_byte) - 1) << third_byte_shift)) >> third_byte_shift;

            bits_left = third_byte_shift;
        }
        break;


    case 3:
        {
            register int bits_from_last_byte = n - 16 - bits_left;
            register int last_byte_shift = 8 - bits_from_last_byte;

            result  = *cp++ & (((1 << bits_left) - 1)); // grab all available in 1st byte
            result <<= 8;
            result |= (*cp++);   // grab all of second byte
            result <<= 8;
            result |= (*cp++);   // grab all of third byte
            result <<= bits_from_last_byte;  // make room for remainder bits
            result |= (*cp & (((1 << bits_from_last_byte) - 1) << last_byte_shift)) >> last_byte_shift;

            bits_left = last_byte_shift;
        }
        break;


    case 4:
        {
            register int bits_from_last_byte = n - 24 - bits_left;
            register int last_byte_shift = 8 - bits_from_last_byte;

            result  = *cp++ & (((1 << bits_left) - 1)); // grab all available in 1st byte
            result <<= 8;
            result |= (*cp++);   // grab all of second byte
            result <<= 8;
            result |= (*cp++);   // grab all of third byte
            result <<= 8;
            result |= (*cp++);   // grab all of fourth byte

            result <<= bits_from_last_byte;  // make room for remainder bits
            result |= (*cp & (((1 << bits_from_last_byte) - 1) << last_byte_shift)) >> last_byte_shift;
            // printf("bits from last byte=%d ",bits_from_last_byte);
            bits_left = last_byte_shift;

        }

        break;

    default :
        /* For larger  than just a few bytes , we'll use a loop, and decrement n bits needed as we go
          this is more expensive than the concise cases above... but we'll have to do it.
             As things are now -- this should not happen, since we can only return 32 bits at a time.
        */
        {

            result  = *cp++ & (((1 << bits_left) - 1)); // grab all available in 1st byte
            n -= bits_left;
            // OR in 8 bits at a time, growing the resulting word
            while (n > 8) {
                result <<= 8;
                result |= (*cp++);   // grab all the next byte
                n -= 8;
            }

            // printf("bits from last byte=%d ",n);

            result |= (*cp & (((1 << n) - 1) << (8 - n))) >> (8 - n);


        }


        break;
    }

    if (!bits_left) {
        cp++;
        bits_left = 8;
    }

    *bits_left_p = bits_left; // return the adjusted bits remaining unparsed in the last byte

    *cpp = cp; // return the advanced pointer

    return (result);
}


static void abuf_dump(abuf_t *bp)
{
    int n;
    int data_size;
    char *cp;
    if (bp) {
        data_size = abuf_byte_len(bp);
        cp = bp->data;
        assert(cp);
        printf("%lu  %lu %lu ", (unsigned long)bp->head, (unsigned long) bp->data,
               (unsigned long) bp->tail);
        printf("abuf: headroom=%d (%d bits,%d @ head) used=%d(%d bits):  ",
               abuf_headroom(bp),
               abuf_headroom_bits(bp),
               bp->head_headspace_bits,
               data_size,
               abuf_bits_len(bp)
               );
        for (n = 0; n < data_size; n++) {

            printf("%02x ", *cp++);
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif // #ifndef _ASNBUF_H_
