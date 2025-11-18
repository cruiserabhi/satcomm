/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LOCATIONMENU_HPP
#define LOCATIONMENU_HPP

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <cstring>
#include <vector>

#include "MyLocationCommandCallback.hpp"
#include "MyLocationListener.hpp"
#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/LocationManager.hpp>

#include "../../common/console_app_framework/ConsoleApp.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

using namespace telux::loc;

class LocationMenu : public ConsoleApp {
public:
   /**
    * Initialize commands and SDK
    */
   int init();

   LocationMenu(std::string appName, std::string cursor);

   ~LocationMenu();

   void registerListenerEx(std::vector<std::string> userInput);
   void deRegisterListenerEx(std::vector<std::string> userInput);
   void startDetailedReports(std::vector<std::string> userInput);
   void startDetailedEngineReports(std::vector<std::string> userInput);
   void startBasicReports(std::vector<std::string> userInput);
   void stopReports(std::vector<std::string> userInput);
   void registerLocationSystemInfo(std::vector<std::string> userInput);
   void deRegisterLocationSystemInfo(std::vector<std::string> userInput);
   void enableDisableTunc(std::vector<std::string> userInput);
   void enableDisablePace(std::vector<std::string> userInput);
   void provideConsentForTerrestrialPositioning(std::vector<std::string> userInput);
   void deleteAllAidingData(std::vector<std::string> userInput);
   void deleteAidingDataWarm(std::vector<std::string> userInput);
   void configureLeverArm(std::vector<std::string> userInput);
   void configureDR(std::vector<std::string> userInput);
   void configureEngineState(std::vector<std::string> userInput);
   void configureEngineIntegrityRisk(std::vector<std::string> userInput);
   void configureConstellation(std::vector<std::string> userInput);
   void configureConstellationEmpty(std::vector<std::string> userInput);
   void configureConstellationDeviceDefault(std::vector<std::string> userInput);
   void configureSecondaryBand(std::vector<std::string> userInput);
   void enableDefaultSecondaryBand(std::vector<std::string> userInput);
   void requestSecondaryBand(std::vector<std::string> userInput);
   void configureRobustLocation(std::vector<std::string> userInput);
   void requestRobustLocation(std::vector<std::string> userInput);
   void requestEnergyConsumedInfo(std::vector<std::string> userInput);
   void getYearOfHw(std::vector<std::string> userInput);
   void getCapabilities(std::vector<std::string> userInput);
   void requestTerrestrialPositioning(std::vector<std::string> userInput);
   void cancelTerrestrialPositioning(std::vector<std::string> userInput);
   void configureNmeaSentence(std::vector<std::string> userInput);
   void configureAllNmeaSentence(std::vector<std::string> userInput);
   void configureMinGpsWeek(std::vector<std::string> userInput);
   void configureMinSVElevation(std::vector<std::string> userInput);
   void requestMinGpsWeek(std::vector<std::string> userInput);
   void requestMinSVElevation(std::vector<std::string> userInput);
   void configureXtraParameters(std::vector<std::string> userInput);
   void requestXtraStatus(std::vector<std::string> userInput);
   void registerConfigListener(std::vector<std::string> userInput);
   void deRegisterConfigListener(std::vector<std::string> userInput);
   void injectMerkleTreeInformation(std::vector<std::string> userInput);
   void configureOsnma(std::vector<std::string> userInput);
   void provideConsentForXtra(std::vector<std::string> userInput);
   int enableReportLogsUtility();
   void enableReportLogs(std::vector<std::string> userInput);
   void enableBasicLocationReportLogs();
   void enableDetailedLocationReportLogs();
   void enableDetailedEngineLocReportLogs();
   void enableLocationSystemInfoLogs();
   void enableSvInfoLogs();
   void enableDataInfoLogs();
   void enableNmeaInfoLogs();
   void enableMeasurementsInfoLogs();
   void enableDisasterCrisisInfoLogs();
   void enableEphemerisInfoLogs();
   void enableEngineNmeaInfoLogs();
   void enableExtendedInfoLogs();
   void dgnssInject(std::vector<std::string> userInput);
   void bodyToSensorUtility(telux::loc::DREngineConfiguration& drConfig);
   void speedScaleUtility(telux::loc::DREngineConfiguration& drConfig);
   void gyroScaleUtility(telux::loc::DREngineConfiguration& drConfig);
   telux::common::Status launchAsRecordingUtility(LocReqEngine engineType);
private:
   telux::common::Status initLocationManager(std::shared_ptr<ILocationManager>
        &locationManager, std::shared_ptr<MyLocationListener> &posListener);
   telux::common::Status initLocationConfigurator(std::shared_ptr<ILocationConfigurator>
        &locationConfigurator);
   void populateXtraConfigParams(telux::loc::XtraConfig &configParams);
   // Member variable to keep the Listener object alive till application ends.
   std::shared_ptr<MyLocationListener> posListener_;
   std::shared_ptr<MyLocationConfigListener> locConfigListener_;
   std::shared_ptr<ILocationManager> locationManager_ = nullptr;
   std::shared_ptr<ILocationConfigurator> locationConfigurator_ = nullptr;
   std::shared_ptr<MyLocationCommandCallback> myLocCmdResponseCb_ = nullptr;
};

#endif  // LOCATIONTESTAPP_HPP
