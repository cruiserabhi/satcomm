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
#ifndef MYSAPCARDLISTENER_HPP
#define MYSAPCARDLISTENER_HPP

#include <chrono>
#include <vector>
#include <string>
#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/tel/CardManager.hpp>
#include <telux/tel/CardDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>

class MySapCommandResponseCallback : public telux::common::ICommandResponseCallback {
public:
   void commandResponse(telux::common::ErrorCode error) override;
};

class MyCardReaderCallback : public telux::tel::ICardReaderCallback {
public:
   void cardReaderResponse(telux::tel::CardReaderStatus responseStatus,
                           telux::common::ErrorCode error) override;
};

class MySapTransmitApduResponseCallback : public telux::tel::ISapCardCommandCallback {
public:
   void onResponse(telux::tel::IccResult result, telux::common::ErrorCode error) override;
};

class MyAtrResponseCallback : public telux::tel::IAtrResponseCallback {
public:
   void atrResponse(std::vector<int> responseAtr, telux::common::ErrorCode error) override;
};

class MySapStateCallback {
public:
   static void sapStateResponse(telux::tel::SapState sapState, telux::common::ErrorCode error);
   static void logSapState(telux::tel::SapState sapState);
};

class MySapCardListener : public telux::tel::ISapCardListener {
public:
   void onServiceStatusChange(telux::common::ServiceStatus status) override;
};

#endif  // MYSAPCARDLISTENER_HPP
