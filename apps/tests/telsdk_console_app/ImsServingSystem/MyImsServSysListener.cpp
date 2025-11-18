/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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


#include <iostream>
#include <string>
#include <sstream>

#include "MyImsServSysListener.hpp"
#include "../Phone/MyPhoneListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

// Implementation of IMS Registration State callback
void MyImsServSysCallback::imsRegStateResponse(SlotId slotId,
    telux::tel::ImsRegistrationInfo status, telux::common::ErrorCode error) {
    std::cout << std::endl << std::endl;
    std::cout << " Request IMS Registration status response received on slotId "
            << static_cast<int>(slotId) << std::endl;
    if(error == telux::common::ErrorCode::SUCCESS) {
        std::cout << "\n";
        PRINT_CB << "IMS Registration Status: "
            << MyImsServSysListener::convertRegStatustoString(status.imsRegStatus)
                << "\n Radio technology: "
                << MyPhoneHelper::radioTechToString(status.rat)
                << "\n Error Code: " << status.errorCode << "\n Error Description: "
                << status.errorString << std::endl;
    } else {
        PRINT_CB << "requestRegistrationInfo failed, errorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

// Implementation of IMS Service Status callback
void MyImsServSysCallback::imsServiceStatusResponse(SlotId slotId,
    telux::tel::ImsServiceInfo service, telux::common::ErrorCode error) {
    std::cout << std::endl << std::endl;
    std::cout << " IMS service status response received on slotId "
            << static_cast<int>(slotId) << std::endl;
    if(error == telux::common::ErrorCode::SUCCESS) {
        std::cout << "\n";
        PRINT_CB << "SMS Service Status over IMS: "
            << MyImsServSysListener::convertServiceStatustoString(service.sms)
            << "\n Voice Service Status over IMS: "
            << MyImsServSysListener::convertServiceStatustoString(service.voice)
            << std::endl;
    } else {
        PRINT_CB << "requestServiceInfo failed, errorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

// Implementation of IMS Pdp Status callback
void MyImsServSysCallback::imsPdpStatusResponse(SlotId slotId,
    telux::tel::ImsPdpStatusInfo status, telux::common::ErrorCode error) {
    std::cout << std::endl << std::endl;
    std::cout << " IMS PDP status response received on slotId "
            << static_cast<int>(slotId) << std::endl;
    if (error == telux::common::ErrorCode::SUCCESS) {
        std::cout << "\n";
        PRINT_CB << "IMS PDP is connected : "
            << ((status.isPdpConnected) ? "Yes" : "No")
            << "\n IMS PDP Failure Error Code: "
            << MyImsServSysListener::convertPdpFailureErrorToString(status.failureCode)
            << "\n IMS PDP Failure Cause/Reason Code: "
            << ((status.isPdpConnected) ? "" : MyImsServSysListener::
               convertPdpFailureReasonTypeToString(status.failureReason.type))
            << "\n IMS PDP Failure Reason: "
            << ((status.isPdpConnected) ? "" : std::to_string(MyImsServSysListener::
                callEndReasonCode(status.failureReason)))
            << "\n IMS PDN Name: " << status.apnName
            << std::endl;
    } else {
        PRINT_CB << "requestPdpStatusInfo failed, errorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

MyImsServSysListener::MyImsServSysListener(SlotId slotId)
   : slotId_(slotId) {
}

void MyImsServSysListener::onImsRegStatusChange(telux::tel::ImsRegistrationInfo status) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << "onImsRegStatusChange, SlotId: " << static_cast<int>(slotId_)
        << std::endl;
    PRINT_NOTIFICATION << "IMS Registration status changed to: "
                << convertRegStatustoString(status.imsRegStatus)
                << "\n Radio Technology: " << MyPhoneHelper::radioTechToString(status.rat)
                << "\n Error Code: " << status.errorCode
                << "\n Error Description: " << status.errorString << std::endl;
}

void MyImsServSysListener::onImsServiceInfoChange(telux::tel::ImsServiceInfo service) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << "onImsServiceInfoChange, SlotId: " << static_cast<int>(slotId_)
        << std::endl;
    PRINT_NOTIFICATION << "SMS Service Status over IMS: "
                << convertServiceStatustoString(service.sms)
                << "\n Voice Service Status over IMS: "
                << convertServiceStatustoString(service.voice)
                << std::endl;
}

void MyImsServSysListener::onImsPdpStatusInfoChange(telux::tel::ImsPdpStatusInfo status) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << "onImsPdpStatusInfoChange, SlotId: " << static_cast<int>(slotId_)
        << std::endl;
    PRINT_NOTIFICATION  "IMS PDP is connected : "
            << ((status.isPdpConnected) ? "Yes" : "No")
            << "\n IMS PDP Failure Error Code: "
            << convertPdpFailureErrorToString(status.failureCode)
            << "\n IMS PDP Failure Cause/Reason Code: "
            << ((status.isPdpConnected) ? "" : convertPdpFailureReasonTypeToString
                (status.failureReason.type))
            << "\n IMS PDP Failure Reason: "
            << ((status.isPdpConnected) ? "" : std::to_string(
               callEndReasonCode(status.failureReason)))
            << "\n IMS PDN Name: " << status.apnName
            << std::endl;
}

std::string MyImsServSysListener::convertRegStatustoString(telux::tel::RegistrationStatus state) {
    std::string stateString;

    switch(state) {
        case telux::tel::RegistrationStatus::NOT_REGISTERED:
            stateString = "NOT_REGISTERED";
            break;
        case telux::tel::RegistrationStatus::REGISTERING:
            stateString = "REGISTERING";
            break;
        case telux::tel::RegistrationStatus::REGISTERED:
            stateString = "REGISTERED";
            break;
        case telux::tel::RegistrationStatus::LIMITED_REGISTERED:
            stateString = "LIMITED_REGISTERED";
            break;
        default:
            stateString = "Unknown registration status";
            break;
    }

    return stateString;
}

std::string MyImsServSysListener::convertServiceStatustoString(
    telux::tel::CellularServiceStatus status) {
    std::string statusString;

    switch(status) {
        case telux::tel::CellularServiceStatus::NO_SERVICE:
            statusString = "NO_SERVICE";
            break;
        case telux::tel::CellularServiceStatus::LIMITED_SERVICE:
            statusString = "LIMITED_SERVICE";
            break;
        case telux::tel::CellularServiceStatus::FULL_SERVICE:
            statusString = "FULL_SERVICE";
            break;
        default:
            statusString = "Unknown service status";
            break;
    }

    return statusString;
}

std::string MyImsServSysListener::convertPdpFailureErrorToString(
    telux::tel::PdpFailureCode errorCode) {
    std::string errorCodeString;

    switch(errorCode) {
        case telux::tel::PdpFailureCode::OTHER_FAILURE:
            errorCodeString = "OTHER_FAILURE";
            break;
        case telux::tel::PdpFailureCode::OPTION_UNSUBSCRIBED:
            errorCodeString = "OPTION_UNSUBSCRIBED";
            break;
        case telux::tel::PdpFailureCode::UNKNOWN_PDP:
            errorCodeString = "UNKNOWN_PDP";
            break;
        case telux::tel::PdpFailureCode::REASON_NOT_SPECIFIED:
            errorCodeString = "REASON_NOT_SPECIFIED";
            break;
        case telux::tel::PdpFailureCode::CONNECTION_BRINGUP_FAILURE:
            errorCodeString = "CONNECTION_BRINGUP_FAILURE";
            break;
        case telux::tel::PdpFailureCode::CONNECTION_IKE_AUTH_FAILURE:
            errorCodeString = "CONNECTION_IKE_AUTH_FAILURE";
            break;
        case telux::tel::PdpFailureCode::USER_AUTH_FAILED:
            errorCodeString = "USER_AUTH_FAILED";
            break;
        default:
            errorCodeString = "OTHER_FAILURE";
            break;
    }

    return errorCodeString;
}

std::string MyImsServSysListener::convertPdpFailureReasonTypeToString(
    telux::common::EndReasonType reasonType) {
    std::string reasonTypeString;
    switch(reasonType) {
        case telux::common::EndReasonType::CE_UNKNOWN:
            reasonTypeString = "UNKNOWN";
            break;
        case telux::common::EndReasonType::CE_MOBILE_IP:
            reasonTypeString = "MOBILE_IP";
            break;
        case telux::common::EndReasonType::CE_INTERNAL:
            reasonTypeString = "INTERNAL";
            break;
        case telux::common::EndReasonType::CE_CALL_MANAGER_DEFINED:
            reasonTypeString = "CALL_MANAGER_DEFINED";
            break;
        case telux::common::EndReasonType::CE_3GPP_SPEC_DEFINED:
            reasonTypeString = "3GPP_SPEC_DEFINED";
            break;
        case telux::common::EndReasonType::CE_PPP:
            reasonTypeString = "PPP";
            break;
        case telux::common::EndReasonType::CE_EHRPD:
            reasonTypeString = "EHRPD";
            break;
        case telux::common::EndReasonType::CE_IPV6:
            reasonTypeString = "IPV6";
            break;
        case telux::common::EndReasonType::CE_HANDOFF:
            reasonTypeString = "HANDOFF";
            break;
        default:
            reasonTypeString = "UNKNOWN";
            break;
    }
    return reasonTypeString;
}

int MyImsServSysListener::callEndReasonCode(telux::common::DataCallEndReason ceReason) {
   switch(ceReason.type) {
      case telux::common::EndReasonType::CE_MOBILE_IP:
         return static_cast<int>(ceReason.IpCode);
      case telux::common::EndReasonType::CE_INTERNAL:
         return static_cast<int>(ceReason.internalCode);
      case telux::common::EndReasonType::CE_CALL_MANAGER_DEFINED:
         return static_cast<int>(ceReason.cmCode);
      case telux::common::EndReasonType::CE_3GPP_SPEC_DEFINED:
         return static_cast<int>(ceReason.specCode);
      case telux::common::EndReasonType::CE_PPP:
         return static_cast<int>(ceReason.pppCode);
      case telux::common::EndReasonType::CE_EHRPD:
         return static_cast<int>(ceReason.ehrpdCode);
      case telux::common::EndReasonType::CE_IPV6:
         return static_cast<int>(ceReason.ipv6Code);
      case telux::common::EndReasonType::CE_HANDOFF:
         return static_cast<int>(ceReason.handOffCode);
      case telux::common::EndReasonType::CE_UNKNOWN:
         return -1;
      default: { return -1; }
   }
}

// Notify ImsServingSystemManager subsystem status
void MyImsServSysListener::onServiceStatusChange(telux::common::ServiceStatus status) {
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
    PRINT_NOTIFICATION << " Ims ServingSystem onServiceStatusChange" << stat << "\n";
}
