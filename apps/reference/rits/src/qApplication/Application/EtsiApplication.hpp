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
 *  Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
  * @file: EtsiApplication.hpp
  *
  * @brief: class for ITS stack - ETSI
  */
#include "ApplicationBase.hpp"
#include "GeoNetRouterImpl.hpp"
using namespace gn;

class EtsiApplication : public ApplicationBase {
public:
    EtsiApplication(char *fileConfiguration, MessageType msgType);
    EtsiApplication(const string txIpv4, const uint16_t txPort,
        const string rxIpv4, const uint16_t rxPort, char* fileConfiguration);

    ~EtsiApplication();

    /* Initialization */
    bool init() override;

    /**
    * Method to setup and perform transmission for ETSI packets.
    * @param index - An uint8_t that is used for which buffer to access
    * @param bufLen - Length of given buffer or message
    * @param txType - Specifies the type of message for decoding
    */
    int transmit(uint8_t index, std::shared_ptr<msg_contents>mc, int16_t bufLen,
            TransmitType txType);

    /**
    * Method to setup and perform reception for ETSI packets.
    * @param index - An uint8_t that is used for which buffer to access
    * @param bufLen - Length of given buffer or message
    */
    int receive(const uint8_t index, const uint16_t bufLen);

    /**
    * Overall method to fill msg_contents struct based on ETSI packet contents.
    * @param mc - A shared pointer to the msg_contents struct
    */
    void fillMsg(std::shared_ptr<msg_contents> mc);

private:

    /**
    * Method to initialize ETSI packets in msg_contents struct.
    * @param mc - A shared pointer to the msg_contents struct
    * @param isRx - A flag to indicate if the packet is used for Rx
    */
    bool initMsg(std::shared_ptr<msg_contents> mc, bool isRx = false) override;

    /**
    * Method to delete and free ETSI packet memory in msg_contents struct.
    * @param mc - A shared pointer to the msg_contents struct
    */
    void freeMsg(std::shared_ptr<msg_contents> mc) override;

    /**
    * Method to setup and fill BTP related information.
    * @param bsm - A pointer to the BTP struct
    */
    void fillBtp(btp_data_t *btp);

    /**
    * Method to setup and fill CAM related information.
    * @param cam - A pointer to the CAM struct
    */
    void fillCam(CAM_t *cam);

    /**
    * Method to setup and fill CAM Location related information.
    * @param cam - A pointer to the CAM struct
    */
    void fillCamLocation(CAM_t *cam);


    void fillCamCan(CAM_t *cam);

    std::unique_ptr<GeoNetRouterImpl> GnRouter;
};
