/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * @file GeoNetUtils.hpp
 * @brief Some utilities of GeoNet
 */
#ifndef _GEONETUTILS_HPP_
#define _GEONETUTILS_HPP_
#include <cmath>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include "GeoNetRouter.hpp"
#include "gn_internal.h"

namespace gn{
    class GeoNetUtils {
    public:
        GeoNetUtils() {
        }
        ~GeoNetUtils() { }

    static bool GnAddrMatch(const gn_addr_t &Addr1, const gn_addr_t &Addr2) {
        if (Addr1.st != Addr2.st ||
                Addr1.mid[0] != Addr2.mid[0] ||
                Addr1.mid[1] != Addr2.mid[1] ||
                Addr1.mid[2] != Addr2.mid[2] ||
                Addr1.mid[3] != Addr2.mid[3] ||
                Addr1.mid[4] != Addr2.mid[4] ||
                Addr1.mid[5] != Addr2.mid[5]) {
            return false;
        }
        return true;
    }
    static bool GnMacMatch(const uint8_t *Addr1, const uint8_t *Addr2) {
        if (Addr1[0] != Addr2[0] ||
                Addr1[1] != Addr2[1] ||
                Addr1[2] != Addr2[2] ||
                Addr1[3] != Addr2[3] ||
                Addr1[4] != Addr2[4] ||
                Addr1[5] != Addr2[5]) {
            return false;
        }
        return true;
    }
    static int GeoDistance(double lat_a, double long_a, double lat_b, double long_b) {
        // Uses the haversine formula to calculate the great-circle distance betwen two points
        double R, a, c, d;
        R = 6371000.0; // Radius of the earth (in meters)
        a = pow((sin(lat_b - lat_a) / 2.0), 2) +
        (cos(lat_a) * cos(lat_b) * pow(sin(long_b - long_a) / 2.0, 2));
        c = 2 * atan2(sqrt(a), sqrt(1.0 - a));
        d = R * c;
        return static_cast<int>(d);
    }

    static int GeoDistance(int32_t lat_a, int32_t long_a,
                    int32_t lat_b, int32_t long_b, geo_pos_unit_e unit) {
        return GeoDistance(static_cast<double>(lat_a * M_PI *unit/180.0),
                static_cast<double>(long_a * M_PI * unit /180.0),
                static_cast<double>(lat_b * M_PI * unit/180.0),
                static_cast<double>(long_b * M_PI * unit/180.0));
    }

    static int GeoBearing(int32_t lat_a, int32_t long_a,
                    int32_t lat_b, int32_t long_b, geo_pos_unit_e unit) {

        double phi1 = (lat_a * M_PI/180.0)/unit;
        double phi2 = phi2 = (lat_b * M_PI/180.0)/unit;
        double delta_phi = ((lat_b - lat_a) * M_PI/180.0)/unit;
        double delta_lamda = ((long_b - long_a) * M_PI/180.0)/unit;

        double y = sin(delta_lamda) * cos(phi2);
        double x = cos(phi1)*sin(phi2) - sin(phi1)*cos(phi2)*cos(delta_lamda);

        return (atan2(y, x)*unit*180/M_PI);
    }

    static void CartesianTransform(int32_t lat_a, int32_t long_a, int32_t lat_c, int32_t long_c,
            int32_t angle_c, geo_pos_unit_e unit, int32_t &x, int32_t &y) {
        int32_t b, d;
        d = GeoDistance(lat_a, long_a, lat_c, long_c, unit);
        b = GeoBearing(lat_a, long_a, lat_c, long_c, unit);

        x = d * cos(((b - angle_c)*M_PI/180)/unit);
        y = d * sin(((b - angle_c)*M_PI/180)/unit);
    }

    /**
     * Give a point (x, y) in cartesian coordinate and a geo area, determine if
     * the point is inside, on the border or outside of the area.
     *
     * @param [in] x x coordinate of the give point in meter.
     * @param [in] y y coordinate of the given point in meters.
     * @param [in] a long side length(for rectangle area) or long semi-axis (for
     * eclipse)
     * @param [in] b short side length(for rectangle area) or short semi-axis
     * (for eclipse).
     * @param [in] Type the area type.
     * @returns > 0 if point is inside the area, = 0 if the point is on the
     * border of the area, or < 0 if the point is outside of the area.
     */
    static int GeoRelativePosition(int x, int y, int a, int b, GeoAreaType Type) {
        int f;
        if (x == 0 && y == 0)
            return 1;
        x *= x;
        y *= y;
        a *= a;
        b *= b;

        switch (Type) {
            case GeoAreaType::GEO_AREA_TYPE_CIRCLE:
                if (a == (x + y))
                    f = 0;
                else if ( a > (x + y))
                    f = 1;
                else
                    f = -1;
                break;
            case GeoAreaType::GEO_AREA_TYPE_RECT:
                f = std::min(a*b - b*x, a*b - a*y);
                break;
            case GeoAreaType::GEO_AREA_TYPE_ELIP:
                if (a*b == (b*x + a*y))
                    f = 0;
                else if (a*b > (b*x + a*y))
                    f = 1;
                else
                    f = -1;
                break;
            default:
                std::cerr << "Invalid Geo Area type" << std::endl;
        }
        return f;
    }

    static uint32_t GetTimestampSinceEpoch() {
        std::chrono::system_clock::time_point EpochTP;
            // Create TAI epoch, which is 2004-01-01 00:00:00.000
            // UTC
            std::tm tm = {0};
            tm.tm_sec = 0;
            tm.tm_min = 0;
            tm.tm_hour = 0;
            tm.tm_mday = 1;
            tm.tm_mon = 0;
            tm.tm_year = 104;
            tm.tm_isdst = -1;
            // Convert std::tm to std::time_t (popular extension)
            std::time_t tt = timegm(&tm);
            // Convert std::time_t to std::chrono::system_clock::time_point
            EpochTP = std::chrono::system_clock::from_time_t(tt);
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - EpochTP);
        return diff.count();
    }
    //static std::chrono::system_clock::time_point EpochTP;
    };
}   //namespace gn
#endif
