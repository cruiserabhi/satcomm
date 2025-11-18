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


#include <chrono>
#include <thread>
/*
 * Steps to register listener for load and capacity updates are:
 * 1. Get SecurityFactory instance.
 * 2. Get an ICAControlManager instance from SecurityFactory.
 * 3. Define listener that implements ICAControlManagerListener.
 * 4. Register listener using registerListener().
 * 5. Start monitoring load by defining parameters and calling startMonitoring().
 * 6. When use-case is complete, stop monitoring using stopMonitoring().
 * 7. Finally, release listener using deRegisterListener().
 */

#include <iostream>

#include <telux/sec/SecurityFactory.hpp>

/* Step - 3 */
class StatsListener : public telux::sec::ICAControlManagerListener {

    void onCapacityUpdate(telux::sec::CACapacity newCapacity) {
        std::cout << "sm2     : " << newCapacity.sm2 << std::endl;
        std::cout << "nist256 : " << newCapacity.nist256 << std::endl;
        std::cout << "nist384 : " << newCapacity.nist384 << std::endl;
        std::cout << "bp256   : " << newCapacity.bp256 << std::endl;
        std::cout << "bp384   : " << newCapacity.bp384 << std::endl;
    }

    void onLoadUpdate(telux::sec::CALoad currentLoad) {
        std::cout << "sm2     : " << currentLoad.sm2 << std::endl;
        std::cout << "nist256 : " << currentLoad.nist256 << std::endl;
        std::cout << "nist384 : " << currentLoad.nist384 << std::endl;
        std::cout << "bp256   : " << currentLoad.bp256 << std::endl;
        std::cout << "bp384   : " << currentLoad.bp384 << std::endl;
    }
};

int main(int argc, char **argv) {

    telux::common::ErrorCode ec;
    std::shared_ptr<telux::sec::ICAControlManager> caCtrlMgr;
    std::shared_ptr<StatsListener> statsListener;
    telux::sec::LoadConfig loadConfig{};

    /* Step - 1 */
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    /* Step - 2 */
    caCtrlMgr = secFact.getCAControlManager(ec);
    if (!caCtrlMgr) {
        std::cout <<
         "can't get ICAControlManager, err " << static_cast<int>(ec) << std::endl;
        return -ENOMEM;
    }

    try {
        statsListener = std::make_shared<StatsListener>();
    } catch (const std::exception& e) {
        std::cout << "can't create StatsListener" << std::endl;
        return -ENOMEM;
    }

    /* Step - 4 */
    ec = caCtrlMgr->registerListener(statsListener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't register, " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    /* Step - 5 */
    loadConfig.calculationInterval = 100; /* 100 milliseconds */
    ec = caCtrlMgr->startMonitoring(loadConfig);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't start monitoring, " << static_cast<int>(ec) << std::endl;
        caCtrlMgr->deRegisterListener(statsListener);
        return -EIO;
    }

    /* Let load become available, listener invoked, before we exit the application */
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    /* Step - 6 */
    ec = caCtrlMgr->stopMonitoring();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't stop monitoring, " << static_cast<int>(ec) << std::endl;
        caCtrlMgr->deRegisterListener(statsListener);
        return -EIO;
    }

    /* Step - 7 */
    caCtrlMgr->deRegisterListener(statsListener);

    return 0;
}
