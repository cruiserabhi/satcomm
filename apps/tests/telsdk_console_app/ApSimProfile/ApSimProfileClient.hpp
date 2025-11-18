/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef APSIMPROFILECLIENT_HPP
#define APSIMPROFILECLIENT_HPP

#include <telux/tel/CardManager.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/ApSimProfileManager.hpp>
#include "ConfigParser.hpp"

class ApSimProfileClient : public std::enable_shared_from_this<ApSimProfileClient> {

 public:
    /**
     * Initialize telephony subsystem
     */
    telux::common::Status init();
    telux::common::Status requestProfileList();
    telux::common::Status enableProfile();
    telux::common::Status disableProfile();

    ApSimProfileClient();
    ~ApSimProfileClient();

 private:
     std::shared_ptr<telux::tel::IApSimProfileManager> apSimProfileManager_ = nullptr;
     std::shared_ptr<telux::tel::IApSimProfileListener> apSimProfileListener_ = nullptr;
     std::shared_ptr<telux::tel::ICardManager> cardManager_ = nullptr;
     std::vector<std::shared_ptr<telux::tel::ICard>> cards_ = {};
     std::shared_ptr<telux::tel::ICardListener> cardListener_ = nullptr;

     std::vector<uint8_t> hexToBytes(const std::string &hex);
     std::string getSwappedIccidString(const std::string &data);
     telux::common::Status registerRefresh(SlotId slotId);
     void printTransmitApduResult(int result);
     void parseIccidFromApduResult(std::string payload);
     std::vector<std::string> tokenize(std::string s, std::string del);

     int openLogicalChannel(SlotId slotId);
     void closeLogicalChannel(SlotId slotId, int channel);
     int transmitApdu(SlotId slotId, int channel, std::vector<uint8_t> data, bool isGetProfile);

     telux::tel::RefreshMode refreshMode_ = telux::tel::RefreshMode::UNKNOWN;
     int refreshSlotId_ = DEFAULT_SLOT_ID;
     uint32_t referenceId_ = 0;
     int indSlotId_ = DEFAULT_SLOT_ID;
     std::string indIccid_ = "";
     std::vector<std::string> iccidList_ = {};

     class MyApCardListener : public telux::tel::ICardListener {
      public:
          void onServiceStatusChange(telux::common::ServiceStatus status) override;
          void onCardInfoChanged(int slotId) override;
          void onRefreshEvent(int slotId, telux::tel::RefreshStage stage,
              telux::tel::RefreshMode mode, std::vector<telux::tel::IccFile> efFiles,
              telux::tel::RefreshParams config) override;
          MyApCardListener(std::weak_ptr<ApSimProfileClient> apSimProfileClient);

      private:
          std::weak_ptr<ApSimProfileClient> apSimProfileClient_;
          static std::string refreshStageToString(telux::tel::RefreshStage stage);
          static std::string refreshModeToString(telux::tel::RefreshMode mode);
          static std::string sessionTypeToString(telux::tel::SessionType type);
     };

     class MyApSimProfileListener : public telux::tel::IApSimProfileListener {
      public:
          void onServiceStatusChange(telux::common::ServiceStatus status) override;
          void onRetrieveProfileListRequest(SlotId slotId, uint32_t referenceId) override;
          void onProfileOperationRequest(SlotId slotId, uint32_t referenceId, std::string iccid,
              bool isEnable) override;
          MyApSimProfileListener(std::weak_ptr<ApSimProfileClient> apSimProfileClient);

      private:
          std::weak_ptr<ApSimProfileClient> apSimProfileClient_;
     };

     class CardRefreshResponseCallback {
      public:
          static void commandResponse(telux::common::ErrorCode error);
     };
};

#endif // APSIMPROFILECLIENT_HPP
