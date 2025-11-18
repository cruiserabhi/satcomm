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

#ifndef MYSMSLISTENER_HPP
#define MYSMSLISTENER_HPP

#include <vector>
#include <telux/tel/SmsManager.hpp>

class MySmsListener : public telux::tel::ISmsListener {
public:
   void onServiceStatusChange(telux::common::ServiceStatus status) override;
   void onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> message) override;
   void onIncomingSms(int phoneId, std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs)
      override;
   void onDeliveryReport(int phoneId, int msgRef, std::string receiverAddress,
      telux::common::ErrorCode error) override;
   void onMemoryFull(int phoneId, telux::tel::StorageType type) override;
};

class MySmsCommandCallback : public telux::common::ICommandResponseCallback {
public:
   void commandResponse(telux::common::ErrorCode error) override;
   static void sendSmsResponse(std::vector<int> msgRefs,
      telux::common::ErrorCode errorCode);
};

class MySmscAddressCallback : public telux::tel::ISmscAddressCallback {
public:
   void smscAddressResponse(const std::string &address, telux::common::ErrorCode error) override;
};

class MySetSmscAddressResponseCallback {
public:
   static void setSmscResponse(telux::common::ErrorCode error);
};

class MySmsDeliveryCallback : public telux::common::ICommandResponseCallback {
public:
   void commandResponse(telux::common::ErrorCode error) override;
};

class SmsStorageCallback {
public:
   static void reqMessageListResponse(std::vector<telux::tel::SmsMetaInfo> infos,
      telux::common::ErrorCode errorCode);
   static void readMsgResponse(telux::tel::SmsMessage smsMsg,  telux::common::ErrorCode errorCode);
   static void deleteResponse(telux::common::ErrorCode errorCode);
   static void reqPreferredStorageResponse(telux::tel::StorageType type,
      telux::common::ErrorCode errorCode);
   static void setPreferredStorageResponse(telux::common::ErrorCode errorCode);
   static void setTagResponse(telux::common::ErrorCode errorCode);
   static void reqStorageDetailsResponse(uint32_t maxCount, uint32_t availableCount,
      telux::common::ErrorCode errorCode);
   static std::string convertTagTypeToString(telux::tel::SmsTagType type);
   static std::string convertStorageTypeToString(telux::tel::StorageType type);
};



#endif
