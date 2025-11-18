/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SASQUISHCLIENT_HPP
#define SASQUISHCLIENT_HPP
#define NANOSECONDS_IN_MILLISEC 1000000
#include <telux/cv2x/prop/CongestionControlDefines.hpp>
#include <telux/cv2x/prop/CongestionControlManager.hpp>
#include <telux/cv2x/prop/V2xPropFactory.hpp>
#include <telux/common/CommonDefines.hpp>
#include "SasquishUtils.hpp"
#include <sys/timerfd.h>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <thread>
#include <stdio.h>      /* printf, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

using namespace std;
using namespace telux;
using namespace telux::cv2x::prop;
using namespace telux::common;

class SquishClient : public ICongestionControlListener{
public:
    void updateSpsTransmitFlow(
        std::shared_ptr<CongestionControlUserData> congestionControlUserData);
    void onCongestionControlDataReady (
        std::shared_ptr<CongestionControlUserData> congestionControlUserData,
            bool critEvent) override;
     void setDataReadySemaphore(sem_t* dataReadySem);
private:
    sem_t* dataReadySem_;
};


#endif  // SASQUISHCLIENT_HPP