/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_PROFILE_MANAGER_STUB_HPP
#define DATA_PROFILE_MANAGER_STUB_HPP

#include <telux/data/DataProfile.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/data/DataProfileListener.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using dataStub::DataProfileManager;

namespace telux {
namespace data {

class DataProfileManagerStub : public IDataProfileManager,
                               public IDataProfileListener {
public:

    DataProfileManagerStub(SlotId slotId, telux::common::InitResponseCb clientCallback);
    ~DataProfileManagerStub();

    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;

    telux::common::Status createProfile(const ProfileParams &profileParams,
        std::shared_ptr<IDataCreateProfileCallback> callback = nullptr) override;
    telux::common::Status deleteProfile(uint8_t profileId, TechPreference techPreference,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr) override;
    telux::common::Status modifyProfile(uint8_t profileId, const ProfileParams &profileParams,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr) override;

    telux::common::Status queryProfile(const ProfileParams &profileParams,
        std::shared_ptr<IDataProfileListCallback> callback = nullptr) override;
    telux::common::Status requestProfile(uint8_t profileId, TechPreference techPreference,
        std::shared_ptr<IDataProfileCallback> callback = nullptr) override;
    telux::common::Status
        requestProfileList(std::shared_ptr<IDataProfileListCallback> callback
        = nullptr) override;

    int getSlotId() override;

    telux::common::Status registerListener(
      std::weak_ptr<telux::data::IDataProfileListener> listener) override;
    telux::common::Status deregisterListener(
      std::weak_ptr<telux::data::IDataProfileListener> listener) override;

    void onProfileUpdate(int profileId,
        TechPreference techPreference, ProfileChangeEvent event) override;
    telux::common::Status cleanup();

private:
    std::mutex initMtx_;
    std::mutex mtx_;
    std::condition_variable cv_;
    SlotId slotId_ = DEFAULT_SLOT_ID;
    bool ready_ = false;
    telux::common::ServiceStatus subSystemStatus_;

    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::mutex mutex_;
    std::vector<std::weak_ptr<IDataProfileListener>> listeners_;
    std::unique_ptr<::dataStub::DataProfileManager::Stub> stub_;

    void initSync(telux::common::InitResponseCb callback);
    bool waitForInitialization();
    void setSubsystemReady(bool status);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void getAvailableListeners(
        std::vector<std::shared_ptr<telux::data::IDataProfileListener>> &listeners);
    static std::string convertApnTypeEnumToString(telux::data::ApnTypes apnType);
    static telux::data::ApnTypes convertApnTypeStringToEnum(std::string apn);
};

} // end of namespace data
} // end of namespace telux

#endif // DATA_PROFILE_MANAGER_STUB_HPP