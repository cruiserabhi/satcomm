/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021,2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file GeoNetRouterImpl.cpp
 * @brief implementation of GeoNetwork router.
 */
#include <cmath>
#include <iomanip>
#include <arpa/inet.h>
#include "KinematicsReceive.h"
#include "GeoNetRouterImpl.hpp"

static uint32_t base2ms[] = {50, 1000, 10000, 100000};
static uint8_t ll_bc[] {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static const char *bh_nh_str[] = {
    "ANY", "COMMON", "SECURE PACKET"};
static const char *ch_nh_str[] = {
    "ANY", "BTP_A", "BTP_B", "IPV6"};
static const char *ch_ht_str[] = {
    "ANY", "BEACON", "GEO_UNICAST", "GEO_ANYCAST", "GEO_BROADCAST", "TSB","LS"};
static const char *ch_hst_str[] = {
    "CIRCLE", "RECT", "ELIP"};
static const char *tsb_hst_str[] = {
    "SINGLE_HOP", "MULTI_HOP"};
static const char *ls_hst_str[] = {
    "REQUEST", "REPLY"};

static void PrintMID(const uint8_t *addr) {
    std::ios_base::fmtflags f( std::cout.flags() );
    for (int i = 0 ; i < 6; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)addr[i]<< " ";
    }
    std::cout.flags( f );
}
namespace gn {
    GeoNetRouterImpl *GeoNetRouterImpl::pInstance = nullptr;

    GeoNetRouterImpl::GeoNetRouterImpl(std::shared_ptr<ILocationListener> locListener,
                                       const GnConfig_t &config) {
        SequenceNumber_ = 0;
        NeighborCount_ = 0;
        CBFstop_ = false;
        locListener_ = locListener;
        LogLevel_ = 0;
        Config_ = config;

        //InitDefaultConfig(Config_);
        if (Config_.itsGnLocalAddrConfMethod == LocalAddrConfigMethod::GN_LOCAL_ADDR_CONF_AUTO)
            itsGnLocalGnAddr_.m = 1;
        else
            itsGnLocalGnAddr_.m = 0;
        itsGnLocalGnAddr_.st = static_cast<int>(Config_.StationType);
        std::memcpy(itsGnLocalGnAddr_.mid, Config_.mid, GN_MID_LEN);
        LocTable_.SetLifeTime(Config_.itsGnLifetimeLocTE);
        LocTable_.SetLocalAddr(itsGnLocalGnAddr_);
        df_txcb = nullptr;
    }

    GeoNetRouterImpl* GeoNetRouterImpl::Instance(std::shared_ptr<ILocationListener> locListener,
                                                 const GnConfig_t &config) {
        GeoNetRouterImpl::pInstance = new GeoNetRouterImpl(locListener, config);
        return GeoNetRouterImpl::pInstance;
    }
    void GeoNetRouterImpl::InitDefaultConfig(GnConfig_t &cfg) {
        // Defalut Mac ID is not set
        cfg.itsGnLocalAddrConfMethod = LocalAddrConfigMethod::GN_LOCAL_ADDR_CONF_ANONYMOUS;
        cfg.itsGnProtocolVersion = 0;
        cfg.StationType = ITSStationType::StationType_passengerCar;
        cfg.itsGnIsMobile = true;
        cfg.itsGnIfType = InterfaceType::GN_IF_TYPE_ITS_G5;
        cfg.itsGnMinUpdateFrequencyEPV = 1000;
        cfg.itsGnPaiInterval = 80;
        cfg.itsGnMaxStuSize = 1398;
        cfg.itsGnMaxGeoNetworkingHeaderSize = 88;
        cfg.itsGnLifetimeLocTE = 20;   /* seconds */
        cfg.itsGnSecurity = SecurityConfig::GN_SECURITY_DISABLED;
        //cfg.itsGnSnDecapResultHandling = GN_DECAP_RESULT_STRICT;
        cfg.itsGnLocationServiceMaxRetrans = 10;
        cfg.itsGnLocationServiceRetransmitTimer = 1000;
        cfg.itsGnLocationServicePacketBufferSize = 1024;
        cfg.itsGnBeaconServiceRetransmitTimer = 3000;
        cfg.itsGnBeaconServiceMaxJitter= cfg.itsGnBeaconServiceRetransmitTimer/4;
        cfg.itsGnDefaultHopLimit = 10;
        cfg.itsGnDPLLength = 8;
        cfg.itsGnMaxPacketLifetime = 600;  /* seconds */
        cfg.itsGnDefaultPacketLifetime  = 60;
        cfg.itsGnMaxPacketDataRate = 100;  /* Ko/s ? */
        cfg.itsGnMaxPacketDataRateEmaBeta = 90;
        cfg.itsGnMaxGeoAreaSize = 10;      /* Km^2 */
        cfg.itsGnMinPacketRepetitionInterval = 100;    /* ms */
        cfg.itsGnNonAreaForwardingAlgorithm = NAF_Algorithm::GN_NAF_GREEDY;
        cfg.itsGnAreaForwardingAlgorithm = AF_Algorithm::GN_AF_CBF;
        cfg.itsGnCbfMinTime = 1;   /* ms */
        cfg.itsGnCbfMaxTime = 100;  /* ms */
        cfg.itsGnDefaultMaxCommunicationRange = 1000;  /* m */
        cfg.itsGnBroadcastCBFDefSectorAngle = 30;      /* degree */
        cfg.itsGnUcForwardingPacketBufferSize = 256;
        cfg.itsGnBcForwardingPacketBufferSize = 1024;
        cfg.itsGnCbfPacketBufferSize = 256;
        cfg.itsGnDefaultTrafficClass = 0;
    }
    void GeoNetRouterImpl::SetConfig(const GnConfig_t &config) {
        Config_ = config;
        if (config.itsGnLocalAddrConfMethod == LocalAddrConfigMethod::GN_LOCAL_ADDR_CONF_AUTO)
            itsGnLocalGnAddr_.m = 1;
        else
            itsGnLocalGnAddr_.m = 0;
        itsGnLocalGnAddr_.st = static_cast<int>(config.StationType);
        std::memcpy(itsGnLocalGnAddr_.mid, config.mid, GN_MID_LEN);
        LocTable_.SetLifeTime(config.itsGnLifetimeLocTE);
        LocTable_.SetLocalAddr(itsGnLocalGnAddr_);
    }

    /**
     * Start the GeoNetRouer state machine.
     */
    void GeoNetRouterImpl::Start(void) {
        auto t = [&](void) {
            this->CBFTimerTask();
        };
        CBFTimerThread_ = std::thread(t);
    }

    void GeoNetRouterImpl::Stop(void) {
        // Stop CBF timer task
        CBFstop_ = true;
        CBFcv_.notify_one();
        // Wait for CBF timer task to finish.
        if (CBFTimerThread_.joinable()) {
            CBFTimerThread_.join();
        }

        // stop and wait all location service tasks, if any.
        for (auto i : LsMap_) {
            auto e = i.second;
            e->Ecv.notify_one();
            e->AsyncResult.get_future().get();
        }
        // TODO: Stop and all queue flush tasks if any.
    }
    uint32_t GeoNetRouterImpl::DecodeLifeTime(uint8_t lt) {
        return (lt >> 2) * base2ms[lt & 0x3];
    }

    uint8_t GeoNetRouterImpl::EncodeLifeTime(uint32_t lt_in_ms) {
        uint8_t lt = 0;

        if (lt_in_ms < 1000) {
            lt = ((lt_in_ms / 50) & 0xFF) << 2;
        } else if (lt_in_ms > 1000 && lt_in_ms < 10000) {
            lt = (((lt_in_ms / 1000) & 0xFF)<< 2) | 1;
        } else if (lt_in_ms < 100000 && lt_in_ms > 10000) {
            lt_in_ms = (((lt_in_ms /10000) & 0xFF) << 2) | 2;
        } else {
            lt_in_ms = (((lt_in_ms /100000) & 0xFF) << 2) | 3;
        }
        return lt;
    }

