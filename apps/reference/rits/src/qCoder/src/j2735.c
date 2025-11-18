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
 * @file j2735.c
 * @brief library for encoding/decoding J2945/1 in the 2016 UPER J2735
 *      Basic Safety Messages (BSM)
 *
 * This module contains the encode/decode functions for J2735 BSM's from the
 * 2016 Data dictionary.The structures in the include file (bsm.h) are key to
 * passing in the contents to be encoded or visa/versa
 *
 */

#include <stdint.h>
#include <assert.h>
#include "utils.h"
#include "v2x_msg.h"
#include "j2735.h"

//should create some system wide macro for debuf prints, rather
// than using these global verbosity controls and printf()
extern int gVerbosity;

/* Pass-in a abp pointer to an abuf, to where we can
   build a  SafetyExtension out of the data in the pktbuf_t pointed to by param db
   This should be a clean fresh abuf, large enough to hold worst case PH plus all options
*/

static void build_VehicleSafetyExt(abuf_t *abp, bsm_value_t *BSM_p)
{
    uint32_t options;
int save_bits_left_align; // only used for run-time validation, could be eliminated.
if (gVerbosity > 6)
    printf("\nbuild_VehicleSafetyExt\n");

// Weed out problems with NULL ptrs.
if (!abp || !BSM_p) {
    goto build_err;
}

save_bits_left_align = abp->tail_bits_left;

asn_ncat_bits(abp, 0, 1); //  is_extended = 0
              //abuf_dump(abp);
options = BSM_p->vehsafeopts;

asn_ncat_bits(abp, options, PART_II_SAFETY_EXT_OPTION_QTY);

//abuf_dump(abp);
if (options & PART_II_SAFETY_EXT_OPTION_EVENTS) {
    asn_ncat_bits(abp, 0, 1); //  is_extended = 0
    if (asn_ncat_bits(abp, BSM_p->events.data, PART_II_SAFETY_EXT_EVENTS_LEN_BITS) >= 0) {
        if (gVerbosity > 5) {
            printf("events 0x%0x added", BSM_p->events.data);
            abuf_dump(abp);
        }
    } else {
        printf("ERROR adding Part_II Event flags\n");
    }
}

if (options & PART_II_SAFETY_EXT_OPTION_PATH_HISTORY) {
    int m;
    int ph_opts = BSM_p->phopts;

    asn_ncat_bits(abp, 0, 1); //  is_extended = 0

    asn_ncat_bits(abp, ph_opts, PATH_HISTORY_OPTIONS_QTY);

    if (gVerbosity > 5) {
        printf("\n ENCODING Part-II PATH HISTORY ph_opts=%0x\n", ph_opts);
    }

    // OK, so options bits say we have an inital position, then we get a FullPositionVector object
    if (ph_opts & PATH_HISTORY_OPTION_INITALPOSITION) {
        uint32_t fpv_options;

        BSM_p->ph.initialPosition.opts.bits.has_utcTime = 0; // KABOB for now

        fpv_options = BSM_p->ph.initialPosition.opts.byte;

        asn_ncat_bits(abp, 0, 1); //  is_extended = 0

        if (gVerbosity > 7) {
            printf(" PH has initial position: options=%0x\n", fpv_options);
        }

        asn_ncat_bits(abp, fpv_options, FULLPOSITIONVECTOR_OPTIONS_QTY);
        if (fpv_options & FULLPOSITIONVECTOR_OPTION_UTCTIME) {
            //asn_push_bits(db,BSM_p->ph.initialPosition.utcTime ,FULLPOSITIONVECTOR_OPTIONS_QTY);
            // KABOB
            printf("Presently unsupported population of PH initial UTC \n");
        }

        // Lat & Long are mandatory elemnts of the inital position
        asn_ncat_bits(abp, BSM_p->ph.initialPosition.lon - BSM_ASN_LONGITUDE_ENCODE_OFFSET, LONGITUDE_LEN_BITS);
        asn_ncat_bits(abp, BSM_p->ph.initialPosition.lat - BSM_ASN_LATITUDE_ENCODE_OFFSET, LATITUDE_LEN_BITS);


        if (fpv_options & FULLPOSITIONVECTOR_OPTION_HEADING) {
            asn_ncat_bits(abp, BSM_p->ph.initialPosition.heading, HEADING_LEN_BITS);
        }

        if (fpv_options & FULLPOSITIONVECTOR_OPTION_ELEVATION) {
            asn_ncat_bits(abp, BSM_p->ph.initialPosition.elevation - BSM_ASN_ELEVATION_ENCODE_OFFSET,
                ELEVATION_LEN_BITS);
        }

        if (fpv_options & FULLPOSITIONVECTOR_OPTION_HEADING) {
            asn_ncat_bits(abp, BSM_p->ph.initialPosition.heading, HEADING_LEN_BITS);
        }

        if (fpv_options & FULLPOSITIONVECTOR_OPTION_POS_ACCURACY) {
            asn_ncat_bits(abp, BSM_p->ph.initialPosition.pos_accuracy.semi_major, SEMIMAJOR_ACCURACY_LEN_BITS);
            asn_ncat_bits(abp, BSM_p->ph.initialPosition.pos_accuracy.semi_minor, SEMIMINOR_ACCURACY_LEN_BITS);
            asn_ncat_bits(abp, BSM_p->ph.initialPosition.pos_accuracy.orientation, SEMIMAJOR_ORIENTATION_LEN_BITS);
        }


        if (fpv_options & FULLPOSITIONVECTOR_OPTION_TIME_CONFIDENCE) {
            asn_ncat_bits(abp, BSM_p->ph.initialPosition.time_confidence, TIME_CONFIDENCE_LEN_BITS);
            }

            if (fpv_options & FULLPOSITIONVECTOR_OPTION_POS_CONFIDENCE) {
                asn_ncat_bits(abp, BSM_p->ph.initialPosition.pos_confidence.xy, POSITION_CONFIDENCE_LEN_BITS);
                asn_ncat_bits(abp, BSM_p->ph.initialPosition.pos_confidence.elevation, ELEVATION_CONFIDENCE_LEN_BITS);
            }

            if (fpv_options & FULLPOSITIONVECTOR_OPTION_SPEED_CONFIDENCE) {
                asn_ncat_bits(abp, BSM_p->ph.initialPosition.motion_confidence_set.heading_confidence,
                    HEADING_CONFIDENCE_LEN_BITS);
                asn_ncat_bits(abp, BSM_p->ph.initialPosition.motion_confidence_set.speed_confidence,
                    SPEED_CONFIDENCE_LEN_BITS);
                asn_ncat_bits(abp, BSM_p->ph.initialPosition.motion_confidence_set.throttle_confidence,
                    THROTTLE_CONFIDENCE_LEN_BITS);
            }
        }


        if (ph_opts & PATH_HISTORY_OPTION_GNSS_STATUS) {
            asn_ncat_bits(abp, BSM_p->ph.gnss_status.data, GNSS_STATUS_LEN_BITS);

        }

        // Crumb data is not optional... must be at least 1, re-use sequence_len var.i
        // earlier value is no longer needed
        // Now at this point, at least one PH Point must be added


        if (gVerbosity > 5) {
            printf("\nPATH HISTORY crumb count: %d\n", BSM_p->ph.qty_crumbs);
        }

        // 1 cumb point is encoded as 0 by ASN rules, way, hence the minus 1
        asn_ncat_bits(abp, BSM_p->ph.qty_crumbs - 1, PATH_HISTORY_SEQUENCE_SIZE_LEN_BITS);

        for (m = 0; m < BSM_p->ph.qty_crumbs; m++) {
            uint8_t ph_crumb_options = BSM_p->ph.ph_crumb[m].opts_u.byte;
            // Now PathHistoryPoint offset sequence
            asn_ncat_bits(abp, 0, 1); //  is_extended = 0

            /*  The list of points a SEQUENCE of:
                latOffset
                lonLoffset
                elevationOffset
                timedOffset
                speed               // OPTIONAL
                posAccuracy             // OPTIONAL
                heading             // OPTIONAL
                 3 optional history points
            */
            asn_ncat_bits(abp, ph_crumb_options, PATH_HISTORY_POINT_OPTIONS_QTY);

            if (gVerbosity > 5) {
                printf("\n  #%2d: millisecond t=%-7d (%d,%d,%d) ", m,
                    BSM_p->ph.ph_crumb[m].timeOffset_ms,
                    BSM_p->ph.ph_crumb[m].latOffset,
                    BSM_p->ph.ph_crumb[m].lonOffset,
                    BSM_p->ph.ph_crumb[m].eleOffset
                    );

            }

            //abuf_dump(abp);

            asn_ncat_bits(abp, BSM_p->ph.ph_crumb[m].latOffset - LAT_OFFSET_MIN_VALUE, LAT_OFFSET_LEN_BITS);
            asn_ncat_bits(abp, BSM_p->ph.ph_crumb[m].lonOffset - LON_OFFSET_MIN_VALUE, LON_OFFSET_LEN_BITS);
            asn_ncat_bits(abp, BSM_p->ph.ph_crumb[m].eleOffset - ELE_OFFSET_MIN_VALUE, ELEVATION_OFFSET_LEN_BITS);
            asn_ncat_bits(abp, (BSM_p->ph.ph_crumb[m].timeOffset_ms - TIME_OFFSET_MIN_VALUE) / 10, TIME_OFFSET_LEN_BITS);
            //abuf_dump(abp);
            if (ph_crumb_options & PATH_HISTORY_POINT_OPTION_SPEED) {
                asn_ncat_bits(abp, BSM_p->ph.ph_crumb[m].speed, PATH_CRUMB_SPEED_LEN_BITS);
            }


            if (ph_crumb_options & PATH_HISTORY_POINT_OPTION_ACCURACY) {
                asn_ncat_bits(abp, BSM_p->ph.ph_crumb[m].accy.semi_major, SEMIMAJOR_ACCURACY_LEN_BITS);
                asn_ncat_bits(abp, BSM_p->ph.ph_crumb[m].accy.semi_minor, SEMIMAJOR_ACCURACY_LEN_BITS);
                asn_ncat_bits(abp, BSM_p->ph.ph_crumb[m].accy.orientation, SEMIMAJOR_ORIENTATION_LEN_BITS);
            }


            if (ph_crumb_options & PATH_HISTORY_POINT_OPTION_HEADING) {
                int tmp_val;

                if (BSM_p->ph.ph_crumb[m].heading_available == V2X_True) {
                    tmp_val = BSM_p->ph.ph_crumb[m].heading_microdegrees / MICRO_DEGREES_PER_COARSE_HEADING_LSB;
                } else {
                    tmp_val = COARSE_HEADING_UNAVAILABLE;
                }

                asn_ncat_bits(abp, tmp_val, COARSE_HEADING_LEN_BITS);


            }

            //abuf_dump(abp);
        } // for each PH point


    }

    if (options & PART_II_SAFETY_EXT_OPTION_PATH_PREDICTION) {

        asn_ncat_bits(abp, 0, 1); //  is_extended = 0
        /*
        if (BSM_p->pp.is_straight ) {
            BSM_p->pp.radius = PATH_RADIUS_STRAIGHT  ;
        }*/

        asn_ncat_bits(abp, BSM_p->pp.radius -  PATH_RADIUS_MIN_OFFSET, PATH_PREDICTION_RADIUS_LEN_BITS);

        asn_ncat_bits(abp, BSM_p->pp.confidence,
            PATH_PREDICTION_CONFIDENCE_LEN_BITS);

    }

    if (options & PART_II_SAFETY_EXT_OPTION_LIGHTS) {
        asn_ncat_bits(abp, 0, 1); //  is_extended = 0
        asn_ncat_bits(abp, BSM_p->lights_in_use.data, LIGHTS_IN_USE_LEN_BITS);

        if (gVerbosity > 5) {
            printf("\nlights 0x%0x added\n", BSM_p->lights_in_use.data);
        }
    }

    abuf_pad_to_even_octet(abp);

    if (save_bits_left_align !=  abp->tail_bits_left) {
        printf(" ASN encoding ERROR [[%d vs %d now]]", save_bits_left_align, abp->tail_bits_left);
        assert(save_bits_left_align ==  abp->tail_bits_left);
    }

    if (gVerbosity > 6) {
        abuf_dump(abp);
    }

    /* need to align  is suprising, given that this is UPER, however we're
     * we're not exactly aligning to a byte boundry, but padding with zeros
     * to make the length of the UPER an interer multiple of *8 bits
     *
     *
     *
    */
    asn_push_len(abp);
    if (gVerbosity > 4) {
        printf("completed VehicleSafetyExt:\n");
        abuf_dump(abp);
    }

    goto done;

build_err:
    fprintf(stderr, " Error in the build_VehicleSafetyExtension");

done:
    return;

}

