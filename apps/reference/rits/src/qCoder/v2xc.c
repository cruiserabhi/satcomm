/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * @file v2xc.c v2x ASN.1 command line decoder
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "v2x_codec.h"

#define MAX_BUF_LEN 2048
#define PKT_TYPE_UNKNOWN    0
#define PKT_TYPE_WSMP       1
#define PKT_TYPE_BSM        2

static char *usage =
    "************************************************************************\n"
    "v2xc - v2x encoder/decoder command line tool.\n"
    "\n"
    "Usage:\n"
    "\n"
    "  ./v2xc [options]\n"
    "\n"
    "  -i[input packet type] can be 'wsmp', 'bsm'.\n"
    "  -f[file name] input file name\n"
    "************************************************************************\n"
;
static void print_help(void)
{
    fprintf(stdout, "%s", usage);
    exit(0);
}

int main(int argc, char **argv)
{
    int c;
    FILE *fpi = NULL;
    size_t buf_len = MAX_BUF_LEN;
    int pkt_type = PKT_TYPE_UNKNOWN;
    msg_contents mc;

    abuf_alloc(&mc.abuf, MAX_BUF_LEN, 200);

    if (argc <= 1) {
        print_help();
    }
    while ((c = getopt(argc, argv, "i:f:h")) != -1) {
        switch (c) {
        case 'f':
            if (!optarg) {
                print_help();
            } else if (!(fpi = fopen(optarg, "r"))) {
                fprintf(stderr, "Failed to open file %s for reading\n",
                        optarg);
                exit(-1);
            } else if ((buf_len = fread(mc.abuf.data, 1, buf_len, fpi)) == MAX_BUF_LEN - 200) {
                fprintf(stderr, "File is too large\n");
                fclose(fpi);
                exit(-1);
            }
            abuf_put(&mc.abuf, buf_len);
            break;
        case 'i':
            if (!optarg) {
                print_help();
            } else if (!strncmp(optarg, "wsmp", strlen("wsmp"))) {
                pkt_type = PKT_TYPE_WSMP;
            } else if (!strncmp(optarg, "bsm", strlen("bsm"))) {
                pkt_type = PKT_TYPE_BSM;
            } else {
                print_help();
            }
            break;
        case 'h':
            print_help();
            break;
        default:
            abort();
        }
    }

    if (pkt_type == PKT_TYPE_BSM) {
        decode_as_j2735(&mc);
    }
    if (NULL != fpi) {
        fclose(fpi);
    }
    return 0;
}
