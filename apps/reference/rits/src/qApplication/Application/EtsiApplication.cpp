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
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /**
  * @file: EtsiApplication.cpp
  *
  * @brief: class for ITS stack - ETSI
  */
#include "EtsiApplication.hpp"

EtsiApplication::EtsiApplication(char *fileConfiguration, MessageType msgType):
    ApplicationBase(fileConfiguration, msgType) {
}

EtsiApplication::EtsiApplication(const string txIpv4, const uint16_t txPort,
    const string rxIpv4, const uint16_t rxPort, char* fileConfiguration) :
    ApplicationBase(txIpv4, txPort, rxIpv4, rxPort, fileConfiguration) {
}

bool EtsiApplication::init() {
    if (not configuration.isValid) {
        printf("EtsiApplication invalid configuration\n");
        return false;
    }

    if (false == ApplicationBase::init()) {
        printf("EtsiApplication initialization failed\n");
        return false;
    }


    GnConfig_t GnCfg;
    GeoNetRouterImpl::InitDefaultConfig(GnCfg);
    memcpy(GnCfg.mid, this->configuration.MacAddr, GN_MID_LEN);
    GnCfg.StationType = static_cast<gn::ITSStationType>(this->configuration.StationType);

    std::unique_ptr<GeoNetRouterImpl> gnp(GeoNetRouterImpl::Instance(
                appLocListener_, GnCfg));

    GnRouter = std::move(gnp);
    GnRouter->SetLogLevel(4);

    return true;
}

EtsiApplication::~EtsiApplication() {
    GnRouter->Stop();

    if (isTxSim) {
        freeMsg(txSimMsg);
    }
    for (auto mc : eventContents) {
        freeMsg(mc);
    }
    for (auto mc : spsContents) {
        freeMsg(mc);
    }
    if (isRxSim) {
        freeMsg(rxSimMsg);
    }
    for (auto mc : receivedContents) {
        freeMsg(mc);
    }
}

bool EtsiApplication::initMsg(std::shared_ptr<msg_contents> mc, bool isRx) {
    mc->stackId = STACK_ID_ETSI;

    if (isRx) {
        // not allocate memory for Rx msg
        mc->gn = nullptr;
        mc->btp = nullptr;
        mc->cam = nullptr;
        mc->denm = nullptr;
    } else {
        mc->gn = malloc(sizeof(GnData_t));
        if (!mc->gn) {
            std::cerr << "alloc gn failed" << endl;
            return false;
        }
        memset(mc->gn, 0, sizeof(GnData_t));

        mc->btp = malloc(sizeof(btp_data_t));
        if (!mc->btp) {
            freeMsg(mc);
            std::cerr << "alloc btp failed" << endl;
            return false;
        }
        memset(mc->btp, 0, sizeof(btp_data_t));

        mc->cam = malloc(sizeof(CAM_t));
        if (!mc->cam) {
            freeMsg(mc);
            std::cerr << "alloc cam failed" << endl;
            return false;
        }
        memset(mc->cam, 0, sizeof(CAM_t));


        mc->denm = malloc(sizeof(DENM_t));
        if (!mc->denm) {
            freeMsg(mc);
            std::cerr << "alloc denm failed" << endl;
            return false;
        }
        memset(mc->denm, 0, sizeof(DENM_t));
    }

    return true;
}

void EtsiApplication::freeMsg(std::shared_ptr<msg_contents> mc) {
    if (mc->gn) {
        free(mc->gn);
        mc->gn = nullptr;
    }
    if (mc->btp) {
        free(mc->btp);
        mc->btp = nullptr;
    }
    if (mc->cam) {
        free_cam(mc->cam);
        mc->cam = nullptr;
    }
    if (mc->denm) {
        free_denm(mc->denm);
        mc->denm = nullptr;
    }
}

void EtsiApplication::fillMsg(std::shared_ptr<msg_contents> mc) {
    fillBtp(static_cast<btp_data_t *>(mc->btp));
    fillCam(static_cast<CAM_t *>(mc->cam));
    mc->etsi_msg_id = ItsPduHeader__messageID_cam;
}

void EtsiApplication::fillBtp(btp_data_t *btp) {
    btp->pkt_type = BTP_PACKET_TYPE_B;
    btp->dp_info = 0;
    btp->d_port = this->configuration.CAMDestinationPort;
}

