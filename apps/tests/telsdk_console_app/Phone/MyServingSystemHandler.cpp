/*
 *  Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
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
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>

#include "MyServingSystemHandler.hpp"
#include "MyPhoneListener.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

std::string MyServingSystemHelper::getRatPreference(telux::tel::RatPreference preference) {
   std::string ratPrefString = "";
   if(preference[telux::tel::PREF_CDMA_1X]) {
      ratPrefString += " CDMA_1X \n";
   }

   if(preference[telux::tel::PREF_CDMA_EVDO]) {
      ratPrefString += " CDMA_EVDO \n";
   }

   if(preference[telux::tel::PREF_GSM]) {
      ratPrefString += " GSM \n";
   }

   if(preference[telux::tel::PREF_WCDMA]) {
      ratPrefString += " WCDMA \n";
   }

   if(preference[telux::tel::PREF_LTE]) {
      ratPrefString += " LTE \n";
   }

   if(preference[telux::tel::PREF_TDSCDMA]) {
      ratPrefString += " TDSCDMA \n";
   }

   if(preference[telux::tel::PREF_NR5G]) {
      ratPrefString += " NR5G_COMBINED \n";
   }

   if(preference[telux::tel::PREF_NR5G_NSA]) {
      ratPrefString += " NR5G_NSA \n";
   }

   if(preference[telux::tel::PREF_NR5G_SA]) {
      ratPrefString += " NR5G_SA \n";
   }
   if (preference[telux::tel::PREF_NB1_NTN]) {
       ratPrefString += " NB1_NTN \n";
   }
   return ratPrefString;
}

std::string MyServingSystemHelper::getEndcAvailability(telux::tel::EndcAvailability isAvailable) {
   std::string availabilityString = "";
   if(isAvailable == telux::tel::EndcAvailability::AVAILABLE) {
      availabilityString += " AVAILABLE \n";
   } else if(isAvailable == telux::tel::EndcAvailability::UNAVAILABLE){
       availabilityString += " NOT AVAILABLE \n";
   } else {
       availabilityString += " UNKNOWN \n";
   }
   return availabilityString;
}

std::string MyServingSystemHelper::getDcnrRestriction(telux::tel::DcnrRestriction isRestricted) {
   std::string restrictedString = "";
   if(isRestricted == telux::tel::DcnrRestriction::RESTRICTED) {
      restrictedString += " RESTRICTED \n";
   } else if(isRestricted == telux::tel::DcnrRestriction::UNRESTRICTED) {
       restrictedString += " NOT RESTRICTED \n";
   } else {
       restrictedString += " UNKNOWN \n";
   }
   return restrictedString;
}

void MyRatPreferenceResponseCallback::ratPreferenceResponse(telux::tel::RatPreference preference,
                                                            telux::common::ErrorCode error) {
   std::cout << "\n\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "\nRAT mode preference: \n"
               << MyServingSystemHelper::getRatPreference(preference);
   } else {
      PRINT_CB << "ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

std::string MyServingSystemHelper::getServiceDomainPref(telux::tel::ServiceDomainPreference preference) {
   std::string prefString = " Unknown";
   switch(preference) {
      case telux::tel::ServiceDomainPreference::CS_ONLY:
         prefString = " Circuit Switched(CS) only";
         break;
      case telux::tel::ServiceDomainPreference::PS_ONLY:
         prefString = " Packet Switched(PS) only";
         break;
      case telux::tel::ServiceDomainPreference::CS_PS:
         prefString = " Circuit Switched and Packet Switched ";
         break;
      default:
         break;
   }
   return prefString;
}

std::string MyServingSystemHelper::getServiceDomain(telux::tel::ServiceDomain domain) {
   std::string domainString = " Unknown ";
   switch(domain) {
      case telux::tel::ServiceDomain::NO_SRV:
         domainString = " No Service ";
         break;
      case telux::tel::ServiceDomain::CS_ONLY:
         domainString = " Circuit Switched(CS) only ";
         break;
      case telux::tel::ServiceDomain::PS_ONLY:
         domainString = " Packet Switched(PS) only ";
         break;
      case telux::tel::ServiceDomain::CS_PS:
         domainString = " Circuit Switched and Packet Switched ";
         break;
      case telux::tel::ServiceDomain::CAMPED:
         domainString = " Camped ";
         break;
      case telux::tel::ServiceDomain::UNKNOWN:
         domainString = " Unknown ";
         break;
      default:
         break;
   }
   return domainString;
}

std::string MyServingSystemHelper::getServiceState(telux::tel::ServiceRegistrationState state) {
   std::string stateString = "Unknown";
   switch(state) {
      case telux::tel::ServiceRegistrationState::NO_SERVICE:
         stateString = "No Service";
         break;
      case telux::tel::ServiceRegistrationState::LIMITED_SERVICE:
         stateString = "Limited Service";
         break;
      case telux::tel::ServiceRegistrationState::IN_SERVICE:
         stateString = "In Service";
         break;
      case telux::tel::ServiceRegistrationState::LIMITED_REGIONAL:
         stateString = "Limited Regional Service";
         break;
      case telux::tel::ServiceRegistrationState::POWER_SAVE:
         stateString = "Power Save";
         break;
      case telux::tel::ServiceRegistrationState::UNKNOWN:
      default:
         stateString = "Unknown";
         break;
   }
   return stateString;
}

std::string MyServingSystemHelper::getSmsDomain(telux::tel::SmsDomain domain) {
   std::string domainString = " Unknown ";
   switch(domain) {
      case telux::tel::SmsDomain::NO_SMS:
         domainString = " No SMS ";
         break;
      case telux::tel::SmsDomain::SMS_ON_IMS:
         domainString = " SMS on IMS ";
         break;
      case telux::tel::SmsDomain::SMS_ON_3GPP:
         domainString = " SMS on 3GPP ";
         break;
      case telux::tel::SmsDomain::UNKNOWN:
         domainString = " Unknown ";
         break;
      default:
         break;
   }
   return domainString;
}

std::string MyServingSystemHelper::getNtnSmsStatus(telux::tel::NtnSmsStatus status) {
    std::string smsStatusString = "Unknown";
    switch (status) {
        case telux::tel::NtnSmsStatus::NOT_AVAILABLE:
            smsStatusString = "Not available";
            break;
        case telux::tel::NtnSmsStatus::TEMP_FAILURE:
            smsStatusString = "Temporary failure";
            break;
        case telux::tel::NtnSmsStatus::AVAILABLE:
            smsStatusString = "Available";
            break;
        case telux::tel::NtnSmsStatus::UNKNOWN:
        default:
            smsStatusString = "Unknown";
            break;
    }
    return smsStatusString;
}

std::string MyServingSystemHelper::getLteCsCapability(telux::tel::LteCsCapability capability) {
   std::string capabilityString = " Unknown ";
   switch(capability) {
      case telux::tel::LteCsCapability::FULL_SERVICE:
         capabilityString = " Full Service ";
         break;
      case telux::tel::LteCsCapability::CSFB_NOT_PREFERRED:
         capabilityString = " CSFB Not Preferred ";
         break;
      case telux::tel::LteCsCapability::SMS_ONLY:
         capabilityString = " SMS Only ";
         break;
      case telux::tel::LteCsCapability::LIMITED:
         capabilityString = " Limited ";
         break;
      case telux::tel::LteCsCapability::BARRED:
         capabilityString = " Barred ";
         break;
      case telux::tel::LteCsCapability::UNKNOWN:
         capabilityString = " Unknown ";
         break;
      default:
         break;
   }
   return capabilityString;
}

std::string MyServingSystemHelper::getCallBarringType(telux::tel::CallsAllowedInCell type) {
   std::string typeString = " Unknown ";
   switch(type) {
      case telux::tel::CallsAllowedInCell::NORMAL_ONLY:
         typeString = " Normal Only ";
         break;
      case telux::tel::CallsAllowedInCell::EMERGENCY_ONLY:
         typeString = " Emergency Only ";
         break;
      case telux::tel::CallsAllowedInCell::NO_CALLS:
         typeString = " No Calls ";
         break;
      case telux::tel::CallsAllowedInCell::ALL_CALLS:
         typeString = " All Calls ";
         break;
      case telux::tel::CallsAllowedInCell::UNKNOWN:
         typeString = " Unknown ";
         break;
      default:
         break;
   }
   return typeString;
}

std::string MyServingSystemHelper::getRadioTechnology(telux::tel::RadioTechnology radioTech) {
    return MyPhoneHelper::radioTechToString(radioTech);
}

void MyServiceDomainPrefResponseCallback::serviceDomainPrefResponse(
   telux::tel::ServiceDomainPreference preference, telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "\n requestServiceDomainPreference is successful.\n Service domain is "
               << MyServingSystemHelper::getServiceDomainPref(preference) << std::endl;
   } else {
      PRINT_CB << "\n requestServiceDomainPreference failed, ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyServingSystemResponsecallback::servingSystemResponse(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "setRatPreference is successful" << std::endl;
   } else {
      PRINT_CB << "setRatPreference Request failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyServingSystemListener::onRatPreferenceChanged(telux::tel::RatPreference preference) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << "RAT mode preference: \n"
                      << MyServingSystemHelper::getRatPreference(preference);
}

void MyServingSystemListener::onServiceDomainPreferenceChanged(
   telux::tel::ServiceDomainPreference preference) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << " Service domain preference is"
                      << MyServingSystemHelper::getServiceDomainPref(preference) << std::endl;
}

void MyServingSystemListener::onSystemInfoChanged(telux::tel::ServingSystemInfo sysInfo) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << " Serving System information is changed" << std::endl;
   PRINT_NOTIFICATION << " Serving RAT is "
      << MyServingSystemHelper::getRadioTechnology(sysInfo.rat) << std::endl;
   PRINT_NOTIFICATION << " Service domain is "
      << MyServingSystemHelper::getServiceDomain(sysInfo.domain) << std::endl;
}

void MyServingSystemListener::onDcStatusChanged(telux::tel::DcStatus dcStatus) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << "\nENDC Availability: \n"
                      << MyServingSystemHelper::getEndcAvailability(dcStatus.endcAvailability);
   PRINT_NOTIFICATION << "\nDCNR Restriction: \n"
                      << MyServingSystemHelper::getDcnrRestriction(dcStatus.dcnrRestriction);
}

void NetworkTimeResponseCallback::networkTimeResponse(telux::tel::NetworkTimeInfo info,
      telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "\n NetworkTime request is successful.\n Network Time: \n";
      MyServingSystemHelper::logNetworkInfo(info);
   } else {
      PRINT_CB << "\n NetworkTime request is failed, ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyServingSystemListener::onNetworkTimeChanged(telux::tel::NetworkTimeInfo info) {
   PRINT_NOTIFICATION << " Network Time information is changed. \n Network Time: \n";
   MyServingSystemHelper::logNetworkInfo(info);
}

void MyServingSystemListener::onNetworkTimeChanged(telux::tel::RadioTechnology radioTech,
    telux::tel::NetworkTimeInfo info) {
    PRINT_NOTIFICATION << " Time information is changed on RAT: " <<
        MyServingSystemHelper::getRadioTechnology(radioTech) << "\n Network Time: \n";
    MyServingSystemHelper::logNetworkInfo(info);
}

void MyServingSystemHelper::logNetworkInfo(telux::tel::NetworkTimeInfo info) {
   std::cout << " Year: " << info.year << "\n"
      << " Month: " << static_cast<int>(info.month) << "\n"
      << " Day: " << static_cast<int>(info.day) << "\n"
      << " Hour: " << static_cast<int>(info.hour) << "\n"
      << " Minute: " << static_cast<int>(info.minute) << "\n"
      << " Second: " << static_cast<int>(info.second) << "\n"
      << " DayOfWeek: " << static_cast<int>(info.dayOfWeek) << "\n"
      << " TimeZone: " << static_cast<int>(info.timeZone) << "\n"
      << " DayLight Saving Adj: " << static_cast<int>(info.dstAdj) << "\n \n"
      << "NITZ Time: " << info.nitzTime << std::endl;
}

void RFBandInfoResponseCallback::rfBandInfoResponse(telux::tel::RFBandInfo bandInfo,
      telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "\n requestRFBandInfo is successful.\n RF Band Info: \n";
      MyServingSystemHelper::logRFBandInfo(bandInfo);
   } else {
      PRINT_CB << "\n requestRFBandInfo failed, ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyServingSystemHelper::logRFBandInfo(telux::tel::RFBandInfo bandInfo) {
   std::cout << "Active RFBand: " << RFBandtoString(bandInfo.band) << "\n"
      << "Active Channel: " << bandInfo.channel << "\n"
      << "Band Width: " << RFBandWidthtoString(bandInfo.bandWidth) << std::endl;
}

std::string MyServingSystemHelper::RFBandtoString(telux::tel::RFBand band) {
   switch(band) {
      case telux::tel::RFBand::INVALID : return "INVALID" ;
      case telux::tel::RFBand::BC_0 : return "BC_0" ;
      case telux::tel::RFBand::BC_1 : return "BC_1" ;
      case telux::tel::RFBand::BC_3 : return "BC_3" ;
      case telux::tel::RFBand::BC_4 : return "BC_4" ;
      case telux::tel::RFBand::BC_5 : return "BC_5" ;
      case telux::tel::RFBand::BC_6 : return "BC_6" ;
      case telux::tel::RFBand::BC_7 : return "BC_7" ;
      case telux::tel::RFBand::BC_8 : return "BC_8" ;
      case telux::tel::RFBand::BC_9 : return "BC_9" ;
      case telux::tel::RFBand::BC_10 : return "BC_10" ;
      case telux::tel::RFBand::BC_11 : return "BC_11" ;
      case telux::tel::RFBand::BC_12 : return "BC_12" ;
      case telux::tel::RFBand::BC_13 : return "BC_13" ;
      case telux::tel::RFBand::BC_14 : return "BC_14" ;
      case telux::tel::RFBand::BC_15 : return "BC_15" ;
      case telux::tel::RFBand::BC_16 : return "BC_16" ;
      case telux::tel::RFBand::BC_17 : return "BC_17" ;
      case telux::tel::RFBand::BC_18 : return "BC_18" ;
      case telux::tel::RFBand::BC_19 : return "BC_19" ;
      case telux::tel::RFBand::GSM_450 : return "GSM_450" ;
      case telux::tel::RFBand::GSM_480 : return "GSM_480" ;
      case telux::tel::RFBand::GSM_750 : return "GSM_750" ;
      case telux::tel::RFBand::GSM_850 : return "GSM_850" ;
      case telux::tel::RFBand::GSM_900_EXTENDED : return "GSM_900_EXTENDED" ;
      case telux::tel::RFBand::GSM_900_PRIMARY  : return "GSM_900_PRIMARY" ;
      case telux::tel::RFBand::GSM_900_RAILWAYS : return "GSM_900_RAILWAYS" ;
      case telux::tel::RFBand::GSM_1800 : return "GSM_1800" ;
      case telux::tel::RFBand::GSM_1900 : return "GSM_1900" ;
      case telux::tel::RFBand::WCDMA_2100 : return "WCDMA_2100" ;
      case telux::tel::RFBand::WCDMA_PCS_1900 : return "WCDMA_PCS_1900" ;
      case telux::tel::RFBand::WCDMA_DCS_1800 : return "WCDMA_DCS_1800" ;
      case telux::tel::RFBand::WCDMA_1700_US  : return "WCDMA_1700_US" ;
      case telux::tel::RFBand::WCDMA_850 : return "WCDMA_850" ;
      case telux::tel::RFBand::WCDMA_800 : return "WCDMA_800" ;
      case telux::tel::RFBand::WCDMA_2600: return "WCDMA_2600" ;
      case telux::tel::RFBand::WCDMA_900 : return "WCDMA_900" ;
      case telux::tel::RFBand::WCDMA_1700_JAPAN : return "WCDMA_1700_JAPAN" ;
      case telux::tel::RFBand::WCDMA_1500_JAPAN : return "WCDMA_1500_JAPAN" ;
      case telux::tel::RFBand::WCDMA_850_JAPAN  : return "WCDMA_850_JAPAN" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_1 : return "E_UTRA_OPERATING_BAND_1" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_2 : return "E_UTRA_OPERATING_BAND_2" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_3 : return "E_UTRA_OPERATING_BAND_3" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_4 : return "E_UTRA_OPERATING_BAND_4" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_5 : return "E_UTRA_OPERATING_BAND_5" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_6 : return "E_UTRA_OPERATING_BAND_6" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_7 : return "E_UTRA_OPERATING_BAND_7" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_8 : return "E_UTRA_OPERATING_BAND_8" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_9 : return "E_UTRA_OPERATING_BAND_9" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_10 : return "E_UTRA_OPERATING_BAND_10" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_11 : return "E_UTRA_OPERATING_BAND_11" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_12 : return "E_UTRA_OPERATING_BAND_12" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_13 : return "E_UTRA_OPERATING_BAND_13" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_14 : return "E_UTRA_OPERATING_BAND_14" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_17 : return "E_UTRA_OPERATING_BAND_17" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_33 : return "E_UTRA_OPERATING_BAND_33" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_34 : return "E_UTRA_OPERATING_BAND_34" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_35 : return "E_UTRA_OPERATING_BAND_35" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_36 : return "E_UTRA_OPERATING_BAND_36" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_37 : return "E_UTRA_OPERATING_BAND_37" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_38 : return "E_UTRA_OPERATING_BAND_38" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_39 : return "E_UTRA_OPERATING_BAND_39" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_40 : return "E_UTRA_OPERATING_BAND_40" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_18 : return "E_UTRA_OPERATING_BAND_18" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_19 : return "E_UTRA_OPERATING_BAND_19" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_20 : return "E_UTRA_OPERATING_BAND_20" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_21 : return "E_UTRA_OPERATING_BAND_21" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_24 : return "E_UTRA_OPERATING_BAND_24" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_25 : return "E_UTRA_OPERATING_BAND_25" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_41 : return "E_UTRA_OPERATING_BAND_41" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_42 : return "E_UTRA_OPERATING_BAND_42" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_43 : return "E_UTRA_OPERATING_BAND_43" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_23 : return "E_UTRA_OPERATING_BAND_23" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_26 : return "E_UTRA_OPERATING_BAND_26" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_32 : return "E_UTRA_OPERATING_BAND_32" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_125 : return "E_UTRA_OPERATING_BAND_125" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_126 : return "E_UTRA_OPERATING_BAND_126" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_127 : return "E_UTRA_OPERATING_BAND_127" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_28 : return "E_UTRA_OPERATING_BAND_28" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_29 : return "E_UTRA_OPERATING_BAND_29" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_30 : return "E_UTRA_OPERATING_BAND_30" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_66 : return "E_UTRA_OPERATING_BAND_66" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_250 : return "E_UTRA_OPERATING_BAND_250" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_46 : return "E_UTRA_OPERATING_BAND_46" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_27 : return "E_UTRA_OPERATING_BAND_27" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_31 : return "E_UTRA_OPERATING_BAND_31" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_71 : return "E_UTRA_OPERATING_BAND_71" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_47 : return "E_UTRA_OPERATING_BAND_47" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_48 : return "E_UTRA_OPERATING_BAND_48" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_67 : return "E_UTRA_OPERATING_BAND_67" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_68 : return "E_UTRA_OPERATING_BAND_68" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_49 : return "E_UTRA_OPERATING_BAND_49" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_85 : return "E_UTRA_OPERATING_BAND_85" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_72 : return "E_UTRA_OPERATING_BAND_72" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_73 : return "E_UTRA_OPERATING_BAND_73" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_86 : return "E_UTRA_OPERATING_BAND_86" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_53 : return "E_UTRA_OPERATING_BAND_53" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_87 : return "E_UTRA_OPERATING_BAND_87" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_88 : return "E_UTRA_OPERATING_BAND_88" ;
      case telux::tel::RFBand::E_UTRA_OPERATING_BAND_70 : return "E_UTRA_OPERATING_BAND_70" ;
      case telux::tel::RFBand::TDSCDMA_BAND_A : return "TDSCDMA_BAND_A" ;
      case telux::tel::RFBand::TDSCDMA_BAND_B : return "TDSCDMA_BAND_B" ;
      case telux::tel::RFBand::TDSCDMA_BAND_C : return "TDSCDMA_BAND_C" ;
      case telux::tel::RFBand::TDSCDMA_BAND_D : return "TDSCDMA_BAND_D" ;
      case telux::tel::RFBand::TDSCDMA_BAND_E : return "TDSCDMA_BAND_E" ;
      case telux::tel::RFBand::TDSCDMA_BAND_F : return "TDSCDMA_BAND_F" ;
      case telux::tel::RFBand::NR5G_BAND_1 : return "NR5G_BAND_1" ;
      case telux::tel::RFBand::NR5G_BAND_2 : return "NR5G_BAND_2" ;
      case telux::tel::RFBand::NR5G_BAND_3 : return "NR5G_BAND_3" ;
      case telux::tel::RFBand::NR5G_BAND_5 : return "NR5G_BAND_5" ;
      case telux::tel::RFBand::NR5G_BAND_7 : return "NR5G_BAND_7" ;
      case telux::tel::RFBand::NR5G_BAND_8 : return "NR5G_BAND_8" ;
      case telux::tel::RFBand::NR5G_BAND_20 : return "NR5G_BAND_20" ;
      case telux::tel::RFBand::NR5G_BAND_28 : return "NR5G_BAND_28" ;
      case telux::tel::RFBand::NR5G_BAND_38 : return "NR5G_BAND_38" ;
      case telux::tel::RFBand::NR5G_BAND_41 : return "NR5G_BAND_41" ;
      case telux::tel::RFBand::NR5G_BAND_50 : return "NR5G_BAND_50" ;
      case telux::tel::RFBand::NR5G_BAND_51 : return "NR5G_BAND_51" ;
      case telux::tel::RFBand::NR5G_BAND_66 : return "NR5G_BAND_66" ;
      case telux::tel::RFBand::NR5G_BAND_70 : return "NR5G_BAND_70" ;
      case telux::tel::RFBand::NR5G_BAND_71 : return "NR5G_BAND_71" ;
      case telux::tel::RFBand::NR5G_BAND_74 : return "NR5G_BAND_74" ;
      case telux::tel::RFBand::NR5G_BAND_75 : return "NR5G_BAND_75" ;
      case telux::tel::RFBand::NR5G_BAND_76 : return "NR5G_BAND_76" ;
      case telux::tel::RFBand::NR5G_BAND_77 : return "NR5G_BAND_77" ;
      case telux::tel::RFBand::NR5G_BAND_78 : return "NR5G_BAND_78" ;
      case telux::tel::RFBand::NR5G_BAND_79 : return "NR5G_BAND_79" ;
      case telux::tel::RFBand::NR5G_BAND_80 : return "NR5G_BAND_80" ;
      case telux::tel::RFBand::NR5G_BAND_81 : return "NR5G_BAND_81" ;
      case telux::tel::RFBand::NR5G_BAND_82 : return "NR5G_BAND_82" ;
      case telux::tel::RFBand::NR5G_BAND_83 : return "NR5G_BAND_83" ;
      case telux::tel::RFBand::NR5G_BAND_84 : return "NR5G_BAND_84" ;
      case telux::tel::RFBand::NR5G_BAND_85 : return "NR5G_BAND_85" ;
      case telux::tel::RFBand::NR5G_BAND_257 : return "NR5G_BAND_257" ;
      case telux::tel::RFBand::NR5G_BAND_258 : return "NR5G_BAND_258" ;
      case telux::tel::RFBand::NR5G_BAND_259 : return "NR5G_BAND_259" ;
      case telux::tel::RFBand::NR5G_BAND_260 : return "NR5G_BAND_260" ;
      case telux::tel::RFBand::NR5G_BAND_261 : return "NR5G_BAND_261" ;
      case telux::tel::RFBand::NR5G_BAND_12 : return "NR5G_BAND_12" ;
      case telux::tel::RFBand::NR5G_BAND_25 : return "NR5G_BAND_25" ;
      case telux::tel::RFBand::NR5G_BAND_34 : return "NR5G_BAND_34" ;
      case telux::tel::RFBand::NR5G_BAND_39 : return "NR5G_BAND_39" ;
      case telux::tel::RFBand::NR5G_BAND_40 : return "NR5G_BAND_40" ;
      case telux::tel::RFBand::NR5G_BAND_65 : return "NR5G_BAND_65" ;
      case telux::tel::RFBand::NR5G_BAND_86 : return "NR5G_BAND_86" ;
      case telux::tel::RFBand::NR5G_BAND_48 : return "NR5G_BAND_48" ;
      case telux::tel::RFBand::NR5G_BAND_14 : return "NR5G_BAND_14" ;
      case telux::tel::RFBand::NR5G_BAND_13 : return "NR5G_BAND_13" ;
      case telux::tel::RFBand::NR5G_BAND_18 : return "NR5G_BAND_18" ;
      case telux::tel::RFBand::NR5G_BAND_26 : return "NR5G_BAND_26" ;
      case telux::tel::RFBand::NR5G_BAND_30 : return "NR5G_BAND_30" ;
      case telux::tel::RFBand::NR5G_BAND_29 : return "NR5G_BAND_29" ;
      case telux::tel::RFBand::NR5G_BAND_53 : return "NR5G_BAND_53" ;
      case telux::tel::RFBand::NR5G_BAND_46 : return "NR5G_BAND_46" ;
      case telux::tel::RFBand::NR5G_BAND_91 : return "NR5G_BAND_91" ;
      case telux::tel::RFBand::NR5G_BAND_92 : return "NR5G_BAND_92" ;
      case telux::tel::RFBand::NR5G_BAND_93 : return "NR5G_BAND_93" ;
      case telux::tel::RFBand::NR5G_BAND_94 : return "NR5G_BAND_94" ;
      default: return "Invalid band";
   }
}

std::string MyServingSystemHelper::RFBandWidthtoString(telux::tel::RFBandWidth bandWidth) {
   switch (bandWidth){
      case telux::tel::RFBandWidth::INVALID_BANDWIDTH : return "INVALID_BANDWIDTH" ;
      case telux::tel::RFBandWidth::LTE_BW_NRB_6      : return "LTE_BW_NRB_6" ;
      case telux::tel::RFBandWidth::LTE_BW_NRB_15     : return "LTE_BW_NRB_15" ;
      case telux::tel::RFBandWidth::LTE_BW_NRB_25     : return "LTE_BW_NRB_25" ;
      case telux::tel::RFBandWidth::LTE_BW_NRB_50     : return "LTE_BW_NRB_50" ;
      case telux::tel::RFBandWidth::LTE_BW_NRB_75     : return "LTE_BW_NRB_75" ;
      case telux::tel::RFBandWidth::LTE_BW_NRB_100    : return "LTE_BW_NRB_100" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_5     : return "NR5G_BW_NRB_5" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_10    : return "NR5G_BW_NRB_10" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_15    : return "NR5G_BW_NRB_15" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_20    : return "NR5G_BW_NRB_20" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_25    : return "NR5G_BW_NRB_25" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_30    : return "NR5G_BW_NRB_30" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_40    : return "NR5G_BW_NRB_40" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_50    : return "NR5G_BW_NRB_50" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_60    : return "NR5G_BW_NRB_60" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_80    : return "NR5G_BW_NRB_80" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_90    : return "NR5G_BW_NRB_90" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_100   : return "NR5G_BW_NRB_100" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_200   : return "NR5G_BW_NRB_200" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_400   : return "NR5G_BW_NRB_400" ;
      case telux::tel::RFBandWidth::GSM_BW_NRB_2      : return "GSM_BW_NRB_2" ;
      case telux::tel::RFBandWidth::TDSCDMA_BW_NRB_2  : return "TDSCDMA_BW_NRB_2" ;
      case telux::tel::RFBandWidth::WCDMA_BW_NRB_5    : return "WCDMA_BW_NRB_5" ;
      case telux::tel::RFBandWidth::WCDMA_BW_NRB_10   : return "WCDMA_BW_NRB_10" ;
      case telux::tel::RFBandWidth::NR5G_BW_NRB_70    : return "NR5G_BW_NRB_70" ;
      default: return "Bandwidth Info UNAVAILABLE";
    }
}

std::string MyServingSystemHelper::gsmRFBandtoString(telux::tel::GsmRFBand gsmBand) {
   switch (gsmBand){
      case telux::tel::GsmRFBand::GSM_INVALID       : return "GSM_INVALID" ;
      case telux::tel::GsmRFBand::GSM_450           : return "GSM_450" ;
      case telux::tel::GsmRFBand::GSM_480           : return "GSM_480" ;
      case telux::tel::GsmRFBand::GSM_750           : return "GSM_750" ;
      case telux::tel::GsmRFBand::GSM_850           : return "GSM_850" ;
      case telux::tel::GsmRFBand::GSM_900_EXTENDED  : return "GSM_900_EXTENDED" ;
      case telux::tel::GsmRFBand::GSM_900_PRIMARY   : return "GSM_900_PRIMARY" ;
      case telux::tel::GsmRFBand::GSM_900_RAILWAYS  : return "GSM_900_RAILWAYS" ;
      case telux::tel::GsmRFBand::GSM_1800          : return "GSM_1800" ;
      case telux::tel::GsmRFBand::GSM_1900          : return "GSM_1900" ;
      default: return "GSM RF Band UNAVAILABLE";
    }
}

std::string MyServingSystemHelper::wcdmaRFBandtoString(telux::tel::WcdmaRFBand wcdmaBand) {
   switch (wcdmaBand){
      case telux::tel::WcdmaRFBand::WCDMA_INVALID       : return "WCDMA_INVALID" ;
      case telux::tel::WcdmaRFBand::WCDMA_2100          : return "WCDMA_2100" ;
      case telux::tel::WcdmaRFBand::WCDMA_PCS_1900      : return "WCDMA_PCS_1900" ;
      case telux::tel::WcdmaRFBand::WCDMA_DCS_1800      : return "WCDMA_DCS_1800" ;
      case telux::tel::WcdmaRFBand::WCDMA_1700_US       : return "WCDMA_1700_US" ;
      case telux::tel::WcdmaRFBand::WCDMA_850           : return "WCDMA_850" ;
      case telux::tel::WcdmaRFBand::WCDMA_800           : return "WCDMA_800" ;
      case telux::tel::WcdmaRFBand::WCDMA_2600          : return "WCDMA_2600" ;
      case telux::tel::WcdmaRFBand::WCDMA_900           : return "WCDMA_900" ;
      case telux::tel::WcdmaRFBand::WCDMA_1700_JAPAN    : return "WCDMA_1700_JAPAN" ;
      case telux::tel::WcdmaRFBand::WCDMA_1500_JAPAN    : return "WCDMA_1500_JAPAN" ;
      case telux::tel::WcdmaRFBand::WCDMA_850_JAPAN     : return "WCDMA_850_JAPAN" ;
      default: return "WCDMA RF Band UNAVAILABLE";
    }
}

void MyServingSystemHelper::logRFBandList
    (std::shared_ptr<telux::tel::IRFBandList> list, bool isPref) {
    std::vector<telux::tel::GsmRFBand> gsmBands = list->getGsmBands();
    if(!gsmBands.empty()) {
        std::cout << "\n GSM bands are: " << std::endl;
        for (auto gsmBand : gsmBands) {
            std::cout << gsmRFBandtoString(gsmBand) << std::endl;
        }
    }
    std::vector<telux::tel::WcdmaRFBand> wcdmaBands = list->getWcdmaBands();
    if(!wcdmaBands.empty()) {
        std::cout << "\n WCDMA bands are: " << std::endl;
        for (auto wcdmaBand : wcdmaBands) {
            std::cout << wcdmaRFBandtoString(wcdmaBand) << std::endl;
        }
    }
    std::vector<telux::tel::LteRFBand> lteBands = list->getLteBands();
    if(!lteBands.empty()) {
        std::cout << "\n LTE bands are: " << std::endl;
        for (auto lteBand : lteBands) {
            std::cout << "E_UTRA_BAND_" << static_cast<int>(lteBand) << std::endl;
        }
    }
    if (isPref) {
        std::vector<telux::tel::NrRFBand> nrSaBands = list->getNrBands(telux::tel::NrType::SA);
        if(!nrSaBands.empty()) {
            std::cout << "\n NR SA bands are: " << std::endl;
            for (auto nrSaBand : nrSaBands) {
                std::cout << "NR5G_BAND_" << static_cast<int>(nrSaBand) << std::endl;
            }
        }
        std::vector<telux::tel::NrRFBand> nrNsaBands =
                    list->getNrBands(telux::tel::NrType::NSA);
        if(!nrNsaBands.empty()) {
            PRINT_CB << "\n NR NSA bands are: " << std::endl;
            for (auto nrNsaBand : nrNsaBands) {
                std::cout << "NR5G_BAND_" << static_cast<int>(nrNsaBand) << std::endl;
            }
        }
    } else {
        std::vector<telux::tel::NrRFBand> nrBands =
                    list->getNrBands(telux::tel::NrType::COMBINED);
        if(!nrBands.empty()) {
            std::cout << "\n NR bands are: " << std::endl;
            for (auto nrBand : nrBands) {
                std::cout << "NR5G_BAND_" << static_cast<int>(nrBand) << std::endl;
            }
        }
    }
}

void RFBandPrefResponseCallback::rfBandPrefResponse
    (std::shared_ptr<telux::tel::IRFBandList> prefList,
      telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "\n requestRFBandPref is successful.\n RF Band preferences: \n";
      MyServingSystemHelper::logRFBandList(prefList, true);
   } else {
      PRINT_CB << "\n requestRFBandPref failed, ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void RFBandPrefResponseCallback::setRFBandPrefResponse(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "setRFBandPref is successful" << std::endl;
   } else {
      PRINT_CB << "setRFBandPref Request failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void RFBandCapabilityResponseCallback::rfBandCapabilityResponse
    (std::shared_ptr<telux::tel::IRFBandList> capabilityList, telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "\n requestRFBandCapability is successful.\n RF Band Capability: \n";
      MyServingSystemHelper::logRFBandList(capabilityList, false);
   } else {
      PRINT_CB << "\n requestRFBandCapability failed, ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyServingSystemListener::onRFBandInfoChanged(telux::tel::RFBandInfo info) {
   PRINT_NOTIFICATION << " RF Band Info is changed. \n RF Band Info: \n";
   MyServingSystemHelper::logRFBandInfo(info);
}

void MyServingSystemListener::onNetworkRejection(telux::tel::NetworkRejectInfo rejectInfo) {
   PRINT_NOTIFICATION << " Network registration rejection occurred."
            << "\n RAT: "
            << MyServingSystemHelper::getRadioTechnology(rejectInfo.rejectSrvInfo.rat)
            << "\n Service Domain: "
            << MyServingSystemHelper::getServiceDomain(rejectInfo.rejectSrvInfo.domain)
            << "\n Reject cause: " << static_cast<int>(rejectInfo.rejectCause)
            << "\n MCC: " << rejectInfo.mcc << "\n MNC: " << rejectInfo.mnc;
}

void MyServingSystemListener::onCallBarringInfoChanged
   (std::vector<telux::tel::CallBarringInfo> barringInfo) {
   PRINT_NOTIFICATION << " Call barring information changed." << std::endl;
   for (int index = 0; index < barringInfo.size(); index++) {
      PRINT_NOTIFICATION << " RAT: "
         << MyServingSystemHelper::getRadioTechnology(barringInfo[index].rat)
         << ", Service Domain: "
         << MyServingSystemHelper::getServiceDomain(barringInfo[index].domain)
         << ", Call type: "
         << MyServingSystemHelper::getCallBarringType(barringInfo[index].callType)
         << std::endl;
   }
}

void MyServingSystemListener::onSmsCapabilityChanged(telux::tel::SmsCapability smsCapability) {
   PRINT_NOTIFICATION << " SMS capability changed."
            << "\n RAT: "
            << MyServingSystemHelper::getRadioTechnology(smsCapability.rat)
            << ((smsCapability.rat != telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN) ?
               "\n SMS Domain: " : "")
            << ((smsCapability.rat != telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN) ?
                MyServingSystemHelper::getSmsDomain(smsCapability.domain) : "")
            << ((smsCapability.rat == telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN) ?
               "\n SMS Service status: " : "")
            << ((smsCapability.rat == telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN) ?
                MyServingSystemHelper::getNtnSmsStatus(smsCapability.smsStatus) : "")
            << std::endl;
}

void MyServingSystemListener::onLteCsCapabilityChanged(telux::tel::LteCsCapability lteCapability) {
   PRINT_NOTIFICATION << " LTE CS capability changed."
            << "\n LTE CS capability: "
            << MyServingSystemHelper::getLteCsCapability(lteCapability);
}

void MyServingSystemListener::onRFBandPreferenceChanged
    (std::shared_ptr<telux::tel::IRFBandList> prefList) {
   PRINT_NOTIFICATION << " RF Band Preference is changed. \n RF Band Preference: \n";
   MyServingSystemHelper::logRFBandList(prefList, true);
}

// Notify ServingSystemManager subsystem status
void MyServingSystemListener::onServiceStatusChange(telux::common::ServiceStatus status) {
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
    PRINT_NOTIFICATION << " ServingSystem onServiceStatusChange" << stat << "\n";
}