static void build_SpecialExt(abuf_t *abp, bsm_value_t *BSM_p)
{
    uint32_t opts;
    int save_bits_left_align;

    if (gVerbosity > 6)
        printf("\nbuild_specialExt\n");
    if (!abp || !BSM_p) {
        goto enc_special_ext_err;
    }
    opts = BSM_p->specvehopts;
    save_bits_left_align = abp->tail_bits_left;

    asn_ncat_bits(abp, 0, 1); //  is_extended = 0
    asn_ncat_bits(abp, opts, SPECIAL_VEH_EXT_OPTIONS_QTY);

    if (opts & SPECIAL_VEH_EXT_OPTION_EMERGENCY_DETAILS) {

        if (gVerbosity > 7) {
            printf("\nEmergencyDetails are present\n");
        }

        asn_ncat_bits(abp, 0, 1); //  is_extended = 0

        uint32_t edopts = BSM_p->edopts;

        asn_ncat_bits(abp, edopts, SPECIAL_VEH_EMERGENCY_DATA_OPTIONS_QTY);

        asn_ncat_bits(abp, BSM_p->vehicleAlerts.sspRights, SPECIAL_VEH_SSP_LEN_BITS);

        asn_ncat_bits(abp, BSM_p->vehicleAlerts.sirenUse, SPECIAL_VEH_SIREN_LEN_BITS);

        asn_ncat_bits(abp, BSM_p->vehicleAlerts.lightsUse, SPECIAL_VEH_LIGHTS_USE_LEN_BITS);

        asn_ncat_bits(abp, BSM_p->vehicleAlerts.multi, SPECIAL_VEH_MULTI_LEN_BITS);

        if (edopts & EMERGENCY_DATA_OPTION_PRIVILEGED_EVENT) {
            if (gVerbosity > 7) {
                printf("\nPrivilegedEvents are present\n");
            }

            asn_ncat_bits(abp, 0, 1); //  is_extended = 0

            asn_ncat_bits(abp, BSM_p->vehicleAlerts.sspRights, SPECIAL_VEH_SSP_LEN_BITS);

            asn_ncat_bits(abp, BSM_p->vehicleAlerts.events.event, SPECIAL_VEH_EVENT_LEN_BITS);

        }

        if (edopts & EMERGENCY_DATA_OPTION_RESPONSE_TYPE) {
            if (gVerbosity > 7) {
                printf("\nResponseType is present\n");
            }

            asn_ncat_bits(abp, 0, 1); //  is_extended = 0

            asn_ncat_bits(abp, BSM_p->vehicleAlerts.responseType, SPECIAL_VEH_REPONSE_TYPE_LEN_BITS);
        }

    }

    if (opts & SPECIAL_VEH_EXT_OPTION_EVENT_DESC) {

        if (gVerbosity > 7)
            printf("\nEventDescriptions are present\n");

        asn_ncat_bits(abp, 0, 1); //  is_extended = 0

        uint32_t eventopts = BSM_p->eventopts;

        asn_ncat_bits(abp, eventopts, SPECIAL_VEH_EVENT_OPTIONS_QTY);

        asn_ncat_bits(abp, BSM_p->description.typeEvent, SPECIAL_VEH_EVENT_LEN_BITS);

        if (eventopts & SPECIAL_VEH_EVENT_OPTION_DESC) {
            int m;
            if (gVerbosity > 7)
                printf("\nDescription is present\n");

            asn_ncat_bits(abp, BSM_p->description.size_desc - 1, SPECIAL_VEH_EVENT_OPTION_DESC_COUNT_BITS);

            for (m = 0; m < BSM_p->description.size_desc; m++) {
                asn_ncat_bits(abp, BSM_p->description.desc[m], SPECIAL_VEH_EVENT_DESC_LEN_BITS);
            }

        }

        if (eventopts & SPECIAL_VEH_EVENT_OPTION_PRIOIRTY) {
            if (gVerbosity > 7)
                printf("\nPriority is present\n");
            asn_ncat_bits(abp, BSM_p->description.priority, SPECIAL_VEH_EVENT_PRIORITY_LEN_BITS);
        }

        if (eventopts & SPECIAL_VEH_EVENT_OPTION_HEADINGSLICE) {
            if (gVerbosity > 7)
                printf("\nHeadingSlice is present\n");
            asn_ncat_bits(abp, BSM_p->description.heading, SPECIAL_VEH_EVENT_DESC_LEN_BITS);
        }

        if (eventopts & SPECIAL_VEH_EVENT_OPTION_EXTENT) {
            if (gVerbosity > 7)
                printf("\nextent is present\n");
            asn_ncat_bits(abp, BSM_p->description.extent, SPECIAL_VEH_EVENT_EXTENT_LEN_BITS);
        }

        if (eventopts & SPECIAL_VEH_EVENT_OPTION_REGIONAL_EXT) {
            if (gVerbosity > 7)
                printf("\nRegional extension is present\n");
        }

    }


    if (opts & SPECIAL_VEH_EXT_OPTION_TRAILER_DATA) {
        if (gVerbosity > 7)
            printf("\nTrailerData are present\n");
    }

    if (gVerbosity > 6) {
        printf(" ASN encoding  [[%d vs %d now]]", save_bits_left_align, abp->tail_bits_left);
    }

    abuf_pad_to_even_octet(abp);

    // These push operations do not mess up the carefully pre-planned
    // reserved headspace, since always pushing an integer number of bytes
    asn_push_len(abp);

    if (save_bits_left_align !=  abp->tail_bits_left) {
        printf(" ASN encoding ERROR [[%d vs %d now]]", save_bits_left_align, abp->tail_bits_left);
        assert(save_bits_left_align ==  abp->tail_bits_left);
    }

    if (gVerbosity > 6) {
        printf("Completed Special Extension: ");
        abuf_dump(abp);
    }

    goto done;

enc_special_ext_err:
    fprintf(stderr, " Error in the build_Special_Extension");

done:
    return;
}

/*
 * Build an ASN Abuf for the BSM "SupplementalExtension"
 *
 */

static void build_SupplementalExt(abuf_t *abp, bsm_value_t *BSM_p)
{
    uint32_t opts;
    int save_bits_left_align;

    if (gVerbosity > 6)
        printf("\nbuild_SupplementalExt\n");
    if (!abp || !BSM_p) {
        goto enc_supplemental_ext_err;
    }
    opts = BSM_p->suppvehopts;
    save_bits_left_align = abp->tail_bits_left;

    asn_ncat_bits(abp, 0, 1); //  is_extended = 0
    asn_ncat_bits(abp, opts, SUPPLEMENT_VEH_EXT_OPTIONS_QTY);

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_CLASSIFICATION) {
        asn_ncat_bits(abp, BSM_p->VehicleClass, 8); //  is_extended = 0

    }

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_CLASS_DETAILS) {
        if (gVerbosity > 0) {
            fprintf(stderr, "unhandled SupplelmentalVehicleExtension: Classification Details\n");
            printf("%d\n", opts);
        }
        goto enc_supplemental_ext_err;
    }

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_VEHICLE_DATA) {
        uint32_t veh_data_options;

        veh_data_options = BSM_p->veh.supplemental_veh_data_options.word;

        asn_ncat_bits(abp, 0, 1); //  is_extended = 0

        asn_ncat_bits(abp, veh_data_options, SUPPLEMENT_VEH_DATA_OPTIONS_QTY);

        if (veh_data_options & SUPPLEMENT_VEH_DATA_OPTION_HEIGHT) {
            uint32_t tmp = BSM_p->veh.height_cm / VEHICLE_DATA_HEIGHT_CM_PER_LSB;

            // encoded in bits of 5 CM each LSB, accoring to ASN.1 spec
            asn_ncat_bits(abp, tmp, VEHICLE_DATA_HEIGHT_LEN_BITS);

        }

        if (veh_data_options & SUPPLEMENT_VEH_DATA_OPTION_BUMPERS) {

            asn_ncat_bits(abp, BSM_p->veh.front_bumper_height_cm, VEHICLE_DATA_BUMPER_HEIGHT_LEN_BITS);
            asn_ncat_bits(abp, BSM_p->veh.rear_bumper_height_cm, VEHICLE_DATA_BUMPER_HEIGHT_LEN_BITS);
        }

        if (veh_data_options & SUPPLEMENT_VEH_DATA_OPTION_MASS) {
            uint8_t tmp;

            tmp = VehicleMassEncode(BSM_p->veh.mass_kg);
            asn_ncat_bits(abp, tmp, VEHICLE_DATA_MASS_LEN_BITS);

        }

        if (veh_data_options & SUPPLEMENT_VEH_DATA_OPTION_TRAILER_WEIGHT) {
            asn_ncat_bits(abp, BSM_p->veh.trailer_weight, VEHICLE_DATA_TRAILER_WEIGHT_LEN_BITS);
        }


    }

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_WEATHER_REPORT) {
        fprintf(stderr, "unhandled SupplelmentalVehicleExtension: WeatherReport\n");
        goto enc_supplemental_ext_err;
    }

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_WEATHER_PROBE) {
        uint32_t weather_options, wiperopts;

        weather_options = BSM_p->weatheropts;

        asn_ncat_bits(abp, 0, 1); //  is_extended = 0

        asn_ncat_bits(abp, weather_options, SUPPLEMENT_WEATHER_OPTIONS_QTY);

        if (weather_options & SUPPLEMENT_WEATHER_AIRTEMP) {
            asn_ncat_bits(abp, BSM_p->airTemp, SUPPLEMENT_WEATHER_AIRTEMP_LEN_BITS);
        }

        if (weather_options & SUPPLEMENT_WEATHER_AIRPRESSURE) {
            asn_ncat_bits(abp, BSM_p->airPressure, SUPPLEMENT_WEATHER_AIRPRESSURE_LEN_BITS);
        }

        if (weather_options & SUPPLEMENT_WEATHER_WIPERS) {
            wiperopts = BSM_p->wiperopts;
            asn_ncat_bits(abp, BSM_p->wiperopts, SUPPLEMENT_WEATHER_WIPEROPT_LEN_BITS);
            asn_ncat_bits(abp, BSM_p->statusFront, SUPPLEMENT_WEATHER_WIPER_STATUS_LEN_BITS);
            asn_ncat_bits(abp, BSM_p->rateFront, SUPPLEMENT_WEATHER_WIPER_RATE_LEN_BITS);
            if (wiperopts & SUPPLEMENT_WEATHER_WIPERS_REAR_STATUS) {
                asn_ncat_bits(abp, BSM_p->statusRear, SUPPLEMENT_WEATHER_WIPER_STATUS_LEN_BITS);
            }
            if (wiperopts & SUPPLEMENT_WEATHER_WIPERS_REAR_RATE) {
                asn_ncat_bits(abp, BSM_p->rateRear, SUPPLEMENT_WEATHER_WIPER_RATE_LEN_BITS);
            }
        }

        /*fprintf(stderr, "unhandled SupplelmentalVehicleExtension: WeatherProbe\n");
        goto enc_supplemental_ext_err;*/
    }

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_OBSTACLE) {
        fprintf(stderr, "unhandled SupplelmentalVehicleExtension: Obstacle on Road\n");
        goto enc_supplemental_ext_err;
    }

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_DISABLED_VEH) {
        fprintf(stderr, "unhandled SupplelmentalVehicleExtension: Disabled Vehicle Report\n");
        goto enc_supplemental_ext_err;
    }

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_SPEED_PROFILE) {
        fprintf(stderr, "unhandled SupplelmentalVehicleExtension: Speed Profile\n");
        goto enc_supplemental_ext_err;
    }

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_RTCM) {
        fprintf(stderr, "unhandled SupplelmentalVehicleExtension: RTCM Corrections\n");
        goto enc_supplemental_ext_err;
    }

    if (opts & SUPPLEMENT_VEH_EXT_OPTION_REGIONAL_EXT) {
        fprintf(stderr, "unhandled regional SupplelmentalVehicleExtension.\n");
        goto enc_supplemental_ext_err;
    }

    if (gVerbosity > 6) {
        printf(" ASN encoding  [[%d vs %d now]]", save_bits_left_align, abp->tail_bits_left);
    }

    abuf_pad_to_even_octet(abp);

    // These push operations do not mess up the carefully pre-planned
    // reserved headspace, since always pushing an integer number of bytes
    asn_push_len(abp);

    if (save_bits_left_align !=  abp->tail_bits_left) {
        printf(" ASN encoding ERROR [[%d vs %d now]]", save_bits_left_align, abp->tail_bits_left);
        assert(save_bits_left_align ==  abp->tail_bits_left);
    }

    if (gVerbosity > 6) {
        printf("completed Supplemental Extension: ");
        abuf_dump(abp);
    }

    goto done;

