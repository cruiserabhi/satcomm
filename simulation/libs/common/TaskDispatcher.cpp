/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "TaskDispatcher.hpp"

namespace telux {
namespace common {

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

TaskDispatcher::TaskDispatcher()
   : TaskDispatcher(1) {
    LOG(DEBUG, __FUNCTION__);
}

TaskDispatcher::TaskDispatcher(int threadCount)
   : taskQueue_{std::make_shared<TaskQueue>()}
   , threadCount_(threadCount) {
    LOG(DEBUG, __FUNCTION__);
    for (int i = 0; i < threadCount_; i++) {
        auto th = make_unique<TaskThread>(taskQueue_);
        th->start();
        taskThreads_.emplace_back(std::move(th));
    }
}

TaskDispatcher::~TaskDispatcher() {
    LOG(DEBUG, __FUNCTION__);
    shutdown();
}

void TaskDispatcher::shutdown() {
    LOG(DEBUG, __FUNCTION__);
    taskQueue_->shutdown();
    taskThreads_.clear();
}

bool TaskDispatcher::isShutdown() const {
    return taskQueue_->isShutdown();
}

}  // end of namespace common
}  // end of namespace telux