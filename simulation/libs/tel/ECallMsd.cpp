/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <sstream>
#include <cstdint>
#include <iomanip>
#include <algorithm>

#include "ECallMsd.hpp"

#include "common/Logger.hpp"

#define MSD_VERSION_TWO 2
#define MSD_VERSION_THREE 3

#define MSD_PDU_SIZE 140  // in bytes
#define INVALID -1

#define ONE_BIT_FIELD 1
#define THREE_BIT_FIELD 3
#define FOUR_BIT_FIELD 4
#define FIVE_BIT_FIELD 5
#define SIX_BIT_FIELD 6
#define SEVEN_BIT_FIELD 7
#define EIGHT_BIT_FIELD 8
#define NINE_BIT_FIELD 9
#define TEN_BIT_FIELD 10
#define THIRTYTWO_BIT_FIELD 32

#define NO_OF_STORAGE_TYPE 127
#define RANGELIMIT_MIN 100
#define RANGELIMIT_MAX 255
#define DELTAV_MIN -255
#define DELTAV_MAX 255

#define MSD_PADDING 0

#define POSITION_CONVERSION 2147483648LL
#define N1N2_DELTA_CONVERSION 512

#define BYTE_SIZE 8
#define MAX_OAD_LENGTH \
    94  // in bytes, The total length of additional data concepts may not
        // exceed 94 bytes of data encoded in ASN.1 UPER.
        // version 2(<= 94), version 3(<= 94).

// Mandatory fields required for the PDU in MSD version-2
// ex: optional_flags (6) + msg_id (8) + control (8) + VIN (102) + vehicleStorage (15) +
// time stamp (32) + vehicle location (64) + vehicle direction (8) = 243 bits
#define MSD_VERSION_TWO_MANDATORY_FIELD_BITS 243
// Mandatory fields required for the PDU in MSD version-3
// ex: optional_flags (4) + msg_id (8) + control (8) + VIN (102) + vehicleStorage (15) +
// time stamp (32) + vehicle location (64) + vehicle direction (8) + recent vehicle location N1(20)
// + recent vehicle location N2(20) = 281 bits
#define MSD_VERSION_THREE_MANDATORY_FIELD_BITS 281

