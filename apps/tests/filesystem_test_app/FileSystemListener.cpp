/*
 *  Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * This file hosts the implemenation of the FileSystemListener class, which is notified of
 * file system events in the platform
 */

#include <iostream>

#include "FileSystemListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

FileSystemListener::FileSystemListener() {
}

FileSystemListener::~FileSystemListener() {
}

void FileSystemListener::printEfsEvent(std::string type, EfsEventInfo eventInfo) {
    EfsEvent event = eventInfo.event;
    telux::common::ErrorCode error = eventInfo.error;
    PRINT_NOTIFICATION << type;
    if (event == EfsEvent::START) {
        std::cout << ": START" << std::endl;
    } else if (event == EfsEvent::END) {
        std::cout << ": END with ErrorCode: " << Utils::getErrorCodeAsString(error) << std::endl;
    } else {
        std::cout << APP_NAME << " ERROR: Invalid EFS restore event notified" << std::endl;
    }
}

void FileSystemListener::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : UNAVAILABLE" << std::endl;
    } else if (status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : AVAILABLE" << std::endl;
    }
}

void FileSystemListener::OnEfsRestoreEvent(EfsEventInfo event) {
    std::cout << std::endl;
    printEfsEvent("Restore EFS", event);
}

void FileSystemListener::OnEfsBackupEvent(EfsEventInfo event) {
    std::cout << std::endl;
    printEfsEvent("Backup EFS", event);
}

void FileSystemListener::OnFsOperationImminentEvent(uint32_t timeLeftToStart) {
    std::cout << "Filesystem operation shall re-enable in seconds " << timeLeftToStart << std::endl;
}
