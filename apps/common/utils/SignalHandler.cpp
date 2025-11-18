/*
 *  Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <stdlib.h>
#include <stddef.h>
#include <execinfo.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <cxxabi.h>
#include <thread>

#include "SignalHandler.hpp"

using __cxxabiv1::__cxa_demangle;

// private static variable definitions
struct sigaction SignalHandler::oldacts_[STANDARD_SIGNAL_NUMS];
std::stringstream SignalHandler::logStream_;

void SignalHandler::dumpTrace(int sigNum, siginfo_t* info, void* ptr) {
    logStream_ << " error number = " << sigNum
               << "(" <<  std::string(strsignal(sigNum)) << ")" << std::endl;

    // Should not happen
    if (info == nullptr) {
        return;
    }

    logStream_ << "Fault adress: " << std::hex << info->si_addr << std::endl;

    void *buffer[MAX_BT_SIZE] = {0};
    size_t nptrs;
    char **strings = nullptr;
    int status = -1;

    nptrs = backtrace(buffer, MAX_BT_SIZE);
    strings = backtrace_symbols(buffer, nptrs);
    if (strings == nullptr) {
        std::cout << "backtrace_symbols failed" << std::endl;
        return;
    }

    for (size_t i = 2; i < nptrs; ++i) {
        std::string name(strings[i]);
        // the output from backtrace_symbols follows following
        // format: <library path>(<Function Name>+<offset with the function>) [Symbol address].
        // By any chance that the rule is not followed, don't do demangling.
        size_t SymbolStart = name.find_first_of("(") + sizeof(char);
        size_t SymbolEnd = name.find_first_of("+");
        if (std::string::npos != SymbolStart &&
            std::string::npos != SymbolEnd &&
            SymbolStart < SymbolEnd) {
            std::string syb = name.substr(SymbolStart, SymbolEnd - SymbolStart);

            // normally main frame is the last frame, and need no demangling
            if (syb == "main") {
                logStream_ << "main()" << std::endl;
                break;
            }
            char* tmp = __cxa_demangle(syb.c_str(), nullptr, 0, &status);
            if (status == 0 && tmp) {
                name = std::string(tmp) + " [" + name.substr(0, SymbolStart - sizeof(char)) + "]";
            }
        }
        logStream_ << name << std::endl;
    }
    free(strings);
    strings = nullptr;

    std::cout << logStream_.str() << std::endl;
    // restore old action, so that coredump file will still be generated
    sigaction(sigNum, &oldacts_[sigNum], nullptr);
    raise(sigNum);
}

/*
 * block signals
 */
static int blockSignals(sigset_t *sigset) {
    if (0 != pthread_sigmask(SIG_BLOCK, sigset, nullptr)) {
        std::cout << "Fail to block signals.\n";
        return -1;
    }

    return 0;
}

bool SignalHandler::registerSignalHandler(sigset_t sigset, SignalHandlerCb cb) {
    // block the signals specified in sigset,
    // so the OS would not deliver signals to the threads which
    // do not wait for it.
    if (blockSignals(&sigset)) {
        return false;
    }

    // Use dedicated thread to wait for the blocked signals in sigset.
    // The cb is supposed to do cleanup then exit.
    // If the cb is not provided, the trace will still be dumped
    std::thread sigHandleThread([sigset, cb] () {
        int sig = -1;
        //sigwait will suspend calling this thread until one of the signals
        //specified in the signal set becomes pending.
        sigwait(&sigset, &sig);
        std::cout << "Signal " << sig
                  << "(" <<  std::string(strsignal(sig)) << ")" << std::endl;
        if (cb) {
            cb(sig);
        }
    });
    sigHandleThread.detach();

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = SignalHandler::dumpTrace;
    action.sa_flags = SA_SIGINFO | SA_NODEFER;

    // following signals are thread-directed signals, which means the signal handler
    // will be involked from the specific thread which cause the signal.
    // It will have backtrace and core registers dumped in the signal handler
    sigaction(SIGSEGV, &action, &oldacts_[SIGSEGV]);
    sigaction(SIGFPE, &action, &oldacts_[SIGFPE]);
    sigaction(SIGILL, &action, &oldacts_[SIGILL]);
    sigaction(SIGBUS, &action, &oldacts_[SIGBUS]);
    sigaction(SIGABRT, &action, &oldacts_[SIGABRT]);

    return true;
}