namespace telux {
namespace tel {
/**
 * Method to retrieve the singleton ECallMsd object.
 */
ECallMsd &ECallMsd::getInstance() {
    static ECallMsd instance;
    return instance;
}

ECallMsd::ECallMsd() {
}

ECallMsd::~ECallMsd() {
}

/**
 * LOGs the MSD values..
 */
void ECallMsd::logMsd(const ECallMsdData &eCallMsdData) {
    LOG(DEBUG, "msdVersion:", static_cast<int>(eCallMsdData.msdVersion));
    LOG(DEBUG, "optionalDataPresent:", eCallMsdData.optionals.optionalDataPresent);
    LOG(DEBUG,
        "recentVehicleLocationN1Present:", eCallMsdData.optionals.recentVehicleLocationN1Present);
    LOG(DEBUG,
        "recentVehicleLocationN2Present:", eCallMsdData.optionals.recentVehicleLocationN2Present);
    LOG(DEBUG, "numberOfPassengersPresent:", eCallMsdData.optionals.numberOfPassengersPresent);
    LOG(DEBUG, "messageIdentifier:", (int)eCallMsdData.messageIdentifier);
    // ECallMsdControlBits eCallMsdData.control.;
    LOG(DEBUG, "automaticActivation:", eCallMsdData.control.automaticActivation);
    LOG(DEBUG, "testCall:", eCallMsdData.control.testCall);
    LOG(DEBUG, "positionCanBeTrusted:", eCallMsdData.control.positionCanBeTrusted);
    LOG(DEBUG, "vehicleType:", (int)eCallMsdData.control.vehicleType);
    // VehicleIdentificationNumber eCallMsdData.vehicleIdentificationNumber;
    LOG(DEBUG, "isowmi:", eCallMsdData.vehicleIdentificationNumber.isowmi);
    LOG(DEBUG, "isovds:", eCallMsdData.vehicleIdentificationNumber.isovds);
    LOG(DEBUG, "isovisModelyear:", eCallMsdData.vehicleIdentificationNumber.isovisModelyear);
    LOG(DEBUG, "isovisSeqPlant :", eCallMsdData.vehicleIdentificationNumber.isovisSeqPlant);
    // VehiclePropulsionStorageType eCallMsdData.vehiclePropulsionStorage.;
    LOG(DEBUG, "gasolineTankPresent:", eCallMsdData.vehiclePropulsionStorage.gasolineTankPresent);
    LOG(DEBUG, "dieselTankPresent:", eCallMsdData.vehiclePropulsionStorage.dieselTankPresent);
    LOG(DEBUG, "compressedNaturalGas:", eCallMsdData.vehiclePropulsionStorage.compressedNaturalGas);
    LOG(DEBUG, "liquidPropaneGas:", eCallMsdData.vehiclePropulsionStorage.liquidPropaneGas);
    LOG(DEBUG,
        "electricEnergyStorage:", eCallMsdData.vehiclePropulsionStorage.electricEnergyStorage);
    LOG(DEBUG, "hydrogenStorage:", eCallMsdData.vehiclePropulsionStorage.hydrogenStorage);
    LOG(DEBUG, "otherStorage:", eCallMsdData.vehiclePropulsionStorage.otherStorage);
    LOG(DEBUG, "TimeStamp:", (int)eCallMsdData.timestamp);
    // VehicleLocation eCallMsdData.vehicleLocation.;
    LOG(DEBUG, "positionLatitude :", eCallMsdData.vehicleLocation.positionLatitude);
    LOG(DEBUG, "positionLongitude:", eCallMsdData.vehicleLocation.positionLongitude);
    LOG(DEBUG, "vehicleDirection:", (int)eCallMsdData.vehicleDirection);
    // VehicleLocationDelta eCallMsdData.recentVehicleLocationN1.;
    LOG(DEBUG, "latitudeDelta :", eCallMsdData.recentVehicleLocationN1.latitudeDelta);
    LOG(DEBUG, "longitudeDelta:", eCallMsdData.recentVehicleLocationN1.longitudeDelta);
    // VehicleLocationDelta eCallMsdData.recentVehicleLocationN2.;
    LOG(DEBUG, "latitudeDelta :", eCallMsdData.recentVehicleLocationN2.latitudeDelta);
    LOG(DEBUG, "longitudeDelta:", eCallMsdData.recentVehicleLocationN2.longitudeDelta);
    LOG(DEBUG, "numberOfPassengers:", (int)eCallMsdData.numberOfPassengers);
    // Optional Additional data
    LOG(DEBUG, "OID:", eCallMsdData.optionalPdu.oid);
    std::vector<uint8_t> data = eCallMsdData.optionalPdu.data;
    std::string dataString(data.begin(), data.end());
    LOG(DEBUG, "OAD:", dataString);
}

template <size_t bitsize>
void ECallMsd::writeMsdPdu(std::bitset<bitsize> b, std::string &pdu) {
    pdu.append(b.to_string());
}

int ECallMsd::calculateMsdMessageLength(const ECallMsdData &eCallMsdData, uint8_t msdVersion) {
    int msdBitsLength = 0;
    if (msdVersion == MSD_VERSION_TWO) {
        msdBitsLength = MSD_VERSION_TWO_MANDATORY_FIELD_BITS;
    } else if (msdVersion == MSD_VERSION_THREE) {
        msdBitsLength = MSD_VERSION_THREE_MANDATORY_FIELD_BITS;
    }
    // Check for optional flag and update msdBits
    bool isOptionalPresent = false;

    if (msdVersion == MSD_VERSION_TWO) {
        // Loc N1 present
        isOptionalPresent = eCallMsdData.optionals.recentVehicleLocationN1Present;
        if (isOptionalPresent) {
            msdBitsLength += TEN_BIT_FIELD;  // lat
            msdBitsLength += TEN_BIT_FIELD;  // long
        }
        // loc N2 present
        isOptionalPresent = eCallMsdData.optionals.recentVehicleLocationN2Present;
        if (isOptionalPresent) {
            msdBitsLength += TEN_BIT_FIELD;  // lat
            msdBitsLength += TEN_BIT_FIELD;  // long
        }
    }

    // No.of passengers
    isOptionalPresent = eCallMsdData.optionals.numberOfPassengersPresent;
    if (isOptionalPresent) {
        msdBitsLength += EIGHT_BIT_FIELD;
    }

    isOptionalPresent = eCallMsdData.optionals.optionalDataPresent;
    if (isOptionalPresent) {
        LOG(DEBUG, __FUNCTION__, " Optional data present");
        // Octet oid
        std::string oidString = eCallMsdData.optionalPdu.oid;
        if (!oidString.empty()) {
            msdBitsLength += EIGHT_BIT_FIELD;  // OID Content length store in byte
            int oidLen = getEncodedOIDLengthInBits(oidString);
            LOG(DEBUG, __FUNCTION__, " OID Content length in bits:", oidLen);
            if (oidLen <= 0) {
                LOG(ERROR, __FUNCTION__, " Invalid OID content");
                return INVALID;
            }
            msdBitsLength += oidLen;
        } else {
            LOG(ERROR, __FUNCTION__, " Invalid OID content");
            return INVALID;
        }
        std::vector<uint8_t> data = eCallMsdData.optionalPdu.data;
        std::string oadString(data.begin(), data.end());
        if (!oadString.empty()) {
            // OAD contains encoded PDU, consider 2 characters for 1 byte of data.
            if ((oadString.length() % 2) != 0) {
                LOG(ERROR, __FUNCTION__, " Invalid OAD content");
                return INVALID;
            }
            int length = oadString.length() / 2;
            if (length > MAX_OAD_LENGTH) {
                LOG(ERROR, __FUNCTION__, " Invalid OAD content for MSD version 2 or 3");
                return INVALID;
            } else {
                msdBitsLength += EIGHT_BIT_FIELD;  // Additional DATA Content length store in byte
                int oadLen = length * BYTE_SIZE;
                LOG(DEBUG, __FUNCTION__, " OAD Content length in bits:", oadLen);
                msdBitsLength += oadLen;
            }
        } else {
            LOG(ERROR, __FUNCTION__, " Invalid OAD content");
            return INVALID;
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " Optional data not present");
    }

    // Add trailing bits to make a byte boundary
    int leftOverBits = msdBitsLength % 8;
    if (leftOverBits != 0) {
        msdBitsLength = msdBitsLength + 8 - leftOverBits;
    }

    LOG(DEBUG, __FUNCTION__, " msdBitsLength = ", msdBitsLength,
        " in bytes = ", (msdBitsLength / 8));

    return msdBitsLength / 8;
}
/**
 * Write printable Pdu From String according to ISO 3779 specification for VIN
 * This function converts the VIN characters to printable string format.
 * ASN.1 encodes all the permitted VIN characters starting from '0'.
 * The acceptable characters for VIN are:
 *     "A"  .. "H" |"J"  .. "N" |"P" |"R"  .. "Z" |"0"  .. "9"
 *     ASN.1 will map the above characters to:
 *     0x0a .. 0x11|0x12 .. 0x16|0x17|0x18 .. 0x20|0x00 .. 0x09
 * @param input denotes user provided VIN string
 * @param pdu denotes the msd pdu being constructed
 * @param pduBitPosition denotes the bit position where the next bit needs to be
 * written
 * @returns pdu denotes the msd pdu constructed
 * @returns pduBitPosition denotes the updated bit position where the next bit
 * needs to be written
 */
void ECallMsd::writePrintableString(std::string input, std::string &msdPdu, bool optionalData) {
    // TODO: we have defines for these numbers : 48, 55, etc.
    for (unsigned int character = 0; character < input.length(); ++character) {
        uint8_t convertedChar = 0;
        if ((input[character] >= '0') && (input[character] <= '9')) {
            convertedChar = (input[character] - 48);
        } else if ((input[character] >= 'A') && (input[character] <= 'H')) {
            convertedChar = (input[character] - 55);
        } else if ((input[character] >= 'J') && (input[character] <= 'N')) {
            convertedChar = (input[character] - 56);
        } else if (input[character] == 'P') {
            convertedChar = (input[character] - 57);
        } else if ((input[character] >= 'R') && (input[character] <= 'Z')) {
            convertedChar = (input[character] - 58);
        } else {
            LOG(ERROR, "writePrintableString Unsupported Char: ", input[character]);
            convertedChar = 0x00;
        }
        LOG(DEBUG, "writePrintableString convertedChar: ", (int)convertedChar);
        if (optionalData) {
            writeMsdPdu(std::bitset<FOUR_BIT_FIELD>(convertedChar), msdPdu);
        } else {
            writeMsdPdu(std::bitset<SIX_BIT_FIELD>(convertedChar), msdPdu);
        }
    }
}

/**
 * Vehicle Identification Number
 */
void ECallMsd::writeVehicleIdentification(const ECallMsdData &eCallMsdData, std::string &msdPdu) {
    LOG(DEBUG, "eCallMsdData.vehicleIdentificationNumber.isowmi: ",
        eCallMsdData.vehicleIdentificationNumber.isowmi);
    writePrintableString(eCallMsdData.vehicleIdentificationNumber.isowmi, msdPdu, false);

    LOG(DEBUG, "eCallMsdData.vehicleIdentificationNumber.isovds: ",
        eCallMsdData.vehicleIdentificationNumber.isovds);
    if (eCallMsdData.vehicleIdentificationNumber.isovds.length() != 6) {
        LOG(ERROR, "Invalid isovds :", eCallMsdData.vehicleIdentificationNumber.isovds,
            "length: ", eCallMsdData.vehicleIdentificationNumber.isovds.length());
        LOG(ERROR,
            "Invalid isovds length :", eCallMsdData.vehicleIdentificationNumber.isovds.length());
        return;
    }

    writePrintableString(eCallMsdData.vehicleIdentificationNumber.isovds, msdPdu, false);

    if (eCallMsdData.vehicleIdentificationNumber.isovisModelyear.length() != 1) {
        LOG(ERROR,
            "Invalid isovisModelyear :", eCallMsdData.vehicleIdentificationNumber.isovisModelyear);
        return;
    }
    LOG(DEBUG, "eCallMsdData.vehicleIdentificationNumber.isovisModelyear: ",
        eCallMsdData.vehicleIdentificationNumber.isovisModelyear);
    writePrintableString(eCallMsdData.vehicleIdentificationNumber.isovisModelyear, msdPdu, false);

    if (eCallMsdData.vehicleIdentificationNumber.isovisSeqPlant.length() != 7) {
        LOG(ERROR,
            "Invalid isovisSeqPlant :", eCallMsdData.vehicleIdentificationNumber.isovisSeqPlant);
        return;
    }
    LOG(DEBUG, "eCallMsdData.vehicleIdentificationNumber.isovisSeqPlant: ",
        eCallMsdData.vehicleIdentificationNumber.isovisSeqPlant);
    writePrintableString(eCallMsdData.vehicleIdentificationNumber.isovisSeqPlant, msdPdu, false);
}

/**
 * Vehicle Propulsion Storage
 */
void ECallMsd::writeVehiclePropulsionStorage(
    const ECallMsdData &eCallMsdData, std::string &msdPdu) {
    LOG(DEBUG, __FUNCTION__);

    /* Extension Marker for the Sequence: VehiclePropulsionStorageType */
    /* Value of 0 to represent no extension additions */
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(0), msdPdu);

