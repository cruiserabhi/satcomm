Sign and verify using EC key {#ec_asym_key_sign_verify_app}
==========================================================================

This sample app demonstrates how to generate a EC key, sign given data using this key and verify data using this key.

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
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_EC)
            .setCryptoOperation(telux::sec::CryptoOperation::CRYPTO_OP_SIGN |
                        telux::sec::CryptoOperation::CRYPTO_OP_VERIFY)
            .setKeySize(256)
            .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
            .build();
   ~~~~~~

### 4. Generate EC key using generateKey() API

   ~~~~~~{.cpp}
    std::vector<uint8_t> kb;
    telux::common::ErrorCode ec;

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate EC asym key, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }
   ~~~~~~

### 5. Define data to be signed

   ~~~~~~{.cpp}
    std::vector<uint8_t> pt {'h', 'e', 'l', 'l', 'o'};
   ~~~~~~

### 6. Define parameters using CryptoParamBuilder for signing/verifying data

   ~~~~~~{.cpp}
    std::shared_ptr<telux::sec::ICryptoParam> cp;

    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_EC)
            .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
            .build();
   ~~~~~~

### 7. Sign the required data using signData() API

   ~~~~~~{.cpp}
    std::vector<uint8_t> sg;

    ec = cryptMgr->signData(cp, kb, pt, sg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't sign data, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }
   ~~~~~~

### 8. Verify authenticity of data using verifyData() API

   ~~~~~~{.cpp}
    ec = cryptMgr->verifyData(cp, kb, pt, sg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        if (ec == telux::common::ErrorCode::VERIFICATION_FAILED)
            std::cout << "Invalid signature for given data!" << std::endl;
        else
            std::cout << "Can't verify data, err: " <<
                            static_cast<int>(ec) << std::endl;
        return -1;
    }
   ~~~~~~
