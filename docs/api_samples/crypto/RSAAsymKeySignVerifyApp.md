Sign and verify using RSA key {#rsa_asym_key_sign_verify_app}
==========================================================================

This sample app demonstrates how to generate a RSA key, sign given data using this key and verify data using this key.

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
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_RSA)
            .setCryptoOperation(telux::sec::CryptoOperation::CRYPTO_OP_SIGN |
                        telux::sec::CryptoOperation::CRYPTO_OP_VERIFY)
            .setKeySize(2048)
            .setPublicExponent(65537)
            .setDigest(telux::sec::Digest::DIGEST_SHA_2_256 |
                    telux::sec::Digest::DIGEST_SHA_2_512)
            .setPadding(telux::sec::Padding::PADDING_RSA_PSS |
                    telux::sec::Padding::PADDING_RSA_PKCS1_1_5_SIGN)
            .build();
   ~~~~~~

### 4. Generate RSA key using generateKey() API

   ~~~~~~{.cpp}
    telux::common::ErrorCode ec;
    std::vector<uint8_t> kb;

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate RSA asym key, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }
   ~~~~~~

### 5. Define data to be signed

   ~~~~~~{.cpp}
    std::vector<uint8_t> pt {'h', 'e', 'l', 'l', 'o'};
   ~~~~~~

###6. Define parameters using CryptoParamBuilder for signing/verifying data

   ~~~~~~{.cpp}
    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_RSA)
            .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
            .setPadding(telux::sec::Padding::PADDING_RSA_PKCS1_1_5_SIGN)
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
            std::cout << "Can't verify data, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }
   ~~~~~~
