/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * @file       MsdProvider.cpp
 *
 * @brief      MsdProvider class fetches the MSD data and will cache the MSD parameters.
 *             It provides utility functions to read the configured values.
 */

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "MsdProvider.hpp"
#include "ConfigParser.hpp"

#define MSD_VERSION_TWO 2
#define MSD_VERSION_THREE 3

telux::tel::ECallMsdData MsdProvider::msdData_ = {};
std::vector<uint8_t> MsdProvider::optionalAdditionalDataContent_ = {};

/**
 * Reads MSD data from file and caches it
 */
telux::tel::ECallMsdData MsdProvider::getMsd() {
    return msdData_;
}

/**
 * Sets MSD optional additional data content.
 */
void MsdProvider::setOptionalAdditionalDataContent(
    std::vector<uint8_t> optionalAdditionalDataContent) {
    optionalAdditionalDataContent_ = optionalAdditionalDataContent;
}

/**
 * Function to read MSD config file containing optional additional data content key value pairs.
 */
telux::tel::ECallOptionalEuroNcapData MsdProvider::readEuroNcapOptionalAdditionalDataContent(
    std::string filename, std::string filePath) {
    std::shared_ptr<ConfigParser> msdSettings = nullptr;
    telux::tel::ECallOptionalEuroNcapData optionalEuroNcapData = {};
    try {
        msdSettings = std::make_shared<ConfigParser>(filename, filePath);
    } catch (std::bad_alloc & e) {
        std::cout << "MSD parsing failed, error: "<< e.what() << std::endl;
        return optionalEuroNcapData;
    }
    // Euro NCAP OPTIONAL_ADDTIONAL_DATA_CONTENT
    auto locationOfImpactAsString = msdSettings->getValue("EURONCAP_LOCATION_OF_IMPACT");
    optionalEuroNcapData.locationOfImpact =
        static_cast<telux::tel::ECallLocationOfImpact>(atoi(locationOfImpactAsString.c_str()));
    auto rollOverDetectedPresentAsString =
        msdSettings->getValue("EURONCAP_ROLL_OVER_DETECTED_PRESENT");
    bool rollOverDetectedPresentAsBool
        = atoi(rollOverDetectedPresentAsString.c_str()) ? true : false;
    optionalEuroNcapData.rollOverDetectedPresent =
        rollOverDetectedPresentAsBool;
    optionalEuroNcapData.rollOverDetected =
        atoi(msdSettings->getValue("EURONCAP_ROLL_OVER_DETECTED").c_str());
    optionalEuroNcapData.deltaV.rangeLimit =
        atoi(msdSettings->getValue("EURONCAP_DELTAV_RANGELIMIT").c_str());
    optionalEuroNcapData.deltaV.deltaVX =
        atoi(msdSettings->getValue("EURONCAP_DELTAV_DELTAVX").c_str());
    optionalEuroNcapData.deltaV.deltaVY =
        atoi(msdSettings->getValue("EURONCAP_DELTAV_DELTAVY").c_str());
    return optionalEuroNcapData;
}

/**
 * Function to read MSD config file containing key value pairs
 */
