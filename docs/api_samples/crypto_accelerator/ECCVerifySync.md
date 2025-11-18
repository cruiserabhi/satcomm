Verify ECDSA digest synchronously {#ecc_verify_sync}
====================================================

This sample app demonstrates how to verify ECDSA digest synchronously.
The steps are:

1. Get SecurityFactory instance.
2. Get ICryptoAcceleratorManager instance from SecurityFactory.
3. Define parameters for verification process.
4. Send parameters for verification (method will block here).
   When the digest is verified, get the result.

   ~~~~~~{.cpp}
#include <iostream>

#include <telux/sec/SecurityFactory.hpp>

/* Digest to verify */
uint8_t dig[] = {
    0x67, 0x45, 0x8b, 0x6b, 0xc6, 0x23, 0x7b, 0x32, 0x69, 0x98, 0x3c, 0x64,
    0x73, 0x48, 0x33, 0x66, 0x51, 0xdc, 0xb0, 0x74, 0xff, 0x5c, 0x49, 0x19,
    0x4a, 0x94, 0xe8, 0x2a, 0xec, 0x58, 0x55, 0x62 };

/* Public key */
uint8_t pubKeyX[] = {
    0x62, 0xd5, 0xe2, 0x2a, 0xff, 0x7a, 0x60, 0x27, 0xe9, 0x0a, 0xd1, 0x0e,
    0x01, 0xa1, 0x3c, 0x23, 0x01, 0xa5, 0x02, 0xa3, 0x79, 0xf9, 0x99, 0x0b,
    0xf3, 0x8e, 0xec, 0xb3, 0x15, 0x0a, 0xb2, 0x3b };
uint8_t pubKeyY[] ={
    0xa7, 0x2f, 0xaf, 0xeb, 0xbc, 0x72, 0xaf, 0xc2, 0x7c, 0x57, 0x82, 0x0e,
    0x9f, 0xef, 0xe2, 0xe9, 0xbd, 0x6c, 0x52, 0x29, 0x1d, 0x85, 0xa4, 0xdf,
    0xe1, 0xaf, 0x17, 0x14, 0xec, 0x00, 0x27, 0x90 };

/* Signature of digest */
uint8_t rSig[] = {
    0xfa, 0xc3, 0x51, 0xad, 0xe4, 0x4e, 0x7a, 0xf9, 0x52, 0xfd, 0x0a, 0x93,
    0x61, 0xc2, 0x8e, 0x32, 0x3c, 0x13, 0x45, 0xa6, 0x60, 0x6a, 0x1c, 0x85,
    0x1c, 0x73, 0x5c, 0x78, 0x0f, 0x16, 0xd4, 0x51 };
uint8_t sSig[] = {
    0x42, 0x82, 0x47, 0xd5, 0xab, 0xe4, 0xae, 0x3f, 0x42, 0xe8, 0x11, 0xac,
    0x04, 0x88, 0x73, 0xe4, 0x04, 0xa1, 0x8c, 0xa8, 0x80, 0x1b, 0x65, 0xdb,
    0x38, 0xb1, 0xb6, 0x10, 0x12, 0x6a, 0x78, 0xd2 };

int main(int argc, char **argv) {

    uint8_t *data;
    telux::common::ErrorCode ec;
    std::vector<uint8_t> resultData;
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;

    uint32_t uniqueId;
    telux::sec::ECCCurve curve;
    telux::sec::DataDigest digest{};
    telux::sec::ECCPoint publicKey{};
    telux::sec::Signature signature{};
    telux::sec::RequestPriority priority;

    /* Step - 1 */
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    /* Step - 2 */
    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec, telux::sec::Mode::MODE_SYNC);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err " << static_cast<int>(ec) << std::endl;
        return -ENOMEM;
    }

    /* Step - 3 */
    uniqueId = 1;
    curve = telux::sec::ECCCurve::CURVE_NISTP256;
    priority = telux::sec::RequestPriority::REQ_PRIORITY_NORMAL;
    digest.digest = dig;
    digest.digestLength = sizeof(dig)/sizeof(dig[0]);
    publicKey.x = pubKeyX;
    publicKey.xLength = sizeof(pubKeyX)/sizeof(pubKeyX[0]);
    publicKey.y = pubKeyY;
    publicKey.yLength = sizeof(pubKeyY)/sizeof(pubKeyY[0]);
    signature.rSignature = rSig;
    signature.sSignature = sSig;
    signature.rsLength = sizeof(rSig)/sizeof(rSig[0]);

    /* Step - 4 */
    ec = cryptAccelMgr->eccVerifyDigest(digest, publicKey, signature,
            curve, uniqueId, priority, resultData);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "verification failed, " << static_cast<int>(ec) << std::endl;
        cryptAccelMgr = nullptr;
        return -1;
    }
    std::cout << "verification passed" << std::endl;

    data = resultData.data();
    for (uint32_t x=0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
        printf("%02x", data[x] & 0xffU);
        if ((x == 31) || (x == 63)) {
            printf("\n");
        }
    }
    printf("\n");

    return 0;
}
   ~~~~~~