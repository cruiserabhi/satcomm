/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "MySmsListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MySmsListener::onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> smsMsg) {
   std::cout << std::endl << std::endl;
   std::shared_ptr<telux::tel::MessagePartInfo> partInfo = smsMsg->getMessagePartInfo();
   if (partInfo) {
      PRINT_NOTIFICATION << "Received SMS on phone ID " << phoneId << " from: "
            << smsMsg->getSender() <<  " to: " << smsMsg->getReceiver()
            << "\n Message: " << smsMsg->getText() << "\n PDU: " << smsMsg->getPdu()
            << " \n RefNumber:" << static_cast <int>(partInfo->refNumber) << " NumberOfSegments:"
            << static_cast <int>(partInfo->numberOfSegments) << " SegmentNumber: "
            << static_cast <int>(partInfo->segmentNumber)
            << std::endl;
   } else {
      PRINT_NOTIFICATION << "Received SMS on phone ID " << phoneId << " from: "
            << smsMsg->getSender() <<  " to: " << smsMsg->getReceiver()
            << "\n Message: " << smsMsg->getText() << "\n PDU: " << smsMsg->getPdu()
            << std::endl;
   }

   telux::tel::SmsMetaInfo metaInfo;
   auto status = smsMsg->getMetaInfo(metaInfo);
   if (status == telux::common::Status::SUCCESS) {
      PRINT_NOTIFICATION << " MsgIndex:" << static_cast <int>(metaInfo.msgIndex) << " Tag: "
            << SmsStorageCallback::convertTagTypeToString(metaInfo.tagType) << std::endl;
   }
}

void MySmsListener::onIncomingSms(int phoneId,
   std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs) {
   std::cout << std::endl;

   std::string text = "";
   std::vector<telux::tel::SmsMessage> messages = *(msgs.get());
   if (messages.size() > 1) {
      PRINT_NOTIFICATION << " Consolidated Multipart Message: " << std::endl;
      PRINT_NOTIFICATION << " Count :" << messages.size() << std::endl;
   } else {
      PRINT_NOTIFICATION << " Message: " << std::endl;
      PRINT_NOTIFICATION << " Count :" << messages.size() << std::endl;
   }
   for (telux::tel::SmsMessage smsMsg : messages) {
      text = text + smsMsg.getText();
      std::shared_ptr<telux::tel::MessagePartInfo> partInfo = smsMsg.getMessagePartInfo();
      if (partInfo) {
         std::cout << "\033[1;35mSegment: \033[0m" << static_cast<int>(partInfo->segmentNumber)
                << "\n SMS Part on phone ID " << phoneId << " from: "
                << smsMsg.getSender() <<  " to: " << smsMsg.getReceiver()
                << "\n Message Part: " << smsMsg.getText() << "\n PDU: " << smsMsg.getPdu()
                << "\n RefNumber:" << static_cast <int>(partInfo->refNumber)
                << " NumberOfSegments:"
                << static_cast <int>(partInfo->numberOfSegments) << " SegmentNumber: "
                << static_cast <int>(partInfo->segmentNumber) << std::endl;
      }
      telux::tel::SmsMetaInfo metaInfo;
      auto status = smsMsg.getMetaInfo(metaInfo);
      if (status == telux::common::Status::SUCCESS) {
         std::cout << "\n MsgIndex:" << static_cast <int>(metaInfo.msgIndex) << " Tag: "
               << SmsStorageCallback::convertTagTypeToString(metaInfo.tagType) << std::endl;
      }
   }
   std::cout << "\033[1;35mComplete Message: \033[0m" <<  "\n" << text << std::endl;
}

void MySmsListener::onDeliveryReport(int phoneId, int msgRef, std::string receiverAddress,
   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "Received delivery report from phone ID " << phoneId << " with MsgRef: "
                      << msgRef << " Receiver Address: "<< receiverAddress <<" Error Desc: "
                      << Utils::getErrorCodeAsString(error) << std::endl;
}

void MySmsListener::onMemoryFull(int phoneId, telux::tel::StorageType type) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "Received memory full indication from phone ID " << phoneId <<
       "  for Storage Type: " << SmsStorageCallback::convertStorageTypeToString(type) << "\n";
}

// Notify SmsManager subsystem status
void MySmsListener::onServiceStatusChange(telux::common::ServiceStatus status) {
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
    PRINT_NOTIFICATION << " Sms onServiceStatusChange" << stat << "\n";
}

