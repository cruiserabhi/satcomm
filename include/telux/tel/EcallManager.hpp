/*
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       EcallManager.hpp
 * @brief      EcallManager allows operations related to emergency call management and
 *             configuration.
 *
 */

#ifndef TELUX_TEL_ECALLMANAGER_HPP
#define TELUX_TEL_ECALLMANAGER_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include "ECallDefines.hpp"

namespace telux {
namespace tel {

// Forward declaration
class IEcallListener;

/** @addtogroup telematics_ecall
 * @{ */

/**
 *@brief   IEcallManager allows operations related to automotive emergency call management and its
 *         related configurations.
 *
 */
class IEcallManager {
public:
    /**
     * Checks the status of IEcallManager sub-system and returns the result.
     *
     * @returns the status of IEcallManager sub-system status @ref telux::common::ServiceStatus
     *
     * @deprecated This API is not being supported
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Set the configuration related to emergency call.
    * The configuration is persistent and takes effect when the next emergency call is dialed.
    *
    * Minimum value of EcallConfig.t9Timer value should be 3600000. If a lesser value is provided,
    * this API will still succeed but the actual value would be set to 3600000.
    *
    * @param [in] config   eCall configuration to be set
    *                      @ref EcallConfig
    *
    * @returns Status of setConfig i.e. success or suitable error code.
    *
    * @deprecated This API is not being supported. Use ICallManager::setECallConfig() API instead.
    */
   virtual telux::common::Status setConfig(EcallConfig config) = 0;

   /**
    * Get the configuration related to emergency call.
    *
    * @param [out] config   Parameter to hold the fetched eCall configuration
    *                       @ref EcallConfig
    *
    * @returns Status of getConfig i.e. success or suitable error code.
    *
    * @deprecated This API is not being supported. Use ICallManager::getECallConfig() API instead.
    */
   virtual telux::common::Status getConfig(EcallConfig &config) = 0;

   /**
    * Register a listener for notifications from the EcallManager.
    *
    * @param [in] listener  Pointer to IEcallListener object that processes the
    *                       notification
    *
    * @returns Status of registerListener i.e. success or suitable error code.
    *
    * @deprecated This API is not being supported
    */
   virtual telux::common::Status registerListener(std::weak_ptr<IEcallListener> listener) = 0;

   /**
    * Deregister a previously registered listener.
    *
    * @param [in] listener    Pointer to IEcallListener object that needs to be
    *                         deregistered.
    *
    * @returns Status of deregisterListener i.e. success or suitable error code.
    *
    * @deprecated This API is not being supported
    */
   virtual telux::common::Status deregisterListener(std::weak_ptr<IEcallListener> listener) = 0;

   virtual ~IEcallManager(){};
};

/**
 * @brief Listener class to notify service status change notifications.
 *        The listener method can be invoked from multiple different threads.
 *        Client needs to make sure that implementation is thread-safe.
 */
class IEcallListener : public common::IServiceStatusListener {
public:

   /**
    * Destructor of IEcallListener
    */
   virtual ~IEcallListener() {
   }
};

/** @} */ /* end_addtogroup telematics_ecall */
}
}

#endif // TELUX_TEL_ECALLMANAGER_HPP
