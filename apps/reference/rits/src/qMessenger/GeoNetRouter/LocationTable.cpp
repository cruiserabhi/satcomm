/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file LocationTable.cpp
 * @brief implementation of location table class
 */
#include <iostream>
#include <iomanip>
#include <climits>
#include <arpa/inet.h>
#include "GeoNetUtils.hpp"
#include "LocationTable.hpp"
using namespace std;
namespace gn {
    LocTableEntry::LocTableEntry(const gn_lpv_t &src, int stype, int version)
        :
        GnAddr_(src.gn_addr),
        StationType_(stype),
        Version_(version),
        LPV_(src),
        LocationServicePending_(false),
        PDR_(0),
        Updated_(false) {
    }

    LocTableEntry::LocTableEntry(const gn_spv_t &dst, int stype, int version)
        :
        GnAddr_(dst.gn_addr),
        StationType_(stype),
        Version_(version),
        LocationServicePending_(false),
        PDR_(0),
        Updated_(false) {
            std::memcpy(&LPV_.gn_addr, &dst.gn_addr, GN_MID_LEN);
            LPV_.tst = dst.tst;
            LPV_.latitude = dst.latitude;
            LPV_.longitude = dst.longitude;
            LPV_.pai = 0;
            LPV_.s = 0;
            LPV_.h = 0;
        }

    LocTableEntry::~LocTableEntry() {
        while(DPL_.size()) {
            DPL_.pop_front();
        }
    }
    //Perform Duplicated Address detection.
    bool LocTableEntry::isDuplicated(uint16_t sn) {
        for (auto n: DPL_) {
            if (n->sn == sn) {
                n->counter++;
                return true;
            }
        }
        if (DPL_.size() >= MAX_DPL_LEN) {
            //Remove oldest one from the front.
            DPL_.pop_front();
        }
        std::shared_ptr<dpl_element_t> ep = NULL;
        try {
            ep = std::make_shared<dpl_element_t>();
        } catch (std::bad_alloc & e) {
            cout << "LocTableEntry bad alloc" << endl;
            return false;
        }

        ep->sn = sn;
        ep->counter = 0;
        DPL_.push_back(ep);
        return false;
    }

    void LocationTable::RefreshTaskStart(void) {
        auto t = [&](void) {
            RefreshTask();
        };
        RefreshThread = std::thread(t);
    }

    void LocationTable::RefreshTaskStop(void) {
        // Purge the table
        std::unique_lock<std::mutex> lk(TableMutex_);
        TableEntries_.erase(TableEntries_.begin(), TableEntries_.end());
        lk.unlock();
        Cv_.notify_one();
        RefreshThread.join();
    }
    const std::shared_ptr<LocTableEntry> LocationTable::Find(const gn_addr_t &GnAddr) {
        return Find(const_cast<uint8_t *>(GnAddr.mid));
    }
    const std::shared_ptr<LocTableEntry> LocationTable::Find(const uint8_t *mid) {
        uint32_t tsnow = gn::GeoNetUtils::GetTimestampSinceEpoch();
        std::lock_guard<std::mutex> lock(TableMutex_);

        auto it = TableEntries_.find(const_cast<uint8_t *>(mid));

        if (it != TableEntries_.end()) {
            auto entry = it->second;
            if ((tsnow - ntohl(entry->LPV_.tst)) > LifeTime_*1000000) {
                TableEntries_.erase(it);
                //TODO: update neighbours count.
            } else {
                return it->second;
            }
        }
        return nullptr;
    }
    bool LocationTable::isDuplicated(const gn_addr_t &GnAddr, uint16_t sn) {
        auto entry = Find(GnAddr);
        if (entry == nullptr)
            return false;
        return entry->isDuplicated(sn);
    }

    const std::shared_ptr<LocTableEntry> LocationTable::Update(const gn_lpv_t &so_pv) {
        std::shared_ptr<LocTableEntry> entry = Find(so_pv.gn_addr);

        std::lock_guard<std::mutex> lock(TableMutex_);

        if (entry != nullptr) {
            //Update LocT PV, clause C.2
            uint32_t so_tst = ntohl(so_pv.tst);
            uint32_t locte_tst = ntohl(entry->LPV_.tst);
            if (((so_tst > locte_tst) && ((so_tst - locte_tst) <= UINT_MAX/2)) ||
                    ((locte_tst > so_tst) && ((locte_tst - so_tst) <= UINT_MAX/2))) {
                entry->LPV_ = so_pv;
            }
            entry->Updated_ = true;
        } else {
            entry = std::make_shared<LocTableEntry>(so_pv);
            TableEntries_.insert(std::pair<uint8_t *, std::shared_ptr<LocTableEntry>>(
                        const_cast<uint8_t *>(so_pv.gn_addr.mid), entry));
        }
        return entry;
    }

