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

#ifndef DGNSSMENU_HPP
#define DGNSSMENU_HPP

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/LocationManager.hpp>
#include <telux/loc/DgnssManager.hpp>
#include "../../common/console_app_framework/ConsoleApp.hpp"

using namespace telux::loc;

enum class DgnssSourceType {
    FILE_SOURCE = 0,
    SERVER_SOURCE = 1
};

class NmeaInfoListener : public ILocationListener {
public:
    void onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) override;
    void getNmeaStr(std::string &nmeaGGA);
private:
    std::string lastNmeaGGA_;
    std::mutex m_;
};

class DgnssMenu : public ConsoleApp, public IDgnssStatusListener,
    public std::enable_shared_from_this<DgnssMenu> {
public:
   /**
    * Initialize commands and SDK
    */
   int init(std::shared_ptr<ILocationManager>);

   DgnssMenu(std::string appName, std::string cursor);

   ~DgnssMenu();

   void injectFromFile(std::vector<std::string> userInput);
   void injectFromServer(std::vector<std::string> userInput);
   void onDgnssStatusUpdate(DgnssStatus status) override;

private:
   telux::common::Status initDgnssManager(std::shared_ptr<IDgnssManager> &dgnssManager);
   int waitforSock(int fd);
   int processRtcmFromServer(void);
   int processRtcmFromFile(void);
   int startNmeaReport(uint32_t interval);
   int sendGGAString(void);

   std::shared_ptr<IDgnssManager> dgnssManager_ = nullptr;
   std::shared_ptr<ILocationManager> locationManager_ = nullptr;
   std::shared_ptr<NmeaInfoListener> nmeaInfoListener_ = nullptr;
   int ntcSocketFd_ = -1;
   int dgnssSourceFd_ = -1;
   bool stop_ = false;
   bool reconnect_ = false;
   DgnssSourceType dgnssSourceType_;


};
#endif  // DGNSMENU_HPP
