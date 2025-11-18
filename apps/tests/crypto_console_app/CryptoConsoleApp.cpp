/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <cstdio>

#include <telux/common/Version.hpp>

#include "common/utils/Utils.hpp"

#include "CryptoConsoleApp.hpp"

CryptoConsoleApp::CryptoConsoleApp(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

CryptoConsoleApp::~CryptoConsoleApp() {
}

void CryptoConsoleApp::getHexStringAsByteArrayFromUsr(
        const std::string choiceToDisplay, std::vector<uint8_t>& usrEntry,
        const uint32_t length) {

    std::size_t x = 0;
    std::size_t idx = 0;
    std::size_t byteArraySize = 0;
    std::string usrInput = "";
    uint8_t byteFromUsr = 0;

    while(1) {
        std::cout << choiceToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()
            || ((usrInput.size() % 2) != 0)) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        byteArraySize = usrInput.size() / 2;

        if ((length) && (byteArraySize != length)) {
            std::cout << "invalid length" << std::endl;
            continue;
        }

        for (x = 0; x < byteArraySize; x++, idx += 2) {
            try {
                byteFromUsr = std::stoul(usrInput.substr(idx, 2), nullptr, 16);
                usrEntry.push_back(byteFromUsr);
            } catch (const std::exception& e) {
                usrEntry.resize(0);
                std::cout << "invalid characters " << usrInput.substr(idx, 2) << std::endl;
                break;
            }
        }

        if (x == byteArraySize) {
            return;
        }
    }
}

void CryptoConsoleApp::getChoiceNumberFromUsr(
        const std::string choicesToDisplay, const uint32_t minVal,
        const uint32_t maxVal, uint32_t& selection, bool multipleOfEigth) {

    uint32_t numFromUsr;
    std::string usrInput = "";

    while(1) {
        std::cout << choicesToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        try {
            numFromUsr = (std::stoul(usrInput) & 0xFFFFFFFF);
        } catch (const std::exception& e) {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }

        if ((numFromUsr > maxVal) || (numFromUsr < minVal)) {
            std::cout << "invalid input " << numFromUsr << std::endl;
            continue;
        }

        if (multipleOfEigth && ((numFromUsr % 8) != 0)) {
            std::cout << "invalid input " << numFromUsr << std::endl;
            continue;
        }

        selection = numFromUsr;
        return;
    }
}

void CryptoConsoleApp::getFileFromUser(std::shared_ptr<std::string>& absoluteFilePath) {
    std::string usrInput = "";

    while(1) {
        std::cout << "Enter absolute file path : ";

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        try {
            absoluteFilePath = std::make_shared<std::string>(usrInput);
        } catch (const std::exception& e) {
            std::cout << "can't allocate string" << std::endl;
            absoluteFilePath = nullptr;
        }
        return;
    }
}

void CryptoConsoleApp::getAbsoluteFilePathFromUser(const std::string choicesToDisplay,
        std::shared_ptr<std::string>& absoluteFilePath) {

    std::string usrInput = "";
    size_t choiceLength;

    while(1) {
        std::cout << choicesToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        choiceLength = usrInput.length();
        for (size_t x = 0; x < choiceLength; x++) {
            usrInput[x] = tolower(usrInput[x]);
        }

        if (!usrInput.compare("no")) {
            absoluteFilePath = nullptr;
            return;
        } else if (!usrInput.compare("yes")) {
            getFileFromUser(absoluteFilePath);
            return;
        } else {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }
    }
}

void CryptoConsoleApp::getMultipleChoiceNumberFromUsr(
        const std::string choicesToDisplay, const uint32_t minVal,
        const uint32_t maxVal, std::vector<uint32_t>& selection) {

    uint32_t numFromUsr;
    std::string usrInput = "";

    while(1) {
        std::cout << choicesToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }

        for (uint32_t x = 0; x < usrInput.size(); x++) {
            switch (usrInput[x]) {
                case '1':
                    numFromUsr = 1;
                    break;
                case '2':
                    numFromUsr = 2;
                    break;
                case '3':
                    numFromUsr = 3;
                    break;
                case '4':
                    numFromUsr = 4;
                    break;
                case '5':
                    numFromUsr = 5;
                    break;
                case '6':
                    numFromUsr = 6;
                    break;
                case '7':
                    numFromUsr = 7;
                    break;
                default:
                    numFromUsr = 0;
                    break;
            }

            if (numFromUsr && ((numFromUsr > maxVal) || (numFromUsr < minVal))) {
                std::cout << "invalid input ignored " << numFromUsr << std::endl;
                continue;
            }
            if (numFromUsr) {
                selection.push_back(numFromUsr);
            }
        }

        return;
    }
}