    void GeoNetRouterImpl::FlushQueueSync(int Qid, const uint8_t *addr, bool purge) {
        std::lock_guard<std::mutex> lk(qMutex_);
        int TsNow = GeoNetUtils::GetTimestampSinceEpoch();

        if (Qid & UC_Q) {
            if (!addr)
                return;
            auto it = UcMapQueue_.find(const_cast<uint8_t *>(addr));
            if (it != UcMapQueue_.end()) {
                auto q = it->second;    //obtain the shared pointer of type QueueT
                while (q->size()) {
                    auto e = q->front();
                    gn_guc_hdr_t *h = reinterpret_cast<gn_guc_hdr_t *>(e->Buffer);
                    if ((TsNow - e->Ts) > DecodeLifeTime(h->bh.lt)) {
                        q->pop_front();
                        continue;
                    }
                    // call radio transmit callback function to send packet.
                    if (e->txcb_)
                        e->txcb_((char *)e->Buffer, (int16_t)e->BufLen);
                }
            }
        }
        if (Qid & LS_Q) {
            if (!addr)
                return; // we need to know which GN_ADDR we need to flush for.
            auto it = LsMapQueue_.find(const_cast<uint8_t *>(addr));
            if (it != LsMapQueue_.end()) {
                auto q = it->second;    //obtain the shared pointer of type QueueT
                while (q->size()) {
                    auto e = q->front();
                    gn_guc_hdr_t *h = reinterpret_cast<gn_guc_hdr_t *>(e->Buffer);
                    if ((purge == true) || ((TsNow - e->Ts) > DecodeLifeTime(h->bh.lt))) {
                        q->pop_front();
                        continue;
                    }
                    // call radio transmit callback function to send packet.
                    if (e->txcb_)
                        e->txcb_((char *)e->Buffer, (int16_t)e->BufLen);
                }
            }
        }
        if (Qid & BC_Q) {
            while (BcQueue_.size()) {
                auto e = BcQueue_.front();
                BcQueue_.pop_front();
                gn_gbc_gac_hdr_t *h = reinterpret_cast<gn_gbc_gac_hdr_t *>(e->Buffer);

                if ((TsNow - e->Ts) > DecodeLifeTime(h->bh.lt)) {
                    continue;
                }
                // call radio transmit callback function to send packet.
                if (e->txcb_)
                    e->txcb_((char *)e->Buffer, (int16_t)e->BufLen);
            }
        }
    }

    void GeoNetRouterImpl::FlushQueue(int Qid, const uint8_t *Addr, bool Purge) {
        int count = 0;
        // TODO :check queue empty before starting task
        if ((Qid & UC_Q) && (UcMapQueue_.empty() == false))
            count++;
        if ((Qid & BC_Q) && (BcQueue_.empty() == false))
            count++;
        if ((Qid & LS_Q) && (LsMapQueue_.empty() == false))
            count++;

        if (!count)
            return;

        //TODO: this need to be improved.
        aSyncFuture_ = std::async(std::launch::async, [this, Qid, Addr, Purge]() {
                this->FlushQueueSync(Qid, Addr, Purge); }).share();
    }

    void GeoNetRouterImpl::Enqueue(int Qid, const uint8_t *Buffer, size_t BufLen,
            txcb_t txcb, const uint8_t *addr) {
        std::lock_guard<std::mutex> lk(qMutex_);
        uint8_t* Buf = (uint8_t*)Buffer;
        if (Qid & UC_Q) {
            if (!addr) {
                std::cerr << "Request to queue packet in UC queue but address is not given" <<
                    std::endl;
                return;
            }
            auto it = UcMapQueue_.find(const_cast<uint8_t *>(addr));
            if (it == UcMapQueue_.end()) {
                QueueT q;
                q.push_back(std::make_shared<Qelement>(Buf, BufLen, txcb));
                // Add the queue to map.
                UcMapQueue_.insert(std::pair<uint8_t *, std::shared_ptr<QueueT>> (
                            const_cast<uint8_t *>(addr), std::make_shared<QueueT>(q)));
            } else {
                auto q = it->second;    // q is shared_ptr of QueueT
                if (q->size() == Config_.itsGnUcForwardingPacketBufferSize) {
                    std::cerr << "UC queue for addr: " << addr << "overrun!" << std::endl;
                    q->pop_front();
                }
                q->push_back(std::make_shared<Qelement>(Buf, BufLen, txcb));
            }
        } else if (Qid & LS_Q) {
            if (!addr) {
                std::cerr << "Request to queue packet into LS queue but address is not given" <<
                    std::endl;
                return;
            }
            auto it = LsMapQueue_.find(const_cast<uint8_t *>(addr));
            if (it == LsMapQueue_.end()) {
                QueueT q;
                q.push_back(std::make_shared<Qelement>(Buf, BufLen, txcb));
                // Add the queue to map.
                UcMapQueue_.insert(std::pair<uint8_t *, std::shared_ptr<QueueT>> (
                            const_cast<uint8_t *>(addr), std::make_shared<QueueT>(q)));
            } else {
                auto q = it->second;    // q is shared_ptr of QueueT
                if (q->size() == Config_.itsGnUcForwardingPacketBufferSize) {
                    std::cerr << "UC queue for addr: " << addr << "overrun!" << std::endl;
                    q->pop_front();
                }
                q->push_back(std::make_shared<Qelement>(Buf, BufLen, txcb));
            }
        } else if (Qid & BC_Q) {
            if (BcQueue_.size() == Config_.itsGnBcForwardingPacketBufferSize) {
                std::cerr << "BC Buffer overrun!" << std::endl;
                BcQueue_.pop_front();
            }
            BcQueue_.push_back(std::make_shared<Qelement>(Buf, BufLen, txcb));
        }
    }


    /**
     * Start Location service synchronously
     */
    void GeoNetRouterImpl::LocationServiceSync(const uint8_t *Addr) {
        int count = 0;
        GnData_t data;
        std::cv_status status;
        std::chrono::milliseconds TimerValue(Config_.itsGnLocationServiceRetransmitTimer);

        // Create a packet queue element to prepare for retransmit.
        size_t BufLen = sizeof(gn_lsreq_hdr_t) + 1; // account for 1 byte cv2x family ID
        auto e = std::make_shared<Qelement>(nullptr, BufLen, df_txcb);
        gn_lsreq_hdr_t *h = reinterpret_cast<gn_lsreq_hdr_t *>(e->Buffer + 1);

        // Initialize LS request packet.
        InitDefaultGnData(data);
        data.pkt_type = PacketType::GN_PACKET_TYPE_LS;
        data.hst = LS_HST_REQUEST;
        InitLSRequest(e->Buffer + 1, sizeof(gn_lsreq_hdr_t), data, Addr);
        *(e->Buffer) = 0x03;

        std::unique_lock<std::mutex> lk(e->EMutex);

        // Insert into map to track the progress, TODO: Lock for LsMap_
        LsMap_.insert(std::pair<uint8_t *, std::shared_ptr<Qelement>>(const_cast<uint8_t *>(Addr), e));

        do {
            count++;
            // Transmit this packet!
            if (df_txcb != nullptr) {
                df_txcb((char *)e->Buffer, (uint16_t)BufLen);
            }
            auto now = std::chrono::system_clock::now();
            status = e->Ecv.wait_until(lk, now + TimerValue);
        } while ((count < Config_.itsGnLocationServiceMaxRetrans) ||
                (status != std::cv_status::timeout));

        if (status == std::cv_status::timeout) {
            // We didn't receive reply, purge the LS packet queue.
            FlushQueue(LS_Q, Addr, true);
            LocTable_.Remove(Addr);
        }
        e->AsyncResult.set_value(0);
    }

