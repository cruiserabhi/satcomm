/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "MsdSettings.hpp"

std::map<std::string, std::string> MsdSettings::msdSettingsMap_;
std::string MsdSettings::filename_;

/**
 * Reads MSD data from file and caches it
 */
telux::tel::ECallMsdData MsdSettings::readMsdFromFile(std::string filename) {
   msdSettingsMap_.clear();

   filename_ = filename;
   telux::tel::ECallMsdData msdData;

   MsdSettings::readMsdSettingsFile();

   std::cout << " ECall MSD Version: " << static_cast<int>(msdData.msdVersion) << std::endl;

   // RECENT_LOCATION_N1_PRESENT
   auto recentVehicleLocationN1PresentAsString
      = MsdSettings::getValue("RECENT_LOCATION_N1_PRESENT");
   bool recentVehicleLocationN1PresentAsBool
      = atoi(recentVehicleLocationN1PresentAsString.c_str()) ? true : false;
   msdData.optionals.recentVehicleLocationN1Present = recentVehicleLocationN1PresentAsBool;

   // RECENT_LOCATION_N2_PRESENT
   auto recentVehicleLocationN2PresentAsString
      = MsdSettings::getValue("RECENT_LOCATION_N2_PRESENT");
   bool recentVehicleLocationN2PresentAsBool
      = atoi(recentVehicleLocationN2PresentAsString.c_str()) ? true : false;
   msdData.optionals.recentVehicleLocationN2Present = recentVehicleLocationN2PresentAsBool;

   // NUMBER_OF_PASSENGERS_PRESENT
   auto numberOfPassengersPresentAsString = MsdSettings::getValue("NUMBER_OF_PASSENGERS_PRESENT");
   bool numberOfPassengersPresentAsBool
      = atoi(numberOfPassengersPresentAsString.c_str()) ? true : false;
   msdData.optionals.numberOfPassengersPresent = numberOfPassengersPresentAsBool;

   // OPTIONAL_ADDITIONAL_DATA_PRESENT
   auto optionalAdditionalDataPresentAsString =
       MsdSettings::getValue("OPTIONAL_ADDITIONAL_DATA_PRESENT");
   bool optionalAdditionalDataPresentAsBool
       = atoi(optionalAdditionalDataPresentAsString.c_str()) ? true : false;
   msdData.optionals.optionalDataPresent = optionalAdditionalDataPresentAsBool;

   // MESSAGE_IDENTIFIER
   msdData.messageIdentifier = atoi(MsdSettings::getValue("MESSAGE_IDENTIFIER").c_str());

   // AUTOMATIC_ACTIVATION
   auto automaticActivationtAsString = MsdSettings::getValue("AUTOMATIC_ACTIVATION");
   bool automaticActivationAsBool = atoi(automaticActivationtAsString.c_str()) ? true : false;
   msdData.control.automaticActivation = automaticActivationAsBool;

   // TEST_CALL
   auto testCallAsString = MsdSettings::getValue("TEST_CALL");
   bool testCallAsBool = atoi(testCallAsString.c_str()) ? true : false;
   msdData.control.testCall = testCallAsBool;

   // POSITION_CAN_BE_TRUSTED
   auto positionCanBeTrustedAsString = MsdSettings::getValue("POSITION_CAN_BE_TRUSTED");
   bool positionCanBeTrustedAsBool = atoi(positionCanBeTrustedAsString.c_str()) ? true : false;
   msdData.control.positionCanBeTrusted = positionCanBeTrustedAsBool;

   // VEHICLE_TYPE
   auto vehicleTypeAsString = MsdSettings::getValue("VEHICLE_TYPE");
   msdData.control.vehicleType
      = static_cast<telux::tel::ECallVehicleType>(atoi(vehicleTypeAsString.c_str()));

   // ISO_WMI
   msdData.vehicleIdentificationNumber.isowmi = MsdSettings::getValue("ISO_WMI");

   // ISO_VDS
   msdData.vehicleIdentificationNumber.isovds = MsdSettings::getValue("ISO_VDS");

   // ISO_VIS_MODEL_YEAR
   msdData.vehicleIdentificationNumber.isovisModelyear
      = MsdSettings::getValue("ISO_VIS_MODEL_YEAR");

   // ISO_VIS_SEQ_PLANT
   msdData.vehicleIdentificationNumber.isovisSeqPlant = MsdSettings::getValue("ISO_VIS_SEQ_PLANT");

   // GASOLINE_TANK_PRESENT
   auto gasolineTankPresentAsString = MsdSettings::getValue("GASOLINE_TANK_PRESENT");
   bool gasolineTankPresentAsBool = atoi(gasolineTankPresentAsString.c_str()) ? true : false;
   msdData.vehiclePropulsionStorage.gasolineTankPresent = gasolineTankPresentAsBool;

   // DIESEL_TANK_PRESENT
   auto dieselTankPresentAsString = MsdSettings::getValue("DIESEL_TANK_PRESENT");
   bool dieselTankPresentAsBool = atoi(dieselTankPresentAsString.c_str()) ? true : false;
   msdData.vehiclePropulsionStorage.dieselTankPresent = dieselTankPresentAsBool;

   // COMPRESSED_NATURALGAS
   auto compressedNaturalGasAsString = MsdSettings::getValue("COMPRESSED_NATURALGAS");
   bool compressedNaturalGasAsBool = atoi(compressedNaturalGasAsString.c_str()) ? true : false;
   msdData.vehiclePropulsionStorage.compressedNaturalGas = compressedNaturalGasAsBool;

   // LIQUID_PROPANE_GAS
   auto liquidPropaneGasAsString = MsdSettings::getValue("LIQUID_PROPANE_GAS");
   bool liquidPropaneGasAsBool = atoi(liquidPropaneGasAsString.c_str()) ? true : false;
   msdData.vehiclePropulsionStorage.liquidPropaneGas = liquidPropaneGasAsBool;

   // ELECTRIC_ENERGY_STORAGE
   auto electricEnergyStorageAsString = MsdSettings::getValue("ELECTRIC_ENERGY_STORAGE");
   bool electricEnergyStorageAsAsBool = atoi(electricEnergyStorageAsString.c_str()) ? true : false;
   msdData.vehiclePropulsionStorage.electricEnergyStorage = electricEnergyStorageAsAsBool;

   // HYDROGEN_STORAGE
   auto hydrogenStorageAsString = MsdSettings::getValue("HYDROGEN_STORAGE");
   bool hydrogenStorageAsBool = atoi(hydrogenStorageAsString.c_str()) ? true : false;
   msdData.vehiclePropulsionStorage.hydrogenStorage = hydrogenStorageAsBool;

   // OTHER_STORAGE
   auto otherStorageAsString = MsdSettings::getValue("OTHER_STORAGE");
   bool otherStorageAsBool = atoi(otherStorageAsString.c_str()) ? true : false;
   msdData.vehiclePropulsionStorage.otherStorage = otherStorageAsBool;

   // TIMESTAMP
   msdData.timestamp = atoi(MsdSettings::getValue("TIMESTAMP").c_str());

   // VEHICLE_POSITION_LATITUDE
   msdData.vehicleLocation.positionLatitude
      = atoi(MsdSettings::getValue("VEHICLE_POSITION_LATITUDE").c_str());

   // VEHICLE_POSITION_LONGITUDE
   msdData.vehicleLocation.positionLongitude
      = atoi(MsdSettings::getValue("VEHICLE_POSITION_LONGITUDE").c_str());

   // VEHICLE_DIRECTION
   msdData.vehicleDirection = atoi(MsdSettings::getValue("VEHICLE_DIRECTION").c_str());

   // RECENT_N1_LATITUDE_DELTA
   msdData.recentVehicleLocationN1.latitudeDelta
      = atoi(MsdSettings::getValue("RECENT_N1_LATITUDE_DELTA").c_str());

   // RECENT_N1_LONGITUDE_DELTA
   msdData.recentVehicleLocationN1.longitudeDelta
      = atoi(MsdSettings::getValue("RECENT_N1_LONGITUDE_DELTA").c_str());

   // RECENT_N2_LATITUDE_DELTA
   msdData.recentVehicleLocationN2.latitudeDelta
      = atoi(MsdSettings::getValue("RECENT_N2_LATITUDE_DELTA").c_str());

   // RECENT_N2_LONGITUDE_DELTA
   msdData.recentVehicleLocationN2.longitudeDelta
      = atoi(MsdSettings::getValue("RECENT_N2_LONGITUDE_DELTA").c_str());

   // NUMBER_OF_PASSENGERS
   msdData.numberOfPassengers = atoi(MsdSettings::getValue("NUMBER_OF_PASSENGERS").c_str());

   // OPTIONAL_ADDTIONAL_DATA OID
   msdData.optionalPdu.oid = MsdSettings::getValue("EUROPEAN_ECALL_OID");
   // OPTIONAL_ADDTIONAL_DATA OAD
   // If already encoded optional additional data content is available, just append to the
   // main MSD, otherwise encode optional additional data content first and then append to the
   // main MSD.
   if (!MsdSettings::getValue("EUROPEAN_ECALL_OAD").empty()) {
       std::string str = MsdSettings::getValue("EUROPEAN_ECALL_OAD");
       std::vector<uint8_t> data(str.begin(), str.end());
       msdData.optionalPdu.data = data;
   } else {
       msdData.optionalPdu.data = getOptionalAdditionalDataContent();
   }
   return msdData;
}