void CryptoConsoleApp::getAlgorithmFromUser(telux::sec::Algorithm& algo,
        uint32_t restriction) {

    uint32_t usrEntry;

    if (restriction == 1) {
        getChoiceNumberFromUsr(
            "Enter algorithm (1 - RSA, 2 - EC, 3 - HMAC): ", 1, 3, usrEntry, false);

        switch (usrEntry) {
            case 1:
                algo = telux::sec::Algorithm::ALGORITHM_RSA;
                break;
            case 2:
                algo = telux::sec::Algorithm::ALGORITHM_EC;
                break;
            case 3:
                algo = telux::sec::Algorithm::ALGORITHM_HMAC;
                break;
            default:
                std::cout << "invalid algorithm " << usrEntry << std::endl;
        }
    } else if (restriction == 2) {
        getChoiceNumberFromUsr(
            "Enter algorithm (1 - RSA, 2 - AES): ", 1, 2, usrEntry, false);

        switch (usrEntry) {
            case 1:
                algo = telux::sec::Algorithm::ALGORITHM_RSA;
                break;
            case 2:
                algo = telux::sec::Algorithm::ALGORITHM_AES;
                break;
            default:
                std::cout << "invalid algorithm " << usrEntry << std::endl;
        }
    } else {
        getChoiceNumberFromUsr(
            "Enter algorithm (1 - RSA, 2 - EC, 3 - AES, 4 - HMAC): ", 1, 4, usrEntry, false);

        switch (usrEntry) {
            case 1:
                algo = telux::sec::Algorithm::ALGORITHM_RSA;
                break;
            case 2:
                algo = telux::sec::Algorithm::ALGORITHM_EC;
                break;
            case 3:
                algo = telux::sec::Algorithm::ALGORITHM_AES;
                break;
            case 4:
                algo = telux::sec::Algorithm::ALGORITHM_HMAC;
                break;
            default:
                std::cout << "invalid algorithm " << usrEntry << std::endl;
        }
    }
}

void CryptoConsoleApp::getOperationFromUser(telux::sec::CryptoOperationTypes& operation,
        uint32_t restriction) {

    uint32_t usrEntry;

    if (restriction == 1) {
        getChoiceNumberFromUsr(
            "Enter key usage (1 - Sign and verify): ",
            1, 1, usrEntry, false);
        switch (usrEntry) {
            case 1:
                operation = telux::sec::CryptoOperation::CRYPTO_OP_SIGN |
                    telux::sec::CryptoOperation::CRYPTO_OP_VERIFY;
                break;
            default:
                std::cout << "invalid crypto operation " << usrEntry << std::endl;
        }
    } else if (restriction == 2) {
        getChoiceNumberFromUsr(
            "Enter key usage (1 - Encrypt and decrypt): ",
            1, 1, usrEntry, false);
        switch (usrEntry) {
            case 1:
                operation = telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT |
                    telux::sec::CryptoOperation::CRYPTO_OP_DECRYPT;
                break;
            default:
                std::cout << "invalid crypto operation " << usrEntry << std::endl;
        }
    } else {
        getChoiceNumberFromUsr(
            "Enter key usage (1 - Sign & verify, 2 - Encrypt & decrypt, 3 - All): ",
            1, 3, usrEntry, false);
        switch (usrEntry) {
            case 1:
                operation = telux::sec::CryptoOperation::CRYPTO_OP_SIGN |
                    telux::sec::CryptoOperation::CRYPTO_OP_VERIFY;
                break;
            case 2:
                operation = telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT |
                    telux::sec::CryptoOperation::CRYPTO_OP_DECRYPT;
                break;
            case 3:
                operation = (telux::sec::CryptoOperation::CRYPTO_OP_SIGN |
                    telux::sec::CryptoOperation::CRYPTO_OP_VERIFY |
                    telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT |
                    telux::sec::CryptoOperation::CRYPTO_OP_DECRYPT);
                break;
            default:
                std::cout << "invalid crypto operation " << usrEntry << std::endl;
        }
    }
}

void CryptoConsoleApp::getDigestFromUser(telux::sec::DigestTypes& digest,
        uint32_t restriction) {

    uint32_t usrEntry;
    std::vector<uint32_t> selection;

    if (restriction == 1) {
        getChoiceNumberFromUsr(
            "Enter digest (1 - SHA2-256): ", 1, 1, usrEntry, false);

        switch (usrEntry) {
            case 1:
                digest = telux::sec::Digest::DIGEST_SHA_2_256;
                break;
            default:
                std::cout << "invalid digest " << usrEntry << std::endl;
        }
    } else {
        std::string choiceStr = "Enter digest, comma separated ";
        choiceStr.append("(1 - None, 2 - MD5, 3 - SHA1, ");
        choiceStr.append("4 - SHA2-224, 5 - SHA2-256, 6 - SHA2-384, 7 - SHA2-512): ");
        getMultipleChoiceNumberFromUsr(choiceStr, 1, 7, selection);

        for (uint32_t x = 0; x < selection.size(); x++) {
            switch (selection.at(x)) {
                case 1:
                    digest |= telux::sec::Digest::DIGEST_NONE;
                    break;
                case 2:
                    digest |= telux::sec::Digest::DIGEST_MD5;
                    break;
                case 3:
                    digest |= telux::sec::Digest::DIGEST_SHA1;
                    break;
                case 4:
                    digest |= telux::sec::Digest::DIGEST_SHA_2_224;
                    break;
                case 5:
                    digest |= telux::sec::Digest::DIGEST_SHA_2_256;
                    break;
                case 6:
                    digest |= telux::sec::Digest::DIGEST_SHA_2_384;
                    break;
                case 7:
                    digest |= telux::sec::Digest::DIGEST_SHA_2_512;
                    break;
                default:
                    std::cout << "invalid digest " << selection.at(x) << std::endl;
            }
        }
    }
}