enc_supplemental_ext_err:
    fprintf(stderr, " Error in the Supplemental Extension");

done:
    return;
}
static int encode_as_bsm(abuf_t *bp, bsm_value_t *BSM_p)
{
    register uint32_t *p32;
    uint8_t *p8;
    register uint32_t tmp = 0;
    register uint32_t v32; // temporary value , offset adjusted and ready to be added to bitstream

    int result = 0;

    if (!bp || !BSM_p) {
        fprintf(stderr, " bsm_encode called with null  pointers\n");
        goto bsm_encode_err;
    }

    /* Not setting any of these three fields, for now:
     * * BSM_FAST32_0_EXTENSION_MASK
     * * BSM_FAST32_1_OPTIONAL_PART2_MASK
     * * BSM_FAST32_2_OPTIONAL_REGIONAL_MASK */

    p32 = (uint32_t *)abuf_put(bp, 4);

    if (BSM_p->has_partII) {
        tmp  =  BSM_FAST32_1_OPTIONAL_PART2_MASK;
    }
    tmp |= BSM_p->MsgCount << BSM_FAST32_3_MSG_COUNT_BIT_SHIFT;
    tmp |= BSM_p->id  >>  BSM_FAST32_10_MSG_ID_SEG2_LEN;

    *p32  = htonl(tmp);

    p32 = (uint32_t *)abuf_put(bp, 4);

    tmp  = BSM_p->id << BSM_FAST32_10_MSG_ID_SEG2_SHIFT;
    tmp  |= BSM_p->secMark_ms <<  BSM_FAST32_42_SECMARK_BIT_SHIFT; // 16 bits from bit #22-6

    v32 =  BSM_p->Latitude - BSM_ASN_LATITUDE_ENCODE_OFFSET; //-900000000;
    tmp  |= v32 >> BSM_FAST32_58_LAT_SEG2_LEN; // 6 of 31 bits from bit #5-0

    *p32  = htonl(tmp);

    p32 = (uint32_t *)abuf_put(bp, 4);

    tmp  =  v32  << BSM_FAST32_58_LAT_SEG2_SHIFT; // 28 bits from bit #31-4

    v32 =  BSM_p->Longitude - BSM_ASN_LONGITUDE_ENCODE_OFFSET;
    tmp |=   v32 >> BSM_FAST32_89_LON_SEG2_LEN; // 6 of 31 bits from bit #5-0
    *p32  = htonl(tmp);
    p32 = (uint32_t *)abuf_put(bp, 4);

    tmp  =  v32 <<  BSM_FAST32_89_LON_SEG2_SHIFT; // 28 bits from bit #31-4

    v32 =  BSM_p->Elevation - BSM_ASN_ELEVATION_ENCODE_OFFSET;
    tmp  |= v32  >> BSM_FAST32_121_ELE_SEG2_LEN;       // 7 of 16 bits from #6-0
    *p32  = htonl(tmp);
    p32 = (uint32_t *)abuf_put(bp, 4);

    tmp = v32 << BSM_FAST32_121_ELE_SEG2_SHIFT;   // 9 lower bits of 16 bits from #31-23

    tmp |= BSM_p->SemiMajorAxisAccuracy  << BSM_FAST32_137_SEMIMAJOR_BIT_SHIFT; // 8 bits @ #22-14
    tmp |= BSM_p->SemiMinorAxisAccuracy <<  BSM_FAST32_145_SEMIMINOR_BIT_SHIFT; // 8 bits @  #13-5
    tmp |= BSM_p->SemiMajorAxisOrientation >> (BSM_FAST32_153_ACC_ORIENT_SEG2_LEN); // 7 of 16 bits in LSB
    *p32  = htonl(tmp);
    p32 = (uint32_t *)abuf_put(bp, 4);

    tmp  = BSM_p->SemiMajorAxisOrientation  << BSM_FAST32_153_ACC_ORIENT_SEG2_SHIFT;
    tmp |= BSM_p->TransmissionState  << BSM_FAST32_169_TRANNY_BIT_SHIFT; // 3 bits from #22-20
    tmp |= BSM_p->Speed << BSM_FAST32_172_SPEED_BIT_SHIFT; // 13 bits from #19-#7
    tmp |= BSM_p->Heading_degrees >> BSM_FAST32_185_HEADING_SEG2_LEN;

    *p32  = htonl(tmp);
    p32 = (uint32_t *)abuf_put(bp, 4);


    tmp = BSM_p->Heading_degrees << BSM_FAST32_185_HEADING_SEG2_SHIFT;
    v32 = BSM_p->SteeringWheelAngle - BSM_ASN_SWA_ENCODE_OFFSET;

    tmp |=  v32 << BSM_FAST32_200_SWA_BIT_SHIFT; // 8 bit SWA @ bit #23
    v32 = BSM_p->AccelLon_cm_per_sec_squared - BSM_ASN_ACCEL_LON_ENCODE_OFFSET;
    tmp |=  v32 << BSM_FAST32_208_ACC_LON_BIT_SHIFT;

    v32 = BSM_p->AccelLat_cm_per_sec_squared - BSM_ASN_ACCEL_LAT_ENCODE_OFFSET;
    tmp |= v32 >> BSM_FAST32_220_ACC_LAT_SEG2_LEN;

    *p32  = htonl(tmp);
    p32 = (uint32_t *)abuf_put(bp, 4);
    tmp  = v32 << BSM_FAST32_220_ACC_LAT_SEG2_SHIFT;

    v32 = BSM_p->AccelVert_two_centi_gs - BSM_ASN_ACCEL_VERT_ENCODE_OFFSET;
    tmp |= v32 << BSM_FAST32_232_ACC_VERT_BIT_SHIFT;

    v32 = BSM_p->AccelYaw_centi_degrees_per_sec - BSM_ASN_ACCEL_YAW_ENCODE_OFFSET;
    tmp |= v32; // << BSM_FAST32_240_ACC_YAW_SHIFT;  0 presently, so leave out the shift

    *p32  = htonl(tmp);
    p32 = (uint32_t *)abuf_put(bp, 4);

    // Coincidence, but in this happens to presently in (in 2016 standards) line up on an even byte boundry.

    tmp  = BSM_p->brakes.word << 16;
    tmp |= BSM_p->VehicleWidth_cm <<  BSM_FAST32_271_VEH_WIDTH_SHIFT; // 10 bits
    tmp |= BSM_p->VehicleLength_cm  >> BSM_FAST32_281_VEH_LEN_SEG2_LEN; // first 7 of  12 bits  VEH_LEN
    if (gVerbosity > 6)
        printf("[brakes.word=%02x, word=%08x]", BSM_p->brakes.word, tmp);

    *p32  = htonl(tmp);

    p8 = (uint8_t *)abuf_put(bp, 1);

    *p8 = BSM_p->VehicleLength_cm  << BSM_FAST32_281_VEH_LEN_SEG2_SHIFT; // LSB 5 bits of  12b bit VEH_LEN
    bp->tail--;
    bp->tail_bits_left = (8 - BSM_FAST32_281_VEH_LEN_SEG2_LEN); // 3 bits left unused

    if (BSM_p->has_partII) {
        abuf_t veh_ext_asnbuf;
        if (abuf_alloc(&veh_ext_asnbuf, 2000, 20) > 0) {

            asn_ncat_bits(bp, BSM_p->qty_partII_extensions  - 1, 3); // -1 due to J2735 ASN range from 1..8

            if (BSM_p->has_safety_extension) {
                asn_ncat_bits(bp, PartII_Id_vehicleSafetyExt, 6);
                //abuf_dump(bp);

                /*
                 * Ok.  this next step deserves some explanation -- we allocate some dummy space
                 * at the headspace of this newly created ASN bitstream, since it will be merged into the BSM
                 * But it has to be made seperately, because the variable enc. length is not known yet.
                 *
                 */


                // make sure buffer can be easily merged into main BSM later.
                abuf_reserve_headspace(&veh_ext_asnbuf, (8 - bp->tail_bits_left));

                if (gVerbosity > 4) {
                    printf("offset by %d headspace new abuf:", (8 - bp->tail_bits_left));
                    abuf_dump(&veh_ext_asnbuf);
                }

                abuf_trim(bp);
                build_VehicleSafetyExt(&veh_ext_asnbuf, BSM_p);
                abuf_trim(&veh_ext_asnbuf); // clean-up any hanging, but unpopulated bytes
                abuf_merge(bp, &veh_ext_asnbuf);

                abuf_purge(&veh_ext_asnbuf, 20);


            }

            if (BSM_p->has_special_extension) {
                abuf_purge(&veh_ext_asnbuf, 20);
                asn_ncat_bits(bp, PartII_Id_specialVehicleExt, 6);

                abuf_reserve_headspace(&veh_ext_asnbuf, (8 - bp->tail_bits_left));
                build_SpecialExt(&veh_ext_asnbuf, BSM_p);

                abuf_trim(&veh_ext_asnbuf); // clean-up any hanging, but unpopulated bytes

                abuf_merge(bp, &veh_ext_asnbuf);
            }

            if (BSM_p->has_supplemental_extension) {
                abuf_purge(&veh_ext_asnbuf, 20);
                asn_ncat_bits(bp, PartII_Id_supplementalVehicleExt, 6);

                abuf_reserve_headspace(&veh_ext_asnbuf, (8 - bp->tail_bits_left));
                build_SupplementalExt(&veh_ext_asnbuf, BSM_p);

                abuf_trim(&veh_ext_asnbuf); // clean-up any hanging, but unpopulated bytes

                abuf_merge(bp, &veh_ext_asnbuf);
            }

            abuf_free(&veh_ext_asnbuf);
        } else {
            printf("abuf_alloc() error\n");
            goto bsm_encode_err;
        }

    }
    /* if this BSM is not extended by optional fields, there might be only one byte
       remaining...so we won't load the next byte into a uint32 like we did previously
       We only have 5 bits more to complete the VehicleLength
    */
    asn_push_len(bp);
    tmp = (uint16_t)J2735_MSGID_BASIC_SAFETY; //dec=20 (0x14)
    asn_push_bits(bp, tmp, 16);
    if(gVerbosity > 4){
        printf("BSM encoded buffer dump\n");
        abuf_dump(bp);
        printf("\n");
    }
    goto bsm_encode_return;

bsm_encode_err:
    printf("BSM encode err\n");
    result = -1;

bsm_encode_return:
    return (result);
}

