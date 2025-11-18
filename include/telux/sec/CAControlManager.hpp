/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file  CAControlManager.hpp
 * @brief CAControlManager provides support for gathering statistical information
 *        about crypto operations that can be used to control crypto accelerator
 *        usage.
 */

#ifndef TELUX_SEC_CACONTROLMANAGER_HPP
#define TELUX_SEC_CACONTROLMANAGER_HPP

#include <cstdint>
#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * Specifies how load should be calculated.
 */
struct LoadConfig {
    /* Defines the time window (in milliseconds) during which
     * load is calculated. At the end of this window, load will be
     * received by @ref ICAControlManagerListener::onLoadUpdate(). */
    uint64_t calculationInterval;
};

/**
 * Represents curve-wise absolute capacity. This value represents
 * capacity as if only that type of curve is used in all crypto
 * operations. For example, a capacity of 3000 for sm2 means, 3000
 * signature verifications of type sm2 can be done under current
 * operating conditions, when no other type of verifications are performed.
 */
struct CACapacity {

    /* SM2 ISO/IEC 14888 */
    uint32_t sm2;

    /* NIST curve P-256 */
    uint32_t nist256;

    /* NIST curve P-396 */
    uint32_t nist384;

    /* Brainpool 256-bit curve */
    uint32_t bp256;

    /* Brainpool 384-bit curve */
    uint32_t bp384;
};

/**
 * Represents curve-wise absolute load as calculated in the time window defined
 * by @ref LoadConfig::calculationInterval. For example, a value of 1000
 * for sm2 means, in that time window, 1000 sm2 type verification were completed.
 * This verification includes both passed and failed signature.
 */
struct CALoad {

    /* SM2 ISO/IEC 14888 */
    uint32_t sm2;

    /* NIST curve P-256 */
    uint32_t nist256;

    /* NIST curve P-396 */
    uint32_t nist384;

    /* Brainpool 256-bit curve */
    uint32_t bp256;

    /* Brainpool 384-bit curve */
    uint32_t bp384;
};

/**
 * Receives load and capacity updates.
 */
class ICAControlManagerListener : public telux::common::ISDKListener {

 public:
    /**
     * Invoked to provide an updated capacity.
     *
     * @param[in] newCapacity New capacity as per current allowed conditions.
     *
     */
    virtual void onCapacityUpdate(struct CACapacity newCapacity) { }

    /**
     * Invoked to provide load on crypto accelerator, as observed during time window
     * defined by @ref LoadConfig::calculationInterval.
     *
     * @param[in] currentLoad Load as observed in the set time window.
     *
     */
    virtual void onLoadUpdate(struct CALoad currentLoad) { }

    /**
     * Destructor for ICAControlManagerListener.
     */
    virtual ~ICAControlManagerListener() { }
};

/*
 * Provides support for gathering statistical information about crypto operations
 * that can be used to control crypto accelerator usage.
 */
class ICAControlManager {

 public:

   /**
    * Registers the given listener to get load and capacity updates in
    * @ref ICAControlManagerListener::onLoadUpdate() and
    * @ref ICAControlManagerListener::onCapacityUpdate() methods.
    *
    * Capacity updates are received whenever capacity changes. Load updates
    * are received as per parameters specified with @ref startMonitoring().
    *
    * @param [in] listener Receives load and capacity updates
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode registerListener(
        std::weak_ptr<ICAControlManagerListener> listener) = 0;

   /**
    * Unregisters the given listener registered previously with @ref registerListener().
    *
    * @param [in] listener Listener to deregister
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is unregistered,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode deRegisterListener(
        std::weak_ptr<ICAControlManagerListener> listener) = 0;

   /**
    * Starts monitoring and reporting load calculated based on the parameters specified.
    * Calculated load is received by @ref ICAControlManagerListener::onLoadUpdate()
    * periodically as per time interval specified.
    *
    * On platforms with access control enabled, caller needs to have TELUX_SEC_CA_CTRL_LOAD_OPS
    * permission to invoke this API successfully.
    *
    * @param [in] loadConfig Defines load calculation parameters
    *
    * @returns @ref telux::common::Status::SUCCESS if the monitoring started,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode startMonitoring(LoadConfig loadConfig) = 0;

   /**
    * Stops monitoring the load calculation previosuly started by @ref startMonitoring().
    *
    * On platforms with access control enabled, caller needs to have TELUX_SEC_CA_CTRL_LOAD_OPS
    * permission to invoke this API successfully.
    *
    * @returns @ref telux::common::Status::SUCCESS if the monitoring stopped,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode stopMonitoring() = 0;

   /**
    * Provides current verification capacity of the crypto accelerator.
    *
    * @param [out] capacity current capacity of the crypto accelerator
    *
    * @returns @ref telux::common::Status::SUCCESS if the capacity is fetched,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode getCapacity(CACapacity& capacity) = 0;

   /**
    * Destructor of ICAControlManager. Cleans up as applicable.
    */
   virtual ~ICAControlManager() { };
};

/** @} */  // end_addtogroup telematics_sec_mgmt

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_CACONTROLMANAGER_HPP
