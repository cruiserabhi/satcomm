/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * file       DgnssManagerStub.hpp
 * brief      DgnssManagerStub provides APIs simulating injection of correction
 *            data location data correction.
 *
 */

#ifndef DGNSSMANAGERSTUB_HPP
#define DGNSSMANAGERSTUB_HPP

#include "telux/loc/DgnssManager.hpp"
#include "common/AsyncTaskQueue.hpp"

namespace telux {

namespace loc {

/**
 * DgnssManagerStub provides an interface simulating injection of
 * RTCM data, register event listener reported by
 * cdfw(correction data framework).
 *
 */
class DgnssManagerStub : public IDgnssManager {
public:

/**
 * Checks the status of location Dgnss subsystems and returns the result.
 *
 * returns True if location Dgnss subsystem is ready for service otherwise false.
 *
 */
    bool isSubsystemReady() override;

/**
 * This status indicates whether the object is in a usable state.
 *
 * returns SERVICE_AVAILABLE    -  If location manager is ready for service.
 *          SERVICE_UNAVAILABLE  -  If location manager is temporarily unavailable.
 *          SERVICE_FAILED       -  If location manager encountered an irrecoverable failure.
 *
 */
    telux::common::ServiceStatus getServiceStatus() override;

/**
 * Wait for location Dgnss subsystem to be ready.
 *
 * returns  A future that caller can wait on to be notified when location
 *           Dgnss subsystem is ready.
 *
 */
    std::future<bool> onSubsystemReady() override;

/**
 * Register a listener for Dgnss injection status update.
 *
 * param [in] listener - Pointer of IDgnssStatusListener object that processes
 * the notification.
 *
 * returns Status of registerListener i.e success or suitable status code.
 *
 */
    telux::common::Status registerListener(std::weak_ptr<IDgnssStatusListener> listener) override;

/**
 * deRegister a listener for Dgnss injection status update.
 *
 * returns Status of registerListener i.e success or suitable status code.
 *
 */
    telux::common::Status deRegisterListener(void) override;

/**
 * Create a Dgnss injection source.
 * Only one source is permitted at any given time. If a new source is to be used, user must call
 * releaseSource() to release previous source before calling this function.
 *
 * param [in] format Dgnss injection data format.
 *
 * returns Success of suitable status code
 *
 */
    telux::common::Status createSource(DgnssDataFormat dataFormat) override;

/**
 * Release current Dgnss injection source (previously created by  createSource() call)
 * This function is to be called if it's determined that current injection data is not
 * suitable anymore, and a new source will be created and used as injection source.
 *
 * param none
 *
 * returns none
 *
 */
    telux::common::Status releaseSource(void) override;

/**
 * Inject correction data
 * This function is to be called when a source has been created, either through a explicit call to
 * createSource(), or after DgnssManagerStub object was instantiated through the factory method(The
 * factory method create a default source for DgnssManagerStub object).
 *
 * param [in] buffer buffer contains the data to be injected.
 * param [in] bufferSize size of the buffer.
 * returns success or suitable status code.
 *
 */
    telux::common::Status injectCorrectionData(const uint8_t* buffer, uint32_t bufferSize) override;

    DgnssManagerStub(DgnssDataFormat dataFormat);

    telux::common::Status init(telux::common::InitResponseCb callback);
/**
 * Destructor of DgnssManagerStub
 */
    virtual ~DgnssManagerStub();

private:
    DgnssDataFormat dataFormat_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    std::weak_ptr<IDgnssStatusListener> statusListener_;
    std::shared_ptr<std::string> dataSource_ = nullptr;
    std::mutex mutex_;
    std::condition_variable cv_;
    void initSync(telux::common::InitResponseCb callback);
    bool waitForInitialization();
};

} // end of namespace loc

} // end of namespace telux

#endif // DGNSSMANAGER_HPP
