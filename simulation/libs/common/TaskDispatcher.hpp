/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *
 * @file    TaskDispatcher.hpp
 * @brief   This class implements thread pool pattern, to execute the tasks asynchronously.
 *
 *          It has APIs to submit any Callable target (function, lambda expression, bind expression,
 *          or another function object) so that it can be invoked asynchronously
 *
 */

#ifndef TASKDISPATCHER_HPP
#define TASKDISPATCHER_HPP

#include <memory>
#include <vector>

#include "TaskQueue.hpp"
#include "TaskThread.hpp"

namespace telux {
namespace common {

/**
 * @brief It's used to submit Callable type and run asynchronously
 */
class TaskDispatcher {
 public:
    TaskDispatcher();
    TaskDispatcher(int threadCount);
    ~TaskDispatcher();

    /**
     * Submit the task on the asynchronous executor thread
     *
     * @param [in] task     Task is a any Callable type (function, lambda expression, bind
     * expression,
     *                      or another function object) to be executed
     * @param [in] args     suitable list of argument types
     *
     * @returns std::future to know the status, this is optional for clients to use it
     */
    template <typename F, typename... Args>
    auto submitTask(F task, Args &&... args) -> std::future<decltype(task(args...))>;

    /**
     * Clears the outstanding tasks and refuses additional task submissions
     */
    void shutdown();

    /**
     * Returns the status of shutdown
     */
    bool isShutdown() const;

 private:
    // Queue of tasks
    std::shared_ptr<TaskQueue> taskQueue_ = nullptr;

    // Thread count;
    int threadCount_;

    // list of threads to execute task on.
    std::vector<std::unique_ptr<TaskThread>> taskThreads_;
};

template <typename F, typename... Args>
auto TaskDispatcher::submitTask(F task, Args &&... args) -> std::future<decltype(task(args...))> {
    LOG(DEBUG, __FUNCTION__);
    return taskQueue_->push(task, std::forward<Args>(args)...);
}

}  // end of namespace common
}  // end of namespace telux

#endif  // THREADPOOL_HPP