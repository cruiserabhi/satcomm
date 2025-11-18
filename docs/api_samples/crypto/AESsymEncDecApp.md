Encrypt and decrypt using AES key {#aes_sym_enc_dec_app}
==========================================================================

This sample app demonstrates how to generate a AES key, encrypt and decrypt given data using this key.

### 1. Get a SecurityFactory instance

   ~~~~~~{.cpp}
    auto &secFact = telux::sec::SecurityFactory::getInstance();
   ~~~~~~

### 2. Get a CryptoManager instance

   ~~~~~~{.cpp}
    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr;

    cryptMgr = secFact.getCryptoManager(ec);
    if (!cryptMgr) {
        std::cout << "Can't allocate CryptoManager, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }
   ~~~~~~

### 3. Define parameters using CryptoParamBuilder for the key

   ~~~~~~{.cpp}
    std::shared_ptr<telux::sec::ICryptoParam> cp;

    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_AES)
            .setCryptoOperation(telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT |
                        telux::sec::CryptoOperation::CRYPTO_OP_DECRYPT)
            .setKeySize(128)
            .setBlockMode(telux::sec::BlockMode::BLOCK_MODE_CBC |
                        telux::sec::BlockMode::BLOCK_MODE_CTR)
            .setPadding(telux::sec::Padding::PADDING_PKCS7 |
                        telux::sec::Padding::PADDING_NONE)
            .setCallerNonce(true)
            .build();
   ~~~~~~

### 4. Generate AES key using generateKey() API

   ~~~~~~{.cpp}
    std::vector<uint8_t> kb;
    telux::common::ErrorCode ec;

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate AES sym key, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }
   ~~~~~~

### 5. Define data to be encrypted

   ~~~~~~{.cpp}
    std::vector<uint8_t> pt {'h', 'e', 'l', 'l', 'o'};
   ~~~~~~

### 6. Specify initialization vector for encryption/decryption

   ~~~~~~{.cpp}
    std::vector<uint8_t> initVector((128/8), 0x01);
   ~~~~~~

### 7. Define parameters using CryptoParamBuilder for encryption/decryption

   ~~~~~~{.cpp}
    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_AES)
            .setBlockMode(telux::sec::BlockMode::BLOCK_MODE_CBC)
            .setPadding(telux::sec::Padding::PADDING_PKCS7)
            .setInitVector(initVector)
            .build();
   ~~~~~~

### 8. Encrypt the required data using encryptData() API

   ~~~~~~{.cpp}
    std::vector<uint8_t> et;

    ec = cryptMgr->encryptData(cp, kb, pt, et);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't encrypt data, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }
   ~~~~~~

### 9. Decrypt data using decryptData() API

   ~~~~~~{.cpp}
    std::vector<uint8_t> dt;

    ec = cryptMgr->decryptData(cp, kb, et, dt);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't decrypt data, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }
   ~~~~~~
