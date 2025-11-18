/*
 *  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#ifndef CONSOLEAPPCOMMAND_HPP
#define CONSOLEAPPCOMMAND_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

/**
 * This class provides a generic way of creating a menu command for a given Telematics SDK API.
 * It is intended to be used in console applications derived from Console App Framework.
 */
class ConsoleAppCommand {
public:
   ConsoleAppCommand(std::string id, std::string name, std::vector<std::string> arguments,
                     std::function<void(std::vector<std::string> &)> command);

   /**
    * Get command id
    */
   std::string getId();

   /**
    * Get command name
    */
   std::string getName();

   /**
    * Get command arguments
    */
   std::vector<std::string> getArguments();

   /**
    * Display the command
    */
   void displayCommand();

   /**
    * Executes the command
    */
   void executeCommand(std::vector<std::string> userInput);

private:
   std::string id_;                      // Command id
   std::string name_;                    // Command  name
   std::vector<std::string> arguments_;  // Input arguments sequence
   std::function<void(std::vector<std::string> &)> command_;
};

#endif  // CONSOLEAPPCOMMAND_HPP