void CryptoConsoleApp::getPaddingFromUser(telux::sec::PaddingTypes& padding,
        uint32_t restriction) {

    uint32_t usrEntry;
    std::vector<uint32_t> selection;

    if (restriction == 1) {
        getChoiceNumberFromUsr(
            "Enter padding (1 - None): ", 1, 1, usrEntry, false);

        switch (usrEntry) {
            case 1:
                padding = telux::sec::Padding::PADDING_NONE;
                break;
            default:
                std::cout << "invalid padding " << usrEntry << std::endl;
        }
    } else if (restriction == 2) {
        getMultipleChoiceNumberFromUsr(
            "Enter padding, comma separated (1 - None, 2- PKCS7): ", 1, 2, selection);

        for (uint32_t x = 0; x < selection.size(); x++) {
            switch (selection.at(x)) {
                case 1:
                    padding |= telux::sec::Padding::PADDING_NONE;
                    break;
                case 2:
                    padding |= telux::sec::Padding::PADDING_PKCS7;
                    break;
                default:
                    std::cout << "invalid padding " << selection.at(x) << std::endl;
            }
        }
    } else if (restriction == 3) {
        getMultipleChoiceNumberFromUsr(
            "Enter padding, comma separated (1 - None, 2- RSA-PSS, 3 - RSA-PKCS1-1-5-SIGN): ",
            1, 3, selection);

        for (uint32_t x = 0; x < selection.size(); x++) {
            switch (selection.at(x)) {
                case 1:
                    padding |= telux::sec::Padding::PADDING_NONE;
                    break;
                case 2:
                    padding |= telux::sec::Padding::PADDING_RSA_PSS;
                    break;
                case 3:
                    padding |= telux::sec::Padding::PADDING_RSA_PKCS1_1_5_SIGN;
                    break;
                default:
                    std::cout << "invalid padding " << selection.at(x) << std::endl;
            }
        }
    } else if (restriction == 4) {
        getMultipleChoiceNumberFromUsr(
            "Enter padding, comma separated (1 - None, 2- RSA-OAEP, 3 - RSA-PKCS1-1-5-ENC): ",
            1, 3, selection);

        for (uint32_t x = 0; x < selection.size(); x++) {
            switch (selection.at(x)) {
                case 1:
                    padding |= telux::sec::Padding::PADDING_NONE;
                    break;
                case 2:
                    padding |= telux::sec::Padding::PADDING_RSA_OAEP;
                    break;
                case 3:
                    padding |= telux::sec::Padding::PADDING_RSA_PKCS1_1_5_ENC;
                    break;
                default:
                    std::cout << "invalid padding " << selection.at(x) << std::endl;
            }
        }
    } else {
        std::string choiceStr = "Enter padding, comma separated ";
        choiceStr.append("(1 - None, 2 - RSA-OAEP, 3 - RSA-PSS, ");
        choiceStr.append("4 - RSA-PKCS1-1-5-ENC, 5 - RSA-PKCS1-1-5-SIGN, 6 - PKCS7): ");
        getChoiceNumberFromUsr(choiceStr, 1, 6, usrEntry, false);

        for (uint32_t x = 0; x < selection.size(); x++) {
            switch (selection.at(x)) {
                case 1:
                    padding |= telux::sec::Padding::PADDING_NONE;
                    break;
                case 2:
                    padding |= telux::sec::Padding::PADDING_RSA_OAEP;
                    break;
                case 3:
                    padding |= telux::sec::Padding::PADDING_RSA_PSS;
                    break;
                case 4:
                    padding |= telux::sec::Padding::PADDING_RSA_PKCS1_1_5_ENC;
                    break;
                case 5:
                    padding |= telux::sec::Padding::PADDING_RSA_PKCS1_1_5_SIGN;
                    break;
                case 6:
                    padding |= telux::sec::Padding::PADDING_PKCS7;
                    break;
                default:
                    std::cout << "invalid padding " << selection.at(x) << std::endl;
            }
        }
    }
}