    // No of fuel storage types 7
    writeMsdPdu(std::bitset<SEVEN_BIT_FIELD>(NO_OF_STORAGE_TYPE), msdPdu);

    bool isPresent = false;

    // gasolineTankPresent
    isPresent = eCallMsdData.vehiclePropulsionStorage.gasolineTankPresent;
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isPresent ? 1 : 0), msdPdu);
    LOG(DEBUG, "eCallMsdData.vehiclePropulsionStorage.gasolineTankPresent:",
        eCallMsdData.vehiclePropulsionStorage.gasolineTankPresent);

    // dieselTankPresent
    isPresent = eCallMsdData.vehiclePropulsionStorage.dieselTankPresent;
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isPresent ? 1 : 0), msdPdu);
    LOG(DEBUG, "eCallMsdData.vehiclePropulsionStorage.dieselTankPresent:",
        eCallMsdData.vehiclePropulsionStorage.dieselTankPresent);

    // compressedNaturalGas
    isPresent = eCallMsdData.vehiclePropulsionStorage.compressedNaturalGas;
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isPresent ? 1 : 0), msdPdu);
    LOG(DEBUG, "eCallMsdData.vehiclePropulsionStorage.compressedNaturalGas:",
        eCallMsdData.vehiclePropulsionStorage.compressedNaturalGas);

    // liquidPropaneGas
    isPresent = eCallMsdData.vehiclePropulsionStorage.liquidPropaneGas;
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isPresent ? 1 : 0), msdPdu);
    LOG(DEBUG, "eCallMsdData.vehiclePropulsionStorage.liquidPropaneGas:",
        eCallMsdData.vehiclePropulsionStorage.liquidPropaneGas);

    // electricEnergyStorage
    isPresent = eCallMsdData.vehiclePropulsionStorage.electricEnergyStorage;
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isPresent ? 1 : 0), msdPdu);
    LOG(DEBUG, "eCallMsdData.vehiclePropulsionStorage.electricEnergyStorage:",
        eCallMsdData.vehiclePropulsionStorage.electricEnergyStorage);

    // hydrogenStorage
    isPresent = eCallMsdData.vehiclePropulsionStorage.hydrogenStorage;
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isPresent ? 1 : 0), msdPdu);
    LOG(DEBUG, "eCallMsdData.vehiclePropulsionStorage.hydrogenStorage:",
        eCallMsdData.vehiclePropulsionStorage.hydrogenStorage);

    // OTHER_STORAGE
    isPresent = eCallMsdData.vehiclePropulsionStorage.otherStorage;
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isPresent ? 1 : 0), msdPdu);
    LOG(DEBUG, "eCallMsdData.vehiclePropulsionStorage.otherStorage:",
        eCallMsdData.vehiclePropulsionStorage.otherStorage);
}

