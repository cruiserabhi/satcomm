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

#ifndef CONSOLEAPP_HPP
#define CONSOLEAPP_HPP

#include <string>
#include <vector>

#include "ConsoleAppCommand.hpp"

/**
 * ConsoleApp provides skeleton implementation to display a list of commands,
 * read user input and trigger commands.
 */
class ConsoleApp {
public:
   /**
    * Displaying menu of supported commands
    */
   void displayMenu();

   /**
    * Add commands into supported commands list
    */
   void addCommands(std::vector<std::shared_ptr<ConsoleAppCommand>> supportedCommandsList);

   /**
    * Initialize commands and display
   */
   void init(){};

   /**
    * Main loop to display commands, read user input and execute the commands
   */
   int mainLoop();

   ConsoleApp(std::string appName, std::string cursor);

private:
   /**
    * A list of commands supported by the ConsoleApp
    */
   std::vector<std::shared_ptr<ConsoleAppCommand>> supportedCommands_;
   std::string appName_, cursor_;

   /**
    * Display cursor to read user input
    */
   void displayCursor();

   /**
    * Display the title banner
    */
   void displayBanner();

   /**
    * Read user input from command line
    */
   std::vector<std::string> readCommand();

   /**
    * Get console app command from user input
    */
   std::shared_ptr<ConsoleAppCommand>
      getAppCommandFromUserInput(std::vector<std::string> inputCommand);
};

#endif  // CONSOLEAPP_HPP
