Calculate ECQV point asynchronously - poll mode {#ecqv_calc_async_poll}
====================================================

This sample app demonstrates how to calculate ECQV point and obtain result asynchronously.
The steps are:

1. Get SecurityFactory instance.
2. Get ICryptoAcceleratorManager instance from SecurityFactory.
3. Define parameters for calculation process.
4. Send parameters for calculation.
5. Obtain the result by polling (method will block here).
6. Parse the result to extract calculation result and ECC point.
7. Finally, release ICryptoAcceleratorManager to release resources.

   ~~~~~~{.cpp}
#include <iostream>

#include <telux/sec/SecurityFactory.hpp>

/* Scalar (hash construct) */
uint8_t scl[] = {
  0xd1, 0x07, 0x3b, 0x4e, 0xbf, 0x65, 0x0a, 0xfe, 0xff, 0x59, 0x7b, 0x1f,
  0x03, 0xe7, 0x51, 0xb4, 0x29, 0x6f, 0x6b, 0x3e, 0x12, 0xe4, 0xff, 0x31,
  0x61, 0xbb, 0x60, 0x5b, 0x0f, 0xa4, 0xc9, 0x39 };

/* Point to multiply (public key reconstruction value) */
uint8_t mulPointX[] = {
  0x79, 0xb3, 0x11, 0x42, 0xc1, 0xd8, 0x25, 0xcc, 0x17, 0xe5, 0xe0, 0xdd,
  0x75, 0xd1, 0xc2, 0x72, 0xb8, 0x7e, 0x7b, 0xd8, 0xe0, 0x21, 0x4a, 0xfc,
  0x32, 0x5d, 0xe3, 0xce, 0x83, 0x02, 0x7d, 0xa6 };
uint8_t mulPointY[] = {
  0xa5, 0x96, 0x93, 0x75, 0x7c, 0x9e, 0xb5, 0x91, 0xbc, 0xa6, 0x21, 0xbd,
  0xb7, 0x16, 0x03, 0xbc, 0x8f, 0xa6, 0xba, 0xc6, 0xd1, 0xde, 0x3d, 0xb0,
  0xf6, 0x8f, 0xb5, 0x7e, 0x93, 0x07, 0xa9, 0xa5 };

/* Point to add (CA public key) */
uint8_t addPointX[] = {
  0x5c, 0x48, 0x40, 0xb1, 0x67, 0xb6, 0xea, 0xb4, 0xc2, 0x79, 0x9b, 0xbe,
  0x32, 0x13, 0x7b, 0x4c, 0x68, 0xb5, 0xb6, 0x80, 0x11, 0x7b, 0x93, 0x4d,
  0x90, 0xce, 0x92, 0x1b, 0x1f, 0x94, 0x6d, 0xe9 };
uint8_t addPointY[] = {
  0x6f, 0xb1, 0x84, 0xe7, 0xcb, 0x35, 0xb2, 0x4a, 0x34, 0x5a, 0x7d, 0x40,
  0x29, 0x55, 0xa3, 0x0c, 0x5b, 0x7b, 0x59, 0x5f, 0x56, 0x98, 0xd7, 0x17,
  0xd6, 0x1c, 0x9d, 0x4c, 0x9f, 0x3c, 0xca, 0x40 };

/* Expected result of the calculation */
uint8_t outPointX[] = {
  0xa8, 0xfa, 0x30, 0x69, 0xb7, 0xf0, 0xe0, 0x8b, 0xe6, 0x31, 0x95, 0x8e,
  0xe4, 0x47, 0x2c, 0x7b, 0x0b, 0xf9, 0xa4, 0x68, 0x52, 0x96, 0xdc, 0x63,
  0x5c, 0x27, 0xc6, 0xd3, 0x4e, 0xc6, 0x2b, 0x9b };
uint8_t outPointY[] = {
  0x2d, 0x94, 0x35, 0xaa, 0xa6, 0x65, 0xec, 0xe0, 0x3f, 0x83, 0xad, 0x0a,
  0xa0, 0x41, 0x65, 0x4c, 0xe3, 0x43, 0x80, 0xc1, 0x35, 0x7e, 0xef, 0xc7,
  0x1a, 0xd8, 0x97, 0x80, 0x5b, 0x62, 0x74, 0xf3 };

int main(int argc, char **argv) {

    uint8_t *data;
    uint32_t numResultsRead = 0;
    telux::common::ErrorCode ec;
    std::vector<telux::sec::OperationResult> results(1);
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;

    uint32_t uniqueId;
    telux::sec::ECCCurve curve;
    telux::sec::ECCPoint multiplicandPoint{};
    telux::sec::ECCPoint addendPoint{};
    telux::sec::Scalar scalar{};
    telux::sec::RequestPriority priority;

    /* Step - 1 */
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    /* Step - 2 */
    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec, telux::sec::Mode::MODE_ASYNC_POLL);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err " << static_cast<int>(ec) << std::endl;
        return -ENOMEM;
    }

    /* Step - 3 */
    uniqueId = 1;
    curve = telux::sec::ECCCurve::CURVE_NISTP256;
    priority = telux::sec::RequestPriority::REQ_PRIORITY_NORMAL;
    scalar.scalar = scl;
    scalar.scalarLength = sizeof(scl)/sizeof(scl[0]);
    multiplicandPoint.x = mulPointX;
    multiplicandPoint.xLength = sizeof(mulPointX)/sizeof(mulPointX[0]);
    multiplicandPoint.y = mulPointY;
    multiplicandPoint.yLength = sizeof(mulPointY)/sizeof(mulPointY[0]);
    addendPoint.x = addPointX;
    addendPoint.xLength = sizeof(addPointX)/sizeof(addPointX[0]);
    addendPoint.y = addPointY;
    addendPoint.yLength = sizeof(addPointY)/sizeof(addPointY[0]);

    /* Step - 4 */
    ec = cryptAccelMgr->ecqvPostDataForMultiplyAndAdd(multiplicandPoint, addendPoint,
            scalar, curve, uniqueId, priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    /* Step - 5 */
    ec = cryptAccelMgr->getAsyncResults(results, 1, -1, numResultsRead);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't get result, " << static_cast<int>(ec) << std::endl;
        cryptAccelMgr = nullptr;
        return -EIO;
    }

    /* Step - 6 */
    std::cout << "uniqueId: " << telux::sec::ResultParser::getId(results[0]) << std::endl;
    std::cout << "operation type: " <<
        static_cast<int>(telux::sec::ResultParser::getOperationType(results[0])) << std::endl;
    std::cout << "error code: " <<
        static_cast<int>(telux::sec::ResultParser::getCAErrorCode(results[0])) << std::endl;

    data = telux::sec::ResultParser::getData(results[0]);
    for (uint32_t x=0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
        printf("%02x", data[x] & 0xffU);
        if ((x == 31) || (x == 63)) {
            printf("\n");
        }
    }
    printf("\n");

    /* Step - 7 */
    cryptAccelMgr = nullptr;

    return 0;
}
   ~~~~~~