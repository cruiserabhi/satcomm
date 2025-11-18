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

#ifndef CRYPTOCONSOLEAPP_HPP
#define CRYPTOCONSOLEAPP_HPP

#include "common/console_app_framework/ConsoleApp.hpp"

#include "CommandProcessor.hpp"

class CryptoConsoleApp : public ConsoleApp {
 public:
    CryptoConsoleApp(std::string appName, std::string cursor);
    ~CryptoConsoleApp();

    void init(void);

    void generateKey(void);
    void signData(void);
    void verifySignature(void);
    void encryptData(void);
    void decryptData(void);
    void importKey(void);
    void exportKey(void);
    void upgradeKey(void);

 private:
    std::shared_ptr<CommandProcessor> cmdProcessor_;

    void getHexStringAsByteArrayFromUsr(const std::string choiceToDisplay,
        std::vector<uint8_t>& usrEntry, const uint32_t length);
    void getChoiceNumberFromUsr(const std::string choicesToDisplay, const uint32_t minVal,
        const uint32_t maxVal, uint32_t& selection, bool multipleOfEigth);
    void getMultipleChoiceNumberFromUsr(const std::string choicesToDisplay,
        const uint32_t minVal, const uint32_t maxVal, std::vector<uint32_t>& selection);
    void getFileFromUser(std::shared_ptr<std::string>& absoluteFilePath);
    void getAbsoluteFilePathFromUser(const std::string choicesToDisplay,
        std::shared_ptr<std::string>& absoluteFilePath);

    void getAlgorithmFromUser(telux::sec::Algorithm& algo, uint32_t restriction);
    void getDigestFromUser(telux::sec::DigestTypes& digest, uint32_t restriction);
    void getPaddingFromUser(telux::sec::PaddingTypes& padding, uint32_t restriction);
    void getBlockModeFromUser(telux::sec::BlockModeTypes& blockMode);
    void getCallerNoncePresentFromUser(bool& callerNoncePresent);
    void getKeySizeFromUser(uint32_t& keySize);
    void getPublicExponentFromUser(uint32_t& publicExponent);
    void getMinMacLengthFromUser(uint32_t& minMacLength, uint32_t maxVal);
    void getMacLengthFromUser(uint32_t& macLength, uint32_t maxVal);
    void getKeyFormatFromUser(telux::sec::KeyFormat& keyFmt);
    void getInitVectorFromUser(std::vector<uint8_t>& iv, uint32_t length);
    void getAssociatedDataFromUser(std::vector<uint8_t>& associatedData);
    void getUniqueDataFromUser(std::vector<uint8_t>& uniqueData);
    void getKeyDataFromUser(std::vector<uint8_t>& keyData);
    void getKeyBlobFromUser(std::vector<uint8_t>& keyBlob);
    void getPlainTextFromUser(std::vector<uint8_t>& plainText);
    void getSignatureFromUser(std::vector<uint8_t>& signature);
    void getEncryptedTextFromUser(std::vector<uint8_t>& encText);
    void getOperationFromUser(telux::sec::CryptoOperationTypes& operation,uint32_t restriction);
};

#endif // CRYPTOCONSOLEAPP_HPP
