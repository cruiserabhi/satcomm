/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <vector>
#include <iostream>
#include <string>

#include "UserInput.hpp"

/*
 * Takes inputs from the user to set diag log collection parameters.
 */
void UserInput::takeConfiguration(telux::platform::diag::DiagConfig &cfg) {
    getSourceType(cfg);
    getSourceInfo(cfg);
    getMode(cfg);
    getMask(cfg);
    if (cfg.method == telux::platform::diag::LogMethod::FILE) {
        getFileSize(cfg);
    } else {
        if (cfg.modeType != telux::platform::diag::DiagLogMode::STREAMING) {
            getWaterMark(cfg);
        }
    }
}

/*
 * Takes inputs from the user to what all peripherals should be drained.
 */
void UserInput::takePeripheralsForDraining(telux::platform::diag::Peripherals &peripherals) {
    std::vector<uint32_t> selection;

    getMultipleChoiceNumbersFromUsr(
        "Enter peripherals (comma separated, 1-modem dsp): ", 1, 1, selection);

    for (uint32_t x = 0; x < selection.size(); x++) {
        switch (selection[x]) {
            case 1:
                peripherals |= telux::platform::diag::DIAG_PERIPHERAL_MODEM_DSP;
                break;
            default:
                std::cout << "invalid peripheral " << selection[x] << std::endl;
                peripherals = 0;
        }
    }
}

/*
 * Gets source type from the user.
 */
void UserInput::getSourceType(telux::platform::diag::DiagConfig &cfg) {
    uint32_t choice = 0;

    getChoiceNumberFromUsr("Enter source type (1-device, 2-peripheral): ", 1, 2, choice);

    if (choice == 1) {
        cfg.srcType = telux::platform::diag::SourceType::DEVICE;
    } else {
        cfg.srcType = telux::platform::diag::SourceType::PERIPHERAL;
    }
}

/*
 * Gets source info from the user.
 */
void UserInput::getSourceInfo(telux::platform::diag::DiagConfig &cfg) {
    std::vector<uint32_t> selection;

    if (cfg.srcType == telux::platform::diag::SourceType::DEVICE) {
/*#if defined(FEATURE_EXTERNAL_AP) || defined(FEATURE_QTI_EXTERNAL_AP)
        getMultipleChoiceNumbersFromUsr(
            "Enter devices (comma separated, 1-mdm, 2-eap, 3-both): ", 1, 3, selection);
#else
        getMultipleChoiceNumbersFromUsr(
            "Enter devices (comma separated, 1-mdm): ", 1, 1, selection);
#endif */

        getMultipleChoiceNumbersFromUsr(
            "Enter devices (comma separated, 1-mdm): ", 1, 1, selection);

        for (uint32_t x = 0; x < selection.size(); x++) {
            switch (selection[x]) {
                case 1:
                    cfg.srcInfo.device |= telux::platform::diag::DIAG_DEVICE_MDM;
                    break;
                case 2:
                    cfg.srcInfo.device |= telux::platform::diag::DIAG_DEVICE_EXTERNAL_AP;
                    break;
                case 3:
                    cfg.srcInfo.device = (telux::platform::diag::DIAG_DEVICE_MDM |
                        telux::platform::diag::DIAG_DEVICE_EXTERNAL_AP);
                    break;
                default:
                    std::cout << "invalid device " << selection[x] << std::endl;
            }
        }
    } else {
        getMultipleChoiceNumbersFromUsr(
            "Enter peripherals (comma separated, 1-integrated AP, 2-modem dsp, 3-both): ",
            1, 3, selection);

        for (uint32_t x = 0; x < selection.size(); x++) {
            switch (selection[x]) {
                case 1:
                    cfg.srcInfo.peripheral |= telux::platform::diag::DIAG_PERIPHERAL_INTEGRATED_AP;
                    break;
                case 2:
                    cfg.srcInfo.peripheral |= telux::platform::diag::DIAG_PERIPHERAL_MODEM_DSP;
                    break;
                case 3:
                    cfg.srcInfo.peripheral = (
                        telux::platform::diag::DIAG_PERIPHERAL_INTEGRATED_AP |
                        telux::platform::diag::DIAG_PERIPHERAL_MODEM_DSP);
                    break;
                default:
                    std::cout << "invalid peripheral " << selection[x] << std::endl;
            }
        }
    }
}

/*
 * Gets mode from the user.
 */
void UserInput::getMode(telux::platform::diag::DiagConfig &cfg) {
    uint32_t choice = 0;

    if (cfg.method == telux::platform::diag::LogMethod::FILE) {
        getChoiceNumberFromUsr("Enter mode (1-streaming, 2-threshold): ", 1, 2, choice);
    } else {
        getChoiceNumberFromUsr(
            "Enter mode (1-streaming, 2-threshold, 3-circular buffer): ", 1, 3, choice);
    }

    switch (choice) {
        case 1:
            cfg.modeType = telux::platform::diag::DiagLogMode::STREAMING;
            break;
        case 2:
            cfg.modeType = telux::platform::diag::DiagLogMode::THRESHOLD;
            break;
        case 3:
            cfg.modeType = telux::platform::diag::DiagLogMode::CIRCULAR_BUFFER;
            break;
        default:
            std::cout << "invalid mode " << choice << std::endl;
    }
}

/*
 * Gets mask(s) file path from the user.
 */
