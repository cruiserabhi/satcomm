/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "SquishClient.hpp"
#include "SasquishUtils.hpp"

void SquishClient::updateSpsTransmitFlow(
    std::shared_ptr<CongestionControlUserData> congestionControlUserData){
}
void SquishClient::onCongestionControlDataReady (
    std::shared_ptr<CongestionControlUserData> congestionControlUserData,
        bool critEvent) {
    if(congestionControlUserData->congestionControlSem){
        sem_post(congestionControlUserData->congestionControlSem);
        // let sasquish know that data is ready
        sem_post(dataReadySem_);
    }
}

void SquishClient::setDataReadySemaphore(sem_t* dataReadySem){
    dataReadySem_ = dataReadySem;
}