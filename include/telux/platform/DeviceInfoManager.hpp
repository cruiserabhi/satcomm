/*
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       DeviceInfoManager.hpp
 * @brief      DeviceInfoManager provides APIs related to Device Info management such as retrieving
 *             IMEI and platform version operations.
 */

#ifndef TELUX_PLATFORM_DEVICEINFOMANAGER_HPP
#define TELUX_PLATFORM_DEVICEINFOMANAGER_HPP

#include <memory>
#include <string>
#include <telux/common/CommonDefines.hpp>
#include <telux/platform/DeviceInfoListener.hpp>

namespace telux {

namespace platform {
/** @addtogroup telematics_platform_deviceinfo
 * @{ */

/**
 * Structure contains the version of the platform software
 */
struct PlatformVersion {
   std::string meta; /**< Meta Version,
                                for example: SA2150P_SA515M.LE_LE.1-3_2-1-00297-STD.INT-1*/
   std::string modem; /**< Modem Version,
                                for example: MPSS.HI.3.1.c3-00114-SDX55_GENAUTO_TEST-1*/
   std::string externalApp; /**< External App Version,
                                for example: LE.UM.3.2.3-72102-SA2150p.Int-1*/
   std::string integratedApp; /**< Integrated App MDM Version,
                                for example: LE.UM.4.1.1-71802-sa515m.Int-1*/
};

/**
 * @brief   IDeviceInfoManager provides interface to to retrieve IMEI and
 *          platform version operations.
 */
class IDeviceInfoManager {
 public:
    /**
     * This status indicates whether the object is in a usable state.
     *
     * @returns @ref telux::common::ServiceStatus indicating the current status of the device info
     *          service.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Registers the listener for FileSystem Manager indications.
     *
     * @param [in] listener      - pointer to implemented listener.
     *
     * @returns status of the registration request.
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IDeviceInfoListener> listener) = 0;

    /**
     * Deregisters the previously registered listener.
     *
     * @param [in] listener      - pointer to registered listener that needs to be removed.
     *
     * @returns status of the deregistration request.
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IDeviceInfoListener> listener)
       = 0;

    /**
     * Get the platform version. On Hypervisor based platforms, on guest VM, only current
     * application processor image is available, other images version data cannot be obtained.
     *
     * @param [out] pv   - @ref telux::platform::PlatformVersion
     *
     * @returns - @ref telux::common::Status
     *
     */
    virtual telux::common::Status getPlatformVersion(PlatformVersion & pv) = 0;

    /**
     * Get the international mobile equipment identity.
     *
     * @param [out] imei   - @ref std::string
     *
     * @returns - @ref telux::common::Status
     *
     */
    virtual telux::common::Status getIMEI(std::string & imei) = 0;

    /**
     * Destructor of IDeviceInfoManager
     */
    virtual ~IDeviceInfoManager(){};
};

/** @} */ /* end_addtogroup telematics_platform_deviceinfo */
}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_DEVICEINFOMANAGER_HPP
