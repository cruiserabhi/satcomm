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
 * Copyright (c) 2022, 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file    TelClientUtils.cpp
 *
 * @brief   TelClientUtils class provides helper functions to convert various parameters to strings.
 */

#include <iostream>
#include <sstream>

#include "TelClientUtils.hpp"

#define CLIENT_NAME "ECall-Tel-Utils: "

std::string TelClientUtils::callDirectionToString(telux::tel::CallDirection cd) {
    switch(cd) {
        case telux::tel::CallDirection::INCOMING:
            return std::string("Incoming call");
        case telux::tel::CallDirection::OUTGOING:
            return std::string("Outgoing call");
        case telux::tel::CallDirection::NONE:
            return std::string("None");
        default:
            std::cout << CLIENT_NAME << "Unexpected call direction = " << (int)cd << std::endl;
            return std::string("unknown");
    }
}

std::string TelClientUtils::callStateToString(telux::tel::CallState cs) {
    switch(cs) {
        case telux::tel::CallState::CALL_IDLE:
            return std::string("Idle call");
        case telux::tel::CallState::CALL_ACTIVE:
            return std::string("Active call");
        case telux::tel::CallState::CALL_ON_HOLD:
            return std::string("On hold call");
        case telux::tel::CallState::CALL_DIALING:
            return std::string("Outgoing call");
        case telux::tel::CallState::CALL_INCOMING:
            return std::string("Incoming call");
        case telux::tel::CallState::CALL_WAITING:
            return std::string("Waiting call");
        case telux::tel::CallState::CALL_ALERTING:
            return std::string("Alerting call");
        case telux::tel::CallState::CALL_ENDED:
            return std::string("Call ended");
        default:
            std::cout << CLIENT_NAME << "Unexpected CallState = " << (int)cs << std::endl;
            return std::string("unknown");
    }
}

std::string TelClientUtils::eCallMsdTransmissionStatusToString(
                    telux::tel::ECallMsdTransmissionStatus status) {
    switch(status) {
        case telux::tel::ECallMsdTransmissionStatus::SUCCESS:
            return std::string("SUCCESS");
        case telux::tel::ECallMsdTransmissionStatus::FAILURE:
            return std::string("FAILURE");
        case telux::tel::ECallMsdTransmissionStatus::MSD_TRANSMISSION_STARTED:
            return std::string("MSD TRANSMISSION STARTED");
        case telux::tel::ECallMsdTransmissionStatus::NACK_OUT_OF_ORDER:
            return std::string("NACK OUT OF ORDER");
        case telux::tel::ECallMsdTransmissionStatus::ACK_OUT_OF_ORDER:
            return std::string("ACK OUT OF ORDER");
        case telux::tel::ECallMsdTransmissionStatus::START_RECEIVED:
            return std::string("SEND-MSD(START) RECEIVED");
        case telux::tel::ECallMsdTransmissionStatus::LL_ACK_RECEIVED:
            return std::string("LL-ACK RECEIVED");
        case telux::tel::ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_STARTED:
            return std::string("OUTBAND MSD TRANSMISSION STARTED");
        case telux::tel::ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_SUCCESS:
            return std::string("OUTBAND MSD TRANSMISSION SUCCESS");
        case telux::tel::ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_FAILURE:
            return std::string("OUTBAND MSD TRANSMISSION FAILURE");
        case telux::tel::ECallMsdTransmissionStatus::LL_NACK_DUE_TO_T7_EXPIRY:
            return std::string("LL_NACK_DUE_TO_T7_EXPIRY");
        case telux::tel::ECallMsdTransmissionStatus::MSD_AL_ACK_CLEARDOWN:
            return std::string("MSD_AL_ACK_CLEARDOWN");
        default:
            std::stringstream ss;
            ss << "Unknown ECallMsdTransmissionStatus  = " << (int)status;
            return ss.str();
    }
}

