/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "TaskQueue.hpp"

namespace telux {
namespace common {

TaskQueue::TaskQueue()
   : shutdown_(false) {
    LOG(DEBUG, __FUNCTION__);
}

std::packaged_task<void()> TaskQueue::pop() {
    std::packaged_task<void()> pt;

    std::unique_lock<std::mutex> lock{mtx_};
    auto isWait = [this]() { return shutdown_ || !queue_.empty(); };
    if (!isWait()) {
        cv_.wait(lock, isWait);
    }

    if (!queue_.empty()) {
        pt = std::move(queue_.front());
        queue_.pop_front();
    }
    return pt;
}

void TaskQueue::shutdown() {
    std::lock_guard<std::mutex> lk{mtx_};
    queue_.clear();
    shutdown_ = true;
    cv_.notify_all();
}

bool TaskQueue::isShutdown() {
    return shutdown_;
}

}  // end of namespace common
}  // end of namespace telux