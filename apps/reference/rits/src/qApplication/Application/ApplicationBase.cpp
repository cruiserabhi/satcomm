/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
  * @file: Application.cpp
  *
  * @brief: Base class for ITS stack application
  */

#include <ifaddrs.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/time.h>
#include "ApplicationBase.hpp"
#include "VehicleReceive.h"
using std::cout;
using std::string;
using std::map;
using std::pair;
using namespace telux::cv2x::prop;
using telux::cv2x::prop::V2xPropFactory;
using telux::cv2x::prop::CongestionControlUtility;
using telux::sec::ICAControlManager;
using telux::sec::SecurityFactory;
using telux::sec::ICAControlManagerListener;
using telux::sec::LoadConfig;
// Congestion Control related variables
CongestionControlData ApplicationBase::congestionControlOut;
sem_t ApplicationBase::congCtrlCbSem;
shared_ptr<CongestionControlUserData> ApplicationBase::congCtrlCbDataPtr;
CongestionControlCalculations ApplicationBase::congCtrlCbData;
v2x_diag_qits_general_data ApplicationBase::generalInfo;
bool ApplicationBase::cbSuccess;
shared_ptr<telux::cv2x::prop::ICongestionControlManager> ApplicationBase::congestionControlManager;
RadioTransmit* QitsCongCtrlListener::spsTransmit_;
FILE* ApplicationBase::csvfp;
std::mutex ApplicationBase::csvMutex;
std::mutex ApplicationBase::hvLocUpdateMtx;
bool ApplicationBase::securityEnabled;
bool ApplicationBase::congCtrlEnabled;
bool ApplicationBase::positionOverride;
double ApplicationBase::overrideLat;
double ApplicationBase::overrideLong;
double ApplicationBase::overrideHead;
double ApplicationBase::overrideElev;
double ApplicationBase::overrideSpeed;
bool ApplicationBase::writeLogFinish;
bool ApplicationBase::exitApp;
shared_ptr<ILocationInfoEx> ApplicationBase::hvLocationInfo;
shared_ptr<ILocationInfoEx> ApplicationBase::lastLocationInfoIdChange;
bool ApplicationBase::securityInitialized;
unsigned int ApplicationBase::idChangeDistance;
uint64_t ApplicationBase::scheduledIdChangeTime;
int ApplicationBase::signFail;
int ApplicationBase::signSuccess;
bool init_loc = false;
static int idChangeTriggercounter = 0 ;
#define EventBitsShift(bits, shift) \
    (unsigned short)(1 & bits) << static_cast<uint8_t>(shift)

std::string getCurrentTimestamp()
{
    using std::chrono::system_clock;
    auto currentTime = std::chrono::system_clock::now();
    char buffer[MAX_TIMESTAMP_BUFFER_SIZE];
    auto sinceEpoch = currentTime.time_since_epoch().count() / 1000000;
    auto millis = sinceEpoch % 1000;
    std::time_t tt = system_clock::to_time_t ( currentTime );
    auto timeinfo = localtime (&tt);
    int ret = strftime (buffer,80,"%F-%H:%M:%S.",timeinfo);
    snprintf(&buffer[ret], MAX_TIMESTAMP_BUFFER_SIZE, "%03d", (int)millis);
    return std::string(buffer);
}

void locCbFn (shared_ptr<ILocationInfoEx> &locationInfo)
{
    ApplicationBase::setHvLocation(locationInfo);
    // callback will pass data to corresponding other components
    #ifdef AEROLINK
    if(ApplicationBase::securityEnabled){
        Kinematics kine;
        kine.latitude = locationInfo->getLatitude() * 10000000;
        kine.longitude = locationInfo->getLongitude() * 10000000;
        kine.elevation = locationInfo->getAltitude() * 10;
        kine.speed = locationInfo->getSpeed() * (250/18);
        // make sure that aerolink knows most recent ego position and leap seconds
        if(ApplicationBase::securityInitialized){
            int result = AerolinkSecurity::setSecCurrLocation(&kine);
            telux::common::Status status =
                locationInfo->getLeapSeconds(kine.leapSeconds);
            if(status == Status::SUCCESS && kine.leapSeconds != 0){
                result = AerolinkSecurity::setLeapSeconds(kine.leapSeconds);
            }
        }
    }
    #endif
    if(ApplicationBase::congCtrlEnabled){
        Position pos = {0};
        double speed = 0.0;
        if(ApplicationBase::positionOverride){
            pos.posLat = ApplicationBase::overrideLat;
            pos.posLong = ApplicationBase::overrideLong;
            pos.heading = ApplicationBase::overrideHead;
            pos.elev = ApplicationBase::overrideElev;
            speed = ApplicationBase::overrideSpeed;
        }else{
            pos.posLat = (locationInfo->getLatitude());
            pos.posLong = (locationInfo->getLongitude());
            pos.heading = (locationInfo->getHeading());
            pos.elev = (locationInfo->getAltitude());
            speed = locationInfo->getSpeed();
        }
        if(ApplicationBase::congestionControlManager){
            CCErrorCode res =
                ApplicationBase::congestionControlManager->updateHostVehicleData(
                pos, speed);
        }
    }

}

void QitsCongCtrlListener::onCongestionControlDataReady (
    std::shared_ptr<CongestionControlUserData> congestionControlUserData,
        bool critEvent) {

    if(congestionControlUserData){
        QitsCongCtrlListener::updateSpsTransmitFlow(congestionControlUserData);
        memcpy(&ApplicationBase::congCtrlCbData,
                congestionControlUserData->congestionControlCalculations.get(),
                sizeof(CongestionControlCalculations));
        if(!critEvent){
            sem_post(congestionControlUserData->congestionControlSem);
        }
    }
}

unsigned short ApplicationBase::getEventsData(const vehicleeventflags_ut *events) {
    unsigned short eventsData = 0;
    eventsData |= EventBitsShift(
        events->bits.eventAirBagDeployment, event_bits_shift_et::SHIFT_AIRBAGDEPLOYMENT);
    eventsData |= EventBitsShift(
        events->bits.eventDisabledVehicle, event_bits_shift_et::SHIFT_DISABLEDVEHICLE);
    eventsData |= EventBitsShift(
        events->bits.eventFlatTire, event_bits_shift_et::SHIFT_FLATTIRE);
    eventsData |= EventBitsShift(
        events->bits.eventWipersChanged, event_bits_shift_et::SHIFT_WIPERSCHANGED);
    eventsData |= EventBitsShift(
        events->bits.eventLightsChanged, event_bits_shift_et::SHIFT_LIGHTSCHANGED);
    eventsData |= EventBitsShift(
        events->bits.eventHardBraking, event_bits_shift_et::SHIFT_HARDBRAKING);
    eventsData |= EventBitsShift(
        events->bits.eventHazardousMaterials, event_bits_shift_et::SHIFT_HAZARDOUSMATERIALS);
    eventsData |= EventBitsShift(events->bits.eventStabilityControlactivated,
        event_bits_shift_et::SHIFT_STABILITYCONTROLACTIVATED);
    eventsData |= EventBitsShift(
        events->bits.eventTractionControlLoss, event_bits_shift_et::SHIFT_TRACTIONCONTROLLOSS);
    eventsData |= EventBitsShift(
        events->bits.eventABSactivated, event_bits_shift_et::SHIFT_ABSACTIVATED);
    eventsData |= EventBitsShift(
        events->bits.eventStopLineViolation, event_bits_shift_et::SHIFT_STOPLINEVIOLATION);
    eventsData |= EventBitsShift(
        events->bits.eventHazardLights, event_bits_shift_et::SHIFT_HAZARDLIGHTS);
    return eventsData;
}

void ApplicationBase::fillEventsData(v2x_diag_event_bit_t *eventBit,
    const vehicleeventflags_ut *events) {
    eventBit->eventAirBagDeployment = events->bits.eventAirBagDeployment;
    eventBit->eventDisabledVehicle = events->bits.eventDisabledVehicle;
    eventBit->eventFlatTire = events->bits.eventFlatTire;
    eventBit->eventWipersChanged = events->bits.eventWipersChanged;
    eventBit->eventLightsChanged = events->bits.eventLightsChanged;
    eventBit->eventHardBraking = events->bits.eventHardBraking;
    eventBit->eventHazardousMaterials = events->bits.eventHazardousMaterials;
    eventBit->eventStabilityControlactivated = events->bits.eventStabilityControlactivated;
    eventBit->eventTractionControlLoss = events->bits.eventTractionControlLoss;
    eventBit->eventABSactivated = events->bits.eventABSactivated;
    eventBit->eventStopLineViolation = events->bits.eventStopLineViolation;
    eventBit->eventHazardLights = events->bits.eventHazardLights;
    eventBit->unused = events->bits.unused;
}

void ApplicationBase::diagLogPktTxRx(bool isTx, TransmitType txType,
    const DiagLogData *logData, const bsm_data *bs) {
    if (!logData) {
        std::cout << "logData is null" << std::endl;
        return;
    }

    if (!bs) {
        std::cout << "bsm_data is null" << std::endl;
        return;
    }

    // fill location and athletic info
    v2x_diag_bsm_data bsm_info = {0};
    bsm_info.msg_count = bs->MsgCount;
    bsm_info.temp_id = bs->id;
    bsm_info.secmark_ms = bs->secMark_ms;
    bsm_info.latitude = bs->Latitude;
    bsm_info.longitude = bs->Longitude;
    bsm_info.semi_major_dev = bs->SemiMajorAxisAccuracy;
    bsm_info.speed = bs->Speed;
    bsm_info.heading = bs->Heading_degrees;
    bsm_info.long_accel = bs->AccelLon_cm_per_sec_squared;
    bsm_info.lat_accel = bs->AccelLat_cm_per_sec_squared;

    // fill other general
    generalInfo.time_stamp_log = timestamp_now();
    generalInfo.time_stamp_msg = logData->currTime;
    generalInfo.gnss_time = 0;
    generalInfo.CPU_Util = (uint32_t)(get_CPU_percentage(logData->monotonicTime) * 100.0);
    generalInfo.GPS_mode = 0;
    generalInfo.msg_valid = logData->validPkt;
    fillEventsData(&(generalInfo.events), &(bs->events));
    generalInfo.hysterisis = 5;
    generalInfo.L2_ID = logData->cbr;

    v2x_diag_transmit_type_et msg_type = txType == TransmitType::SPS ? DIAG_SPS : DIAG_EVENT;
    bool congCtrlPrepared = logData->enableCongCtrl && logData->congCtrlInitialized;
    if (isTx) {
        // tx
        v2x_qits_general_tx_info info = {0};
        V2X_QITS_GENERAL_TX_PKG *msg = (V2X_QITS_GENERAL_TX_PKG *)((uint32_t *)(&info) + 1);
        if (congCtrlPrepared) {
            generalInfo.tracking_error = congCtrlCbData.trackingError ?
                (uint32_t)(congCtrlCbData.trackingError * 100) : 0;
            generalInfo.vehicle_density_in_range = (uint32_t)(congCtrlCbData.smoothDens * 100.0);
            msg->channel_quality_indication = congCtrlCbData.channData ?
                (uint32_t)(congCtrlCbData.channData->channQualInd * 100.0) : 0;
            generalInfo.max_ITT = congCtrlCbData.maxITT;
        }
        msg->bsm_data = bsm_info;
        msg->general_data = generalInfo;
        msg->tx_interval = logData->txInterval;
        msg->DCC_random_time = (long unsigned int)0;
        msg->msg_type = msg_type;
        utility_->sendLogPacket(&info, PKT_ID_QITS_TX_FLOW);
    } else {
        // rx
        v2x_qits_general_rx_info info = {0};
        V2X_QITS_GENERAL_RX_PKG *msg = (V2X_QITS_GENERAL_RX_PKG *)((uint32_t *)(&info) + 1);
        if (congCtrlPrepared) {
            generalInfo.tracking_error = congCtrlCbData.trackingError ?
                (uint32_t)(congCtrlCbData.trackingError * 100) : 0;
            generalInfo.vehicle_density_in_range = (uint32_t)(congCtrlCbData.smoothDens * 100.0);
            generalInfo.max_ITT = congCtrlCbData.maxITT;
            msg->total_RVs = congCtrlCbData.totalRvsInRange;
            msg->distance_from_RV = bs->distFromRV;
        }
        msg->bsm_data = bsm_info;
        msg->general_data = generalInfo;
        msg->msg_type = msg_type;
        utility_->sendLogPacket(&info, PKT_ID_QITS_RX_FLOW);
    }
}

void ApplicationBase::diagLogPktGenericInfo() {
    v2x_qits_general_periodic_info info = {0};
    V2X_QITS_GENERAL_PERIODIC_PKG *msg = (V2X_QITS_GENERAL_PERIODIC_PKG *)((uint32_t *)(&info) + 1);
    bool congCtrlPrepared = this->configuration.enableCongCtrl && congCtrlInitialized;
    if (congCtrlPrepared) {
        msg->max_ITT = congCtrlCbData.maxITT;
        msg->vehicle_density_in_range = (uint32_t)(congCtrlCbData.smoothDens * 100.0);
        msg->total_RVs = congCtrlCbData.totalRvsInRange;
        msg->tracking_error = congCtrlCbData.trackingError;
    }

    msg->CPU_Util = generalInfo.CPU_Util;
    msg->L2_ID = generalInfo.L2_ID;
    msg->events = generalInfo.events;

    utility_->sendLogPacket(&info, PKT_ID_QITS_GENERIC_INFO);
}

void ApplicationBase::setHvLocation(shared_ptr<ILocationInfoEx>& hvLocationInfoIn){
    // need a lock here
    lock_guard<mutex> lk(hvLocUpdateMtx);
    ApplicationBase::hvLocationInfo = hvLocationInfoIn;
#if AEROLINK
    if(!init_loc){
        lastLocationInfoIdChange = hvLocationInfo;
        init_loc = true;
    }
#endif
}


void ApplicationBase::writeSecurityLog(char* tmpLogStr, uint32_t maxBufSize, FILE *myfp){
    // can pass mbd, signing, and verif stats here and other settings in future
}

/* Log Format:
 * TimeStamp    TimeStamp_ms    Time_monotonic
 * LogRecType   L2 ID    CBR Percent    CPU_Util
 * TXInterval   msgCnt  TempId  GPGSAMode
 * secMark  lat long    semi_major_dev  speed
 * heading  longAccel   latAccel    Tracking_Error
 * vehicleDensityInRange    ChannelQualityIndication
 * BSMValid max_ITT GPS-Time    Events  DCC random time Hysterisis
 */
