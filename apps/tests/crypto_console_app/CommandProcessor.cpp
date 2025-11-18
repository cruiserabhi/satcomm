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

#include <sys/stat.h>
#include <iostream>
#include <cstdio>

#include <telux/sec/CryptoParamBuilder.hpp>

#include "CommandProcessor.hpp"

int CommandProcessor::init() {
    telux::common::ErrorCode ec;
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    cryptMgr_ = secFact.getCryptoManager(ec);
    if (!cryptMgr_) {
        std::cout << "Can't get ICryptoManager, err: " <<
            static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

void CommandProcessor::byteArrayToHexString(std::vector<uint8_t> data) {
    uint8_t *buf;
    uint32_t length;

    if (data.size()) {
        length = data.size();
        buf = data.data();
        for (uint32_t x = 0; x < length; x++) {
            printf("%02x", buf[x] & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
        fflush(stdout);
    }
}

int CommandProcessor::saveOnFileSystem(std::vector<uint8_t> const &data,
        std::shared_ptr<std::string> absoluteFilePath) {

    FILE *file;
    size_t dataLenSaved;

    file = fopen(absoluteFilePath->c_str(), "wb");
    if(!file) {
        std::cout << "can't create file " << *absoluteFilePath << std::endl;
        return -1;
    }

    dataLenSaved = fwrite(data.data(), 1, data.size(), file);
    if (dataLenSaved != data.size()) {
        std::cout <<
        "can't save full data on file, data size: " << data.size() << std::endl;
        /* don't treat as fatal, continue further */
    }

    fflush(file);
    fclose(file);

    return 0;
}

int CommandProcessor::readFileIntoVector(std::vector<uint8_t>& data,
        std::shared_ptr<std::string> absoluteFilePath) {

    FILE *file;
    uint8_t *tmp = nullptr;
    size_t dataLengthRead, lastChunkSize;
    struct stat fileInfo;
    int idx, ret, fullLoopCount;

    ret = stat(absoluteFilePath->c_str(), &fileInfo);
    if (ret < 0) {
        std::cout << "can't get file length " << *absoluteFilePath << std::endl;
        return -1;
    }

    tmp = new (std::nothrow) uint8_t[fileInfo.st_size];
    if (!tmp) {
        std::cout << "can't allocate memory" << std::endl;
        return -1;
    }

    file = fopen(absoluteFilePath->c_str(), "rb");
    if(!file) {
        std::cout << "can't open file " << *absoluteFilePath << std::endl;
        delete[] tmp;
        return -1;
    }

    fullLoopCount = fileInfo.st_size / 1024;
    lastChunkSize = fileInfo.st_size % 1024;

    idx = 0;
    for (int x=0; x < fullLoopCount; x++) {
        dataLengthRead = fread(&tmp[idx], 1, 1024, file);
        if (dataLengthRead != 1024) {
            std::cout << "can't read file " << *absoluteFilePath << std::endl;
            delete[] tmp;
            fclose(file);
            return -1;
        }
        idx = idx + 1024;
    }

    if (lastChunkSize) {
        dataLengthRead = fread(&tmp[idx], 1, lastChunkSize, file);
        if (dataLengthRead != lastChunkSize) {
            std::cout << "can't read file " << *absoluteFilePath << std::endl;
            delete[] tmp;
            fclose(file);
            return -1;
        }
    }

    data = std::vector<uint8_t>(tmp, tmp + fileInfo.st_size);

    delete[] tmp;
    fclose(file);

    return 0;
}

void CommandProcessor::generateKey(Request request,
        std::shared_ptr<std::string> keyBlobFile) {

    int ret;
    telux::common::ErrorCode ec;
    telux::sec::CryptoParamBuilder cpb;
    std::shared_ptr<telux::sec::ICryptoParam> cp;

    cpb = cpb.setAlgorithm(request.algo);
    cpb = cpb.setCryptoOperation(request.operation);
    cpb = cpb.setDigest(request.digest);
    cpb = cpb.setPadding(request.padding);
    cpb = cpb.setBlockMode(request.blockMode);
    cpb = cpb.setCallerNonce(request.callerNoncePresent);
    cpb = cpb.setInitVector(request.initVector);
    cpb = cpb.setMinimumMacLength(request.minMacLength);
    cpb = cpb.setPublicExponent(request.publicExponent);
    cpb = cpb.setKeySize(request.keySize);
    cp = cpb.build();

    ec = cryptMgr_->generateKey(cp, request.textA);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't generate key, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    if (keyBlobFile) {
        std::cout << "Generated key blob." << std::endl;
        ret = saveOnFileSystem(request.textA, keyBlobFile);
        if (ret < 0) {
            return;
        }
        std::cout << "Saved in file : " << *keyBlobFile << std::endl;
        return;
    }

    std::cout << "Generated key blob (displaying as hex string):" << std::endl;
    byteArrayToHexString(request.textA);
}

void CommandProcessor::signData(Request request,
        std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> plainTxtFile,
        std::shared_ptr<std::string> signatureFile) {

    int ret;
    telux::common::ErrorCode ec;
    telux::sec::CryptoParamBuilder cpb;
    std::shared_ptr<telux::sec::ICryptoParam> cp;

    cpb = cpb.setAlgorithm(request.algo);
    cpb = cpb.setDigest(request.digest);
    cpb = cpb.setPadding(request.padding);
    cpb = cpb.setMacLength(request.macLength);
    cp = cpb.build();

    if (keyBlobFile) {
        ret = readFileIntoVector(request.textA, keyBlobFile);
        if (ret < 0) {
            return;
        }
    }

    if (plainTxtFile) {
        ret = readFileIntoVector(request.textB, plainTxtFile);
        if (ret < 0) {
            return;
        }
    }

    ec = cryptMgr_->signData(cp, request.textA, request.textB, request.textC);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't sign, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    if (signatureFile) {
        std::cout << "Generated signature." << std::endl;
        ret = saveOnFileSystem(request.textC, signatureFile);
        if (ret < 0) {
            return;
        }
        std::cout << "Saved in file : " << *signatureFile << std::endl;
        return;
    }

    std::cout << "Generated signature (displaying as hex string):" << std::endl;
    byteArrayToHexString(request.textC);
}

void CommandProcessor::verifySignature(Request request,
        std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> plainTxtFile,
        std::shared_ptr<std::string> signatureFile) {

    int ret;
    telux::common::ErrorCode ec;
    telux::sec::CryptoParamBuilder cpb;
    std::shared_ptr<telux::sec::ICryptoParam> cp;

    cpb = cpb.setAlgorithm(request.algo);
    cpb = cpb.setCryptoOperation(request.operation);
    cpb = cpb.setDigest(request.digest);
    cpb = cpb.setPadding(request.padding);
    cp = cpb.build();

    if (keyBlobFile) {
        ret = readFileIntoVector(request.textA, keyBlobFile);
        if (ret < 0) {
            return;
        }
    }

    if (plainTxtFile) {
        ret = readFileIntoVector(request.textB, plainTxtFile);
        if (ret < 0) {
            return;
        }
    }

    if (signatureFile) {
        ret = readFileIntoVector(request.textC, signatureFile);
        if (ret < 0) {
            return;
        }
    }

    ec = cryptMgr_->verifyData(cp, request.textA, request.textB, request.textC);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "invalid signature, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "verification succeeded, err " << static_cast<int>(ec) << std::endl;
}

void CommandProcessor::encryptData(Request request,
        std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> plainTxtFile,
        std::shared_ptr<std::string> encTxtFile) {

    int ret;
    telux::common::ErrorCode ec;
    telux::sec::CryptoParamBuilder cpb;
    std::shared_ptr<telux::sec::ICryptoParam> cp;

    cpb = cpb.setAlgorithm(request.algo);
    cpb = cpb.setDigest(request.digest);
    cpb = cpb.setPadding(request.padding);
    cpb = cpb.setBlockMode(request.blockMode);
    cpb = cpb.setInitVector(request.initVector);
    cpb = cpb.setMacLength(request.macLength);
    cp = cpb.build();

    if (keyBlobFile) {
        ret = readFileIntoVector(request.textA, keyBlobFile);
        if (ret < 0) {
            return;
        }
    }

    if (plainTxtFile) {
        ret = readFileIntoVector(request.textB, plainTxtFile);
        if (ret < 0) {
            return;
        }
    }

    ec = cryptMgr_->encryptData(cp, request.textA, request.textB, request.encData);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't encrypt, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    if (encTxtFile) {
        std::cout << "Encrypted data." << std::endl;
        ret = saveOnFileSystem(request.encData->encryptedText, encTxtFile);
        if (ret < 0) {
            return;
        }
        std::cout << "Saved in file : " << *encTxtFile << std::endl;
        return;
    }

    std::cout << "Encrypted data (displaying as hex string): " << std::endl;
    byteArrayToHexString(request.encData->encryptedText);
}

void CommandProcessor::decryptData(Request request,
        std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> encTxtFile,
        std::shared_ptr<std::string> plainTxtFile) {

    int ret;
    telux::common::ErrorCode ec;
    telux::sec::CryptoParamBuilder cpb;
    std::shared_ptr<telux::sec::ICryptoParam> cp;

    cpb = cpb.setAlgorithm(request.algo);
    cpb = cpb.setDigest(request.digest);
    cpb = cpb.setPadding(request.padding);
    cpb = cpb.setBlockMode(request.blockMode);
    cpb = cpb.setInitVector(request.initVector);
    cpb = cpb.setMacLength(request.macLength);
    cp = cpb.build();

    if (keyBlobFile) {
        ret = readFileIntoVector(request.textA, keyBlobFile);
        if (ret < 0) {
            return;
        }
    }

    if (encTxtFile) {
        ret = readFileIntoVector(request.textC, encTxtFile);
        if (ret < 0) {
            return;
        }
    }

    ec = cryptMgr_->decryptData(cp, request.textA, request.textC, request.textB);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't decrypt, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    if (plainTxtFile) {
        std::cout << "Decrypted data." << std::endl;
        ret = saveOnFileSystem(request.textB, plainTxtFile);
        if (ret < 0) {
            return;
        }
        std::cout << "Saved in file : " << *plainTxtFile << std::endl;
        return;
    }

    std::cout << "Decrypted data (displaying as hex string): " << std::endl;
    byteArrayToHexString(request.textB);
}

