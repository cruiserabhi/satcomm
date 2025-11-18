/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file  SecurityFactory.hpp
 * @brief SecurityFactory allows creation of CryptoManager.
 */

#ifndef TELUX_SEC_SECURITYFACTORY_HPP
#define TELUX_SEC_SECURITYFACTORY_HPP

#include <telux/sec/CryptoManager.hpp>
#include <telux/sec/CryptoAcceleratorManager.hpp>
#include <telux/sec/CAControlManager.hpp>
#include <telux/sec/RandomNumberManager.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * @brief SecurityFactory allows creation of ICryptoManager and ICryptoAcceleratorManager.
 */
class SecurityFactory {
 public:
    /**
     * Gets the SecurityFactory instance.
     */
    static SecurityFactory &getInstance();

    /**
     * Instantiates a CryptoManager instance that can be used to perform key management
     * and cryptographic operations.
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if ICryptoManager is created
     *                successfully, otherwise, an appropriate error code
     *
     * @returns ICryptoManager instance
     *
     */
    virtual std::shared_ptr<ICryptoManager> getCryptoManager(
       telux::common::ErrorCode &ec) = 0;

    /**
     * Provides a CryptoAcceleratorManager instance that can be used to perform
     * cryptographic operations requiring elliptic-curve cryptography (ECC)
     * verifications and calculations.
     *
     * Providing ICryptoAcceleratorListener instance is mandatory when using
     * Mode::MODE_ASYNC_LISTENER. It is not required with modes, Mode::MODE_SYNC and
     * Mode::MODE_ASYNC_POLL for cryptographic operations.
     *
     * To receive subsystem-restart (SSR) updates, application must provide
     * ICryptoAcceleratorListener instance (irrespective of Mode::*) and implement
     * method telux::common::IServiceStatusListener::onServiceStatusChange().
     *
     * Specifying mode (Mode::*) defines how an application will send request and
     * receive cryptographic results.
     *
     * Passing listener determines whether an application is also interested in SSR
     * updates in addition to cryptographic results or not.
     *
     * On platforms with access control enabled, caller needs to have TELUX_SEC_ACCELERATOR_MGR
     * permission to invoke this API successfully.
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if ICryptoAcceleratorManager is
     *                created successfully, otherwise, an appropriate error code
     *
     * @param[in] mode Defines how users obtain verification and calculation results
     *
     * @param[in] cryptoAccelListener Optional, listener for ECC signature verification
     *                                and ECQV calculation results
     *
     * @returns ICryptoAcceleratorManager instance
     *
     */
    virtual std::shared_ptr<ICryptoAcceleratorManager> getCryptoAcceleratorManager(
      telux::common::ErrorCode &ec, Mode mode,
      std::weak_ptr<ICryptoAcceleratorListener> cryptoAccelListener = std::weak_ptr<
      ICryptoAcceleratorListener>()
    ) = 0;

    /**
     * Provides an ICAControlManager instance that can be used to collect statistical
     * information about usage of the crypto accelerator.
     *
     * On platforms with access control enabled, caller needs to have TELUX_SEC_CA_CONTROL_MGR
     * permission to invoke this API successfully.
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if the ICAControlManager is
     *                created successfully, otherwise, an appropriate error code
     *
     * @returns ICAControlManager instance
     *
     */
    virtual std::shared_ptr<ICAControlManager> getCAControlManager(
       telux::common::ErrorCode &ec) = 0;

    /**
     * Provides an IRandomNumberManager instance that can be used to generate random
     * number/data.
     *
     * @param[in] generatorSource Random number generator source to use
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if the IRandomNumberManager is
     *                created successfully, telux::common::ErrorCode::INCOMPATIBLE_STATE
     *                if the platform has been configured to use a RNG that does not
     *                correspond to the RNGSource passed to the API, otherwise, an
     *                appropriate error code.
     *
     * @returns IRandomNumberManager instance
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual std::shared_ptr<IRandomNumberManager> getRandomNumberManager(
       RNGSource generatorSource, telux::common::ErrorCode &ec) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    SecurityFactory();
    virtual ~SecurityFactory();
#endif

 private:
    SecurityFactory(const SecurityFactory &) = delete;
    SecurityFactory &operator=(const SecurityFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_SECURITYFACTORY_HPP
