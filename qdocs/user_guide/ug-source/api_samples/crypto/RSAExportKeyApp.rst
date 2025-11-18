.. _rsa_export_key_app:

Export a RSA key 
==========================================================================

This sample app demonstrates how to generate a RSA key and export it given standard format.

1. Get a SecurityFactory instance

.. code-block::

    auto &secFact = telux::sec::SecurityFactory::getInstance();


2. Get a CryptoManager instance

.. code-block::

    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr;

    cryptMgr = secFact.getCryptoManager(ec);
    if (!cryptMgr) {
        std::cout << "Can't allocate CryptoManager, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }


3. Define parameters using CryptoParamBuilder for the key

.. code-block::

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


4. Generate RSA key using generateKey() API

.. code-block::

    telux::common::ErrorCode ec;
    std::vector<uint8_t> kb;

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate RSA asym key, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }



5. Export key in given format in an array using exportKey() API

.. code-block::

    std::vector<uint8_t> keyData;

    ec = cryptMgr->exportKey(telux::sec::KeyFormat::KEY_FORMAT_X509, kb, keyData);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't export key, err: " <<
                static_cast<int>(ec) << std::endl;
        return -1;
    }