void CryptoConsoleApp::getBlockModeFromUser(telux::sec::BlockModeTypes& blockMode) {

    std::vector<uint32_t> selection;

    getMultipleChoiceNumberFromUsr(
        "Enter block mode, comma separated (1 - ECB, 2 - CBC, 3 - CTR, 4 - GCM): ",
        1, 4, selection);

    for (uint32_t x = 0; x < selection.size(); x++) {
        switch (selection.at(x)) {
            case 1:
                blockMode |= telux::sec::BlockMode::BLOCK_MODE_ECB;
                break;
            case 2:
                blockMode |= telux::sec::BlockMode::BLOCK_MODE_CBC;
                break;
            case 3:
                blockMode |= telux::sec::BlockMode::BLOCK_MODE_CTR;
                break;
            case 4:
                blockMode |= telux::sec::BlockMode::BLOCK_MODE_GCM;
                break;
            default:
                std::cout << "invalid block mode " << selection.at(x) << std::endl;
        }
    }
}

void CryptoConsoleApp::getCallerNoncePresentFromUser(bool& callerNoncePresent) {

    std::string usrInput = "";
    size_t choiceLength;

    while(1) {
        std::cout << "Caller nonce will be given (yes/no): ";
        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        choiceLength = usrInput.length();
        for (size_t x = 0; x < choiceLength; x++) {
            usrInput[x] = tolower(usrInput[x]);
        }

        if (!usrInput.compare("no")) {
            callerNoncePresent = false;
            return;
        } else if (!usrInput.compare("yes")) {
            callerNoncePresent = true;
            return;
        } else {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }
    }
}

void CryptoConsoleApp::getKeyFormatFromUser(telux::sec::KeyFormat& keyFmt) {

    uint32_t usrEntry;

    getChoiceNumberFromUsr(
        "Enter key format (1 - X509, 2 - PKCS8, 3 - Raw): ", 1, 3, usrEntry, false);

    switch (usrEntry) {
        case 1:
            keyFmt = telux::sec::KeyFormat::KEY_FORMAT_X509;
            break;
        case 2:
            keyFmt = telux::sec::KeyFormat::KEY_FORMAT_PKCS8;
            break;
        case 3:
            keyFmt = telux::sec::KeyFormat::KEY_FORMAT_RAW;
            break;
        default:
            std::cout << "invalid key format " << usrEntry << std::endl;
    }
}

void CryptoConsoleApp::getInitVectorFromUser(std::vector<uint8_t>& iv, uint32_t length) {

    std::string usrInput = "";
    size_t choiceLength;

    while(1) {
        std::cout << "supply init vector (yes/no): ";
        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        choiceLength = usrInput.length();
        for (size_t x = 0; x < choiceLength; x++) {
            usrInput[x] = tolower(usrInput[x]);
        }

        if (!usrInput.compare("no")) {
            iv.resize(0);
            return;
        } else if (!usrInput.compare("yes")) {
            getHexStringAsByteArrayFromUsr("Enter init vector (nonce as hex string): ", iv, length);
            return;
        } else {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }
    }
}

void CryptoConsoleApp::getAssociatedDataFromUser(std::vector<uint8_t>& associatedData) {

    std::string usrInput = "";
    size_t choiceLength;

    while(1) {
        std::cout << "supply associated data (yes/no): ";
        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        choiceLength = usrInput.length();
        for (size_t x = 0; x < choiceLength; x++) {
            usrInput[x] = tolower(usrInput[x]);
        }

        if (!usrInput.compare("no")) {
            associatedData.resize(0);
            return;
        } else if (!usrInput.compare("yes")) {
            getHexStringAsByteArrayFromUsr("Enter associated data (as hex string): ", associatedData, 0);
            return;
        } else {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }
    }
}

void CryptoConsoleApp::getUniqueDataFromUser(std::vector<uint8_t>& uniqueData) {

    std::string usrInput = "";
    size_t choiceLength;

    while(1) {
        std::cout << "set unique data (yes/no): ";
        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        choiceLength = usrInput.length();
        for (size_t x = 0; x < choiceLength; x++) {
            usrInput[x] = tolower(usrInput[x]);
        }

        if (!usrInput.compare("no")) {
            uniqueData.resize(0);
            return;
        } else if (!usrInput.compare("yes")) {
            getHexStringAsByteArrayFromUsr("Enter unique data (as hex string): ", uniqueData, 0);
            return;
        } else {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }
    }
}

void CryptoConsoleApp::getKeySizeFromUser(uint32_t& keySize) {

    getChoiceNumberFromUsr("Enter key size: ", 64, 2048, keySize, false);
}

void CryptoConsoleApp::getPublicExponentFromUser(uint32_t& publicExponent) {

    getChoiceNumberFromUsr("Enter public exponent (3 or 65537): ",
        3, 65537, publicExponent, false);
}