std::string TelClientUtils::callEndCauseToString(telux::tel::CallEndCause callEndCause) {
    switch(callEndCause) {
        case telux::tel::CallEndCause::UNOBTAINABLE_NUMBER:
            return std::string("Unobtainable number");
        case telux::tel::CallEndCause::NO_ROUTE_TO_DESTINATION:
            return std::string("No route to destination");
        case telux::tel::CallEndCause::CHANNEL_UNACCEPTABLE:
            return std::string("Channel unacceptable");
        case telux::tel::CallEndCause::OPERATOR_DETERMINED_BARRING:
            return std::string("Operator determined barring");
        case telux::tel::CallEndCause::NORMAL:
            return std::string("Normal");
        case telux::tel::CallEndCause::BUSY:
        case telux::tel::CallEndCause::USER_BUSY:
        case telux::tel::CallEndCause::SIP_BUSY:
            return std::string("Busy");
        case telux::tel::CallEndCause::NO_USER_RESPONDING:
            return std::string("No user responding");
        case telux::tel::CallEndCause::NO_ANSWER_FROM_USER:
            return std::string("No answer from user");
        case telux::tel::CallEndCause::NOT_REACHABLE:
        case telux::tel::CallEndCause::SIP_NOT_REACHABLE:
            return std::string("Not reachable");
        case telux::tel::CallEndCause::CALL_REJECTED:
        case telux::tel::CallEndCause::USER_REJECT:
        case telux::tel::CallEndCause::SIP_USER_REJECTED:
        case telux::tel::CallEndCause::SIP_REQUEST_CANCELLED:
            return std::string("Call rejected");
        case telux::tel::CallEndCause::NUMBER_CHANGED:
            return std::string("Number changed");
        case telux::tel::CallEndCause::PREEMPTION:
            return std::string("Preemption");
        case telux::tel::CallEndCause::DESTINATION_OUT_OF_ORDER:
            return std::string("Destination out of order");
        case telux::tel::CallEndCause::INVALID_NUMBER_FORMAT:
            return std::string("Invalid number format");
        case telux::tel::CallEndCause::FACILITY_REJECTED:
            return std::string("Facility rejected");
        case telux::tel::CallEndCause::RESP_TO_STATUS_ENQUIRY:
            return std::string("Resp to status enquiry");
        case telux::tel::CallEndCause::NORMAL_UNSPECIFIED:
            return std::string("Normal unspecified");
        case telux::tel::CallEndCause::CONGESTION:
            return std::string("Congestion");
        case telux::tel::CallEndCause::NETWORK_OUT_OF_ORDER:
            return std::string("Network out of order");
        case telux::tel::CallEndCause::TEMPORARY_FAILURE:
            return std::string("Temporary failure");
        case telux::tel::CallEndCause::SWITCHING_EQUIPMENT_CONGESTION:
            return std::string("Switching equipment congestion");
        case telux::tel::CallEndCause::ACCESS_INFORMATION_DISCARDED:
            return std::string("Access information discarded");
        case telux::tel::CallEndCause::REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE:
            return std::string("Requested circuit or channel not available");
        case telux::tel::CallEndCause::RESOURCES_UNAVAILABLE_OR_UNSPECIFIED:
            return std::string("Resources unavailable or unspecified");
        case telux::tel::CallEndCause::QOS_UNAVAILABLE:
            return std::string("QOS unavailable");
        case telux::tel::CallEndCause::REQUESTED_FACILITY_NOT_SUBSCRIBED:
            return std::string("Requested facility not subscribed");
        case telux::tel::CallEndCause::INCOMING_CALLS_BARRED_WITHIN_CUG:
            return std::string("Incoming calls barred within CUG");
        case telux::tel::CallEndCause::BEARER_CAPABILITY_NOT_AUTHORIZED:
            return std::string("Bearer capability not authorized");
        case telux::tel::CallEndCause::BEARER_CAPABILITY_UNAVAILABLE:
            return std::string("Bearer capability unavailable");
        case telux::tel::CallEndCause::SERVICE_OPTION_NOT_AVAILABLE:
            return std::string("Service option not available");
        case telux::tel::CallEndCause::BEARER_SERVICE_NOT_IMPLEMENTED:
            return std::string("Bearer service not implemented");
        case telux::tel::CallEndCause::ACM_LIMIT_EXCEEDED:
            return std::string("Acm limit exceeded");
        case telux::tel::CallEndCause::REQUESTED_FACILITY_NOT_IMPLEMENTED:
            return std::string("Requested facility not implemented");
        case telux::tel::CallEndCause::ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE:
            return std::string("Only digital information bearer availablE");
        case telux::tel::CallEndCause::SERVICE_OR_OPTION_NOT_IMPLEMENTED:
            return std::string("Service or option not implemented");
        case telux::tel::CallEndCause::INVALID_TRANSACTION_IDENTIFIER:
            return std::string("Invalid transaction identifier");
        case telux::tel::CallEndCause::USER_NOT_MEMBER_OF_CUG:
            return std::string("User not member of CUG");
        case telux::tel::CallEndCause::INCOMPATIBLE_DESTINATION:
            return std::string("Incompatible destination");
        case telux::tel::CallEndCause::INVALID_TRANSIT_NW_SELECTION:
            return std::string("Invalid transit nw selection");
        case telux::tel::CallEndCause::SEMANTICALLY_INCORRECT_MESSAGE:
            return std::string("Semantically incorrect message");
        case telux::tel::CallEndCause::INVALID_MANDATORY_INFORMATION:
            return std::string("Invalid mandatory information");
        case telux::tel::CallEndCause::MESSAGE_TYPE_NON_IMPLEMENTED:
            return std::string("Message type non implemented");
        case telux::tel::CallEndCause::MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE:
            return std::string("Message type not compatible with protocol state");
        case telux::tel::CallEndCause::INFORMATION_ELEMENT_NON_EXISTENT:
            return std::string("Information element non existent");
        case telux::tel::CallEndCause::CONDITIONAL_IE_ERROR:
            return std::string("Conditional ie error");
        case telux::tel::CallEndCause::MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE:
            return std::string("Message not compatible with protocol state");
        case telux::tel::CallEndCause::RECOVERY_ON_TIMER_EXPIRED:
            return std::string("Recovery on timer expired");
        case telux::tel::CallEndCause::PROTOCOL_ERROR_UNSPECIFIED:
            return std::string("Protocol error unspecified");
        case telux::tel::CallEndCause::INTERWORKING_UNSPECIFIED:
            return std::string("Interworking unspecified");
        case telux::tel::CallEndCause::CALL_BARRED:
            return std::string("Call barred");
        case telux::tel::CallEndCause::FDN_BLOCKED:
            return std::string("FDN blocked");
        case telux::tel::CallEndCause::IMSI_UNKNOWN_IN_VLR:
            return std::string("IMSI unknown in VLR");
        case telux::tel::CallEndCause::IMEI_NOT_ACCEPTED:
            return std::string("IMEI not accepted");
        case telux::tel::CallEndCause::DIAL_MODIFIED_TO_USSD:
            return std::string("Dial modified to USSD");
        case telux::tel::CallEndCause::DIAL_MODIFIED_TO_SS:
            return std::string("Dial modified to SS");
        case telux::tel::CallEndCause::DIAL_MODIFIED_TO_DIAL:
            return std::string("Dial modified to dial");
      case telux::tel::CallEndCause::RADIO_OFF:
         return std::string("Radio off");
      case telux::tel::CallEndCause::OUT_OF_SERVICE:
         return std::string("Out of service");
      case telux::tel::CallEndCause::NO_VALID_SIM:
         return std::string("No valid sim");
      case telux::tel::CallEndCause::RADIO_INTERNAL_ERROR:
         return std::string("Radio internal error");
      case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT:
         return std::string("Network response timeout");
      case telux::tel::CallEndCause::NETWORK_REJECT:
         return std::string("Network reject");
      case telux::tel::CallEndCause::RADIO_ACCESS_FAILURE:
         return std::string("Radio access failure");
      case telux::tel::CallEndCause::RADIO_LINK_FAILURE:
         return std::string("Radio link failure");
      case telux::tel::CallEndCause::RADIO_LINK_LOST:
         return std::string("Radio link lost");
      case telux::tel::CallEndCause::RADIO_UPLINK_FAILURE:
         return std::string("Radio uplink failure");
      case telux::tel::CallEndCause::RADIO_SETUP_FAILURE:
         return std::string("Radio setup failure");
      case telux::tel::CallEndCause::RADIO_RELEASE_NORMAL:
         return std::string("Radio release normal");
      case telux::tel::CallEndCause::RADIO_RELEASE_ABNORMAL:
         return std::string("Radio release abnormal");
      case telux::tel::CallEndCause::ACCESS_CLASS_BLOCKED:
         return std::string("Access class barring");
      case telux::tel::CallEndCause::NETWORK_DETACH:
         return std::string("Network detach");
      case telux::tel::CallEndCause::CDMA_LOCKED_UNTIL_POWER_CYCLE:
         return std::string("CDMA locked until power cycle");
      case telux::tel::CallEndCause::CDMA_DROP:
         return std::string("CDMA drop");
      case telux::tel::CallEndCause::CDMA_INTERCEPT:
         return std::string("CDMA intercept");
      case telux::tel::CallEndCause::CDMA_REORDER:
         return std::string("CDMA reorder");
      case telux::tel::CallEndCause::CDMA_SO_REJECT:
         return std::string("CDMA SO reject");
      case telux::tel::CallEndCause::CDMA_RETRY_ORDER:
         return std::string("CDMA retry order");
      case telux::tel::CallEndCause::CDMA_ACCESS_FAILURE:
         return std::string("CDMA access failure");
      case telux::tel::CallEndCause::CDMA_PREEMPTED:
         return std::string("CDMA preempted");
      case telux::tel::CallEndCause::CDMA_NOT_EMERGENCY:
         return std::string("CDMA not emergency");
      case telux::tel::CallEndCause::CDMA_ACCESS_BLOCKED:
         return std::string("CDMA access blocked");
      case telux::tel::CallEndCause::EMERGENCY_TEMP_FAILURE:
         return std::string("Emergency temporary failure");
      case telux::tel::CallEndCause::EMERGENCY_PERM_FAILURE:
         return std::string("Emergency permanent failure");
      case telux::tel::CallEndCause::HO_NOT_FEASIBLE:
         return std::string("Hand over not feasible");
      case telux::tel::CallEndCause::LOW_BATTERY:
         return std::string("Low battery");
      case telux::tel::CallEndCause::BLACKLISTED_CALL_ID:
         return std::string("Blacklisted call ID");
      case telux::tel::CallEndCause::CS_RETRY_REQUIRED:
         return std::string("CS retry required");
      case telux::tel::CallEndCause::NETWORK_UNAVAILABLE:
         return std::string("Network unavailable");
      case telux::tel::CallEndCause::FEATURE_UNAVAILABLE:
         return std::string("Feature unavailable");
      case telux::tel::CallEndCause::SIP_ERROR:
         return std::string("SIP error");
      case telux::tel::CallEndCause::MISC:
         return std::string("MISC");
      case telux::tel::CallEndCause::ANSWERED_ELSEWHERE:
         return std::string("Answered elsewhere");
      case telux::tel::CallEndCause::PULL_OUT_OF_SYNC:
         return std::string("Pull out of sync");
      case telux::tel::CallEndCause::CAUSE_CALL_PULLED:
         return std::string("Cause call pulled");
      case telux::tel::CallEndCause::SIP_REDIRECTED:
         return std::string("Redirected");
      case telux::tel::CallEndCause::SIP_BAD_REQUEST:
         return std::string("Bad request");
      case telux::tel::CallEndCause::SIP_FORBIDDEN:
         return std::string("Forbidden");
      case telux::tel::CallEndCause::SIP_NOT_FOUND:
         return std::string("Not found");
      case telux::tel::CallEndCause::SIP_NOT_SUPPORTED:
         return std::string("Not Supported");
      case telux::tel::CallEndCause::SIP_REQUEST_TIMEOUT:
         return std::string("Request timeout");
      case telux::tel::CallEndCause::SIP_TEMPORARILY_UNAVAILABLE:
         return std::string("Temporary unavailable");
      case telux::tel::CallEndCause::SIP_BAD_ADDRESS:
         return std::string("Bad address");
      case telux::tel::CallEndCause::SIP_NOT_ACCEPTABLE:
         return std::string("Not acceptable");
      case telux::tel::CallEndCause::SIP_SERVER_INTERNAL_ERROR:
         return std::string("Server internal error");
      case telux::tel::CallEndCause::SIP_SERVER_NOT_IMPLEMENTED:
         return std::string("Server not implemented");
      case telux::tel::CallEndCause::SIP_SERVER_BAD_GATEWAY:
         return std::string("Bad gateway");
      case telux::tel::CallEndCause::SIP_SERVICE_UNAVAILABLE:
         return std::string("Service unavailable");
      case telux::tel::CallEndCause::SIP_SERVER_TIMEOUT:
         return std::string("Server timeout");
      case telux::tel::CallEndCause::SIP_SERVER_VERSION_UNSUPPORTED:
         return std::string("Server version unsupported");
      case telux::tel::CallEndCause::SIP_SERVER_MESSAGE_TOOLARGE:
         return std::string("Message too large");
      case telux::tel::CallEndCause::SIP_SERVER_PRECONDITION_FAILURE:
         return std::string("Precondition failure");
      case telux::tel::CallEndCause::SIP_GLOBAL_ERROR:
         return std::string("Global error");
      case telux::tel::CallEndCause::MEDIA_INIT_FAILED:
         return std::string("Media init failed");
      case telux::tel::CallEndCause::MEDIA_NO_DATA:
         return std::string("Media no data");
      case telux::tel::CallEndCause::MEDIA_NOT_ACCEPTABLE:
         return std::string("Media not acceptable");
      case telux::tel::CallEndCause::MEDIA_UNSPECIFIED_ERROR:
         return std::string("Media unspecified error");
      case telux::tel::CallEndCause::HOLD_RESUME_FAILED:
         return std::string("Hold resume failed");
      case telux::tel::CallEndCause::HOLD_RESUME_CANCELED:
         return std::string("Hold resume cancelled");
      case telux::tel::CallEndCause::HOLD_REINVITE_COLLISION:
         return std::string("Hold reinvite collision");
      case telux::tel::CallEndCause::SIP_ALTERNATE_EMERGENCY_CALL:
         return std::string("Emergency call");
      case telux::tel::CallEndCause::NO_CSFB_IN_CS_ROAM:
         return std::string("No cs fallback in roaming network");
      case telux::tel::CallEndCause::SRV_NOT_REGISTERED:
         return std::string("Service no registered");
      case telux::tel::CallEndCause::CALL_TYPE_NOT_ALLOWED:
         return std::string("Call type is not allowed");
      case telux::tel::CallEndCause::EMRG_CALL_ONGOING:
         return std::string("Emergency call ongoing");
      case telux::tel::CallEndCause::CALL_SETUP_ONGOING:
         return std::string("Call setup ongoing");
      case telux::tel::CallEndCause::MAX_CALL_LIMIT_REACHED:
         return std::string("Maximum call limit reached");
      case telux::tel::CallEndCause::UNSUPPORTED_SIP_HDRS:
         return std::string("Unsupported sip header");
      case telux::tel::CallEndCause::CALL_TRANSFER_ONGOING:
         return std::string("Call transfer ongoing");
      case telux::tel::CallEndCause::PRACK_TIMEOUT:
         return std::string("Memory failure");
      case telux::tel::CallEndCause::QOS_FAILURE:
         return std::string("Lack of dedicated barrier");
      case telux::tel::CallEndCause::ONGOING_HANDOVER:
         return std::string("Handover ongoing");
      case telux::tel::CallEndCause::VT_WITH_TTY_NOT_ALLOWED:
         return std::string("VT and TTY not supported together");
      case telux::tel::CallEndCause::CALL_UPGRADE_ONGOING:
         return std::string("Call upgrade is ongoing");
      case telux::tel::CallEndCause::CONFERENCE_WITH_TTY_NOT_ALLOWED:
         return std::string("Conference with TTY not allowed");
      case telux::tel::CallEndCause::CALL_CONFERENCE_ONGOING:
         return std::string("Call conference ongoing");
      case telux::tel::CallEndCause::VT_WITH_AVPF_NOT_ALLOWED:
         return std::string("VT with AVPF not allowed");
      case telux::tel::CallEndCause::ENCRYPTION_CALL_ONGOING:
         return std::string("Encryption call is ongoing");
      case telux::tel::CallEndCause::CALL_ONGOING_CW_DISABLED:
         return std::string("Call waiting disabled for incoming call");
      case telux::tel::CallEndCause::CALL_ON_OTHER_SUB:
         return std::string("Call on other subscription");
      case telux::tel::CallEndCause::ONE_X_COLLISION:
         return std::string("CDMA collision");
      case telux::tel::CallEndCause::UI_NOT_READY:
         return std::string("UI is not reay for incomg call");
      case telux::tel::CallEndCause::CS_CALL_ONGOING:
         return std::string("CS call is ongoing");
      case telux::tel::CallEndCause::REJECTED_ELSEWHERE:
         return std::string("One of the devices rejected the call");
      case telux::tel::CallEndCause::USER_REJECTED_SESSION_MODIFICATION:
         return std::string("Session modification is rejected");
      case telux::tel::CallEndCause::USER_CANCELLED_SESSION_MODIFICATION:
         return std::string("Session modification is cancelled");
      case telux::tel::CallEndCause::SESSION_MODIFICATION_FAILED:
         return std::string("Session modification is failed");
      case telux::tel::CallEndCause::SIP_UNAUTHORIZED:
         return std::string("Unauthorized");
      case telux::tel::CallEndCause::SIP_PAYMENT_REQUIRED:
          return std::string("Payment required");
      case telux::tel::CallEndCause::SIP_METHOD_NOT_ALLOWED:
         return std::string("Method not allowed");
      case telux::tel::CallEndCause::SIP_PROXY_AUTHENTICATION_REQUIRED:
         return std::string("Proxy authentication required");
      case telux::tel::CallEndCause::SIP_REQUEST_ENTITY_TOO_LARGE:
         return std::string("Request entity too large");
      case telux::tel::CallEndCause::SIP_REQUEST_URI_TOO_LARGE:
         return std::string("Request URI too large");
      case telux::tel::CallEndCause::SIP_EXTENSION_REQUIRED:
         return std::string("Extension requied");
      case telux::tel::CallEndCause::SIP_INTERVAL_TOO_BRIEF:
         return std::string("Interval too brief");
      case telux::tel::CallEndCause::SIP_CALL_OR_TRANS_DOES_NOT_EXIST:
         return std::string("Call/Transcation does not exist");
      case telux::tel::CallEndCause::SIP_LOOP_DETECTED:
         return std::string("Loop detected");
      case telux::tel::CallEndCause::SIP_TOO_MANY_HOPS:
         return std::string("Too many hops");
      case telux::tel::CallEndCause::SIP_AMBIGUOUS:
         return std::string("Ambiguous");
      case telux::tel::CallEndCause::SIP_REQUEST_PENDING:
         return std::string("Request pending");
      case telux::tel::CallEndCause::SIP_UNDECIPHERABLE:
         return std::string("Undecipherable");
      case telux::tel::CallEndCause::RETRY_ON_IMS_WITHOUT_RTT:
         return std::string("Retry call by disabling RTT");
      case telux::tel::CallEndCause::MAX_PS_CALLS:
         return std::string("Maximum PS calls exceeded");
      case telux::tel::CallEndCause::SIP_MULTIPLE_CHOICES:
         return std::string("Multiple choices");
      case telux::tel::CallEndCause::SIP_MOVED_PERMANENTLY:
         return std::string("Moved permanently");
      case telux::tel::CallEndCause::SIP_MOVED_TEMPORARILY:
         return std::string("Moved temporarily");
      case telux::tel::CallEndCause::SIP_USE_PROXY:
         return std::string("Use proxy");
      case telux::tel::CallEndCause::SIP_ALTERNATE_SERVICE:
         return std::string("Alternative service");
      case telux::tel::CallEndCause::SIP_UNSUPPORTED_URI_SCHEME:
         return std::string("Unsupported URI scheme");
      case telux::tel::CallEndCause::SIP_REMOTE_UNSUPP_MEDIA_TYPE:
         return std::string("Unsupported media type");
      case telux::tel::CallEndCause::SIP_BAD_EXTENSION:
         return std::string("Bad extension");
      case telux::tel::CallEndCause::DSDA_CONCURRENT_CALL_NOT_POSSIBLE:
         return std::string("Concurrent call is not possible");
      case telux::tel::CallEndCause::EPSFB_FAILURE:
         return std::string("EPS fallback failure");
      case telux::tel::CallEndCause::TWAIT_EXPIRED:
         return std::string("Twait timer expired");
      case telux::tel::CallEndCause::TCP_CONNECTION_REQ:
         return std::string("TCP connection rejected");
      case telux::tel::CallEndCause::THERMAL_EMERGENCY:
         return std::string("Thermal emergency");
      case telux::tel::CallEndCause::ERROR_UNSPECIFIED:
         return std::string("Error unspecified");
      default:
         std::stringstream ss;
         ss << "Unknown call fail cause = " << (int)callEndCause;
         return ss.str();
   }
}

