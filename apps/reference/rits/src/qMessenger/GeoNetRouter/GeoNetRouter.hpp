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
 * @file GeoRouter.hpp
 * @brief constants and data structure definition for external include
 */
#ifndef __GEOROUTER_HPP__
#define __GEOROUTER_HPP__
#include <cstdint>

#define GN_MID_LEN  6
namespace gn{
/**
 * GeoNetwork upper protocol type.
 */
enum class UpperProtocol {
    GN_UPPER_PROTO_ANY = 0,
    GN_UPPER_PROTO_BTP_A,
    GN_UPPER_PROTO_BTP_B,
    GN_UPPER_PROTO_GN6,
};

/**
 * GeoNetwork packet type
 */
enum class PacketType {
    GN_PACKET_TYPE_ANY = 0,
    GN_PACKET_TYPE_BEACON = 1,
    GN_PACKET_TYPE_GEOUNICAST = 2,
    GN_PACKET_TYPE_GEOANYCAST = 3,
    GN_PACKET_TYPE_GEOBROADCAST = 4,
    GN_PCCKET_TYPE_TSB = 5,
    GN_PACKET_TYPE_SHB = 5,             /*!< Header type is TSB, header subtype is SHB */
    /* bellow types are used internally only */
    GN_PACKET_TYPE_LS = 6,
};
/* ETSI EN 302 931 Geographical Area Definition */
enum class GeoAreaType
{
    GEO_AREA_TYPE_CIRCLE = 0,
    GEO_AREA_TYPE_RECT,
    GEO_AREA_TYPE_ELIP
};
/*
 * Geo Position Lat/long unit
 */
enum class GeoPosUnit
{
    GEO_POS_UNIT_DEGREE = 1,
    GEO_POS_UNIT_MICRO_DEGREE = 1000000,
    GEO_POS_UNIT_TENTH_MICRO_DEGREE = 10000000,
};


/*! \struct
 *  \brief Geographical area data structure.
 */
typedef struct GeoArea {
    int32_t gp_latitude;
    int32_t gp_longitude;
    uint16_t dist_a;
    uint16_t dist_b;
    uint16_t angle;
    GeoAreaType area_type;
} GeoArea_t;

/*! \struct
 *  \brief ego position vector.
 */
typedef struct gn_epv {
    uint32_t tst_epv;       /*!< time in milliseconds at which
                              the latitude and longitude of the ITS-S were
                              acquired by the GeoAdhoc router. */

    int32_t latitude_epv;   /*!< Latitude of the GeoAdhoc router reference
                              position expressed in 1/10 degree */

    int32_t longitude_epv;  /*!< Longitude of the GeoAdhoc router reference
                              position expressed in 1/10 degree */

    int16_t s_epv;          /*!< Speed of the GeoAdhoc router expressed in
                              signed uinits of 0.01 meter/second */
    uint16_t h_epv;         /*!< Heading of the GeoAdhoc router, expressed in
                              unsigned units of 0.1 degree from North */
    int pai_epv;            /*!< position accuracy indicator */
} gn_epv_t;

/**
 * GeoNetwork destination type.
 */
enum class DestinationType {
    DE_TYPE_GEO_AREA = 0,
    DE_TYPE_GN_ADDR
};
#if 0
union gn_de_param {
    geo_area_t target_area; /*!< targeted Geographical area for GBC/GAC */
    gn_addr_t de_addr;      /*!< destination address for GUC */
};
/*! \var typedef
 *  \brief type definition for gn_de_param union.
 */
typedef union gn_de_param gn_de_param_u;
#endif
/*! \enum
 *  \brief Local GN address configuration method enum
 */
enum class LocalAddrConfigMethod
{
    GN_LOCAL_ADDR_CONF_AUTO = 0,
    GN_LOCAL_ADDR_CONF_MANAGED,
    GN_LOCAL_ADDR_CONF_ANONYMOUS
};


/**
 * GN ITS station type enum
 */
enum class ITSStationType
{
    StationType_unknown = 0,
    StationType_pedestrian  = 1,
    StationType_cyclist = 2,
    StationType_moped   = 3,
    StationType_motorcycle  = 4,
    StationType_passengerCar    = 5,
    StationType_bus = 6,
    StationType_lightTruck  = 7,
    StationType_heavyTruck  = 8,
    StationType_trailer = 9,
    StationType_specialVehicles = 10,
    StationType_tram    = 11,
    StationType_roadSideUnit    = 15
};

/**
 * GN interface type enum
 */
enum class InterfaceType{
    GN_IF_TYPE_UNSPECIFIED = 0,
    GN_IF_TYPE_ITS_G5
};

/**
 * GN security feature enum.
 */
enum class SecurityConfig {
    GN_SECURITY_DISABLED = 0,
    GN_SECURITY_ENABLED
};
enum class SecurityProfile {
    SEC_PROFILE_UNSECURED = 0,
    SEC_PROFILE_SECURED
};

/**
 * Area Forwarding Algorithm
 */
enum class AF_Algorithm {
    GN_AF_UNSPECIFIED = 0,
    GN_AF_SIMPLE = 1,
    GN_AF_CBF = 2,
    GN_AF_AF = 3
};

/*
 * Non-Area Forwarding algorithm
 */
enum class NAF_Algorithm {
    GN_NAF_UNSPECIFIED = 0,
    GN_NAF_GREEDY = 1,
    GN_NAF_CBF = 2
};

/**
 *  GeoNetworking global config data.
 */
typedef struct GnConfig {
    //gn_addr_t itsGnLocalGnAddr;
    uint8_t mid[GN_MID_LEN];
    LocalAddrConfigMethod itsGnLocalAddrConfMethod;
    int itsGnProtocolVersion;
    ITSStationType StationType;
    bool itsGnIsMobile;
    InterfaceType itsGnIfType;
    int itsGnMinUpdateFrequencyEPV;
    int itsGnPaiInterval;
    int itsGnMaxStuSize;
    int itsGnMaxGeoNetworkingHeaderSize;
    int itsGnLifetimeLocTE;
    SecurityConfig itsGnSecurity;
    //gn_decap_result_e itsGnSnDecapResultHandling;
    int itsGnLocationServiceMaxRetrans;
    int itsGnLocationServiceRetransmitTimer;
    int itsGnLocationServicePacketBufferSize;
    int itsGnBeaconServiceRetransmitTimer;
    int itsGnBeaconServiceMaxJitter;
    int itsGnDefaultHopLimit;
    int itsGnDPLLength;
    int itsGnMaxPacketLifetime;
    int itsGnDefaultPacketLifetime;
    int itsGnMaxPacketDataRate;
    int itsGnMaxPacketDataRateEmaBeta;
    int itsGnMaxGeoAreaSize;
    int itsGnMinPacketRepetitionInterval;
    NAF_Algorithm itsGnNonAreaForwardingAlgorithm;
    AF_Algorithm  itsGnAreaForwardingAlgorithm;
    int itsGnCbfMinTime;
    int itsGnCbfMaxTime;
    int itsGnDefaultMaxCommunicationRange;
    int itsGnBroadcastCBFDefSectorAngle;
    int itsGnUcForwardingPacketBufferSize;
    int itsGnBcForwardingPacketBufferSize;
    int itsGnCbfPacketBufferSize;
    int itsGnDefaultTrafficClass;
}GnConfig_t;


/**
 *  Parameters passed in when invoking GN data service, or returned when receiving data
 *  from GN data service.
 */
typedef struct GnData {
    UpperProtocol upper_prot;   /*!< upper protocol entity */
    PacketType pkt_type;        /*!< packet type, e.g. GUC, SHB, TSB, GBC, GAC */
    bool is_shb;
    int payload_len;
    DestinationType d_type;
    uint8_t d_addr[GN_MID_LEN];
    GeoArea_t d_area;
    int max_lt;                 /*!< maximum tolerable time in [s] a GeoNetworking packet
                                    can be buffered until it reaches its destination */

    int repetition_interval;    /*!< the duration between two consecutive
                                  transmissions of the same GeoNetworking
                                  packet during maximum repetition time of
                                  a packet in [ms]. */

    int max_repetition_time;    /*!< specifies the duration in [ms] for which
                                  the packet will be repeated if the Repetition
                                  interval is set */

    int max_hl;                 /*!< specifies the number of hops a packet is
                                  allowed to have in the network */

    int tc;                     /*!< Traffic class */
    int hst;                    /*!< Only for internal use */
} GnData_t;

} //namespace gn
#endif  //__GEOROUTER_HPP__
