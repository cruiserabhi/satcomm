/*
 *  Copyright (c) 2021,2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file    EcallScanFailHandler.cpp
 *
 * @brief   EcallScanFailHandler handles the emergency network scan fail indication by auto
 *          triggering the high capability switch if the indication received on low
 *          capability sub.
 */

#include <iostream>

#include <telux/tel/PhoneFactory.hpp>

#include "TelClient.hpp"
#include "Utils.hpp"

#define CLIENT_NAME "ECall-EcallScanFailHandler: "

TelClient::EcallScanFailHandler::EcallScanFailHandler(std::weak_ptr<TelClient> telClient)
   : eCallTelClient_(telClient) {
}

TelClient::EcallScanFailHandler::~EcallScanFailHandler() {
   if (multiSimMgr_) {
      multiSimMgr_ = nullptr;
   }
}

telux::common::Status TelClient::EcallScanFailHandler::init() {
   auto sp = eCallTelClient_.lock();
   if (sp) {
      if (sp->callMgr_) {
         auto status = sp->callMgr_->registerListener(shared_from_this());
         if (status != telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << "Failed to register a Call listener\n";
            return telux::common::Status::FAILED;
         } else {
            std::cout << CLIENT_NAME << "Registered a Call listener\n";
         }
      } else {
         std::cout << CLIENT_NAME << "Call manager is NULL\n";
         return telux::common::Status::FAILED;
      }
   } else {
      std::cout << CLIENT_NAME << "init::Obsolete weak pointer\n";
      return telux::common::Status::FAILED;
   }

   std::promise<telux::common::ServiceStatus> prom;
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   multiSimMgr_ = phoneFactory.getMultiSimManager([&](telux::common::ServiceStatus status) {
      prom.set_value(status);
   });
   if (!multiSimMgr_) {
      std::cout << CLIENT_NAME << "ERROR - MultiSimManger is null \n";
      return telux::common::Status::FAILED;
   }
   telux::common::ServiceStatus multiSimMgrStatus = multiSimMgr_->getServiceStatus();
   if (multiSimMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << CLIENT_NAME << "MultiSimManger subsystem is not ready, Please wait \n";
   }
   multiSimMgrStatus = prom.get_future().get();
   if (multiSimMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE ) {
      std::cout << CLIENT_NAME << "MultiSim subsystem is ready\n";
   } else {
      std::cout << CLIENT_NAME << "Unable to initialise MultiSimManger subsystem " << std::endl;
      return telux::common::Status::FAILED;
   }
   return telux::common::Status::SUCCESS;
}

// Handles the response for high capability switch request
void TelClient::EcallScanFailHandler::setHighCapabilityResponse(telux::common::ErrorCode error) {
   if (error == telux::common::ErrorCode::SUCCESS) {
      std::cout << CLIENT_NAME <<"Set high capability request executed successfully\n";
      std::shared_ptr<CallStatusListener> callListener = nullptr;
      auto sp = eCallTelClient_.lock();
      if (!sp) {
         std::cout << CLIENT_NAME << "setHighCapabilityResponse::Obsolete weak pointer\n";
         return;
      }
      //On successful high capability switch, initiate an ecall
      for (auto it = sp->eCallDataMap_.begin(); it != sp->eCallDataMap_.end(); it++) {
         if (it->second.triggerHighCapSwitch &&
            (it->second.msdTransmissionStatus == telux::tel::ECallMsdTransmissionStatus::SUCCESS)) {
            auto status = telux::common::Status::FAILED;
            if (it->second.isCustomNumber) {
               status = sp->startECall(it->first, it->second.msdPdu, it->second.msdData,
                  it->second.category, it->second.dialNumber, it->second.transmitMsd,
                     callListener );
            } else {
               status = sp->startECall(it->first, it->second.msdPdu, it->second.msdData,
                  it->second.category, it->second.variant, it->second.transmitMsd, callListener);
            }
            if (status == telux::common::Status::SUCCESS) {
               std::cout << CLIENT_NAME <<"Initiated an Ecall on slot: " << it->first <<"\n";
            } else {
               std::cout << CLIENT_NAME <<"Failed to initiate an Ecall on slot: "
                  << it->first <<"\n";
            }
         } else {
            std::cout << CLIENT_NAME << "Slot: " << it->first << " MSD transmission status:"
              << static_cast<int>(it->second.msdTransmissionStatus) << " isTriggerHighCapSwitch: "
                 << it->second.triggerHighCapSwitch << "\n";
         }
      }
   } else {
      std::cout << CLIENT_NAME << "Set high capability request failed, errorCode: "
         << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << "\n";
   }
}

