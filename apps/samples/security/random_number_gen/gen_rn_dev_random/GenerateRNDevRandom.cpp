/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * Sample application to demonstrate how to:
 * 1. How to generate random number and random data bytes using TRNG.
 */

#include <iostream>

#include <telux/sec/RandomNumberManager.hpp>
#include <telux/sec/SecurityFactory.hpp>

int main(int argc, char **argv) {

    size_t numBytes = 0;
    uint8_t *data = nullptr;
    uint32_t randNum32 = 0;
    uint64_t randNum64 = 0;
    telux::common::ErrorCode ec;
    std::vector<uint8_t> generatedData(16, 0);

    std::shared_ptr<telux::sec::IRandomNumberManager> rngMgr;

    /* Get SecurityFactory instance */
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    /* Get CryptoManager instance with TRNG as source */
    rngMgr = secFact.getRandomNumberManager(telux::sec::RNGSource::DEV_RANDOM, ec);
    if (!rngMgr) {
        std::cout << "Can't allocate IRandomNumberManager, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }

    /* Generate 32 bit random number */
    ec = rngMgr->getRandomNumber(randNum32);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed 32 bit number generation, err: " <<
            static_cast<int>(ec) << std::endl;
        return -1;
    }

    std::cout << "32 bit random number generated: " << randNum32 << std::endl;

    /* Generate 64 bit random number */
    ec = rngMgr->getRandomNumber(randNum64);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed 64 bit number generation, err: " <<
            static_cast<int>(ec) << std::endl;
        return -1;
    }

    std::cout << "64 bit random number generated: " << randNum64 << std::endl;

    /* Generate 16 random data bytes */
    ec = rngMgr->getRandomData(generatedData, numBytes);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed data generation, err: " <<
            static_cast<int>(ec) << std::endl;
        return -1;
    }

    printf("numBytes: %zu\n", numBytes);
    printf("random data generated: ");
    data = generatedData.data();
    for (size_t x = 0; x < numBytes; x++) {
        printf("%02x", data[x] & 0xffU);
    }
    printf("\n");
    fflush(stdout);

    return 0;
}