    /**
     * Update the Location Table, this is for processing GUC packet, only to be
     * called when "We" are forwarder instead of the destination of the GUC
     * packet.
     *
     * @param [in] so_pv source position vector.
     * @param [in] de_pv destination position vector.
     */
    const std::shared_ptr<LocTableEntry> LocationTable::Update(gn_spv_t &de_pv) {
        std::shared_ptr<LocTableEntry> entry = Find(de_pv.gn_addr);
        std::lock_guard<std::mutex> lock(TableMutex_);

        if (entry == nullptr) {
            entry = std::make_shared<LocTableEntry>(de_pv);
            TableEntries_.insert(std::pair<uint8_t *, std::shared_ptr<LocTableEntry>>(
                        de_pv.gn_addr.mid, entry));
        } else {
            entry->Updated_ = true;
            if (entry->isNeighbor_ == false) {
                // Update the LocTE position vector indicatd by DE_PV.
                entry->LPV_.latitude = de_pv.latitude;
                entry->LPV_.longitude = de_pv.longitude;
                entry->LPV_.tst = de_pv.tst;
            } else {
                // Update the Packet's DE PV.  ETSI EN 102 636-4-1 C.3
                uint32_t locte_tst = ntohl(entry->LPV_.tst);
                uint32_t de_tst = ntohl(de_pv.tst);
                if (((locte_tst > de_tst) && ((locte_tst - de_tst) <= UINT_MAX/2)) ||
                        ((de_tst > locte_tst) && ((de_tst - locte_tst) <= UINT_MAX/2))) {
                    de_pv.tst = entry->LPV_.tst;
                    de_pv.latitude = entry->LPV_.latitude;
                    de_pv.longitude = entry->LPV_.longitude;
                }
            }
        }
        return entry;
    }

    const std::shared_ptr<LocTableEntry> LocationTable::FindShortestLocTe(
            int32_t target_lat, int32_t target_long, int &shortest_dis ) {
        std::lock_guard<std::mutex> lock(TableMutex_);
        shortest_dis = INT_MAX;
        std::shared_ptr<LocTableEntry> LocTe = nullptr;
        for (auto i : TableEntries_) {
            auto e = i.second;
            if (e->isNeighbor() == true) {
                int32_t lpv_lat = static_cast<int32_t>(ntohl(e->LPV_.latitude));
                int32_t lpv_long = static_cast<int32_t>(ntohl(e->LPV_.longitude));
                int d = GeoNetUtils::GeoDistance(target_lat, target_long,
                    lpv_lat, lpv_long, GEO_POS_UNIT_TENTH_MICRO_DEGREE);
                if (d < shortest_dis) {
                    shortest_dis = d;
                    LocTe = e;
                }
            }
        }
        return LocTe;
    }
    void LocationTable::Remove(const uint8_t *mid) {
        std::lock_guard<std::mutex> lock(TableMutex_);
        auto it = TableEntries_.find(const_cast<uint8_t *>(mid));
        if (it != TableEntries_.end()) {
            TableEntries_.erase(it->first);
        }
    }
    void LocationTable::RefreshTask(void) {
        std::unique_lock<std::mutex> lk(TableMutex_);
        std::chrono::milliseconds TimerValue(10000);    //referesh every 10 seconds
        do {
            for (auto i : TableEntries_) {
                auto e = i.second;
                uint32_t ts_now = GeoNetUtils::GetTimestampSinceEpoch();
                uint32_t ts_e = ntohl(e->LPV_.tst);
                if ((ts_now - ts_e) > LifeTime_*1000) {
                    TableEntries_.erase(i.first);
                }
            }
            //wait_until will unlock the TableMutex_
            auto now = std::chrono::system_clock::now();
            auto status = Cv_.wait_until(lk, now + TimerValue);
            if (status == std::cv_status::timeout)
                continue;
            else
                break;
        } while(true);
    }

    void LocationTable::Dump(void) {
        // Print a heading
        cout << setw(32) << setfill(' ')<< "GN_ADDR|" << setw(12) << "TS";
        cout << setw(12) << setfill(' ')<< "LAT" << setw(12) << setfill(' ') << "LONG";
        cout << " PAI " << setw(6) << setfill(' ') << "SPEED" << " HEADING ";
        cout << " LS Pending IsNeighbor   PDR   UPDATED" << std::endl;
        for (auto i : TableEntries_) {
            auto e = i.second;
            cout << "M: " <<  e->LPV_.gn_addr.m << " ST: " << e->LPV_.gn_addr.st<< 4 << "MID ";
            for (int i = 0; i < GN_MID_LEN; i++)
                cout << setw(2) << setfill('0') << e->LPV_.gn_addr.mid[i] << ":";
            cout << setw(12) << setfill(' ') << e->LPV_.tst;
            cout << setw(12) << setfill(' ')<< e->LPV_.latitude << setw(12) << setfill(' ') <<
                e->LPV_.longitude;
            cout << setw(5) << setfill(' ') << e->LPV_.pai << setw(6) << setfill(' ') <<
                e->LPV_.s << setw(9) << setfill(' ') << e->LPV_.h;
            cout << setw(11) << setfill(' ') << e->LocationServicePending_ << setw(10) <<
                setfill(' ') << e->isNeighbor_ << setw(7)<<setfill(' ') << e->PDR_ << setw(7) <<
                setfill(' ') << e->Updated_ << endl;
        }
    }
}