void ECallMsd::writeVehicleLocationDelta(
    const ECallMsdData &eCallMsdData, std::string &msdPdu, int16_t latitude, int16_t longitude) {
    writeMsdPdu(std::bitset<TEN_BIT_FIELD>(latitude + N1N2_DELTA_CONVERSION), msdPdu);
    writeMsdPdu(std::bitset<TEN_BIT_FIELD>(longitude + N1N2_DELTA_CONVERSION), msdPdu);
    LOG(DEBUG, "eCallMsdData.recentVehicleLocation ( N1 or N2) latitude: ", latitude,
        ", longitude: ", longitude);
}

telux::common::Status ECallMsd::writeOptionalAdditionalData(
    const ECallMsdData &eCallMsdData, std::string &msdPdu) {
    telux::common::Status status = telux::common::Status::FAILED;
    LOG(DEBUG, __FUNCTION__);
    status = writeOid(eCallMsdData, msdPdu);
    if (status != telux::common::Status::SUCCESS) {
        return status;
    }
    status = writeOptionalData(eCallMsdData, msdPdu);
    return status;
}

telux::common::Status ECallMsd::writeOid(const ECallMsdData &eCallMsdData, std::string &msdPdu) {
    std::vector<int> encodedOid;
    std::string oidString = eCallMsdData.optionalPdu.oid;
    if (!oidString.empty()) {
        std::vector<int> octets = convertOidToOctets(oidString);
        encodedOid              = uperEncodingForOctets(octets);
        LOG(DEBUG, __FUNCTION__, " OID length = ", encodedOid.size());
        if (encodedOid.size() <= 0) {
            LOG(ERROR, __FUNCTION__, " Invalid OID content");
            return telux::common::Status::INVALIDPARAM;
        }
        // Write OID size to pdu
        writeMsdPdu(std::bitset<EIGHT_BIT_FIELD>(encodedOid.size()), msdPdu);
        for (unsigned int index = 0; index < encodedOid.size(); index++) {
            LOG(DEBUG, __FUNCTION__, " encodedOid: ", encodedOid[index]);
            // Write OID content to PDU
            writeMsdPdu(std::bitset<EIGHT_BIT_FIELD>(encodedOid[index]), msdPdu);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Invalid OID content");
        return telux::common::Status::INVALIDPARAM;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * MSD optional data
 */
telux::common::Status ECallMsd::writeOptionalData(
    const ECallMsdData &eCallMsdData, std::string &msdPdu) {
    std::vector<uint8_t> data = eCallMsdData.optionalPdu.data;
    std::string oadString(data.begin(), data.end());
    if (!oadString.empty()) {
        if ((oadString.length() % 2) != 0) {
            LOG(ERROR, __FUNCTION__, " Invalid OAD content");
            return telux::common::Status::INVALIDPARAM;
        }
        int dataLen = oadString.length() / 2;
        if (dataLen > MAX_OAD_LENGTH) {
            LOG(ERROR, __FUNCTION__, " Invalid OAD content for MSD version 2 or 3");
            return telux::common::Status::INVALIDPARAM;
        } else {
            LOG(DEBUG, __FUNCTION__, " datalen = ", dataLen);
            // Additional DATA Content size write to pdu
            writeMsdPdu(std::bitset<EIGHT_BIT_FIELD>(dataLen), msdPdu);
            // OAD can contain printable character, convert to 4 bit binary and write to pdu.
            writePrintableString(oadString, msdPdu, true);
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " Optional additional data is not present");
        return telux::common::Status::INVALIDPARAM;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Generate MSD PDU and return the same
 * @param eCallMsdData is of type structure ECallMsdData containing data fields
 * required to compose MSD
 * @param pdu as string (max length of "MSD_PDU_SIZE(140)" bytes, returns null
 * string if any error
 * @returns Status of encoding
 * is detected)
 */
telux::common::Status ECallMsd::generateECallMsd(
    const ECallMsdData &eCallMsdData, std::vector<uint8_t> &pdu) {
    LOG(DEBUG, __FUNCTION__);
    std::string msdPdu = "";

    // Decoding logic is based on MSD version provided in eCallMsdData.msdVersion field
    // Supports MSD Version-2: CEN 15722 2015 and MSD Version-3: CEN 15722 2020
    uint8_t msdVersion = eCallMsdData.msdVersion;
    if ((msdVersion != MSD_VERSION_TWO) && (msdVersion != MSD_VERSION_THREE)) {
        LOG(ERROR, __FUNCTION__, " Unsupported msdVersion: ", (int)msdVersion);
        return telux::common::Status::INVALIDPARAM;
    }
    writeMsdPdu(std::bitset<EIGHT_BIT_FIELD>(msdVersion), msdPdu);

    int pduSize = calculateMsdMessageLength(eCallMsdData, msdVersion);
    LOG(DEBUG, __FUNCTION__, " PDU Size = ", pduSize);

    if ((msdVersion == MSD_VERSION_TWO)
        && (pduSize < (MSD_VERSION_TWO_MANDATORY_FIELD_BITS / BYTE_SIZE))) {
        return telux::common::Status::FAILED;
    } else if ((msdVersion == MSD_VERSION_THREE)
               && (pduSize < (MSD_VERSION_THREE_MANDATORY_FIELD_BITS / BYTE_SIZE))) {
        return telux::common::Status::FAILED;
    }
    writeMsdPdu(std::bitset<EIGHT_BIT_FIELD>(pduSize), msdPdu);

    // OPTIONALS - START
    // Extension Marker or flag for the Sequence: MSDMessage
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(0), msdPdu);
    // OPTIONAL optionalAdditionalData.Present
    bool isOptionalDataPresent = eCallMsdData.optionals.optionalDataPresent;
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isOptionalDataPresent ? 1 : 0), msdPdu);

    // Extension Marker for the Sequence: MSDStructure
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(0), msdPdu);

    // N1 present
    bool isN1LocPresent = eCallMsdData.optionals.recentVehicleLocationN1Present;
    // N2 present
    bool isN2LocPresent = eCallMsdData.optionals.recentVehicleLocationN2Present;
    // recentVehicleLocationN1 and recentVehicleLocationN2 are optional fields only in MSD
    // version-2. These are mandatory fields in MSD version-3.
    if (msdVersion == MSD_VERSION_TWO) {
        writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isN1LocPresent ? 1 : 0), msdPdu);
        writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isN2LocPresent ? 1 : 0), msdPdu);
    } else if (!isN1LocPresent || !isN2LocPresent) {
        LOG(ERROR, __FUNCTION__, " isN1LocPresent(", isN1LocPresent, ") or isN2LocPresent(",
            isN2LocPresent, ") are not SET for MSDv3");
        return telux::common::Status::INVALIDPARAM;
    }
    // No.of passengers
    bool isNoPassengerPresent = eCallMsdData.optionals.numberOfPassengersPresent;
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(isNoPassengerPresent ? 1 : 0), msdPdu);
    // OPTIONALS - END

    // message identifier
    writeMsdPdu(std::bitset<EIGHT_BIT_FIELD>(eCallMsdData.messageIdentifier), msdPdu);

    // Mandatory fields CONTROL
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(eCallMsdData.control.automaticActivation), msdPdu);

    // test call
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(eCallMsdData.control.testCall), msdPdu);

    // Position Can Be Trusted
    writeMsdPdu(std::bitset<ONE_BIT_FIELD>(eCallMsdData.control.positionCanBeTrusted), msdPdu);

    // Extension Marker for vehicleType
    if (msdVersion == MSD_VERSION_THREE) {
        writeMsdPdu(std::bitset<ONE_BIT_FIELD>(0), msdPdu);
    }

    // eCall_vehicletype
    writeMsdPdu(std::bitset<FIVE_BIT_FIELD>(eCallMsdData.control.vehicleType), msdPdu);
    // Vehicle Identification Number
    writeVehicleIdentification(eCallMsdData, msdPdu);

    // Vehicle Propulsion Storage
    writeVehiclePropulsionStorage(eCallMsdData, msdPdu);

    // time stamp
    writeMsdPdu(std::bitset<THIRTYTWO_BIT_FIELD>(eCallMsdData.timestamp), msdPdu);

    // vehicleLocation + POSITION_CONVERSION
    writeMsdPdu(std::bitset<THIRTYTWO_BIT_FIELD>(
                    eCallMsdData.vehicleLocation.positionLatitude + POSITION_CONVERSION),
        msdPdu);
    writeMsdPdu(std::bitset<THIRTYTWO_BIT_FIELD>(
                    eCallMsdData.vehicleLocation.positionLongitude + POSITION_CONVERSION),
        msdPdu);

    // vehicleDirection
    writeMsdPdu(std::bitset<EIGHT_BIT_FIELD>(eCallMsdData.vehicleDirection), msdPdu);

    // N1
    if (isN1LocPresent || (msdVersion == MSD_VERSION_THREE)) {
        writeVehicleLocationDelta(eCallMsdData, msdPdu,
            eCallMsdData.recentVehicleLocationN1.latitudeDelta,
            eCallMsdData.recentVehicleLocationN1.longitudeDelta);
    }

    // N2
    if (isN2LocPresent || (msdVersion == MSD_VERSION_THREE)) {
        writeVehicleLocationDelta(eCallMsdData, msdPdu,
            eCallMsdData.recentVehicleLocationN2.latitudeDelta,
            eCallMsdData.recentVehicleLocationN2.longitudeDelta);
    }

    // # of passengers
    if (isNoPassengerPresent) {
        writeMsdPdu(std::bitset<EIGHT_BIT_FIELD>(eCallMsdData.numberOfPassengers), msdPdu);
    }

    // Optional additional data
    if (isOptionalDataPresent) {
        telux::common::Status status = writeOptionalAdditionalData(eCallMsdData, msdPdu);
        if (status != telux::common::Status::SUCCESS) {
            return status;
        }
    }

    // set trailing bits to zero
    while (msdPdu.length() % 8 != 0) {
        writeMsdPdu(std::bitset<ONE_BIT_FIELD>(MSD_PADDING), msdPdu);
    }

    // Clear the provided input vector
    pdu.clear();
    // Convert bit representation into vector of uints
    for (unsigned int i = 0; i < msdPdu.length(); i = i + 8) {
        std::string sstr = msdPdu.substr(i, 8);
        uint8_t val      = std::stoi(sstr, nullptr, 2);
        pdu.push_back(val);
    }

    std::stringstream ss;
    for (auto i : pdu) {
        ss << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << (int)i;
    }
    LOG(DEBUG, __FUNCTION__, " PDU Hex String = ", ss.str());
    if ((ss.str().length() / 2) > MSD_PDU_SIZE) {
        LOG(DEBUG, " ECall MSD pdu should not exceed 140 bytes");
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

/*
 * Get encoded OID length in bits.
 */
int ECallMsd::getEncodedOIDLengthInBits(std::string oidString) {
    std::vector<int> octets     = convertOidToOctets(oidString);
    std::vector<int> encodedOid = uperEncodingForOctets(octets);
    int oidLengthInBits         = (encodedOid.size() * BYTE_SIZE);
    LOG(DEBUG, __FUNCTION__, " Oid length in bits, ", oidLengthInBits);
    return oidLengthInBits;
}

/**
 * Convert OID string to octets.
 */
std::vector<int> ECallMsd::convertOidToOctets(std::string oid) {
    std::vector<int> oidOctets;
    std::istringstream stream(oid);
    int oidOctet;
    while (stream >> oidOctet) {
        oidOctets.emplace_back(oidOctet);
        if (stream.peek() == '.') {
            stream.ignore();
        }
    }
    return oidOctets;
}

/**
 * Returns encoded data corresponding to OID octets.
 */
std::vector<int> ECallMsd::uperEncodingForOctets(std::vector<int> octetOids) {
    std::vector<int> encodedOctets;
    std::vector<int> tempOctets;
    for (unsigned int index = 0; index < octetOids.size(); index++) {
        tempOctets = processOctet(octetOids[index]);
        encodedOctets.insert(encodedOctets.end(), tempOctets.begin(), tempOctets.end());
        tempOctets.clear();
    }
    for (unsigned int index = 0; index < encodedOctets.size(); ++index) {
        LOG(DEBUG, __FUNCTION__, " encodedOctets: ", encodedOctets[index]);
    }
    return encodedOctets;
}

/**
 * Returns vector after applying UPER encoding where octet value is less than or equal to 127
 * are pushed to cached encoded vector and values greater than 127 are encoded into multiple bytes
 * and pushed to cached encoded vector.
 */
std::vector<int> ECallMsd::processOctet(int oidValue) {
    std::vector<int> encodedOctets;
    if (oidValue <= 127) {
        encodedOctets.emplace_back(oidValue);
    } else {
        encodedOctets = encodingToMultiByte(oidValue);
    }
    return encodedOctets;
}

/**
 * Encode OID octet to multi byte using uper encoding if octet value is > 127.
 */
std::vector<int> ECallMsd::encodingToMultiByte(int octet) {
    std::vector<int> encodedOctets;
    std::stringstream stream;
    stream << std::hex << octet;
    std::string result(stream.str());
    LOG(DEBUG, __FUNCTION__, " Input Hex String: ", std::hex, result);
    int counter = 0;
    // In case of length with odd value, increment counter by 1 for encoding 2 bytes at a time
    if (result.length() % 2 == 0) {
        counter = result.length() / 2;
    } else {
        counter = result.length() / 2 + 1;
    }
    int temp       = octet;
    int bitToCheck = 7;
    // bitToCheck is used depending on counter value, for counter greater than 3
    // bittocheck to decrement not otherwise
    for (int i = 1; i < counter; i++) {
        if (counter > 2) {
            temp = temp >> 8;
            bitToCheck--;
        } else {
            temp       = temp >> 8;
            bitToCheck = 7;
            break;
        }
    }
    LOG(DEBUG, __FUNCTION__, " bitToCheck: ", bitToCheck);
    // Iterate from leftmost bit till bitToCheck to see if any bit is 1,
    // if yes increment the counter as another byte gets added during left shifting
    for (int bit = 7; bit >= bitToCheck; bit--) {
        int flag = ((temp >> bit) & 0x01);
        LOG(DEBUG, __FUNCTION__, " flag: ", flag);
        if (flag == 1) {
            // Increment the counter if flag is set
            counter = counter + 1;
            break;
        }
    }

    temp = octet;  // number which will contain the result
    LOG(DEBUG, __FUNCTION__, " temp: ", temp);
    int leftMostBit = 0;
    for (int i = 0; i < counter; i++) {
        // Right most byte is encoded, where leftmost bit is ignored
        if (i == 0) {
            int out = temp & 0x7f;  // Ignore left most bit and push the value to vector
            encodedOctets.emplace_back(out);
            leftMostBit = ((temp >> 7) & 0x01);  // Preserve left most bit of the value for use
                                                 // with further encoding
            LOG(DEBUG, __FUNCTION__, " leftMostBit ", leftMostBit);
            LOG(DEBUG, __FUNCTION__, " temp: ", temp);
        } else {
            // Encoding of left most bytes
            temp = temp >> 8;  // temp holds left most byte of the oid value
            LOG(DEBUG, __FUNCTION__, " temp: ", temp);
            int left = temp;
            left     = left << 1;  // Bits left shift by 1
            LOG(DEBUG, __FUNCTION__, " left: ", left);
            temp = left;
            left = left | 0x80;  // Setting left most bit as 1
            left = left | leftMostBit;  // left most bit in above if condition was 1,
                                        // Set left most bit as 1
            LOG(DEBUG, __FUNCTION__, " leftMostBit: ", leftMostBit);
            left = left & 0xff;  // separate out right most part i.e if value was 38D, left after
                                 // this operation would be 8D
            LOG(DEBUG, __FUNCTION__, " left: ", left);
            encodedOctets.push_back(left);
            leftMostBit = ((temp >> 7) & 0x01);  // preserve leftmost bit of byte from line 415
            LOG(DEBUG, __FUNCTION__, " temp leftMostBit: ", leftMostBit);
        }
    }

    std::reverse(encodedOctets.begin(), encodedOctets.end());
    for (unsigned int i = 0; i < encodedOctets.size(); i++) {
        LOG(DEBUG, __FUNCTION__, " encodedOctets ", encodedOctets[i]);
    }
    return encodedOctets;
}

uint16_t ECallMsd::encodeOneByteField(
    uint16_t bitOffset, uint16_t noOfBits, uint8_t *dataField, uint8_t *encodedData) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<uint8_t> oneByteBitMask({0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01});
    uint8_t bitOffsetPos = bitOffset & 0x07;
    uint8_t bitPos       = 8 - ((noOfBits & 0x07) ? noOfBits & 0x07 : (noOfBits & 0x07) + 8);
    encodedData += bitOffset >> 3;
    for (uint16_t i = 0; i < noOfBits; i++) {
        uint8_t val  = (*dataField) & oneByteBitMask[bitPos];
        uint8_t mask = oneByteBitMask[bitPos];
        int8_t shift = bitOffsetPos - bitPos;
        if (shift >= 0) {
            val >>= shift;
            mask >>= shift;
        } else {
            val <<= (-shift);
            mask <<= (-shift);
        }

        *encodedData &= ~mask;
        *encodedData |= val;
        bitPos++;
        bitOffsetPos++;
        if (bitPos > 7) {
            dataField++;
            bitPos = 0;
        }
        if (bitOffsetPos > 7) {
            encodedData++;
            bitOffsetPos = 0;
        }
    }
    return bitOffset + noOfBits;
}

