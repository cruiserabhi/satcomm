/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <vector>
#include <telux/common/Version.hpp>

#include "VersionInfo.hpp"
#include "Logger.hpp"

#define VERSIONS_SIZE 3

using namespace telux::common;

namespace telux {
namespace common {

SdkVersion Version::getSdkVersion() {
    SdkVersion sdkVersion;
    std::string versionStr(SDK_VERSION);
    std::string delimiter = ".";
    std::string token;
    std::vector<std::string> tokens;

    size_t position = 0;
    size_t end = 0;
    size_t strLength = 0;
    do {
        end = versionStr.find(delimiter, position);
        strLength = end - position;
        token = versionStr.substr(position, strLength);
        if(!token.empty()) {
            tokens.emplace_back(token);
        }
        position += strLength + delimiter.length();
    } while(end != std::string::npos);

    if(tokens.size() == VERSIONS_SIZE) {
        sdkVersion.major = std::stoi(tokens.at(0));
        sdkVersion.minor = std::stoi(tokens.at(1));
        sdkVersion.patch = std::stoi(tokens.at(2));
    } else {
        LOG(DEBUG, " Invalid version length ");
    }
    return sdkVersion;
}

std::string Version::getReleaseName() {
   return RELEASE_NAME;
}

}
}