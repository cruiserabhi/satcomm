/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "SMSTrigger.hpp"
#include <telux/common/Log.hpp>
#include <future>
#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/PhoneFactory.hpp>

SMSTrigger::SMSTrigger(std::shared_ptr<EventManager> eventManager) {
   LOG(DEBUG, __FUNCTION__);
   eventManager_ = eventManager;
}

SMSTrigger::~SMSTrigger() {
   LOG(DEBUG, __FUNCTION__);
}

bool SMSTrigger::init() {
   LOG(DEBUG, __FUNCTION__);

   config_ =  ConfigParser::getInstance();
   loadConfig();
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

   std::string configSlot = config_->getValue("SMS_TRIGGER", "SLOT_ID");
   int slotId = stoi(configSlot);

   if (slotId == MAX_SLOT_ID && !telux::common::DeviceConfig::isMultiSimSupported()) {
      LOG(ERROR, __FUNCTION__, " ERROR - multi sim support not available. slotId = ", configSlot);
      return false;
   }

   std::promise<telux::common::ServiceStatus> prom;
   auto smsMgr = phoneFactory.getSmsManager(slotId, [&](telux::common::ServiceStatus status)
                                             { prom.set_value(status); });
   if (!smsMgr) {
      LOG(ERROR, __FUNCTION__, " ERROR - Failed to get SMS Manager instance slotId = ", slotId);
      return false;
   }

   LOG(DEBUG, __FUNCTION__, " Waiting for SMS Manager to be ready slotId = ", slotId);
   telux::common::ServiceStatus smsMgrStatus = prom.get_future().get();
   if (smsMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      LOG(DEBUG, __FUNCTION__,  " SMS Manager is ready slotId = ", slotId);
      auto status = smsMgr->registerListener(shared_from_this());
      if (status != telux::common::Status::SUCCESS) {
         LOG(ERROR, __FUNCTION__,  " ERROR - Failed to register listener slotId = ", slotId);
         return false;
      }
      smsManager_ = smsMgr;
   } else {
      LOG(ERROR, __FUNCTION__,  " ERROR - Unable to initialize SMS Manager slotId = ", slotId);
      return false;
   }

   return true;
}

void SMSTrigger::onIncomingSms(int phoneId,
                               std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs) {

   LOG(DEBUG, __FUNCTION__, " Consolidated Multipart Message: ");
   std::string text = "";
   std::vector<telux::tel::SmsMessage> messages = *(msgs.get());
   LOG(DEBUG, __FUNCTION__, "Count :", messages.size());

   for (telux::tel::SmsMessage smsMsg : messages) {
      text = text + smsMsg.getText();

      std::shared_ptr<telux::tel::MessagePartInfo> partInfo = smsMsg.getMessagePartInfo();
      if (partInfo) {
         std::string tmpLog = " mSegment: " + std::to_string(partInfo->segmentNumber) +
            "\n SMS Part on phone ID " + std::to_string(phoneId) + " from: " + smsMsg.getSender() +
            " to: " + smsMsg.getReceiver() + "\n Message Part: " + smsMsg.getText() + "\n PDU: " +
            smsMsg.getPdu() + "\n RefNumber:" + std::to_string(partInfo->refNumber) +
            " NumberOfSegments:" + std::to_string(partInfo->numberOfSegments) + " SegmentNumber: " +
            std::to_string(partInfo->segmentNumber);
         LOG(DEBUG, __FUNCTION__, tmpLog);
      }
   }
   LOG(DEBUG, __FUNCTION__, " Complete Message :", text);

   std::async(std::launch::async, [this, text] {
      TcuActivityState tcuActivityState = TcuActivityState::UNKNOWN;
      std::string machineName = ALL_MACHINES;
      if(validateTrigger(text, tcuActivityState, machineName)) {
         this->triggerEvent(tcuActivityState, machineName);
      }
   });
}

void SMSTrigger::onEventRejected(shared_ptr<Event> event, EventStatus reason) {
   LOG(DEBUG, __FUNCTION__, " ", event->toString());
}

void SMSTrigger::onEventProcessed(shared_ptr<Event> event, bool success) {
   LOG(DEBUG, __FUNCTION__, " ", event->toString());
}

void SMSTrigger::triggerEvent(TcuActivityState eventState, std::string machineName) {
   LOG(DEBUG, __FUNCTION__);

   std::shared_ptr<Event> event = std::make_shared<Event>(eventState, machineName,
      TriggerType::SMS_TRIGGER);
   if (event) {
      if (eventManager_) {
         eventManager_->pushEvent(event);
      } else {
         LOG(ERROR, __FUNCTION__, "  event manager is not available ");
      }
   } else {
      LOG(ERROR, __FUNCTION__, " unable to create event");
   }
}

bool SMSTrigger::validateTrigger(std::string text, TcuActivityState& tcuActivityState,
   std::string& machineName) {
   LOG(DEBUG, __FUNCTION__, " ", text);
   // to avoid \n and \ in a string which might lead to not matching trigger text
   text.erase(std::remove(text.begin(), text.end(), '\n'), text.cend());
   text.erase(std::remove(text.begin(), text.end(), '\\'), text.cend());
   size_t deliminatorPosition = 0;
   if(( deliminatorPosition = text.find(MACHINE_NAME_DELIMINATOR)) != std::string::npos ) {
      machineName = text.substr(deliminatorPosition + sizeof(MACHINE_NAME_DELIMINATOR),
         text.length());
      text = text.substr(0, deliminatorPosition);
   }

   if (triggerText_.find(text) == triggerText_.end()) {
      LOG(ERROR, __FUNCTION__, " invalid trigger text, text = ", text);
   } else {
      LOG(INFO, __FUNCTION__, " valid trigger text, text = ", text,
         "\n , machine name = ", machineName);
      tcuActivityState = triggerText_[text];
      return true;
   }
   return false;
}

bool SMSTrigger::loadConfig() {
   LOG(DEBUG, __FUNCTION__);
   std::map<std::string, TcuActivityState> expectedTrigger{
       {TRIGGER_SUSPEND, TcuActivityState::SUSPEND},
       {TRIGGER_RESUME, TcuActivityState::RESUME},
       {TRIGGER_SHUTDOWN, TcuActivityState::SHUTDOWN}};
   try {
      std::string configTriggerText = "";
      for (auto itr = expectedTrigger.begin(); itr != expectedTrigger.end(); ++itr) {
         configTriggerText = config_->getValue("SMS_TRIGGER", itr->first);
         if (!configTriggerText.empty()) {
            if (triggerText_.find(configTriggerText) != triggerText_.end()) {
               LOG(ERROR, __FUNCTION__, " Error : same trigger for multiple state");
               return false;
            }
            triggerText_.insert({configTriggerText, itr->second});
         }
      }
   } catch (const std::invalid_argument& ia) {
      LOG(ERROR, __FUNCTION__, " Error : invalid argument");
      return false;
   }
   return true;
}