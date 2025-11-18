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

/* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFactoryStub.hpp
 *
 *
 */

#ifndef SENSORFACTORYIMPL_HPP
#define SENSORFACTORYIMPL_HPP

#include <memory>
#include <vector>
#include <mutex>

#include <telux/sensor/SensorFactory.hpp>
#include "SensorFeatureManagerStub.hpp"
#include "SensorManagerStub.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/FactoryHelper.hpp"

namespace telux{
namespace sensor{

class SensorFactoryStub : public SensorFactory,
                          public telux::common::FactoryHelper {
   public:
      static SensorFactory &getInstance();
      std::shared_ptr<ISensorManager> getSensorManager(
          telux::common::InitResponseCb clientCallback = nullptr) override;
      std::shared_ptr<ISensorFeatureManager> getSensorFeatureManager(
          telux::common::InitResponseCb clientCallback = nullptr) override;

   private:
      SensorFactoryStub();
      ~SensorFactoryStub();

      /**
       * Helper method to notify all listeners the completion of initialization with the provided
       * status
       */
      void initCompleteNotifier(std::vector<telux::common::InitResponseCb> &initCallbacks_,
          telux::common::ServiceStatus status);
      std::weak_ptr<SensorFeatureManagerStub> sensorFeatureManager_;
      std::weak_ptr<ISensorManager> sensorManager_;
      std::vector<telux::common::InitResponseCb> smInitCallbacks_;   // Sensor manager callbacks
      std::vector<telux::common::InitResponseCb> sfmInitCallbacks_;  // Sensor feature manager
                                                                     // callbacks
      std::mutex factoryGuard_;
  };
}
}
#endif  // SENSORFACTORY_HPP