/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <cstdio>

#include "common/utils/Utils.hpp"

#include "CryptoOperationMenu.hpp"

CryptoOperationMenu::CryptoOperationMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

CryptoOperationMenu::~CryptoOperationMenu() {
}

void CryptoOperationMenu::getHexStringAsByteArrayFromUsr(
        const std::string choiceToDisplay, std::vector<uint8_t>& usrEntry) {

    std::string usrInput = "";
    std::size_t x = 0;
    std::size_t idx = 0;
    std::size_t byteArraySize = 0;
    bool dataIsValid = true;
    uint8_t byteFromUsr = 0;

    while(1) {
        std::cout << choiceToDisplay;

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()
            || ((usrInput.size() % 2) != 0)) {
            std::cout << "invalid input" << std::endl;
            usrInput = "";
            continue;
        }

        byteArraySize = usrInput.size() / 2;

        for (x = 0; x < byteArraySize; x++, idx += 2) {
            try {
                byteFromUsr = std::stoul(usrInput.substr(idx, 2), nullptr, 16);
                usrEntry.push_back(byteFromUsr);
            } catch (const std::exception& e) {
                usrEntry.resize(0);
                dataIsValid = false;
                std::cout << "invalid input: " << usrInput << std::endl;
                break;
            }
        }

        if (dataIsValid) {
            return;
        }
    }
}

void CryptoOperationMenu::getChoiceNumberFromUsr(
        const std::string choicesToDisplay, const uint32_t minVal,
        const uint32_t maxVal, uint32_t& selection) {

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

void CryptoOperationMenu::getUniqueIdFromUser(uint32_t& uniqueId) {

    getChoiceNumberFromUsr("Enter unique id: ", 0, 4095, uniqueId);
}

void CryptoOperationMenu::getCurveFromUser(telux::sec::ECCCurve& curve) {

    uint32_t usrEntry;

    getChoiceNumberFromUsr(
       "Enter curve (1 - sm2, 2 - nist256, 3 - nist384, 4 - brainpool256, 5 - brainpool384): ",
       1, 5, usrEntry);

    switch (usrEntry) {
        case 1:
            curve = telux::sec::ECCCurve::CURVE_SM2;
            break;
        case 2:
            curve = telux::sec::ECCCurve::CURVE_NISTP256;
            break;
        case 3:
            curve = telux::sec::ECCCurve::CURVE_NISTP384;
            break;
        case 4:
            curve = telux::sec::ECCCurve::CURVE_BRAINPOOLP256R1;
            break;
        case 5:
            curve = telux::sec::ECCCurve::CURVE_BRAINPOOLP384R1;
            break;
        default:
            std::cout << "invalid curve " << usrEntry << std::endl;
    }
}

void CryptoOperationMenu::getPriorityFromUser(telux::sec::RequestPriority& priority) {

    uint32_t usrEntry;

    getChoiceNumberFromUsr(
        "Enter priority (1 - normal, 2 - high): ", 1, 2, usrEntry);

    switch (usrEntry) {
        case 1:
            priority = telux::sec::RequestPriority::REQ_PRIORITY_NORMAL;
            break;
        case 2:
            priority = telux::sec::RequestPriority::REQ_PRIORITY_HIGH;
            break;
        default:
            std::cout << "invalid priority " << usrEntry << std::endl;
    }
}

void CryptoOperationMenu::getTimeoutFromUser(uint32_t& timeout) {

    getChoiceNumberFromUsr(
        "Enter timeout (0 - indefinite or 1 to 2147483647 milliseconds): ",
        0, 2147483647, timeout);
}

/* Do ECC signature verification */
void CryptoOperationMenu::verify() {

    VerificationRequest request{};

    getUniqueIdFromUser(request.uniqueId);
    getCurveFromUser(request.curve);
    getPriorityFromUser(request.priority);

    getHexStringAsByteArrayFromUsr(
        "Enter digest to verify (as hex string): ", request.digest);

    getHexStringAsByteArrayFromUsr(
        "Enter public key x-coordinate (as hex string): ", request.publicKeyX);
    getHexStringAsByteArrayFromUsr(
        "Enter public key y-coordinate (as hex string): ", request.publicKeyY);

    getHexStringAsByteArrayFromUsr(
        "Enter signature r-component (as hex string): ", request.signatureR);
    getHexStringAsByteArrayFromUsr(
        "Enter signature s-component (as hex string): ", request.signatureS);

    if (mode_ == telux::sec::Mode::MODE_ASYNC_POLL) {
        getTimeoutFromUser(request.timeout);
    }

    cmdProcessor_->verifyDigest(request);
    printf("\n");
    fflush(stdout);
}

/* Do ECQV point calculation */
void CryptoOperationMenu::calculate() {

    CalculationRequest request{};

    getUniqueIdFromUser(request.uniqueId);
    getCurveFromUser(request.curve);
    getPriorityFromUser(request.priority);

    getHexStringAsByteArrayFromUsr(
      "Enter scalar (as hex string): ", request.scalar);

    getHexStringAsByteArrayFromUsr(
      "Enter multiplicand point x-coordinate (as hex string): ", request.multiplicandPointX);
    getHexStringAsByteArrayFromUsr(
      "Enter multiplicand point y-coordinate (as hex string): ", request.multiplicandPointY);

    getHexStringAsByteArrayFromUsr(
      "Enter addend point x-coordinate (as hex string): ", request.addendPointX);
    getHexStringAsByteArrayFromUsr(
      "Enter addend point y-coordinate (as hex string): ", request.addendPointY);

    if (mode_ == telux::sec::Mode::MODE_ASYNC_POLL) {
        getTimeoutFromUser(request.timeout);
    }

    cmdProcessor_->calculatePoint(request);
    printf("\n");
    fflush(stdout);
}

telux::common::ErrorCode CryptoOperationMenu::init(telux::sec::Mode mode) {

    try {
        cmdProcessor_ = std::make_shared<CommandProcessor>();
    } catch (const std::exception& e) {
         std::cout << "can't create CommandProcessor" << std::endl;
        return telux::common::ErrorCode::NO_MEMORY;
    }

    telux::common::ErrorCode ec = cmdProcessor_->init(mode);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        return ec;
    }

    std::shared_ptr<ConsoleAppCommand> verify = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "Verify digest", {},
        std::bind(&CryptoOperationMenu::verify, this)));

    std::shared_ptr<ConsoleAppCommand> calculate = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Calculate point", {},
        std::bind(&CryptoOperationMenu::calculate, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> operationCmds = {
        verify, calculate };

    mode_ = mode;

    ConsoleApp::addCommands(operationCmds);
    ConsoleApp::displayMenu();

    return telux::common::ErrorCode::SUCCESS;
}