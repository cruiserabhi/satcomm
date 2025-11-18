/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
  * @file: Ldm.cpp
  *
  * @brief: Implementation of Ldm.
  *
  */



#include "Ldm.h"
#include "RadioInterface.h"
#include <memory>
using std::map;
using std::vector;
using std::pair;
using std::mutex;
using std::lock_guard;
using std::find;
using std::cout;
using std::endl;
using std::dec;
using std::shared_ptr;
using telux::cv2x::TrustedUEInfo;
using telux::cv2x::TrafficCategory;
static bool stopThread = false;

// mutexes defined in header
/*
     mutex sync
     mutex freeSlotMutex;
     mutex idIndexMapMutex;
     mutex ldmContentsMutex;
*/

Ldm::Ldm(const uint16_t size, shared_ptr<ICv2xRadio> radio) {
    this->bsmContents.reserve(2 * size);
    for (uint16_t i = 0; i < size; i++)
    {
        msg_contents msg = {0};
        this->bsmContents.push_back(std::make_shared<msg_contents>(msg));
        this->bsmFreeSlotIndices.push_back(i);
    }
    cv2xRadio_ = radio;
}

int Ldm::getIndex(const uint32_t id) {
    map<uint32_t, uint32_t>::iterator iter = this->bsmIdIndexMap.find(id);
    if (iter != this->bsmIdIndexMap.end()){
        return this->bsmIdIndexMap[id];
    }
    else {
        return INVALID_DATA;
    }
}

void Ldm::setIndex(const uint32_t rvId, const uint32_t freeSlotIndex,
        std::shared_ptr<msg_contents> mc) {
    const auto usedSlotIndex = this->getIndex(rvId);
    lock_guard<mutex> lk(this->idIndexMapMutex);
    if (usedSlotIndex != INVALID_DATA && usedSlotIndex != DIRTY_DATA) {
        lock_guard<mutex> lk2(this->freeSlotMutex);
        this->bsmFreeSlotIndices.push_back(usedSlotIndex);
        this->bsmIdIndexMap[rvId] = freeSlotIndex;
    }
    else {
        if (usedSlotIndex == DIRTY_DATA)
        {
            this->bsmIdIndexMap[rvId] = freeSlotIndex;
        }
        else {
            this->bsmIdIndexMap.insert
                    (pair<uint32_t, uint32_t>(rvId, freeSlotIndex));
        }
    }
    if(ldmVerbosity > 1){
        printf("Copying decoded bsm of car id %d into ldm at index: %d\n",
                    rvId, freeSlotIndex);
        printf("Bsm summary: \n");
    }
    // here, for whatever reason, they are providing the specific bsm
    if(mc != nullptr){
        if(ldmVerbosity > 1){
            print_summary_RV(mc.get());
        }
        if(mc->j2735_msg != nullptr){
            lock_guard<mutex> lk3(this->ldmContentsMutex);
                this->bsmContents[freeSlotIndex]->j2735_msg =
                    (bsm_value_t *) calloc(1, sizeof(bsm_value_t));
                memcpy(reinterpret_cast<bsm_value_t *>
                            (this->bsmContents[freeSlotIndex]->j2735_msg),
                            mc->j2735_msg, sizeof(bsm_value_t));
        }
    }else{
        // here, they directly decoded the bsm into a returned free slot index
        if(ldmVerbosity > 1){
            print_summary_RV(bsmContents[freeSlotIndex].get());
        }
    }
}

uint32_t Ldm::getFreeBsmSlotIdx() {
    lock_guard<mutex> lk(this->freeSlotMutex);
    if (!this->bsmFreeSlotIndices.empty()){
        uint32_t freeSlotIndex = this->bsmFreeSlotIndices.front();
        this->bsmFreeSlotIndices.pop_front();
        return freeSlotIndex;
    }
    else {
        msg_contents msg = {0};
        lock_guard<mutex> lk2(this->ldmContentsMutex);
        this->bsmContents.push_back(std::make_shared<msg_contents>(msg));
        return this->bsmContents.size() - 1;
    }
}

bool Ldm::hasBsm(const uint32_t id){
    map<uint32_t, uint32_t>::iterator iter = this->bsmIdIndexMap.find(id);
    return (iter != this->bsmIdIndexMap.end());
}