void CommandProcessor::importKey(Request request,
        std::shared_ptr<std::string> keyDataFile,
        std::shared_ptr<std::string> keyBlobFile) {

    int ret;
    telux::common::ErrorCode ec;
    telux::sec::CryptoParamBuilder cpb;
    std::shared_ptr<telux::sec::ICryptoParam> cp;

    cpb = cpb.setAlgorithm(request.algo);
    cpb = cpb.setCryptoOperation(request.operation);
    cpb = cpb.setDigest(request.digest);
    cpb = cpb.setPadding(request.padding);
    cpb = cpb.setBlockMode(request.blockMode);
    cpb = cpb.setPublicExponent(request.publicExponent);
    cpb = cpb.setMinimumMacLength(request.minMacLength);
    cpb = cpb.setCallerNonce(request.callerNoncePresent);
    cp = cpb.build();

    if (keyDataFile) {
        ret = readFileIntoVector(request.textB, keyDataFile);
        if (ret < 0) {
            return;
        }
    }

    ec = cryptMgr_->importKey(cp, request.keyFmt, request.textB, request.textA);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't import key, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    if (keyBlobFile) {
        std::cout << "Imported key." << std::endl;
        ret = saveOnFileSystem(request.textA, keyBlobFile);
        if (ret < 0) {
            return;
        }
        std::cout << "Saved in file : " << *keyBlobFile << std::endl;
        return;
    }

    std::cout << "Imported key (displaying as hex string): " << std::endl;
    byteArrayToHexString(request.textA);
}

