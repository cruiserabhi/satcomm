/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * 1. Generate EC asymmetric key
 * 2. Sign given data using this key
 * 3. Verify data using this key
 */

#include <iostream>

#include <telux/sec/SecurityFactory.hpp>
#include <telux/sec/CryptoManager.hpp>
#include <telux/sec/CryptoParamBuilder.hpp>

int generateECKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
            std::vector<uint8_t> &kb) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    // Define parameters for the key
    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_EC)
            .setCryptoOperation(telux::sec::CryptoOperation::CRYPTO_OP_SIGN |
                        telux::sec::CryptoOperation::CRYPTO_OP_VERIFY)
            .setKeySize(256)
            .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
            .build();

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate EC asym key, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int signDataUsingECKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
            std::vector<uint8_t> kb,
            std::vector<uint8_t> pt,
            std::vector<uint8_t> &sg) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_EC)
            .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
            .build();

    ec = cryptMgr->signData(cp, kb, pt, sg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't sign data, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int verifyDataUsingECSignature(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
            std::vector<uint8_t> kb,
            std::vector<uint8_t> pt,
            std::vector<uint8_t> sg) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_EC)
            .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
            .build();

    ec = cryptMgr->verifyData(cp, kb, pt, sg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        if (ec == telux::common::ErrorCode::VERIFICATION_FAILED)
            std::cout << "Invalid signature for given data!" << std::endl;
        else
            std::cout << "Can't verify data, err: " <<
                            static_cast<int>(ec) << std::endl;
        return -1;
    }

    std::cout << "Data verified!" << std::endl;
    return 0;
}

int main(int argc, char **argv) {

    int ret;
    telux::common::ErrorCode ec;
    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr;

    std::vector<uint8_t> sg;
    std::vector<uint8_t> kb;

    // Specify data to be signed and verified
    std::vector<uint8_t> pt {'h', 'e', 'l', 'l', 'o'};

    // Get SecurityFactory instance
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    // Get CryptoManager instance
    cryptMgr = secFact.getCryptoManager(ec);
    if (!cryptMgr) {
        std::cout << "Can't allocate CryptoManager, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }

    // Generate EC asymmetric key
    ret = generateECKey(cryptMgr, kb);
    if (ret) {
        return ret;
    }

    // Sign the given data
    ret = signDataUsingECKey(cryptMgr, kb, pt, sg);
    if (ret) {
        return ret;
    }

    // Verify if signature is valid or not for the given data
    ret = verifyDataUsingECSignature(cryptMgr, kb, pt, sg);
    if (ret) {
        return ret;
    }

    return ret;
}
