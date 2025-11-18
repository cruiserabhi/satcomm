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
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file GeoNetRouterImpl.hpp
 * @brief implementation of GeoNetwork router, header.
 */
#ifndef GEONETROUTER_IMPL_HPP__
#define GEONETROUTER_IMPL_HPP_

#include <cstddef>
#include <memory>
#include <queue>
#include <deque>
#include <chrono>
#include "v2x_msg.h"
#include "GeoNetRouter.hpp"
#include "GeoNetUtils.hpp"
#include "LocationTable.hpp"

// bitmask to identify which queue/s to flush.
#define LS_Q            (1)
#define UC_Q            (1 << 2)
#define BC_Q            (1 << 3)

// radio transmit callback function.
typedef std::function<void(const char *, uint16_t)> txcb_t;

namespace gn {

/**
 * Queue element class used in various queue/map strucuture.
 * Note the member "To"(Timeout) is only used for CBF queue.
 */
class Qelement {
public:
    Qelement() {}
    Qelement(uint8_t *p, size_t BufLen, txcb_t txcb) : BufLen(BufLen),
        txcb_(txcb),
        To(0),
        Counter(0) {
        Buffer = new uint8_t[BufLen];
        if (Buffer && p) {
            memcpy(Buffer, p, BufLen);
        }
        Ts = gn::GeoNetUtils::GetTimestampSinceEpoch();
    }
    Qelement(uint8_t *p, size_t BufLen, txcb_t txcb, int to) : BufLen(BufLen),
        txcb_(txcb),
        To(to),
        Counter(0) {
        Buffer = new uint8_t[BufLen];
        if (Buffer && p) {
            memcpy(Buffer, p, BufLen);
        }
        Ts = gn::GeoNetUtils::GetTimestampSinceEpoch();
    }
    ~Qelement() {
        if (Buffer) {
            delete[] Buffer;
            Buffer = nullptr;
        }
    }

    int To;
    int Counter;
    int Ts;
    std::mutex EMutex;
    std::condition_variable Ecv;
    std::promise<int> AsyncResult;
    uint8_t* Buffer = nullptr;
    size_t BufLen;

    txcb_t txcb_;
};

// Compare function used by CBF queue(implemented by priority_queue), the top
// element will have smallest timeout value(To)
struct CompareTo {
    bool operator()(std::shared_ptr<Qelement> const& e1, std::shared_ptr<Qelement> const& e2) {
        return (e1->To > e2->To);
    }
};

typedef std::deque<std::shared_ptr<Qelement>> QueueT;

class GeoNetRouterImpl {
private:
    GeoNetRouterImpl(std::shared_ptr<ILocationListener> locListener, const GnConfig_t &config);
    static GeoNetRouterImpl *pInstance;

public:
    static GeoNetRouterImpl* Instance(std::shared_ptr<ILocationListener> locListener,
            const GnConfig_t &config);
    static void InitDefaultConfig(GnConfig_t &config);
    void InitDefaultGnData(GnData_t &data);
    void SetConfig(const GnConfig_t &config);
    void SetDefaultTransmitCb(txcb_t txcb) {
        df_txcb = txcb;
    }

    /**
     * Process a received packet in GeoNetwork router
     * @param [in] Buffer, buffer contains received raw data.
     * @param [in] BufLen, length of the received data.
     * @param [in/out] data reference to GeoNetwork data.
     *
     * @returns:
     * 0: packet should be passed to upper layer, the GeoNetwork related information
     *  is extracted into data reference.
     * 1: packet is forwarded.
     *-1: packet should be discarded by upper layer.
     */
    int Receive(uint8_t *Buffer, size_t BufLen, GnData_t &data);
    /**
     * Route the packet in GeoNetwork router, insert the header and send it out.
     *
     * @param[in] Buffer buffer contains the packet data.
     * @param[in] BufLen length of the packet.
     * @param[in] data data contains GeoNetwork related information for routing.
     *
     * @returns:
     * 0: The packet is sent immediately.
     * 1: The packet is queued in GeoNetwork router, will be sent in later time.
     *-1: The packet can not be sent or queued.
     *
     * NOTE: The input "Buffer" should have enough "headroom" for router to
     * insert header,
     */
    int Transmit(std::shared_ptr<msg_contents> mc, size_t BufLen, const GnData_t &data,
            txcb_t transmit_cb);