void ApplicationBase::writeCongCtrlLog(char* tmpLogStr, uint32_t maxBufSize, FILE *myfp,
    CongestionControlCalculations* congestionControlCalculations, bool validPkt,
    uint16_t eventsData) {
    if (!congestionControlCalculations) {
        std::cerr << "Invalid congestionControl output struct provided\n";
        return;
    }

    if(tmpLogStr == nullptr){
        std::cerr << "Invalid input buffer\n";
        return;
    }

    char* tmpPtr = tmpLogStr;
    char* const endBuf = tmpPtr + maxBufSize;

    if (congestionControlCalculations->trackingError) {
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
            congestionControlCalculations->trackingError);
    }
    else {
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "0.0,");
    }
    tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
        congestionControlCalculations->smoothDens);


    if (congestionControlCalculations->channData) {
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
            congestionControlCalculations->channData->channQualInd);
    }
    else {
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "0.0,");
    }
    tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%d,",
        validPkt  ? 1: 0);
    tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%" PRIu64 ",",
        congestionControlCalculations->maxITT);

    // gps time, event, random time
    tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
        0.0);

    tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%u,", eventsData);

    //sps enhancement data
    if (congestionControlCalculations->spsEnhanceData) {
        // random time - todo
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%lu,%d",
            (long unsigned int)0, 5);
    }
    else {
       tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr,  "%lu,%d",
            (long unsigned int)0, 5);
    }
}

// first need to call setup function to initialize the lcm id change
// then periodically call "idChange" and check return value and updates in idChangeData
void ApplicationBase::changeIdentity(sem_t* idChangeCbSem){
    // pseudonym cert change
    exitApp = false;
    uint64_t currTime, timeSinceLastIdChange;
    double hvLatNew, hvLonNew, hvLatOld, hvLonOld;
    unsigned int distSinceLastIdChange;
    if(lastIdChangeTime == 0){
        lastIdChangeTime = timestamp_now();
    }
    if(!exitApp)
    {
        // wait for distance requirement
        // if an event is happening, cert change must not happen
        sem_wait(&idChangeData.idSem);
        if(!criticalState){
            // here we need to check for two things:
            // interval has passed and distance has been covered since last id change
            currTime = timestamp_now();
            if(currTime > lastIdChangeTime) {
                timeSinceLastIdChange = currTime - lastIdChangeTime;
            }else{
                timeSinceLastIdChange = 0;
            }
            {
                lock_guard<mutex> lk(hvLocUpdateMtx);
                hvLatNew = (hvLocationInfo->getLatitude());
                hvLonNew = (hvLocationInfo->getLongitude());
            }
            hvLatOld = lastLocationInfoIdChange->getLatitude();
            hvLonOld = lastLocationInfoIdChange->getLongitude();
            // check if there have been any fixes yet
            if(init_loc){
                distSinceLastIdChange =
                     bsmCompute2dDistance(hvLatOld, hvLonOld, hvLatNew, hvLonNew);
            }else{
                distSinceLastIdChange = 0;
            }
            // check if both conditions satisfied
            if(timeSinceLastIdChange >= configuration.idChangeInterval &&
                distSinceLastIdChange >= ApplicationBase::idChangeDistance){
                // perform id change and record current position and time
                int ret = SecService->idChange();

                if( ret < 0 ){
                    if(appVerbosity > 1) {
                        fprintf(stderr,"Id Change Failure\n");
                    }
                }
                else{
                    if(appVerbosity > 7 ){
                        printf("Id Change Init Call Success\n");
                        std::cout << "Time is: " << lastIdChangeTime << "\n";
                        std::cout << "Position is, lat: " << hvLatNew << ", lon: "
                            << hvLonNew << "\n";
                        std::cout << "Current time: " << currTime << "\n";
                        std::cout << "Last id change time: " << lastIdChangeTime << "\n";
                        std::cout << "Time since last id change: "
                            << timeSinceLastIdChange << "\n";
                        std::cout << "Config id change interval: " <<
                            configuration.idChangeInterval <<"\n";
                        std::cout << "Distance since last id change " <<
                            distSinceLastIdChange << "\n";
                        std::cout << "Config id change distance: " <<
                            ApplicationBase::idChangeDistance << "\n";
                    }
                    // wait for the callback to complete
                    if(idChangeCbSem != nullptr){
                        sem_wait(idChangeCbSem);
                    }
                    // if not simulation, perform l2 src randomization
                    if (!this->isTxSim) { // radio
                        for(int index = 0 ; index < spsTransmits.size(); index++){
                            this->spsTransmits[index].updateSrcL2();
                        }
                    }
                }

                lastIdChangeTime = timestamp_now();
                lastLocationInfoIdChange = hvLocationInfo;
            }
        }
        sem_post(&idChangeData.idSem);
    }
}

void ApplicationBase::updateL2RvMap(uint32_t l2SrcId, rv_specs* rvSpec) {

    lock_guard<mutex> lk(l2MapMtx);
    this->l2RvMap[l2SrcId] = *rvSpec;
}

uint32_t ApplicationBase::vehiclesInRange() {
    lock_guard<mutex> lk(l2MapMtx);
    return this->l2RvMap.size();
}

void ApplicationBase::setL2RvFilteringList(int rate) {
    if (appVerbosity > 5) {
        std::cout << "L2 list filtering rate is" << rate << std::endl;
    }
    //Assuming RV are sending at 10 HZ, find the no. of vehicles to filter
    //Should we make this divisor dynamic?
    int vehsToFilter = rate / 10;
    int vehsFiltered = 0;
    lock_guard<mutex> lk(l2MapMtx);
    std::vector<L2FilterInfo> rvListToFilter;

    for (pair<uint32_t, rv_specs> element : this->l2RvMap) {
        L2FilterInfo rvSrc = {0};
        rvSrc.srcL2Id = element.first;
        rvSrc.pppp = 0;
        rvSrc.durationMs = this->configuration.l2FilteringTime;
        const auto now = timestamp_now();
        const auto diff = now - element.second.hv_timestamp_ms;
        //Remove the L2 id entery for Rv that has not sent a message in a long time.
        if (this->configuration.l2IdTimeThreshold * 10000 < diff) {
            if (appVerbosity > 5)
                cout << "Removing L2 id:" << element.first << endl;
            this->l2RvMap.erase(element.first);
        } else {
            //RV is out of HV zone
            if (element.second.out_of_zone == true) {
                rvListToFilter.push_back(rvSrc);
                vehsFiltered++;

            }
            //RV ttc is greater than max ttc and it's not deacclerating. If a car ahead of us
            //start deacc chances are it might crash with us.
            else if ((element.second.ttc >= 10000) && (element.second.rapid_decl == false)) {
                rvListToFilter.push_back(rvSrc);
                vehsFiltered++;
            }

            //RV behind us are deacc then we can filter it
            else if ((element.second.ttc >= 10000) && (element.second.rapid_decl == true) &&
                    (element.second.lt == SAME_LANE_BACK_SAMEDIR || element.second.lt ==
                    ADJLEFT_LANE_BACK_SAMEDIR ||
                    element.second.lt == ADJRIGHT_LANE_BACK_SAMEDIR)) {
                rvListToFilter.push_back(rvSrc);
                vehsFiltered++;
            }

            //RV is stopped and not in the same lane ahead
            else if (element.second.stopped == true && element.second.lt != 1) {
                rvListToFilter.push_back(rvSrc);
                vehsFiltered++;
            }

            //After checking the cases, check if the no of RV to filter is met.
            if (vehsFiltered == vehsToFilter) {
                radioReceives[0].setL2Filters(rvListToFilter);
                break;
            }
        }
    }
}

ApplicationBase::ApplicationBase(char* fileConfiguration, MessageType msgType,
    bool enableCsvLog, bool enableDiagLog){
    generalInfo = {0};
    if (enableDiagLog) {
        enableDiagLog_ = enableDiagLog;
        if (!utility_) {
            utility_ = std::make_shared<QUtils>();
        }
        utility_->initDiagLog();
    }
    enableCsvLog_ = enableCsvLog;
    exitApp = false;
    MsgType = msgType;
    currVehState = nullptr;
    signSuccess = 0;
    signFail = 0;
    // set parameters according to config file
    loadConfiguration(fileConfiguration);
}

ApplicationBase::ApplicationBase(const string txIpv4, const uint16_t txPort,
            const string rxIpv4, const uint16_t rxPort,
            char* fileConfiguration, bool enableCsvLog, bool enableDiagLog) {
    generalInfo = {0};
    if (enableDiagLog) {
        enableDiagLog_ = enableDiagLog;
        if (!utility_) {
            utility_ = std::make_shared<QUtils>();
        }
        utility_->initDiagLog();
    }
    enableCsvLog_ = enableCsvLog;
    exitApp = false;
    currVehState = nullptr;
    signSuccess = 0;
    signFail = 0;
    if (this->loadConfiguration(fileConfiguration)) {
        return;
    }

    if (txPort)
    {
        this->simTxSetup(txIpv4, txPort);
        this->isTxSim = true;
    }
    if (rxPort) {
        this->simRxSetup(rxIpv4, rxPort);
        this->isRxSim = true;
        // check if we want to also send packets while we receive over Ethernet
        if(this->configuration.enableTxAlways){
            if(this->configuration.tx_port &&
                    !this->configuration.ipv4_dest.empty()){
                printf("Attempting RX and TX over Ethernet at same time\n");
                this->simTxSetup(this->configuration.ipv4_dest,
                        this->configuration.tx_port);
                this->isTxSim = true;
            }else{
                // turn the flag off so that driver program knows
                printf("Please provide TX Port and Dest IP in config file\n");
                printf("Entering only RX mode\n");
                this->configuration.enableTxAlways = false;
            }
        }
    }
}

bool ApplicationBase::init() {
    if(configuration.enableL2Filtering) {
        cv2xTmListener = std::make_shared<Cv2xTmListener>(appVerbosity);
    }
    // set up kinematics listener
    if(configuration.enableLocationFixes){
        if (appVerbosity > 5){
            std::cout << "Enabling location fixes\n";
        }
        appLocListener_ = make_shared<LocListener>();
        appLocListener_->setLocCbFn(&locCbFn);
        locListeners.push_back(appLocListener_);
        kinematicsReceive = std::make_shared<KinematicsReceive>
                (locListeners, this->configuration.locationInterval);
    }
    if (!(isTxSim || isRxSim)) {
        // setup radio flows
        if (0 != setup(MsgType, false)) {
            printf("radio setup failed\n");
            return false;
        }
        // one-time initialization for security ; if any
        if (this->configuration.enableSecurity == true) {
        #ifdef AEROLINK
            try{
              // LCM Constructor for Aerolink
              idChangeData.idChangeCbSem = &idChangeCbSem;
              sem_init(idChangeData.idChangeCbSem, 0, 1);
              if(!this->configuration.lcmName.empty() && this->configuration.idChangeInterval){
                  SecService = unique_ptr<SecurityService>(AerolinkSecurity::Instance(
                          configuration.securityContextName,
                          configuration.securityCountryCode,
                          configuration.lcmName.c_str(),
                          std::ref(idChangeData)
                          ));
              }else{
                  // Non-LCM Constructor for Aerolink
                  SecService = unique_ptr<SecurityService>(AerolinkSecurity::Instance(
                          configuration.securityContextName,
                          configuration.securityCountryCode));
              }
              ApplicationBase::securityInitialized = true;
              // set the verbosity of aerolink
              SecService->setSecVerbosity(this->configuration.secVerbosity);
            }catch(const std::runtime_error& error){
                fprintf(stderr, "Aerolink init failed: Please check config params \n");
                fprintf(stderr, "Attempting to close all radio flows\n");
                prepareForExit();
                closeAllRadio();
                exit(0);
            }
        #else
            // If no Aerolink security library is specified
            SecService = unique_ptr<NullSecurity>(NullSecurity::Instance(
                        configuration.securityContextName,
                        configuration.securityCountryCode));
        #endif
        }

        if (configuration.enableL2FloodingDetect) {
            // if flooding mitigation enabled
            // if configuration.floodingMitigationEnabled
            // setup telux security service to get the mvm stats
            auto& secFactory = SecurityFactory::getInstance();
            telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;
            caControlMgr = secFactory.getCAControlManager(ec);
            cacMgrListr = std::make_shared<CaControlManagerListener>();
            LoadConfig loadConfig = {0};
            loadConfig.calculationInterval = configuration.loadUpdateInterval; // 1000 ms
            ec = caControlMgr->registerListener(cacMgrListr);
            ec = caControlMgr->startMonitoring(loadConfig);
        }
    } else {
        if (this->configuration.enableSecurity == true) {
#ifdef AEROLINK
            SecService = unique_ptr<SecurityService>(AerolinkSecurity::Instance(
                         configuration.securityContextName,
                         configuration.securityCountryCode));
#else
            SecService = unique_ptr<NullSecurity>(NullSecurity::Instance(
                         configuration.securityContextName,
                         configuration.securityCountryCode));
#endif
        }
    }

    sem_init(&this->rx_sem, 0, 1);
    sem_init(&this->log_sem, 0, 1);
    sem_init(&idChangeData.idSem, 0, 1);
    cb =
        [this](bool emergent,
               const current_dynamic_vehicle_state_t* const vehicle_state = nullptr) {
            vehicleEventReport(emergent, vehicle_state);
    };

    if (configuration.enableVehicleDataCallbacks) {
        VehRec.enableVehicleReceive(cb);
    }

    if (configuration.qMonEnabled && qMonConfig) // Add to config
    {
        qMon = std::make_shared<QMonitor>(*qMonConfig);
    }

    //init messages for sending.
    if (isTxSim) {
        if (!initMsg(txSimMsg)) {
            return false;
        }
    }
    for (auto mc : eventContents) {
        if (!initMsg(mc)) {
            return false;
        }
    }
    for (auto mc : spsContents) {
        if (!initMsg(mc)) {
            return false;
        }
    }

    if (isRxSim) {
        if (!initMsg(rxSimMsg, true)) {
            return false;
        }
    }

    for (auto mc : receivedContents) {
        if (!initMsg(mc)) {
            return false;
        }
    }

    return true;
}

ApplicationBase::~ApplicationBase() {
    if(appVerbosity){
        std::cout << "ApplicationBase destructing" << std::endl;
    }

    // call prepare for exit here again in case it wasn't called previously
    prepareForExit();

    if(SecService){
        SecService->lockIdChange();
        SecService->deinit();
        SecService.reset();
    }
    if (enableDiagLog_ && utility_) {
        utility_->deInitDiagLog();
    }

    if (ldm) {
        delete ldm;
        ldm = nullptr;
    }
    {
        std::unique_lock<std::mutex> loc(stateMtx);
        exitApp = true;
        stateCv.notify_all();
        if(nullptr != this->currVehState){
            free(currVehState);
        }
    }

    {
       std::unique_lock<std::mutex> lock(csvMutex);
       if (nullptr != csvfp && !configuration.enableAsync) {
           writeMutexCv.wait(lock, []{ return writeLogFinish; });
           fclose(csvfp);
           csvfp = nullptr;
       }
    }
}

