/**
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ModemManager.hpp
 *
 * @brief      ModemManager class has common APIs used across verticals like current serving RAT,
 *             domain and operating mode etc.
 *
 */

/**
 * ModemManager class has common APIs used across verticals like current serving RAT, domain and
 * operating mode etc.
 */

#ifndef TELUX_COMMON_MODEMMANAGER_HPP
#define TELUX_COMMON_MODEMMANAGER_HPP

#include <memory>
#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace common {

class IModemManager {

public:

    /**
     * Get voice service state
     *
     * @param [in] slotId           Unique identifier for the SIM slot.
     * @param [in] serviceInfo      Reference to voice service info object provided by caller.
     *
     * @returns returns suitable error code.
     *
     */
    virtual telux::common::ErrorCode getVoiceServiceState(int slotId,
        telStub::VoiceServiceStateInfo& serviceInfo) = 0;

    /**
     * Get system info like serving RAT and domain.
     *
     * @param [in] slotId          Unique identifier for the SIM slot.
     * @param [in] servingRat      Reference to serving RAT object provided by caller.
     * @param [in] servingDomain   Reference to serving domain object provided by caller.
     *
     * @returns suitable error code.
     *
     */
    virtual telux::common::ErrorCode getSystemInfo(int slotId, telStub::RadioTechnology &servingRat,
        telStub::ServiceDomainInfo_Domain &servingDomain) = 0;

    /**
     * Get eCall operating mode
     *
     * @param [in] slotId       Unique identifier for the SIM slot.
     * @param [in] mode         Reference to eCall mode info object provided by caller.

     * @returns suitable error code.
     */
    virtual telux::common::ErrorCode getEcallOperatingMode(int slotId,
        telStub::ECallMode& mode) = 0;

    /**
     * Destructor for IModemManager
     */
    virtual ~IModemManager(){};

};

}  // End of namespace common

}  // End of namespace telux

#endif  // TELUX_COMMON_MODEMMANAGER_HPP