telux::tel::ECallOptionalEuroNcapData MsdSettings::readEuroNcapOptionalAdditionalDataContent(
    std::string filename) {
    msdSettingsMap_.clear();

    filename_ = filename;
    MsdSettings::readMsdSettingsFile();
    telux::tel::ECallOptionalEuroNcapData optionalEuroNcapData;
    // Euro NCAP OPTIONAL_ADDTIONAL_DATA_CONTENT
    auto locationOfImpactAsString = MsdSettings::getValue("EURONCAP_LOCATION_OF_IMPACT");
    optionalEuroNcapData.locationOfImpact =
        static_cast<telux::tel::ECallLocationOfImpact>(atoi(locationOfImpactAsString.c_str()));
    auto rollOverDetectedPresentAsString =
        MsdSettings::getValue("EURONCAP_ROLL_OVER_DETECTED_PRESENT");
    bool rollOverDetectedPresentAsBool
        = atoi(rollOverDetectedPresentAsString.c_str()) ? true : false;
    optionalEuroNcapData.rollOverDetectedPresent =
        rollOverDetectedPresentAsBool;
    optionalEuroNcapData.rollOverDetected =
        atoi(MsdSettings::getValue("EURONCAP_ROLL_OVER_DETECTED").c_str());
    optionalEuroNcapData.deltaV.rangeLimit =
        atoi(MsdSettings::getValue("EURONCAP_DELTAV_RANGELIMIT").c_str());
    optionalEuroNcapData.deltaV.deltaVX =
        atoi(MsdSettings::getValue("EURONCAP_DELTAV_DELTAVX").c_str());
    optionalEuroNcapData.deltaV.deltaVY =
        atoi(MsdSettings::getValue("EURONCAP_DELTAV_DELTAVY").c_str());
    return optionalEuroNcapData;
}

