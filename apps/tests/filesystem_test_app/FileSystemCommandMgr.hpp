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


#ifndef FILESYSTEMCOMMANDMGR_HPP
#define FILESYSTEMCOMMANDMGR_HPP

#include <memory>
#include <string>
#include <sstream>

#include <telux/platform/PlatformFactory.hpp>

class FileSystemListener;

class FileSystemCommandMgr : public std::enable_shared_from_this<FileSystemCommandMgr> {
 public:
    FileSystemCommandMgr();
    ~FileSystemCommandMgr();

    int init();
    void registerForUpdates();
    void deregisterFromUpdates();

    void startEfsBackup();
    void prepareForEcall();
    void eCallCompleted();
    void prepareForOtaStart();
    void otaCompleted();
    void prepareForOtaResume();
    void startAbSync();

 private:
    std::shared_ptr<telux::platform::IFsManager> fsMgr_;
    std::shared_ptr<FileSystemListener> fsListener_;
    template <typename T>
    static void getInput(std::string prompt, T &input) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        std::stringstream ss(line);
        ss >> input;
        bool valid = false;
        do {
            if (!ss.bad() && ss.eof() && !ss.fail()) {
                valid = true;
            } else {
                // If an error occurs then an error flag is set and future attempts to get
                // input will fail. Clear the error flag on cin.
                std::cin.clear();
                // Clear the string stream's states and buffer
                ss.clear();
                ss.str("");
                std::cout << "Invalid input, please re-enter" << std::endl;
                std::cout << prompt;
                std::getline(std::cin, line);
                ss << line;
                ss >> input;
            }
        } while (!valid);
    }
};
#endif  // FILESYSTEMCOMMANDMGR_HPP
