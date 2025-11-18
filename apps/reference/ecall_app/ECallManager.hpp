/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 *
 * Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ECALLMANAGER_HPP
#define ECALLMANAGER_HPP

#include <telux/tel/ECallDefines.hpp>

#include "TelClient.hpp"
#include "LocationClient.hpp"
#include "AudioClient.hpp"
#include "ThermClient.hpp"
#include "MsdProvider.hpp"
#include "ConfigParser.hpp"

class ECallManager : public LocationListener,
                     public CallStatusListener,
                     public std::enable_shared_from_this<ECallManager> {
 public:
    /**
     * Initialize necessary Telematics-SDK components like location, audio, etc. and and get
     * required parameters from the configuration file
     */
    telux::common::Status init();

    /**
     * This function triggers the standard eCall procedure(eg.112)
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     * @param [in] category     ECallCategory
     * @param [in] transmitMsd  Configures MSD transmission at MO call connect
     * @param [in] variant      ECallVariant
     * @param [in] msdPdu       MSD PDU that will be transmitted at call connect. If this is empty,
     *                          either the MSD as per configuration file or the default MSD will
     *                          be used.
     *
     * @returns Status of triggerECall i.e success or suitable status code.
     */
    telux::common::Status triggerECall(
        int phoneId, ECallCategory category, ECallVariant variant, bool transmitMsd,
        std::vector<uint8_t> msdPdu);

    /**
     * This function triggers a voice eCall procedure to the specified phone number
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     * @param [in] category     ECallCategory
     * @param [in] transmitMsd  Configures MSD transmission at MO call connect
     * @param [in] dialNumber   phone number to be dialed
     * @param [in] msdPdu       MSD PDU that will be transmitted at call connect. If this is empty,
     *                          either the MSD as per configuration file or the default MSD will
     *                          be used.
     *
     * @returns Status of triggerECall i.e success or suitable status code.
     */
    telux::common::Status triggerECall(
        int phoneId, ECallCategory category, const std::string dialNumber, bool transmitMsd,
        std::vector<uint8_t> msdPdu);

    /**
     * This function triggers a voice eCall procedure to the specified phone number over IMS
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is
     *                          performed
     * @param [in] dialNumber   phone number to be dialed
     * @param [in] contentType  Optional content type for SIP request
     * @param [in] acceptInfo   Optional accept type for SIP request
     *
     */
    telux::common::Status triggerECall(
        int phoneId, const std::string dialNumber, std::string contentType, std::string acceptInfo);

    /**
     * This function triggers a self test ERA-GLONASS eCall procedure to the specified phone number
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is
     *                          performed
     * @param [in] dialNumber   Phone number to be dialed
     * @param [in] msdPdu       MSD PDU that will be transmitted at call connect.
     *
     */
    telux::common::Status triggerECall(
        int phoneId, const std::string dialNumber, std::vector<uint8_t> msdPdu);

    /**
     * This function answers an incoming call
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     *
     * @returns Status of answerCall i.e success or suitable status code.
     *
     */
    telux::common::Status answerCall(int phoneId);

    /**
     * This function is called to update eCall MSD for Tps eCall over IMS
     *
     */
    telux::common::Status updateEcallMSD();

    /**
     * This function hangs up an ongoing call dialed/answered previously
     *
     * @param [in] phoneId     Represents phone corresponding to which the operation is performed
     * @param [in] callIndex   Represents the call on which the operation is performed
     *
     * @returns Status of hangupCall i.e success or suitable status code.
     *
     */
    telux::common::Status hangupCall(int phoneId, int callIndex);

    /**
     * Dump the list of calls in progress
     *
     * @returns Status of getCalls i.e success or suitable status code.
     */
    telux::common::Status getCalls();

    /**
     * This function requests status of various eCall HLAP timers
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     *
     * @returns Status of requestHlapTimerStatus i.e success or suitable status code.
     *
     */
    telux::common::Status requestHlapTimerStatus(int phoneId);

    /**
     * This function requests to stop T10 eCall High Level Application Protocol(HLAP) timer
     *
     * @param [in] phoneId   Represents phone corresponding to which eCall operation is performed
     *
     * @returns Status of stopT10Timer i.e success or suitable status code.
     *
     */
    telux::common::Status stopT10Timer(int phoneId);

    /**
     * This function requests to set the value of eCall High Level Application Protocol(HLAP)
     * timer
     *
     * @param [in] phoneId       Represents phone corresponding to which eCall operation is
     *                           performed
     * @param [in] type          @ref HlapTimerType
     * @param [in] timeDuration  Represents the time duration.
     *
     * @returns Status of setHlapTimer i.e success or suitable status code.
     *
     */
    telux::common::Status setHlapTimer(int phoneId, HlapTimerType type, uint32_t timeDuration);

    /**
     * This function requests to get the value of eCall High Level Application Protocol(HLAP)
     * timer
     *
     * @param [in] phoneId   Represents phone corresponding to which eCall operation is performed
     * @param [in] type      @ref HlapTimerType
     *
     * @returns Status of getHlapTimer i.e success or suitable status code.
     *
     */
    telux::common::Status getHlapTimer(int phoneId, HlapTimerType type);

    /**
     * Get various configuration parameters related to eCall
     *
     * @returns Status of getECallConfig i.e success or suitable status code.
     *
     */
    telux::common::Status getECallConfig();

    /**
     * Set various configuration parameters related to eCall
     *
     * @param [in] config configuration to be written
     *
     * @returns Status of setECallConfig i.e success or suitable status code.
     *
     */
    telux::common::Status setECallConfig(EcallConfig config);

    /**
     * Restart eCall High Level Application Protocol (HLAP) timer for residual timer duration.
     *
     * @param [in] phoneId     Represents phone corresponding to which eCall operation is performed
     * @param [in] id          Timer ID
     * @param [in] duration    Time gap between two successive redial attempts
     *
     * @returns Status for restartECallHlapTimer i.e success or suitable status code.
     *
     */
    telux::common::Status restartECallHlapTimer(int phoneId, EcallHlapTimerId id, int duration);

    /**
     * Gets encoded optional additional data content for eCall MSD.
     *
     * @returns Status of  getEncodedOptionalAdditionalDataContent i.e success or suitable
     * status code.
     *
     */
    telux::common::Status getEncodedOptionalAdditionalDataContent();

    /**
     * Gets encoded eCall MSD payload.
     *
     * @returns Error code for getECallMsdPayload i.e success or suitable
     * status code.
     *
     */
    telux::common::ErrorCode getECallMsdPayload();

    /**
     * Configure eCall redial parameters for call origination failure or call drop
     *
     * @returns status code for configureECallRedial i.e success or suitable
     * status code.
     *
     */
    telux::common::Status configureECallRedial(int config, std::vector<int> &timeGap);

    /**
     * Get eCall redial parameters for call origination failure and call drop
     *
     * @returns error code code for getECallRedialConfig i.e success or suitable
     * error code.
     *
     */
    telux::common::ErrorCode getECallRedialConfig();

    /**
     * This function requests to set the value of POST TEST REGISTRATION timer.
     *
     * @returns status code for setPostTestRegistrationTimer i.e success or suitable
     * status code.
     *
     */
    telux::common::Status setPostTestRegistrationTimer(int phoneId, uint32_t timeDuration);

    /**
     * This function requests to get the value of POST TEST REGISTRATION timer.
     *
     * @returns error code for getECallPostTestRegistrationTimer i.e success or suitable
     * error code.
     *
     */
    telux::common::ErrorCode getECallPostTestRegistrationTimer(int phoneId);

    void onLocationUpdate(ECallLocationInfo locInfo) override;
    void onCallDisconnect() override;
    void onCallConnect(int phoneId) override;

    ECallManager();
    ~ECallManager();

 private:
    /**
     * This function updates the cached MSD data stored in Modem
     *
     * @param [in] phoneId  Represents phone corresponding to which the operation will be performed
     *
     * @returns Status of updateMSD i.e success or suitable status code.
     *
     */
    telux::common::Status updateMSD(int phoneId);

    /**
     * This function enables necessary functionalities in various subsystems(location, audio, etc.),
     * that are required for an eCall
     *
     * @param [in] phoneId  Represents phone corresponding to which the operation will be performed
     *
     */
    void setup(int phoneId);

    /**
     * This function disables the functionalities in various subsystems(location, audio, etc.)
     * Typically performed when an eCall ends
     */
    void cleanup();

    /**
     * This function indicates if atleast one location fix is received after the eCall is triggered.
     * Useful in creating the MSD with valid location information.
     */
    bool isLocationReceived();
    void setLocationReceived(bool state);

    /**
     * Function to parse the settings from the eCall configuration file and fetch the static MSD
     * Data
     */
    void parseAppConfig();

    /** Member variables to hold Manager objects of various Telematics-SDK components */
    std::shared_ptr<TelClient> telClient_;
    std::shared_ptr<LocationClient> locClient_;
    std::shared_ptr<AudioClient> audioClient_;
    std::shared_ptr<ThermClient> thermClient_;

    /** Represents the phone corresponding to the eCall session */
    int phoneId_;
    /** Local copy of MSD data structure that will be used in transmission */
    ECallMsdData msdData_;
    /** Local copy of MSD optional additional data content. */
    ECallOptionalEuroNcapData optionalAdditionalDataContent_;
    /** Local copy of MSD raw PDU that will be used in transmission */
    std::vector<uint8_t> msdPdu_ {};
    /** Interval for which the location-fix updates needs to be received */
    uint32_t locUpdateIntervalMs_;
    std::mutex mutex_;
    bool locFixReceived_;
    std::condition_variable locUpdateCV_;
    /** Variables to store audio settings for eCall voice conversation */
    std::vector<DeviceType> audioDevices_ {DeviceType::DEVICE_TYPE_SPEAKER,
                     DeviceType::DEVICE_TYPE_MIC};
    uint32_t voiceSampleRate_;
    AudioFormat voiceFormat_;
    ChannelTypeMask voiceChannels_;
    EcnrMode ecnrMode_;
    bool isTpsEcallOverImsTriggered;    //To check if Tps eCall over IMS is triggered
};

#endif  // ECALLMANAGER_HPP
