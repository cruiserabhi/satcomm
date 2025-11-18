/*
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       PlatformFactory.hpp
 *
 * @brief      PlatformFactory creates a set of managers which provide the corresponding
 *             platform services.
 */

#ifndef TELUX_PLATFORM_PLATFORMFACTORY_HPP
#define TELUX_PLATFORM_PLATFORMFACTORY_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/FsManager.hpp>
#include <telux/platform/DeviceInfoManager.hpp>
#include <telux/platform/TimeManager.hpp>

#include <telux/platform/hardware/AntennaManager.hpp>

namespace telux {

namespace platform {
/** @addtogroup telematics_platform
 * @{ */

/**
 * @brief   PlatformFactory allows creation of Platform services related classes.
 */

class PlatformFactory {
 public:
    /**
     * Get instance of platform Factory
     */
    static PlatformFactory &getInstance();

    /**
     * Get instance of filesystem manager (IFsManager). The filesystem manager supports
     * notification of filesystem events like EFS restore indications.
     *
     * @param [in] callback      Optional callback to get the initialization status of
     *                           FsManager. @ref telux::common::InitResponseCb
     *
     * @returns pointer of @ref IFsManager object.
     */
    virtual std::shared_ptr<IFsManager> getFsManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * Get instance of device info manager (IDeviceInfoManager). The device info manager
     * supports device info request like retrieving IMEI and platform version.
     *
     * @param [in] callback      Optional callback to get the initialization status of
     *                           FsManager. @ref telux::common::InitResponseCb
     *
     * @returns pointer of @ref IDeviceInfoManager object.
     */
    virtual std::shared_ptr<IDeviceInfoManager> getDeviceInfoManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * Gets a time manager (ITimeManger) instance. The time manager
     * supports registering for time reports.
     *
     * @param [in] callback      Optional callback to get the initialization status of
     *                           ITimeManager. @ref telux::common::InitResponseCb
     *
     * @returns ITimeManager instance or nullptr if time management is not supported.
     */
    virtual std::shared_ptr<ITimeManager> getTimeManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * Gets an antenna manager (IAntennaManager) instance.
     *
     * @param [in] callback   Optional callback to get the initialization status of
     *                        antenna manager @ref telux::common::InitResponseCb
     *
     * @returns IAntennaManager instance or nullptr if antenna management is not
     * supported.
     *
     */
    virtual std::shared_ptr<hardware::IAntennaManager> getAntennaManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    PlatformFactory();
    virtual ~PlatformFactory();
#endif

 private:
    PlatformFactory(const PlatformFactory &) = delete;
    PlatformFactory &operator=(const PlatformFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_platform */
}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_PLATFORMFACTORY_HPP