    void GeoNetRouterImpl::LocationServiceStart(const uint8_t *Addr) {
        auto t = [&](const uint8_t *Addr) {
            this->LocationServiceSync(Addr);
        };
        LocationServiceThread_ = std::thread(t, Addr);
    }

    /**
     * CBF timer task
     */
    void GeoNetRouterImpl::CBFTimerTask(void) {
        std::unique_lock<std::mutex> lk(CBFmutex_);

        // if the CBF queue is not empty, set the timer interval to 1
        // millisecond. Otherwise, set it to 1 second.
        std::chrono::milliseconds TimerValueBusy(1);
        std::chrono::milliseconds TimerValueIdle(1000);

        do {

            int TsNow = GeoNetUtils::GetTimestampSinceEpoch();
            std::cv_status status;
            while(!CBFqueue_.empty()) {
                auto e = CBFqueue_.top();
                if ((TsNow - e->Ts) >= e->To) {
                    CBFqueue_.pop();
                    // TODO: Send the packet out;
                }

            }
            //wait_until will unlock the CBFmutex_
            auto now = std::chrono::system_clock::now();
            if (!CBFqueue_.empty())
                status = CBFcv_.wait_until(lk, now + TimerValueBusy);
            else
                status = CBFcv_.wait_until(lk, now + TimerValueIdle);

            if (status == std::cv_status::timeout)
                continue;
            else if (CBFstop_ == false)
                continue;
            else
                break;


        } while(true);
    }

    /**
     * Non-Area Forwarding - Greedy forwarding, used for GBC/GAC and GUC.
     *
     * @param [in] PktType the packet type being forwarded.
     * @param [in] buffer pointer to the packet buffer.
     * @param [in] next hop link-layer address.
     * @returns 0 indicate no forwarder is found, packet should be queued.
     *          1 indicate the LL address is returned packet can be forwarded.
     *          -1 indicate packet should be discarded.
     */
    int GeoNetRouterImpl::NAF_GF(PacketType PktType, const uint8_t *Buffer,
            uint8_t *NextAddr) {

        int32_t Latitude, Longitude;
        int MFR, RetVal = -1;
        gn_epv_t Epv;
        const gn_chdr_t *CommonHdr;
        bool NH_Found = false;

        // Obtain destination Geo position based on packet type.
        if (PktType == PacketType::GN_PACKET_TYPE_GEOUNICAST) {
            const gn_guc_hdr_t *h = reinterpret_cast<const gn_guc_hdr_t *>(Buffer);
            CommonHdr = reinterpret_cast<const gn_chdr_t *>(&h->ch);
            Latitude = h->de_pv.latitude;
            Longitude = h->de_pv.longitude;

        } else if (PktType == PacketType::GN_PACKET_TYPE_GEOANYCAST) {
            const gn_gbc_gac_hdr_t * h = reinterpret_cast<const gn_gbc_gac_hdr_t *>(Buffer);
            CommonHdr = reinterpret_cast<const gn_chdr_t *>(&h->ch);
            Latitude = h->gp_latitude;
            Longitude = h->gp_longitude;
        } else {
            std::cerr << "wrong forwarding algorithm" << std::endl;
            return -1;
        }
        // Read our position vector
        ReadEPV(Epv);

        // Calculate MFR
        MFR = GeoNetUtils::GeoDistance(Latitude, Longitude, Epv.latitude_epv, Epv.longitude_epv,
                GEO_POS_UNIT_TENTH_MICRO_DEGREE);

        // Go through the Location table, find all neighbors and calculate the
        // distances between neighbor and destination, pick the shortest
        // distance.
        int shortest_dis;
        std::shared_ptr<LocTableEntry> LocTe_next = LocTable_.FindShortestLocTe(Latitude, Longitude,
                shortest_dis);

        if (LocTe_next && (shortest_dis < MFR)) {
            // We found next hop.
            std::memcpy(NextAddr, LocTe_next->GnAddr_.mid, GN_MID_LEN);
            RetVal = 1;
        } else {
            if (TC_SCF(CommonHdr->tc)) {
                //TODO: Queue it in appropriete queue
                RetVal = 0;
            } else {
                //Use broadcast address.
                std::memcpy(NextAddr, ll_bc, GN_MID_LEN);
                RetVal = 1;
            }
        }
        return RetVal;
    }

    /**
     * Non-Area Forwarding - contention based forwarding algorithm.
     *
     * @param[in] PktType the type of the packet being forwarded.
     * @param[in] Buffer the input packet buffer.
     * @param[in/out] buffer to store the returned next hop link layer address.
     * @returns 0: indicate packet should be queued.
     *          1: indicate nh_ll_address is returned, packet can be forwarded.
     *          -1: indicate packet should be discarded.
     */
    int GeoNetRouterImpl::NAF_CBF(PacketType PktType, const uint8_t *Buffer, uint8_t *NextAddr) {
        int32_t Latitude, Longitude;
        int Timeout;
        std::shared_ptr<LocTableEntry> LocTe = nullptr;
        gn_epv_t Epv;
        const gn_chdr_t *CommonHdr;
        const gn_addr_t *SoAddr;

        if (PktType == PacketType::GN_PACKET_TYPE_GEOUNICAST) {
            const gn_guc_hdr_t *h = reinterpret_cast<const gn_guc_hdr_t *>(Buffer);
            CommonHdr = reinterpret_cast<const gn_chdr_t *>(&(h->ch));
            SoAddr = reinterpret_cast<const gn_addr_t *>(&(h->so_pv.gn_addr));
            Latitude = h->de_pv.latitude;
            Longitude = h->de_pv.longitude;
        } else if (PktType == PacketType::GN_PACKET_TYPE_GEOANYCAST ||
                PktType == PacketType::GN_PACKET_TYPE_GEOBROADCAST) {
            const gn_gbc_gac_hdr_t *h = reinterpret_cast<const gn_gbc_gac_hdr_t *>(Buffer);
            CommonHdr = reinterpret_cast<const gn_chdr_t *>(&(h->ch));
            SoAddr = reinterpret_cast<const gn_addr_t *>(&(h->so_pv.gn_addr));
            Latitude = h->gp_latitude;
            Longitude = h->gp_longitude;
        } else {
            std::cerr << "Wrong forwarding algorithm" << std::endl;
            return -1;
        }

        if (GeoNetUtils::GnAddrMatch(*SoAddr, itsGnLocalGnAddr_) == true) {
            // We are the source, broadcast it.
            std::memcpy(NextAddr, ll_bc, GN_MID_LEN);
            return 1;
        }

        //TODO: Examin the CBF queue, remove and stop it's timer if it found
        //this packet.

        // Calculate CBF Timeout
        LocTe = LocTable_.Find(*SoAddr);
        if (LocTe && LocTe->getPAI() == 1) {
            int progress, dle, dse;
            ReadEPV(Epv);

            // dle: distance between local router to destination.
            dle = GeoNetUtils::GeoDistance(Latitude, Longitude, Epv.latitude_epv, Epv.longitude_epv,
                    GEO_POS_UNIT_TENTH_MICRO_DEGREE);

            // dse: distance between sender and destination.
            gn_lpv_t lpv = LocTe->getLPV();
            dse = GeoNetUtils::GeoDistance(Latitude, Longitude, lpv.latitude, lpv.longitude,
                    GEO_POS_UNIT_TENTH_MICRO_DEGREE);

            // forwarding progress.
            progress = dse - dle;
            if (progress > 0) {
                // We (the router) is closer to the destination
                if (progress > Config_.itsGnDefaultMaxCommunicationRange) {
                    Timeout = Config_.itsGnCbfMinTime;
                } else {
                    Timeout = Config_.itsGnCbfMaxTime +
                        ((Config_.itsGnCbfMinTime - Config_.itsGnCbfMaxTime) * progress)/
                        Config_.itsGnDefaultMaxCommunicationRange;
                }
                // TODO: queue the packet in CBF queue and start timer.
                return 0;
            } else {
                return -1;
            }
        } else {
            Timeout = Config_.itsGnCbfMaxTime;
            // TODO: queue the packet in CBF queue and start timer.
            return 0;
        }
        return -1;
    }

