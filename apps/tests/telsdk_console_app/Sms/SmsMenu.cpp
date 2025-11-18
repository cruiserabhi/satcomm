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
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * SmsMenu provides menu options to invoke SMS functions such as send SMS,
 * receive SMS etc.
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2

#define DELETE_ALL 0
#define DELETE_ALL_MESSAGE_TAG 1
#define DELETE_AT_INDEX 2
#define DEFAULT_INDEX 0

#include "SmsMenu.hpp"

SmsMenu::SmsMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

SmsMenu::~SmsMenu() {
   for (auto index = 0; index < smsManagers_.size(); index++) {
       smsManagers_[index]->removeListener(smsListener_);
   }
   mySmsCmdCb_ = nullptr;
   mySmscAddrCb_ = nullptr;
   smsListener_ = nullptr;
   mySmsDeliveryCb_ = nullptr;
}

bool SmsMenu::init() {
   int noOfSlots = MIN_SIM_SLOT_COUNT;
    if(telux::common::DeviceConfig::isMultiSimSupported()) {
        noOfSlots = MAX_SIM_SLOT_COUNT;
    }
    mySmsCmdCb_ = std::make_shared<MySmsCommandCallback>();
    mySmscAddrCb_ = std::make_shared<MySmscAddressCallback>();
    mySmsDeliveryCb_ = std::make_shared<MySmsDeliveryCallback>();
    smsListener_ = std::make_shared<MySmsListener>();

    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    for (auto index = 1; index <= noOfSlots; index++) {
      std::promise<telux::common::ServiceStatus> prom;
      auto smsMgr = phoneFactory.getSmsManager(index, [&](telux::common::ServiceStatus status) {
          prom.set_value(status);
      });
      if (!smsMgr) {
          std::cout << "ERROR - Failed to get SMS Manager instance \n";
          return false;
      }

      std::cout << " Waiting for SMS Manager to be ready \n";
      telux::common::ServiceStatus smsMgrStatus = prom.get_future().get();
      if (smsMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
          std::cout << "SMS Manager is ready \n";
          auto status = smsMgr->registerListener(smsListener_);
          if(status != telux::common::Status::SUCCESS) {
              std::cout << "ERROR - Failed to register listener \n";
              return false;
          }
          smsManagers_.emplace_back(smsMgr);
      } else {
          std::cout << "ERROR - Unable to initialize SMS Manager \n";
          return false;
      }
    }
   std::shared_ptr<ConsoleAppCommand> sendSmsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Send_SMS", {}, std::bind(&SmsMenu::sendSms, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getSmscAddrCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("2", "Get_SMSC_address", {},
                        std::bind(&SmsMenu::getSmscAddr, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setSmscAddrCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("3", "Set_SMSC_address", {},
                        std::bind(&SmsMenu::setSmscAddr, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getMsgEncodingSizeCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Calculate_message_attributes", {},
         std::bind(&SmsMenu::calculateMessageAttributes, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> sendEnhancedSmsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "5", "Send_Enhanced_SMS", {}, std::bind(&SmsMenu::sendEnhancedSms, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> sendRawSmsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "6", "Send_Raw_SMS", {}, std::bind(&SmsMenu::sendRawSms, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> sendSmsMessageListCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "7", "Send_Request_Message_List", {}, std::bind(&SmsMenu::sendRequestMessageList, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> sendReadMessageCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "8", "Send_Read_Message", {}, std::bind(&SmsMenu::sendReadMessage, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> deleteMessageCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "9", "Delete_Message", {}, std::bind(&SmsMenu::deleteMessage, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestPreferredStorageCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "10", "Request_Preferred_Storage", {}, std::bind(&SmsMenu::requestPreferredStorage, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setPreferredStorageCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "11", "Set_Preferred_Storage", {}, std::bind(&SmsMenu::setPreferredStorage, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setTagCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "12", "Set_Tag", {}, std::bind(&SmsMenu::setTag, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestStorageDetailsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "13", "Request_Storage_Details", {}, std::bind(&SmsMenu::requestStorageDetails, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("14", "Select_sim_slot", {},
                        std::bind(&SmsMenu::selectSimSlot, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListSmsSubMenu
      = {sendSmsCommand, getSmscAddrCommand, setSmscAddrCommand, getMsgEncodingSizeCommand,
         sendEnhancedSmsCommand, sendRawSmsCommand, sendSmsMessageListCommand,
         sendReadMessageCommand, deleteMessageCommand, requestPreferredStorageCommand,
         setPreferredStorageCommand, setTagCommand, requestStorageDetailsCommand};

   if (smsManagers_.size() > 1) {
       commandsListSmsSubMenu.emplace_back(selectSimSlotCommand);
   }

   addCommands(commandsListSmsSubMenu);
   ConsoleApp::displayMenu();
   std::cout << "Device is listening for any incoming messages" << std::endl;
   return true;
}

bool SmsMenu::isDialable (char ch) {
   return ('0' <= ch && ch <= '9') || ch == '*' || ch == '#' || ch == '+';
}

bool SmsMenu::isValidPhoneNumber(std::string address) {
   int count = 0;
   for (char& ch : address) {
      if (!isDialable(ch)) {
         return false;
      }
   }
   return true;
}

// SMS Requests
void SmsMenu::sendSms(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   char delimiter = '\n';

   std::string receiverAddress;
   std::cout << "Enter phone number: ";
   std::getline(std::cin, receiverAddress, delimiter);

   if (!isValidPhoneNumber(receiverAddress)) {
      std::cout << "Invalid Receiver Address \n";
      return;
   }

   std::string message;
   std::cout << "Enter message: ";
   std::getline(std::cin, message, delimiter);

   std::string deliveryReportNeeded;
   do {
      std::cout << "Do you need delivery status (y/n): ";
      std::getline(std::cin, deliveryReportNeeded, delimiter);
      std::transform(deliveryReportNeeded.begin(), deliveryReportNeeded.end(),
         deliveryReportNeeded.begin(), ::tolower);
   } while((deliveryReportNeeded != "y") && (deliveryReportNeeded != "n"));

   telux::common::Status status = telux::common::Status::FAILED;
   if(deliveryReportNeeded == "y") {
        status = smsManager->sendSms(message, receiverAddress, mySmsCmdCb_, mySmsDeliveryCb_);
   } else {
      status = smsManager->sendSms(message, receiverAddress, mySmsCmdCb_);
   }

   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Send SMS request successful\n";
   } else if (status == telux::common::Status::INVALIDPARAM) {
      std::cout << "Entered SMS text is not in UTF-8 encoded format.\n";
   } else {
      std::cout << "Send SMS request failed\n";
   }
}

void SmsMenu::sendEnhancedSms(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   char delimiter = '\n';

   std::string receiverAddress;
   std::cout << "Enter phone number: ";
   std::getline(std::cin, receiverAddress, delimiter);

   if (!isValidPhoneNumber(receiverAddress)) {
      std::cout << "Invalid Receiver Address \n";
      return;
   }

   std::string message;
   std::cout << "Enter message: ";
   std::getline(std::cin, message, delimiter);

   std::string deliveryReportNeeded;
   bool isDeliveryReportNeeded = false;
   do {
      std::cout << "Do you need delivery status (y/n): ";
      std::getline(std::cin, deliveryReportNeeded, delimiter);
      std::transform(deliveryReportNeeded.begin(), deliveryReportNeeded.end(),
         deliveryReportNeeded.begin(), ::tolower);
   } while((deliveryReportNeeded != "y") && (deliveryReportNeeded != "n"));

   telux::common::Status status = telux::common::Status::FAILED;
   if(deliveryReportNeeded == "y") {
      isDeliveryReportNeeded = true;
   } else {
      isDeliveryReportNeeded = false;
   }

   std::string smscAddress;
   std::cout << "Enter SMSC number: ";
   std::getline(std::cin, smscAddress, delimiter);

   status = smsManager->sendSms(message, receiverAddress, isDeliveryReportNeeded,
      MySmsCommandCallback::sendSmsResponse, smscAddress);

   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Send SMS request successful\n";
   } else if (status == telux::common::Status::INVALIDPARAM) {
      std::cout << "Please use Putty with character-set as UTF-8 to provide the input\n";
   } else {
      std::cout << "Send SMS request failed\n";
   }
}

void SmsMenu::sendRawSms(std::vector<std::string> userInput) {

   auto smsManager = smsManagers_[slot_ - 1];
   char delimiter = '\n';
   std::string needMorePdu;
   std::vector<telux::tel::PduBuffer> rawPdus;

   do {
      std::string message;
      std::cout << "Enter raw pdu: ";
      std::getline(std::cin, message, delimiter);
      if (message.empty()) {
          std::cout << " Raw PDU input is empty\n";
          return;
      }

      std::vector<uint8_t> buffer(message.begin(), message.end());
      rawPdus.emplace_back(buffer);

      std::cout << "Do you want to enter more raw Pdu (y/n): ";
      std::getline(std::cin, needMorePdu, delimiter);
      std::transform(needMorePdu.begin(), needMorePdu.end(),
         needMorePdu.begin(), ::tolower);
   } while(needMorePdu == "y");

   if (needMorePdu != "n") {
      std::cout << "Invalid input provided \n";
      return;
   }

   telux::common::Status status = smsManager->sendRawSms(rawPdus,
      MySmsCommandCallback::sendSmsResponse);

   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Send SMS request successful\n";
   } else if(status == telux::common::Status::INVALIDPARAM) {
      std::cout << "Send SMS request failed - Invalid input(s)\n";
   } else {
      std::cout << "Send SMS request failed\n";
   }
}

void SmsMenu::getSmscAddr(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   auto ret = smsManager->requestSmscAddress(mySmscAddrCb_);
   std::cout << (ret == telux::common::Status::SUCCESS ? "Request SmscAddress successful"
                                                       : "Request SmscAddress failed")
             << '\n';
}

void SmsMenu::setSmscAddr(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::cout << "set SMSC Address \n" << std::endl;
   char delimiter = '\n';

   std::string smscAddress;
   std::cout << "Enter SMSC number: ";
   std::getline(std::cin, smscAddress, delimiter);
   auto ret
      = smsManager->setSmscAddress(smscAddress, MySetSmscAddressResponseCallback::setSmscResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Set SmscAddress request success" << std::endl;
   } else {
      std::cout << "Set SmscAddress request failed" << std::endl;
   }
}

std::string SmsMenu::smsEncodingTypeToString(telux::tel::SmsEncoding format) {
   std::string smsFormat = "";
   switch(format) {
      case telux::tel::SmsEncoding::GSM7:
         smsFormat = "GSM7";
         break;
      case telux::tel::SmsEncoding::UCS2:
         smsFormat = "UCS2";
         break;
      case telux::tel::SmsEncoding::GSM8:
         smsFormat = "GSM8";
         break;
      default:
         smsFormat = "UNKNOWN";
   }
   return smsFormat;
}

void SmsMenu::calculateMessageAttributes(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::string smsMessage;
   char delimiter = '\n';

   std::cout << "Enter Message: ";
   std::getline(std::cin, smsMessage, delimiter);

   auto msgAttributes = smsManager->calculateMessageAttributes(smsMessage);
   std::cout
      << "Message Attributes \n encoding: " << smsEncodingTypeToString(msgAttributes.encoding)
      << "\n numberOfSegments: " << msgAttributes.numberOfSegments
      << "\n segmentSize: " << msgAttributes.segmentSize
      << "\n numberOfCharsLeftInLastSegment: " << msgAttributes.numberOfCharsLeftInLastSegment
      << std::endl;
}

void SmsMenu::selectSimSlot(std::vector<std::string> userInput) {
   std::string slotSelection;
   char delimiter = '\n';

   std::cout << "Enter the desired SIM slot (1-Primary, 2-Secondary): ";
   std::getline(std::cin, slotSelection, delimiter);

   if (!slotSelection.empty()) {
      try {
         int slot = std::stoi(slotSelection);
         if (slot > MAX_SLOT_ID || slot < DEFAULT_SLOT_ID) {
            std::cout << "Invalid slot entered, using default slot" << std::endl;
            slot_ = DEFAULT_SLOT_ID;
         } else {
            slot_ = slot;
         }
      } catch (const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
            << slotSelection << std::endl;
         return;
      }
   } else {
      std::cout << "Empty input, enter the correct slot" << std::endl;
   }
}

void SmsMenu::sendRequestMessageList(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::cout << " Request Message List \n" << std::endl;
   char delimiter = '\n';
   std::string tagType;
   std::cout << "Enter SMS tag type : \nUNKNOWN = -1 \nMT_READ = 0 \nMT_NOT_READ = 1";
   std::cout << "\nMO_SENT = 2 \nMO_NOT_SENT = 3 \nChoose type: ";
   std::getline(std::cin, tagType, delimiter);
   int smsTagType = -1;
   try {
      smsTagType = stoi(tagType);
   } catch (const std::exception &e) {
      std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
         << tagType << std::endl;
      return;
   }
   auto ret = smsManager->requestSmsMessageList(static_cast<telux::tel::SmsTagType>(smsTagType),
      SmsStorageCallback::reqMessageListResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Request message list succeeded" << std::endl;
   } else {
      std::cout << "Request message list failed" << std::endl;
   }
}

void SmsMenu::sendReadMessage(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::cout << " Read Message \n" << std::endl;
   char delimiter = '\n';
   std::string messageIndex;
   std::cout << "Enter message index: ";
   std::getline(std::cin, messageIndex, delimiter);
   uint32_t msgIndex = DEFAULT_INDEX;
   try {
      msgIndex = stoi(messageIndex);
   } catch (const std::exception &e) {
      std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
         << messageIndex << std::endl;
      return;
   }
   auto ret = smsManager->readMessage(msgIndex, SmsStorageCallback::readMsgResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Read message request succeeded" << std::endl;
   } else {
      std::cout << "Read message request failed" << std::endl;
   }
}

void SmsMenu::deleteMessage(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::cout << " Delete Message \n" << std::endl;
   char delimiter = '\n';
   std::string delType;
   std::cout << "Enter Delete type : \nDELETE_ALL = 0 \nDELETE_ALL_MESSAGE_TAG = 1";
   std::cout << "\nDELETE_AT_INDEX = 2 \nChoose type: ";
   std::getline(std::cin, delType, delimiter);

   int deleteType = -1;
   int smsTagType = -1;
   uint32_t msgIndex = DEFAULT_INDEX;
   try {
      deleteType = stoi(delType);
      if (deleteType == DELETE_ALL_MESSAGE_TAG) {
         std::string tagType;
         std::cout << "Enter SMS tag type : \nUNKNOWN = -1 \nMT_READ = 0 \nMT_NOT_READ = 1";
         std::cout << "\nMO_SENT = 2 \nMO_NOT_SENT = 3 \nChoose type: ";
         std::getline(std::cin, tagType, delimiter);
         smsTagType = stoi(tagType);
      } else if (deleteType == DELETE_AT_INDEX) {
         std::string messageIndex;
         std::cout << "Enter message index: ";
         std::getline(std::cin, messageIndex, delimiter);
         msgIndex = stoi(messageIndex);
      }
   } catch (const std::exception &e) {
      std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: " << std::endl;
      return;
   }

   telux::tel::DeleteInfo info;
   info.delType = static_cast<telux::tel::DeleteType>(deleteType);
   info.tagType = static_cast<telux::tel::SmsTagType>(smsTagType);
   info.msgIndex = msgIndex;

   auto ret = smsManager->deleteMessage(info, SmsStorageCallback::deleteResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Delete message succeeded" << std::endl;
   } else {
      std::cout << "Delete message failed" << std::endl;
   }
}

void SmsMenu::requestPreferredStorage(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::cout << " Request Preferred Storage \n" << std::endl;
   auto ret = smsManager->requestPreferredStorage(SmsStorageCallback::reqPreferredStorageResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Request preferred storage succeeded" << std::endl;
   } else {
      std::cout << "Request preferred storage failed" << std::endl;
   }
}

void SmsMenu::setPreferredStorage(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::cout << " Set Preferred Storage \n" << std::endl;
   char delimiter = '\n';
   std::string storageType;
   std::cout << "Enter Storage type : \nNONE = 0 \nSIM = 1 \nChoose type: ";
   std::getline(std::cin, storageType, delimiter);
   int type = -1;
   try {
      type = stoi(storageType);
   } catch (const std::exception &e) {
      std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
         << storageType << std::endl;
      return;
   }
   auto ret = smsManager->setPreferredStorage(static_cast<telux::tel::StorageType>(type),
      SmsStorageCallback::setPreferredStorageResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Set Preferred Storage request succeeded" << std::endl;
   } else {
      std::cout << "Set Preferred Storage request failed" << std::endl;
   }
}

void SmsMenu::setTag(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::cout << " Set Tag \n" << std::endl;
   char delimiter = '\n';
   std::string messageIndex;
   std::cout << "Enter message index: ";
   std::getline(std::cin, messageIndex, delimiter);
   uint32_t msgIndex = DEFAULT_INDEX;
   std::string tagType;
   std::cout << "Enter SMS tag type : \nUNKNOWN = -1 \nMT_READ = 0 \nMT_NOT_READ = 1";
   std::cout << "\nMO_SENT = 2 \nMO_NOT_SENT = 3 \nChoose type: ";
   std::getline(std::cin, tagType, delimiter);
   int smsTagType = -1;
   try {
      smsTagType = stoi(tagType);
      msgIndex = stoi(messageIndex);
   } catch (const std::exception &e) {
      std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
         << messageIndex << std::endl;
      return;
   }
   auto ret = smsManager->setTag(msgIndex, static_cast<telux::tel::SmsTagType>(smsTagType),
      SmsStorageCallback::setTagResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Set tag request succeeded" << std::endl;
   } else {
      std::cout << "Set tag request failed" << std::endl;
   }
}

void SmsMenu::requestStorageDetails(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   auto ret = smsManager->requestStorageDetails(SmsStorageCallback::reqStorageDetailsResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Request for SIM storage details succeeded" << std::endl;
   } else {
      std::cout << "Request for SIM storage details failed" << std::endl;
   }
}
