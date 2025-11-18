/*
 *  Copyright (c) 2018-2019 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <sstream>

#include "ConsoleApp.hpp"
#include "ConsoleAppCommand.hpp"

const std::string MENU_DIVIDER = "------------------------------------------------";

ConsoleApp::ConsoleApp(std::string appName, std::string cursor)
   : appName_(appName)
   , cursor_(cursor) {
}

/**
 * Displaying Menu of Applications Requested
 */
void ConsoleApp::displayMenu() {
   displayBanner();
   // Iterate through the supportedCommands_ list and display all the commands
   for(auto command : supportedCommands_) {
      command->displayCommand();
   }
   std::cout << std::endl;
   std::cout << "   ? / h - help" << std::endl;
   std::cout << "   q / 0 - exit" << std::endl << std::endl;
   std::cout << MENU_DIVIDER << std::endl << std::endl;
}

/**
 * Display Cursor to Read User Input
 */
void ConsoleApp::displayCursor() {
   std::cout << cursor_;
}

/**
 * Display the title banner
 */
void ConsoleApp::displayBanner() {
   std::cout << MENU_DIVIDER << std::endl;
   int paddingLength = MENU_DIVIDER.length() / 2 + appName_.length() / 2;
   std::cout << std::setw(paddingLength) << appName_ << std::endl;
   std::cout << MENU_DIVIDER << std::endl << std::endl;
}

/**
 * Read user request from command line
 */
std::vector<std::string> ConsoleApp::readCommand() {
   ConsoleApp::displayCursor();
   // input string
   std::string command;

   std::getline(std::cin, command);
   if (std::cin.fail() || std::cin.bad() || std::cin.eof()) {
      std::cin.clear();
      std::cin.ignore();
      std::cout << "\ncin has entered bad state" << std::endl;
      command = "quit";
   }

   // separate input string based on whitespace
   std::istringstream iss(command);

   // iterate on a stream and store collection of substring into vector of strings
   std::vector<std::string> userInput(std::istream_iterator<std::string>{iss},
                                      std::istream_iterator<std::string>());
   return userInput;
}

inline bool operator==(const std::shared_ptr<ConsoleAppCommand> &command,
                       const std::vector<std::string> &inputCommand) {
    std::string cmdStr = command->getName();
    std::string inpStr = inputCommand[0];
    // Function to convert to lowercase
    std::transform(cmdStr.begin(), cmdStr.end(), cmdStr.begin(), ::tolower);
    std::transform(inpStr.begin(), inpStr.end(), inpStr.begin(), ::tolower);

    if(command->getId() == inpStr || cmdStr == inpStr) {
        if(command->getArguments().size() == (inputCommand.size() - 1)) {
            return true;
        }
    }

    // Tokenize command name to a vector.
    std::istringstream iss(command->getName());

    // iterate on a stream and store collection of substring into vector of strings
    std::vector<std::string> commandTokens(std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>());

    /**
     * There can be 3 cases of passing a command.
     *
     * 1. Just command name separated by '_'. For example -
     * Start_Basic_Reports (commandName)
     * Start_Basic_Reports (inputCommand)
     * This case is already handled in the above checks.
     * We also check if the inputID is the same as command ID in the above check.
     * Later, the argument list is also compared for both the cases.
     *
     * 2. We can have space separated commands. For example -
     * Request terrestrial positioning (commandTokens)
     * Request terrestrial positioning (inputCommand)
     * If the vector size is same, we try matching each token of the command name.
     *
     * 3. Space separated commands with arguments. For example -
     * Make Call           (commandTokens)
     * Make Call <number>  (inputCommand)
     * So, if the inputCommand size > commandName, we try matching each token of the command name.
     * Next, we check whether the argument size is the same as the expected size and return.
     *
     * 4. If the inputCommand size == commandName but if command expects arguments and input
     * command doesn't provide arguments, we should return false.
     * Hangup <index>   (commandName expecting index as argument)
     * Hangup           (inputCommand didn't provide argument)
     * This should return false.
     */
    if(inputCommand.size() >= commandTokens.size()) {
        for(size_t i = 0; i < commandTokens.size(); i++) {
            if(inputCommand[i] != commandTokens[i]) {
                return false;
            }
        }
        if(inputCommand.size() > commandTokens.size()) {
            size_t argumentSize = inputCommand.size() - commandTokens.size();
            if(argumentSize != command->getArguments().size()) {
                return false;
            }
        }
        if((inputCommand.size() == commandTokens.size()) &&
            (command->getArguments().size() >= 1)) {
            return false;
        }
        return true;
    }

   return false;
}
/**
 * Get console app command from user input
 */
std::shared_ptr<ConsoleAppCommand>
   ConsoleApp::getAppCommandFromUserInput(std::vector<std::string> inputCommand) {
   if(inputCommand.size() > 0) {
      for(auto command : supportedCommands_) {
         if(command == inputCommand) {
            return command;
         }
      }
   }
   return nullptr;
}

/**
 * Add  commands into supportCommands_ list
 */
void ConsoleApp::addCommands(std::vector<std::shared_ptr<ConsoleAppCommand>> supportedCommandsList) {
   for(auto command : supportedCommandsList) {
      supportedCommands_.emplace_back(command);
   }
}

int ConsoleApp::mainLoop() {
   while(true) {
      std::vector<std::string> userInput = readCommand();
      if(userInput.size() == 0)
         continue;
      if(userInput[0] == "0" || userInput[0] == "exit" || userInput[0] == "q"
         || userInput[0] == "quit" || userInput[0] == "back") {
         break;
      } else if(userInput[0] == "?" || userInput[0] == "help" || userInput[0] == "h") {
         displayMenu();
         continue;
      }

      std::shared_ptr<ConsoleAppCommand> conAppCmd = getAppCommandFromUserInput(userInput);
      if(conAppCmd) {
         conAppCmd->executeCommand(userInput);
      } else {
         std::cout << "Invalid command: " << userInput[0] << " entered." << std::endl;
         std::cout << "Please enter valid command and arguments." << std::endl;
      }
   }
   return 0;
}