    /**
     * Area forwarding - contention based forwarding algorithm.
     *
     * @note for GBC/GAC packets only.
     *
     * @param[in] Buffer the packet buffer.
     * @param[in] NextAddr next hop link-layer address.
     * @returns 0: indicates the packet is queued.
     *          1: indicates the next hop LL address is returned.
     *         -1: indicates packet should be discarded.
     */
    int GeoNetRouterImpl::AF_CBF(const uint8_t *Buffer, uint8_t *NextAddr) {
        std::shared_ptr<LocTableEntry> LocTe = nullptr;
        int Timeout;
        const gn_gbc_gac_hdr_t *h = reinterpret_cast<const gn_gbc_gac_hdr_t *>(Buffer);

        if (GeoNetUtils::GnAddrMatch(h->so_pv.gn_addr, itsGnLocalGnAddr_) == true) {
            std::memcpy(NextAddr, ll_bc, GN_MID_LEN);
            return 1;
        }

        // TODO: examain CBF queue, look for same packet, remove it if found.

        LocTe = LocTable_.Find(h->so_pv.gn_addr);
        if (LocTe && LocTe->getPAI() == 1) {
            gn_epv_t epv;
            gn_lpv_t lpv;
            ReadEPV(epv);
            lpv = LocTe->getLPV();
            int Dist = GeoNetUtils::GeoDistance(lpv.latitude, lpv.longitude,
                    epv.latitude_epv, epv.longitude_epv, GEO_POS_UNIT_TENTH_MICRO_DEGREE);
            if (Dist <= Config_.itsGnDefaultMaxCommunicationRange) {
                Timeout = Config_.itsGnCbfMaxTime +
                    ((Config_.itsGnCbfMinTime - Config_.itsGnCbfMaxTime) * Dist)/
                    Config_.itsGnDefaultMaxCommunicationRange;

            } else {
                Timeout = Config_.itsGnCbfMinTime;
            }
        } else {
            Timeout = Config_.itsGnCbfMinTime;
        }

        // TODO: queue packet in CBF queue and set timer.
        return 0;
    }

    /**
     * GBC/GAC fowarding algorithm selection.
     *
     * @param [in] Buffer packet buffer.
     * @param [in] NextAddr returned next hop link-layer address.
     * @returns
     */
    int GeoNetRouterImpl::ForwardAlgorithmSelect(const uint8_t *Buffer, uint8_t *NextAddr) {
        PacketType PktType;
        GeoAreaType AreaType;
        gn_epv_t Epv;
        int x, y, f, RetValue;

        const gn_gbc_gac_hdr_t *h = reinterpret_cast<const gn_gbc_gac_hdr_t *>(Buffer);

        if (h->ch.ht == GN_COMMON_HEADER_TYPE_GEOANYCAST)
            PktType = PacketType::GN_PACKET_TYPE_GEOANYCAST;
        else if (h->ch.ht == GN_COMMON_HEADER_TYPE_GEOBROADCAST)
            PktType = PacketType::GN_PACKET_TYPE_GEOBROADCAST;
        else {
            std::cerr << "can not handle packet type: " << h->ch.ht << std::endl;
            return -1;
        }
        ReadEPV(Epv);
        // Calculate local router's x-y coordinate using the center of
        // destination area as cartesian coordinate center.
        GeoNetUtils::CartesianTransform(Epv.latitude_epv, Epv.longitude_epv,
                h->gp_latitude, h->gp_longitude, h->angle,
                GEO_POS_UNIT_TENTH_MICRO_DEGREE, x, y);
        // Determine the relative position of local router with respect to target
        // area
        if (h->ch.hst == GBC_HST_CIRCLE)
            AreaType = GeoAreaType::GEO_AREA_TYPE_CIRCLE;
        else if (h->ch.hst == GBC_HST_RECT)
            AreaType = GeoAreaType::GEO_AREA_TYPE_RECT;
        else
            AreaType = GeoAreaType::GEO_AREA_TYPE_ELIP;

        f = GeoNetUtils::GeoRelativePosition(x, y, h->dist_a, h->dist_b, AreaType );

        if (f >= 0) {
            // We are inside the target area, Area Forwarding
            switch(Config_.itsGnAreaForwardingAlgorithm) {
                case gn::AF_Algorithm::GN_AF_SIMPLE:
                    memcpy(NextAddr, ll_bc, GN_MID_LEN);
                    RetValue = 1;
                    break;
                case gn::AF_Algorithm::GN_AF_CBF:
                    RetValue = AF_CBF(Buffer, NextAddr);
                default:
                    // Default is simple forwarding.
                    memcpy(NextAddr, ll_bc, GN_MID_LEN);
                    RetValue = 1;
            }
        } else {
            // We are outside of the target area, Non-Area forwarding
            std::shared_ptr<LocTableEntry> LocTe = LocTable_.Find(h->so_pv.gn_addr);
            if (LocTe) {
                // Determine the sender's relative position with respect to target
                // area.
                gn_lpv_t lpv = LocTe->getLPV();
                GeoNetUtils::CartesianTransform(lpv.latitude, lpv.longitude,
                        h->gp_latitude, h->gp_longitude, h->angle,
                        GEO_POS_UNIT_TENTH_MICRO_DEGREE, x, y);
                f = GeoNetUtils::GeoRelativePosition(x, y, h->dist_a, h->dist_b, AreaType);

                if ((LocTe->getPAI() == 1) && (f >= 0)) {
                    // The sender is located inside targeted area, we don't
                    // rebroadcast it.
                    RetValue = -1;
                } else {
                    switch(Config_.itsGnNonAreaForwardingAlgorithm) {
                        case gn::NAF_Algorithm::GN_NAF_GREEDY:
                            RetValue = NAF_GF(PktType, Buffer, NextAddr);
                            break;
                        case gn::NAF_Algorithm::GN_NAF_CBF:
                            RetValue = NAF_CBF(PktType, Buffer, NextAddr);
                        default:
                            RetValue = NAF_GF(PktType, Buffer, NextAddr);
                    }
                }
            } else {
                // We shouldn't be here since each rx packet must be processd
                // first and LocTE will be created for it.
                std::memcpy(NextAddr, ll_bc, GN_MID_LEN);
                RetValue = 1;
            }
        }
        return RetValue;
    }

