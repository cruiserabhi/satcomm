/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
  * @file: VehicleReceive.cpp
  *
  * @brief: Implementation of VehicleReceive.h
  *
  */

#include "VehicleReceive.h"

VehicleReceive::~VehicleReceive(){
    disableVehicleReceive();
}

bool VehicleReceive::enableVehicleReceive(VehicleEventsCallback cb) {
    evtCallback = cb;

    handle = v2x_vehicle_register_listener(VehicleReceive::onVehicleDataChanges, this);
    if (handle == V2X_VDATA_HANDLE_BAD) {
        cout<<"Error creating listener on Vehicle Receive.\n";
        return false;
    }

    return true;
}

bool VehicleReceive::disableVehicleReceive() {
    if (handle == V2X_VDATA_HANDLE_BAD) {
        return true;
    }

    if (v2x_vehicle_deregister_for_callback(handle)) {
        cout<<"Error remove listener on Vehicle Receive.\n";
        return false;
    }

    return true;
}

void VehicleReceive::onVehicleDataChanges(current_dynamic_vehicle_state_t *vehicleData,
                                          void *context) {

    if (nullptr == context) {
        cout << "Wrong Vehicle context.\n";
        return;
    }

    if (nullptr == vehicleData) {
        cout << "vehicleData argument is null\n";
        return;
    }

    VehicleReceive* veh = static_cast<VehicleReceive*>(context);

    auto critical = veh->events.any();

    vehicleData->events.bits.eventAirBagDeployment ? veh->events.set(AIR_BAG_DEPLOYED) :
        veh->events.reset(AIR_BAG_DEPLOYED);
    vehicleData->events.bits.eventDisabledVehicle ? veh->events.set(VEHICLE_DISABLED) :
        veh->events.reset(VEHICLE_DISABLED);
    vehicleData->events.bits.eventFlatTire ? veh->events.set(FLAT_TIRE) :
        veh->events.reset(FLAT_TIRE);
    vehicleData->events.bits.eventHardBraking ? veh->events.set(HARD_BRAKE) :
        veh->events.reset(HARD_BRAKE);
    vehicleData->events.bits.eventStabilityControlactivated ?
        veh->events.set(STABILITY_CTRL_ACTIVE) : veh->events.reset(STABILITY_CTRL_ACTIVE);
    vehicleData->events.bits.eventTractionControlLoss ? veh->events.set(TRACTION_CTRL_ACTIVE) :
        veh->events.reset(TRACTION_CTRL_ACTIVE);
    vehicleData->events.bits.eventABSactivated ? veh->events.set(ABS_ACTIVE) :
        veh->events.reset(ABS_ACTIVE);

    if (critical && veh->events.none()) {
        // critical events disappear
        veh->evtCallback(false, nullptr);
    } else if (veh->events.any()) {
        // critical events appear
        veh->evtCallback(true, vehicleData);
    }
}