void EtsiApplication::fillCam(CAM_t *cam) {
    memset(cam, 0, sizeof(CAM_t));
    cam->header.protocolVersion = ItsPduHeader__protocolVersion_currentVersion; // value is 1
    cam->header.messageID = ItsPduHeader__messageID_cam; // value is 2(cam)
    cam->header.stationID = 0;
    fillCamLocation(cam);
    fillCamCan(cam);
}

void EtsiApplication::fillCamLocation(CAM_t *cam) {
    shared_ptr<ILocationInfoEx> locationInfo = appLocListener_->getLocation();

    if (locationInfo == nullptr) {
        return;
    }

    ReferencePosition_t *RefPos = &(cam->cam.camParameters.basicContainer.referencePosition);

    // These unit convertion according to ETSI TS 102 894-2
    RefPos->latitude = (locationInfo->getLatitude() * 10000000);
    RefPos->longitude = (locationInfo->getLongitude() * 10000000);
    RefPos->altitude.altitudeValue = (locationInfo->getAltitude() * 100);
    RefPos->altitude.altitudeConfidence = AltitudeConfidence_alt_000_20;
    RefPos->positionConfidenceEllipse.semiMajorConfidence =
        (locationInfo->getHorizontalUncertaintySemiMajor() * 20);

    RefPos->positionConfidenceEllipse.semiMinorConfidence =
        (locationInfo->getHorizontalUncertaintySemiMinor() * 20);

    RefPos->positionConfidenceEllipse.semiMajorOrientation = 0;
    cam->cam.camParameters.highFrequencyContainer.present =
        HighFrequencyContainer_PR_basicVehicleContainerHighFrequency;

    BasicVehicleContainerHighFrequency_t *bvchf =
        &cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
    bvchf->heading.headingValue = 0;
    bvchf->heading.headingConfidence = HeadingConfidence_equalOrWithinZeroPointOneDegree;
    bvchf->speed.speedValue = (100 * locationInfo->getSpeed()); // 0.01 m/s
    bvchf->speed.speedConfidence = SpeedConfidence_equalOrWithinOneMeterPerSec;
    bvchf->driveDirection = DriveDirection_forward;
    bvchf->vehicleLength.vehicleLengthValue = 6;
    bvchf->vehicleLength.vehicleLengthConfidenceIndication =
        VehicleLengthConfidenceIndication_trailerPresenceIsUnknown;
    bvchf->vehicleWidth = 30;   //3 meters
    bvchf->longitudinalAcceleration.longitudinalAccelerationValue =
        (100 * locationInfo->getBodyFrameData().longAccel);

    bvchf->longitudinalAcceleration.longitudinalAccelerationConfidence =
        AccelerationConfidence_pointOneMeterPerSecSquared;
    bvchf->curvature.curvatureValue = CurvatureValue_straight;
    bvchf->curvature.curvatureConfidence = CurvatureConfidence_onePerMeter_0_00002;
    bvchf->curvatureCalculationMode = CurvatureCalculationMode_yawRateUsed;
    // radian to degree conversion, need multiply (180 div PI = 57.2958)
    // yawRate.yawRateValue unit: 0.01 degree per second
    bvchf->yawRate.yawRateValue = (locationInfo->getBodyFrameData().yawRate * 5729);
    bvchf->yawRate.yawRateConfidence = YawRateConfidence_degSec_000_10;
}

void EtsiApplication::fillCamCan(CAM_t *cam) {
    BasicVehicleContainerHighFrequency_t &bvchf =
        cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
    // vehicle hight/width/steerin wheel angle, etc.
}

