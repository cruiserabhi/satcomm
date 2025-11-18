/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
#ifndef RSPLISTENER_HPP
#define RSPLISTENER_HPP

#include <telux/tel/SimProfileListener.hpp>
#include <telux/tel/SimProfileDefines.hpp>

class RspListener : public telux::tel::ISimProfileListener {
 public:
    void onDownloadStatus(SlotId slotId, telux::tel::DownloadStatus status,
        telux::tel::DownloadErrorCause cause) override;
    void onUserDisplayInfo(SlotId slotId, bool userConsentRequired,
        telux::tel::PolicyRuleMask mask) override;
    void onConfirmationCodeRequired(SlotId slotId, std::string profileName) override;

 private:
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    std::string profileDownloadStatusToString(telux::tel::DownloadStatus status);
    std::string profileDownloadErrorCauseToString(telux::tel::DownloadErrorCause cause);
    std::string pprMaskToString(telux::tel::PolicyRuleMask mask);
};

#endif  // RSPLISTENER_HPP
