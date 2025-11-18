/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef FILEMETHOD_HPP
#define FILEMETHOD_HPP

#include <memory>

#include <telux/platform/diag/DiagLogManager.hpp>

#include "CollectionMethod.hpp"

class FileMethod : public CollectionMethod,
                   public std::enable_shared_from_this<FileMethod> {
 public:
    FileMethod(std::string menuTitle, std::string cursor,
        std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr);
    ~FileMethod();

    void showFileMenu();

 private:
};

#endif // FILEMETHOD_HPP