void CryptoConsoleApp::getMinMacLengthFromUser(uint32_t& minMacLength, uint32_t maxVal) {

    getChoiceNumberFromUsr("Enter minimum MAC length: ", 64, maxVal, minMacLength, true);
}

void CryptoConsoleApp::getMacLengthFromUser(uint32_t& macLength, uint32_t maxVal) {

    getChoiceNumberFromUsr("Enter MAC length: ", 64, maxVal, macLength, true);
}

void CryptoConsoleApp::getKeyDataFromUser(std::vector<uint8_t>& keyData) {

    getHexStringAsByteArrayFromUsr("Enter key data (as hex string): ", keyData, 0);
}

void CryptoConsoleApp::getKeyBlobFromUser(std::vector<uint8_t>& keyBlob) {

    getHexStringAsByteArrayFromUsr("Enter key blob (as hex string): ", keyBlob, 0);
}

void CryptoConsoleApp::getPlainTextFromUser(std::vector<uint8_t>& plainText) {

    getHexStringAsByteArrayFromUsr("Enter plain text (as hex string): ", plainText, 0);
}

void CryptoConsoleApp::getSignatureFromUser(std::vector<uint8_t>& signature) {

    getHexStringAsByteArrayFromUsr("Enter signature (as hex string): ", signature, 0);
}

void CryptoConsoleApp::getEncryptedTextFromUser(std::vector<uint8_t>& encText) {

    getHexStringAsByteArrayFromUsr("Enter encrypted text: ", encText, 0);
}

void CryptoConsoleApp::generateKey() {

    Request request{};
    std::shared_ptr<std::string> keyBlobFile;

    getAlgorithmFromUser(request.algo, 3);

    switch (request.algo) {
        case telux::sec::Algorithm::ALGORITHM_RSA:
            getOperationFromUser(request.operation, 3);
            getKeySizeFromUser(request.keySize);
            getPublicExponentFromUser(request.publicExponent);
            getDigestFromUser(request.digest, 2);
            if (((request.operation & telux::sec::CryptoOperation::CRYPTO_OP_SIGN)
                    == telux::sec::CryptoOperation::CRYPTO_OP_SIGN) &&
                ((request.operation & telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT)
                    == telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT)) {
                getPaddingFromUser(request.padding, 5);
            } else if ((request.operation & telux::sec::CryptoOperation::CRYPTO_OP_SIGN)
                    == telux::sec::CryptoOperation::CRYPTO_OP_SIGN) {
                getPaddingFromUser(request.padding, 3);
            } else if ((request.operation & telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT)
                    == telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT) {
                getPaddingFromUser(request.padding, 4);
            } else {
            }
            break;
        case telux::sec::Algorithm::ALGORITHM_EC:
            getOperationFromUser(request.operation, 1);
            getDigestFromUser(request.digest, 2);
            getKeySizeFromUser(request.keySize);
            break;
        case telux::sec::Algorithm::ALGORITHM_AES:
            getOperationFromUser(request.operation, 2);
            getKeySizeFromUser(request.keySize);
            getBlockModeFromUser(request.blockMode);
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_GCM)
                    == telux::sec::BlockMode::BLOCK_MODE_GCM) {
                getMinMacLengthFromUser(request.minMacLength, 128);
            }
            if (((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_ECB)
                    == telux::sec::BlockMode::BLOCK_MODE_ECB) ||
                ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_CBC)
                    == telux::sec::BlockMode::BLOCK_MODE_CBC)) {
                getPaddingFromUser(request.padding, 2);
            } else if (((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_GCM)
                    == telux::sec::BlockMode::BLOCK_MODE_GCM) ||
                ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_CTR)
                    == telux::sec::BlockMode::BLOCK_MODE_CTR)) {
                getPaddingFromUser(request.padding, 1);
            } else {
            }
            if (request.blockMode != telux::sec::BlockMode::BLOCK_MODE_ECB) {
                getCallerNoncePresentFromUser(request.callerNoncePresent);
            }
            break;
        case telux::sec::Algorithm::ALGORITHM_HMAC:
            getOperationFromUser(request.operation, 1);
            getKeySizeFromUser(request.keySize);
            getDigestFromUser(request.digest, 1);
            getMinMacLengthFromUser(request.minMacLength, 256);
            break;
        default:
            std::cout << "invalid algorithm " << static_cast<int>(request.algo) << std::endl;
            return;
    }

    getUniqueDataFromUser(request.uniqueData);
    getAbsoluteFilePathFromUser("Save key blob on file (yes/no) : ", keyBlobFile);

    cmdProcessor_->generateKey(request, keyBlobFile);
    printf("\n");
    fflush(stdout);
}