    void Start(void);
    void Stop(void);
    void SetLogLevel(int level) {
        LogLevel_ = level;
    }

private:
    uint16_t GetNextSN() {
        return SequenceNumber_++;
    }
    // state machine functions
    uint32_t DecodeLifeTime(uint8_t lt_field);
    uint8_t EncodeLifeTime(uint32_t lt_in_ms);
    void FlushQueueSync(int Qid, const uint8_t *addr, bool purge);
    void FlushQueue(int Qid, const uint8_t *Addr = nullptr, bool Purge = false);
    void Enqueue(int Qid, const uint8_t *Buffer, size_t BufLen, txcb_t txcb,
            const uint8_t *Addr = nullptr);
    void LocationServiceSync(const uint8_t *Addr);
    void LocationServiceStart(const uint8_t *Addr);
    void CBFTimerTask(void);
    void ReadEPV(gn_epv_t &epv);
    void DumpLPV(const gn_lpv_t &lpv);

    // Forwarding algorithms
    int NAF_GF(PacketType PktType, const uint8_t *Buffer, uint8_t *NextAddr);
    int NAF_CBF(PacketType PktType, const uint8_t *Buffer, uint8_t *NextAddr);
    int AF_CBF(const uint8_t *Buffer, uint8_t *NextAddr);
    int ForwardAlgorithmSelect(const uint8_t *Buffer, uint8_t *NextAddr);

    // Rx functions
    int ReceiveBeacon_or_SHB(PacketType PktType, const uint8_t *Buffer, size_t BufLen,
            GnData_t &data);
    int ReceiveGUC(uint8_t *Buffer, size_t BufLen, GnData_t &data);
    int ReceiveGBCGAC(uint8_t *Buffer, size_t BufLen, GnData_t &data);
    int ReceiveTSB(uint8_t *Buffer, size_t BufLen, GnData_t &data);
    int ReceiveLocationService(gn_ls_hst_e hst, const uint8_t *Buffer, size_t BufLen,
            GnData_t &data);

    // Tx functions
    void InitSourceLPV(gn_lpv_t *lpv);
    void InitDestinationArea(gn_gbc_gac_hdr_t *h, const GnData_t &data);
    int InitBH_and_CH(uint8_t *Buffer, const GnData_t &data);
    int InitLSRequest(uint8_t *Buffer, size_t BufLen, const GnData_t &data, const uint8_t *Addr);
    int TransmitGUC(uint8_t *Buffer, size_t BufLen, const GnData_t &data, txcb_t txcb);
    int TransmitGBCGAC(uint8_t *Buffer, size_t BufLen, const GnData_t &data, txcb_t txcb);
    int TransmitTSB(uint8_t *Buffer, size_t BufLen, const GnData_t &data, txcb_t txcb);
    int TransmitSHB(uint8_t *Buffer, size_t BufLen, const GnData_t &data, txcb_t txcb);

    // Member variables.
    uint16_t SequenceNumber_;
    int NeighborCount_;
    GnConfig_t Config_;
    LocationTable LocTable_;
    std::thread LocationServiceThread_;

    // member variables for CBF (contention based forwarding) implementation.
    std::mutex CBFmutex_;
    std::condition_variable CBFcv_;
    std::priority_queue<std::shared_ptr<Qelement>, std::vector<std::shared_ptr<Qelement>>, CompareTo> CBFqueue_;
    std::atomic<bool> CBFstop_;
    std::promise<int> CBFresult_;
    std::thread CBFTimerThread_;

    std::mutex qMutex_;
    std::shared_future<void> aSyncFuture_;
    // map use gn_addr->addr as key, each map element contains a deque.
    std::map<uint8_t *, std::shared_ptr<QueueT>, AddrCompare> LsMapQueue_;
    std::map<uint8_t *, std::shared_ptr<QueueT>, AddrCompare> UcMapQueue_;
    std::deque<std::shared_ptr<Qelement>> BcQueue_; //Also store TSB packets.

    // LsMap stores on-going LS request packets and status.
    std::map<uint8_t *, std::shared_ptr<Qelement>, AddrCompare> LsMap_;

    gn_addr_t itsGnLocalGnAddr_;

    std::shared_ptr<ILocationListener> locListener_;
    txcb_t df_txcb; // default radio transmit function.
    int LogLevel_;

};
} // namespace gn
#endif