// Request to trigger high capability switch
telux::common::Status TelClient::EcallScanFailHandler::setHighCapability(int phoneId) {
   std::cout << CLIENT_NAME << "High capability switch request on phoneId: " << phoneId << "\n";
   if (!multiSimMgr_) {
      std::cout << CLIENT_NAME << "Invalid MultiSim Manager\n";
      return telux::common::Status::FAILED;
   }
   auto setHighCapabilityCallback = [&](telux::common::ErrorCode error) {
        setHighCapabilityResponse(error); };
   auto status = multiSimMgr_->setHighCapability(phoneId, setHighCapabilityCallback);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << CLIENT_NAME << "High capability switch request failed on slot: " << phoneId
         << "\n";
      auto sp = eCallTelClient_.lock();
      if (sp) {
         sp->eCallDataMap_.erase(phoneId);
      } else {
         std::cout << CLIENT_NAME << "setHighCapability::Obsolete weak pointer\n";
      }
   }
   return status;
}

//Handles the response of high capability request
void TelClient::EcallScanFailHandler::requestHighCapabilityResponse(int highCapSlotId,
   telux::common::ErrorCode error) {
   auto sp = eCallTelClient_.lock();
   if (!sp) {
      std::cout << CLIENT_NAME << "requestHighCapabilityResponse::Obsolete weak pointer\n";
      return;
   }
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << CLIENT_NAME <<"High capability is on slot: " << highCapSlotId << "\n";
      // If the emergency network scan fail indication is received on high capability slot only,
      // then no operation needed from the app
      // If the emergency network scan fail indication is received on low capability slot, then
      // 1. Hangup the ecall if the ecall is not hanged up by modem
      //    Note: In a scenario, when emergency network scan fail indication took more time i.e,
      //    more than 30seconds, modem will hangup the ecall once the scan completes.
      // 2. Trigger high capability switch for the same slot
      // 3. Initiate an ecall from app with the cached info if MSD transmission was successful
      int eCallNWScanFailedOnSlot = SlotId::INVALID_SLOT_ID;
      for (auto it = sp->eCallDataMap_.begin(); it != sp->eCallDataMap_.end(); ++it) {
         if (it->second.eCallNWScanFailed) {
            eCallNWScanFailedOnSlot = it->first;
            break;
         }
      }
      //Check for sub capability
      if (eCallNWScanFailedOnSlot != SlotId::INVALID_SLOT_ID) {
         if (highCapSlotId != eCallNWScanFailedOnSlot) {
            sp->eCallDataMap_[eCallNWScanFailedOnSlot].triggerHighCapSwitch = true;
            if (sp->isECallInProgress()) {
               if (sp->eCall_) {
                  int eCallOnSlot = sp->eCall_->getPhoneId();
                  int eCallOnIndex = sp->eCall_->getCallIndex();
                  std::cout << CLIENT_NAME <<"Hanging up the ecall on slot: " << eCallOnSlot
                     << " with index:" << eCallOnIndex << "\n";
                  auto status =  sp->hangup(eCallOnSlot, eCallOnIndex);
                  if (status != telux::common::Status::SUCCESS) {
                     std::cout << CLIENT_NAME << "Failed to Hangup the eCall on slot: "
                        << eCallOnSlot << "\n";
                     return;
                   }
               }
            } else {
               std::cout << CLIENT_NAME << "ECall is already ended\n";
               setHighCapability(eCallNWScanFailedOnSlot);
            }
         } else {
            std::cout << CLIENT_NAME << "ECall is already on high capability slot\n";
            sp->eCallDataMap_.erase(eCallNWScanFailedOnSlot);
            return;
         }
      }
   } else {
      std::cout << CLIENT_NAME << "High capability request failed with errorCode: "
         << static_cast<int>(error) << ", description: " << Utils::getErrorCodeAsString(error)
            << "\n";
      if (sp->isECallInProgress()) {
         if (sp->eCall_) {
            sp->eCallDataMap_.erase(sp->eCall_->getPhoneId());
         }
      }
   }
}