void CommandProcessor::exportKey(Request request,
        std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> expDataFile) {

    int ret;
    telux::common::ErrorCode ec;

    if (keyBlobFile) {
        ret = readFileIntoVector(request.textA, keyBlobFile);
        if (ret < 0) {
            return;
        }
    }

    ec = cryptMgr_->exportKey(request.keyFmt, request.textA, request.textB);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't export key, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    if (expDataFile) {
        std::cout << "Exported key." << std::endl;
        ret = saveOnFileSystem(request.textB, expDataFile);
        if (ret < 0) {
            return;
        }
        std::cout << "Saved in file : " << *expDataFile << std::endl;
        return;
    }

    std::cout << "Exported key (displaying as hex string): " << std::endl;
    byteArrayToHexString(request.textB);
}

void CommandProcessor::upgradeKey(Request request,
        std::shared_ptr<std::string> keyBlobFileOld,
        std::shared_ptr<std::string> keyBlobFileNew) {

    int ret;
    telux::common::ErrorCode ec;
    std::shared_ptr<telux::sec::ICryptoParam> cp;

    if (request.uniqueData.size() > 0) {
        cp = telux::sec::CryptoParamBuilder()
                .setUniqueData(request.uniqueData)
                .build();
    }

    if (keyBlobFileOld) {
        ret = readFileIntoVector(request.textA, keyBlobFileOld);
        if (ret < 0) {
            return;
        }
    }

    ec = cryptMgr_->upgradeKey(cp, request.textA, request.textB);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't upgrade key, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    if (request.textB.size() > 0) {
        if (keyBlobFileNew) {
            std::cout << "Upgraded key." << std::endl;
            ret = saveOnFileSystem(request.textC, keyBlobFileNew);
            if (ret < 0) {
                return;
            }
            std::cout << "Saved in file : " << *keyBlobFileNew << std::endl;
            return;
        }

        std::cout << "Upgraded key (displaying as hex string): " << std::endl;
        byteArrayToHexString(request.textB);
    }

    std::cout << "Key is already upto date." << std::endl;
}