void CryptoConsoleApp::signData() {

    Request request{};
    std::shared_ptr<std::string> keyBlobFile;
    std::shared_ptr<std::string> plainTxtFile;
    std::shared_ptr<std::string> signatureFile;

    getAlgorithmFromUser(request.algo, 1);

    switch (request.algo) {
        case telux::sec::Algorithm::ALGORITHM_RSA:
            getDigestFromUser(request.digest, 2);
            getPaddingFromUser(request.padding, 3);
            break;
        case telux::sec::Algorithm::ALGORITHM_EC:
            getDigestFromUser(request.digest, 2);
            break;
        case telux::sec::Algorithm::ALGORITHM_HMAC:
            getDigestFromUser(request.digest, 1);
            getMacLengthFromUser(request.macLength, 512);
            break;
        default:
            std::cout << "invalid algorithm " << static_cast<int>(request.algo) << std::endl;
            return;
    }

    getAbsoluteFilePathFromUser("Read key blob from file (yes/no) : ", keyBlobFile);
    if (!keyBlobFile) {
        getKeyBlobFromUser(request.textA);
    }

    getAbsoluteFilePathFromUser("Read data to sign from file (yes/no) : ", plainTxtFile);
    if (!plainTxtFile) {
        getPlainTextFromUser(request.textB);
    }

    getAbsoluteFilePathFromUser("Save signature in file (yes/no) : ", signatureFile);

    cmdProcessor_->signData(request, keyBlobFile, plainTxtFile, signatureFile);
    printf("\n");
    fflush(stdout);
}

void CryptoConsoleApp::verifySignature() {

    Request request{};
    std::shared_ptr<std::string> keyBlobFile;
    std::shared_ptr<std::string> plainTxtFile;
    std::shared_ptr<std::string> signatureFile;

    getAlgorithmFromUser(request.algo, 1);

    switch (request.algo) {
        case telux::sec::Algorithm::ALGORITHM_RSA:
            getDigestFromUser(request.digest, 2);
            getPaddingFromUser(request.padding, 3);
            break;
        case telux::sec::Algorithm::ALGORITHM_EC:
            getDigestFromUser(request.digest, 2);
            break;
        case telux::sec::Algorithm::ALGORITHM_HMAC:
            getDigestFromUser(request.digest, 1);
            break;
        default:
            std::cout << "invalid algorithm " << static_cast<int>(request.algo) << std::endl;
            return;
    }

    getAbsoluteFilePathFromUser("Read key blob from file (yes/no) : ", keyBlobFile);
    if (!keyBlobFile) {
        getKeyBlobFromUser(request.textA);
    }

    getAbsoluteFilePathFromUser("Read signed data from file (yes/no) : ", plainTxtFile);
    if (!plainTxtFile) {
        getPlainTextFromUser(request.textB);
    }

    getAbsoluteFilePathFromUser("Read signature from file (yes/no) : ", signatureFile);
    if (!signatureFile) {
        getSignatureFromUser(request.textC);
    }

    cmdProcessor_->verifySignature(request, keyBlobFile, plainTxtFile, signatureFile);
    printf("\n");
    fflush(stdout);
}

void CryptoConsoleApp::encryptData() {

    Request request{};
    std::shared_ptr<std::string> keyBlobFile;
    std::shared_ptr<std::string> plainTxtFile;
    std::shared_ptr<std::string> encTxtFile;

    getAlgorithmFromUser(request.algo, 2);

    getAbsoluteFilePathFromUser("Read key blob from file (yes/no) : ", keyBlobFile);
    if (!keyBlobFile) {
        getKeyBlobFromUser(request.textA);
    }

    getAbsoluteFilePathFromUser("Read data to encrypt from file (yes/no) : ", plainTxtFile);
    if (!plainTxtFile) {
        getPlainTextFromUser(request.textB);
    }

    getAbsoluteFilePathFromUser("Save encrypted data in file (yes/no) : ", encTxtFile);

    switch (request.algo) {
        case telux::sec::Algorithm::ALGORITHM_RSA:
            getDigestFromUser(request.digest, 2);
            getPaddingFromUser(request.padding, 4);
            break;
        case telux::sec::Algorithm::ALGORITHM_AES:
            getBlockModeFromUser(request.blockMode);
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_ECB)
                    == telux::sec::BlockMode::BLOCK_MODE_ECB) {
                getPaddingFromUser(request.padding, 2);
            }
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_CBC)
                    == telux::sec::BlockMode::BLOCK_MODE_CBC) {
                getPaddingFromUser(request.padding, 2);
                getInitVectorFromUser(request.initVector, 16);
            }
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_CTR)
                    == telux::sec::BlockMode::BLOCK_MODE_CTR) {
                getPaddingFromUser(request.padding, 1);
                getInitVectorFromUser(request.initVector, 16);
            }
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_GCM)
                    == telux::sec::BlockMode::BLOCK_MODE_GCM) {
                getPaddingFromUser(request.padding, 1);
                getInitVectorFromUser(request.initVector, 12);
                getMacLengthFromUser(request.macLength, 128);
                getAssociatedDataFromUser(request.associatedData);
            }
            break;
        default:
            std::cout << "invalid algorithm " << static_cast<int>(request.algo) << std::endl;
            return;
    }

    cmdProcessor_->encryptData(request, keyBlobFile, plainTxtFile, encTxtFile);
    printf("\n");
    fflush(stdout);
}

