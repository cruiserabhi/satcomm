/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ECALLMSD_HPP
#define ECALLMSD_HPP

#include <bitset>
#include <vector>

#include <telux/tel/ECallDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

/**
 * ECallMsd class - An utility class with helper functions required for ECall MSD creation
 */
class ECallMsd {

 public:
    /**
     * Method to retrieve the singleton ECallMsd object.
     */
    static ECallMsd &getInstance();
    /**
     * Logs the eCallMsd fields.
     */
    void logMsd(const ECallMsdData &eCallMsdData);

    /**
     * Generate MSD PDU and provide the same
     */
    telux::common::Status generateECallMsd(const ECallMsdData &msdData, std::vector<uint8_t> &pdu);

    /**
     * Gets encoded optional additional data content for ecall MSD.
     */
    telux::common::Status encodeEuroNcapOptionalAdditionalDataContent(
        ECallOptionalEuroNcapData optionalEuroNcapData, std::vector<uint8_t> &data);

 private:
    /**
     * Constructor of ECallMsd. Opens file stream for logging.
     */
    ECallMsd();
    /**
     * Default destructor of ECallMsd class. Closes open file stream.
     */
    ~ECallMsd();
    /**
     * Singleton implementation, copy constructor and assignment operator are disabled.
     */
    ECallMsd(const ECallMsd &)            = delete;
    ECallMsd &operator=(const ECallMsd &) = delete;

    ECallMsdData eCallMsdData_;

    /**
     * Vehicle Identification Number
     */
    void writeVehicleIdentification(const ECallMsdData &eCallMsdData, std::string &msdPdu);
    /**
     * Vehicle Propulsion Storage
     */
    void writeVehiclePropulsionStorage(const ECallMsdData &eCallMsdData, std::string &msdPdu);
    /**
     * Vehicle Location Delta N1 and N2
     */
    void writeVehicleLocationDelta(
        const ECallMsdData &eCallMsdData, std::string &msdPdu, int16_t latitude, int16_t longitude);

    /**
     * MSD optional data
     */
    telux::common::Status writeOptionalData(const ECallMsdData &eCallMsdData, std::string &msdPdu);
    /**
     * Generate 6Bit Pdu From String according to ISO 3779 specification for VIN
     * and 4 bit Pdu from encoded optional additional data String.
     * This function converts the VIN or optional additional data content characters
     * to printable string format.
     */
    void writePrintableString(std::string input, std::string &pdu, bool optionalData = false);
    /**
     * MSD optional object identifier.
     * Generate 8 bit Pdu from String by using UPER encoding and write into bitset.
     */
    telux::common::Status writeOid(const ECallMsdData &eCallMsdData, std::string &msdPdu);
    /**
     * MSD optional additional data.
     */
    telux::common::Status writeOptionalAdditionalData(
        const ECallMsdData &eCallMsdData, std::string &msdPdu);
    /**
     * Writes PDU string serially
     */
    template <size_t bitsize>
    void writeMsdPdu(std::bitset<bitsize> b, std::string &pdu);

    int calculateMsdMessageLength(const ECallMsdData &eCallMsdData, uint8_t msdVersion);

    int calculateAdditionalDataLength(const ECallMsdData &eCallMsdData);

    int getEncodedOIDLengthInBits(std::string oid);

    std::vector<int> convertOidToOctets(std::string oid);
    /**
     * Returns encoded data corresponding to OID octets.
     */
    std::vector<int> uperEncodingForOctets(std::vector<int> octetOid);
    /**
     * Returns vector after applying UPER encoding where octet value is less than or equal to 127
     * are pushed to cached encoded vector and values greater than 127 are encoded into multiple
     * bytes and pushed to cached encoded vector.
     */
    std::vector<int> processOctet(int oidValue);
    /**
     * Encode OID octet to multi byte using uper encoding if octet value is > 127.
     */
    std::vector<int> encodingToMultiByte(int octet);
    /**
     * Encodes one byte MSD optional additional data content.
     */
    uint16_t encodeOneByteField(
        uint16_t bitOffset, uint16_t noOfBits, uint8_t *dataField, uint8_t *encodedData);
    /**
     * Encodes two bytes MSD optional additional data content.
     */
    uint16_t encodeTwoBytesField(
        uint16_t bitOffset, uint16_t noOfBits, uint16_t *dataField, uint8_t *encodedData);
};

}  // End of namespace tel

}  // End of namespace telux

#endif  // ECallMsd_HPP