void ApplicationBase::detectFloodAndMitigate(bool& stateOn,
    std::vector<L2FilterInfo>* rvListToFilter){

    if(rvListToFilter == NULL){
        std::cout << "NULL struct for l2 filter info list passed\n";
        return;
    }
    // reset to empty
    rvListToFilter->clear();

    // monitor mvm load and l2 src addresses
    // cacMgrListr
    telux::common::ErrorCode ec = caControlMgr->getCapacity(currCapacity);

    if(ec != telux::common::ErrorCode::SUCCESS){
        std::cerr << "Error attempting to get mvm capacity \n";
        return;
    }

    rv_specs* rvsp;
    uint32_t filteringTime = 0; // util is less than the threshold
    L2FilterInfo rvSrc = {0};

    if(configuration.mvmCapacityOverride){
        currCapacity.nist256 = this->configuration.mvmCapacity;
        if(configuration.floodDetectVerbosity > 3){
            std::cout << "Using provided mvm capacity " << currCapacity.nist256 << "\n";
        }
    }else{
        telux::common::ErrorCode ec = caControlMgr->getCapacity(currCapacity);
        if(configuration.floodDetectVerbosity > 3){
            std::cout << "Using the actual mvm capacity " << currCapacity.nist256 << "\n";
        }
    }

    currUtil = (double)(cacMgrListr->currLoad.nist256) / (double)(currCapacity.nist256);
    if(configuration.floodDetectVerbosity > 0){
        std::cout << "Load for NIST is: " << cacMgrListr->currLoad.nist256 << "\n";
        std::cout << "Capacity for NIST is: " << currCapacity.nist256 << "\n";
        std::cout << "Curr Utilization for NIST is: " << currUtil << "\n";
    }
    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    uint64_t currTime_ms = (currTime.tv_sec * 1000) + (currTime.tv_usec / 1000);
    for(pair<uint32_t, rv_specs> l2RvElement : this->l2RvMap){
        // determine the incoming rate per l2 src addr
        rvsp = &l2RvElement.second;
        if(rvsp->totalCnt > 1 && rvsp->lastTotalCnt < rvsp->totalCnt){
            if(configuration.floodDetectVerbosity > 7){
                std::cout << "This rv is: " << l2RvElement.first << "\n";
                std::cout << "Last msg count field for this RV is: " << rvsp->lastCnt << "\n";
                std::cout << "Last rx time for this RV is: " << rvsp->lastTime <<"\n";
                std::cout << "Total number of rxed packets from this RV is " << rvsp->totalCnt << "\n";
                std::cout << "The last count for this rv is: " << rvsp->lastTotalCnt << "\n";
                std::cout << "Flooding nist msg rate threshold is: " <<
                    (configuration.mvmUtilThreshold *  currCapacity.nist256) << "\n";
                std::cout << " Threshold for flood attack single " <<
                    configuration.floodAttackThreshSingle << " \n";
                std::cout << " Threshold for mvm util to be attack is " <<
                    (configuration.mvmUtilThreshold)  << "\n";
            }

            // calculate the rate every 1000 ms?
            if(rvsp->totalCnt > 1 && rvsp->lastTotalCnt < rvsp->totalCnt){
                // number of packets sent within last period is current rate for single l2 addr
                l2RvMap[l2RvElement.first].msgRate =
                    1000.0 * ((double)(rvsp->totalCnt - rvsp->lastTotalCnt) /
                    (double)(currTime_ms - rvsp->lastTime)) ;
                if(configuration.floodDetectVerbosity > 7){
                    std::cout << "The total count for this rv is: " << rvsp->totalCnt << "\n";
                    std::cout << "The last total count for this rv is: " << rvsp->lastTotalCnt << "\n";
                    std::cout << "The current msg rate for this rv is: " <<
                        l2RvMap[l2RvElement.first].msgRate << "\n";
                    std::cout << "Last time is: " << rvsp->lastTime << " and curr time is: " <<
                            currTime_ms<< "\n";
                    std::cout << "Time difference is: "  << (currTime_ms - rvsp->lastTime) << "ms \n";
                }
            }

            if( (l2RvMap[l2RvElement.first].msgRate > configuration.floodAttackThreshSingle)
                    && (currUtil >= (configuration.mvmUtilThreshold))){
                // calculate expected filtering time
                if( currUtil < 1.0 ){ // < 1 and >= threshold
                    filteringTime = (uint32_t)((100.0 * currUtil) - 25.0); // ms
                }else{ // >= 1
                    filteringTime = 75; // ms
                }

                // add this l2 address to the list of addr to filter
                // somehow need to identify portion from a single vehicle is a high value.
                // via the l2 address
                rvSrc.srcL2Id = l2RvElement.first;
                rvSrc.pppp = 0;
                rvSrc.durationMs = filteringTime;
                if(configuration.floodDetectVerbosity > 1){
                    std::cout << "Detected flooding attack\n";
                    std::cout << "Current utilization is: " << currUtil << "\n";
                    std::cout << "Adding " << rvSrc.srcL2Id << " to rv list to filter\n";
                    std::cout << "Filtering time should be " << rvSrc.durationMs << "\n";
                    std::cout << " (100.0 * currUtil) " << (100.0 * currUtil) << "\n";
                    std::cout << " Calculated filter time is " << filteringTime << "\n";
                    std::cout << "new utilization should be around: " <<
                        (((100.0 - filteringTime)/100.0)) << "\n";
                }
                rvListToFilter->push_back(rvSrc);
            }else{
                if(configuration.floodDetectVerbosity > 3){
                    std::cout << "There is no flooding attack from l2 " <<
                        l2RvElement.first << " happening\n";
                }
                stateOn = false;
            }
            l2RvMap[l2RvElement.first].lastTime = currTime_ms;
            l2RvMap[l2RvElement.first].lastTotalCnt = rvsp->totalCnt;
        }
    }

    // evaluation instance logic
    if(rvListToFilter->size()){
        // move to state 1 - flooding attack happening
        stateOn = true;
    }else{
        stateOn = false;
    }
 }


void ApplicationBase::vehicleEventReport(bool emergent,
    const current_dynamic_vehicle_state_t* const vehicle_state) {
    bool notify = false;

    // need to check for the critical event before moving to emergent state.
    // boolean should be just a success flag rather than emergent
    if (emergent) {
        // check for emergency critical events here:
        // hard braking, ABS, traction control,
        // and stability control event
        notify = true;
        {
            std::unique_lock<std::mutex> loc(stateMtx);
            criticalState = true;
            newEvent = true;
        }
        //TODO: use vehicle_state to construct critical BSM messages
        if(this->currVehState == NULL){
            this->currVehState = (current_dynamic_vehicle_state_t*)
                calloc(sizeof(current_dynamic_vehicle_state_t), 1);
                // probably need to free it later
        }
        // we'd need to fill local can data for non critical events too
        // so that we can fill the bsm
        if(this->currVehState != NULL && vehicle_state != NULL){
            memcpy(this->currVehState, vehicle_state, sizeof(current_dynamic_vehicle_state_t));
        }
        // also get static vehicle state; need to provide pointer to the callback function
    } else {
        if (criticalState) {
            notify = true;
            std::unique_lock<std::mutex> loc(stateMtx);
            criticalState = false;
            newEvent = false;
            currVehState->events.data = 0;
            // if congestion control enabled, notify the congestion control library
            if(this->configuration.enableCongCtrl && congestionControlManager != NULL){
               congestionControlManager->disableCriticalEvent();
            }
        }
    }

    if (notify) {
        // Wonder if should lockIDChange first, then do notify ???
        // if do lockIdChange first, might introduce some latency of sending event BSM
        // if notify first, low probability that ID would change when sending the event
        stateCv.notify_all();
        if (criticalState) {
#if AEROLINK
           if (this->configuration.enableSecurity == true) {
               if (SecService->lockIdChange()) {
                   printf("Fail to lock ID change\n");
               }
           }

        } else {
           // unblock id change
           if (this->configuration.enableSecurity == true) {
               if (SecService->unlockIdChange()) {
                   printf("Fail to lock ID change\n");
               }
           }
#endif
        }
    }
}
void ApplicationBase::prepareForExit() {
    exitApp = true; 
    sem_post(&this->rx_sem);
    sem_post(&this->log_sem);
    sem_post(&idChangeData.idSem);

    if(configuration.enableCongCtrl &&
                congestionControlManager && congCtrlInitialized){
        if(congestionControlManager->
                    getCongestionControlUserData()->congestionControlSem){
            sem_post(congestionControlManager->
                        getCongestionControlUserData()->congestionControlSem);
        }
        congestionControlManager->stopCongestionControl();
        congCtrlInitialized = false;
    }

    {
        std::unique_lock<std::mutex> loc(stateMtx);
        stateCv.notify_all();
    }
    {
        lock_guard<std::mutex> lock(csvMutex);
        writeLogFinish = true;
        writeMutexCv.notify_all();
    }
    // notify all radio interface to prepare for exit
    for (uint8_t i = 0; i<this->eventTransmits.size(); i++) {
        this->eventTransmits[i].prepareForExit();
    }
    for (uint8_t i = 0; i < this->spsTransmits.size(); i++) {
        this->spsTransmits[i].prepareForExit();
    }
    for (uint8_t i = 0; i < this->radioReceives.size(); i++) {
        this->radioReceives[i].prepareForExit();
    }
    if (kinematicsReceive != nullptr) {
        kinematicsReceive->close();
    }
}

bool ApplicationBase::pendingTillEmergency() {
    bool ret = true;
    std::unique_lock<std::mutex> loc(stateMtx);
    if (!criticalState) {
        stateCv.wait(loc,
                     [this, &ret] {
                         if (true == newEvent) {
                             ret = true;
                             // reset the event flag
                             newEvent = false;
                             return true;
                         }
                         if (exitApp) {
                             ret = false;
                             return true;
                         }
                         return false;
                     });
        // if congestion control enabled, notify the congestion control library
        if(this->configuration.enableCongCtrl && congestionControlManager != NULL){
           congestionControlManager->notifyCriticalEvent();
        }
    }
    return ret;
}

bool ApplicationBase::pendingTillNoEmergency() {
    bool ret = true;
    if (criticalState) {
        std::unique_lock<std::mutex> loc(stateMtx);
        stateCv.wait(loc,
                     [this, &ret] {
                         if (false == criticalState) {
                             ret = true;
                             return true;
                         }

                         if (exitApp) {
                             ret = false;
                             return true;
                         }

                         return false;
                     });
    }

    return true;
}

//Calculates received packets per second for Throttle Manager
void ApplicationBase::tmCommunication() {
    int load = 0;
    sem_wait(&this->log_sem);
    if (appVerbosity > 3) {
        printf("Arrival rate is: %d\n", this->totalRxSuccessPerSecond);
    }
    load = this->totalRxSuccessPerSecond;
    this->totalRxSuccessPerSecond = 0;
    sem_post(&this->log_sem);

    if (load != 0) {
        this->prevFilterRate = this->filterRate;
        if (abs((load)-(this->prevArrivalRate)) >= this->configuration.deltaInRxRate) {
            //set load to throttle manager
            cv2xTmListener->setLoad(load);
            this->prevArrivalRate=load;
        }
    }
}

uint16_t ApplicationBase::delimiterPos(string line, vector<string> delimiters){
    uint16_t pos = 65535; //Largest possible value of 16 bits.
    for (int i = 0; i < delimiters.size(); i++)
    {
        uint16_t delimiterPos = line.find(delimiters[i]);
        if (pos > delimiterPos) {
            pos = delimiterPos;
        }
    }
    return pos;
}

int ApplicationBase::loadConfiguration(char* file) {
    map <string, string> configs;
    string line;
    vector<string> delimiters = { " ", "\t", "#", "="};
    ifstream configFile(file);
    if (configFile.is_open())
    {
        while (getline(configFile, line))
        {
            if (line[0] != '#' && !line.empty())
            {
                uint8_t pos = 0;
                uint8_t end = ApplicationBase::delimiterPos(line, delimiters);
                string key = line.substr(pos, end);
                line.erase(0, end);
                while (line[0] == ' ' || line[0] == '=' || line[0] == '\t') {
                    line.erase(0, 1);
                }
                end = ApplicationBase::delimiterPos(line, delimiters);
                string value = line.substr(pos, end);
                configs.insert(pair<string, string>(key, value));
            }
        }
        this->saveConfiguration(configs);
        int nice = getpriority(PRIO_PROCESS, 0);
        if(configuration.appVerbosity){
            fprintf(stdout, "Current process priority value is %d\n", nice);
        }
        return 0;
    }

    cout<<"Error opening config file.\n";
    return -1;
}