void CryptoConsoleApp::decryptData() {

    Request request{};
    std::shared_ptr<std::string> keyBlobFile;
    std::shared_ptr<std::string> plainTxtFile;
    std::shared_ptr<std::string> encTxtFile;

    getAlgorithmFromUser(request.algo, 2);

    getAbsoluteFilePathFromUser("Read key blob from file (yes/no) : ", keyBlobFile);
    if (!keyBlobFile) {
        getKeyBlobFromUser(request.textA);
    }

    getAbsoluteFilePathFromUser("Read encrypted data from file (yes/no) : ", encTxtFile);
    if (!encTxtFile) {
        getEncryptedTextFromUser(request.textC);
    }

    getAbsoluteFilePathFromUser("Save decrypted data in file (yes/no) : ", plainTxtFile);

    switch (request.algo) {
        case telux::sec::Algorithm::ALGORITHM_RSA:
            getDigestFromUser(request.digest, 2);
            getPaddingFromUser(request.padding, 4);
            break;
        case telux::sec::Algorithm::ALGORITHM_AES:
            getBlockModeFromUser(request.blockMode);
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_ECB)
                    == telux::sec::BlockMode::BLOCK_MODE_ECB) {
                getPaddingFromUser(request.padding, 2);
            }
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_CBC)
                    == telux::sec::BlockMode::BLOCK_MODE_CBC) {
                getPaddingFromUser(request.padding, 2);
                getInitVectorFromUser(request.initVector, 16);
            }
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_CTR)
                    == telux::sec::BlockMode::BLOCK_MODE_CTR) {
                getPaddingFromUser(request.padding, 1);
                getInitVectorFromUser(request.initVector, 16);
            }
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_GCM)
                    == telux::sec::BlockMode::BLOCK_MODE_GCM) {
                getPaddingFromUser(request.padding, 1);
                getInitVectorFromUser(request.initVector, 12);
                getMacLengthFromUser(request.macLength, 128);
                getAssociatedDataFromUser(request.associatedData);
            }
            break;
        default:
            std::cout << "invalid algorithm " << static_cast<int>(request.algo) << std::endl;
            return;
    }

    cmdProcessor_->decryptData(request, keyBlobFile, encTxtFile, plainTxtFile);
    printf("\n");
    fflush(stdout);
}

void CryptoConsoleApp::importKey() {

    Request request{};
    std::shared_ptr<std::string> keyBlobFile;
    std::shared_ptr<std::string> keyDataFile;

    getAlgorithmFromUser(request.algo, 3);

    switch (request.algo) {
        case telux::sec::Algorithm::ALGORITHM_RSA:
            getOperationFromUser(request.operation, 3);
            getDigestFromUser(request.digest, 2);
            if ((request.operation & telux::sec::CryptoOperation::CRYPTO_OP_SIGN)
                    == telux::sec::CryptoOperation::CRYPTO_OP_SIGN) {
                getPaddingFromUser(request.padding, 3);
            }
            if ((request.operation & telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT)
                    == telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT) {
                getPaddingFromUser(request.padding, 4);
            }
            if (((request.operation & telux::sec::CryptoOperation::CRYPTO_OP_SIGN)
                    == telux::sec::CryptoOperation::CRYPTO_OP_SIGN) &&
                ((request.operation & telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT)
                    == telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT)) {
                getPaddingFromUser(request.padding, 5);
            }
            getPublicExponentFromUser(request.publicExponent);
            break;
        case telux::sec::Algorithm::ALGORITHM_EC:
            getOperationFromUser(request.operation, 1);
            getDigestFromUser(request.digest, 2);
            break;
        case telux::sec::Algorithm::ALGORITHM_AES:
            getOperationFromUser(request.operation, 2);
            getBlockModeFromUser(request.blockMode);
            if (((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_ECB)
                    == telux::sec::BlockMode::BLOCK_MODE_ECB) ||
                ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_CBC)
                    == telux::sec::BlockMode::BLOCK_MODE_CBC)) {
                getPaddingFromUser(request.padding, 2);
            } else if (((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_GCM)
                    == telux::sec::BlockMode::BLOCK_MODE_GCM) ||
                ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_CTR)
                    == telux::sec::BlockMode::BLOCK_MODE_CTR)) {
                getPaddingFromUser(request.padding, 1);
            } else {
            }
            if ((request.blockMode & telux::sec::BlockMode::BLOCK_MODE_GCM)
                    == telux::sec::BlockMode::BLOCK_MODE_GCM) {
                getMinMacLengthFromUser(request.minMacLength, 128);
            }
            if (request.blockMode != telux::sec::BlockMode::BLOCK_MODE_ECB) {
                getCallerNoncePresentFromUser(request.callerNoncePresent);
            }
            break;
        case telux::sec::Algorithm::ALGORITHM_HMAC:
            getOperationFromUser(request.operation, 1);
            getDigestFromUser(request.digest, 1);
            getMinMacLengthFromUser(request.minMacLength, 256);
            break;
        default:
            std::cout << "invalid algorithm " << static_cast<int>(request.algo) << std::endl;
            return;
    }

    getUniqueDataFromUser(request.uniqueData);
    getKeyFormatFromUser(request.keyFmt);

    getAbsoluteFilePathFromUser("Read key data from file (yes/no) : ", keyDataFile);
    if (!keyDataFile) {
        getKeyDataFromUser(request.textB);
    }

    getAbsoluteFilePathFromUser("Save keyblob in file (yes/no) : ", keyBlobFile);

    cmdProcessor_->importKey(request, keyDataFile, keyBlobFile);
    printf("\n");
    fflush(stdout);
}