void MsdProvider::init(std::string filename, std::string filePath) {
    std::shared_ptr<ConfigParser> msdSettings = nullptr;
    try {
        msdSettings = std::make_shared<ConfigParser>(filename, filePath);
    } catch (std::bad_alloc & e) {
        std::cout << "MSD parsing failed, error: "<< e.what() << std::endl;
        return;
    }
    // Parse MSD Version. When relevant config is not found, default to MSD version-2
    if(msdSettings->getValue("MSD_VERSION").empty()) {
        msdData_.msdVersion = MSD_VERSION_TWO;
    } else {
        msdData_.msdVersion = atoi(msdSettings->getValue("MSD_VERSION").c_str());
    }
    std::cout << "ECall MSD Version: " << static_cast<int>(msdData_.msdVersion) << std::endl;

    // Recent location information is optional only in MSD version-2
    if(msdData_.msdVersion == MSD_VERSION_TWO) {
        // RECENT_LOCATION_N1_PRESENT
        auto recentVehicleLocationN1PresentAsString
          = msdSettings->getValue("RECENT_LOCATION_N1_PRESENT");
        bool recentVehicleLocationN1PresentAsBool
          = atoi(recentVehicleLocationN1PresentAsString.c_str()) ? true : false;
        msdData_.optionals.recentVehicleLocationN1Present = recentVehicleLocationN1PresentAsBool;

        // RECENT_LOCATION_N2_PRESENT
        auto recentVehicleLocationN2PresentAsString
          = msdSettings->getValue("RECENT_LOCATION_N2_PRESENT");
        bool recentVehicleLocationN2PresentAsBool
          = atoi(recentVehicleLocationN2PresentAsString.c_str()) ? true : false;
        msdData_.optionals.recentVehicleLocationN2Present = recentVehicleLocationN2PresentAsBool;
    } else if(msdData_.msdVersion == MSD_VERSION_THREE) {
        msdData_.optionals.recentVehicleLocationN1Present = true;
        msdData_.optionals.recentVehicleLocationN2Present = true;
    }

    // NUMBER_OF_PASSENGERS_PRESENT
    auto numberOfPassengersPresentAsString = msdSettings->getValue("NUMBER_OF_PASSENGERS_PRESENT");
    bool numberOfPassengersPresentAsBool
      = atoi(numberOfPassengersPresentAsString.c_str()) ? true : false;
    msdData_.optionals.numberOfPassengersPresent = numberOfPassengersPresentAsBool;

    // OPTIONAL_ADDITIONAL_DATA_PRESENT
    auto optionalAdditionalDataPresentAsString =
        msdSettings->getValue("OPTIONAL_ADDITIONAL_DATA_PRESENT");
    bool optionalAdditionalDataPresentAsBool
        = atoi(optionalAdditionalDataPresentAsString.c_str()) ? true : false;
    msdData_.optionals.optionalDataPresent = optionalAdditionalDataPresentAsBool;

    // MESSAGE_IDENTIFIER
    msdData_.messageIdentifier = atoi(msdSettings->getValue("MESSAGE_IDENTIFIER").c_str());

    // AUTOMATIC_ACTIVATION
    auto automaticActivationtAsString = msdSettings->getValue("AUTOMATIC_ACTIVATION");
    bool automaticActivationAsBool = atoi(automaticActivationtAsString.c_str()) ? true : false;
    msdData_.control.automaticActivation = automaticActivationAsBool;

    // TEST_CALL
    auto testCallAsString = msdSettings->getValue("TEST_CALL");
    bool testCallAsBool = atoi(testCallAsString.c_str()) ? true : false;
    msdData_.control.testCall = testCallAsBool;

    // POSITION_CAN_BE_TRUSTED
    auto positionCanBeTrustedAsString = msdSettings->getValue("POSITION_CAN_BE_TRUSTED");
    bool positionCanBeTrustedAsBool = atoi(positionCanBeTrustedAsString.c_str()) ? true : false;
    msdData_.control.positionCanBeTrusted = positionCanBeTrustedAsBool;

    // VEHICLE_TYPE
    auto vehicleTypeAsString = msdSettings->getValue("VEHICLE_TYPE");
    msdData_.control.vehicleType
      = static_cast<telux::tel::ECallVehicleType>(atoi(vehicleTypeAsString.c_str()));

    // ISO_WMI
    msdData_.vehicleIdentificationNumber.isowmi = msdSettings->getValue("ISO_WMI");

    // ISO_VDS
    msdData_.vehicleIdentificationNumber.isovds = msdSettings->getValue("ISO_VDS");

    // ISO_VIS_MODEL_YEAR
    msdData_.vehicleIdentificationNumber.isovisModelyear
      = msdSettings->getValue("ISO_VIS_MODEL_YEAR");

    // ISO_VIS_SEQ_PLANT
    msdData_.vehicleIdentificationNumber.isovisSeqPlant =
                                            msdSettings->getValue("ISO_VIS_SEQ_PLANT");

    // GASOLINE_TANK_PRESENT
    auto gasolineTankPresentAsString = msdSettings->getValue("GASOLINE_TANK_PRESENT");
    bool gasolineTankPresentAsBool = atoi(gasolineTankPresentAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.gasolineTankPresent = gasolineTankPresentAsBool;

    // DIESEL_TANK_PRESENT
    auto dieselTankPresentAsString = msdSettings->getValue("DIESEL_TANK_PRESENT");
    bool dieselTankPresentAsBool = atoi(dieselTankPresentAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.dieselTankPresent = dieselTankPresentAsBool;

    // COMPRESSED_NATURALGAS
    auto compressedNaturalGasAsString = msdSettings->getValue("COMPRESSED_NATURALGAS");
    bool compressedNaturalGasAsBool = atoi(compressedNaturalGasAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.compressedNaturalGas = compressedNaturalGasAsBool;

    // LIQUID_PROPANE_GAS
    auto liquidPropaneGasAsString = msdSettings->getValue("LIQUID_PROPANE_GAS");
    bool liquidPropaneGasAsBool = atoi(liquidPropaneGasAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.liquidPropaneGas = liquidPropaneGasAsBool;

    // ELECTRIC_ENERGY_STORAGE
    auto electricEnergyStorageAsString = msdSettings->getValue("ELECTRIC_ENERGY_STORAGE");
    bool electricEnergyStorageAsAsBool = atoi(electricEnergyStorageAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.electricEnergyStorage = electricEnergyStorageAsAsBool;

    // HYDROGEN_STORAGE
    auto hydrogenStorageAsString = msdSettings->getValue("HYDROGEN_STORAGE");
    bool hydrogenStorageAsBool = atoi(hydrogenStorageAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.hydrogenStorage = hydrogenStorageAsBool;

    // OTHER_STORAGE
    auto otherStorageAsString = msdSettings->getValue("OTHER_STORAGE");
    bool otherStorageAsBool = atoi(otherStorageAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.otherStorage = otherStorageAsBool;

    // TIMESTAMP
    msdData_.timestamp = atoi(msdSettings->getValue("TIMESTAMP").c_str());

    // VEHICLE_POSITION_LATITUDE
    msdData_.vehicleLocation.positionLatitude
      = atoi(msdSettings->getValue("VEHICLE_POSITION_LATITUDE").c_str());

    // VEHICLE_POSITION_LONGITUDE
    msdData_.vehicleLocation.positionLongitude
      = atoi(msdSettings->getValue("VEHICLE_POSITION_LONGITUDE").c_str());

    // VEHICLE_DIRECTION
    msdData_.vehicleDirection = atoi(msdSettings->getValue("VEHICLE_DIRECTION").c_str());

    // RECENT_N1_LATITUDE_DELTA
    msdData_.recentVehicleLocationN1.latitudeDelta
      = atoi(msdSettings->getValue("RECENT_N1_LATITUDE_DELTA").c_str());

    // RECENT_N1_LONGITUDE_DELTA
    msdData_.recentVehicleLocationN1.longitudeDelta
      = atoi(msdSettings->getValue("RECENT_N1_LONGITUDE_DELTA").c_str());

    // RECENT_N2_LATITUDE_DELTA
    msdData_.recentVehicleLocationN2.latitudeDelta
      = atoi(msdSettings->getValue("RECENT_N2_LATITUDE_DELTA").c_str());

    // RECENT_N2_LONGITUDE_DELTA
    msdData_.recentVehicleLocationN2.longitudeDelta
      = atoi(msdSettings->getValue("RECENT_N2_LONGITUDE_DELTA").c_str());

    // NUMBER_OF_PASSENGERS
    msdData_.numberOfPassengers = atoi(msdSettings->getValue("NUMBER_OF_PASSENGERS").c_str());

    // OPTIONAL_ADDTIONAL_DATA OID
    msdData_.optionalPdu.oid = msdSettings->getValue("EUROPEAN_ECALL_OID");
    // OPTIONAL_ADDTIONAL_DATA OAD
    // If encoded string is available, directly append to the main MSD, otherwise
    // encode optional additional data content first and then append to the main MSD.
    if (!msdSettings->getValue("EUROPEAN_ECALL_OAD").empty()) {
        std::string str = msdSettings->getValue("EUROPEAN_ECALL_OAD");
        std::vector<uint8_t> data(str.begin(), str.end());
        msdData_.optionalPdu.data = data;
    } else {
        msdData_.optionalPdu.data = optionalAdditionalDataContent_;
   }
}
