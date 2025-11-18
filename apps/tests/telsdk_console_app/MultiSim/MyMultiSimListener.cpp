/*
 *  Copyright (c) 2020 The Linux Foundation. All rights reserved.
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
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include "MyMultiSimListener.hpp"
#include "MyMultiSimHandler.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void MyMultiSimListener::onHighCapabilityChanged(int slotId) {
    std::cout << "\n";
    PRINT_NOTIFICATION << "onHighCapabilityChanged called" << std::endl;
    PRINT_NOTIFICATION << "High capability changed to slot " << slotId << std::endl;
}

void MyMultiSimListener::onSlotStatusChanged(std::map<SlotId, telux::tel::SlotStatus> slotStatus) {
    PRINT_NOTIFICATION << "Slot status change notification received" << std::endl;
    for(auto it = slotStatus.begin(); it != slotStatus.end(); ++it) {
        auto slotId = it->first;
        auto slotStatus = it->second;
        PRINT_NOTIFICATION << " SlotId: " << static_cast<int>(slotId)
                     << ", SlotState: " << MyMultiSimHelper::slotStateToString(slotStatus.slotState)
                     << ", CardState: " << MyMultiSimHelper::cardStateToString(slotStatus.cardState)
                     << ", CardError: " << MyMultiSimHelper::cardErrorToString(slotStatus.cardError)
                     << std::endl;
    }
}

// Notify MultiSimManager subsystem status
void MyMultiSimListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::string stat = "";
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }
    PRINT_NOTIFICATION << " MultiSim onServiceStatusChange" << stat << "\n";
}