std::string TelClientUtils::eCallHlapTimerStatusToString(telux::tel::HlapTimerStatus status) {
    switch(status) {
        case telux::tel::HlapTimerStatus::INACTIVE:
            return std::string("INACTIVE");
        case telux::tel::HlapTimerStatus::ACTIVE:
            return std::string("ACTIVE");
        case telux::tel::HlapTimerStatus::UNKNOWN:
            return std::string("UNKNOWN");
        default:
            std::stringstream ss;
            ss << "Unknown HlapTimerStatus  = " << (int)status;
            return ss.str();
    }
}

std::string TelClientUtils::eCallHlapTimerEventToString(telux::tel::HlapTimerEvent event) {
    switch(event) {
        case telux::tel::HlapTimerEvent::STARTED:
            return std::string("STARTED");
        case telux::tel::HlapTimerEvent::STOPPED:
            return std::string("STOPPED");
        case telux::tel::HlapTimerEvent::EXPIRED:
            return std::string("EXPIRED");
        case telux::tel::HlapTimerEvent::UNKNOWN:
            return std::string("UNKNOWN");
        case telux::tel::HlapTimerEvent::UNCHANGED:
            return std::string("UNCHANGED");
        case telux::tel::HlapTimerEvent::RESUMED:
            return std::string("RESUMED");
        default:
            std::stringstream ss;
            ss << "Unknown HlapTimerEvent  = " << (int)event;
            return ss.str();
    }
}