void UserInput::getMask(telux::platform::diag::DiagConfig &cfg) {

    std::shared_ptr<std::string> eapFile;
    std::shared_ptr<std::string> mdmFile;

/*#if defined(FEATURE_EXTERNAL_AP) || defined(FEATURE_QTI_EXTERNAL_AP)
    getAbsoluteFilePathFromUser("Specify EAP mask file (yes/no): ", eapFile);
    cfg.eapLogMaskFile = *eapFile;

    if ((cfg.srcInfo.device & telux::platform::diag::DIAG_DEVICE_MDM) ==
        telux::platform::diag::DIAG_DEVICE_MDM) {
        getAbsoluteFilePathFromUser("Specify MDM mask file (yes/no): ", mdmFile);
        cfg.mdmLogMaskFile = *mdmFile;
    }
#else
    getFileFromUser("Enter MDM mask file (absolute path): ", mdmFile);
    cfg.mdmLogMaskFile = *mdmFile;
#endif*/

    getFileFromUser("Enter MDM mask file (absolute path): ", mdmFile);
    cfg.mdmLogMaskFile = *mdmFile;
}

/*
 * Gets min and max file sizes from the user.
 */
void UserInput::getFileSize(telux::platform::diag::DiagConfig &cfg) {
    uint32_t choice = 0;

    getChoiceNumberFromUsr(
        "Enter max file size (between 1 to 100 MB): ", 1, 100, choice);
     cfg.methodConfig.fileConfig.maxSize = choice;

    choice = 0;
    getChoiceNumberFromUsr(
        "Enter max number of files (between 2 to 100): ", 2, 100, choice);
     cfg.methodConfig.fileConfig.maxNumber = choice;
}

/*
 * Gets low and high water marks from the user.
 */
void UserInput::getWaterMark(telux::platform::diag::DiagConfig &cfg) {
    uint32_t choice = 0;

    getChoiceNumberFromUsr(
        "Enter low water mark (between 1 to 100): ", 1, 100, choice);
     cfg.modeConfig.bufferedModeConfig.lowWaterMark = choice;

    choice = 0;
    getChoiceNumberFromUsr(
        "Enter high water mark (between 1 to 100): ", 1, 100, choice);
     cfg.modeConfig.bufferedModeConfig.highWaterMark = choice;
}

/*
 * Helper to get specific number from the user.
 */
void UserInput::getChoiceNumberFromUsr(
    const std::string choicesToDisplay, const uint32_t minVal,
    const uint32_t maxVal, uint32_t &selection) {

    uint32_t numFromUsr;
    std::string usrInput = "";

    while(1) {
        std::cout << choicesToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            usrInput = "";
            continue;
        }

        try {
            numFromUsr = std::stoul(usrInput);
        } catch (const std::exception& e) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        if ((numFromUsr > maxVal) || (numFromUsr < minVal)) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        selection = numFromUsr;
        return;
    }
}

/*
 * Helper to get specific numbers from the user for setting bitmasks fields.
 */
void UserInput::getMultipleChoiceNumbersFromUsr(
    const std::string choicesToDisplay, const uint32_t minVal,
    const uint32_t maxVal, std::vector<uint32_t> &selection) {

    uint32_t numFromUsr;
    std::string usrInput = "";

    while(1) {
        std::cout << choicesToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }

        for (uint32_t x = 0; x < usrInput.size(); x++) {
            switch (usrInput[x]) {
                case '1':
                    numFromUsr = 1;
                    break;
                case '2':
                    numFromUsr = 2;
                    break;
                case '3':
                    numFromUsr = 3;
                    break;
                default:
                    numFromUsr = 0;
                    break;
            }

            if (numFromUsr && ((numFromUsr > maxVal) || (numFromUsr < minVal))) {
                std::cout << "invalid input ignored " << numFromUsr << std::endl;
                continue;
            }
            if (numFromUsr) {
                selection.push_back(numFromUsr);
            }
        }

        return;
    }
}

/*
 * Helper to get absolute path of a file on file system from the user.
 * User can provide path or skip.
 */
void UserInput::getAbsoluteFilePathFromUser(const std::string choicesToDisplay,
        std::shared_ptr<std::string> &absoluteFilePath) {

    std::string usrInput = "";
    size_t choiceLength;

    while(1) {
        std::cout << choicesToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        choiceLength = usrInput.length();
        for (size_t x = 0; x < choiceLength; x++) {
            usrInput[x] = tolower(usrInput[x]);
        }

        if (!usrInput.compare("no")) {
            absoluteFilePath = nullptr;
            return;
        } else if (!usrInput.compare("yes")) {
            getFileFromUser(absoluteFilePath);
            return;
        } else {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }
    }
}

/*
 * Helper to get absolute path of a file on file system from the user.
 */
void UserInput::getFileFromUser(std::shared_ptr<std::string>& absoluteFilePath) {
    std::string usrInput = "";

    while(1) {
        std::cout << "Enter file's absolute path : ";

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        try {
            absoluteFilePath = std::make_shared<std::string>(usrInput);
        } catch (const std::exception& e) {
            std::cout << "can't allocate string" << std::endl;
            absoluteFilePath = nullptr;
        }
        return;
    }
}

/*
 * Helper to get absolute path of a file on file system from the user.
 */
void UserInput::getFileFromUser(const std::string textToDisplay,
        std::shared_ptr<std::string> &absoluteFilePath) {
    std::string usrInput = "";

    while(1) {
        std::cout << textToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        try {
            absoluteFilePath = std::make_shared<std::string>(usrInput);
        } catch (const std::exception& e) {
            std::cout << "can't allocate string" << std::endl;
            absoluteFilePath = nullptr;
        }
        return;
    }
}
