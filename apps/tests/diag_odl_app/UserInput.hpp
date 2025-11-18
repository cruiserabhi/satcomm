/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef USERINPUT_HPP
#define USERINPUT_HPP

#include <cstdint>
#include <string>

#include <telux/platform/diag/DiagLogManager.hpp>

class UserInput {
 public:
    void takeConfiguration(telux::platform::diag::DiagConfig &cfg);
    void takePeripheralsForDraining(telux::platform::diag::Peripherals &peripherals);

 private:
    void getChoiceNumberFromUsr(const std::string choicesToDisplay,
        const uint32_t minVal, const uint32_t maxVal, uint32_t &selection);
    void getMultipleChoiceNumbersFromUsr(const std::string choicesToDisplay,
        const uint32_t minVal, const uint32_t maxVal,
        std::vector<uint32_t> &selection);
    void getAbsoluteFilePathFromUser(const std::string choicesToDisplay,
        std::shared_ptr<std::string> &absoluteFilePath);
    void getFileFromUser(const std::string textToDisplay,
        std::shared_ptr<std::string> &absoluteFilePath);
    void getFileFromUser(std::shared_ptr<std::string>& absoluteFilePath);

    void getSourceType(telux::platform::diag::DiagConfig &cfg);
    void getSourceInfo(telux::platform::diag::DiagConfig &cfg);
    void getMode(telux::platform::diag::DiagConfig &cfg);
    void getMask(telux::platform::diag::DiagConfig &cfg);
    void getFileSize(telux::platform::diag::DiagConfig &cfg);
    void getWaterMark(telux::platform::diag::DiagConfig &cfg);
};

#endif // USERINPUT_HPP