/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * Steps to verify ECDSA digest and obtain result in listener are:
 * 1. Define listener that will receive verification result.
 * 2. Get SecurityFactory instance.
 * 3. Get ICryptoAcceleratorManager instance from SecurityFactory.
 * 4. Define parameters for verification process.
 * 5. Send parameters for verification.
 * 6. Receive result in the registered listener.
 */

#include <iostream>
#include <chrono>
#include <thread>

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

class ResultListener : public telux::sec::ICryptoAcceleratorListener {

    /* Step - 6 */
    void onVerificationResult(uint32_t uniqueId, telux::common::ErrorCode ec,
        std::vector<uint8_t> resultData) {

        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout <<
                "verification failed, err: " << static_cast<int>(ec) <<
                " uniqueId: " << uniqueId << std::endl;
            return;
        }

        std::cout << "verification passed, uniqueId: " << uniqueId << std::endl;

        uint8_t *data = resultData.data();
        for (uint32_t x = 0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
            printf("%02x", data[x] & 0xffU);
            if ((x == 31) || (x == 63)) {
                printf("\n");
            }
        }
        printf("\n");
    }

    void onCalculationResult(uint32_t uniqueId, telux::common::ErrorCode ec,
        std::vector<uint8_t> resultData) {
    }
};

int main(int argc, char **argv) {

    telux::common::ErrorCode ec;
    std::shared_ptr<ResultListener> resultListener;
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;

    uint32_t uniqueId;
    telux::sec::ECCCurve curve;
    telux::sec::DataDigest digest{};
    telux::sec::ECCPoint publicKey{};
    telux::sec::Signature signature{};
    telux::sec::RequestPriority priority;

    /* Step - 1 */
    resultListener = std::make_shared<ResultListener>();

    /* Step - 2 */
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    /* Step - 3 */
    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec,
        telux::sec::Mode::MODE_ASYNC_LISTENER, resultListener);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err " << static_cast<int>(ec) << std::endl;
        return -ENOMEM;
    }

    /* Step - 4 */
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

    /* Step - 5 */
    ec = cryptAccelMgr->eccPostDigestForVerification(digest, publicKey, signature,
            curve, uniqueId, priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, " << static_cast<int>(ec) << std::endl;
    }

    /* Let result become available, listener invoked, before we exit the application */
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    return 0;
}
