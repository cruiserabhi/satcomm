/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @brief      This class reads task queues and executes those tasks
 */

#ifndef TASKTHREAD_HPP
#define TASKTHREAD_HPP

#include <thread>

#include "TaskQueue.hpp"

namespace telux {
namespace common {

class TaskThread {
 public:
    /**
     * Constructor for TaskThread to read from given task queue
     *
     * @param [in]  taskQueue        Tasks to execute from task queue
     *
     * @note: this does not start a thread
     */
    TaskThread(std::shared_ptr<TaskQueue> taskQueue);

    ~TaskThread();

    /**
     * Start executing tasks on the thread
     */
    void start();

    /**
     * Check whether task thread is active or shutdown
     */
    bool isShutdown();

 private:
    // process tasks
    void processTasks();

    // weak_ptr for the TaskQueue, if task queue is not accessible there is no need to execute
    // tasks
    std::weak_ptr<TaskQueue> taskQueue_;

    // flag to indicate stop execution
    std::atomic_bool shutdown_;

    // Thread to run the tasks on
    std::thread thread_;
};

}  // end of namespace common
}  // end of namespace telux

#endif