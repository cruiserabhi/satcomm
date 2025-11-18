/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <cstring>
#include <bitset>
#include <future>

#include <telux/common/Log.hpp>

class SimulationConfigParser;

using namespace telux::common;

namespace telux {
namespace common {

/**
 * Logger class - A singleton class which provides interface to log messages to
 *                a console, diag and to a log file.
 * Log level is configurable
 */
class Logger {

public:
   static Logger &getInstance();

   //getXxxLogLevel functions are utilized in the test cases.
   /*
    * Get the current console logging level
    */
   LogLevel getConsoleLogLevel();

   /*
    * Get the file console logging level
    */
   LogLevel getFileLogLevel();

   /*
    * Get the syslog logging level
    */
   LogLevel getSyslogLogLevel();

   /*
    * write log a message to console, diag and a log file based on the settings.
    */
   void writeLogMessage(std::ostringstream &os, LogLevel logLevel, const std::string &fileName,
                        const int &component, const std::string &lineNo);

   /*
    * get the current date and time of the device
    */
   const std::string getCurrentTime();

   /*
    * start logging, it will initialize Logger dependencies for the first log request.
    *
    */
   inline bool startLogger();
   /*
    * Singleton implementation, copy constructors are disabled.
    */
   Logger(const Logger &) = delete;

   Logger();
   ~Logger();

   /*
    * To check if logging is allowed for the given level and component
    * at least on one log sink
    */
   bool isLoggingEnabled(LogLevel logLevel, const int& component);

private:

   /*
    * Logger status
    */
   enum class LoggerStatus {
       INIT,          // default, initial status from start up,
                      // need to call Logger::init() to make logger work
       NOT_AVAILABLE, // Logging functionality disabled
       AVAILABLE,     // Able to use Logger for logging
   };

   /*
    * initialization of logging
    */
   void init();

   /*
    * set log file max size
    */
   void initLogFileMaxSize();

   /*
    * Get log level from given level string
    */
   LogLevel getLogLevel(std::string logLevelString);

   /*
    * set log file name
    */
   void initLogFileName();

   /*
    * set console logging level to a desired threshold
    */
   void initConsoleLogLevel();

   /*
    * set file logging level to a desired threshold
    */
   void initFileLogLevel();

   /*
    * set syslog logging level to a desired threshold
    */
   void initSyslogLogLevel();

   /*
    * set component filter to log a desired component
    */
   void initComponentLogging();

   /*
    * set date and time
    */
   void initDateTime();

   /*
    * To provide time stamp with Nano Seconds
    */
   void getTimeStampNs(struct timespec *tp);

   /*
    * initialize log file
    */
   void initFileLogging();

   /*
    * initialize logs to console
    */
   void initConsoleLogging();

   /*
    * write logs message to console
    */
   void writeToConsole(std::ostringstream &outputStream);

   /*
    * write log message to file
    */
   void writeToFile(std::ostringstream &outputStream);

   /*
    * write log message to syslog
    */
   void writeToSyslog(std::ostringstream &outputStream, LogLevel logLevel);

   /*
    * set pid
    */
   void initProcessId();

   /*
    * set process name
    */
   void initProcessName();

   /*
    * check if this component should be logged
    */
   bool isComponentLogged(const int component);

   /*
    * Perform current file backup.
    */
   bool backupLogFile();

   /*
    * Acquire lock on the log file.
    */
   int acquireLock(int &fileDescriptor);

   ino_t reopenLogFile();

   /*
    * Update max log level.
    */
   void updateMaxLogLevel();

   static Logger instance;

   std::atomic<LoggerStatus> logStatus_{LoggerStatus::INIT};
   LogLevel consoleLogLevel_, fileLogLevel_;
   LogLevel syslogLogLevel_;
   LogLevel maxLogLevel_ ;
   std::ofstream logFileStream_;
   std::mutex logFileMutex_;
   std::shared_ptr<SimulationConfigParser> config_;

   bool isLoggingToFileEnabled_ = false;
   bool isLoggingToConsoleEnabled_ = false;
   bool isDateTimeEnabled_ = false;
   std::string logFileFullName_;
   int logFileMaxSize_;
   int processID_;
   std::bitset<64> componentLogFilter_;
   std::string processName_;
   ino_t inodeNumber_ = 0;
};

bool Logger::startLogger() {
    if (logStatus_ == LoggerStatus::AVAILABLE) {
        return true;
    }

    if (logStatus_ == LoggerStatus::NOT_AVAILABLE) {
        return false;
    }

    if (logStatus_ == LoggerStatus::INIT) {
        init();
        logStatus_.store(LoggerStatus::AVAILABLE);
    }

    return true;
}
}
}

#endif