//Request the slot of high capability
telux::common::Status TelClient::EcallScanFailHandler::requestHighCapability() {
   std::cout << CLIENT_NAME << "Request high capability slot info\n";
   if (!multiSimMgr_) {
      std::cout << CLIENT_NAME << "Invalid MultiSim Manager\n";
      return telux::common::Status::FAILED;
   }
   auto requestHighCapabilityCallback = [&](int slotId, telux::common::ErrorCode error) {
      requestHighCapabilityResponse(slotId, error); };
   auto status = multiSimMgr_->requestHighCapability(requestHighCapabilityCallback);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << CLIENT_NAME << "High capability request failed\n";
      return telux::common::Status::FAILED;
   }
   return telux::common::Status::SUCCESS;
}

//Notifies the emergency network scan fail indication
//Assuming the below scenario w.r.t to ecall/emergency call
// 1. We cannot have two ecalls/emergency calls across the device at the same time.
// 2. We cannot have an ecall and an emergency call on same sub at same time.
// 3. We can have one ecall and one emergency call on different sub at the same time.
// 4. When there is an ecall(ex:112) ongoing, we cannot initiate a voice call.
// 5. When there is already a voice call, we can initiate an emergency call(911/112)
//    and voice call goes to hold state.
// 6. When there is a voice call (on hold) and an emergency call (active) already and an emergency
//    network scan fail indication is received on low cap sub then voice call will still be on
//    hold and only ecall has to be hanged up from app.
void TelClient::EcallScanFailHandler::onEmergencyNetworkScanFail(int phoneId) {
   std::cout << "\n";
   std::cout << CLIENT_NAME << "onEmergencyNetworkScanFail called \n"
      << "Network scan completed and no service reported on slotId: " << phoneId << "\n";
   // requestHighCapability response
   auto sp = eCallTelClient_.lock();
   if (sp) {
      sp->eCallDataMap_[phoneId].eCallNWScanFailed = true;
   } else {
      std::cout << CLIENT_NAME << "onEmergencyNetworkScanFail::Obsolete weak pointer\n";
      return;
   }
   //To check which slot is on high capability
   auto status = requestHighCapability();
   if (status != telux::common::Status::SUCCESS) {
      sp = eCallTelClient_.lock();
      if (sp) {
         sp->eCallDataMap_.erase(phoneId);
      } else {
         std::cout << CLIENT_NAME << "onEmergencyNetworkScanFail::Obsolete weak pointer\n";
         return;
      }
   }
}

void TelClient::EcallScanFailHandler::onCallInfoChange(std::shared_ptr<ICall> call) {
   if (NULL == call) {
      std::cout << CLIENT_NAME << "Call object is null\n";
      return;
   }
   if(call->getCallState() == telux::tel::CallState::CALL_ENDED) {
      auto sp = eCallTelClient_.lock();
      if (sp) {
         if(!sp->isECallInProgress()) {
            int phoneId = call->getPhoneId();
            auto it = sp->eCallDataMap_.find(phoneId);
            if (it != sp->eCallDataMap_.end()) {
               if (it->second.triggerHighCapSwitch) {
                  auto status = setHighCapability(phoneId);
                  if (status != telux::common::Status::SUCCESS) {
                     sp->eCallDataMap_.erase(phoneId);
                  }
               }
            } else {
               std::cout << CLIENT_NAME << "No eCall info found corresponding to the slot\n";
               return;
            }
         }
      } else {
         std::cout << CLIENT_NAME << "onCallInfoChange::Obsolete weak pointer\n";
         return;
      }
   }
}
