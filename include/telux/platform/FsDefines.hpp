/*
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       FsDefines.hpp
 *
 * @brief      This file contains enumerations and variables used for filesystem susbsystem.
 *
 */

#ifndef TELUX_PLATFORM_FSDEFINES_HPP
#define TELUX_PLATFORM_FSDEFINES_HPP

#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace platform {
/** @addtogroup telematics_platform_filesystem
 * @{ */

/* Enum to denote the EFS backup/restore operation state */
enum class EfsEvent {
    START, /**< Indicating the beginning of Backup/Restore operation */
    END,   /**< Indicating the completion of Backup/Restore operation */
};

/* EfsEventInfo captures the event related data */
struct EfsEventInfo {
    EfsEvent event;                 /**< The event being notified */
    telux::common::ErrorCode error; /**< @ref telux::common::ErrorCode associated with the event */
};

/* Enum to denote status of operations */
enum class OperationStatus {
    UNKNOWN,
    SUCCESS, /*< Indicates a successful operation*/
    FAILURE, /*< Indicates a failed operation*/
};

/* Enum to denote ota operation */
enum class OtaOperation {
    INVALID,
    START,  /*< Used whenever the client is starting an OTA operation*/
    RESUME, /*< Used whenever the client is resuming a previously started OTA operation*/
};

/** @} */ /* end_addtogroup telematics_platform_filesystem */
}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_FSDEFINES_HPP
