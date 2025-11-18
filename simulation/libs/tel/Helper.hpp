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
 * @file       Helper.hpp
 *
 * @brief
 *
 */

#ifndef HELPER_HPP
#define HELPER_HPP

#include <telux/tel/SmsManager.hpp>
#include <telux/tel/PhoneDefines.hpp>

enum TelEventType {
    UNKNOWN,
    SMS_MEMORY_FULL,
    SMS_INCOMING
};

enum CallApi {
    makeECallWithMsd = 0,
    makeTpsECallOverCSWithMsd = 1,
    makeTpsECallOverIMS = 2,
    makeECallWithRawMsd = 3,
    makeTpsECallOverCSWithRawMsd = 4,
    makeECallWithoutMsd = 5,
    makeTpsECallOverCSWithoutMsd = 6,
    updateEcallMsd = 7,
    updateECallRawMsd = 8,
    makeVoiceCall = 9,
    makeRttVoiceCall = 10
};

class Helper  {
public:
    static telux::tel::SmsTagType getTagType(std::string tagType ) {
        if(tagType == "MT_READ") {
            return telux::tel::SmsTagType::MT_READ;
        } else if (tagType == "MT_NOT_READ") {
            return telux::tel::SmsTagType::MT_NOT_READ;
        } else {
            return telux::tel::SmsTagType::UNKNOWN;
        }
    }

    static telux::tel::SmsEncoding getencodingMethod(std::string encoding ) {
        if(encoding == "GSM7") {
            return telux::tel::SmsEncoding::GSM7;
        } else if (encoding == "GSM8") {
            return telux::tel::SmsEncoding::GSM8;
        } else if (encoding == "UCS2") {
            return telux::tel::SmsEncoding::UCS2;
        } else {
            return telux::tel::SmsEncoding::UNKNOWN;
        }
    }

    static std::string tagTypeToString(telux::tel::SmsTagType type) {
        switch(type) {
            case telux::tel::SmsTagType::MT_READ:
                return "MT_READ";
            case telux::tel::SmsTagType::MT_NOT_READ:
                return "MT_NOT_READ";
            case telux::tel::SmsTagType::UNKNOWN:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    static telux::tel::StorageType getstorageType(std::string storageType ) {
        if(storageType == "SIM") {
            return telux::tel::StorageType::SIM;
        } else if (storageType == "NONE") {
            return telux::tel::StorageType::NONE;
        } else {
            return telux::tel::StorageType::UNKNOWN;
        }
    }

    static std::string storageTypeToString(telux::tel::StorageType type) {
        switch(type) {
            case telux::tel::StorageType::SIM:
                return "SIM";
            case telux::tel::StorageType::NONE:
                return "NONE";
            case telux::tel::StorageType::UNKNOWN:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    static std::string encodingToString(telux::tel::SmsEncoding encoding) {
        switch(encoding) {
            case telux::tel::SmsEncoding::GSM7:
                return "GSM7";
            case telux::tel::SmsEncoding::GSM8:
                return "GSM8";
            case telux::tel::SmsEncoding::UCS2:
                return "UCS2";
            case telux::tel::SmsEncoding::UNKNOWN:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    static telux::tel::CallState getCallState(std::string callState ) {
        if(callState == "CALL_IDLE") {
            return telux::tel::CallState::CALL_IDLE;
        } else if (callState == "CALL_ACTIVE") {
            return telux::tel::CallState::CALL_ACTIVE;
        } else if (callState == "CALL_HOLD") {
            return telux::tel::CallState::CALL_ON_HOLD;
        } else if (callState == "CALL_DIALING") {
            return telux::tel::CallState::CALL_DIALING;
        } else if (callState == "CALL_INCOMING") {
            return telux::tel::CallState::CALL_INCOMING;
        } else if (callState == "CALL_WAITING") {
            return telux::tel::CallState::CALL_WAITING;
        } else if (callState == "CALL_ALERTING") {
            return telux::tel::CallState::CALL_ALERTING;
        } else if (callState == "CALL_ENDED") {
            return telux::tel::CallState::CALL_ENDED;
        } else {
            return telux::tel::CallState::CALL_IDLE;
        }
    }

    static std::string getCallStateInString(telux::tel::CallState callState) {
        if(callState == telux::tel::CallState::CALL_IDLE) {
            return "CALL_IDLE";
        } else if (callState == telux::tel::CallState::CALL_ACTIVE) {
            return "CALL_ACTIVE";
        } else if (callState == telux::tel::CallState::CALL_ON_HOLD) {
            return "CALL_HOLD";
        } else if (callState == telux::tel::CallState::CALL_DIALING) {
            return "CALL_DIALING";
        } else if (callState == telux::tel::CallState::CALL_INCOMING) {
            return "CALL_INCOMING";
        } else if (callState == telux::tel::CallState::CALL_WAITING) {
            return "CALL_WAITING";
        } else if (callState == telux::tel::CallState::CALL_ALERTING) {
            return "CALL_ALERTING";
        } else if (callState == telux::tel::CallState::CALL_ENDED) {
            return "CALL_ENDED";
        } else {
            return "CALL_IDLE";
        }
    }
};

#endif // HELPER_HPP