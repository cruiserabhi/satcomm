/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef MYECALLLISTENER_HPP
#define MYECALLLISTENER_HPP

#include <memory>
#include <string>

#include <telux/tel/Call.hpp>
#include <telux/tel/CallListener.hpp>

class MyECallListener : public telux::tel::ICallListener {
   void onIncomingCall(std::shared_ptr<telux::tel::ICall> call) override;
   void onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) override;
   void onECallMsdTransmissionStatus(int phoneId, telux::common::ErrorCode errorCode) override;
   void onECallMsdTransmissionStatus(
      int phoneId, telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus) override;
   void onEmergencyNetworkScanFail(int phoneId) override;
   void onEcbmChange(telux::tel::EcbMode mode) override;
   void onServiceStatusChange(telux::common::ServiceStatus status) override;
   /**
    * Get current time
    */
   std::string getCurrentTime();

   /*
    * Get the call state in string format
    */
   std::string callStateToString(telux::tel::CallState cs);

   std::string callDirectionToString(telux::tel::CallDirection cd);
   /**
    * Get the call end cause in string format from call end cause code
    */
   std::string callEndCauseToString(telux::tel::CallEndCause callEndCause);
   /*
    * Get ECallMsdTransmissionStatus in string
    */
   std::string eCallMsdTransmissionStatusToString(telux::tel::ECallMsdTransmissionStatus status);
   /*
   * Get count of active calls on a slotId.
   */
   int getCallsOnSlot(SlotId slotId);
};

class MyEcbmCallback {
public:
   static void onRequestEcbmResponseCallback(telux::tel::EcbMode ecbMode,
       telux::common::ErrorCode error);
   static void onResponseCallback(telux::common::ErrorCode error);
};
class MyCallCommandCallback : public telux::common::ICommandResponseCallback {
public:
   MyCallCommandCallback(std::string commandName);
   void commandResponse(telux::common::ErrorCode error) override;

private:
   std::string commandName_;
};
#endif  // MYCALLLISTENER_HPP