int EtsiApplication::transmit(uint8_t index, std::shared_ptr<msg_contents> mc, int16_t bufLen,
        TransmitType txType) {
    GnData_t gd;

    // gd is used to set GeoNetwork parameters.
    GnRouter->InitDefaultGnData(gd);

    // upper_proto should match the setting in fillBtp
    gd.upper_prot = gn::UpperProtocol::GN_UPPER_PROTO_BTP_B;

    // packet type SHB (Single Hop Broadcast), will test more type later
    gd.pkt_type = gn::PacketType::GN_PACKET_TYPE_SHB;
    gd.is_shb = true;
    gd.payload_len = static_cast<int>(bufLen);
    gd.tc = 2;

    int ret = -1;
    // if the packet type is GBC(GeoNetwork Broadcast) or GAC (GeoNetwork Any
    // Cast), we also need to set the destination geographic area. But since we
    // are tesitng SHB here, we don't need to do that.

    // We create a transmit callback function before sending the packet to
    // GeoNet router, the router use it to send the packet to radio transmiter.
    // NOTE: the GnRouter->Transmit() will insert GN header and 1 byte cv2x
    // family ID
    if (this->isTxSim) {
        auto cb = std::bind(&RadioTransmit::transmit, simTransmit.get(),
                            std::placeholders::_1, std::placeholders::_2,
                            Priority::PRIORITY_UNKNOWN);
        ret = GnRouter->Transmit(mc, static_cast<size_t>(bufLen), gd, cb);
        return ret;
    }
    if (txType == TransmitType::SPS) {
        // SPS priority is set when creating the flow
        auto cb = std::bind(&RadioTransmit::transmit, &this->spsTransmits[index],
                std::placeholders::_1, std::placeholders::_2, Priority::PRIORITY_UNKNOWN);
        ret = GnRouter->Transmit(mc, static_cast<size_t>(bufLen), gd, cb);
    } else if (txType == TransmitType::EVENT) {
        // event priority is set per packet using traffic class
        auto cb = std::bind(&RadioTransmit::transmit, &this->eventTransmits[index],
                std::placeholders::_1, std::placeholders::_2, this->configuration.eventPriority);
        ret = GnRouter->Transmit(mc, static_cast<size_t>(bufLen), gd, cb);
    }
    return ret;
}

int EtsiApplication::receive(const uint8_t index, const uint16_t bufLen) {
    std::shared_ptr<msg_contents> mc = nullptr;
    int ret;
    uint64_t timestamp = 0;
    if (isRxSim) {
        mc = rxSimMsg;
    } else {
        mc = receivedContents[index];
    }

    if(mc->abuf.head == NULL || mc->abuf.size == 0){
        abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
    } else {
        abuf_reset(&mc->abuf, ABUF_HEADROOM);
    }

    mc->decoded = false;
    if (mc->gn == nullptr) {
        mc->gn = new char[sizeof(GnData_t)];
    }
    GnData_t &gd = *(static_cast<GnData_t *>(mc->gn));

    ret = radioReceives[0].receive(mc->abuf.data, ABUF_LEN-ABUF_HEADROOM);
    timestamp = timestamp_now();
    // Make sure packet is successfully received
    if(ret < MIN_PACKET_LEN || ret > MAX_PACKET_LEN || mc == nullptr){
        if(appVerbosity > 4){
            if(ret < 0){
                printf("Receive returned with error.\n");
            }else if(ret > 0 && ret < MIN_PACKET_LEN){
                printf("Dropping packet with %d bytes. Needs to be at least %d bytes.\n",
                        ret, MIN_PACKET_LEN);
            }else if(ret > 0 && ret >= MAX_PACKET_LEN){
                printf("Dropping packet with %d bytes. Needs to be less than %d bytes.\n",
                        ret, MAX_PACKET_LEN);
            }
            // if ret is 0, then polling timed out
        }
        return -1;
    }

    // needs to be done for data pointer to not override tail pointer
    mc->abuf.tail = mc->abuf.data + ret;

    if(appVerbosity > 7){
       printf("\n 2) Full rx packet with length %d\n", ret);
       print_buffer((uint8_t*)mc->abuf.data, ret);
       printf("\n");
    }

    // skip one byte cv2x family ID
    abuf_pull(&mc->abuf, 1);
    auto len = ret - 1;
    // process received packet in GeoNetRouter, packet may be dropped by router.
    ret = GnRouter->Receive(reinterpret_cast<uint8_t *>(mc->abuf.data), len, gd);
    if (ret == 0) {
        // remove GN header and decode it.
        abuf_pull(&mc->abuf, len - gd.payload_len);
        if (gd.upper_prot == UpperProtocol::GN_UPPER_PROTO_BTP_A)
            mc->btp_pkt_type = BTP_PACKET_TYPE_A;
        else if (gd.upper_prot == UpperProtocol::GN_UPPER_PROTO_BTP_B)
            mc->btp_pkt_type = BTP_PACKET_TYPE_B;
        else {
            std::cerr << "Unsupported transport type" << std::endl;
            return -1;
        }
        if (decode_msg(mc.get()) >= 0) {
            mc->decoded = true;
        }
    }
    return ret;
}
