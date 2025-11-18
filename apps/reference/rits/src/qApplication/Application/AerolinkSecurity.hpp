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
 *Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without
 *modification, are permitted (subject to the limitations in the
 *disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file: AerolinkSecurity.hpp
 *
 * @brief: Security Service wrapper for aerolink
 */

#ifndef AEROLINK_SECURITY_HPP_
#define AEROLINK_SECURITY_HPP_

#include <stdexcept>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <set>
#include "viicsec.h"
#include "MisbehaviorData.h"
#include "SecurityService.hpp"
#include "v2x_msg.h"
#include "v2x_codec.h"
#include "KinematicsReceive.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <semaphore.h>
#include <atomic>
#include <thread>
#include <iterator>
#include <chrono>
#include <sstream>

static size_t chunksize = 10000;

class AerolinkSecurity : public SecurityService {
    public:
        static AerolinkSecurity *pInstance;
        static AerolinkSecurity *Instance(std::string ctxName, uint16_t countryCode);
        static AerolinkSecurity *Instance(std::string ctxName, uint16_t countryCode,
                                             uint8_t keyGenMethod);
        static AerolinkSecurity *Instance(std::string ctxName, uint16_t countryCode,
                                             char const* lcmName, IDChangeData& idChangeData);
        int ExtractMsg( void* smp,
                        const SecurityOpt &opt,
                        const uint8_t * msg,
                        uint32_t msgLen,
                        uint8_t const *payload,
                        uint32_t       payloadLen,
                        uint32_t       &dot2HdrLen);
        int SignMsg(const SecurityOpt &opt, const uint8_t *msg, uint32_t msgLen,
                    uint8_t *signedSpdu, uint32_t &signedSpduLen,
                    SecurityService::SignType type = SecurityService::SignType::ST_AUTO);
        int VerifyMsg(const SecurityOpt &opt);
        int checkConsistencyandRelevancy(void* smp, const SecurityOpt &opt);
        int asyncVerify(Kinematics rvKine,
            MisbehaviorStats* misbehaviorStat,void *asyncCbData,
            uint8_t priority, ValidateCallback callBackFunction,
            SecuredMessageParserC* msgParseContext);
        static int setSecCurrLocation(Kinematics* hvKine);
        static int setLeapSeconds(uint32_t leapSeconds);
        int sspCheck(void* smp, uint8_t const* ssp);
        int idChange() override;
        int lockIdChange() override;
        int unlockIdChange() override;
        int createNewSmp(SecuredMessageParserC* smpPtr);
        int createNewSmg(SecuredMessageGeneratorC* smgPtr);
        AEROLINK_RESULT mbdCheck(Kinematics* rvBsmInfo,
            MisbehaviorStats* misbehaviorStat,
            SecuredMessageParserC* smp);
        void fillBsmDataForMbd(Kinematics* rvBsmData);

    private:
       // ctor for aerolink w/o encryption
       AerolinkSecurity(const std::string ctxName, uint16_t countryCode);
       // overloaded ctor for aerolink w/ encryption enabled
       AerolinkSecurity(const std::string ctxName, uint16_t countryCode,
                           uint8_t keyGenMethod);
       // overloaded ctor for aerolink w/ idchange enabled
       AerolinkSecurity(const std::string ctxName, uint16_t countryCode,
                           char const* lcmName, IDChangeData& idChangeData);

       static void signCallback(AEROLINK_RESULT returnCode, void *userCallbackData,
                            uint8_t* cbSignedSpduData, uint32_t cbSignedSpduDataLen);
       static void verifyCallback(AEROLINK_RESULT returnCode,
                            void *userCallbackData);
       int decryptMsg();
       int encryptMsg(uint8_t const * const plainText, uint32_t plainTextLength,
                    uint8_t isPayloadSpdu, uint8_t * const encryptedData,
                    uint32_t * const encryptedDataLength);
       int syncVerify(
            Kinematics hvKine, Kinematics rvKine,
            VerifStats *verifStat, MisbehaviorStats* misbehaviorStat);

        SecuredMessageGeneratorC smg_;
        SecuredMessageParserC smp_;
        int init(void);
        void deinit(void);
        ~AerolinkSecurity();
        SecurityContextC secContext_;
        AerolinkEncryptionKey  encryptionKey;
        // Multi-Threading Aerolink Variables
        std::map<std::thread::id, SecuredMessageParserC> threadSmps;
        std::map<std::thread::id, SecuredMessageGeneratorC> threadSmgs;
        std::map<std::thread::id, sem_t> verifSmpSems;
        std::map<std::thread::id, sem_t> signSmgSems;
        void setStartTime(double start);
        void setSecVerbosity(uint8_t verbosity);
        bool addNewThrSmp(std::thread::id thrId);
        bool addNewThrSmg(std::thread::id thrId);
        SecuredMessageParserC* getThrSmp(std::thread::id thrId);
        SecuredMessageGeneratorC* getThrSmg(std::thread::id thrId);
        sem_t* getThrSmpSem(std::thread::id thrId);
        sem_t* getThrSmgSem(std::thread::id thrId);

        // should be unique public encryption keys
        //    AerolinkEncryptionKey const * const recipients_[] = {};
        //    std::set<AerolinkEncryptionKey const *> recipients_;
        std::vector<AerolinkEncryptionKey const *> recipients_;
        uint32_t numRecipients_;
        uint8_t keyGenMethod_;
        char lcmName_[50] = "\0";
        IDChangeData* idChangeData_;
        bool enableMisbehavior;
        bool enableConsistency;
        bool enableRelevance;
        std::shared_ptr<BsmData> misbehaviorAppDataPtr = nullptr;
        std::shared_ptr<MisbehaviorDetectedType> misbehaviorResultPtr = nullptr;
        void printBytes(char *label, uint8_t buffer[], uint32_t length);
};
#endif