void Ldm::gbCollector(const uint16_t waitTime, const uint8_t timeThreshold) {
    while (!stopThread) {
        if(ldmVerbosity){
            printLdmIdMap();
        }
        this->idIndexMapMutex.lock();
        // go through id - slot map to remove old ldm contents
        for (pair<uint32_t, uint32_t> element : this->bsmIdIndexMap) {
            if(element.second != DIRTY_DATA &&
                    this->bsmContents[element.second]->j2735_msg != nullptr){
                const auto now = timestamp_now();
                bsm_value_t *bsmp = reinterpret_cast<bsm_value_t *>
                          (this->bsmContents[element.second]->j2735_msg);
                const auto dif = now - bsmp->timestamp_ms;
                if (timeThreshold * 10000 < dif) {
                    if(ldmVerbosity > 1){
                        cout << "Packet for RV " << dec << element.first <<
                                " is too old now" << endl;
                        print_summary_RV(bsmContents[element.second].get());
                        cout << "Time Dif (ms): " << dec << dif << endl;
                    }
                    // requires locking for free slot indices struct?
                    this->bsmFreeSlotIndices.push_back(element.second);
                    this->bsmIdIndexMap[element.first] = DIRTY_DATA;
                    // remove element instead?

                    if(ldmVerbosity > 1){
                        printf("Back index value of free indices is: %u\n",
                           this->bsmFreeSlotIndices.back());
                        printf("Removing old bsm at slot: %d\n", element.second);
                    }
                }
            }
        }
        this->idIndexMapMutex.unlock();
        sleep(waitTime);
    }
}

void Ldm::startGb(const uint16_t gbTime, const uint8_t timeThreshold) {
    auto gbThread = [&](uint16_t waitTime, uint8_t timeTresh) {
                            gbCollector(waitTime, timeTresh);};

    if (!gbStarted) {
        this->gbThread = thread(gbThread, gbTime, timeThreshold);
        gbStarted = true;
    }
    else {
        if(ldmVerbosity)
            cout << "Garbage Collector already started.";
    }
}

void Ldm::stopGb(){
    if(gbStopped){
        return;
    }
    if(ldmVerbosity)
        cout << "Stopping Garbage Collector.\n";
    stopThread = true;
    gbStopped = true;
}

void Ldm::cv2xUpdateTrustedUEListCallback(ErrorCode error) {
    if (ErrorCode::SUCCESS != error) {
        cout << "Error Updating UE List.\n";
    }
}

void Ldm::trustedScan() {
    auto i = 0;
    while (true) {
        for (auto info : tunnelTimingInfoList.trustedUEs) {
            //TODO SDK needs timestamp to take away UEs that are too old.
        }
        auto respCb = [&](ErrorCode error) {
                cv2xUpdateTrustedUEListCallback(error);
        };

        if (!cv2xRadio_ or
            Status::SUCCESS != cv2xRadio_->updateTrustedUEList(tunnelTimingInfoList, respCb)) {
            cerr << "update trusted UE list failed!" << endl;
            return;
        }

        sleep(5);
    }
}

void Ldm::startTrusted() {
    auto trustedThread = [&]() {
        trustedScan();
    };

    if (!trustedStarted) {
        this->trustedThread = thread(trustedThread);
        trustedStarted = true;
    }
    else {
        if(ldmVerbosity)
            cout << "Trust and Malicious list scan already started running.";
    }
}

/*
* TODO: Print out following contents
* Basic info for each RV from BSM (maybe CAM can be later)
* tempID
* Range in meters
* Time to collision (TTC)
* last bytes/digest of signing cert
* time since last heard
* PPPP
* length /width – very important at plugtests for idenifying the OEM
* Decoded event flags (highlight if critical event)
*/
void Ldm::printLdmIdMap() {
    cout << "Status of Ldm Contents: " << endl;
    cout << "Total Slots in Ldm: " << this->bsmContents.size() << endl;
    cout << "Free Slots in Ldm: " << this->bsmFreeSlotIndices.size() << endl;
    cout << "Total Unique RVs Seen: " << this->bsmIdIndexMap.size() << endl;
    lock_guard<mutex> lk(this->idIndexMapMutex);
    lock_guard<mutex> lk2(this->ldmContentsMutex);
    auto activeRvIds = 0;
    for (pair<uint32_t, uint32_t> element : this->bsmIdIndexMap) {
        if (element.second != INVALID_DATA && element.second != DIRTY_DATA)
        {
            cout << "Temp Id: " << dec << element.first <<
                " has data in slot " << dec << element.second << endl;
            cout << "BSM Summary:\n";
            print_summary_RV(this->bsmContents[element.second].get());
            activeRvIds++;
        }
    }
    cout << "Total Unique RVs: " << activeRvIds << endl;
}

bool Ldm::isTrusted(uint32_t id) {
    //TODO Finds if vehicle is trusted by iterating STL vector
    //tunnelTimingInfoList.trustedUEs

    return true;
}

