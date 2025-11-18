/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @brief   This class contains queue of tasks to run
 */

#ifndef TASKQUEUE_HPP
#define TASKQUEUE_HPP

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <deque>

#include "Logger.hpp"

namespace telux {
namespace common {

class TaskQueue {
 public:
    TaskQueue();

    /**
     * push the task back of the queue
     *
     * @param [in] task    A task to push to the back of the queue
     * @param [in] args    The arguments to call the task with
     *
     * @returns std::future to know the status, this is optional for clients to use it
     */

    template <typename F, typename... Args>
    auto push(F task, Args &&... args) -> std::future<decltype(task(args...))> {
        return pushTo(std::forward<F>(task), std::forward<Args>(args)...);
    }

    /**
     * Pops first task from the queue
     */
    std::packaged_task<void()> pop();

    /**
     * Clears the outstanding tasks and refuses additional task submissions
     */
    void shutdown();

    /**
     * Check the status of shutdown
     */
    bool isShutdown();

 private:
    // queue of tasks
    std::deque<std::packaged_task<void()>> queue_;

    // mutex to protect queue_
    std::mutex mtx_;

    // A condition variable to wait for new tasks to be placed on the queue
    std::condition_variable cv_;

    // flag for indicating whether to accept more tasks or not
    std::atomic_bool shutdown_;

    template <typename F, typename... Args>
    auto pushTo(F task, Args &&... args) -> std::future<decltype(task(args...))>;
};

template <typename F, typename... Args>
auto TaskQueue::pushTo(F task, Args &&... args) -> std::future<decltype(task(args...))> {
    LOG(DEBUG, " pushTo ");
    using returnType = decltype(task(args...));
    auto boundedTask = std::bind(std::forward<F>(task), std::forward<Args>(args)...);

    auto pkgedTask = std::packaged_task<returnType()>(boundedTask);
    auto future = pkgedTask.get_future();
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.emplace_back(std::move(pkgedTask));
    }
    cv_.notify_one();
    return future;
}

}  // end of namespace common
}  // end of namespace telux

#endif