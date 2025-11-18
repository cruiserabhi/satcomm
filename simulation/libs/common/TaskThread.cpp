/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "TaskThread.hpp"

namespace telux {
namespace common {

TaskThread::TaskThread(std::shared_ptr<TaskQueue> taskQueue)
   : taskQueue_(taskQueue)
   , shutdown_(false) {
    LOG(DEBUG, __FUNCTION__);
}

TaskThread::~TaskThread() {
    LOG(DEBUG, __FUNCTION__);
    shutdown_ = true;

    if (thread_.joinable()) {
        thread_.join();
    }
}

void TaskThread::start() {
    thread_ = std::thread{[=] { processTasks(); }};
}

bool TaskThread::isShutdown() {
    return shutdown_;
}

void TaskThread::processTasks() {
    while (!shutdown_) {
        auto tq = taskQueue_.lock();
        if (tq && !tq->isShutdown()) {
            auto task = tq->pop();
            if (task.valid()) {
                LOG(DEBUG, " processing task");
                task();
            } else {
                LOG(DEBUG, " unable to process task");
            }
        } else {
            // taskQueue_ is not available or destroyed
            // killing the thread
            shutdown_ = true;
        }
    }
}

}  // end of namespace common
}  // end of namespace telux