static int decode_as_bsm(msg_contents *mc)
{
    register uint32_t tmp;
    register uint32_t tmp32; // temporary value storage

    int len_remaining;
    int result = 0;
    uint32_t *p32;
    uint8_t *p8; // byte pointer, instead of 32bit pointer
    int bits_left;
    uint32_t is_extended;  //extension marker from UPER encodings -- used as a boolean
    uint32_t  sequence_len;
    uint32_t opts;

    if(!mc)
        goto bsm_decode_err;

    if(!mc->j2735_msg) {
        mc->j2735_msg  = calloc(sizeof(bsm_value_t), 1);
        if(!mc->j2735_msg)
            goto bsm_decode_err;
    }

    bsm_value_t* BSM_p = mc->j2735_msg;

    p32 = (uint32_t *)mc->abuf.data;
    len_remaining = mc->l3_payload_len;
    if (gVerbosity > 7) {
        printf("abuf payload: \n");
        print_buffer(mc->abuf.data, mc->payload_len);
        printf("\n\nl3_payload: \n");
        print_buffer(mc->l3_payload, mc->payload_len);
        printf("\n\n");
    }
    // save a ptr to the last byte of mesage, to bounds check before trying to decode
    uint8_t* last_byte_p = (uint8_t*) p32 + len_remaining - 1;


    if (len_remaining < MIN_BSM_CORE_OCTETS)  {
        printf(" frame too short to even contain core BSM\n");
        goto BSM_too_short;
    }

    tmp = ntohl(*(uint32_t *)p32++);

    if (gVerbosity > 7) {
        print_buffer(mc->abuf.data, mc->payload_len);
    }

    if (gVerbosity > 7) {
        printf(" EXT_MASK=%08x %08x | %08x %08x \n", tmp,
            BSM_FAST32_0_EXTENSION_MASK,
            BSM_FAST32_1_OPTIONAL_PART2_MASK,
            BSM_FAST32_2_OPTIONAL_REGIONAL_MASK);
    }

    if (tmp & BSM_FAST32_0_EXTENSION_MASK) {
        /* Extension flag bit -- we could support this, but will change all
            the fast parsing coded below,  will have to dissect using the other asn_get_n bits
             dissector
        */
        goto unsupported;
    }

    BSM_p->has_partII = (tmp & BSM_FAST32_1_OPTIONAL_PART2_MASK);
    if ((gVerbosity > 7) && BSM_p->has_partII) {
        printf(" BSM Part II present \n");
    }

    BSM_p->has_regional_extensions = (tmp & BSM_FAST32_2_OPTIONAL_REGIONAL_MASK);
    if ((gVerbosity > 7) && BSM_p->has_regional_extensions) {
        printf(" BSM local extensions included\n");
    }

    assert(BSM_FAST32_3_MSG_COUNT_BIT_MASK == 0x1fc00000);
    assert(BSM_FAST32_3_MSG_COUNT_BIT_SHIFT == (32 - 7 - 3));

    BSM_p->MsgCount = (tmp &  BSM_FAST32_3_MSG_COUNT_BIT_MASK) >> BSM_FAST32_3_MSG_COUNT_BIT_SHIFT;

    assert((uint32_t)BSM_FAST32_10_MSG_ID_SEG1_MASK == 0x003fffff);

    BSM_p->id  =  (tmp &  BSM_FAST32_10_MSG_ID_SEG1_MASK) << BSM_FAST32_10_MSG_ID_SEG2_LEN;

    tmp = ntohl(*(uint32_t *)p32++);

    BSM_p->id |= (tmp &  BSM_FAST32_10_MSG_ID_SEG2_MASK) >> BSM_FAST32_10_MSG_ID_SEG2_SHIFT;

    assert(BSM_FAST32_42_SECMARK_BIT_SHIFT == 6);
    BSM_p->secMark_ms = (tmp & BSM_FAST32_42_SECMARK_BIT_MASK) >>
        BSM_FAST32_42_SECMARK_BIT_SHIFT; // 16 bits from bit #22-6
    BSM_p->timestamp_ms = calc_timestamp_from_secmark(BSM_p->secMark_ms);
    // now we are into the 3D Position ASN.1
    // contains  an extension field, and two option elements (Altitude, and regional)

#if 0
    printf("58.1 #%d[%d]:%08x  << %d\n", BSM_FAST32_58_LAT_BIT_NUM, BSM_FAST32_58_LAT_SEG1_LEN, BSM_FAST32_58_LAT_SEG1_MASK,
           BSM_FAST32_58_LAT_SEG2_LEN);
    printf("58.2 #%d[%d]:%08x  >> %d\n", 0, BSM_FAST32_58_LAT_SEG2_LEN,
           BSM_FAST32_58_LAT_SEG2_MASK, BSM_FAST32_58_LAT_SEG2_SHIFT);
#endif

    assert(BSM_FAST32_58_LAT_SEG2_LEN == (LATITUDE_LEN_BITS - 6));
    assert(BSM_FAST32_58_LAT_SEG1_MASK == 0x3f);
    assert(BSM_FAST32_58_LAT_SEG2_MASK == 0xffffff80);
    assert(BSM_FAST32_58_LAT_SEG2_SHIFT == 32 - (LATITUDE_LEN_BITS - 6));


    tmp32  = (tmp & BSM_FAST32_58_LAT_SEG1_MASK) << BSM_FAST32_58_LAT_SEG2_LEN; // 6 of 31 bits from bit #5-0
    tmp = ntohl(*(uint32_t *)p32++);
    tmp32 |= (tmp & BSM_FAST32_58_LAT_SEG2_MASK) >> BSM_FAST32_58_LAT_SEG2_SHIFT; // 28 bits from bit #31-4

    BSM_p->Latitude = tmp32 + BSM_ASN_LATITUDE_ENCODE_OFFSET; //-900000000;

#if 0
    printf("89.1 #%d:%08x  << %d\n", BSM_FAST32_89_LON_BIT_NUM, BSM_FAST32_89_LON_SEG1_MASK, BSM_FAST32_89_LON_SEG2_LEN);
    printf("89.2 #%d:%08x  >> %d\n", 0, BSM_FAST32_89_LON_SEG2_MASK, BSM_FAST32_89_LON_SEG2_SHIFT);
#endif
    assert(BSM_FAST32_89_LON_SEG2_LEN == (LONGITUDE_LEN_BITS - 7));
    assert(BSM_FAST32_89_LON_SEG1_MASK == 0x7f);
    assert(BSM_FAST32_89_LON_SEG2_MASK == 0xffffff80);
    assert(BSM_FAST32_89_LON_SEG2_SHIFT == 32 - (LONGITUDE_LEN_BITS - 7));

    tmp32  =  (tmp & BSM_FAST32_89_LON_SEG1_MASK) << BSM_FAST32_89_LON_SEG2_LEN; // 6 of 31 bits from bit #5-0
    tmp = ntohl(*(uint32_t *)p32++);
    tmp32 |= (tmp & BSM_FAST32_89_LON_SEG2_MASK) >> BSM_FAST32_89_LON_SEG2_SHIFT; // 28 bits from bit #31-4


    BSM_p->Longitude = tmp32 + BSM_ASN_LONGITUDE_ENCODE_OFFSET;

    assert(BSM_FAST32_121_ELE_SEG1_LEN == 7);
    assert(BSM_FAST32_121_ELE_SEG2_LEN  == (16 - 7));
    assert(BSM_FAST32_121_ELE_SEG1_MASK     == 0x7f);
    assert(BSM_FAST32_121_ELE_SEG2_MASK     == 0xff800000);
    assert(BSM_FAST32_121_ELE_SEG2_SHIFT  == (32 - (16 - 7)));

    tmp32  = (tmp & BSM_FAST32_121_ELE_SEG1_MASK) << BSM_FAST32_121_ELE_SEG2_LEN;       // 7 of 16 bits from #6-0
    tmp = ntohl(*(uint32_t *)p32++);
    tmp32 |= (tmp & BSM_FAST32_121_ELE_SEG2_MASK) >> BSM_FAST32_121_ELE_SEG2_SHIFT
    ;   // 9 lower bits of 16 bits from #31-23

    BSM_p->Elevation = tmp32 + BSM_ASN_ELEVATION_ENCODE_OFFSET;

    assert(BSM_FAST32_137_SEMIMAJOR_BIT_MASK ==  0x007f8000);
    assert(BSM_FAST32_137_SEMIMAJOR_BIT_SHIFT == 1 + 22 - SEMIMAJOR_ACCURACY_LEN_BITS);

    assert(BSM_FAST32_145_SEMIMINOR_BIT_MASK ==  0x7f80);
    assert(BSM_FAST32_145_SEMIMINOR_BIT_SHIFT == 1 + 14 - SEMIMINOR_ACCURACY_LEN_BITS);

    assert(BSM_FAST32_153_ACC_ORIENT_SEG2_LEN == (SEMIMAJOR_ORIENTATION_LEN_BITS - 7));
    assert(BSM_FAST32_153_ACC_ORIENT_SEG1_MASK == 0x7f);
    assert(BSM_FAST32_153_ACC_ORIENT_SEG2_MASK == 0xff800000);
    assert(BSM_FAST32_153_ACC_ORIENT_SEG2_SHIFT == 32 - (SEMIMAJOR_ORIENTATION_LEN_BITS - 7));


    // 8 bits starting at #22-14
    BSM_p->SemiMajorAxisAccuracy     =  (tmp & BSM_FAST32_137_SEMIMAJOR_BIT_MASK) >>  BSM_FAST32_137_SEMIMAJOR_BIT_SHIFT;

    // 8 bits starting at #13-5
    BSM_p->SemiMinorAxisAccuracy     =  (tmp & BSM_FAST32_145_SEMIMINOR_BIT_MASK) >>  BSM_FAST32_145_SEMIMINOR_BIT_SHIFT;

    BSM_p->SemiMajorAxisOrientation  =  (tmp & BSM_FAST32_153_ACC_ORIENT_SEG1_MASK) <<
        (BSM_FAST32_153_ACC_ORIENT_SEG2_LEN); // 7 of 16 bits in LSB
    tmp = ntohl(*(uint32_t *)p32++);
    BSM_p->SemiMajorAxisOrientation |=  (tmp & BSM_FAST32_153_ACC_ORIENT_SEG2_MASK) >> BSM_FAST32_153_ACC_ORIENT_SEG2_SHIFT
    ;

    assert(BSM_FAST32_169_TRANNY_BIT_MASK == 0x00700000);
    assert(BSM_FAST32_169_TRANNY_BIT_SHIFT ==   (32 - (16 - 7) - 3));

    assert(BSM_FAST32_172_SPEED_BIT_MASK ==  0x000FFF80);
    assert(BSM_FAST32_172_SPEED_BIT_SHIFT ==   (1 + 19 - SPEED_LEN_BITS));

    assert(BSM_FAST32_185_HEADING_SEG2_MASK == 0xff000000);
    assert(BSM_FAST32_185_HEADING_SEG2_SHIFT == (32 - (HEADING_LEN_BITS - 7)));

    assert(BSM_FAST32_169_TRANNY_BIT_MASK == 0x00700000);
    assert(BSM_FAST32_169_TRANNY_BIT_SHIFT   == (32 - (16 - 7) - 3));
    BSM_p->TransmissionState =          (tmp & BSM_FAST32_169_TRANNY_BIT_MASK) >> BSM_FAST32_169_TRANNY_BIT_SHIFT
    ; // 3 bits from #22-20

    assert(BSM_FAST32_172_SPEED_BIT_MASK == 0x000FFF80);
    assert(BSM_FAST32_172_SPEED_BIT_SHIFT == (1 + 19 - SPEED_LEN_BITS));

    BSM_p->Speed =                      (tmp & BSM_FAST32_172_SPEED_BIT_MASK) >> (BSM_FAST32_172_SPEED_BIT_SHIFT)
    ; // 13 bits from #19-#7

    assert(BSM_FAST32_185_HEADING_SEG2_MASK     ==  0xff000000);
    assert(BSM_FAST32_185_HEADING_SEG2_SHIFT ==  (32 - (HEADING_LEN_BITS - 7)));

    BSM_p->Heading_degrees =            (tmp & BSM_FAST32_185_HEADING_SEG1_MASK) << BSM_FAST32_185_HEADING_SEG2_LEN;
    tmp = ntohl(*(uint32_t *)p32++);
    BSM_p->Heading_degrees |=           (tmp & BSM_FAST32_185_HEADING_SEG2_MASK) >>
        BSM_FAST32_185_HEADING_SEG2_SHIFT; // Heading LSB from bit 8 bits @ b31

    assert(BSM_FAST32_200_SWA_BIT_MASK ==  0x00ff0000);
    assert(BSM_FAST32_200_SWA_BIT_SHIFT == (1 + 23 - 8));

    // 8 bit steering wheel angle, start bit #23
    tmp32 =                              (tmp & BSM_FAST32_200_SWA_BIT_MASK) >> BSM_FAST32_200_SWA_BIT_SHIFT;
    BSM_p->SteeringWheelAngle = tmp32 + BSM_ASN_SWA_ENCODE_OFFSET;

#if 0
    printf("208   #%d:%08x  >> %d\n", BSM_FAST32_208_ACC_LON_BIT_NUM,
           BSM_FAST32_208_ACC_LON_BIT_MASK,
           BSM_FAST32_208_ACC_LON_BIT_SHIFT);
#endif

    assert(BSM_FAST32_208_ACC_LON_BIT_MASK ==  0x0000fff0);
    assert(BSM_FAST32_208_ACC_LON_BIT_SHIFT == (1 + 15 - 12));

    tmp32 =                             (tmp & BSM_FAST32_208_ACC_LON_BIT_MASK) >> BSM_FAST32_208_ACC_LON_BIT_SHIFT;
    BSM_p->AccelLon_cm_per_sec_squared = tmp32 + BSM_ASN_ACCEL_LON_ENCODE_OFFSET;

    assert(BSM_FAST32_220_ACC_LAT_SEG2_MASK == 0xff000000);
    assert(BSM_FAST32_220_ACC_LAT_SEG2_SHIFT     == (32 - (12 - 4)));

    tmp32 =                              (tmp & BSM_FAST32_220_ACC_LAT_SEG1_MASK) << BSM_FAST32_220_ACC_LAT_SEG2_LEN;
    tmp = ntohl(*(uint32_t *)p32++);
    tmp32 |=                             (tmp & BSM_FAST32_220_ACC_LAT_SEG2_MASK) >> BSM_FAST32_220_ACC_LAT_SEG2_SHIFT;
    BSM_p->AccelLat_cm_per_sec_squared = tmp32 + BSM_ASN_ACCEL_LAT_ENCODE_OFFSET;

    assert(BSM_FAST32_232_ACC_VERT_BIT_MASK == 0x00ff0000);
    assert(BSM_FAST32_232_ACC_VERT_BIT_SHIFT == (1 + 23 - 8));

    tmp32 =                               (tmp & BSM_FAST32_232_ACC_VERT_BIT_MASK) >>
        BSM_FAST32_232_ACC_VERT_BIT_SHIFT;

    BSM_p->AccelVert_two_centi_gs = tmp32 + BSM_ASN_ACCEL_VERT_ENCODE_OFFSET;


    assert(BSM_FAST32_240_ACC_YAW_MASK  == 0x0000ffff);
    assert(BSM_FAST32_240_ACC_YAW_SHIFT == (1 + 15 - 16));

    tmp32 =                               (tmp & BSM_FAST32_240_ACC_YAW_MASK) >>
        BSM_FAST32_240_ACC_YAW_SHIFT;

    BSM_p->AccelYaw_centi_degrees_per_sec = tmp32  + BSM_ASN_ACCEL_YAW_ENCODE_OFFSET;

    // Coincidence, but in this happens to presently in (in 2016 standards) line up on an even byte boundry.
    // note, we use the 15 bits of Brake status as a 16 bit pointer, but then VEH_WIDTH loaded in as 32

    BSM_p->brakes.word = ntohs(*(uint16_t *)p32);
    tmp = ntohl(*(uint32_t *)p32++);

    assert(BSM_FAST32_271_VEH_WIDTH_MASK == 0x0001ff80);
    assert(BSM_FAST32_271_VEH_WIDTH_SHIFT == (1 + 16 - 10));

    BSM_p->VehicleWidth_cm = (tmp & BSM_FAST32_271_VEH_WIDTH_MASK) >> BSM_FAST32_271_VEH_WIDTH_SHIFT; // 10 bits


    /* if this BSM is not extended by optional fields, there might be only one byte
       remaining...so we won't load the next byte into a uint32 like we did previously
       We only have 5 bits more to complete the VehicleLength
    */

    assert(BSM_FAST32_281_VEH_LEN_SEG1_MASK == 0x0000007F);
    assert(BSM_FAST32_281_VEH_LEN_SEG2_MASK == 0xf8);
    assert(BSM_FAST32_281_VEH_LEN_SEG2_SHIFT == (8 - (12 - 7))); // Note -- calculated for an 8 bit word

    BSM_p->VehicleLength_cm =        (tmp & BSM_FAST32_281_VEH_LEN_SEG1_MASK) << BSM_FAST32_281_VEH_LEN_SEG2_LEN
    ; // 12 bits

    if ((uint8_t *)p32 <= last_byte_p) {
        BSM_p->VehicleLength_cm |= ((*(uint8_t *)p32) & BSM_FAST32_281_VEH_LEN_SEG2_MASK) >>
            BSM_FAST32_281_VEH_LEN_SEG2_SHIFT; // last 3 of 12 bits
    } else {
        printf("error -- premature end of BSM Core:  1 byte short \n");
    }

    bits_left = 3;

    // now deal with the Optional Part II content, if ASN UPER flags inidcated it was present

    // ID is tpyically a 4 character string in testing, could print string if ascii human readable

    if (gVerbosity > 7) {
        printf("count=%d, ID=0x%04x secMark=%d Lat=%d Lon=%d ele=%d tranny=%d speed=%d, heading=%d"
            " steer=%d accel {%d,%d,%d,%d} width/len=%d/%d cm\n",
            BSM_p->MsgCount, BSM_p->id, BSM_p->secMark_ms,
            BSM_p->Latitude, BSM_p->Longitude,
            BSM_p->Elevation,
            BSM_p->TransmissionState,
            BSM_p->Speed,
            BSM_p->Heading_degrees,
            BSM_p->SteeringWheelAngle,
            BSM_p->AccelLon_cm_per_sec_squared,
            BSM_p->AccelLat_cm_per_sec_squared,
            BSM_p->AccelVert_two_centi_gs,
            BSM_p->AccelYaw_centi_degrees_per_sec,
            BSM_p->VehicleWidth_cm,
            BSM_p->VehicleLength_cm
            );


        printf(
            "   bsm.brakes_applied.data=0x%02x\n"
            "   bsm.traction_control_status=%d\n"
            "   bsm.antilock_brake_status=%d\n"
            "   bsm.stability_control_status=%d\n"
            "   bsm.brake_boost_applied=%d\n"
            "   bsm.aux_brake_status=%d\n",
            BSM_p->brakes.word,
            BSM_p->brakes.bits.traction_control_status,
            BSM_p->brakes.bits.antilock_brake_status,
            BSM_p->brakes.bits.stability_control_status,
            BSM_p->brakes.bits.brake_boost_applied,
            BSM_p->brakes.bits.aux_brake_status
            );
    }

    p8 = (uint8_t *)p32; // byte pointer, instead of 32bit pointer, since this is the type some helper funcs  use

    if (BSM_p->has_partII) {
        int iterations;

        /*  OK  "save_bits_left  can be  a little difficult to undertand
         *  this variable is used to remember what the offset/fractional unused portion of bits
         *  within a byte are , where the Part II "open type" extension begins .We need this, because
         *  the sequence length is specified in the UPER encoding in bytes... so if the decode only uses a fraction
         *  of those bytes, we need to restart the outer object, exactly n bytes after where we were at the end of
         *  the length specification
         *  UPER is supposed to be un-padded, but in this case there could be 1-7 bits. due to IEC8825-2 sec 11.11.1
         *    which requires "ASN.1 open type" to be treated as an "outermost type" and apply the padding to
         *  round out the encoding to a modulo 8 number of bits, even though in our case this does not mean
         *  octet alignment
         */
        int save_bits_left;

        if (gVerbosity > 7) {
            printf("PART-II:");
            print_buffer(p8, last_byte_p - (uint8_t *)p8 + 1); // full len remain is :last_byte_0-p32
        }

        // 3 bit SEQUENCE-OF  PARTII-EXT-ID-AND-TYPE
        // bp->qty_partII_extensions = ((*p8) & 0x07)+1 ;
        // If there are fewer bytes remaining than we need... then lets jump to error
        if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(3, bits_left)) {
            goto BSM_too_short;
        }

        if (gVerbosity > 7) {
            printf("\n  [bits_left=%d, *p8=0x%02x] ", bits_left, *p8);
        }


        BSM_p->qty_partII_extensions = get_next_n_bits(&p8, 3, &bits_left) + 1;


        if (p8 >= last_byte_p) {
            goto bsm_decode_err;
        }

        if (gVerbosity > 2) {
            printf(" %d items ", BSM_p->qty_partII_extensions);
        }

        for (iterations = 0; iterations < BSM_p->qty_partII_extensions; iterations++) {

            if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(PART_II_ID_LEN_BITS, bits_left)) {
                goto BSM_too_short;
            }

            if (gVerbosity > 7) {
                printf("\n  [bits_left=%d, *p8=0x%02x] ", bits_left, *p8);
            }

            tmp = get_next_n_bits(&p8, PART_II_ID_LEN_BITS, &bits_left);

            if (gVerbosity > 7) {
                printf("\n#%d extension PartII- ID=%d\n", iterations, tmp);
            }

            // So exactly which Part-II Extension is this?
            switch (tmp) {

            case PartII_Id_vehicleSafetyExt:
                {
                    BSM_p->has_safety_extension = 1;
                    uint32_t fpv_options;
                    uint32_t options;

                    if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(8, bits_left)) {
                        goto BSM_too_short;
                    }

                    // Sequence length is the # of bytes in the "SEQUENCE " since an "open type"
                    sequence_len = parse_asn_variable_length_enc(&p8, &bits_left);

                    if (gVerbosity > 2) {
                        printf("PartII_Id_vehicleSafetyExt:  len=%d bytes (%ld remain) ", sequence_len,
                            last_byte_p - p8);
                    }

                    // treat as an outermost, 0 padding at end to make even "sequence_len"*8 bits
                    save_bits_left = bits_left;

                    // Check that the number of reamining bytes is at least long enough
                    // to contain the declared sequence length
                    if ((last_byte_p - p8) < (sequence_len - 1)) { // KABOB
                        goto BSM_too_short;
                    }

                    is_extended = get_next_n_bits(&p8, 1, &bits_left);

                    options = get_next_n_bits(&p8, PART_II_SAFETY_EXT_OPTION_QTY, &bits_left);
                    BSM_p->vehsafeopts = options;

                    if (options & PART_II_SAFETY_EXT_OPTION_EVENTS) {
                        is_extended = get_next_n_bits(&p8, 1, &bits_left);

                        BSM_p->events.data = get_next_n_bits(&p8, PART_II_SAFETY_EXT_EVENTS_LEN_BITS, &bits_left);

                        if (gVerbosity > 3) {
                            printf("events flags=0x%0x ", BSM_p->events.data);
                        }
                    }

                    if (options & PART_II_SAFETY_EXT_OPTION_PATH_HISTORY) {
                        int m;
                        int ph_opts;
                        is_extended = get_next_n_bits(&p8, 1, &bits_left);
                        ph_opts = get_next_n_bits(&p8, PATH_HISTORY_OPTIONS_QTY, &bits_left);
                        BSM_p->phopts = ph_opts;
                        if (gVerbosity > 7) {
                            printf("\n PATH HISTORY ext=%d: ph_opts=%0x\n", is_extended, ph_opts);
                        }

                        // OK, so options bits say we have an inital position, then we get a FullPositionVector object
                        if (ph_opts & PATH_HISTORY_OPTION_INITALPOSITION) {
                            is_extended = get_next_n_bits(&p8, 1, &bits_left);
                            fpv_options = get_next_n_bits(&p8, FULLPOSITIONVECTOR_OPTIONS_QTY, &bits_left);

                            if (gVerbosity > 7) {
                                printf(" PH has initial position, ext=%d: options=%0x\n", is_extended, fpv_options);
                            }

                            BSM_p->ph.initialPosition.opts.byte = fpv_options;

                            if (fpv_options & FULLPOSITIONVECTOR_OPTION_UTCTIME) {
                                // Have to now populate the DDateTime_t; // J2735 DDateTime element -- non extendable

                                BSM_p->ph.initialPosition.utcTime.opts.byte =\
                                    get_next_n_bits(&p8, DDATETIME_OPTIONS_QTY, &bits_left);

                                // 7 optional members, check and load each

                                if (BSM_p->ph.initialPosition.utcTime.opts.bits.has_year) {
                                    BSM_p->ph.initialPosition.utcTime.year =\
                                        get_next_n_bits(&p8, DDATETIME_DYEAR_LEN_BITS, &bits_left);
                                }

                                if (BSM_p->ph.initialPosition.utcTime.opts.bits.has_month) {
                                    BSM_p->ph.initialPosition.utcTime.month =\
                                        get_next_n_bits(&p8, DDATETIME_DMONTH_LEN_BITS, &bits_left);
                                }

                                if (BSM_p->ph.initialPosition.utcTime.opts.bits.has_day) {
                                    BSM_p->ph.initialPosition.utcTime.day =\
                                        get_next_n_bits(&p8, DDATETIME_DDAY_LEN_BITS, &bits_left);
                                }

                                if (BSM_p->ph.initialPosition.utcTime.opts.bits.has_hour) {
                                    BSM_p->ph.initialPosition.utcTime.hour =\
                                        get_next_n_bits(&p8, DDATETIME_DHOUR_LEN_BITS, &bits_left);
                                }

                                if (BSM_p->ph.initialPosition.utcTime.opts.bits.has_minute) {
                                    BSM_p->ph.initialPosition.utcTime.minute =\
                                        get_next_n_bits(&p8, DDATETIME_DMINUTE_LEN_BITS, &bits_left);
                                }

                                // Actually milliseconds, but J2735 calls it "DSecond"
                                if (BSM_p->ph.initialPosition.utcTime.opts.bits.has_second) {
                                    BSM_p->ph.initialPosition.utcTime.second =\
                                        get_next_n_bits(&p8, DDATETIME_DSECOND_LEN_BITS, &bits_left);
                                }

                                // Timezone offset is encoded with a -840 adjustment
                                if (BSM_p->ph.initialPosition.utcTime.opts.bits.has_offset) {
                                    int val = J2735_DOFFSET_MIN +\
                                        get_next_n_bits(&p8, DDATETIME_DOFFSET_LEN_BITS, &bits_left);

                                    if (val <= J2735_DOFFSET_MAX) {
                                        BSM_p->ph.initialPosition.utcTime.offset = val;
                                    } else {
                                        if (gVerbosity > 4) {
                                            printf("WARN: PH initial position has bad timezone offset=%d\n", val);
                                        }
                                        BSM_p->ph.initialPosition.utcTime.offset = J2735_DOFFSET_MAX;

                                    }
                                }


                            }

                            // Lat & Long are mandatory elemnts of the inital position
                            // Intersting note: different from Core of BSM, here longitude comes first.
                            BSM_p->ph.initialPosition.lon = BSM_ASN_LONGITUDE_ENCODE_OFFSET +\
                                get_next_n_bits(&p8, LONGITUDE_LEN_BITS, &bits_left);

                            BSM_p->ph.initialPosition.lat = BSM_ASN_LATITUDE_ENCODE_OFFSET +\
                                get_next_n_bits(&p8, LATITUDE_LEN_BITS, &bits_left);

                            if (fpv_options & FULLPOSITIONVECTOR_OPTION_ELEVATION) {
                                BSM_p->ph.initialPosition.elevation = BSM_ASN_ELEVATION_ENCODE_OFFSET +\
                                    get_next_n_bits(&p8, ELEVATION_LEN_BITS, &bits_left);
                            }

                            if (fpv_options & FULLPOSITIONVECTOR_OPTION_HEADING) {
                                BSM_p->ph.initialPosition.heading =\
                                    get_next_n_bits(&p8, HEADING_LEN_BITS, &bits_left);
                            }

                            if (fpv_options & FULLPOSITIONVECTOR_OPTION_SPEED) {
                                BSM_p->ph.initialPosition.speed = get_next_n_bits(&p8, SPEED_LEN_BITS, &bits_left);
                            }

                            if (fpv_options & FULLPOSITIONVECTOR_OPTION_POS_ACCURACY) {
                                BSM_p->ph.initialPosition.pos_accuracy.semi_major =\
                                    get_next_n_bits(&p8, SEMIMAJOR_ACCURACY_LEN_BITS, &bits_left);
                                BSM_p->ph.initialPosition.pos_accuracy.semi_minor =\
                                    get_next_n_bits(&p8, SEMIMINOR_ACCURACY_LEN_BITS, &bits_left);
                                BSM_p->ph.initialPosition.pos_accuracy.orientation =\
                                    get_next_n_bits(&p8, SEMIMAJOR_ORIENTATION_LEN_BITS, &bits_left);
                            }

                            if (fpv_options & FULLPOSITIONVECTOR_OPTION_TIME_CONFIDENCE) {
                                BSM_p->ph.initialPosition.time_confidence =\
                                    get_next_n_bits(&p8, TIME_CONFIDENCE_LEN_BITS, &bits_left);
                            }

                            if (fpv_options & FULLPOSITIONVECTOR_OPTION_POS_CONFIDENCE) {
                                BSM_p->ph.initialPosition.pos_confidence.xy =\
                                    get_next_n_bits(&p8, POSITION_CONFIDENCE_LEN_BITS, &bits_left);
                                BSM_p->ph.initialPosition.pos_confidence.elevation =\
                                    get_next_n_bits(&p8, ELEVATION_CONFIDENCE_LEN_BITS, &bits_left);
                            }

                            if (fpv_options & FULLPOSITIONVECTOR_OPTION_SPEED_CONFIDENCE) {
                                BSM_p->ph.initialPosition.motion_confidence_set.heading_confidence =\
                                    get_next_n_bits(&p8, HEADING_CONFIDENCE_LEN_BITS, &bits_left);

                                BSM_p->ph.initialPosition.motion_confidence_set.speed_confidence =\
                                    get_next_n_bits(&p8, SPEED_CONFIDENCE_LEN_BITS, &bits_left);

                                BSM_p->ph.initialPosition.motion_confidence_set.throttle_confidence =\
                                    get_next_n_bits(&p8, THROTTLE_CONFIDENCE_LEN_BITS, &bits_left);
                            }


                        }

                        if (ph_opts & PATH_HISTORY_OPTION_GNSS_STATUS) {
                            BSM_p->ph.gnss_status.data =\
                                get_next_n_bits(&p8, GNSS_STATUS_LEN_BITS, &bits_left);

                        }

                        // Crumb data is not optional... must be at least 1, re-use sequence_len var.i
                        // earlier value of sequence_len is no longer needed
                        sequence_len = get_next_n_bits(&p8, PATH_HISTORY_SEQUENCE_SIZE_LEN_BITS, &bits_left) + 1;
                        // 1 crumb point is encoded as 0 by ASN rules, but somehow its not that way
                        // sequence_len ++;

                        if (gVerbosity > 7) {
                            printf("PATH HISTORY crumb count: %d\n", sequence_len);
                        }
                        BSM_p->ph.qty_crumbs = sequence_len;
                        if (sequence_len > MAX_PATH_HISTORY_POINTS_QTY) {
                            if (gVerbosity > 3) {
                                printf(" PATH HISTORY crumb count too high\n");
                            }
                            goto bsm_decode_err;
                        }

                        for (m = 0; m < sequence_len; m++) {
                            uint32_t ph_crumb_options;
                            // Now PathHistoryPoint offset sequence

                            is_extended = get_next_n_bits(&p8, 1, &bits_left);

                            /*  The list of points a SEQUENCE of:
                                latOffset
                                lonLoffset
                                elevationOffset
                                timedOffset
                                speed               // OPTIONAL
                                posAccuracy             // OPTIONAL
                                heading             // OPTIONAL
                                 3 optional history points
                            */
                            ph_crumb_options = get_next_n_bits(&p8, PATH_HISTORY_POINT_OPTIONS_QTY, &bits_left);
                            BSM_p->ph.ph_crumb[m].opts_u.byte = ph_crumb_options;

                            BSM_p->ph.ph_crumb[m].latOffset =\
                                get_next_n_bits(&p8, LAT_OFFSET_LEN_BITS, &bits_left) + LAT_OFFSET_MIN_VALUE;

                            BSM_p->ph.ph_crumb[m].lonOffset =\
                                get_next_n_bits(&p8, LON_OFFSET_LEN_BITS, &bits_left) + LON_OFFSET_MIN_VALUE;

                            BSM_p->ph.ph_crumb[m].eleOffset =\
                                get_next_n_bits(&p8, ELEVATION_OFFSET_LEN_BITS, &bits_left) + ELE_OFFSET_MIN_VALUE;

                            BSM_p->ph.ph_crumb[m].timeOffset_ms =\
                                10 * (get_next_n_bits(&p8, TIME_OFFSET_LEN_BITS, &bits_left) + TIME_OFFSET_MIN_VALUE);

                            if (gVerbosity > 7) {
                                printf("\n  #%2d: millisecond t=%-7d (%d,%d,%d) ", m,
                                    BSM_p->ph.ph_crumb[m].timeOffset_ms,
                                    BSM_p->ph.ph_crumb[m].latOffset,
                                    BSM_p->ph.ph_crumb[m].lonOffset,
                                    BSM_p->ph.ph_crumb[m].eleOffset
                                    );

                            }

                            if (ph_crumb_options & PATH_HISTORY_POINT_OPTION_SPEED) {
                                BSM_p->ph.ph_crumb[m].speed =\
                                    get_next_n_bits(&p8, PATH_CRUMB_SPEED_LEN_BITS, &bits_left);

                                if (gVerbosity > 7)
                                    printf("v=%d ",
                                        BSM_p->ph.ph_crumb[m].speed);
                            }


                            if (ph_crumb_options & PATH_HISTORY_POINT_OPTION_ACCURACY) {
                                BSM_p->ph.ph_crumb[m].accy.semi_major =\
                                    get_next_n_bits(&p8, SEMIMAJOR_ACCURACY_LEN_BITS, &bits_left);

                                BSM_p->ph.ph_crumb[m].accy.semi_minor =\
                                    get_next_n_bits(&p8, SEMIMAJOR_ACCURACY_LEN_BITS, &bits_left);

                                BSM_p->ph.ph_crumb[m].accy.orientation =\
                                    get_next_n_bits(&p8, SEMIMAJOR_ORIENTATION_LEN_BITS, &bits_left);
                            }

                            BSM_p->ph.ph_crumb[m].heading_available = V2X_False;

                            if (ph_crumb_options & PATH_HISTORY_POINT_OPTION_HEADING) {
                                int tmp_val;
                                tmp_val = get_next_n_bits(&p8, COARSE_HEADING_LEN_BITS, &bits_left);

                                if (tmp_val < COARSE_HEADING_UNAVAILABLE) {
                                    BSM_p->ph.ph_crumb[m].heading_available = V2X_True;
                                    // each LSB is 1.5 Degrees, so this multiply converts to micro-degrees
                                    BSM_p->ph.ph_crumb[m].heading_microdegrees =\
                                        tmp_val * MICRO_DEGREES_PER_COARSE_HEADING_LSB;
                                    if (gVerbosity > 7)
                                        printf("head=%f deg  ",
                                            BSM_p->ph.ph_crumb[m].heading_microdegrees / 1000.0);
                                }
                            }


                        }

#if 0
                        if ( bits_left != 0 ) {
                            if ((bits_left > save_bits_left2)) {
                                bits_left = save_bits_left2;
                            } else {
                                p8++;
                                bits_left =  save_bits_left2;
                            }
                        }
#endif


                    }

                    if (options & PART_II_SAFETY_EXT_OPTION_PATH_PREDICTION) {
                        is_extended = get_next_n_bits(&p8, 1, &bits_left);

                        BSM_p->pp.radius =\
                            get_next_n_bits(&p8, PATH_PREDICTION_RADIUS_LEN_BITS, &bits_left);
                        BSM_p->pp.is_straight = (BSM_p->pp.radius == PATH_RADIUS_STRAIGHT) ? V2X_True : V2X_False;

                        BSM_p->pp.confidence =\
                            get_next_n_bits(&p8, PATH_PREDICTION_CONFIDENCE_LEN_BITS, &bits_left);

                        if (gVerbosity > 7)
                            printf("\n  PATH_PREDICTION ext=%d radius=%d (encoded:0x%04x) confidence=%2.1f (encoded:0x%0x) p8: %02x, bits_left =%d\n ",
                                is_extended,
                                BSM_p->pp.radius + PATH_RADIUS_MIN_OFFSET,
                                BSM_p->pp.radius,
                                (double)BSM_p->pp.confidence / 2.0,
                                BSM_p->pp.confidence,
                                *p8,
                                bits_left
                                );

                        BSM_p->pp.radius += PATH_RADIUS_MIN_OFFSET;

                    }

                    if (options & PART_II_SAFETY_EXT_OPTION_LIGHTS) {
                        is_extended = get_next_n_bits(&p8, 1, &bits_left);

                        BSM_p->lights_in_use.data =\
                            get_next_n_bits(&p8, LIGHTS_IN_USE_LEN_BITS, &bits_left);

                        if (gVerbosity > 7)
                            printf("\n  Lights extension included  ext=%d lights=%02x \n",
                                is_extended, BSM_p->lights_in_use.data);
                    }

                }

                break;


            case PartII_Id_specialVehicleExt :
                {
                    BSM_p->has_special_extension = 1;
                    // Sequence length is the # of bytes in the "SEQUENCE " since an "open type"
                    uint32_t specvehopts;
                    sequence_len = parse_asn_variable_length_enc(&p8, &bits_left);
                    save_bits_left = bits_left;

                    is_extended = get_next_n_bits(&p8, 1, &bits_left);
                    if (gVerbosity > 7) {
                        printf("PartII_Id_specialVehicleExt (emergency vehicle, events, hazmat, wideload, etc_) ext=%d len=%d ", is_extended, sequence_len);
                    }

                    // Variable length -- could be one, or very long //

                    if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(3, bits_left)) {
                        goto BSM_too_short;
                    }

                    specvehopts = get_next_n_bits(&p8, SPECIAL_VEH_EXT_OPTIONS_QTY, &bits_left);
                    BSM_p->specvehopts = specvehopts;

                    if (specvehopts & SPECIAL_VEH_EXT_OPTION_EMERGENCY_DETAILS) {
                        if (gVerbosity > 7) {
                            printf("\nEmergencyDetails are present\n");
                        }

                        is_extended = get_next_n_bits(&p8, 1, &bits_left);
                        uint32_t edopts = get_next_n_bits(&p8, SPECIAL_VEH_EMERGENCY_DATA_OPTIONS_QTY, &bits_left);
                        BSM_p->edopts = edopts;
                        BSM_p->vehicleAlerts.sspRights = get_next_n_bits(&p8, SPECIAL_VEH_SSP_LEN_BITS, &bits_left);

                        if (gVerbosity > 7) {
                            printf("[SSPindex=%d ] ", BSM_p->vehicleAlerts.sspRights);
                        }

                        BSM_p->vehicleAlerts.sirenUse = get_next_n_bits(&p8, SPECIAL_VEH_SIREN_LEN_BITS, &bits_left);

                        if (gVerbosity > 7) {
                            printf("[SirenUse=%d ] ", BSM_p->vehicleAlerts.sirenUse);
                        }

                        BSM_p->vehicleAlerts.lightsUse = get_next_n_bits(&p8, SPECIAL_VEH_LIGHTS_USE_LEN_BITS, &bits_left);

                        if (gVerbosity > 7) {
                            printf("[lightsUse=%d ] ", BSM_p->vehicleAlerts.lightsUse);
                        }

                        BSM_p->vehicleAlerts.multi = get_next_n_bits(&p8, SPECIAL_VEH_MULTI_LEN_BITS, &bits_left);

                        if (gVerbosity > 7) {
                            printf("[multi=%d ] ", BSM_p->vehicleAlerts.multi);
                        }

                        if (edopts & EMERGENCY_DATA_OPTION_PRIVILEGED_EVENT) {
                            if (gVerbosity > 7) {
                                printf("\nPrivilegedEvents are present\n");
                            }

                            is_extended = get_next_n_bits(&p8, 1, &bits_left);

                            BSM_p->vehicleAlerts.events.sspRights = get_next_n_bits(&p8, SPECIAL_VEH_SSP_LEN_BITS, &bits_left);

                            if (gVerbosity > 7) {
                                printf("[SSPindex=%d ] ", BSM_p->vehicleAlerts.events.sspRights);
                            }

                            BSM_p->vehicleAlerts.events.event = get_next_n_bits(&p8, SPECIAL_VEH_EVENT_LEN_BITS, &bits_left);

                            if (gVerbosity > 7) {
                                printf("[SSPindex=%d ] ", BSM_p->vehicleAlerts.events.event);
                            }

                        }

                        if (edopts & EMERGENCY_DATA_OPTION_RESPONSE_TYPE) {
                            if (gVerbosity > 7) {
                                printf("\nResponseType is present\n");
                            }

                            is_extended = get_next_n_bits(&p8, 1, &bits_left);

                            BSM_p->vehicleAlerts.responseType = get_next_n_bits(&p8, SPECIAL_VEH_REPONSE_TYPE_LEN_BITS, &bits_left);

                            if (gVerbosity > 7) {
                                printf("[responseType=%d ] ", BSM_p->vehicleAlerts.responseType);
                            }
                        }

                    }
                    if (specvehopts & SPECIAL_VEH_EXT_OPTION_EVENT_DESC) {

                        if (gVerbosity > 7)
                            printf("\nEventDescriptions are present, bits_left: %d, p8 : %02x %02x\n", bits_left, *p8, *(p8 + 1));

                        is_extended = get_next_n_bits(&p8, 1, &bits_left);

                        uint32_t eventopts;

                        eventopts = get_next_n_bits(&p8, SPECIAL_VEH_EVENT_OPTIONS_QTY, &bits_left);
                        BSM_p->eventopts = eventopts;
                        BSM_p->description.typeEvent = get_next_n_bits(&p8, SPECIAL_VEH_EVENT_LEN_BITS, &bits_left);

                        if (gVerbosity > 7)
                            printf("[description.typeEvent=%d ] ", BSM_p->description.typeEvent);

                        if (eventopts & SPECIAL_VEH_EVENT_OPTION_DESC) {
                            int m;
                            if (gVerbosity > 7)
                                printf("\nDescription is present\n");
                            sequence_len = get_next_n_bits(&p8, SPECIAL_VEH_EVENT_OPTION_DESC_COUNT_BITS, &bits_left) + 1;

                            BSM_p->description.size_desc = sequence_len;

                            for ( m = 0; m < BSM_p->description.size_desc; m++) {
                                BSM_p->description.desc[m] = get_next_n_bits(&p8, SPECIAL_VEH_EVENT_DESC_LEN_BITS, &bits_left);
                            }

                        }

                        if (eventopts & SPECIAL_VEH_EVENT_OPTION_PRIOIRTY) {
                            if (gVerbosity > 7)
                                printf("\nPriority is present\n");
                            BSM_p->description.priority = get_next_n_bits(&p8, SPECIAL_VEH_EVENT_PRIORITY_LEN_BITS, &bits_left);
                        }

                        if (eventopts & SPECIAL_VEH_EVENT_OPTION_HEADINGSLICE) {
                            if (gVerbosity > 7)
                                printf("\nHeadingSlice is present\n");
                            BSM_p->description.heading = get_next_n_bits(&p8, SPECIAL_VEH_EVENT_DESC_LEN_BITS, &bits_left);
                        }

                        if (eventopts & SPECIAL_VEH_EVENT_OPTION_EXTENT) {
                            if (gVerbosity > 7)
                                printf("\nextent is present\n");
                            BSM_p->description.extent = get_next_n_bits(&p8, SPECIAL_VEH_EVENT_EXTENT_LEN_BITS, &bits_left);
                        }

                        if (eventopts & SPECIAL_VEH_EVENT_OPTION_REGIONAL_EXT) {
                            if (gVerbosity > 7)
                                printf("\nRegional extension is present\n");
                        }

                    }


                    if (specvehopts & SPECIAL_VEH_EXT_OPTION_TRAILER_DATA) {
                        if (gVerbosity > 7)
                            printf("\nTrailerData are present\n");
                        is_extended = get_next_n_bits(&p8, 1, &bits_left);
                    }

                }

                break;


            case PartII_Id_supplementalVehicleExt: // Sequence length is the # of bytes in the "SEQUENCE " since an "open type"
                BSM_p->has_supplemental_extension = 1;
                sequence_len = parse_asn_variable_length_enc(&p8, &bits_left);
                save_bits_left = bits_left;

                if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(1 + SUPPLEMENT_VEH_EXT_OPTIONS_QTY, bits_left)) {
                    goto BSM_too_short;
                }

                is_extended = get_next_n_bits(&p8, 1, &bits_left);
                opts = get_next_n_bits(&p8, SUPPLEMENT_VEH_EXT_OPTIONS_QTY, &bits_left);
                BSM_p->suppvehopts = opts;
                if (gVerbosity > 7)
                    printf("PartII_Id_supplementalVehicleExt: (ext=%d len=%dB opts=0x%03x or %d)\n ",
                        is_extended, sequence_len, opts, opts);


                if (opts & SUPPLEMENT_VEH_EXT_OPTION_CLASSIFICATION) {
                    BSM_p->VehicleClass = get_next_n_bits(&p8, SUPPLEMENT_VEH_CLASS_LEN_BITS, &bits_left);
                    if (gVerbosity > 7)
                        printf("\nBasic vehicle class: %d\n", BSM_p->VehicleClass);
                }

                if (opts & SUPPLEMENT_VEH_EXT_OPTION_CLASS_DETAILS) {

                    if (gVerbosity > 1)
                        fprintf(stderr, "unhandled SupplementalVehicleExtension: Classification Details\n");
                    goto bsm_decode_err;

                }

                if (opts & SUPPLEMENT_VEH_EXT_OPTION_VEHICLE_DATA) {
                    uint32_t veh_data_options;

                    // remaining unparsed msg should have at least 1 bit for extension, plus bits for the options field
                    if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(1 + SUPPLEMENT_VEH_DATA_OPTIONS_QTY, bits_left)) {
                        goto BSM_too_short;
                    }


                    is_extended = get_next_n_bits(&p8, 1, &bits_left);

                    BSM_p->veh.supplemental_veh_data_options.word =
                        veh_data_options = get_next_n_bits(&p8, SUPPLEMENT_VEH_DATA_OPTIONS_QTY, &bits_left);

                    if (gVerbosity > 7)
                        printf("\n  VEHICLE_DATA(ext=%d opts=0x%03x) ,bits_left: %d, p8: %02x: ",
                            is_extended, veh_data_options, bits_left, *p8);

                    if (veh_data_options & SUPPLEMENT_VEH_DATA_OPTION_HEIGHT) {
                        uint32_t tmp;
                        if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(VEHICLE_DATA_HEIGHT_LEN_BITS, bits_left)) {
                            goto BSM_too_short;
                        }


                        tmp = get_next_n_bits(&p8, VEHICLE_DATA_HEIGHT_LEN_BITS, &bits_left);

                        // encoded in nits of 5 CM each LSB, accoring to ASN.1 spec
                        BSM_p->veh.height_cm = tmp * VEHICLE_DATA_HEIGHT_CM_PER_LSB;
                        if (gVerbosity > 7) {
                            printf("[VehicleHeight=%d cm] ", BSM_p->veh.height_cm);
                        }
                    }

                    if (veh_data_options & SUPPLEMENT_VEH_DATA_OPTION_BUMPERS) {
                        if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(2 *
                                VEHICLE_DATA_BUMPER_HEIGHT_LEN_BITS, bits_left)) {
                            goto BSM_too_short;
                        }

                        BSM_p->veh.front_bumper_height_cm =
                            get_next_n_bits(&p8, VEHICLE_DATA_BUMPER_HEIGHT_LEN_BITS, &bits_left);

                        BSM_p->veh.rear_bumper_height_cm =
                            get_next_n_bits(&p8, VEHICLE_DATA_BUMPER_HEIGHT_LEN_BITS, &bits_left);

                        if (gVerbosity > 7)
                            printf("[Bumper Heights=%d %d cm] ",
                                BSM_p->veh.front_bumper_height_cm,
                                BSM_p->veh.rear_bumper_height_cm);
                    }

                    if (veh_data_options & SUPPLEMENT_VEH_DATA_OPTION_MASS) {
                        uint8_t tmp;

                        if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(VEHICLE_DATA_MASS_LEN_BITS, bits_left)) {
                            goto BSM_too_short;
                        }

                        tmp = get_next_n_bits(&p8, VEHICLE_DATA_MASS_LEN_BITS, &bits_left);

                        BSM_p->veh.mass_kg = VehicleMassDecode(tmp);    // Special encoding table

                        if (gVerbosity > 7)
                            printf("VehData: encoded vehicle mass:%0d, decoded=%d kg\n",
                                tmp, BSM_p->veh.mass_kg);

                    }

                    if (veh_data_options & SUPPLEMENT_VEH_DATA_OPTION_TRAILER_WEIGHT) {
                        if ((last_byte_p - p8) < calc_bytes_needed_for_n_more_bits(
                                VEHICLE_DATA_TRAILER_WEIGHT_LEN_BITS, bits_left)) {
                            goto BSM_too_short;
                        }

                        BSM_p->veh.trailer_weight =
                            get_next_n_bits(&p8, VEHICLE_DATA_TRAILER_WEIGHT_LEN_BITS, &bits_left);
                    }

                    if (gVerbosity > 7) {
                        printf("VehData: ext=%d bytes opts=0x%0x [4 bits]", is_extended, veh_data_options);
                    }

                }

                if (opts & SUPPLEMENT_VEH_EXT_OPTION_WEATHER_REPORT) {
                    fprintf(stderr, "unhandled SupplelmentalVehicleExtension: WeatherReport\n");
                    goto bsm_decode_err;
                }

                if (opts & SUPPLEMENT_VEH_EXT_OPTION_WEATHER_PROBE) {
                    uint32_t weatheropts;
                    is_extended = get_next_n_bits(&p8, 1, &bits_left);
                    weatheropts = get_next_n_bits(&p8, SUPPLEMENT_WEATHER_OPTIONS_QTY, &bits_left);
                    BSM_p->weatheropts = weatheropts;
                    if (weatheropts & SUPPLEMENT_WEATHER_AIRTEMP) {
                        BSM_p->airTemp = get_next_n_bits(&p8, SUPPLEMENT_WEATHER_AIRTEMP_LEN_BITS, &bits_left);
                        if (gVerbosity > 7)
                            printf("\nBSM_p->airTemp: %d\n", BSM_p->airTemp);
                    }

                    if (weatheropts & SUPPLEMENT_WEATHER_AIRPRESSURE) {
                        BSM_p->airPressure = get_next_n_bits(&p8, SUPPLEMENT_WEATHER_AIRPRESSURE_LEN_BITS, &bits_left);
                        if (gVerbosity > 7)
                            printf("\nBSM_p->airPressure: %d\n", BSM_p->airPressure);
                    }

                    if (weatheropts & SUPPLEMENT_WEATHER_WIPERS) {
                        uint32_t wiperopts = get_next_n_bits(&p8, SUPPLEMENT_WEATHER_WIPEROPT_LEN_BITS, &bits_left);
                        BSM_p->wiperopts = wiperopts;
                        BSM_p->statusFront = get_next_n_bits(&p8, SUPPLEMENT_WEATHER_WIPER_STATUS_LEN_BITS, &bits_left);

                        if (gVerbosity > 7)
                            printf("\nBSM_p->statusFront: %d\n", BSM_p->statusFront);

                        BSM_p->rateFront = get_next_n_bits(&p8, SUPPLEMENT_WEATHER_WIPER_RATE_LEN_BITS, &bits_left);

                        if (gVerbosity > 7)
                            printf("\nBSM_p->rateFront: %d\n", BSM_p->rateFront);

                        if (wiperopts & SUPPLEMENT_WEATHER_WIPERS_REAR_STATUS) {
                            BSM_p->statusRear = get_next_n_bits(&p8, SUPPLEMENT_WEATHER_WIPER_STATUS_LEN_BITS, &bits_left);

                            if (gVerbosity > 7)
                                printf("\nBSM_p->statusRear: %d\n", BSM_p->statusRear);
                        }

                        if (wiperopts & SUPPLEMENT_WEATHER_WIPERS_REAR_RATE) {
                            BSM_p->rateRear = get_next_n_bits(&p8, SUPPLEMENT_WEATHER_WIPER_RATE_LEN_BITS, &bits_left);

                            if (gVerbosity > 7)
                                printf("\nBSM_p->rateRear: %d\n", BSM_p->rateRear);
                        }
                    }

                }

                if (opts & SUPPLEMENT_VEH_EXT_OPTION_OBSTACLE) {
                    fprintf(stderr, "unhandled SupplelmentalVehicleExtension: Obstacle on Road\n");
                    goto bsm_decode_err;
                }

                if (opts & SUPPLEMENT_VEH_EXT_OPTION_DISABLED_VEH) {
                    fprintf(stderr, "unhandled SupplelmentalVehicleExtension: Disabled Vehicle Report\n");
                    goto bsm_decode_err;
                }

                if (opts & SUPPLEMENT_VEH_EXT_OPTION_SPEED_PROFILE) {
                    fprintf(stderr, "unhandled SupplelmentalVehicleExtension: Speed Profile\n");
                    goto bsm_decode_err;
                }

                if (opts & SUPPLEMENT_VEH_EXT_OPTION_RTCM) {
                    fprintf(stderr, "unhandled SupplelmentalVehicleExtension: RTCM Corrections\n");
                    goto bsm_decode_err;
                }

                if (opts & SUPPLEMENT_VEH_EXT_OPTION_REGIONAL_EXT) {
                    fprintf(stderr, "unhandled regional SupplelmentalVehicleExtension.\n");
                    goto bsm_decode_err;
                }


                break;

            default:
                fprintf(stderr, "unhandled extension ID #%d.\n", tmp);
                goto bsm_decode_err;
            }

            /*
             *  We have to leave the UPER stream on an even number of bytes. (Sequence_length)
             *  which is why we saved the "save_bits_left" where we started
             */

            if (bits_left != 0) {
                if ((bits_left >= save_bits_left)) {
                    bits_left = save_bits_left;
                } else {
                    p8++;
                    bits_left =  save_bits_left;
                }
            }
            /* Note -- at this point we are not necessarily octet-aligned.  The above lines just round
                out the qty of bits in the open type sequnce to be n*8
            */

        }
    }else{ 
        // zero out the data if no events, just in case so previous existing data is cleared
        BSM_p->events.data = 0;
    }

    if (BSM_p->has_regional_extensions && ((last_byte_p - (uint8_t *)p8) > 0)) {
        printf("has local extensions? -- not presently supported in decode\n");
    }

    if (gVerbosity > 2) {
        int bytes_left = last_byte_p - p8;
        if (bytes_left > 0) {
            printf("\nBSMDecode complete, %ld bytes unparsed, bits_left=%d\n",
                (last_byte_p - p8), bits_left);
            // Possibly print-out the remnant

            printf("REMNANT:\n");
            print_buffer(p8, last_byte_p - (uint8_t *)p8 + 1); // full len remain is :last_byte_0-p32
        } else {
            printf("\n      BSMDecode complete, %d bits padding \n",
                bits_left);
        }

    }

    goto BSM_decode_return;