void MsdSettings::setOptionalAdditionalDataContent(
    std::vector<uint8_t> optionalAdditionalDataContent) {
    encodedOptionalAdditionalDataContent_ = optionalAdditionalDataContent;
}

std::vector<uint8_t> MsdSettings::getOptionalAdditionalDataContent() {
    return encodedOptionalAdditionalDataContent_;
}

/*
 * Get the user defined value for any configuration msdSetting
 */
std::string MsdSettings::getValue(std::string key) {
   auto msdSettingsIterator = msdSettingsMap_.find(key);
   if(msdSettingsIterator != msdSettingsMap_.end()) {
      return msdSettingsIterator->second;
   } else {
      return std::string("");  // return an empty string when the setting is not configured.
   }
}

/**
 * Utility function to read config file with key value pairs
 * Prepares a map of key value pairs from Key=Value format
 * Discards leading spaces, blank lines and lines starting with #
 * Removes any leading or training spaces around Key and Value if any
 */
void MsdSettings::readMsdSettingsFile() {
   // Create a file stream from the file name
   std::ifstream configFileStream(filename_);
   if(!configFileStream.is_open()) {
       std::cout << " MSD settings file(" << filename_ << ") is not found" << std::endl;
       return;
   }

   // Iterate through each parameter in the file and read the key value pairs
   std::string param;
   while(std::getline(configFileStream >> std::ws, param)) {
      std::string key;
      std::istringstream paramStream(param);
      if(std::getline(paramStream, key, '=')) {
         // Ignore lines starting with # character
         if(key[0] == '#') {
            continue;
         }
         key = std::regex_replace(key, std::regex(" +$"), "");
         if(key.length() > 0) {
            std::string value;
            if(std::getline(paramStream, value)) {
               value = std::regex_replace(value, std::regex("^ +| +$"), "");
               msdSettingsMap_[key] = value;
            }
         }
      }
   }
}

/*
 * Prints the contents of the map. i.e. all configured msdSettings
 */
void MsdSettings::printMsdSettings() {
   for(auto &iter : msdSettingsMap_) {
      std::cout << "Key: " << iter.first << ",     "
                << "Value: " << iter.second << std::endl;
   }
}
