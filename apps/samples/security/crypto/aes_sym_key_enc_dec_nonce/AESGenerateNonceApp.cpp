/*
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * 1. Generate AES symmetric key
 * 2. Encrypt given data using this key and generate nonce
 * 3. Decrypt given data using this key and generated nonce
 */

#include <iostream>
#include <cstring>

#include <telux/sec/SecurityFactory.hpp>
#include <telux/sec/CryptoManager.hpp>
#include <telux/sec/CryptoParamBuilder.hpp>

int generateAESKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
            std::vector<uint8_t> &kb) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    // Define parameters for the key
    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_AES)
            .setCryptoOperation(telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT |
                        telux::sec::CryptoOperation::CRYPTO_OP_DECRYPT)
            .setKeySize(128)
            .setBlockMode(telux::sec::BlockMode::BLOCK_MODE_CBC |
                        telux::sec::BlockMode::BLOCK_MODE_CTR)
            .setPadding(telux::sec::Padding::PADDING_PKCS7 |
                        telux::sec::Padding::PADDING_NONE)
            .build();

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate AES sym key, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int encryptDataWithAESKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
            std::vector<uint8_t> kb,
            std::vector<uint8_t> pt,
            std::shared_ptr<telux::sec::EncryptedData> &ed) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_AES)
            .setBlockMode(telux::sec::BlockMode::BLOCK_MODE_CBC)
            .setPadding(telux::sec::Padding::PADDING_PKCS7)
            .build();

    ec = cryptMgr->encryptData(cp, kb, pt, ed);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't encrypt data, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int decryptDataWithAESKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
            std::vector<uint8_t> kb,
            std::shared_ptr<telux::sec::EncryptedData> ed,
            std::vector<uint8_t> &dt) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_AES)
            .setBlockMode(telux::sec::BlockMode::BLOCK_MODE_CBC)
            .setPadding(telux::sec::Padding::PADDING_PKCS7)
            .setInitVector(ed->nonce)
            .build();

    ec = cryptMgr->decryptData(cp, kb, ed->encryptedText, dt);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't decrypt data, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {

    int ret;
    telux::common::ErrorCode ec;
    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr;

    std::shared_ptr<telux::sec::EncryptedData> ed;
    std::vector<uint8_t> dt;
    std::vector<uint8_t> kb;

    // Specify data to be encrypted
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

    // Generate AES asymmetric key
    ret = generateAESKey(cryptMgr, kb);
    if (ret) {
        return ret;
    }

    // Encrypt data
    ret = encryptDataWithAESKey(cryptMgr, kb, pt, ed);
    if (ret) {
        return ret;
    }

    // Decrypt data
    ret = decryptDataWithAESKey(cryptMgr, kb, ed, dt);
    if (ret) {
        return ret;
    }

    // Compare encrypted and decrypted data matches
    if (pt == dt) {
        std::cout << "Enc & Dec data matches!" << std::endl;
    } else {
        std::cout << "Enc & Dec data do not match!" << std::endl;
    }

    return ret;
}