void ApplicationBase::saveConfiguration(map<string, string> configs) {

    // by default the ITS process priority should be set to highest (-20)
    // however, for testing purposes, qits priority can be altered
    if (configs.end() != configs.find("procPriority")) {
        this->configuration.procPriority =
            stoi(configs["procPriority"], nullptr, 10);
    }
    if(this->configuration.procPriority < MIN_NICE ||
        this->configuration.procPriority > MAX_NICE){
        this->configuration.procPriority = DEFAULT_PROCESS_PRIORITY;
    }
    if(setpriority(PRIO_PROCESS, 0,
            this->configuration.procPriority) < 0) {
        fprintf(stderr, "Setting priority to %d failed\n",
            this->configuration.procPriority);
        int nice = getpriority(PRIO_PROCESS, 0);
        fprintf(stderr, "Current priority of process will be: %d\n", nice);
    }

    if (configs.end() != configs.find("EnablePreRecorded")) {
        istringstream is(configs["EnablePreRecorded"]);
        is >> boolalpha >> this->configuration.enablePreRecorded;
    }

    if (configs.end() != configs.find("PreRecordedFile")) {
        this->configuration.preRecordedFile = configs["PreRecordedFile"];
    }

    if (configs.end() != configs.find("preRecordedBsmLog")) {
        istringstream is(configs["preRecordedBsmLog"]);
        is >> boolalpha >> this->configuration.preRecordedBsmLog;
    }

    if (configs.end() != configs.find("TransmitRateInterval")) {
        this->configuration.transmitRate = stoi(configs["TransmitRateInterval"], nullptr, 10);
    }

    if (configs.end() != configs.find("SpsPeriodicity")) {
        this->configuration.spsPeriodicity = stoi(configs["SpsPeriodicity"], nullptr, 10);
    }

    stringstream stream;
    if (configs.end() != configs.find("SpsFlows")) {
        auto num = stoi(configs["SpsFlows"], nullptr, 10);
        if (configs.end() != configs.find("SpsPorts")) {
            stream.str(configs["SpsPorts"]);
            for (uint32_t i = 0; i < num; i++) {
                string port;
                getline(stream, port, ',');
                if (port.empty()) {
                    break;
                }
                this->configuration.spsPorts.push_back(stoi(port, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("SpsDestAddrs")) {
            stream.str(configs["SpsDestAddrs"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string spsDestAddrs;
                getline(stream, spsDestAddrs, ',');
                if (spsDestAddrs.empty()) {
                    break;
                }
                this->configuration.spsDestAddrs.push_back(spsDestAddrs);
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("SpsDestPorts")) {
            stream.str(configs["SpsDestPorts"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string port;
                getline(stream, port, ',');
                if (port.empty()) {
                    break;
                }
                this->configuration.spsDestPorts.push_back(stoi(port, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("SpsServiceIDs")) {
            stream.str(configs["SpsServiceIDs"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string spsServiceIDs;
                getline(stream, spsServiceIDs, ',');
                if (spsServiceIDs.empty()) {
                    break;
                }
                this->configuration.spsServiceIDs.push_back(stoi(spsServiceIDs, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }
    }

    if (configs.end() != configs.find("EventFlows")) {
        auto num = stoi(configs["EventFlows"], nullptr, 10);
        if (configs.end() != configs.find("EventPorts")) {
            stream.str(configs["EventPorts"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string port;
                getline(stream, port, ',');
                if (port.empty()) {
                    break;
                }
                this->configuration.eventPorts.push_back(stoi(port, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("EventDestAddrs")) {
            stream.str(configs["EventDestAddrs"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string EventDestAddrs;
                getline(stream, EventDestAddrs, ',');
                if (EventDestAddrs.empty()) {
                    break;
                }
                this->configuration.eventDestAddrs.push_back(EventDestAddrs);
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("EventDestPorts")) {
            stream.str(configs["EventDestPorts"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string port;
                getline(stream, port, ',');
                if (port.empty()) {
                    break;
                }
                this->configuration.eventDestPorts.push_back(stoi(port, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("EventServiceIDs")) {
            stream.str(configs["EventServiceIDs"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string eventServiceIDs;
                getline(stream, eventServiceIDs, ',');
                if (eventServiceIDs.empty()) {
                    break;
                }
                this->configuration.eventServiceIDs.push_back(
                    stoi(eventServiceIDs, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }
    }

    if (configs.end() != configs.find("ReceiveFlows")
        and configs.end() != configs.find("ReceivePorts")) {
        stream.str(configs["ReceivePorts"]);
        for (uint32_t i = 0; i < stoi(configs["ReceiveFlows"], nullptr, 10); i++)
        {
            string port;
            getline(stream, port, ',');
            if (port.empty()) {
                break;
            }
            this->configuration.receivePorts.push_back(stoi(port, nullptr, 10));
        }
        stream.str("");
        stream.clear();
    }


    if (configs.end() != configs.find("ReceiveSubIds")){
        stream.str(configs["ReceiveSubIds"]);
        for (uint32_t i = 0; i < stoi(configs["ReceiveSubIds"], nullptr, 10); i++)
        {
            string rxSubId;
            getline(stream, rxSubId, ',');
            if (rxSubId.empty()) {
                break;
            }
            this->configuration.receiveSubIds.push_back(stoi(rxSubId, nullptr, 10));
        }
        stream.str("");
        stream.clear();
    }

    // if empty, add rx sub id
    if(this->configuration.receiveSubIds.empty()){
        this->configuration.receiveSubIds.push_back(DEFAULT_BSM_PSID);
    }

    if (configs.end() != configs.find("LocationInterval")) {
        this->configuration.locationInterval = stoi(configs["LocationInterval"], nullptr, 10);
    }

    if (configs.end() != configs.find("enableLocationFixes")) {
        if (configs["enableLocationFixes"].find("true") != std::string::npos)
            this->configuration.enableLocationFixes = true;
        else
            this->configuration.enableLocationFixes = false;
    }

    if (configs.end() != configs.find("leapSeconds")) {
        this->configuration.leapSeconds = (uint8_t)stoi(configs["leapSeconds"], nullptr, 10);
    }

    if (configs.end() != configs.find("WraServiceID")) {
        this->configuration.wraServiceId = stoi(configs["WraServiceID"], nullptr, 10);
    }

    if (configs.end() != configs.find("BsmJitter")) {
        this->configuration.bsmJitter = stoi(configs["BsmJitter"], nullptr, 10);
    }

    if (configs.end() != configs.find("EnableVehicleExt")) {
        istringstream is2(configs["EnableVehicleExt"]);
        is2 >> boolalpha >> this->configuration.enableVehicleExt;
    }

    if (configs.end() != configs.find("PathHistoryPoints")) {
        this->configuration.pathHistoryPoints = stoi(configs["PathHistoryPoints"], nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleWidth")) {
        this->configuration.vehicleWidth = stoi(configs["VehicleWidth"], nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleLength")) {
        this->configuration.vehicleLength = stoi(configs["VehicleLength"], nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleHeight")) {
        this->configuration.vehicleHeight = stoi(configs["VehicleHeight"], nullptr, 10);
    }

    if (configs.end() != configs.find("FrontBumperHeight")) {
        this->configuration.frontBumperHeight = stoi(configs["FrontBumperHeight"], nullptr, 10);
    }

    if (configs.end() != configs.find("RearBumperHeight")) {
        this->configuration.rearBumperHeight = stoi(configs["RearBumperHeight"], nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleMass")) {
        this->configuration.vehicleMass = stoi(configs["VehicleMass"], nullptr, 10);
    }

    if (configs.end() != configs.find("BasicVehicleClass")) {
        this->configuration.vehicleClass = stoi(configs["BasicVehicleClass"], nullptr, 10);
    }

    if (configs.end() != configs.find("SirenInUse")) {
        this->configuration.sirenUse = stoi(configs["SirenInUse"], nullptr, 10);
    }

    if (configs.end() != configs.find("LightBarInUse")) {
        this->configuration.lightBarUse = stoi(configs["LightBarInUse"], nullptr, 10);
    }

    if (configs.end() != configs.find("SpecialVehicleTypeEvent")) {
        this->configuration.specialVehicleTypeEvent = stoi(configs["SpecialVehicleTypeEvent"],
                nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleType")) {
        this->configuration.vehicleType = stoi(configs["VehicleType"], nullptr, 10);
    }

    if (configs.end() != configs.find("LdmSize")) {
        this->configuration.ldmSize = stoi(configs["LdmSize"], nullptr, 10);
    }

    if (configs.end() != configs.find("LdmGbTime")) {
        this->configuration.ldmGbTime = stoi(configs["LdmGbTime"], nullptr, 10);
    }

    if (configs.end() != configs.find("LdmGbTimeThreshold")) {
        this->configuration.ldmGbTimeThreshold =
            stoi(configs["LdmGbTimeThreshold"], nullptr, 10);
    }

    if (configs.end() != configs.find("TTunc")) {
        this->configuration.tunc = stoi(configs["TTunc"], nullptr, 10);
    }

    if (configs.end() != configs.find("TAge")) {
        this->configuration.age = stoi(configs["TAge"], nullptr, 10);
    }

    if (configs.end() != configs.find("TPacketError")) {
        this->configuration.packetError = stoi(configs["TPacketError"], nullptr, 10);
    }

    if (configs.end() != configs.find("TUncertainty3D")) {
        this->configuration.uncertainty3D = stoi(configs["TUncertainty3D"], nullptr, 10);
    }

    if (configs.end() != configs.find("TDistance")) {
        this->configuration.distance3D = stoi(configs["TDistance"], nullptr, 10);
    }

    if (configs.end() != configs.find("enableVehicleDataCallbacks")) {
        istringstream is(configs["enableVehicleDataCallbacks"]);
        is >> boolalpha >> this->configuration.enableVehicleDataCallbacks;
    }

    if (configs.end() != configs.find("SourceIpv4Address")) {
        this->configuration.ipv4_src = configs["SourceIpv4Address"];
    }

    if (configs.end() != configs.find("enableTxAlways")) {
        // for tx and rx at same time
        istringstream is4(configs["enableTxAlways"]);
        is4 >> boolalpha >> this->configuration.enableTxAlways;
    }

    if (configs.end() != configs.find("DestIpv4Address")) {
        // only used when enableTxAlways and for Ethernet
        this->configuration.ipv4_dest = configs["DestIpv4Address"];
    }

    if (configs.end() != configs.find("TxPort")) {
        this->configuration.tx_port = stoi(configs["TxPort"], nullptr, 10);
    }

    /* ETSI config items */
    if (configs.find("MacAddr") != configs.end()) {
        int i = 0;
        auto pos = 0, prev = 0;
        do {
            pos = configs["MacAddr"].find(":", prev);
            if (pos != std::string::npos) {
                this->configuration.MacAddr[i] =
                    stoi(configs["MacAddr"].substr(prev, pos), 0, 16);
            }
            prev = pos + 1;
            i++;
        } while(pos != std::string::npos);
        pos = configs["MacAddr"].rfind(" ");
        this->configuration.MacAddr[5] =
            stoi(configs["MacAddr"].substr(pos + 1, std::string::npos), 0, 16);
    }
    if(configs.find("StationType") != configs.end()) {
        this->configuration.StationType = stoi(configs["StationType"]);
    }
    if (configs.find("CAMDestinationPort") != configs.end()) {
        this->configuration.CAMDestinationPort = (uint16_t)stoi(configs["CAMDestinationPort"]);
    }
    // security-only psid value. this can differ from the sps service id value for testing.
    if (configs.find("psidValue") != configs.end()) {
        configuration.psid = stoi(configs["psidValue"],0,16);
    }
    if (configs.find("fakeRVTempIds") != configs.end()) {
        if (configs["fakeRVTempIds"].find("true") != std::string::npos){
            this->configuration.fakeRVTempIds = true;
            if (configs.find("totalFakeRVTempIds") != configs.end()) {
                this->configuration.totalFakeRVTempIds = stoi(configs["totalFakeRVTempIds"]);
            }
        }else{
            this->configuration.fakeRVTempIds = false;
        }
    }

    if (configs.find("RVTransmitLossSimulation") != configs.end()) {
        configuration.RVTransmitLossSimulation =
            stoi(configs["RVTransmitLossSimulation"]);
    }

    /* Security service */
    if (configs.find("EnableSecurity") != configs.end()) {
        if (configs["EnableSecurity"].find("true") != std::string::npos)
            this->configuration.enableSecurity = true;
        else
            this->configuration.enableSecurity = false;
    }

    if (configuration.enableSecurity == true) {
        ApplicationBase::securityEnabled = true;
        if (configs.find("SecurityContextName") != configs.end()) {
            configuration.securityContextName = configs["SecurityContextName"];
        }
        if (configs.find("SecurityCountryCode") != configs.end()) {
            configuration.securityCountryCode = stoi(configs["SecurityCountryCode"], 0, 16);
        }
        if(configs.find("enableSsp") != configs.end()){
            if (configs["enableSsp"].find("true") != std::string::npos){
                this->configuration.enableSsp = true;
                if (configs.find("sspValue") != configs.end()) {
                    char* end;
                    uint8_t num = (uint8_t)std::count(configs["sspValue"].begin(),
                                    configs["sspValue"].end(), ':');
                    if(configs["sspValue"].back() != ':'){
                        num++;
                    }
                    this->configuration.sspLength = num;
                    stream.str(configs["sspValue"]);
                    for (uint32_t i = 0; i < num; i++)
                    {
                        string s;
                        getline(stream, s, ':');
                        if (s.empty()) {
                            break;
                        }
                        this->configuration.sspValueVect.push_back(s);
                        this->configuration.ssp[i] =
                                (uint8_t)strtol(
                                    this->configuration.sspValueVect.at(i).c_str(), &end,16);
                    }
                    stream.str("");
                    stream.clear();
                }
            }else{
                this->configuration.enableSsp = false;
                this->configuration.sspLength = 0;
            }
        }

        if(configs.find("enableSspMask") != configs.end()){
            if (configs["enableSspMask"].find("true") != std::string::npos){
                this->configuration.enableSspMask = true;
                if(configs.find("sspMask") != configs.end() &&
                        this->configuration.enableSsp == true &&
                        this->configuration.enableSspMask == true){
                    char* end;
                    uint8_t num = (uint8_t)std::count(configs["sspMask"].begin(),
                                    configs["sspMask"].end(), ':');
                    if(configs["sspMask"].back() != ':'){
                        num++;
                    }
                    this->configuration.sspMaskLength = num;
                    stream.str(configs["sspMask"]);
                    for (uint32_t i = 0; i < num; i++)
                    {
                        string s;
                        getline(stream, s, ':');
                        if (s.empty()) {
                            break;
                        }
                        this->configuration.sspMaskVect.push_back(s);
                        this->configuration.sspMask[i] =
                                (uint8_t)strtol(
                                    this->configuration.sspMaskVect.at(i).c_str(), &end,16);
                    }
                    stream.str("");
                    stream.clear();
                }
            }else{
                this->configuration.enableSspMask = false;
                this->configuration.sspMaskLength = 0;
            }
        }


        /*For ssp check */
        if (configs.find("expectedSspValue") != configs.end()) {
            char* end;
            uint8_t num = (uint8_t)std::count(configs["expectedSspValue"].begin(),
                    configs["expectedSspValue"].end(), ':');
            if(configs["expectedSspValue"].back() != ':'){
                num++;
            }
            this->configuration.expectedSspLength = num;
            stream.str(configs["expectedSspValue"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string s;
                getline(stream, s, ':');
                if (s.empty()) {
                    break;
                }
                this->configuration.expectedSspValueVect.push_back(s);
                this->configuration.expectedSsp[i] =
                        (uint8_t)strtol(
                                this->configuration.expectedSspValueVect.at(i).c_str(), &end,16);
            }
            stream.str("");
            stream.clear();
        }
        else{
            this->configuration.expectedSspLength = 0;
        }

        if(configs.find("setGenLocation") != configs.end()) {
            istringstream is4(configs["setGenLocation"]);
            is4 >> boolalpha >> configuration.setGenLocation;
        }

        if(configs.find("enableAsync") != configs.end()) {
            istringstream is4(configs["enableAsync"]);
            is4 >> boolalpha >> configuration.enableAsync;
        }

        if(configs.find("enableConsistency") != configs.end()) {
            istringstream is4(configs["enableConsistency"]);
            is4 >> boolalpha >> configuration.enableConsistency;
        }

        if(configs.find("enableRelevance") != configs.end()) {
            istringstream is4(configs["enableRelevance"]);
            is4 >> boolalpha >> configuration.enableRelevance;
        }

        if(configs.find("overridePsidCheck") != configs.end()) {
            istringstream is4(configs["overridePsidCheck"]);
            is4 >> boolalpha >> configuration.overridePsidCheck;
        }

        if(configs.find("emergencyVehicleEventTX") != configs.end()) {
            istringstream is4(configs["emergencyVehicleEventTX"]);
            is4 >> boolalpha >> configuration.emergencyVehicleEventTX;
        }

        /* Signing-related statistics */
        if(configs.find("enableSignStatLog") != configs.end()){
           istringstream is7(configs["enableSignStatLog"]);
           is7 >> boolalpha >> configuration.enableSignStatLog;
        }

        if(configs.find("signStatLogListSize") != configs.end()){
            this->configuration.signStatsSize =
            (uint32_t)stoi(configs["signStatLogListSize"]);
        }

        if(configs.find("signStatLogFile") != configs.end()){
            this->configuration.signStatLogFile = configs["signStatLogFile"];
        }

        if((configuration.enableSignStatLog) && (configuration.appVerbosity > 1)){
            std::cout << "Signing statistic logging is ON" << std::endl;
            std::cout << "Statistics for last " << configuration.signStatsSize <<
                " signs will be reported by each thread" << std::endl;
            std::cout << "Upon closure, statistics will be dumped to logfile: " <<
                configuration.signStatLogFile << std::endl;
        }

        /* Verification Latency-related statistics */
        if(configs.find("enableVerifStatLog") != configs.end()){
           istringstream is8(configs["enableVerifStatLog"]);
           is8 >> boolalpha >> configuration.enableVerifStatLog;
        }

        if(configs.find("verifStatLogListSize") != configs.end()){
            this->configuration.verifStatsSize =
                (uint32_t)stoi(configs["verifStatLogListSize"]);
        }

        if(configs.find("verifStatLogFile") != configs.end()){
            this->configuration.verifStatLogFile = configs["verifStatLogFile"];
        }

        if((configuration.enableVerifStatLog) && (configuration.appVerbosity > 1)){
            std::cout << "Verification statistic logging is ON" << std::endl;
            std::cout << "Statistics for last " << configuration.verifStatsSize <<
                " verifications will be reported by each thread" << std::endl;
            std::cout << "Upon closure, statistics will be dumped to logfile: " <<
                configuration.verifStatLogFile << std::endl;
        }

        /* Verification Results-related statistics */
        if(configs.find("enableVerifResLog") != configs.end()){
           istringstream is8(configs["enableVerifResLog"]);
           is8 >> boolalpha >> configuration.enableVerifResLog;
        }

        if(configs.find("verifResLogSize") != configs.end()){
            this->configuration.verifResLogSize =
                (uint32_t)stoi(configs["verifResLogSize"]);
        }

        if(configs.find("verifResLogFile") != configs.end()){
            this->configuration.verifResLogFile = configs["verifResLogFile"];
        }

        if((configuration.enableVerifResLog) && (configuration.appVerbosity > 1)){
            std::cout << "Verification Results logging is ON" << std::endl;
            std::cout << "Verification Results will not be logged on Console" << std::endl;
            std::cout << "Results for last " << configuration.verifResLogSize <<
                " verifications will be reported by each thread" << std::endl;
            std::cout << "Upon closure, statistics will be dumped to logfile: " <<
                configuration.verifResLogFile << std::endl;
        }

        /** Pseudonym/ID Change */
        if(configs.find("lcmName") != configs.end()){
            this->configuration.lcmName = configs["lcmName"];
        }else{
            this->configuration.lcmName = "";
        }

        if(configs.find("idChangeInterval") != configs.end()) {
            this->configuration.idChangeInterval =
                (unsigned int)stoi(configs["idChangeInterval"]);
        }

        if(configs.find("idChangeDistance") != configs.end()) {
            ApplicationBase::idChangeDistance =
                (unsigned int)stoi(configs["idChangeDistance"]);
        }

        /** Process both signed and unsigned packets */
        if(configs.find("acceptAll") != configs.end()){
            istringstream is9(configs["acceptAll"]);
            is9 >> boolalpha >> configuration.acceptAll;
            if(configuration.acceptAll){
                printf("Accepting both signed and unsigned messages\n");
            }else{
                printf("Only accepting signed messages\n");
            }
        }
        /* Misbehavior-related statistics */
        if(configs.find("enableMbd") != configs.end()){
            istringstream is1(configs["enableMbd"]);
            is1 >> boolalpha >> configuration.enableMbd;
            std::cout << "Misbehavior checks enabled\n";
            if(configuration.enableMbd) {
                if(configs.find("enableMbdStatLog") != configs.end()){
                    istringstream is8(configs["enableMbdStatLog"]);
                    is8 >> boolalpha >> configuration.enableMbdStatLog;
                    if(configuration.enableMbdStatLog){
                        if(configs.find("mbdStatLogListSize") != configs.end()){
                            this->configuration.mbdStatLogListSize =
                                (uint32_t)stoi(configs["mbdStatLogListSize"]);
                        }
                        if(configs.find("mbdStatLogFile") != configs.end()){
                            this->configuration.mbdStatLogFile = configs["mbdStatLogFile"];
                        }
                        if(configuration.appVerbosity > 1){
                            std::cout << "Misbehavior statistic logging is ON" << std::endl;
                            std::cout << "Statistics for last " <<
                                configuration.mbdStatLogListSize <<
                                " misbehavior will be reported by each thread" << std::endl;
                            std::cout << "Upon closure, statistics will be dumped to logfile: " <<
                                configuration.mbdStatLogFile << std::endl;
                        }
                    } else{
                        std::cout << "Misbehavior statistic logging is off" << std::endl;
                    }
                }
            }
        }

        if (configs.find("overrideVerifResult") != configs.end())
        {
            istringstream is(configs["overrideVerifResult"]);
            is >> boolalpha >> this->configuration.overrideVerifResult;
        }
        if(this->configuration.overrideVerifResult){
            this->configuration.overrideVerifValue = stoi(configs["overrideVerifValue"]);
        }

        /* Flooding attack detection and mitigation config items */
        if (configs.find("enableL2FloodingDetect") != configs.end())
        {
            istringstream is(configs["enableL2FloodingDetect"]);
            is >> boolalpha >> this->configuration.enableL2FloodingDetect;
        }
        if(configs.find("floodDetectVerbosity") != configs.end()){
            this->configuration.floodDetectVerbosity = stoi(configs["floodDetectVerbosity"]);
        }
        if(configs.find("commandInterval") != configs.end()) {
            this->configuration.commandInterval = stoi(configs["commandInterval"]);
        }
        if(configs.find("tShiftInterval") != configs.end()) {
            this->configuration.tShiftInterval = stoi(configs["tShiftInterval"]);
        }
        if(configs.find("nCommandInterval_0") != configs.end()) {
            this->configuration.nCommandInterval_0 = stoi(configs["nCommandInterval_0"]);
        }
        if(configs.find("nCommandInterval_1") != configs.end()) {
            this->configuration.nCommandInterval_1 = stoi(configs["nCommandInterval_1"]);
        }
        if(configs.find("floodAttackThreshTotal") != configs.end()) {
            this->configuration.floodAttackThreshTotal = stoi(configs["floodAttackThreshTotal"]);
        }
        if(configs.find("floodAttackThreshSingle") != configs.end()) {
            this->configuration.floodAttackThreshSingle = stoi(configs["floodAttackThreshSingle"]);
        }
        if(configs.find("loadUpdateInterval") != configs.end()) {
            this->configuration.loadUpdateInterval = stoi(configs["loadUpdateInterval"]);
        }
        if(configs.find("mvmUtilThreshold") != configs.end()) {
            this->configuration.mvmUtilThreshold = stod(configs["mvmUtilThreshold"]);
        }

        if (configs.find("mvmCapacityOverride") != configs.end())
        {
            istringstream is(configs["mvmCapacityOverride"]);
            is >> boolalpha >> this->configuration.mvmCapacityOverride;
        }
        if(configs.find("mvmCapacity") != configs.end()) {
            this->configuration.mvmCapacity = stoi(configs["mvmCapacity"]);
        }
    }

    /* codec debug */
    if (configs.find("codecVerbosity") != configs.end()) {
        this->configuration.codecVerbosity =
            (uint8_t)stoi(configs["codecVerbosity"]);
        set_codec_verbosity(stoi(configs["codecVerbosity"]));
    }

    /*app debug */
    if (configs.find("appVerbosity") != configs.end()) {
        setAppVerbosity(stoi(configs["appVerbosity"]));
        //qMon
    }

    /* ldm debug */
    if (configs.find("ldmVerbosity") != configs.end()) {
        this->configuration.ldmVerbosity =
            (uint8_t)stoi(configs["ldmVerbosity"]);
    }

    /* driver debug */
    if (configs.find("driverVerbosity") != configs.end()) {
        this->configuration.driverVerbosity =
            (uint8_t)stoi(configs["driverVerbosity"]);
    }
    /* security debug */
    if (configs.find("secVerbosity") != configs.end()) {
        this->configuration.secVerbosity =
            (uint8_t)stoi(configs["secVerbosity"]);
    }

    /* Multi-parallelism */
    if(configs.find("numRxThreadsEth") != configs.end()) {
        this->configuration.numRxThreadsEth = (uint8_t)stoi(configs["numRxThreadsEth"]);
    }
    if(configs.find("numRxThreadsRadio") != configs.end()) {
        this->configuration.numRxThreadsRadio = (uint8_t)stoi(configs["numRxThreadsRadio"]);
    }

    /** Filtering */
    if (configs.find("filterInterval") != configs.end()) {
        this->configuration.filterInterval = (unsigned int)stoi(configs["filterInterval"]);
    }

    /** Increase/decrease in the rx rate required to communicate to TM */
    if (configs.find("deltaInRxRate") != configs.end()) {
        this->configuration.deltaInRxRate = (unsigned int)stoi(configs["deltaInRxRate"]);
    }

    /** Enable L2 src filtering */
    if (configs.find("enableL2SrcFiltering") != configs.end()) {
        istringstream ipstream1(configs["enableL2SrcFiltering"]);
        ipstream1 >> boolalpha >> configuration.enableL2Filtering;
    }

    if (configs.find("l2SrcFilteringTime") != configs.end()) {
        istringstream ipstream2(configs["l2SrcFilteringTime"]);
        ipstream2 >> boolalpha >> configuration.l2FilteringTime;
    }

    if (configs.find("l2SrcIdTimeThresholdSec") != configs.end()) {
        istringstream ipstream3(configs["l2SrcIdTimeThresholdSec"]);
        ipstream3 >> boolalpha >> configuration.l2IdTimeThreshold;
    }

    /* WSA */
    configuration.routerLifetime = 0;
    configuration.ipPrefixLength = 0;
    if(configs.find("routerLifetime") != configs.end()){
        configuration.routerLifetime = stoi(configs["routerLifetime"]);
    }
    if (configs.find("ipPrefix") != configs.end()) {
        configuration.ipPrefix = configs["ipPrefix"];
    }
    if (configs.find("ipPrefixLength") != configs.end()) {
        configuration.ipPrefixLength = stoi(configs["ipPrefixLength"]);
    }
    if (configs.find("defaultGateway") != configs.end()) {
        configuration.defaultGateway = configs["defaultGateway"];
    }
    if (configs.find("primaryDns") != configs.end()) {
        configuration.primaryDns = configs["primaryDns"];
    }
    if (configs.find("wsaInfoFile") != configs.end()) {
        configuration.wsaInfoFile = configs["wsaInfoFile"];
    }
    if (configs.end() != configs.find("wsaInterval")) {
        this->configuration.wsaInterval = stoi(configs["wsaInterval"], nullptr, 10);
        // assume WSA Tx interval < 100ms is incorrect, re-set it to 100ms.
        if (this->configuration.wsaInterval < 100) {
            this->configuration.wsaInterval = 100;
        }
    }

    if(configs.find("wildcardRx") != configs.end()){
       istringstream is8(configs["wildcardRx"]);
       is8 >> boolalpha >> configuration.wildcardRx;
    }

    if (configs.end() != configs.find("Padding")) {
        this->configuration.padding = stoi(configs["Padding"], nullptr, 10);
        this->configuration.padding = (this->configuration.padding > MAX_PADDING_LEN) ?
                                       MAX_PADDING_LEN : this->configuration.padding;
    }

    if (configs.end() != configs.find("UnsignedBsmResSize")) {
        this->configuration.spsReservationSize =
                    (this->configuration.enableSecurity) ?
                                    stoi(configs["SignedBsmResSize"], nullptr, 10)
                                    : stoi(configs["UnsignedBsmResSize"], nullptr, 10);
        if(this->configuration.padding >= 0) {
            this->configuration.spsReservationSize += this->configuration.padding;
        }
    }

    if (configs.end() != configs.find("SpsPriority")) {
        this->configuration.spsPriority = static_cast<Priority>(
            stoi(configs["SpsPriority"], nullptr, 10));
    }

    if (configs.end() != configs.find("EventPriority")) {
        this->configuration.eventPriority = static_cast<Priority>(
            stoi(configs["EventPriority"], nullptr, 10));
    }

    this->configuration.isValid = true;

    // qMonitor Configuration
    if (configs.find("qMonEnabled") != configs.end())
    {
        istringstream is(configs["qMonEnabled"]);
        is >> boolalpha >> this->configuration.qMonEnabled;
        qMonConfig = std::make_shared<QMonitor::Configuration>();
    }
    // Add qMonConfig elements here after this line.
    // e.g. qMonConfig->sockDomain = AF_INET; // etc etc...

    // enable 2d distance calculation log when logging for each packet
    if (configs.end() != configs.find("enableDistanceLogs")) {
        istringstream is(configs["enableDistanceLogs"]);
        is >> boolalpha >> this->configuration.enableDistanceLogs;
    }

    // use the user-provided position in config file
    if (configs.end() != configs.find("positionOverride")) {
        istringstream is(configs["positionOverride"]);
        is >> boolalpha >> this->configuration.positionOverride;
        if(this->configuration.positionOverride == true){
            ApplicationBase::positionOverride = true;
            if(configs.find("overrideLat") != configs.end()){
                this->configuration.overrideLat = stod(configs["overrideLat"]);
                ApplicationBase::overrideLat = this->configuration.overrideLat;
            }
            if(configs.find("overrideLong") != configs.end()){
                this->configuration.overrideLong = stod(configs["overrideLong"]);
                ApplicationBase::overrideLong = this->configuration.overrideLong;
            }
            if(configs.find("overrideHead") != configs.end()){
                this->configuration.overrideHead = stod(configs["overrideHead"]);
                ApplicationBase::overrideHead = this->configuration.overrideHead;
            }
            if(configs.find("overrideElev") != configs.end()){
                this->configuration.overrideElev = stod(configs["overrideElev"]);
                ApplicationBase::overrideElev = this->configuration.overrideElev;
            }
            if(configs.find("overrideSpeed") != configs.end()){
                this->configuration.overrideSpeed = stod(configs["overrideSpeed"]);
                ApplicationBase::overrideSpeed = this->configuration.overrideSpeed;
            }
            std::cout << "OVERRIDING POSITION FIXES WITH CONFIG ITEMS: ";
            std::cout << "\nLAT: " << this->configuration.overrideLat << ", ";
            std::cout << "\nLON: " << this->configuration.overrideLong << ", ";
            std::cout << "\nELE: " << this->configuration.overrideHead << ", ";
            std::cout << "\nHEAD: " << this->configuration.overrideElev << ", ";
            std::cout << "\nSPD: " << this->configuration.overrideSpeed << "\n";
        }
    }

    /*
      check if congestion control is enabled and begin setting the cong ctrl config parameters
    */
    if (configs.end() != configs.find("enableCongCtrl")) {
        istringstream is(configs["enableCongCtrl"]);
        is >> boolalpha >> this->configuration.enableCongCtrl;
        if(this->configuration.enableCongCtrl == true){
            ApplicationBase::congCtrlEnabled = true;
            saveCongCtrlConfig(configs);
        }
    }
}

void ApplicationBase::saveCongCtrlConfig(map<string, string> configs){
    std::cout << "Proceeding to find and save cong ctrl config parameters\n";

    if(configs.find("congCtrlType") != configs.end()){
        congCtrlConfig.congCtrlType = stoi(configs["congCtrlType"]);
    }
    if(configs.find("enableCongCtrlLogging") != configs.end()){
        congCtrlConfig.enableCongCtrlLogging = stoi(configs["enableCongCtrlLogging"]);
    }
    if(configs.find("cbpMeasInterval") != configs.end()){
        congCtrlConfig.cbpMeasInterval = stoi(configs["cbpMeasInterval"]);
    }
    if(configs.find("cbpWeightFactor") != configs.end()){
        congCtrlConfig.cbpWeightFactor = stod(configs["cbpWeightFactor"]);
    }
    if(configs.find("perInterval") != configs.end()){
        congCtrlConfig.perInterval = stoi(configs["perInterval"]);
    }
    if(configs.find("perSubInterval") != configs.end()){
        congCtrlConfig.perSubInterval = stoi(configs["perSubInterval"]);
    }
    if(configs.find("perMax") != configs.end()){
        congCtrlConfig.perMax = stod(configs["perMax"]);
    }
    if(configs.find("minChanQualInd") != configs.end()){
        congCtrlConfig.minChanQualInd = stod(configs["minChanQualInd"]);
    }
    if(configs.find("maxChanQualInd") != configs.end()){
        congCtrlConfig.maxChanQualInd = stod(configs["maxChanQualInd"]);
    }
    if(configs.find("vDensityWeightFactor") != configs.end()){
        congCtrlConfig.vDensityWeightFactor = stod(configs["vDensityWeightFactor"]);
    }
    if(configs.find("vDensityCoefficient") != configs.end()){
        congCtrlConfig.vDensityCoefficient = stod(configs["vDensityCoefficient"]);
    }
    if(configs.find("vDensityMinPerRange") != configs.end()){
        congCtrlConfig.vDensityMinPerRange = stoi(configs["vDensityMinPerRange"]);
    }
    if(configs.find("UseStaticVDensity") != configs.end()){
        congCtrlConfig.UseStaticVDensity = stoi(configs["UseStaticVDensity"]);
    }
    if(configs.find("vDensity") != configs.end()){
        congCtrlConfig.vDensity = stoi(configs["vDensity"]);
    }
    if(configs.find("txCtrlInterval") != configs.end()){
        congCtrlConfig.txCtrlInterval = stoi(configs["txCtrlInterval"]);
    }
    if(configs.find("hvTEMinTimeDiff") != configs.end()){
        congCtrlConfig.hvTEMinTimeDiff = stoi(configs["hvTEMinTimeDiff"]);
    }
    if(configs.find("hvTEMaxTimeDiff") != configs.end()){
        congCtrlConfig.hvTEMaxTimeDiff = stoi(configs["hvTEMaxTimeDiff"]);
    }
    if(configs.find("rvTEMinTimeDiff") != configs.end()){
        congCtrlConfig.rvTEMinTimeDiff = stoi(configs["rvTEMinTimeDiff"]);
    }
    if(configs.find("rvTEMaxTimeDiff") != configs.end()){
        congCtrlConfig.rvTEMaxTimeDiff = stoi(configs["rvTEMaxTimeDiff"]);
    }
    if(configs.find("teErrSensitivity") != configs.end()){
        congCtrlConfig.teErrSensitivity = stoi(configs["teErrSensitivity"]);
    }
    if(configs.find("teMinThresh") != configs.end()){
        congCtrlConfig.teMinThresh = stod(configs["teMinThresh"]);
    }
    if(configs.find("teMaxThresh") != configs.end()){
        congCtrlConfig.teMaxThresh = stod(configs["teMaxThresh"]);
    }
    if(configs.find("minItt") != configs.end()){
        congCtrlConfig.minItt = stoi(configs["minItt"]);
    }
    if(configs.find("txRand") != configs.end()){
        congCtrlConfig.txRand = stoi(configs["txRand"]);
    }
    if(configs.find("timeAccuracy") != configs.end()){
        congCtrlConfig.timeAccuracy = stoi(configs["timeAccuracy"]);
    }
    if(configs.find("vMax_ITT") != configs.end()){
        congCtrlConfig.maxItt = stoi(configs["vMax_ITT"]);
    }
    if(configs.find("vRescheduleTh") != configs.end()){
        congCtrlConfig.reschedThresh = stoi(configs["vRescheduleTh"]);
    }
    if(configs.find("supraGain") != configs.end()){
        congCtrlConfig.supraGain = stod(configs["supraGain"]);
    }
    if(configs.find("minChanUtil") != configs.end()){
        congCtrlConfig.minChanUtil = stoi(configs["minChanUtil"]);
    }
    if(configs.find("maxChanUtil") != configs.end()){
        congCtrlConfig.maxChanUtil = stoi(configs["maxChanUtil"]);
    }
    if(configs.find("minRadiPwr") != configs.end()){
        congCtrlConfig.minRadiPwr = stoi(configs["minRadiPwr"]);
    }
    if(configs.find("maxRadiPwr") != configs.end()){
        congCtrlConfig.maxRadiPwr = stoi(configs["maxRadiPwr"]);
    }

    if (configs.find("enableSpsEnhancements") != configs.end()) {
        istringstream is(configs["enableSpsEnhancements"]);
        is >> boolalpha >> congCtrlConfig.enableSpsEnhancements;
        if (congCtrlConfig.enableSpsEnhancements) {
            std::cout << "SPS Enhancements Enabled\n";
        }
        if(configs.find("cv2xMaxITTRounding") != configs.end()){
            congCtrlConfig.cv2xMaxITTRounding = stoi(configs["cv2xMaxITTRounding"]);
        }

        if (configs.find("spsEnhIntervalRound") != configs.end()) {
            congCtrlConfig.spsEnhIntervalRound = stoi(configs["spsEnhIntervalRound"]);
        }
        if (configs.find("spsEnhHysterPerc") != configs.end()) {
            congCtrlConfig.spsEnhHysterPerc = stoi(configs["spsEnhHysterPerc"]);
        }
        if (configs.find("spsEnhDelayPerc") != configs.end()) {
            congCtrlConfig.spsEnhDelayPerc = stoi(configs["spsEnhDelayPerc"]);
        }

    }
}

void ApplicationBase::simTxSetup(const string ipv4, const uint16_t port) {
    RadioOpt radioOpt;
    radioOpt.ipv4_src = configuration.ipv4_src;
    simTransmit = std::unique_ptr<RadioTransmit>
            (new RadioTransmit(radioOpt, ipv4, port));
    simTransmit->set_radio_verbosity(this->configuration.codecVerbosity);
    txSimMsg = std::make_shared<msg_contents>(msg_contents{0});
    abuf_alloc(&txSimMsg->abuf, ABUF_LEN, ABUF_HEADROOM);
}

void ApplicationBase::simRxSetup(const string ipv4, const uint16_t port) {
    if (this->configuration.ldmSize && this->ldm == nullptr) {
        this->ldm = new Ldm(this->configuration.ldmSize);
    }
    RadioOpt radioOpt;
    radioOpt.ipv4_src = configuration.ipv4_src;
    simReceive = std::unique_ptr<RadioReceive>
            (new RadioReceive(radioOpt, ipv4, port));
    simReceive->set_radio_verbosity(this->configuration.codecVerbosity);
    rxSimMsg = std::make_shared<msg_contents>(msg_contents{0});
    abuf_alloc(&rxSimMsg->abuf, ABUF_LEN, ABUF_HEADROOM);
}

// CV2X supported SPS period {20,50,100,...,900,1000} ms
int ApplicationBase::adjustSpsPeriodicity(int intervalMs) {
    if (intervalMs < 50) {
        return 20;
    } else if (intervalMs < 100) {
        return 50;
    }
    int ret = intervalMs / 100;
    if (ret >= 10) {
        return 1000;
    }
    return (ret * 100);
}

int ApplicationBase::setup(MessageType msgType, bool reSetup) {
    uint8_t i = 0;
    EventFlowInfo eventInfo;
    SpsFlowInfo spsInfo;

    // close all flows before re-setup
    if (true == reSetup) {
        std::cout << "Closing all radio\n";
        closeAllRadio();
    }

    if (MessageType::WSA == msgType) {
        spsInfo.periodicityMs = this->configuration.wsaInterval;
    } else {
        spsInfo.periodicityMs = this->configuration.spsPeriodicity;
    }
    spsInfo.periodicityMs = adjustSpsPeriodicity(spsInfo.periodicityMs);

    // set sps priority to user specified value
    spsInfo.priority = this->configuration.spsPriority;
    spsInfo.nbytesReserved = this->configuration.spsReservationSize;
    if(appVerbosity > 3) {
        cout << "SPS period set to " << spsInfo.periodicityMs << "ms" << endl;
        cout << "SPS priority set to " << static_cast<uint32_t>(spsInfo.priority) << endl;
        cout << "SPS reservation size set to " << spsInfo.nbytesReserved << endl;
    }

    for (auto port : this->configuration.spsPorts)
    {

        RadioTransmit tx(spsInfo, TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP,
                         port, this->configuration.spsServiceIDs[i]);
        // save Tx instance only if create Tx flow succeeded
        if (tx.flow) {
            this->spsTransmits.push_back(std::move(tx));
        } else {
            cerr << "ApplicationBase::setup error in creating Tx SPS flow!" <<
                    " with spsServiceId: " << this->configuration.spsServiceIDs[i]
                    << endl;
            return -1;
        }

        this->spsTransmits[i].configureIpv6(this->configuration.spsDestPorts[i],
                this->configuration.spsDestAddrs[i].c_str());
        /* radio debug */
        if (this->configuration.codecVerbosity) {
            this->spsTransmits[i].
                set_radio_verbosity(this->configuration.codecVerbosity);
        }

        // use previous content if re-setup
        if (false == reSetup) {
            std::shared_ptr<msg_contents> mc = std::make_shared<msg_contents>(msg_contents{0});
            abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
            this->spsContents.push_back(mc);
        }

        i += 1;
    }
    i = 0;
    for (auto port : this->configuration.receivePorts)
    {
        printf("Creating new rx subscription with port : %d\n", port);
        std::shared_ptr<std::vector<uint32_t>> ids = nullptr;
        if (configuration.wildcardRx == false) {
            ids = std::make_shared<std::vector<uint32_t>>(this->configuration.receiveSubIds);
        }
        RadioReceive rx(TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP, port, ids);
        // save Rx instance only if create Rx flow succeeded
        if (nullptr != rx.gRxSub) {
            this->radioReceives.push_back(rx);
        } else {
            cerr << "ApplicationBase::setup error in creating Rx subscription!";
            if (configuration.wildcardRx == false) {
                cerr << " with receiveSubIds: ";
                for(int j = 0; j < configuration.receiveSubIds.size(); j++){
                    cerr << "" << this->configuration.receiveSubIds[i]<< ", ";
                }
            }
            cerr << "" << endl;
            return -1;
        }

        /* radio debug */
        if (this->configuration.codecVerbosity && radioReceives.size()) {
            this->radioReceives[i].
                set_radio_verbosity(this->configuration.codecVerbosity);
        }

        // use previous content if re-setup
        if (false == reSetup) {
            std::shared_ptr<msg_contents> mc = std::make_shared<msg_contents>(msg_contents{0});
            abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
            this->receivedContents.push_back(mc);
        }
        i += 1;
    }
    i = 0;
    for (auto port : this->configuration.eventPorts)
    {
        RadioTransmit tx(eventInfo, TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP,
                         port, this->configuration.eventServiceIDs[i]);
        // save Tx instance only if create Tx flow succeeded
        if (tx.flow) {
            this->eventTransmits.push_back(std::move(tx));
        } else {
            cerr << "ApplicationBase::setup error in creating Tx event flow!"
                    << endl;
            return -1;
        }
        this->eventTransmits[i].configureIpv6(this->configuration.eventDestPorts[i],
                this->configuration.eventDestAddrs[i].c_str());
        /* radio debug */
        if (this->configuration.codecVerbosity) {
            this->eventTransmits[i].
                set_radio_verbosity(this->configuration.codecVerbosity);
        }

        // use previous content if re-setup
        if (false == reSetup) {
            std::shared_ptr<msg_contents> mc = std::make_shared<msg_contents>(msg_contents{0});
            abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
            this->eventContents.push_back(mc);
        }
        i += 1;
    }

    // setup ldm
    if(this->configuration.ldmSize and this->radioReceives.size() > 0 and !this->ldm){
        this->ldm = new Ldm(this->configuration.ldmSize, this->radioReceives[0].getCv2xRadio());
    }
    return 0;
}

void ApplicationBase::setupLdm(){
    this->ldm->startGb(this->configuration.ldmGbTime,
            this->configuration.ldmGbTimeThreshold);
    this->ldm->packeLossThresh = this->configuration.packetError;
    this->ldm->distanceThresh = this->configuration.distance3D;
    this->ldm->positionCertaintyThresh = this->configuration.uncertainty3D;
    this->ldm->tuncThresh = this->configuration.tunc;
    this->ldm->ageThresh = this->configuration.age;
    this->ldm->setLdmVerbosity(this->configuration.ldmVerbosity);
}

void ApplicationBase::fillSecurity(ieee1609_2_data *secData) {
    secData->protocolVersion = 3;
    if (this->configuration.enableSecurity == true)
        secData->content = signedData;
    else
        secData->content = unsecuredData;
    secData->tagclass = (ieee1609_2_tagclass)2;
}

// This function maybe overloaded to perform additonal operation before calling
// radio tx function.
int ApplicationBase::transmit(uint8_t index, std::shared_ptr<msg_contents> mc,
    int16_t bufLen, TransmitType txType) {
    std::thread::id tid = std::this_thread::get_id();
    if(MsgType ==  MessageType::BSM) {
        qMon->tData[tid].txBSMs++;
    }
    // If positive, should be the # of bytes sent
    // Else, something went wrong
    int ret = -1;

    // ethernet
    if (this->isTxSim) {
        ret = simTransmit->transmit(mc->abuf.data, bufLen,
                                    this->configuration.eventPriority);
    }else{ // radio
        if (txType == TransmitType::SPS) {
            // SPS priority is set when creating the flow
            ret =  this->spsTransmits[index].transmit(mc->abuf.data, bufLen,
                                                      Priority::PRIORITY_UNKNOWN);
        } else if (txType == TransmitType::EVENT) {
            // event priority is set per packet using traffic class
            ret =this->eventTransmits[index].transmit(mc->abuf.data, bufLen,
                                                      this->configuration.eventPriority);
        }
    }
    return ret;
}

int ApplicationBase::send(uint8_t index, TransmitType txType) {
    const auto i = index;
    auto encLength = 0;
    std::shared_ptr<msg_contents> mc = nullptr;
    bool validMessage = false;
    uint64_t currTime = 0;
    // if congestion control enabled, only send when congestionControl tells us to:
    if(this->configuration.enableCongCtrl && congCtrlInitialized
        && !(criticalState && txType == TransmitType::EVENT) && !exitApp){
        sem_wait (congestionControlManager->
                    getCongestionControlUserData()->congestionControlSem);
    }
    if(exitApp){
        return -1;
    }

    if (this->isTxSim) {
        mc = txSimMsg;
    } else if (txType == TransmitType::SPS) {
        mc = spsContents[i];
    } else if (txType == TransmitType::EVENT) {
        mc = eventContents[i];
    } else {
        return -1;
    }

#if AEROLINK
    // check if identiy and cert change needs to be performed
    if(configuration.enableSecurity && !this->configuration.lcmName.empty()
            && this->configuration.idChangeInterval && kinematicsReceive &&
            appLocListener_ && hvLocationInfo) {
        changeIdentity(idChangeData.idChangeCbSem);
    }
#endif
    // reserve headroom in abuf if padding is specified
    abuf_reset(&mc->abuf, ABUF_HEADROOM + this->configuration.padding);
    fillMsg(mc);
    encLength = encode_msg(mc.get());
    if (this->configuration.enableSecurity) {
        encLength =
            encodeAndSignMsg(mc,
                             txType == TransmitType::EVENT ?
                             SecurityService::SignType::ST_CERTIFICATE :
                             SecurityService::SignType::ST_AUTO);
    }
    // this will avoid any log writing if the log file was never opened
    // save timestamp before sendto
    currTime = timestamp_now();
    if(this->configuration.enableCongCtrl && txType != TransmitType::EVENT && !exitApp){
        /* Will start it here because to prevent desynchronization
            between the transmit thread and congestion control startup */
        /* Start congestion control threads */
        if(!congCtrlInitialized){
            auto &v2xPropFactory = V2xPropFactory::getInstance();
            congestionControlManager = v2xPropFactory.getCongestionControlManager();
            congCtrlListener = std::make_shared<QitsCongCtrlListener>();
            congestionControlManager->registerListener(congCtrlListener);
            congCtrlInitialized = true;
            congestionControlManager->updateCongestionControlType
                ((CongestionControlType)congCtrlConfig.congCtrlType);
            CongestionControlUtility::setLoggingLevel(congCtrlConfig.enableCongCtrlLogging);
            congestionControlManager->updateCbpConfig(congCtrlConfig.cbpWeightFactor,
                congCtrlConfig.cbpMeasInterval);
            congestionControlManager->enableSpsEnhancements
                (congCtrlConfig.enableSpsEnhancements);
            congestionControlManager->updatePERConfig(congCtrlConfig.perMax,
                congCtrlConfig.perInterval, congCtrlConfig.perSubInterval);
            congestionControlManager->updateDensConfig
                (congCtrlConfig.vDensityCoefficient, congCtrlConfig.vDensityWeightFactor,
                congCtrlConfig.vDensityMinPerRange);
            congestionControlManager->updateTeConfig
                (congCtrlConfig.txCtrlInterval, congCtrlConfig.hvTEMinTimeDiff,
                congCtrlConfig.hvTEMaxTimeDiff, congCtrlConfig.rvTEMinTimeDiff,
                congCtrlConfig.rvTEMaxTimeDiff, congCtrlConfig.teMinThresh,
                congCtrlConfig.teMaxThresh, congCtrlConfig.teErrSensitivity);
            congestionControlManager->updateIttConfig(congCtrlConfig.reschedThresh,
            congCtrlConfig.timeAccuracy,
                congCtrlConfig.minItt, congCtrlConfig.maxItt, congCtrlConfig.txRand);
            if(congCtrlConfig.enableSpsEnhancements && !this->isTxSim){
                congestionControlManager->updateSpsEnhanceConfig
                    (congCtrlConfig.spsEnhIntervalRound, congCtrlConfig.spsEnhDelayPerc,
                    congCtrlConfig.spsEnhHysterPerc);
                // need to use squish api for this in future
                QitsCongCtrlListener::spsTransmit_ = &this->spsTransmits[index];
            }
            if(CCErrorCode::SUCCESS !=
                    congestionControlManager->startCongestionControl()){
                std::cerr << "Congestion control manager failed start up\n";
            }
            congCtrlCbDataPtr = std::make_shared<CongestionControlUserData>();
            sem_wait (congestionControlManager->
                    getCongestionControlUserData()->congestionControlSem);
            currTime = timestamp_now();
            lastTxTime = currTime;
        }
    }
    int ret = 0;
    if((criticalState && txType == TransmitType::EVENT) ||
       (!criticalState && txType == TransmitType::SPS)) {
        ret = this->transmit(index, mc, encLength, txType);
        if (encLength > 0 && ret > 0) {
            validMessage = true;
            if (csvfp || enableDiagLog_) {
                currTime = timestamp_now();

                txInterval = currTime - lastTxTime;
                if(lastTxTime == 0){
                    txInterval = 0;
                }
                lastTxTime = currTime;

                // check if valid msg contents pointer
                if (mc) {
                    int psid = 0;
                    if(mc->wsmp){
                        psid = ((wsmp_data_t *)mc->wsmp)->psid;
                    }
                    if(psid == PSID_BSM && mc.get()->j2735_msg ){
                        // we do not care about non bsms for now
                        bsm_value_t *bsm = (bsm_value_t*)(mc.get()->j2735_msg);
                        bsm_data bs = {0};
                        bs.id = bsm->id;
                        bs.timestamp_ms = bsm->timestamp_ms;
                        bs.secMark_ms = bsm->secMark_ms;
                        bs.MsgCount = bsm->MsgCount;
                        bs.Latitude = bsm->Latitude;
                        bs.Longitude = bsm->Longitude;
                        bs.Elevation = bsm->Elevation;
                        bs.SemiMajorAxisAccuracy = bsm->SemiMajorAxisAccuracy;
                        bs.SemiMinorAxisAccuracy = bsm->SemiMinorAxisAccuracy;
                        bs.SemiMajorAxisOrientation = bsm->SemiMajorAxisOrientation;
                        bs.TransmissionState = bsm->TransmissionState;
                        bs.Speed = bsm->Speed;
                        bs.Heading_degrees = bsm->Heading_degrees;
                        bs.SteeringWheelAngle = bsm->SteeringWheelAngle;
                        bs.AccelLon_cm_per_sec_squared = bsm->AccelLon_cm_per_sec_squared;
                        bs.AccelLat_cm_per_sec_squared = bsm->AccelLat_cm_per_sec_squared;
                        bs.AccelVert_two_centi_gs = bsm->AccelVert_two_centi_gs;
                        bs.AccelYaw_centi_degrees_per_sec = bsm->AccelYaw_centi_degrees_per_sec;
                        bs.brakes = bsm->brakes;
                        bs.VehicleWidth_cm = bsm->VehicleWidth_cm;
                        bs.VehicleLength_cm = bsm->VehicleLength_cm;
                        bs.events = bsm->events;
                        uint8_t cbr;
                        uint64_t monotonicTime;
                        if (txType == TransmitType::SPS) {
                            cbr = spsTransmits[index].getCBRValue();
                            monotonicTime = spsTransmits[index].latestTxRxTimeMonotonic();
                        } else {
                            cbr = eventTransmits[index].getCBRValue();
                            monotonicTime = eventTransmits[index].latestTxRxTimeMonotonic();
                        }
                        if (csvfp) {
                            // write the log here for this tx now.
                            // using tx timestamp made before sendto
                            writeLog(index, 0, true, txType, validMessage, currTime, PSID_BSM,
                                monotonicTime, 0.0, 0, 0, cbr, &bs, 0.0, 0, txInterval,
                                configuration.enableCongCtrl, congCtrlInitialized, &writeMutexCv);
                        }
                        if (enableDiagLog_) {
                            DiagLogData logData = {0};
                            logData.validPkt = validMessage;
                            logData.currTime = currTime;
                            logData.cbr = cbr;
                            logData.monotonicTime = monotonicTime;
                            logData.txInterval = txInterval;
                            logData.enableCongCtrl = configuration.enableCongCtrl;
                            logData.monotonicTime = congCtrlInitialized;
                            diagLogPktTxRx(true, txType, &logData, &bs);
                        }
                    }
                }
            }
        }
        if(kinematicsReceive && appLocListener_ && hvLocationInfo){
            {
                lock_guard<mutex> lk(hvLocUpdateMtx);
                locTimeMs_ = hvLocationInfo->getTimeStamp();
                locPositionDop_ = hvLocationInfo->getPositionDop();
                locNumSvUsed_ = hvLocationInfo->getNumSvUsed();
            }
        }
    }
    return (validMessage ? encLength : 0);
}

int ApplicationBase::encodeAndSignMsg(std::shared_ptr<msg_contents> mc,
                                      SecurityService::SignType type) {
    // The message need to be signed/encrypted after layer 3
    SecurityOpt sopt = {};
    uint8_t signedSpdu[512];
    uint32_t signedSpduLen = 512;
    sopt.psidValue = this->configuration.psid;
    if (this->configuration.sspLength){
         memcpy(sopt.sspValue, this->configuration.ssp,
            this->configuration.sspLength);
         if(this->configuration.enableSspMask && this->configuration.sspMaskLength){
            memcpy(sopt.sspMaskValue, this->configuration.sspMask,
                this->configuration.sspMaskLength);
         }
        sopt.sspLength = this->configuration.sspLength;
        sopt.sspMaskLength = this->configuration.sspMaskLength;
    }
    sopt.secVerbosity = this->configuration.secVerbosity;
    std::thread::id tid = std::this_thread::get_id();
    if(configuration.enableSignStatLog){
        if (thrSignLatencies[tid].size() > signStatIdx[tid]) {
            sopt.signStat = &thrSignLatencies[tid].at(signStatIdx[tid]);
        }else{
            signStatIdx[tid] = 0;
            sopt.signStat = &thrSignLatencies[tid].at(signStatIdx[tid]);
        }
    }
    auto encLength = 0;
    if (mc->abuf.tail_bits_left != 8)
        encLength = mc->abuf.tail - mc->abuf.data + 1;
    else
        encLength = mc->abuf.tail - mc->abuf.data;

    // Aerolink handles IEEE1609.2 header insertion, but this requires us to
    // make buffer copy of the header and the payload.
    if (SecService->SignMsg(sopt, (uint8_t*)mc->abuf.data,
                            encLength, signedSpdu, signedSpduLen, type) < 0) {
        signFail++;
        return -1;
    }else{
        signSuccess++;
    }
    if(configuration.enableSignStatLog){
        // successful signing, increment the sign stat idx
        signStatIdx[tid]++;
        signStatIdx[tid]%=thrSignLatencies[tid].size();
    }
    abuf_reset(&mc->abuf, ABUF_HEADROOM + this->configuration.padding);
    asn_ncat(&mc->abuf, (char *)signedSpdu, signedSpduLen);
    // transmit packet
    return encode_msg_continue(mc.get());
}

int ApplicationBase::receive(const uint8_t index, const uint16_t bufLen) {
    return -1;
}

int ApplicationBase::receive(const uint8_t index, const uint16_t bufLen,
        const uint32_t ldmIndex) {
    return -1;
}

void ApplicationBase::closeAllRadio() {
    if(appVerbosity){
        std::cout << "Attempting to close all flows\n";
    }

    for (uint8_t i = 0; i<this->eventTransmits.size(); i++) {
        this->eventTransmits[i].closeFlow();
    }
    eventTransmits.clear();

    for (uint8_t i = 0; i < this->spsTransmits.size(); i++) {
        this->spsTransmits[i].closeFlow();
    }
    spsTransmits.clear();

    for (uint8_t i = 0; i < this->radioReceives.size(); i++) {
        this->radioReceives[i].closeFlow();
    }
    radioReceives.clear();

    if(appVerbosity){
        std::cout << "Finished closing all flows\n";
    }
}

/**
 * Instantiate and initialize any variables associated with
 * security statistics logging
 */
void ApplicationBase::initVerifLogging() {
    std::vector<VerifStats> stats;
    sem_wait(&this->log_sem);
    for(int i = 0 ; i < configuration.verifStatsSize; i++)
        stats.push_back(VerifStats());
    thrVerifLatencies[std::this_thread::get_id()] = stats;
    if(remove(configuration.verifStatLogFile.c_str()) != 0){
        if(appVerbosity > 4)
            cerr << "Error deleting log file" << endl;
    }
    sem_post(&this->log_sem);
}

/**
 * Function to print out - if any - security related statistics
 * gathered from security side
 */
void ApplicationBase::writeVerifLogging() {
    ofstream file;
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    file.open(configuration.verifStatLogFile.c_str(),
                std::ofstream::out | std::ofstream::app);
    std::vector<VerifStats> stats;
    auto itr = thrVerifLatencies.find(std::this_thread::get_id());
    if (itr != thrVerifLatencies.end()){
        stats = itr->second;
    }
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->timestamp != 0.0 && it->verifLatency != 0.0) {
            file << it->timestamp << ", " <<
                        it->verifLatency << std::endl;
        }
    }
    file.close();
    sem_post(&this->log_sem);
}

bool compareByVHz(const ResultLoggingStats &a, const ResultLoggingStats &b)
{
    return a.asyncVerifSuccess < b.asyncVerifSuccess;
}
/**
 * Instantiate and initialize any variables associated with
 * verification results and statistics logging
 */
void ApplicationBase::initResultsLogging() {
    std::vector<ResultLoggingStats> stats;
    sem_wait(&this->log_sem);
    for(int i = 0 ; i < configuration.verifResLogSize; i++)
        stats.push_back(ResultLoggingStats());
    thrResLoggingValues[std::this_thread::get_id()] = stats;
    if(remove(configuration.verifResLogFile.c_str()) != 0){
        if(appVerbosity > 4)
            cerr << "Error deleting log file" << endl;
    }
    sem_post(&this->log_sem);
}

/**
 * Function to print out - verification results and statistics
 * gathered from security side
 */
void ApplicationBase::writeResultsLogging() {
    ofstream file;
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    file.open(configuration.verifResLogFile.c_str(),
                std::ofstream::out | std::ofstream::app);
    std::vector<ResultLoggingStats> stats;
    auto itr = thrResLoggingValues.find(std::this_thread::get_id());
    if (itr != thrResLoggingValues.end()){
        stats = itr->second;
    }
    std::sort(stats.begin(), stats.end(), compareByVHz);
    file<<"ThreadID,"<<"TotalSuccessfulVerifs,"<<"BatchVerifRate (kVhz),"
        <<"BatchTimeStep (ms)"<<std::endl;
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->currTimeStamp != 0.0 && it->asyncVerifSuccess != 0.0) {
            file  <<std::hex << it->tid << std::dec << ", " <<it->asyncVerifSuccess
                << ", " << it->rate << " , " << it->dur <<std::endl;
        }
    }
    file.close();
    sem_post(&this->log_sem);
}

/**
 * Instantiate and initialize any variables associated with
 * security statistics logging
 */
void ApplicationBase::initSignLogging() {
    std::vector<SignStats> stats;
    sem_wait(&this->log_sem);
    for(int i = 0 ; i < configuration.signStatsSize; i++)
        stats.push_back(SignStats());
    thrSignLatencies[std::this_thread::get_id()] = stats;
    if(remove(configuration.signStatLogFile.c_str()) != 0){
        if(appVerbosity > 4)
            cerr << "Error deleting log file" << endl;
    }
    sem_post(&this->log_sem);
}

/**
 * Function to print out - if any - security related statistics
 * gathered from security side
 */
void ApplicationBase::writeSignLogging() {
    ofstream file;
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    file.open(configuration.signStatLogFile.c_str(),
                std::ofstream::out | std::ofstream::app);
    std::vector<SignStats> stats;
    if (auto itr = thrSignLatencies.find(std::this_thread::get_id());
            itr != thrSignLatencies.end()){
        stats = itr->second;
    }
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->timestamp != 0.0 && it->signLatency != 0.0) {
            file << it->timestamp << ", " <<
                        it->signLatency << std::endl;
        }
    }
    file.close();
    sem_post(&this->log_sem);
}

/**
 * Instantiate and initialize any variables associated with
 *  Misbehavior statistics logging
 */
void ApplicationBase::initMisbehaviorLogging() {
    std::vector<MisbehaviorStats> stats;
    sem_wait(&this->log_sem);
    for(int i = 0 ; i < configuration.mbdStatLogListSize; i++)
        stats.push_back(MisbehaviorStats());
    thrMisbehaviorLatencies[std::this_thread::get_id()] = stats;
    if(remove(configuration.mbdStatLogFile.c_str()) != 0){
        if(appVerbosity > 4)
            cerr << "Error deleting log file" << endl;
    }
    sem_post(&this->log_sem);
}

/**
 * Function to print out - if any - Misbehavior related statistics
 * gathered from security side.
 */
void ApplicationBase::writeMisbehaviorLogging() {
    ofstream file;
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    file.open(configuration.mbdStatLogFile.c_str(),
                std::ofstream::out | std::ofstream::app);
    std::vector<MisbehaviorStats> stats = thrMisbehaviorLatencies[std::this_thread::get_id()];
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->timestamp != 0.0 && it->misbehaviorLatency != 0.0) {
            file << it->timestamp << ", " <<
                        it->misbehaviorLatency << std::endl;
        }
    }
    file.close();
    sem_post(&this->log_sem);
}

int ApplicationBase::getSysV2xIpIfaceAddr(string& ipAddr) {
    int result = -1;
    struct ifaddrs *ifap;
    struct ifaddrs *ifa;
    char addr[INET6_ADDRSTRLEN];
    string v2xIfName;

    //get V2X-IP iface name from the radio instance used for Tx WSA
    if (this->spsTransmits.empty()
        or this->spsTransmits[0].getV2xIfaceName(TrafficIpType::TRAFFIC_IP, v2xIfName)
        or v2xIfName.empty()) {
        cerr << "Failed to get V2X-IP iface name" << endl;
        return -1;
    }

    if (-1 == getifaddrs(&ifap)) {
        cerr << "Failed to get ifaddr!" << endl;
        return -1;
    }

    ifa = ifap;
    while (ifa && ifa->ifa_name) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6) {
            string ifaName(ifa->ifa_name);
            if (ifaName == v2xIfName) {
                getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), addr, sizeof(addr),
                            NULL, 0, NI_NUMERICHOST);
                ipAddr = string(addr);
                if(appVerbosity > 3) {
                    cout << "Found V2X ifaceName:" << ifaName << " addr:" << ipAddr << endl;
                }
                result = 0;
                break;
            }
        }
        ifa = ifa->ifa_next;
    }
    freeifaddrs(ifap);

    if (result) {
        cerr << "Found no global IPv6 address for V2X IP iface!" << endl;
    }

    return result;
}

int ApplicationBase::updateCachedV2xIpIfaceAddr() {
    // get the old addr
    string oldAddr;
    {
        std::unique_lock<std::mutex> lock(v2xIpAddrMtx_);
        oldAddr = v2xIpAddr_;
    }

    string newAddr;
    if (0 == getSysV2xIpIfaceAddr(newAddr) and !newAddr.empty()) {
        if (oldAddr == newAddr) {
            cout << "V2X IP address not changed!" << endl;
        } else {
            // update local stored address
            std::unique_lock<std::mutex> lock(v2xIpAddrMtx_);
            v2xIpAddr_ = newAddr;
            if (appVerbosity > 3) {
                cout << "V2X IP address is upated to:" << newAddr << endl;
            }
            return 0;
        }
    }

    cerr << "Failed to update V2X IP iface address!" << endl;
    return -1;
}

int ApplicationBase::getV2xIpIfaceAddr(string& addr) {
    {
        std::unique_lock<std::mutex> lock(v2xIpAddrMtx_);
        addr = v2xIpAddr_;
    }

    if (addr.empty()) {
        cout << "Get V2X IP address failed!" << endl;
        return -1;
    }

    if (appVerbosity > 3) {
        cout << "Get V2X IP address " << addr << endl;
    }
    return 0;
}

bool ApplicationBase::openBsmLogFile(const std::string& fullPathName) {
    bool res = false;
    if (not enableCsvLog_) {
        return res;
    }
    {
        lock_guard<std::mutex> lock(csvMutex);
        if ((nullptr == csvfp) && !fullPathName.empty()) {
            csvfp = fopen(fullPathName.c_str(), "w+");
            if (!csvfp) {
                cerr << "Failed to open log file " << fullPathName << std::endl;
            } else {
                if(configuration.appVerbosity){
                    std::cout << "Open log " << fullPathName
                        << " success!" << std::endl;
                }
                res = true;
                write_bsm_header(csvfp);
            }
        }
    }
    if (res) {
        for(int index = 0 ; index < spsTransmits.size(); index++) {
            this->spsTransmits[index].enableCsvLog(enableCsvLog_);
        }
        for(int index = 0 ; index < eventTransmits.size(); index++) {
            this->eventTransmits[index].enableCsvLog(enableCsvLog_);
        }
        for(int index = 0 ; index < radioReceives.size(); index++) {
            this->radioReceives[index].enableCsvLog(enableCsvLog_);
        }
    }

    return res;
}

/* Log Format:
 * TimeStamp    TimeStamp_ms    Time_monotonic
 * LogRecType   L2 ID    CBR Percent    CPU_Util
 * TXInterval   msgCnt  TempId  GPGSAMode
 * secMark  lat long    semi_major_dev  speed
 * heading  longAccel   latAccel    Tracking_Error
 * vehicleDensityInRange    ChannelQualityIndication
 * BSMValid max_ITT GPS-Time    Events  DCC random time Hysterisis
 */
void ApplicationBase::writeLogHeader(FILE *fp) {
    // Writes log header to the csv file pointed by fp.
    fprintf(fp, LOG_HEADER);
    // TODO add security headers here too
    fprintf(fp, "\n");
}

bool ApplicationBase::openLogFile(const std::string& fullPathName) {
    bool res = false;
    if (not enableCsvLog_) {
        return res;
    }
    {
        lock_guard<std::mutex> lock(csvMutex);
        if ((nullptr == csvfp) && !fullPathName.empty()) {
            csvfp = fopen(fullPathName.c_str(), "w+");
            if (!csvfp) {
                cerr << "Failed to open log file " << fullPathName << std::endl;
            } else {
                std::cout << "Open log " << fullPathName << " success!" << std::endl;
                res = true;
                writeLogHeader(csvfp);
            }
        }
    }
    if (res) {
        for(int index = 0 ; index < spsTransmits.size(); index++) {
            this->spsTransmits[index].enableCsvLog(enableCsvLog_);
        }
        for(int index = 0 ; index < eventTransmits.size(); index++) {
            this->eventTransmits[index].enableCsvLog(enableCsvLog_);
        }
        for(int index = 0 ; index < radioReceives.size(); index++) {
            this->radioReceives[index].enableCsvLog(enableCsvLog_);
        }
    }

    return res;
}


void ApplicationBase::writeLog(const uint8_t index,
    uint32_t l2SrcAddr, bool isTx, TransmitType txType, bool validPkt,
    uint64_t timestamp, uint32_t psid, uint64_t monotonicTime,
    float locPositionDop, uint16_t locNumSvUsed, uint64_t locTimeMs, uint8_t cbr,
    bsm_data* bs, double distFromRV, uint32_t RVsInRange,
    uint64_t txInterval, bool enableCongCtrl, bool congCtrlInitialized,
    std::condition_variable* writeMutexCv) {
    uint64_t periodicityMs = 0;
    int res = -1;
    struct timespec ts;
    uint64_t timestamp_now_ms = 0;

    if (not csvfp) {
        return;
    }
    // build a string and then only lock for just that part in writing to the file
    char tmpLogBuf[650] = "";
    char* curChar = tmpLogBuf;
    char* const endChar = tmpLogBuf + sizeof tmpLogBuf - 1;
    char tmpLogStr[200] = "";
    timestamp_now_ms = timestamp_now();

    /* Log Format:
     * TimeStamp    TimeStamp_ms    Time_monotonic
     * LogRecType   L2 ID    CBR Percent    CPU_Util
     * TXInterval   msgCnt  TempId  GPGSAMode
     * secMark  lat long    semi_major_dev  speed
     * heading  longAccel   latAccel    Tracking_Error
     * vehicleDensityInRange    ChannelQualityIndication
     * BSMValid max_ITT GPS-Time    Events  DCC random time Hysterisis
     */
    if(exitApp){
        return;
    }
    // write general data to log
    // build the string in this function instead of immediately writing
    int ret = 0;
    if(writeMutexCv && !exitApp){
        lock_guard<std::mutex> lock(csvMutex);
        writeLogFinish = false;
    }
    ret  = writeGeneralLog(tmpLogStr, 200, bs, csvfp, isTx, periodicityMs, validPkt,
                RVsInRange, getCurrentTimestamp().c_str(), monotonicTime, timestamp,
                locPositionDop, locNumSvUsed, locTimeMs, cbr, txInterval, l2SrcAddr);

    // check if error in writing general data
    if(ret == -1){
        return;
    }
    curChar += snprintf(curChar, endChar-curChar, "%s", tmpLogStr);
    // if congestion control enabled, write cong ctrl data to log
    unsigned short eventsData = getEventsData(&(bs->events));

    if (enableCongCtrl && congCtrlInitialized && isTx) {
        // get a snapshot of the current cong control calculation
        writeCongCtrlLog(tmpLogStr, 200, csvfp,
            &congCtrlCbData,
            validPkt, eventsData);
    }else{
        // make sure to write commas for the empty fields
        snprintf(tmpLogStr, 200, "0.0,0.0,0.0,%d,%lu,0.0,%u,0,%d",
            validPkt ? 1 : 0,
            (enableCongCtrl && congCtrlInitialized) ?
              congCtrlCbData.maxITT : 0,
            eventsData, 5);
    }
    curChar += snprintf(curChar, endChar-curChar, "%s", tmpLogStr);
    memset(tmpLogStr, 0, sizeof(tmpLogStr));
    if(enableCongCtrl && congCtrlInitialized && !isTx){
        char* tmpPtr = tmpLogStr;
        char* const endBuf = tmpPtr + 50;
        snprintf(tmpLogStr, endBuf-tmpPtr, ",%d,%f", congCtrlCbData.totalRvsInRange,
            bs->distFromRV);
        curChar += snprintf(curChar, endChar-curChar, "%s", tmpLogStr);
    }

    {

        if(writeMutexCv && !exitApp){
            lock_guard<std::mutex> lock(csvMutex);

            // lock here to prevent race conditions when writing to file
            fprintf(csvfp, "%s\n", tmpLogBuf);
            writeLogFinish = true;
            writeMutexCv->notify_all();
        }
    }
}
