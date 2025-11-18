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


/**
 * @file       SignalHandler.hpp
 * @brief      Provide API to register signal handler, applications can provide a
 *             callback and signal sets when using the API. This utility will block
 *             the provided signal, and then starts a dedicated thread to wait for
 *             one of the signals delivered to the proces.
 *             Depending on which signal is delivered by the OS, it may:
 *             1. Generate the backtrace and core registers for thread-directed signals
 *                (SIGSEGV, SIGFPE, etc) because of hardware exceptions.
 *             2. Call the user supplied callback in the dedicated thread when it is
 *                woke up by one of the blocked signals.
 *
 */

#ifndef SIGNALHADNDLER_HPP
#define SIGNALHADNDLER_HPP

#include <signal.h>
#include <functional>
#include <sstream>
#include <execinfo.h>

using SignalHandlerCb = std::function<void(int sig)>;

/**
 * @brief Provide a way to handle signal/exceptions inside telsdk and provide backtrace
 *        in case of program exceptions.
 */
class SignalHandler {
public:
   /**
    * Register signal handler.
    *
    * This need to be be called at the very beginning of telux applications.
    *
    * It handles signals in two ways:
    *
    * 1. For signals intended for the process in general and not meant for a specific thread,
    *    block them if specified by caller and starts a dedicated thread to block wait
    *    for one of signals to become pending, then call the user supplied callback.
    *
    * 2. For signals intended for a specific thread (like SIGSEGV, SIGFPE, SIGILL, SIGBUS,
    *    SIGABRT, etc) which are caused because of hardware exception. A method is registered
    *    to generate the backtrace for analysis. This method also saves the previous action for
    *    these signals in order to have the the coredump file generated.
    *
    * @param[in] sigset - specify which signals to be blocked and handled
    * @param[in] cb     - the callback registered which is used to handle the signals
    *                   - if cb is not provided or set to nullptr, the signals in sigset
    *                   - will still be caught and the backtrace will still be dumped
    *
    * NOTE:
    * The parameter sigset should not contain thread directed signals (SIGBUS, SIGFPE, SIGILL,
    * SIGSEGV, SIGABRT, etc), as noted by posix specification, if these signals are generated
    * while they are blocked, the result is undefined.
    *
    * @returns true if success, otherwise false.
    */
   static bool registerSignalHandler(sigset_t sigset, SignalHandlerCb cb = nullptr);



private:
   // private static variable declarations
   static constexpr unsigned MAX_BT_SIZE = 20;
   static constexpr unsigned STANDARD_SIGNAL_NUMS = 32;
   static struct sigaction oldacts_[STANDARD_SIGNAL_NUMS];
   static std::stringstream logStream_;

   /**
    * Generate backtrace and dump Core registers
    *
    * This is the signal handler registered and it tries to dump the backtrace and core
    * registers when applicable, restores the default hanlder for the signal received,
    * then triggers the signal again using raise() system call.
    *
    * @note: 1. It is better for the compiler to
    *           1). use "-rdynamic" to the compiler to export all the symbols
    *           2). use "-O0" to disable the compiler optimization
    *           3). use "-fno-omit-frame-pointer" to make sure all the stack frames are presented
    *           4). use "-g" to enable debug information
    *        2. A selinux rule is needed for applications to allow the capability to signal itself
    *           in order to raise the signal for invoking previous/default signal action.
    */
   static void dumpTrace(int sigNum, siginfo_t* siginfo, void* ptr);

};
#endif
