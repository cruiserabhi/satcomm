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
/**
 * @file LocationTable.hpp
 * @brief implementation of location table class, header.
 */
#include <map>
#include <cstring>
#include <deque>
#include <memory>
#include <future>
#include <mutex>
#include "GeoNetRouter.hpp"
#include "gn_internal.h"

#define MAX_DPL_LEN     20
namespace gn {
    /**
     * Private compare function for LocationTable entry map
     */
    struct AddrCompare : public std::binary_function<const uint8_t *, const uint8_t *, bool> {
        public:
            bool operator()(const uint8_t *addr1, const uint8_t *addr2) const {
                return std::memcmp(addr1, addr2, GN_MID_LEN) < 0;
            }
    };
    /**
     * Duplicated packet list(DLP) element.
     */
    typedef struct dpl_element {
        uint16_t sn;
        uint32_t counter;
    } dpl_element_t;

    class LocTableEntry {
    public:
        LocTableEntry(){}
        LocTableEntry(const gn_lpv_t &src, int stype = 0, int version = 1);
        LocTableEntry(const gn_spv_t &dst, int stype = 0, int version = 1);

        ~LocTableEntry();

        bool isDuplicated(uint16_t sn);
        bool isNeighbor(void) {
            return (true == isNeighbor_);
        }
        bool IsLSPending(void) {
            return (true == LocationServicePending_);
        }
        bool isUpdated(void) {
            return (true == Updated_);
        }
        int getPAI(void) {
            return LPV_.pai;
        }
        gn_lpv_t getLPV(void) {
            return LPV_;
        }
        void setNeighbor(bool n) {
            isNeighbor_ = n;
        }
        void SetLSPending(bool n) {
            LocationServicePending_ = n;
        }
        gn_addr_t GnAddr_;              //Address of the ITS-S

    private:
        int StationType_;               //Type of the ITS-S
        int Version_;                   //Protocol version of the ITS-S

        // NOTE: the LPV_ here is the raw received LPV, all integers are in
        // network byte order, we need to convert to host order before doing any
        // comparison/calculation.
        gn_lpv_t LPV_;
        bool LocationServicePending_;   //If Location Service is in progress.
        bool isNeighbor_;               //If router is in direct communication.
        //uint32_t LastTST_;                //Last received timestamp.
        double PDR_;                    //Packet data rate.
        bool Updated_;                  //If this is a newly created entry
        std::deque<std::shared_ptr<dpl_element_t>> DPL_; //duplicated packet list.

        friend class LocationTable;
    };

    class LocationTable {
    public:
        LocationTable() {
        }
        LocationTable(int32_t LifeTime, gn_addr_t LocalAddr) :
            LifeTime_(LifeTime),
            LocalAddr_(LocalAddr) {
        }
        ~LocationTable() {
            // Must call RefreshTaskStop() first before destruction.
        }

        void SetLifeTime(int32_t LifeTime) {
            LifeTime_ = LifeTime;
        }
        void SetLocalAddr(gn_addr_t LocalAddr) {
            LocalAddr_ = LocalAddr;
        }

        void RefreshTaskStart(void);
        void RefreshTaskStop(void);
        const std::shared_ptr<LocTableEntry> Find(const gn_addr_t &GnAddr);
        const std::shared_ptr<LocTableEntry> Find(const uint8_t *mid);
        const std::shared_ptr<LocTableEntry> Update(const gn_lpv_t &so_pv);
        const std::shared_ptr<LocTableEntry> Update(gn_spv_t &de_pv);
        const std::shared_ptr<LocTableEntry> FindShortestLocTe(int32_t target_lat,
                int32_t target_long, int &shortest_dis);
        void Remove(const uint8_t *mid);
        bool isDuplicated(const gn_addr_t &GnAddr, uint16_t sn);
        void Dump(void);

    private:
        void RefreshTask(void);
        std::mutex TableMutex_;
        std::condition_variable Cv_;
        //std::promise<int> RefreshTaskResult_;
        std::thread RefreshThread;
        std::map<uint8_t *, std::shared_ptr<LocTableEntry>, AddrCompare> TableEntries_;
        int LifeTime_;  //in seconds
        gn_addr_t LocalAddr_;
        //AsyncTaskQueue<void> taskQ_;
    };
}


