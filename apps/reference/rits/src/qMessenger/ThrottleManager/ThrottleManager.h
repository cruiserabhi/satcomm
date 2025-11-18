/**
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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

/**
  * @file: ThrottleManager.hpp
  *
  * @brief: Application that handles and abstracts the communication to Throttle Manager
  *
  */
#include <iostream>
#include <string>
#include <future>
#include <unistd.h>

#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xThrottleManager.hpp>

using namespace telux::cv2x;
static std::promise<telux::common::ErrorCode> gCallbackPromise;
static int tmFilterRate = 0;
static void cv2xsetVerificationLoadCallback(telux::common::ErrorCode error);

class Cv2xTmListener : public ICv2xThrottleManagerListener {

  static std::shared_ptr<Cv2xTmListener> instance;
  void onFilterRateAdjustment(int rate);
  int tmVerbosity=0;

  public:
  /*
   * Default Constructor. Uses default verbosity.
   */
  Cv2xTmListener();

  /**
    * Constructor that creates a Cv2xTmListener Object
    * @param tmVerbosity - Verbosity set for debug prints.
    */
  Cv2xTmListener(int tmVerbosity);

  //Set verification load to throttle manager.
  int setLoad(int load);

  //Get the filter rate from throttle manager.
  int getFilterRate();

  bool cv2xTmStatusUpdated = false;
  telux::common::ServiceStatus cv2xTmStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
  std::condition_variable cv;
  std::mutex mtx;
  Cv2xFactory& cv2xFactory = Cv2xFactory::getInstance();
  std::shared_ptr<ICv2xThrottleManager> cv2xThrottleManager;
};