    void GeoNetRouterImpl::ReadEPV(gn_epv_t &epv) {
        shared_ptr<ILocationInfoEx> locationInfo =
            std::dynamic_pointer_cast<LocListener>(locListener_)->getLocation();

        if (locationInfo) {
            // the epv lat/long is expressed in 1/10 micro-degree
            epv.latitude_epv = (locationInfo->getLatitude() * 10000000);
            epv.longitude_epv = (locationInfo->getLongitude() * 10000000);
            // speed is expressed in 0.01 meter/second
            epv.s_epv = (100 * locationInfo->getSpeed());
            // its expressed in 0.1 degree from north
            epv.h_epv = (locationInfo->getHeading() / 0.1);
        }

        epv.tst_epv = GeoNetUtils::GetTimestampSinceEpoch();

        //int SemiMajorAccu = (locationInfo->getHorizontalUncertaintySemiMajor() * 20);
        int SemiMajorAccu = 0;  //force to pai = 1
        if (SemiMajorAccu < (Config_.itsGnPaiInterval / 2))
            epv.pai_epv = 1;
        else
            epv.pai_epv = 0;
    }
    void GeoNetRouterImpl::DumpLPV(const gn_lpv_t &lpv) {
        std::cout << "LPV:" << std::endl;
        std::cout << "ADDR: M: " << (int)lpv.gn_addr.m;
        std::cout << " ST: " << (int)lpv.gn_addr.st << " MID: ";
        std::ios_base::fmtflags f( std::cout.flags() );
        for (int i = 0 ; i < GN_MID_LEN; i++) {
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)lpv.gn_addr.mid[i]
                << " ";
        }
        std::cout.flags( f );
        std::cout << std::endl;
        std::cout << "TST: " << (uint32_t)ntohl(lpv.tst) << std::endl;
        std::cout << "LAT: " << (int32_t)ntohl(lpv.latitude) << " LONG: " <<
            (int32_t)ntohl(lpv.longitude);
        std::cout << " SPEED: "<< (int)ntohs(lpv.s) << " HEADING: " << (int)ntohs(lpv.h) <<
            " PAI: " << (int)lpv.pai << std::endl;
    }

    int GeoNetRouterImpl::ReceiveBeacon_or_SHB(PacketType PktType, const uint8_t *Buffer,
            size_t BufLen, GnData_t &data) {
        const gn_lpv_t *so_pv;

        // Nobody should forward it, so we don't perform DPD
        if (PktType == PacketType::GN_PACKET_TYPE_BEACON) {
            const gn_beacon_hdr_t *h = reinterpret_cast<const gn_beacon_hdr_t *>(Buffer);
            so_pv = &(h->so_pv);
        } else {
            const gn_shb_hdr_t *h = reinterpret_cast<const gn_shb_hdr_t *>(Buffer);
            so_pv = &(h->so_pv);
            if (LogLevel_ > 2)
                DumpLPV(h->so_pv);
        }
        std::shared_ptr<LocTableEntry> LocTe = LocTable_.Update(*so_pv);
        if (LocTe) {
            LocTe->setNeighbor(true);
            NeighborCount_++;
        }
        if (PktType == PacketType::GN_PACKET_TYPE_SHB) {
            data.is_shb = true;
            return 0;
        }
        return 1;
    }

    int GeoNetRouterImpl::ReceiveGUC(uint8_t *Buffer, size_t BufLen, GnData_t &data) {
        uint8_t NextAddr[GN_MID_LEN];
        std::shared_ptr<LocTableEntry> LocTe;
        int fwd;
        gn_guc_hdr_t *h = reinterpret_cast<gn_guc_hdr_t *>(Buffer);

        if (GeoNetUtils::GnAddrMatch(h->de_pv.gn_addr, itsGnLocalGnAddr_) == true) {
            if (LocTable_.isDuplicated(h->so_pv.gn_addr, h->sn) == true) {
                //LOG(DEBUG, "Duplicate detected, drop packet");
                return -1;
            }
            LocTe =LocTable_.Update(h->so_pv);
            // TODO: Flush UC forward queue,and LS packet queue.
        } else {
            //Forwarder.
            if (Config_.itsGnNonAreaForwardingAlgorithm != NAF_Algorithm::GN_NAF_CBF) {
                if (LocTable_.isDuplicated(h->so_pv.gn_addr, h->sn) == true) {
                    return -1;
                }
            }
            LocTe = LocTable_.Update(h->so_pv);
            // TODO: Flush UC forward queue, and LS packet queue.
            if (h->bh.rhl == 0) {
                // No more hop.
                return -1;
            }

            if (NeighborCount_ == 0 && TC_SCF(h->ch.tc)) {
                // TODO: queue it to GUC foward queue.
                return 1;
            }
            if (Config_.itsGnNonAreaForwardingAlgorithm == NAF_Algorithm::GN_NAF_CBF) {
                fwd = NAF_CBF(PacketType::GN_PACKET_TYPE_GEOUNICAST, Buffer, NextAddr);
                // TODO: queue it to CBF queue.
            } else {
                fwd = NAF_GF(PacketType::GN_PACKET_TYPE_GEOUNICAST, Buffer, NextAddr);
                if (fwd == 0) {
                    // TODO: queue it to UC foward queue.
                } else if (fwd > 0) {
                    // TODO: send out.
                } else {
                    // Drop it.
                }
            }
        }
        return 1;
    }

    int GeoNetRouterImpl::ReceiveGBCGAC(uint8_t *Buffer, size_t BufLen, GnData_t &data) {
        int x, y, f;
        gn_epv_t Epv;
        GeoAreaType AreaType;
        bool exec_dpd = false;
        gn_gbc_gac_hdr_t *h = reinterpret_cast<gn_gbc_gac_hdr_t *>(Buffer);

        // Determine if we are in targeted area.
        ReadEPV(Epv);
        // Calculate local router's x-y coordinate using the center of
        // destination area as cartesian coordinate center.
        int32_t gp_lat = static_cast<int32_t>(ntohl(h->gp_latitude));
        int32_t gp_long = static_cast<int32_t>(ntohl(h->gp_longitude));
        uint16_t angle = ntohs(h->angle);
        uint16_t dist_a = ntohs(h->dist_a);
        uint16_t dist_b = ntohs(h->dist_b);
        GeoNetUtils::CartesianTransform(Epv.latitude_epv, Epv.longitude_epv,
                gp_lat, gp_long, angle,
                GEO_POS_UNIT_TENTH_MICRO_DEGREE, x, y);
        if (h->ch.hst == GBC_HST_CIRCLE)
            AreaType = GeoAreaType::GEO_AREA_TYPE_CIRCLE;
        else if (h->ch.hst == GBC_HST_RECT)
            AreaType = GeoAreaType::GEO_AREA_TYPE_RECT;
        else
            AreaType = GeoAreaType::GEO_AREA_TYPE_ELIP;

        f = GeoNetUtils::GeoRelativePosition(x, y, dist_a, dist_b, AreaType );

        if ((f < 0 && Config_.itsGnNonAreaForwardingAlgorithm != gn::NAF_Algorithm::GN_NAF_CBF) ||
            (f >= 0 && Config_.itsGnAreaForwardingAlgorithm == gn::AF_Algorithm::GN_AF_SIMPLE)) {
            exec_dpd = true;
        }
        std::shared_ptr<LocTableEntry> LocTe = LocTable_.Find(h->so_pv.gn_addr);
        if (LocTe) {
            if (LocTe->isDuplicated(h->sn) == true) {
                //LOG(DEBUG, "Received duplicated GBC/GAC packet");
                return -1;
            }
        }
        LocTe = LocTable_.Update(h->so_pv);
        FlushQueue(LS_Q|UC_Q, h->so_pv.gn_addr.mid);

        if (f >= 0) {
            // Allocate memory and duplicate this packet for forwarding, the
            // original buffer will be passed up to upper layer.
            std::unique_ptr<uint8_t[]> duppkt(new uint8_t(BufLen));
            std::memcpy(duppkt.get(), Buffer, BufLen);
            Buffer = duppkt.get();
        }

        // Execute forwarding first to avoid forwarding delay, use the default
        // radio transmit callback, if supplied.
        if (h->bh.rhl > 0) {
            if (NeighborCount_ == 0) {
                Enqueue(BC_Q, Buffer, BufLen, df_txcb);
            } else {
                int val;
                uint8_t NextAddr[GN_MID_LEN];
                val = ForwardAlgorithmSelect(Buffer, NextAddr);
                if (val > 0) {
                    //TODO: Send it out.
                } else if (val == 0) {
                    Enqueue(BC_Q, Buffer, BufLen, df_txcb);
                }
            }
        }
        if (f >= 0) {
            return 0;
        }
        return 1;
    }

    int GeoNetRouterImpl::ReceiveLocationService(gn_ls_hst_e hst, const uint8_t *Buffer,
            size_t BufLen, GnData_t &data) {
        int retVal = -1;
        std::shared_ptr<LocTableEntry> LocTe;
        if (hst == LS_HST_REQUEST) {
            const gn_lsreq_hdr_t *h = reinterpret_cast<const gn_lsreq_hdr_t *>(Buffer);

            if (GeoNetUtils::GnAddrMatch(h->req_addr, itsGnLocalGnAddr_) == true) {
                LocTe = LocTable_.Find(h->so_pv.gn_addr);
                if (LocTe) {
                    if (LocTe->isDuplicated(h->sn) == true)
                        return -1;
                }
                LocTe = LocTable_.Update(h->so_pv);
                // TODO: send reply asynchronously
                retVal = 1;
            }
        } else {
            const gn_lsreply_hdr_t *h = reinterpret_cast<const gn_lsreply_hdr_t *>(Buffer);
            if (GeoNetUtils::GnAddrMatch(h->de_pv.gn_addr, itsGnLocalGnAddr_) == true) {
                // Source operation
                LocTe = LocTable_.Find(h->so_pv.gn_addr);
                if (LocTe) {
                    if (LocTe->isDuplicated(h->sn) == true) {
                        return -1;
                    }
                }
                LocTe = LocTable_.Update(h->so_pv);
                LocTe->SetLSPending(false);
                // Notify location service thread we have the reply.
                auto iterator = LsMap_.find(const_cast<uint8_t *>(h->so_pv.gn_addr.mid));
                if (iterator != LsMap_.end()) {
                    auto e = iterator->second; //e is of type Qelement pointer.
                    e->Ecv.notify_all();

                    FlushQueue(LS_Q, h->so_pv.gn_addr.mid, false);
                }
                retVal = 1;
            } else {
                // Forward operation, pretend we received GUC
                retVal = ReceiveGUC(const_cast<uint8_t *>(Buffer), BufLen, data);
            }
        }
        return retVal;
    }

    int GeoNetRouterImpl::ReceiveTSB(uint8_t *Buffer, size_t BufLen, GnData_t &data) {
        std::shared_ptr<LocTableEntry> LocTe;
        const gn_tsb_hdr_t *h = reinterpret_cast<const gn_tsb_hdr_t *>(Buffer);

        LocTe = LocTable_.Find(h->so_pv.gn_addr);
        if (LocTe) {
            if (LocTe->isDuplicated(h->sn) == true) {
                return -1;
            }
        }
        LocTe = LocTable_.Update(h->so_pv);
        FlushQueue(UC_Q|LS_Q, h->so_pv.gn_addr.mid, false);

        if (h->bh.rhl > 0) {
            // Allocate memory and duplicate this packet for forwarding, the
            // original buffer will be passed up to upper layer.
            std::unique_ptr<uint8_t[]> duppkt(new uint8_t(BufLen));
            std::memcpy(duppkt.get(), Buffer, BufLen);
            Buffer = duppkt.get();

            if(NeighborCount_ == 0 && TC_SCF(h->ch.tc)) {
                Enqueue(BC_Q, Buffer, BufLen, df_txcb);
            } else {
                // TODO: Send it out (Buffer)
            }
        }
        return 0;   // 0 means PDU is passed to upper layer.
    }


    int GeoNetRouterImpl::Receive(uint8_t *Buffer, size_t BufLen, GnData_t &data) {
        int retVal = -1;

        if (Buffer == NULL) {
            std::cerr <<"invlaid input" << std::endl;
            return -1;
        }
        for(int i=0; i < 10; ++i)
            fprintf(stdout, "%02x ", Buffer[i]);
        gn_chdr_t *ch;
        gn_bhdr_t *bh = reinterpret_cast<gn_bhdr_t *>(Buffer);

        //Basic header processing
        if (bh->version > 1) {
            std::cerr << "Unsupported protocol version: " << std::endl;
            return -1;
        }
        if (bh->nh > BASIC_NH_TYPE_SECURED_PACKET ||
                bh->nh <= BASIC_NH_TYPE_ANY) {
            std::cerr << "Unknown Next Header " << (int)bh->nh << " in BH" << std::endl;
            return -1;
        }
        if (LogLevel_ > 2) {
            std::cout << "GeoNetwork Header:" << std::endl;
            std::cout << "BH: Ver: " << static_cast<int>(bh->version) << std::endl;
            std::cout << "    RHL: " << static_cast<int>(bh->rhl) << std::endl;
            std::cout << "    NH:  " << bh_nh_str[static_cast<int>(bh->nh)] << std::endl;
        }
        if (bh->rhl > 0) {
            bh->rhl--;
        }

        // Common header processing
        ch = reinterpret_cast<gn_chdr_t *>(Buffer + sizeof(gn_bhdr_t));
        if (ch->nh > GN_COMMON_NH_TYPE_IPV6 ||
                ch->nh <= GN_COMMON_NH_TYPE_ANY) {
            std::cerr << "Unknown Next Header in CH " << static_cast<int>(ch->nh);
            return -1;
        } else if (ch->ht > GN_COMMON_HEADER_TYPE_LS ||
                ch->ht <= GN_COMMON_HEADER_TYPE_ANY) {
            std::cerr << "Unknown Header Type in CH " << static_cast<int>(ch->ht);
            return -1;
        } else {
            if (ch->ht == GN_COMMON_HEADER_TYPE_GEOUNICAST ||
                    ch->ht == GN_COMMON_HEADER_TYPE_GEOANYCAST) {
                if (ch->hst > GBC_HST_ELIP || ch->hst < 0) {
                    std::cerr << "Unkown HST in CH " << static_cast<int>(ch->hst);
                    return -1;
                }
            }
            if (ch->ht == GN_COMMON_HEADER_TYPE_LS ||
                    ch->ht == GN_COMMON_HEADER_TYPE_TSB) {
                if (ch->hst > TSB_HST_MULTI_HOP ||
                        ch->hst < 0) {
                    std::cerr << "Unkown HST in CH " << static_cast<int>(ch->hst);
                    return -1;
                }
            }
        }
        if (ch->mhl < bh->rhl) {
            if (LogLevel_ > 0) {
                std::cout << "Hop limit reached, drop packet" << std::endl;
            }
            return -1;
        }
        InitDefaultGnData(data);
        data.upper_prot = static_cast<UpperProtocol>(ch->nh);
        data.pkt_type = static_cast<PacketType>(ch->ht);
        data.payload_len = static_cast<int>(ntohs(ch->pl));
        data.tc = static_cast<int>(ch->tc);

        if (LogLevel_ > 2) {
            std::cout << "CH: NH: " << ch_nh_str[static_cast<int>(ch->nh)] << std::endl;
            std::cout << "    HT: " << ch_ht_str[static_cast<int>(ch->ht)] << std::endl;;
            if (ch->ht == GN_COMMON_HEADER_TYPE_TSB)
                std::cout << "    HST:  " << tsb_hst_str[static_cast<int>(ch->hst)] << std::endl;
            else if (ch->ht == GN_COMMON_HEADER_TYPE_LS)
                std::cout << "    HST:  " << ls_hst_str[static_cast<int>(ch->hst)] << std::endl;
            else
                std::cout << "    HST:  " << ch_hst_str[static_cast<int>(ch->hst)] << std::endl;
            std::cout << "    TC: " << static_cast<int>(ch->tc) << std::endl;
            std::cout << "    FLAGS: " << static_cast<int>(ch->flags) << std::endl;
            std::cout << "    PL: " << static_cast<int>(data.payload_len) << std::endl;;
            std::cout << "   MHL: " << static_cast<int>(ch->mhl) << std::endl;
        }

        // We received a packet, that means we have neighbor, those packets queued
        // in BC forward queue can be sent now.
        FlushQueue(BC_Q);

        switch(ch->ht) {
            case GN_COMMON_HEADER_TYPE_ANY:
                std::cerr <<"Received unspecified GN packet" << std::endl;
                return -1;
            case GN_COMMON_HEADER_TYPE_BEACON:
                retVal = ReceiveBeacon_or_SHB(PacketType::GN_PACKET_TYPE_BEACON, Buffer, BufLen,
                        data);
                break;
            case GN_COMMON_HEADER_TYPE_GEOUNICAST:
                retVal = ReceiveGUC(Buffer, BufLen, data);
                break;
            case GN_COMMON_HEADER_TYPE_GEOANYCAST:
            case GN_COMMON_HEADER_TYPE_GEOBROADCAST:
                retVal = ReceiveGBCGAC(Buffer, BufLen, data);
                break;
            case GN_COMMON_HEADER_TYPE_TSB:
                if (ch->hst == TSB_HST_SINGLE_HOP)
                    retVal = ReceiveBeacon_or_SHB(PacketType::GN_PACKET_TYPE_SHB, Buffer, BufLen,
                            data);
                else
                    retVal = ReceiveTSB(Buffer, BufLen, data);
                break;
            case GN_COMMON_HEADER_TYPE_LS:
                retVal = ReceiveLocationService(static_cast<gn_ls_hst_e>(ch->hst), Buffer, BufLen,
                        data);
                break;
            default:
                std::cerr << "Header Type: " << ch->ht << " is invalid" << std::endl;
        }
        return retVal;
    }

    void GeoNetRouterImpl::InitDefaultGnData(GnData_t &data) {
        data.upper_prot = gn::UpperProtocol::GN_UPPER_PROTO_ANY;
        data.pkt_type = gn::PacketType::GN_PACKET_TYPE_ANY;
        data.payload_len = 0;
        data.d_type = gn::DestinationType::DE_TYPE_GN_ADDR;
        std::memcpy(data.d_addr, ll_bc, GN_MID_LEN);
        data.max_lt = Config_.itsGnMaxPacketLifetime;
        data.repetition_interval = Config_.itsGnMinPacketRepetitionInterval;
        //data.max_repetition_interval = 0;
        data.max_hl = -1;
        data.tc = -1;
    }
    /**
     * Initialize Basic and Common header
     */
    int GeoNetRouterImpl::InitBH_and_CH(uint8_t *Buffer, const GnData_t &data) {
        gn_bhdr_t *bh = reinterpret_cast<gn_bhdr_t *>(Buffer);
        gn_chdr_t *ch = reinterpret_cast<gn_chdr_t *>(Buffer + sizeof(gn_bhdr_t));

        // start with common header first.
        // For beacon, the data.upper_proto should be initialized with 0.
        ch->nh = static_cast<int>(data.upper_prot) & 0xF;
        ch->reserved = 0;
        ch->ht = static_cast<int>(data.pkt_type) & 0xF;

        // Header subtype.
        switch(data.pkt_type) {
            case PacketType::GN_PACKET_TYPE_ANY:
            case PacketType::GN_PACKET_TYPE_BEACON:
            case PacketType::GN_PACKET_TYPE_GEOUNICAST:
                ch->hst = 0;
            case PacketType::GN_PCCKET_TYPE_TSB:
                ch->hst = data.is_shb == true ? 0:1;
                break;
            case PacketType::GN_PACKET_TYPE_GEOANYCAST:
            case PacketType::GN_PACKET_TYPE_GEOBROADCAST:
                ch->hst = static_cast<int>(data.d_area.area_type) & 0xF;
                break;
            case PacketType::GN_PACKET_TYPE_LS:
                ch->hst = 0; // TODO location service request?
                break;
            default:
                return -1;
        }
        if (data.max_hl > 0) {
            ch->mhl = data.max_hl & 0xFF;
        } else {
            ch->mhl = 1;    // simple, limit to one hop
        }
        ch->tc = data.tc & 0xFF;
        ch->pl = htons(data.payload_len);
        ch->flags = (Config_.itsGnIsMobile == true) ? 1 << 7 : 0;
        ch->res = 0;

        // Basic header.
        bh->version = Config_.itsGnProtocolVersion & 0xF;
        bh->nh = BASIC_NH_TYPE_COMMON_HDR  & 0xF;
        bh->reserved = 0;
        if (data.max_lt < 0) {
            bh->lt = EncodeLifeTime(Config_.itsGnDefaultPacketLifetime);
        } else {
            bh->lt = EncodeLifeTime(data.max_lt);
        }
        bh->rhl = ch->mhl;

        return 0;
    }
    void GeoNetRouterImpl::InitSourceLPV(gn_lpv_t *lpv) {
        gn_epv_t epv;
        memcpy(&lpv->gn_addr, &itsGnLocalGnAddr_, sizeof(gn_addr_t));
        ReadEPV(epv);
        lpv->tst = htonl(epv.tst_epv);
        lpv->latitude = static_cast<int32_t>(htonl(epv.latitude_epv));
        lpv->longitude = static_cast<int32_t>(htonl(epv.longitude_epv));
        lpv->s = htons(epv.s_epv);
        lpv->h = htons(epv.h_epv);
        lpv->pai = epv.pai_epv;
    }

    void GeoNetRouterImpl::InitDestinationArea(gn_gbc_gac_hdr_t *h, const GnData_t &data) {
        h->gp_latitude = static_cast<int32_t>(htonl(data.d_area.gp_latitude));
        h->gp_longitude = static_cast<int32_t>(htonl(data.d_area.gp_longitude));
        h->dist_a = htons(data.d_area.dist_a);
        h->dist_b = htons(data.d_area.dist_b);
        h->angle = htons(data.d_area.angle);
    }
    int GeoNetRouterImpl::InitLSRequest(uint8_t *Buffer, size_t BufLen, const GnData_t &data,
            const uint8_t *Addr) {
        gn_epv_t Epv;
        if (InitBH_and_CH(Buffer, data) < 0) {
            return -1;
        }
        gn_lsreq_hdr_t *h = reinterpret_cast<gn_lsreq_hdr_t *>(Buffer);
        h->sn = GetNextSN();
        h->reserved = 0;
        InitSourceLPV(&h->so_pv);
        std::memcpy(&h->req_addr, Addr, sizeof(gn_addr_t));
        return 1;
    }

    /****************************************************************************
     * The transmit functions start from here.
     ****************************************************************************/
    int GeoNetRouterImpl::TransmitGUC(uint8_t *Buffer, size_t BufLen, const GnData_t &data,
            txcb_t txcb) {
        int RetValue = 0;
        gn_guc_hdr_t *h;
        std::shared_ptr<LocTableEntry> LocTe;
        uint8_t NextAddr[GN_MID_LEN];

        if (RetValue = InitBH_and_CH(Buffer + 1, data) < 0) {
            return RetValue;
        }
        h = reinterpret_cast<gn_guc_hdr_t *>(Buffer + 1);
        h->sn = GetNextSN();
        h->reserved = 0;
        InitSourceLPV(&h->so_pv);

        if ((LocTe = LocTable_.Find(data.d_addr)) == nullptr) {
            Enqueue(LS_Q, Buffer, BufLen, txcb, data.d_addr);
            LocationServiceStart(data.d_addr);
            RetValue = 1;
        } else if (LocTe->isNeighbor() == true) {
            // DE is our direct neighbor, we can just send to it.
        } else if (NeighborCount_ == 0) {
            Enqueue(UC_Q, Buffer, BufLen, txcb, data.d_addr);
            RetValue = 1;
        } else {
            if (Config_.itsGnNonAreaForwardingAlgorithm == gn::NAF_Algorithm::GN_NAF_UNSPECIFIED ||
                    Config_.itsGnNonAreaForwardingAlgorithm == gn::NAF_Algorithm::GN_NAF_GREEDY)
                RetValue = NAF_GF(PacketType::GN_PACKET_TYPE_GEOUNICAST, Buffer + 1, NextAddr);
            else
                RetValue = NAF_CBF(PacketType::GN_PACKET_TYPE_GEOUNICAST, Buffer + 1, NextAddr);

            if (RetValue == 1) {
                //TODO: Send it out.
            }
        }

        return RetValue;
    }

    int GeoNetRouterImpl::TransmitGBCGAC(uint8_t *Buffer, size_t BufLen, const GnData_t &data,
            txcb_t txcb) {
        int RetValue;
        gn_gbc_gac_hdr_t *h;
        LocTableEntry LocTe;
        uint8_t NextAddr[GN_MID_LEN];

        if (data.d_type != DestinationType::DE_TYPE_GEO_AREA)
            return -1;
        if (InitBH_and_CH(Buffer + 1, data) < 0)
            return -1;

        h = reinterpret_cast<gn_gbc_gac_hdr_t *>(Buffer + 1);
        h->sn = GetNextSN();
        h->res1 = h->res2 = 0;
        InitSourceLPV(&h->so_pv);
        InitDestinationArea(h, data);

        if (NeighborCount_ ==  0 && TC_SCF(data.tc)) {
            Enqueue(BC_Q, Buffer, BufLen, txcb);
            RetValue = 0;
        } else {
            RetValue = ForwardAlgorithmSelect(Buffer + 1, NextAddr);
            if (LogLevel_ > 2) {
                std::cout << "ForwardAlgorithmSelect returned " << RetValue << std::endl;
            }
            if (RetValue == 1 && txcb) {
                txcb(reinterpret_cast<char *>(Buffer),
                        static_cast<int16_t>(data.payload_len + sizeof(gn_gbc_gac_hdr_t)));
                RetValue = 0;
            } else if (RetValue == 0) {
                // TODO: queue it
            }
        }

        return RetValue;
    }

    int GeoNetRouterImpl::TransmitTSB(uint8_t *Buffer, size_t BufLen, const GnData_t &data,
            txcb_t txcb) {
        int retValue = -1;
        gn_tsb_hdr_t *h;
        uint8_t NextAddr[GN_MID_LEN]; //broadcast address.

        if (InitBH_and_CH(Buffer + 1, data) < 0) {
            return -1;
        }
        h = reinterpret_cast<gn_tsb_hdr_t *>(Buffer + 1);
        h->sn = GetNextSN();
        h->reserved = 0;
        InitSourceLPV(&h->so_pv);

        if (NeighborCount_ == 0 && TC_SCF(data.tc)) {
            Enqueue(BC_Q, Buffer, BufLen, txcb);
            retValue = 1;
        } else if (txcb){
            txcb(reinterpret_cast<char *>(Buffer), (uint16_t)BufLen);
            retValue = 0;
        }
        return retValue;
    }

    int GeoNetRouterImpl::TransmitSHB(uint8_t *Buffer, size_t BufLen, const GnData_t &data,
            txcb_t txcb) {
        gn_shb_hdr_t *h;
        uint8_t NextAddr[GN_MID_LEN]; //broadcast address.
        if (LogLevel_ > 2) {
            cout << "Transmit SHB: "<<BufLen << " bytes, neighbors=" << NeighborCount_<< std::endl;
        }

        // Need to skip cv2x family ID byte before init GN header.
        if (InitBH_and_CH(Buffer + 1, data) < 0) {
            std::cerr << "Failed to init BH and CH " << std::endl;
            return -1;
        }
        h = reinterpret_cast<gn_shb_hdr_t *>(Buffer + 1);
        InitSourceLPV(&h->so_pv);
        h->mdd = 0;

        if (NeighborCount_ == 0 && TC_SCF(data.tc)) {
            if (LogLevel_ > 2) {
                std::cout << "no neighbor present, packet is queued" << std::endl;
            }
            Enqueue(BC_Q, Buffer, BufLen, txcb);
            return 1;
        } else if (txcb) {
            std::cout << "GeoNetRouterImpl::TransmitSHB: send " << BufLen << " bytes" << std::endl;
            txcb(reinterpret_cast<char *>(Buffer), (uint16_t)BufLen);
            //txcb(reinterpret_cast<char *>(buffer), (uint16_t)sizeof(buffer));
            return 0;
        }
        return -1;
    }

    int GeoNetRouterImpl::Transmit(std::shared_ptr<msg_contents> mc, size_t BufLen,
            const GnData_t &data, txcb_t txcb) {
        int retVal = -1;

        uint8_t *p;
        // Insert one byte Cv2x family ID (0x03) in front of GeoNetwork header.
        switch(data.pkt_type) {
            case PacketType::GN_PACKET_TYPE_GEOUNICAST:
                if (mc->abuf.data - mc->abuf.head < sizeof(gn_guc_hdr_t) + 1) {
                    std::cerr << "GeoNetRouterImpl::Transmit: No enough headroom" << std::endl;
                    return retVal;
                }
                p = (uint8_t *)abuf_push(&mc->abuf, sizeof(gn_guc_hdr_t) + 1);
                BufLen += sizeof(gn_guc_hdr_t) + 1;
                if (!p) {
                   std::cerr << "GN_PACKET_TYPE_GEOUNICAST req is null" << std::endl;
                   return retVal;
                }
                *p = 0x03;
                retVal = TransmitGUC(p, BufLen, data, txcb);
                break;
            case PacketType::GN_PACKET_TYPE_GEOANYCAST:
            case PacketType::GN_PACKET_TYPE_GEOBROADCAST:
                if (mc->abuf.data - mc->abuf.head < sizeof(gn_gbc_gac_hdr_t) + 1) {
                    std::cerr << "GeoNetRouterImpl::Transmit: No enough headroom" << std::endl;
                    return retVal;
                }
                p = (uint8_t *)abuf_push(&mc->abuf, sizeof(gn_gbc_gac_hdr_t) + 1);
                BufLen += sizeof(gn_gbc_gac_hdr_t) + 1;
                if (!p) {
                   std::cerr << "GN_PACKET_TYPE_GEOANYCAST| GN_PACKET_TYPE_GEOBROADCAST \
                       req is null" << std::endl;
                   return retVal;
                }
                *p = 0x03;
                retVal = TransmitGBCGAC(p, BufLen, data, txcb);
            case PacketType::GN_PCCKET_TYPE_TSB:
                if (data.is_shb == false) {
                    if (mc->abuf.data - mc->abuf.head < sizeof(gn_tsb_hdr_t) + 1) {
                        std::cerr << "GeoNetRouterImpl::Transmit: No enough headroom" << std::endl;
                        return retVal;
                    }
                    p = (uint8_t *)abuf_push(&mc->abuf, sizeof(gn_tsb_hdr_t) + 1);
                    BufLen += sizeof(gn_tsb_hdr_t) + 1;
                    if (!p) {
                       std::cerr << "GN_PCCKET_TYPE_TSB req is null" << std::endl;
                       return retVal;
                    }
                    *p = 0x03;
                    retVal = TransmitTSB(p, BufLen, data, txcb);
                } else {
                    if (mc->abuf.data - mc->abuf.head < sizeof(gn_shb_hdr_t) + 1) {
                        std::cerr << "GeoNetRouterImpl::Transmit: No enough headroom" << std::endl;
                        return retVal;
                    }
                    p = (uint8_t *)abuf_push(&mc->abuf, sizeof(gn_shb_hdr_t) + 1);
                    BufLen += sizeof(gn_shb_hdr_t) + 1;
                    if (!p) {
                       std::cerr << "GN_PCCKET_TYPE_TSB req data is null" << std::endl;
                       return retVal;
                    }
                    *p = 0x03;
                    retVal = TransmitSHB(p, BufLen, data, txcb);
                }
                break;
            default:
                std::cerr << "unknown packet type to send" << std::endl;
                break;
        }

        return retVal;
    }

} // namespace gn
