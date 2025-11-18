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


/**
 * @file       CardFileHandlerStub.hpp
 *
 * @brief      Implementation of ICardFileHandler
 *
 */

#ifndef CARD_FILEHANDLER_STUB_HPP
#define CARD_FILEHANDLER_STUB_HPP

#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include "CardAppStub.hpp"
#include <telux/tel/CardManager.hpp>
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using telStub::CardService;


namespace telux {
namespace tel {

class CardFileHandlerStub : public ICardFileHandler  {
public:
    telux::common::Status readEFLinearFixed(std::string filePath, uint16_t fileId,
        int recordNum, std::string aid, EfOperationCallback callback) override;
    telux::common::Status readEFLinearFixedAll(std::string filePath, uint16_t fileId,
        std::string aid, EfReadAllRecordsCallback callback) override;
    telux::common::Status readEFTransparent(std::string filePath, uint16_t fileId, int size,
        std::string aid, EfOperationCallback callback) override;
    telux::common::Status writeEFLinearFixed(std::string filePath, uint16_t fileId,
        int recordNum, std::vector<uint8_t> data, std::string pin2, std::string aid,
        EfOperationCallback callback) override;
    telux::common::Status writeEFTransparent(std::string filePath, uint16_t fileId,
        std::vector<uint8_t> data, std::string aid, EfOperationCallback callback) override;
    telux::common::Status requestEFAttributes(EfType efType, std::string filePath,
        uint16_t fileId, std::string aid, EfGetFileAttributesCallback callback) override;
    SlotId getSlotId() override;
    telux::common::Status updateCardApps(std::vector<std::shared_ptr<CardAppStub>> cardApps);
    bool isAppReady(std::string aid);
    void cleanup();
    CardFileHandlerStub(SlotId slotId);
private:
    SlotId slotId_;
    std::mutex mtx_;
    std::unique_ptr<::telStub::CardService::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::vector<std::shared_ptr<CardAppStub>> cardApps_;
    void invokeCallback(EfOperationCallback callback,
        telux::common::ErrorCode error, telux::tel::IccResult iccresult, int cbDelay);
    void invokeCallback(EfReadAllRecordsCallback callback,
        telux::common::ErrorCode error, std::vector<IccResult> records, int cbDelay);
    void invokeCallback(EfGetFileAttributesCallback callback, telux::common::ErrorCode error,
        telux::tel::IccResult iccresult, telux::tel::FileAttributes attributes, int cbDelay );
};

} // end of namespace tel

} // end of namespace telux

#endif // CARD_FILEHANDLER_STUB_HPP