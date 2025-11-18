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
 *Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
  * @file: NullSecurity.hpp
  *
  * @brief: A dummy security implementation, just to get code to compile without
  * the need of Aerolink library.
  */
#ifndef NULLSECURITY_HPP_
#define NULLSECURITY_HPP_

#include "SecurityService.hpp"
#include <iostream>

class NullSecurity : public SecurityService {
private:
    NullSecurity(std::string ctxName, uint16_t countryCode);
    static NullSecurity *pInstance;
public:
    static NullSecurity *Instance(std::string ctxName, uint16_t countryCode);
    int ExtractMsg( void* smp,
                    const SecurityOpt &opt,
                    const uint8_t * msg,
                    uint32_t msgLen,
                    uint8_t const *payload,
                    uint32_t       payloadLen,
                    uint32_t       &dot2HdrLen){
        std::cout << "NULL security ExtractMsg() " << std::endl;
        return -1;
    }
    int SignMsg(const SecurityOpt &opt, const uint8_t *msg, uint32_t msgLen, uint8_t *signedSpdu,
            uint32_t &signedSpduLen,
            SecurityService::SignType type = SecurityService::SignType::ST_AUTO) {
        std::cout << "NULL security SignMsg()" << std::endl;
        return -1;
    }
    int VerifyMsg(const SecurityOpt &opt) {
        std::cout << "NULL security VerifyMsg() " << std::endl;
        return -1;
    }
    void setSecVerbosity(uint8_t verbosity) { };
    int sspCheck( void* smp, uint8_t const* ssp) {return 0;};
    int idChange() {return 0;};
    int lockIdChange() {return 0;};
    int unlockIdChange() {return 0;};
    int init() {return 0; }
    void deinit() { }
};
#endif