unsupported:
    if (gVerbosity > 1) {
        printf(" unsupported extended BSM\n");
    }
    goto BSM_decode_return;

BSM_too_short:
    if (gVerbosity > 1) {
        printf("BSM ended earlier than expected\n");
    }

bsm_decode_err:
    if (gVerbosity) {
        printf("BSM decode err\n");
        if ((last_byte_p - p8)) {
            printf("        ERR-REMNANT bits_left=%d:", bits_left);
            print_buffer(p8, last_byte_p - (uint8_t *)p8 + 1); // full len remain is :last_byte_0-p32
        }
    }
    // Perhaps you want to exit on an unparsable error, should make a command line param
    // exit (-3);
    result = -1;

BSM_decode_return:

    return (result);
}
#if 0
/* Parses the data pointed to at db->payload (perhaps a WSMP patload)
    as a J2735 message , fisrt 2 bytes as message ID, then variable length
    configures the three "j2735" items in the db structure (id, len, and the data ptr)
*/

static void decode_general_j2735_msg_id_and_len(pktbuf_t *db)
{
    uint8_t *ptr = db->payload;
    int bits_left = 8; // everything starts Octet aligned.

    db->j2735_msg_id = ntohs(*(uint16_t *)ptr);
    ptr += 2;

    db->j2735_msg_len =\
        parse_asn_variable_length_enc(&ptr, &bits_left); // always octet aligned.

    db->j2735_msg = ptr;

}