void TelClientUtils::printEcallConfig(EcallConfig config) {
    auto validityMask = config.configValidityMask;
    if(validityMask.test(ECALL_CONFIG_MUTE_RX_AUDIO)) {
        std::cout << CLIENT_NAME << " Config to mute audio during MSD Transmission: " <<
        (config.muteRxAudio?"Mute":"Unmute") << std::endl;
    }
    if(validityMask.test(ECALL_CONFIG_NUM_TYPE)) {
        if(config.numType == ECallNumType::DEFAULT) {
            std::cout << CLIENT_NAME << " Dial number type configured: Default" << std::endl;
        } else {
            std::cout << CLIENT_NAME << " Dial number type configured: Overridden" << std::endl;
        }
    }
    if(validityMask.test(ECALL_CONFIG_OVERRIDDEN_NUM)) {
        std::cout << CLIENT_NAME << " Configured overridden number: " << config.overriddenNum
            << std::endl;
    }
    if(validityMask.test(ECALL_CONFIG_USE_CANNED_MSD)) {
        std::cout << CLIENT_NAME << " Configured to use canned MSD: " <<
            (config.useCannedMsd?"Yes":"No") << std::endl;
    }
    if(validityMask.test(ECALL_CONFIG_GNSS_UPDATE_INTERVAL)) {
        std::cout << CLIENT_NAME << " GNSS update interval: " << config.gnssUpdateInterval
            << std::endl;
    }
    if(validityMask.test(ECALL_CONFIG_T2_TIMER)) {
        std::cout << CLIENT_NAME << " T2 Timer value: " << config.t2Timer << std::endl;
    }
    if(validityMask.test(ECALL_CONFIG_T7_TIMER)) {
        std::cout << CLIENT_NAME << " T7 Timer value: " << config.t7Timer << std::endl;
    }
    if(validityMask.test(ECALL_CONFIG_T9_TIMER)) {
        std::cout << CLIENT_NAME << " T9 Timer value: " << config.t9Timer << std::endl;
    }
    if(validityMask.test(ECALL_CONFIG_MSD_VERSION)) {
        std::cout << CLIENT_NAME << " MSD Version: " << static_cast<int>(config.msdVersion)
            << std::endl;
    }
}

