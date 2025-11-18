/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef COMMANDPROCESSOR_HPP
#define COMMANDPROCESSOR_HPP

#include <telux/sec/SecurityFactory.hpp>

struct Request {
    bool callerNoncePresent;
    uint32_t keySize;
    uint32_t macLength;
    uint32_t minMacLength;
    uint32_t publicExponent;
    telux::sec::Algorithm algo;
    telux::sec::KeyFormat keyFmt;
    telux::sec::DigestTypes digest;
    telux::sec::PaddingTypes padding;
    telux::sec::BlockModeTypes blockMode;
    telux::sec::CryptoOperationTypes operation;
    std::shared_ptr<telux::sec::EncryptedData> encData;
    std::vector<uint8_t> textA;
    std::vector<uint8_t> textB;
    std::vector<uint8_t> textC;
    std::vector<uint8_t> initVector;
    std::vector<uint8_t> uniqueData;
    std::vector<uint8_t> associatedData;
};

class CommandProcessor {
 public:
    int init(void);

    void generateKey(Request request,
            std::shared_ptr<std::string> keyBlobFile);

    void signData(Request request,
            std::shared_ptr<std::string> keyBlobFile,
            std::shared_ptr<std::string> plainTxtFile,
            std::shared_ptr<std::string> signatureFile);

    void verifySignature(Request request,
            std::shared_ptr<std::string> keyBlobFile,
            std::shared_ptr<std::string> plainTxtFile,
            std::shared_ptr<std::string> signatureFile);

    void encryptData(Request request,
            std::shared_ptr<std::string> keyBlobFile,
            std::shared_ptr<std::string> plainTxtFile,
            std::shared_ptr<std::string> encTxtFile);

    void decryptData(Request request,
            std::shared_ptr<std::string> keyBlobFile,
            std::shared_ptr<std::string> encTxtFile,
            std::shared_ptr<std::string> plainTxtFile);

    void importKey(Request request,
            std::shared_ptr<std::string> keyDataFile,
            std::shared_ptr<std::string> keyBlobFile);

    void exportKey(Request request,
            std::shared_ptr<std::string> keyBlobFile,
            std::shared_ptr<std::string> expDataFile);

    void upgradeKey(Request request,
            std::shared_ptr<std::string> keyBlobFileOld,
            std::shared_ptr<std::string> keyBlobFileNew);

 private:
    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr_;

    void byteArrayToHexString(std::vector<uint8_t> data);

    int saveOnFileSystem(std::vector<uint8_t> const &data,
         std::shared_ptr<std::string> absoluteFilePath);

    int readFileIntoVector(std::vector<uint8_t>& data,
        std::shared_ptr<std::string> absoluteFilePath);
};

#endif  // COMMANDPROCESSOR_HPP
