/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CALLBACKMETHOD_HPP
#define CALLBACKMETHOD_HPP

#include <memory>

#include <telux/platform/diag/DiagLogManager.hpp>

#include "CollectionMethod.hpp"

class LogsReceiver : public telux::platform::diag::IDiagListener {
 public:
    void onAvailableLogs(uint8_t *data, int length) override;
};

class CallbackMethod : public CollectionMethod,
                       public std::enable_shared_from_this<CallbackMethod> {
 public:
    CallbackMethod(std::string menuTitle, std::string cursor,
        std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr);
    ~CallbackMethod();

    void showCallbackMenu();
    int initCallbackMethod();

 private:
    std::shared_ptr<LogsReceiver> logsReceiver_;
    void drainPeripheralBuffer();
};

#endif // CALLBACKMETHOD_HPP