// Implementation of My SMS callback
void MySmsCommandCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "sendSmsResponse successfully" << std::endl;
   } else {
      PRINT_CB << "sendSmsResponse failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

// Implementation of My SMS callback
void MySmsCommandCallback::sendSmsResponse(std::vector<int> msgRefs,
   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "sendSmsResponse successfully" << std::endl;
      PRINT_CB << " MsgRefs Size: "<< msgRefs.size() << std::endl;
      for (int i: msgRefs) {
         PRINT_CB << " MsgRef : " << i << std::endl;
      }
   } else {
      PRINT_CB << "sendSmsResponse failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

// Implementation of SMSC Address callback
void MySmscAddressCallback::smscAddressResponse(const std::string &address,
                                                telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "requestSmscAddress smscAddressResponse: " << address << std::endl;
   } else {
      PRINT_CB << "requestSmscAddress failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

// Implementation of set SMSC Address callback
void MySetSmscAddressResponseCallback::setSmscResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "setSmscAddress sent successfully" << std::endl;
   } else {
      PRINT_CB << "setSmscAddress failed with errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MySmsDeliveryCallback::commandResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "SMS Delivered successfully" << std::endl;
   } else {
      PRINT_CB << "SMS Delivery failed, errorCode: " << (int)error
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

std::string SmsStorageCallback::convertTagTypeToString(telux::tel::SmsTagType type) {
   switch(type) {
      case telux::tel::SmsTagType::UNKNOWN:
         return "Unknown";
      case telux::tel::SmsTagType::MT_READ:
         return "MT_READ";
      case telux::tel::SmsTagType::MT_NOT_READ:
         return "MT_NOT_READ";
   }
   return "Unknown";
}

std::string SmsStorageCallback::convertStorageTypeToString(telux::tel::StorageType type) {
   switch(type) {
      case telux::tel::StorageType::UNKNOWN:
         return "Unknown";
      case telux::tel::StorageType::NONE:
         return "NONE";
      case telux::tel::StorageType::SIM:
         return "SIM";
   }
   return "Unknown";
}

// Implementation of request message list callback
void SmsStorageCallback::reqMessageListResponse(std::vector<telux::tel::SmsMetaInfo> infos,
   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if (error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Request for message list sent successfully " << "\n";
      PRINT_CB << " SMS List Size: " << infos.size() << "\n";
      for (auto &info : infos) {
         PRINT_CB << " Msg Index: " << info.msgIndex << " Tag Type: "
                  << convertTagTypeToString(info.tagType) << "\n";
      }
   } else {
      PRINT_CB << " Request for message list failed with errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << "\n";
   }
}

// Implementation of read message callback
void SmsStorageCallback::readMsgResponse(telux::tel::SmsMessage smsMsg,
   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if (error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Read message sent successfully " << "\n";
      std::shared_ptr<telux::tel::MessagePartInfo> partInfo = smsMsg.getMessagePartInfo();
      if (partInfo) {
         PRINT_CB << " Multi Part Message " << std::endl;
         PRINT_CB << " Message: " << smsMsg.getText() << "\n PDU: " << smsMsg.getPdu()
               << " \n RefNumber:" << static_cast <int>(partInfo->refNumber) << " NumberOfSegments:"
               << static_cast <int>(partInfo->numberOfSegments) << " SegmentNumber: "
               << static_cast <int>(partInfo->segmentNumber)
               << std::endl;
      } else {
         PRINT_CB << "\n Message: " << smsMsg.getText() << "\n PDU: " << smsMsg.getPdu()
               << std::endl;
      }
   } else {
      PRINT_CB << " Request for read message failed with errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << "\n";
   }
   telux::tel::SmsMetaInfo metaInfo;
   auto status = smsMsg.getMetaInfo(metaInfo);
   if (status == telux::common::Status::SUCCESS) {
      PRINT_CB << " MsgIndex:" << static_cast <int>(metaInfo.msgIndex) << " Tag: "
            << convertTagTypeToString(metaInfo.tagType) << std::endl;
   }
}

// Implementation of delete message callback
void SmsStorageCallback::deleteResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if (error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Delete message successfully " << "\n";
   } else {
      PRINT_CB << " Delete message failed with errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << "\n";
   }
}

// Implementation of request preferred storage callback
void SmsStorageCallback::reqPreferredStorageResponse(telux::tel::StorageType type,
   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if (error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Request for preferred storage sent successfully " << "\n";
      PRINT_CB << " Storage Type: " << convertStorageTypeToString(type) << "\n";
   } else {
      PRINT_CB << " Request for preferred storage failed with errorCode: " <<
         static_cast<int>(error) << ", description: " << Utils::getErrorCodeAsString(error) << "\n";
   }
}

// Implementation of set preferred storage callback
void SmsStorageCallback::setPreferredStorageResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if (error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Set preferred storage successfully " << "\n";
   } else {
      PRINT_CB << " Set preferred storage failed with errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << "\n";
   }
}

// Implementation of set tag callback
void SmsStorageCallback::setTagResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if (error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Set tag successfully " << "\n";
   } else {
      PRINT_CB << " Set tag failed with errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << "\n";
   }
}

// Implementation of request storage details callback
void SmsStorageCallback::reqStorageDetailsResponse(uint32_t maxCount, uint32_t availableCount,
      telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if (error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " SIM Storage details: " << "\n";
      PRINT_CB << " Maximum count of messages allowed: " << maxCount <<
         " Available SIM messages count: " << availableCount <<"\n";
   } else {
      PRINT_CB << " Request for storage details failed with errorCode: " <<
         static_cast<int>(error) << ", description: " << Utils::getErrorCodeAsString(error) << "\n";
   }
}
