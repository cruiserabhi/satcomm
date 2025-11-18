/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2021, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * This is a test application to register and receive EFS related events
 * The application can run in
 * (1) Listen mode, where the registration to the notifications are automatically done
 * (2) Console mode, where the registration and deregistration to the notifications can be
 * controlled via console
 */

#include <getopt.h>
#include <iostream>
#include <vector>
#include <csignal>

extern "C" {
#include "unistd.h"
}

#include "FileSystemCommandMgr.hpp"
#include "FileSystemTestApp.hpp"
#include "Utils.hpp"

std::shared_ptr<FileSystemTestApp> fileSystemTestApp_ = nullptr;

void FileSystemTestApp::printHelp() {

    std::cout << "Usage: " << APP_NAME << " options" << std::endl;
    std::cout << "   -h --help              : print the help menu" << std::endl;
}

Status FileSystemTestApp::parseArguments(int argc, char **argv) {
    int arg;
    while (1) {
        static struct option long_options[] = {{"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};
        int opt_index = 0;
        arg = getopt_long(argc, argv, "h", long_options, &opt_index);
        if (arg == -1) {
            break;
        }
        switch (arg) {
            case 'h':
                printHelp();
                break;
            default:
                printHelp();
                return Status::INVALIDPARAM;
        }
    }
    return Status::SUCCESS;
}

FileSystemTestApp::FileSystemTestApp()
   : ConsoleApp("FileSystem Management Menu", "fs-mgmt> ")
   , myFsCmdMgr_(nullptr) {
}

FileSystemTestApp::~FileSystemTestApp() {
    if (myFsCmdMgr_) {
        myFsCmdMgr_->deregisterFromUpdates();
    }
    myFsCmdMgr_ = nullptr;
}

void signalHandler(int signum) {
    fileSystemTestApp_->signalHandler(signum);
}

void FileSystemTestApp::signalHandler(int signum) {
    std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
    cleanup();
    exit(1);
}

int FileSystemTestApp::init() {
    myFsCmdMgr_ = std::make_shared<FileSystemCommandMgr>();
    int rc = myFsCmdMgr_->init();
    if (rc) {
        return -1;
    }
    return 0;
}

void FileSystemTestApp::cleanup() {
    if (myFsCmdMgr_) {
        myFsCmdMgr_->deregisterFromUpdates();
    }
    myFsCmdMgr_ = nullptr;
}

void FileSystemTestApp::consoleinit() {
    std::shared_ptr<ConsoleAppCommand> startEfsBackupCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Start_Efs_Backup", {},
            std::bind(&FileSystemCommandMgr::startEfsBackup, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> prepareForEcallCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Prepare_For_Ecall", {},
            std::bind(&FileSystemCommandMgr::prepareForEcall, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> eCallCompletedCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "ECall_Completed", {},
            std::bind(&FileSystemCommandMgr::eCallCompleted, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> prepareForOtaStartCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Prepare_For_Ota_Start", {},
            std::bind(&FileSystemCommandMgr::prepareForOtaStart, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> otaCompletedCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "5", "Ota_Completed", {}, std::bind(&FileSystemCommandMgr::otaCompleted, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> prepareForOtaResumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Prepare_For_Ota_Resume", {},
            std::bind(&FileSystemCommandMgr::prepareForOtaResume, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> startAbSyncCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "7", "Start_AbSync", {}, std::bind(&FileSystemCommandMgr::startAbSync, myFsCmdMgr_)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> fileSystemTestAppCommands
        = {startEfsBackupCommand, prepareForEcallCommand, eCallCompletedCommand,
            prepareForOtaStartCommand, otaCompletedCommand, prepareForOtaResumeCommand,
            startAbSyncCommand};

    ConsoleApp::addCommands(fileSystemTestAppCommands);
    ConsoleApp::displayMenu();
}

/**
 * Main routine
 */
int main(int argc, char **argv) {
    Status ret = Status::FAILED;
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << APP_NAME << "Adding supplementary groups failed!" << std::endl;
    }
    fileSystemTestApp_ = std::make_shared<FileSystemTestApp>();
    if (0 != fileSystemTestApp_->init()) {
        std::cout << APP_NAME << " Failed to initialize the File system management service"
                  << std::endl;
        return -1;
    }
    signal(SIGINT, signalHandler);
    ret = fileSystemTestApp_->parseArguments(argc, argv);
    if (ret != Status::SUCCESS) {
        return -1;
    }
    fileSystemTestApp_->consoleinit();
    fileSystemTestApp_->mainLoop();
    std::cout << "Exiting application..." << std::endl;
    fileSystemTestApp_->cleanup();
    fileSystemTestApp_ = nullptr;
    return 0;
}
