/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
 *
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to get the default subscription and listen to
 * the subscription changes. The steps are as follows:
 *
 *  1. Get a PhoneFactory instance.
 *  2. Get a ISubscriptionManager instance from PhoneFactory.
 *  3. Wait for the subscription service to become available.
 *  4. Register the listener which will receive updates whenever subscription changes.
 *  5. Get default subscription.
 *  6. Finally, when the use case is over, deregister the listener.
 *
 * Usage:
 * # ./subscription_app
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/SubscriptionManager.hpp>

class SubscriptionInfo : public telux::tel::ISubscriptionListener,
                         public std::enable_shared_from_this<SubscriptionInfo> {
 public:
    int init() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        subscriptionMgr_ = phoneFactory.getSubscriptionManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!subscriptionMgr_) {
            std::cout << "Can't get ISubscriptionManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Subscription service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        status = subscriptionMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        /* Step - 6 */
        status = subscriptionMgr_->removeListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int getDefaultSubscription() {
        telux::common::Status status;
        std::shared_ptr<telux::tel::ISubscription> subscription;

        /* Step - 5 */
        subscription = subscriptionMgr_->getSubscription(DEFAULT_SLOT_ID, &status);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't get current subscription, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!subscription) {
            std::cout << "Empty subscription" << std::endl;
            return 0;
        }

        std::cout << "\nSubscription details:" << std::endl;
        std::cout << " CarrierName : " << subscription->getCarrierName() << std::endl;
        std::cout << " PhoneNumber : " << subscription->getPhoneNumber() << std::endl;
        std::cout << " IccId : " << subscription->getIccId() << std::endl;
        std::cout << " Mcc : " << subscription->getMcc() << std::endl;
        std::cout << " Mnc : " << subscription->getMnc() << std::endl;
        std::cout << " SlotId : " << subscription->getSlotId() << std::endl;
        std::cout << " Imsi : " << subscription->getImsi() << std::endl;

        return 0;
    }

    void onSubscriptionInfoChanged(
        std::shared_ptr<telux::tel::ISubscription> newSubscription) override {

        std::cout << "onSubscriptionInfoChanged()" << std::endl;
        if (!newSubscription) {
            std::cout << "Empty subscription" << std::endl;
            return;
        }

        std::cout << " CarrierName : " << newSubscription->getCarrierName() << std::endl;
        std::cout << " PhoneNumber : " << newSubscription->getPhoneNumber() << std::endl;
        std::cout << " IccId : " << newSubscription->getIccId() << std::endl;
        std::cout << " Mcc : " << newSubscription->getMcc() << std::endl;
        std::cout << " Mnc : " << newSubscription->getMnc() << std::endl;
        std::cout << " SlotId : " << newSubscription->getSlotId() << std::endl;
        std::cout << " Imsi : " << newSubscription->getImsi() << std::endl;
    }

 private:
    std::shared_ptr<telux::tel::ISubscriptionManager> subscriptionMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<SubscriptionInfo> app;

    try {
        app = std::make_shared<SubscriptionInfo>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate SubscriptionInfo" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->getDefaultSubscription();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nSubscription app exiting" << std::endl;
    return 0;
}