list<shared_ptr<msg_contents>> Ldm::bsmSnapshot() {
    lock_guard<mutex> lk(this->idIndexMapMutex);
    lock_guard<mutex> lk2(this->ldmContentsMutex);
    list<shared_ptr<msg_contents>> snap;
    auto i = 0;
    for (pair<uint32_t, uint32_t> element : this->bsmIdIndexMap) {
        if (element.second != INVALID_DATA && element.second != DIRTY_DATA)
        {
            snap.push_back(std::make_shared<msg_contents>());
            //snap[snap.size()-1] = std::move(bsmContents[element.second]);
        }
    }

    return snap;
}

list<shared_ptr<msg_contents>> Ldm::bsmTrustedSnapshot() {
    lock_guard<mutex> lk(this->idIndexMapMutex);
    lock_guard<mutex> lk2(this->ldmContentsMutex);
    list<shared_ptr<msg_contents>> snap;
    auto i = 0;
    for (pair<uint32_t, uint32_t> element : this->bsmIdIndexMap) {
        if (element.second != INVALID_DATA && element.second != DIRTY_DATA)
        {
            if (isTrusted(element.first)) {
                snap.push_back(std::make_shared<msg_contents>());
                //snap[snap.size()-1] = std::move(bsmContents[element.second]);
            }
        }
    }

    return snap;
}

bool Ldm::validCert(uint32_t id) {
    //TODO Implement security validation of certs.
    return true;

}

bool Ldm::filterBsm(const uint32_t index) {
    const auto i = index;
    //TODO
    //After bsm has been decoded check security.
    //If is wrong, give index to freeBsm contents and put MAC address in malicious list.
    //if verified, give id to trusted list.
    //Returns true if message has been filtered, false else.
    msg_contents* msg = this->bsmContents[index].get();
    bsm_value_t *bsm = reinterpret_cast<bsm_value_t *>(msg->j2735_msg);
    //const auto id = msg->j2735.bsm.id; // FIX: Use L2 instead of temp ID.
    const auto id = bsm->id;
    TrustedUEInfo tunnelInfo;
    const auto begin = tunnelTimingInfoList.maliciousIds.begin();
    const auto end = tunnelTimingInfoList.maliciousIds.end();
    const auto malicious = find(begin,end, id) != end;
    uint32_t age = -1;
    const auto trusted = isTrusted(id);
    if (malicious) {
        return true;
    }

    //populate info with the right data and send it to malicious.
    tunnelInfo.sourceL2Id = id; //FIX: Use L2 instead of temp ID.
    tunnelInfo.timeUncertainty = tuncs[id]; //FIX: Use bsm tunnelInfo when it is fix.
    tunnelInfo.positionConfidenceLevel = 0; //TODO:You can calculate this from BSM data.
    tunnelInfo.propagationDelay = 0; //TODO get that data.

    if (hasBsm(id))
    {
        const auto i = this->bsmIdIndexMap[id]; //Careful with parallelism, you can use a lock to access here.
        if (i != DIRTY_DATA && i != INVALID_DATA)
        {
            msg_contents* prevMsg = this->bsmContents[i].get();
            bsm_value_t *prev_bsm =
                reinterpret_cast<bsm_value_t *>(this->bsmContents[i]->j2735_msg);
            const auto packetDif = bsm->MsgCount - prev_bsm->MsgCount;
            age = prev_bsm->timestamp_ms;
            if (bsm->timestamp_ms == prev_bsm->timestamp_ms) {
                if (trusted) {
                    //TODO remove from trusted list
                }
                tunnelTimingInfoList.maliciousIds.push_back(id);
            }
            // Check for packet loss
            if (packetDif > 1 && packetDif < 127)
            {
                if (bsmPacketsLost.find(id) == bsmPacketsLost.end()) {
                    bsmPacketsLost.insert(pair<uint32_t, uint32_t>(id, packetDif));
                }
                else {
                    bsmPacketsLost[id] += packetDif;
                }
            }
        }

    }else {
        age = 0;
    }

    const auto distance = 0;//TODO calcultate 3D distance
    const auto positionCertainty = 0; //TODO calculate position certainty

    //Running Filtering Options
    if (!validCert(id) || (bsmPacketsLost[id] > packeLossThresh) || (age > ageThresh)
        || (distance <distanceThresh) || (positionCertainty > positionCertaintyThresh)
        || (tuncs[id] >tuncThresh)){
        if (trusted) {
            //TODO Remove from trusted
        }
        tunnelTimingInfoList.maliciousIds.push_back(id);
        return true;
    }
    else {
        if (!trusted) {
            //TODO Add to trusted
        }
    }

    return false;
}
