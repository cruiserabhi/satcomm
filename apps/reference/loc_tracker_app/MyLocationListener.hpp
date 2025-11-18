/*
 *  Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
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

#ifndef MYLOCATIONLISTENER_HPP
#define MYLOCATIONLISTENER_HPP

#include <vector>

#include <telux/tel/SmsManager.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/LocationListener.hpp>
#include <telux/loc/LocationManager.hpp>

class MyLocationListener : public telux::loc::ILocationListener {
public:
   void onDetailedLocationUpdate(const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) override;

   void onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) override {
   }

   void setRequestReceived(bool requestReceived, std::string senderNumber_);

   MyLocationListener();

   ~MyLocationListener() {
   }

private:
   std::shared_ptr<telux::tel::ISmsManager> smsManager_;
   std::mutex locationLock_;
   bool requestReceived_ = false;
   std::string senderNumber_;
};

class MyLocationCommandCallback : public telux::common::ICommandResponseCallback {
public:
   MyLocationCommandCallback();
   void commandResponse(telux::common::ErrorCode error);
};

#endif  // MYLOCATIONLISTENER_HPP
