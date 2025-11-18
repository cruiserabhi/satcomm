/*
 *  Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to make an ecall. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a ICallManager instance from the PhoneFactory.
 * 3. Wait for the call manager service to become available.
 * 4. Print an eCall MSD payload.
 * 5. Trigger an ecall with retrieved pdu.
 * 6. Receive status of the ecall in callback.
 * 7. Wait while the call is in progress.
 * 8. Finally, when the use case is over, hangup the call.
 *
 * Usage:
 * # ./make_ecall_with_pdu_app
 */

#include <errno.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/CallManager.hpp>

class ECallerWithPdu : public telux::tel::IMakeCallCallback,
                       public std::enable_shared_from_this<ECallerWithPdu> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        callMgr_ = phoneFactory.getCallManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!callMgr_) {
            std::cout << "Can't get ICallManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Call manager service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int triggerECall() {
        telux::common::Status status;
        int emergencyCategory = 64;
        int eCallVariant = 1;
        int phoneId = DEFAULT_PHONE_ID;
        /* Populate eCallMsdData with valid information */
        telux::tel::ECallMsdData eCallMsdData{};
        eCallMsdData.optionals.recentVehicleLocationN1Present = true;
        eCallMsdData.optionals.recentVehicleLocationN2Present = true;
        eCallMsdData.optionals.numberOfPassengersPresent = 1;
        eCallMsdData.messageIdentifier = 60;
        eCallMsdData.control.automaticActivation = true;
        eCallMsdData.control.testCall = false;
        eCallMsdData.control.positionCanBeTrusted = true;
        eCallMsdData.control.vehicleType = telux::tel::ECallVehicleType::PASSENGER_VEHICLE_CLASS_M1;
        eCallMsdData.vehicleIdentificationNumber.isowmi = "ECA";
        eCallMsdData.vehicleIdentificationNumber.isovds = "LLEXAM";
        eCallMsdData.vehicleIdentificationNumber.isovisModelyear = "P";
        eCallMsdData.vehicleIdentificationNumber.isovisSeqPlant = "LE02013";
        eCallMsdData.vehiclePropulsionStorage.gasolineTankPresent = true;
        eCallMsdData.vehiclePropulsionStorage.dieselTankPresent = true;
        eCallMsdData.vehiclePropulsionStorage.compressedNaturalGas = false;
        eCallMsdData.vehiclePropulsionStorage.liquidPropaneGas = false;
        eCallMsdData.vehiclePropulsionStorage.electricEnergyStorage = false;
        eCallMsdData.vehiclePropulsionStorage.hydrogenStorage = false;
        eCallMsdData.vehiclePropulsionStorage.otherStorage = false;
        eCallMsdData.timestamp = 1367878452;
        eCallMsdData.vehicleLocation.positionLatitude = 123;
        eCallMsdData.vehicleLocation.positionLongitude = 1234;
        eCallMsdData.vehicleDirection = 4;
        eCallMsdData.optionals.optionalDataPresent = true;
        eCallMsdData.recentVehicleLocationN1.latitudeDelta = -1;
        eCallMsdData.recentVehicleLocationN1.longitudeDelta = -10;
        eCallMsdData.recentVehicleLocationN2.latitudeDelta = -1;
        eCallMsdData.recentVehicleLocationN2.longitudeDelta = -30;
        eCallMsdData.numberOfPassengers = 2;
        eCallMsdData.optionalPdu.oid = "8.1";
        /* If already encoded optional additional data content is available, fill "oadData",
          otherwise fill Euro NCAP optional additional data content fields.
          # For example, std::string oadData("0832D28480"); */
        std::string oadData("");
        if (!oadData.empty()) {
            std::vector<uint8_t> data(oadData.begin(), oadData.end());
            eCallMsdData.optionalPdu.data = data;
        } else {
            std::vector<uint8_t> data;
            // get encoded optional additional data content
            telux::tel::ECallOptionalEuroNcapData optionalEuroNcapData = {};
            // refer ECallLocationOfImpact for more values.
            optionalEuroNcapData.locationOfImpact = telux::tel::ECallLocationOfImpact::FRONT;
            optionalEuroNcapData.rollOverDetectedPresent = false;
            optionalEuroNcapData.rollOverDetected = false;
            // deltav range limit is 100 to 255
            optionalEuroNcapData.deltaV.rangeLimit = 125;
            // deltav VX range is -255 to 255
            optionalEuroNcapData.deltaV.deltaVX = -45;
            // deltav VY range is -255 to 255
            optionalEuroNcapData.deltaV.deltaVY = 10;
            auto encodeOADContentStatus = callMgr_->encodeEuroNcapOptionalAdditionalData(
                optionalEuroNcapData, data);
            if (encodeOADContentStatus != telux::common::Status::SUCCESS) {
                std::cout << " Optional additional data content encoding failed" << std::endl;
                return 1;
            }
            eCallMsdData.optionalPdu.data = data;
        }

        /* Step - 4 */
        std::vector<uint8_t> msdPdu = {};
        telux::common::ErrorCode errCode = callMgr_->encodeECallMsd(eCallMsdData, msdPdu);
        if (errCode == telux::common::ErrorCode::SUCCESS) {
            std::stringstream ss;
            for (auto i : msdPdu) {
                ss << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << (int)i;
            }
            std::cout << "ECall MSD payload is : " << ss.str() << std::endl;
        } else {
            std::cout << "ERROR - Failed to retrieve ecall msd payload,"
                << " Error:" << static_cast<int>(errCode) << "\n";
        }
        /* Step - 5 */
        status = callMgr_->makeECall(phoneId,
            msdPdu, emergencyCategory, eCallVariant,
            std::bind(&ECallerWithPdu::makeCallResponse, this, std::placeholders::_1,
                 std::placeholders::_2));
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't call, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Call initiated" << std::endl;
        return 0;
    }

    int terminateCall() {
        telux::common::Status status;

        /* Step - 8 */
        status = dialedCall_->hangup();
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Failed to hangup, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Call termination initiated" << std::endl;
        return 0;
    }

    /* Step - 6 */
    void makeCallResponse(telux::common::ErrorCode ec,
            std::shared_ptr<telux::tel::ICall> call) override {
        std::cout << "makeCallResponse()" << std::endl;

        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to call, err " << static_cast<int>(ec) << std::endl;
            return;
        }

        dialedCall_ = call;

        std::cout << "Index " << call->getCallIndex() <<
            " direction " << static_cast<int>(call->getCallDirection()) <<
            " number " << call->getRemotePartyNumber() << std::endl;
    }

 private:
    std::shared_ptr<telux::tel::ICall> dialedCall_;
    std::shared_ptr<telux::tel::ICallManager> callMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<ECallerWithPdu> app;

    try {
        app = std::make_shared<ECallerWithPdu>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate ECallerPdu" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->triggerECall();
    if (ret < 0) {
        return ret;
    }

    /* Step - 7 */
    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::minutes(3));

    ret = app->terminateCall();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nECall app exiting" << std::endl;
    return 0;
}