uint16_t ECallMsd::encodeTwoBytesField(
    uint16_t bitOffset, uint16_t noOfBits, uint16_t *dataField, uint8_t *encodedData) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t bitOffsetCurr = bitOffset;
    std::vector<uint16_t> twoBytesBitMask({0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x100,
        0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000});
    for (uint16_t i = 0; i < noOfBits; i++) {
        if ((*dataField & twoBytesBitMask[noOfBits - i - 1]) != 0) {
            encodedData[bitOffsetCurr >> 3] |= 0x01 << (7 - (bitOffsetCurr & 0x07));
        } else {
            encodedData[bitOffsetCurr >> 3] &= ~(0x01 << (7 - (bitOffsetCurr & 0x07)));
        }
        bitOffsetCurr++;
    }
    return bitOffset + noOfBits;
}

/* Euro NCAP Optional additional data content encoding
A.2.2 ASN.1 definition
MSD_ADDITIIONAL_EURONCAP_1
DEFINITIONS
AUTOMATIC TAGS ::=
BEGIN
-- Definition can be used to decode data in the data part
-- of optionalAdditionalData in the MSD message.
--
-- AdditionalData::= SEQUENCE {
-- oid RELATIVE-OID,
-- data OCTET STRING (CONTAINING INCINFO)
-- }
INCINFO ::= SEQUENCE {
 locationOfImpact IILocations,
 rolloverDetected BOOLEAN OPTIONAL,
 deltaV IIDeltaV,
 ...
}
IIDeltaV ::= SEQUENCE {
 rangeLimit INTEGER(100..255),
 deltaVX INTEGER(-255..255),
 deltaVY INTEGER(-255..255),
 ...
}
IILocations ::= ENUMERATED {
 unknown(0),
 none(1),
 front(2),
 rear(3),
 driverSide(4),
 nonDriverSide(5),
 other(6),
TB 040
16
 ...
}
END

Encoding of overall optional additional data.
1. First 8 bits: length of UPER encoded relative-oid.
2. Next 16 bits: 0801  // oid RELATIVE-OID "8.1"
3. Next 8 bits : length of  UPER encoded OCTET STRING (CONTAINING INCINFO) (will be byte aligned)
3.a. 1 bit for INCINFO extension marker (zero for now)
3.b. 1 bit for rollOverDetected OPTIONAL  (set if rolloverDetected to be included)
3.c. 1 bit for IILocations extnesion marker
3.d. 3 bits for enum IILocations
3.e. 1 bit rollOverDetected
3.f. 8 bits for rangeLimit (subtract 100 from the actual value)
3.g. 9 bits for deltaVX (cap to -255..255 and add 255)
3.h. 9 bits for deltaVY (cap to -255..255 and add 255) */
telux::common::Status ECallMsd::encodeEuroNcapOptionalAdditionalDataContent(
    ECallOptionalEuroNcapData optionalEuroNcapData, std::vector<uint8_t> &data) {
    LOG(DEBUG, __FUNCTION__);
    auto status                                  = telux::common::Status::FAILED;
    uint8_t extension_flag                       = 0;
    int offset                                   = 0;
    uint16_t msdOptionalAdditionalDataContentLen = 0;
    uint8_t encodedData[MAX_OAD_LENGTH];
    memset(encodedData, 0, sizeof(encodedData));
    if (encodedData) {
        // Extension Marker for the Sequence: INCINFO
        offset = encodeOneByteField(offset, ONE_BIT_FIELD, &extension_flag, encodedData);
        // rollOverDetected optional flag
        offset = encodeOneByteField(offset, ONE_BIT_FIELD,
            (uint8_t *)&optionalEuroNcapData.rollOverDetectedPresent, encodedData);
        // Extension Marker for the Enumerated: ILocations
        offset = encodeOneByteField(offset, ONE_BIT_FIELD, &extension_flag, encodedData);
        // location of impact
        int locationOfImpact = static_cast<int>(optionalEuroNcapData.locationOfImpact);
        if (locationOfImpact < static_cast<int>(ECallLocationOfImpact::UNKNOWN)
            || locationOfImpact > static_cast<int>(ECallLocationOfImpact::OTHER)) {
            LOG(ERROR, __FUNCTION__, " invalid location of impact ", locationOfImpact);
            status = telux::common::Status::INVALIDPARAM;
            return status;
        }
        offset = encodeOneByteField(
            offset, THREE_BIT_FIELD, (uint8_t *)&locationOfImpact, encodedData);
        // rollOverDetected
        if (optionalEuroNcapData.rollOverDetectedPresent) {
            offset = encodeOneByteField(offset, ONE_BIT_FIELD,
                (uint8_t *)&optionalEuroNcapData.rollOverDetected, encodedData);
        }
        // delta-v range limit
        int rangeLimit = static_cast<int>(optionalEuroNcapData.deltaV.rangeLimit);
        if (rangeLimit < RANGELIMIT_MIN || rangeLimit > RANGELIMIT_MAX) {
            LOG(ERROR, __FUNCTION__, " invalid rangelimit = ", rangeLimit);
            status = telux::common::Status::INVALIDPARAM;
            return status;
        }
        rangeLimit = rangeLimit - RANGELIMIT_MIN;
        // delta-v deltaVX
        int16_t deltaVX = optionalEuroNcapData.deltaV.deltaVX;
        if (deltaVX < DELTAV_MIN || deltaVX > DELTAV_MAX) {
            LOG(ERROR, __FUNCTION__, " invalid deltaVX = ", deltaVX);
            status = telux::common::Status::INVALIDPARAM;
            return status;
        }
        deltaVX = deltaVX - DELTAV_MIN;
        // delta-v deltaVY
        int16_t deltaVY = optionalEuroNcapData.deltaV.deltaVY;
        if (deltaVY < DELTAV_MIN || deltaVY > DELTAV_MAX) {
            LOG(ERROR, __FUNCTION__, " invalid deltaVY = ", deltaVY);
            status = telux::common::Status::INVALIDPARAM;
            return status;
        }
        deltaVY = deltaVY - DELTAV_MIN;
        // Extension Marker for the Sequence: IDeltaV
        offset = encodeOneByteField(offset, ONE_BIT_FIELD, &extension_flag, encodedData);
        offset = encodeOneByteField(offset, EIGHT_BIT_FIELD, (uint8_t *)&rangeLimit, encodedData);
        offset = encodeTwoBytesField(offset, NINE_BIT_FIELD, (uint16_t *)&deltaVX, encodedData);
        offset = encodeTwoBytesField(offset, NINE_BIT_FIELD, (uint16_t *)&deltaVY, encodedData);

        if (offset % BYTE_SIZE) {
            msdOptionalAdditionalDataContentLen = (offset / BYTE_SIZE) + 1;
        } else {
            msdOptionalAdditionalDataContentLen = (offset / BYTE_SIZE);
        }
        LOG(INFO, __FUNCTION__,
            " MSD optional additional data content length = ", msdOptionalAdditionalDataContentLen,
            " bytes for ", offset, " bits");
        std::string oadDataString;
        for (int i = 0; i < static_cast<int>(msdOptionalAdditionalDataContentLen); i++) {
            char s1 = char(encodedData[i] >> 4);
            char s2 = char(encodedData[i] & 0xf);
            // if character is greater than 1 byte, add ascii value of 7(55),
            // otherwise ascii value of 0(48) is added to character.
            s1 > 9 ? s1 += 55 : s1 += 48;
            s2 > 9 ? s2 += 55 : s2 += 48;
            // append single character at the end
            oadDataString.append(1, s1);
            oadDataString.append(1, s2);
        }
        LOG(DEBUG, " Euro NCAP MSD OAD data content = ", oadDataString.c_str());
        std::vector<uint8_t> encodedBytes(oadDataString.begin(), oadDataString.end());
        // copy encoded data
        data   = encodedBytes;
        status = telux::common::Status::SUCCESS;
    } else {
        status = telux::common::Status::FAILED;
    }
    return status;
}

}  // namespace tel
}  // namespace telux