void CryptoConsoleApp::exportKey() {

    Request request{};
    std::shared_ptr<std::string> keyBlobFile;
    std::shared_ptr<std::string> expDataFile;

    getKeyFormatFromUser(request.keyFmt);

    getAbsoluteFilePathFromUser("Read old key blob from file (yes/no) : ", keyBlobFile);
    if (!keyBlobFile) {
        getKeyBlobFromUser(request.textA);
    }

    getAbsoluteFilePathFromUser("Save exported data in file (yes/no) : ", expDataFile);

    cmdProcessor_->exportKey(request, keyBlobFile, expDataFile);
    printf("\n");
    fflush(stdout);
}

void CryptoConsoleApp::upgradeKey() {

    Request request{};
    std::shared_ptr<std::string> keyBlobFileOld;
    std::shared_ptr<std::string> keyBlobFileNew;

    getAbsoluteFilePathFromUser("Read old key blob from file (yes/no) : ", keyBlobFileOld);
    if (!keyBlobFileOld) {
        getKeyBlobFromUser(request.textA);
    }

    getAbsoluteFilePathFromUser("Save new key blob in file (yes/no) : ", keyBlobFileNew);

    getUniqueDataFromUser(request.uniqueData);

    cmdProcessor_->upgradeKey(request, keyBlobFileOld, keyBlobFileNew);
    printf("\n");
    fflush(stdout);
}

/*
 *  Prepare menu and display on console.
 */
void CryptoConsoleApp::init() {

    try {
        cmdProcessor_ = std::make_shared<CommandProcessor>();
    } catch (const std::exception& e) {
         std::cout << "can't create CommandProcessor" << std::endl;
        return;
    }

    int ret = cmdProcessor_->init();
    if (ret < 0) {
        return;
    }

    std::shared_ptr<ConsoleAppCommand> keyGenCmd = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "Generate key", {},
        std::bind(&CryptoConsoleApp::generateKey, this)));

    std::shared_ptr<ConsoleAppCommand> signCmd = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Sign data", {},
        std::bind(&CryptoConsoleApp::signData, this)));

    std::shared_ptr<ConsoleAppCommand> verifyCmd = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("3", "Verify signature", {},
        std::bind(&CryptoConsoleApp::verifySignature, this)));

    std::shared_ptr<ConsoleAppCommand> encryptCmd = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("4", "Encrypt data", {},
        std::bind(&CryptoConsoleApp::encryptData, this)));

    std::shared_ptr<ConsoleAppCommand> decryptCmd = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("5", "Decrypt data", {},
        std::bind(&CryptoConsoleApp::decryptData, this)));

    std::shared_ptr<ConsoleAppCommand> importCmd = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("6", "Import key", {},
        std::bind(&CryptoConsoleApp::importKey, this)));

    std::shared_ptr<ConsoleAppCommand> exportCmd = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("7", "Export key", {},
        std::bind(&CryptoConsoleApp::exportKey, this)));

    std::shared_ptr<ConsoleAppCommand> upgradeCmd = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("8", "Upgrade key", {},
        std::bind(&CryptoConsoleApp::upgradeKey, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainCmds = {
        keyGenCmd, signCmd, verifyCmd, encryptCmd, decryptCmd,
        importCmd, exportCmd, upgradeCmd };

    ConsoleApp::addCommands(mainCmds);
    ConsoleApp::displayMenu();
}

int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();

    std::string sdkReleaseName = telux::common::Version::getReleaseName();

    std::string appName = "Crypto console app - SDK v"
                          + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch) + "\n"
                          + "Release name: " + sdkReleaseName;

    auto cryptApp = std::make_shared<CryptoConsoleApp>(appName, "crpto> ");

    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};

    int ret = Utils::setSupplementaryGroups(supplementaryGrps);
    if (ret < 0) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    cryptApp->init();

    return cryptApp->mainLoop();
}