void TelClientUtils::printEncodedOptionalAdditionalDataContent(std::string encodedString) {
    if (!encodedString.empty()) {
        std::cout << CLIENT_NAME << " Encoded optional additional data content: "
            << encodedString << std::endl;
    } else {
        std::cout << CLIENT_NAME << " Encoded optional additional data content is empty"
            << std::endl;
    }
}

void TelClientUtils::printECallMsdPayload(std::string encodedPdu) {
    if (!encodedPdu.empty()) {
        std::cout << CLIENT_NAME << " Encoded eCall MSD payload: " << encodedPdu
            << std::endl;
    } else {
        std::cout << CLIENT_NAME << " Encoded eCall MSD payload is empty"
            << std::endl;
    }
}

std::string TelClientUtils::eCallRedialReasonToString(ReasonType reason) {
    switch(reason) {
        case telux::tel::ReasonType::CALL_ORIG_FAILURE:
            return std::string(" call origination failure");
        case telux::tel::ReasonType::CALL_DROP:
            return std::string(" call drop failure");
        case telux::tel::ReasonType::MAX_REDIAL_ATTEMPTED:
            return std::string(" maximum redial count reached");
        case telux::tel::ReasonType::CALL_CONNECTED:
            return std::string(" call connected successfully");
        case telux::tel::ReasonType::NONE:
            return std::string(" none");
        default:
            std::stringstream ss;
            ss << " Unknown ReasonType  = " << (int)reason;
            return ss.str();
    }
}
