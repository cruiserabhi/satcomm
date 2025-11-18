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
 * @file  ConnectionSecurityFactory.hpp
 * @brief ConnectionSecurityFactory allows creation of managers dealing with
 *        connection security.
 */

#ifndef TELUX_SEC_CONNECTIONSECURITYFACTORY_HPP
#define TELUX_SEC_CONNECTIONSECURITYFACTORY_HPP

#include <telux/sec/CellularSecurityManager.hpp>
#include <telux/sec/WiFiSecurityManager.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * @brief ConnectionConnectionSecurityFactory allows creation of CellularSecurityManager
 * and WiFiSecurityManager.
 */
class ConnectionSecurityFactory {

 public:

    /**
     * Gets the ConnectionSecurityFactory instance.
     */
    static ConnectionSecurityFactory &getInstance();

    /**
     * Provides an ICellularSecurityManager instance that detects and monitors
     * security threats and generates security scan reports for cellular connections.
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if ICellularSecurityManager
     *                is created successfully, otherwise, an appropriate error code
     *
     * @returns ICellularSecurityManager instance or nullptr, if an error occurred
     *
     */
    virtual std::shared_ptr<ICellularSecurityManager> getCellularSecurityManager(
        telux::common::ErrorCode &ec) = 0;

    /**
     * Provides an IWiFiSecurityManager instance that detects and monitors
     * security threats and generates security analysis reports for WiFi connections.
     *
     * @param [in] callback Callback to receive the WiFiSecurityManager initialization status.
     *
     * @returns IWiFiSecurityManager instance or nullptr, if an error occurred
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual std::shared_ptr<IWiFiSecurityManager> getWiFiSecurityManager(
        telux::common::InitResponseCb callback) = 0;

    /**
     * Provides an IWiFiSecurityManager instance that detects and monitors
     * security threats and generates security analysis reports for Wi-Fi connections.
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if IWiFiSecurityManager
     *                is created successfully, otherwise, an appropriate error code
     *
     * @returns IWiFiSecurityManager instance or nullptr, if an error occurred
     *
     * @deprected use the getWiFiSecurityManager(telux::common::InitResponseCb callback) API instead
     */
    virtual std::shared_ptr<IWiFiSecurityManager> getWiFiSecurityManager(
        telux::common::ErrorCode &ec) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    ConnectionSecurityFactory();
    virtual ~ConnectionSecurityFactory();
#endif

 private:
    ConnectionSecurityFactory(const ConnectionSecurityFactory &) = delete;
    ConnectionSecurityFactory &operator=(const ConnectionSecurityFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_CONNECTIONSECURITYFACTORY_HPP
