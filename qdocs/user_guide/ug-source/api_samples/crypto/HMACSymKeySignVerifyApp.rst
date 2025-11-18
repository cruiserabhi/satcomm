.. _hmac_sym_key_sign_verify:

Sign and verify using HMAC key 
==========================================================================

This sample app demonstrates how to generate a HMAC key, sign given data using this key and verify data using this key.

1. Include headers files pertaining to security APIs

.. code-block::

    #include <telux/sec/CryptoParamBuilder.hpp>
    #include <telux/sec/CryptoManager.hpp>
    #include <telux/sec/SecurityFactory.hpp>


2. Get a SecurityFactory instance

.. code-block::

    auto &secFact = telux::sec::SecurityFactory::getInstance();


3. Get a CryptoManager instance

.. code-block::

    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr;

    cryptMgr = secFact.getCryptoManager(ec);
    if (!cryptMgr) {
        std::cout << "Can't allocate CryptoManager, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }


4. Define parameters using CryptoParamBuilder for the key

.. code-block::

    std::shared_ptr<telux::sec::ICryptoParam> cp;

    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_HMAC)
            .setCryptoOperation(telux::sec::CryptoOperation::CRYPTO_OP_SIGN |
                        telux::sec::CryptoOperation::CRYPTO_OP_VERIFY)
            .setKeySize(128)
            .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
            .setMinimumMacLength(64)
            .build();


5. Generate HMAC key using generateKey() API

.. code-block::

    std::vector<uint8_t> kb;
    telux::common::ErrorCode ec;

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate HMAC key, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }


6. Define data to be signed

.. code-block::

    std::vector<uint8_t> pt {'h', 'e', 'l', 'l', 'o'};


7. Define parameters using CryptoParamBuilder for signing/verifying data

.. code-block::

    cp = telux::sec::CryptoParamBuilder()
            .setAlgorithm(telux::sec::Algorithm::ALGORITHM_HMAC)
            .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
            .build();


8. Sign the required data using signData() API

.. code-block::

    std::vector<uint8_t> sg;

    ec = cryptMgr->signData(cp, kb, pt, sg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't sign data, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }


9. Verify authenticity of data using verifyData() API

.. code-block::

    ec = cryptMgr->verifyData(cp, kb, pt, sg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        if (ec == telux::common::ErrorCode::VERIFICATION_FAILED)
            std::cout << "Invalid signature for given data!" << std::endl;
        else
            std::cout << "Can't verify data, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