#endif
// decode as a J2735 message, whch always becins as a message ID, which tells us
// if it is a UPER or DER type message.  (so in other words, encodting might vary!)
// returns a 0 if there are problems
int  decode_as_j2735(msg_contents *mc)
{
    uint16_t msg_id = 0;
    int  msg_len; // J2735 message content length, after msg ID & length

    // at some point, tail is not properly synchronized with data
    if (mc->abuf.data && ((mc->abuf.tail - mc->abuf.data) > 4)) {
        int bits_left = 8;  // everything starts Octet aligned.
        uint8_t* msg = (uint8_t*)mc->l3_payload;
        if (gVerbosity > 4){
              printf("<<<J2735 uint16_t l3_payload msgid=%02x%02x>>>\n", *(msg),
                *(msg+1));
            printf("<<<J2735 uint16_t abuf msgid=%02x%02x>>>\n", (mc->abuf.data[0]),
                    (mc->abuf.data[1]));
            printf("Information about the abuf struct\n");
            abuf_dump(&mc->abuf);
            printf("\n");
            printf("BSM encoded buffer dump\n");
            print_buffer(mc->abuf.data, mc->payload_len);
            printf("\n");
        }

        uint16_t* ptr  = (uint16_t*)abuf_pull(&mc->abuf, sizeof(uint16_t));
        msg_id = ntohs(*ptr);
        mc->l3_payload = mc->l3_payload + sizeof(uint16_t);
        mc->j2735_msg_id = msg_id;
        mc->msgId = msg_id;

        //   From  ASN.1 sepc "DSRCmsgID ::= INTEGER (0..32767)"
        switch (msg_id) {

        case J2735_MSGID_RESERVED_DER:
        case J2735_MSGID_ALA_CARTE_DER:
        case J2735_MSGID_BASIC_SAFETY_MESSAGE_DER:
        case J2735_MSGID_BASIC_SAFETY_MESSAGE_VERBOSE_DER:
        case J2735_MSGID_COMMON_SAFETY_REQUEST_DER:
        case J2735_MSGID_EMERGENCY_VEHICLE_ALERT_DER:
        case J2735_MSGID_INTERSECTION_COLLISION_DER:
        case J2735_MSGID_MAP_DATA_DER:
        case J2735_MSGID_NMEA_CORRECTIONS_DER:
        case J2735_MSGID_PROBE_DATA_MANAGEMENT_DER:
        case J2735_MSGID_PROBE_VEHICLE_DATA_DER:
        case J2735_MSGID_ROAD_SIDE_ALERT_DER:
        case J2735_MSGID_RTCM_CORRECTIONS_DER:
        case J2735_MSGID_SIGNAL_PHASE_AND_TIMING_DER:
        case J2735_MSGID_SIGNAL_REQUEST_DER:
        case J2735_MSGID_SIGNAL_STATUS_DER:
        case J2735_MSGID_TRAVELER_INFORMATION_DER:
        case J2735_MSGID_UPER_FRAME_DER:
            printf("older retired Message: %d\n", msg_id);
            break;
            // --
            // -- UPER forms are all the remaining cases
            // --
        case J2735_MSGID_MAP_DATA:
        case J2735_MSGID_SIGNAl_PHASE_AND_TIMING :
        case J2735_MSGID_COMMON_SAFETY_REQUEST    :     // -- CSR
        case J2735_MSGID_EMERGENCY_VEHICLE_ALERT  :     // -- EVA
        case J2735_MSGID_INTERSECTION_COLLISION   :     // -- ICA
        case J2735_MSGID_NMEA_CORRECTIONS         :     // -- NMEA
        case J2735_MSGID_PROBE_DATA_MANAGEMENT    :     // -- PDM
        case J2735_MSGID_PROBE_VEHICLE_DATA       :     // -- PVD
        case J2735_MSGID_ROAD_SIDE_ALERT          :     // -- RSA
        case J2735_MSGID_RTCM_CORRECTIONS         :     // -- RTCM
        case J2735_MSGID_SIGNAL_REQUEST           :     // -- SRM
        case J2735_MSGID_SIGNAL_STATUS            :     // -- SSM
        case J2735_MSGID_TRAVELER_INFORMATION     :     // -- TIM
        case J2735_MSGID_PERSONAL_SAFETY          :     // -- PSM
            printf("Msg : %d is not supported\n", msg_id);
            break;
            //-- The Below values are reserved for local message testing use
            // --

        case J2735_MSGID_TEST_00:
        case J2735_MSGID_TEST_01:
        case J2735_MSGID_TEST_02:
        case J2735_MSGID_TEST_03:
        case J2735_MSGID_TEST_04:
        case J2735_MSGID_TEST_05:
        case J2735_MSGID_TEST_06:
        case J2735_MSGID_TEST_07:
        case J2735_MSGID_TEST_08:
        case J2735_MSGID_TEST_09:
        case J2735_MSGID_TEST_10:
        case J2735_MSGID_TEST_11:
        case J2735_MSGID_TEST_12:
        case J2735_MSGID_TEST_13:
        case J2735_MSGID_TEST_14:
        case J2735_MSGID_TEST_15:
            printf("No code to parse J2735 Test Message ID #%d\n", msg_id);
            break;
        case J2735_MSGID_BASIC_SAFETY:             //0x14 = 20, // -- BSM, heartbeat msg
            msg_len =
                 parse_asn_variable_length_enc(
                            (unsigned char **)&mc->abuf.data, &bits_left);
            msg_len =
                 parse_asn_variable_length_enc(
                            (unsigned char **)&mc->l3_payload, &bits_left);
            mc->payload_len = msg_len;
            decode_as_bsm(mc);

            break;
        default:
            if(gVerbosity>7)
                printf("unrecognized J2735 payload msgID=%d\n", msg_id);
            break;
        }
    }

decode_err:
    return (msg_id);
}

int encode_as_j2735(msg_contents *mc)
{
    if (!mc || !mc->abuf.data) {
        fprintf(stderr, "%s: invalid input\n", __func__);
        return -1;
    }

    if (mc->j2735_msg_id == J2735_MSGID_BASIC_SAFETY) {
        bsm_value_t *BSM_p = mc->j2735_msg;
        encode_as_bsm(&mc->abuf, BSM_p);
    }

    return 0;
}
