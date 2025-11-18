/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * This Sample application demonstrates the usage of SDK data library
 * to start data call and handle throttle indications.
 * The steps are as follows:
 *
 *  1.   Get DataFactory instance.
 *  2.   Get a IDataConnectionManager instance from DataFactory.
 *  3.   Wait for the data service to become available.
 *  4.   Register a listener which will receive updates whenever
 *       status of the call is changed or ThrottledApnInfo is updated.
 *  5.   Define parameters for the call and place the data call.
 *  5.1. If Data profile is throttled wait to get un-throttled.
 *  6.   Finally, when the use case is over, deregister the listener.
 *
 * Usage:
 * # ./data_call_app 1 1 0
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <telux/data/DataFactory.hpp>
#include <thread>

static int profileId = 0;
#define MAX_START_DATA_CALL_RETRY 5
#define DATA_CALL_RETRY_TIMER 2

// Implementation of IDataConnectionListener
class DataConnectionManager
    : public telux::data::IDataConnectionListener,
      public std::enable_shared_from_this<telux::data::IDataConnectionListener> {
  public:
   bool init(SlotId slotId) {
      telux::common::ServiceStatus serviceStatus;
      std::promise<telux::common::ServiceStatus> p{};

      // [1] Get DataFactory instance.
      auto &dataFactory = telux::data::DataFactory::getInstance();

      // [2] Get a IDataConnectionManager instance from DataFactory.
      dataConnMgr_ = dataFactory.getDataConnectionManager(slotId,
             [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
      });

      if (!dataConnMgr_) {
         std::cout << "Can't get IDataConnectionManager" << std::endl;
         return false;
      }

      // [3] Wait for the data service to become available.
      serviceStatus = p.get_future().get();
      if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
         std::cout << "Data service unavailable, status " <<
               static_cast<int>(serviceStatus) << std::endl;
         return false;
      }

      // [4] Register for Data listener
      if (dataConnMgr_->registerListener(shared_from_this()) !=
          telux::common::Status::SUCCESS) {
         return false;
      }

      std::cout << "Initialization complete" << std::endl;
      return true;
   }

   void startDataCall(telux::data::OperationType opType = telux::data::OperationType::DATA_LOCAL) {
      // [5] attempt to start data call
      std::promise<telux::common::ErrorCode> prom = std::promise<telux::common::ErrorCode>();
      {
         telux::common::Status status;
         std::unique_lock<std::mutex> lock(dataCallUpdateMutex_);
         if (isDataCallRequestInProgress_ || isDataCallConnected_) {
            // data call is connected or data call request already in progress
            return;
         }
         if (++dataCallAttempt_ > MAX_START_DATA_CALL_RETRY) {
            // max data call retry attempt reached
            std::cout << "failed to start data call attempted "
                      << MAX_START_DATA_CALL_RETRY << " times" << std::endl;
            exit(1);
         }
         std::cout << "start data call attempt: " << dataCallAttempt_ << std::endl;

         telux::data::DataCallParams dataCallParams;
         dataCallParams.profileId = profileId;
         dataCallParams.ipFamilyType = telux::data::IpFamilyType::IPV4;
         dataCallParams.operationType = opType;
         status = dataConnMgr_->startDataCall(
             dataCallParams,
             [this, &prom](
                 const std::shared_ptr<telux::data::IDataCall> &dataCall,
                 telux::common::ErrorCode errorCode) {
                // callback of start data call
                std::cout << "startCallResponse: errorCode: "
                          << static_cast<int>(errorCode) << std::endl;
                prom.set_value(errorCode);
             });

         if (status == telux::common::Status::SUCCESS &&
             prom.get_future().get() == telux::common::ErrorCode::SUCCESS) {
            isDataCallRequestInProgress_ = true;
            return;
         }
      }

      if (isProfileThrottled()) {
         //[5.1] data profile is throttled wait to get un-throttled @ref [8]
         std::cout << "\n data profile is throttled wait to get un-throttled";
      } else {
         std::this_thread::sleep_for(
             std::chrono::seconds(DATA_CALL_RETRY_TIMER));
         // retry start data call
         std::cout << "\n retry start data call ";
         startDataCall();
      }
   }

   // This function is called when there is a change in the data call
   void onDataCallInfoChanged(
       const std::shared_ptr<telux::data::IDataCall> &dataCall) override {
      // Start data call update
      {
         std::unique_lock<std::mutex> lock(dataCallUpdateMutex_);
         std::cout << "\n onDataCallInfoChanged";
         logDataCallDetails(dataCall);
         telux::data::DataCallStatus status = dataCall->getDataCallStatus();
         if (status == telux::data::DataCallStatus::NET_CONNECTED) {
            // Start data call success
            std::cout << "\n onDataCallInfoChanged data call connected !!!";
            isDataCallConnected_ = true;
            isDataCallRequestInProgress_ = false;
            return;
         } else if (status == telux::data::DataCallStatus::NET_CONNECTING) {
            std::cout << "\n Trying to connect data call";
            return;
         } else {
            // start data call failed
            isDataCallConnected_ = false;
            isDataCallRequestInProgress_ = false;
         }
      }

      if (isProfileThrottled()) {
         // data profile is throttled wait to get un-throttled @ref [6]
         std::cout << "\n data profile is throttled wait to get un-throttled";
         return;
      } else {
         std::this_thread::sleep_for(
             std::chrono::seconds(DATA_CALL_RETRY_TIMER));
         // retry start data call
         std::cout << "\n retry start data call ";
         startDataCall();
      }
   }

   bool isProfileThrottled() {
      // Check if the data profile throttled
      std::promise<bool> prom = std::promise<bool>();
      telux::common::Status status = dataConnMgr_->requestThrottledApnInfo(
          [&](const std::vector<telux::data::APNThrottleInfo> &throttleInfoList,
              telux::common::ErrorCode error) {
             std::cout << "startCallResponse: errorCode: "
                       << static_cast<int>(error) << std::endl;
             logThrottledApnInfoChanged(throttleInfoList);
             bool isProfileThrottled = false;
             if (profileId) {
                for (auto throttleInfo : throttleInfoList) {
                   for (int tProfId : throttleInfo.profileIds) {
                      if (profileId == tProfId) {
                         isProfileThrottled = true;
                         break;
                      }
                   }
                }
             }
             prom.set_value(isProfileThrottled);
          });
      if (status == telux::common::Status::SUCCESS) {
         return prom.get_future().get();
      } else {
         std::cout
             << "Error: failed to trigger requestThrottledApnInfo; status: "
             << static_cast<int>(status) << std::endl;
         return false;
      }
   }

   // This function is called when the throttled state changes, such as when a
   // new APN is throttled or an existing throttled APN is no longer throttled
   // after the timeout. APNs that are not throttled anymore will not appear in
   // the list of throttled APNs.
   void onThrottledApnInfoChanged(
       const std::vector<telux::data::APNThrottleInfo> &throttleInfoList) {
      bool reTriggerStartDataCall = false;
      {
         std::unique_lock<std::mutex> lock(throttleUpdateMutex_);
         logThrottledApnInfoChanged(throttleInfoList);
         bool newThrottleState = false;
         if (profileId) {
            for (auto throttleInfo : throttleInfoList) {
               for (int tProfId : throttleInfo.profileIds) {
                  // absence of profile id in throttle info considered as the
                  // profile is not throttled
                  if (profileId == tProfId) {
                     newThrottleState = true;
                     if (throttleInfo.isBlocked) {
                        // APN blocked on all plmns
                        std::cout << "\n APN = " << throttleInfo.apn
                                  << "; is blocked on all plmns!!!\n";
                     }
                     break;
                  }
               }
            }
         }
         // profile was throttled before now as per the updated indication it's
         // not throttled. So retry to start the data call
         if (isProfileThrottled_ && !newThrottleState) {
            reTriggerStartDataCall = true;
         }
         isProfileThrottled_ = newThrottleState;
      }

      if (reTriggerStartDataCall) {
         startDataCall();
      }
   }

   void cleanUp() {
      // [6] deregister listener and loose data connection manager reference
      dataConnMgr_->deregisterListener(shared_from_this());
      dataConnMgr_ = nullptr;
   }

  private:
   void logDataCallDetails(
       const std::shared_ptr<telux::data::IDataCall> &dataCall) {
      std::cout << " ** DataCall Details **\n";
      std::cout << " SlotID: " << dataCall->getSlotId() << std::endl;
      std::cout << " ProfileID: " << dataCall->getProfileId() << std::endl;
      std::cout << " interfaceName: " << dataCall->getInterfaceName()
                << std::endl;
      std::cout << " DataCallStatus: " << (int)dataCall->getDataCallStatus()
                << std::endl;
      std::cout << " DataCallEndReason: Type = "
                << static_cast<int>(dataCall->getDataCallEndReason().type)
                << std::endl;
      std::list<telux::data::IpAddrInfo> ipAddrList =
          dataCall->getIpAddressInfo();
      for (auto &it : ipAddrList) {
         std::cout << "\n ifAddress: " << it.ifAddress
                   << "\n primaryDnsAddress: " << it.primaryDnsAddress
                   << "\n secondaryDnsAddress: " << it.secondaryDnsAddress
                   << '\n';
      }
      std::cout << " IpFamilyType: "
                << static_cast<int>(dataCall->getIpFamilyType()) << '\n';
      std::cout << " TechPreference: "
                << static_cast<int>(dataCall->getTechPreference()) << '\n';
      std::cout << " DataBearerTechnology: "
                << static_cast<int>(dataCall->getCurrentBearerTech()) << '\n';
   }

   void logThrottledApnInfoChanged(
       const std::vector<telux::data::APNThrottleInfo> &throttleInfoList) {
      std::cout << "** onThrottledApnInfoChanged **" << std::endl;
      std::cout << " Number of throttled APN: " << throttleInfoList.size()
                << std::endl;
      int index = 0;
      for (auto throttleInfo : throttleInfoList) {
         std::cout << " index = " << ++index << std::endl << " Profile IDs = ";
         for (int profileId : throttleInfo.profileIds) {
            std::cout << profileId << ", ";
         }
         std::cout << std::endl
                   << " APN: " << throttleInfo.apn << std::endl
                   << " ipv4Time (msec): " << throttleInfo.ipv4Time << std::endl
                   << " ipv6Time (msec): " << throttleInfo.ipv6Time << std::endl
                   << " isBlocked: "
                   << (throttleInfo.isBlocked ? "True" : "False") << std::endl
                   << " mcc: " << throttleInfo.mcc << std::endl
                   << " mnc: " << throttleInfo.mnc << std::endl
                   << std::endl;
      }
   }

   std::shared_ptr<telux::data::IDataConnectionManager> dataConnMgr_;
   int dataCallAttempt_ = 0;
   bool isProfileThrottled_ = false;
   std::mutex throttleUpdateMutex_;

   bool isDataCallConnected_ = false;
   bool isDataCallRequestInProgress_ = false;
   std::mutex dataCallUpdateMutex_;
};

int main(int argc, char *argv[]) {

   SlotId slotId;
   telux::data::OperationType opType;

   if (argc == 4) {
      profileId = std::atoi(argv[1]);
      slotId = static_cast<SlotId>(std::atoi(argv[2]));
      opType = static_cast<telux::data::OperationType>(std::atoi(argv[3]));
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_call_app <profile_id> <slot_id> <optype>\n";
      std::cout << "\n\t ./data_call_app 1 1 0  --> to start the data call on "
                   "Profile Id 1, slot Id 1, OperationType 0 <0>  \n";
      return 1;
   }

   // initialize data connection manager
   std::shared_ptr<DataConnectionManager> dataConnMgr = std::make_shared<DataConnectionManager>();
   if (dataConnMgr->init(slotId)) {
      // attempt to start data call
      dataConnMgr->startDataCall(opType);
   } else {
      std::cout << "\n\nfailed to initialize data connection manager!!! \n\n";
      return 1;
   }

   // exit logic
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // Cleanup
   dataConnMgr->cleanUp();

   std::cout << "\nData call app exiting" << std::endl;
   return 0;
}
