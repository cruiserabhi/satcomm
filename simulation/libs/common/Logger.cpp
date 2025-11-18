/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <limits.h>
}

#include <algorithm>
#include <vector>
#include <thread>
#include "Logger.hpp"
#include "CommonUtils.hpp"
#include "SimulationConfigParser.hpp"

#define STAT_FAILURE -1

#define DEFAULT_LOG_FILE_NAME "tel.log"

#define DEFAULT_LOG_FILE_MAX_SIZE 5 * 1024 * 1024  // 5 MB
static constexpr uint8_t UMASK_BITS = 0002;

namespace telux {
namespace common {

Logger Logger::instance;

Logger &Logger::getInstance() {
   return instance;
}

Logger::Logger() {
}

Logger::~Logger() {
   std::lock_guard<std::mutex> lock(logFileMutex_);
   logStatus_.store(LoggerStatus::NOT_AVAILABLE);
   // Close log file stream if it is open
   if(logFileStream_.is_open()) {
      logFileStream_.close();
   }
}

void Logger::initProcessId() {
   processID_ = getpid();
}

void Logger::initProcessName() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    std::string fullPath = std::string(path, (count > 0) ? count : 0);
    auto const pos = fullPath.find_last_of('/');
    processName_ = fullPath.substr(pos + 1);
}

void Logger::init() {
    // set required data member by reading configuration file
    config_ = std::make_shared<SimulationConfigParser>();
    initFileLogLevel();
    initConsoleLogLevel();
    initSyslogLogLevel();
    initComponentLogging();
    initLogFileName();
    initLogFileMaxSize();
    initDateTime();
    initProcessName();
    initProcessId();

    // initialize logging
    initFileLogging();
    initConsoleLogging();
    updateMaxLogLevel();
}

void Logger::initComponentLogging() {
   //Getting the value from tel.conf
   std::string tempStr = config_->getValue(std::string("TELUX_LOG_COMPONENT_FILTER"));

   if (!tempStr.empty()) {
      std::vector<std::string> logFilter = CommonUtils::splitString(tempStr, ',');
      for(auto s:logFilter) {
         int vertical = stoi(s);
         componentLogFilter_.set(vertical);
      }
   }
}

void Logger::initFileLogging() {
   if(fileLogLevel_ == LogLevel::LEVEL_NONE) {
      isLoggingToFileEnabled_ = false;
   }

   isLoggingToFileEnabled_ = true;
   // Initialize the log file
   if(!logFileStream_.is_open()) {
      // Open the log file for writing
      mode_t mask = umask(UMASK_BITS);
      std::string logFile = logFileFullName_;
      logFileStream_.open(logFile, std::ios::app);
      if(logFileStream_.rdstate() != std::ios_base::goodbit) {
          syslog(LOG_NOTICE, "%s open %s fail", __FUNCTION__, logFile.c_str());
          umask(mask);
          return;
      }
      struct stat st;
      if (stat(logFile.c_str(), &st) != STAT_FAILURE) {
         inodeNumber_ = st.st_ino;
      }
      umask(mask);
   }
}

void Logger::initConsoleLogging() {
   if(consoleLogLevel_ == LogLevel::LEVEL_NONE) {
      isLoggingToConsoleEnabled_ = false;
   }
   isLoggingToConsoleEnabled_ = true;
}

void Logger::initLogFileName() {
   logFileFullName_ = config_->getValue(std::string("LOG_FILE_PATH"));

   if (logFileFullName_.length() > 0)
      logFileFullName_ += "/";

   std::string logFileName = config_->getValue(std::string("LOG_FILE_NAME"));
   if(logFileName != "") {
      logFileFullName_ += logFileName;
   } else {
      logFileFullName_ += DEFAULT_LOG_FILE_NAME;  // Default log file name if not configured
   }
}

void Logger::initLogFileMaxSize() {
   std::string val = config_->getValue(std::string("MAX_LOG_FILE_SIZE"));
   logFileMaxSize_ = DEFAULT_LOG_FILE_MAX_SIZE;

   if(!val.empty()) {
      logFileMaxSize_ = stoi(val);
   }
}

// Routine Provides time stamp in Nano Seconds
void Logger::getTimeStampNs(struct timespec *tp) {
   tp->tv_sec = tp->tv_nsec = 0;
   clock_gettime(CLOCK_BOOTTIME, tp);
}

const std::string Logger::getCurrentTime() {
   std::tm tmSnapshot;
   std::stringstream ss;
   // Get the current calendar time (Epoch time)
   std::time_t nowTime = std::time(nullptr);

   // std::put_time to get the date and time information from a given calendar time.
   // ::localtime_r() converts the calendar time to broken-down time representation
   // but stores the data in a user-supplied struct, and it is thread safe
   ss << std::put_time(::localtime_r(&nowTime, &tmSnapshot), "%b-%d-%Y %H:%M:%S");
   return ss.str();
}

void Logger::initDateTime() {
   std::string val = config_->getValue(std::string("LOG_PREFIX_DATE_TIME"));
   if(!val.empty()) {
      if(val == "TRUE") {
         isDateTimeEnabled_ = true;
      } else if(val == "FALSE") {
         isDateTimeEnabled_ = false;
      }
   }
}

void Logger::initConsoleLogLevel() {
   consoleLogLevel_ = LogLevel::LEVEL_INFO;
   std::string logLevelString = config_->getValue(std::string("CONSOLE_LOG_LEVEL"));
   if(!logLevelString.empty()) {
      consoleLogLevel_ = getLogLevel(logLevelString);
   }
}

LogLevel Logger::getConsoleLogLevel() {
   return consoleLogLevel_;
}

void Logger::initFileLogLevel() {
   fileLogLevel_ = LogLevel::LEVEL_INFO;
   std::string logLevelString = config_->getValue(std::string("FILE_LOG_LEVEL"));
   if(!logLevelString.empty()) {
      fileLogLevel_ = getLogLevel(logLevelString);
   }
}

LogLevel Logger::getFileLogLevel() {
   return fileLogLevel_;
}

void Logger::initSyslogLogLevel() {
   syslogLogLevel_ = LogLevel::LEVEL_DEBUG;
   std::string logLevelString = config_->getValue(std::string("SYSLOG_LOG_LEVEL"));
   if(!logLevelString.empty()) {
      syslogLogLevel_ = getLogLevel(logLevelString);
   }
}

LogLevel Logger::getSyslogLogLevel() {
   return syslogLogLevel_;
}

void Logger::writeToConsole(std::ostringstream &outputStream) {
   // newline applied will flush the buffer to stdout
   std::cout << outputStream.str() << std::endl;
}

/**
 * If log file size larger than specified max allowed size then backup the log file and reopen it
 * so log prints to the new log file rather than the backup log file.
 * If the log file inode number changed which means the log file was backup by someone
 * else, reopen it so log message prints to expected log file.
 * Write log message to specified file if the fstream has goodbit, otherwise prints to syslog;
 */
void Logger::writeToFile(std::ostringstream &outputStream) {
    /*
     * The flock used during backup synchronizes processes and not threads.
     * Also, need to protect log file stream when it is accessed by multiple threads.
     */
    std::lock_guard<std::mutex> lock(logFileMutex_);
    if (logStatus_ != LoggerStatus::AVAILABLE) {
        return;
    }
    bool logFileChanged = false;
    struct stat st;
    if (stat(logFileFullName_.c_str(), &st) == STAT_FAILURE) {
        return;
    }

    if(!logFileChanged) {
        if (st.st_size > logFileMaxSize_) {
            logFileChanged = backupLogFile();
        }
        if (inodeNumber_ != st.st_ino) {
            logFileChanged = true;
        }
    }
    //Updating the iNode number after a successful backup.
    if(logFileChanged) {
        inodeNumber_ = reopenLogFile();
    }

    if(logFileStream_.rdstate() == std::ios_base::goodbit) {
        // Write the log message into the file
        logFileStream_ << outputStream.str() << std::endl;
    } else {
        syslog(LOG_NOTICE, "%s", outputStream.str().c_str());
    }
}

void Logger::writeToSyslog(std::ostringstream &outputStream, LogLevel logLevel) {
   std::string logMessage = outputStream.str();
   switch(logLevel) {
      /*
       * Mapping of log levels in syslog
       * Perf logs are mapped to LOG_CRIT.
       * Error logs are mapped to LOG_ERR.
       * Warning logs are mapped to LOG_WARNING.
       * Info logs are mapped to LOG_INFO.
       * Debug logs are mapped to LOG_DEBUG.
       */

      case LogLevel::LEVEL_PERF:
         syslog(LOG_CRIT, "%s", logMessage.c_str());
         break;
      case LogLevel::LEVEL_ERROR:
         syslog(LOG_ERR, "%s", logMessage.c_str());
         break;
      case LogLevel::LEVEL_WARNING:
         syslog(LOG_WARNING, "%s", logMessage.c_str());
         break;
      case LogLevel::LEVEL_INFO:
         syslog(LOG_INFO, "%s", logMessage.c_str());
         break;
      case LogLevel::LEVEL_DEBUG:
         syslog(LOG_DEBUG, "%s", logMessage.c_str());
         break;
      default:
         break;
   }
}

void Logger::writeLogMessage(std::ostringstream &os, LogLevel logLevel, const std::string &fileName,
                             const int &component, const std::string &lineNo) {
   std::string timeStamp = "";
   std::string fileNameAndLineNo = "";
   std::string processIdAndName = "";
   std::ostringstream outputStream;
   struct timespec tp;
   tp.tv_sec = tp.tv_nsec = 0;

   // Get current date and time from system if LOG_PREFIX_DATE_TIME flag enabled
   if(isDateTimeEnabled_) {
      timeStamp = " " + this->getCurrentTime();
   }

   // get process id and name
   processIdAndName = std::to_string(processID_) + "/" + processName_;

   // get the filename from full path
   const char *lastSlash = std::strrchr(fileName.c_str(), '/');
   if(lastSlash != nullptr) {
      fileNameAndLineNo = " " + std::string(lastSlash + 1) + "(" + lineNo + ") ";
   } else {
      fileNameAndLineNo = " " + std::string(fileName) + "(" + lineNo + ") ";
   }

   switch(logLevel) {
      case LogLevel::LEVEL_ERROR:
         outputStream << "[E]" << timeStamp << " " << processIdAndName << fileNameAndLineNo;
         break;
      case LogLevel::LEVEL_WARNING:
         outputStream << "[W]" << timeStamp << " " << processIdAndName << fileNameAndLineNo;
         break;
      case LogLevel::LEVEL_INFO:
         outputStream << "[I]" << timeStamp << " " << processIdAndName << fileNameAndLineNo;
         break;
      case LogLevel::LEVEL_DEBUG:
         outputStream << "[D]" << timeStamp << " " << processIdAndName << fileNameAndLineNo;
         break;
      case LogLevel::LEVEL_PERF:
         // Get current time in nano second from BOOT
         this->getTimeStampNs(&tp);
         outputStream << "[TS]" << timeStamp << " " << tp.tv_sec << "." << tp.tv_nsec << " " <<
         processIdAndName << fileNameAndLineNo;
         break;
      default:
         break;
   }

   // Print thread id for debugging
   outputStream << std::this_thread::get_id() << ": ";

   //Check if the ostringstream containing the input argument is empty
   if(!os.str().empty()) {
      outputStream << os.str();

      if(consoleLogLevel_ >= logLevel) {
         writeToConsole(outputStream);
      }

      // Don't log into file unless logging to file is enabled
      if(fileLogLevel_ >= logLevel) {
         writeToFile(outputStream);
      }

      if(syslogLogLevel_ >= logLevel) {
         writeToSyslog(outputStream, logLevel);
      }
   }
}

LogLevel Logger::getLogLevel(std::string logLevelString) {
   LogLevel logLevel = LogLevel::LEVEL_DEBUG;  // default log level
   if(logLevelString == "PERF"){
      logLevel = LogLevel::LEVEL_PERF;
   }else if(logLevelString == "ERROR") {
      logLevel = LogLevel::LEVEL_ERROR;
   } else if(logLevelString == "WARNING") {
      logLevel = LogLevel::LEVEL_WARNING;
   } else if(logLevelString == "INFO") {
      logLevel = LogLevel::LEVEL_INFO;
   } else if(logLevelString == "NONE") {
      logLevel = LogLevel::LEVEL_NONE;
   }
   return logLevel;
}

bool Logger::isComponentLogged(const int component) {

   bool shouldLog = false;

   // componentLogFilter_ 0th bit set handle case when all logs are printed.
   // componentLogFilter_.test(component) handle case when component specified in configuration
   // component = 0 handle case when app uses using logger api.
   if (componentLogFilter_.test(0) || componentLogFilter_.test(component) || component == 0) {
      shouldLog = true;
   }
   return shouldLog;
}

void Logger::updateMaxLogLevel() {
   maxLogLevel_ =
      std::max({consoleLogLevel_, fileLogLevel_, syslogLogLevel_});
}

bool Logger::isLoggingEnabled(LogLevel logLevel, const int& component) {

   //When component-filtering is enabled then the logs from that technology domain, the logs from
   //common & qmi domain and the apps logs are also printed.
   bool allowComponent = isComponentLogged(component);
   bool allowedLogging = maxLogLevel_ >= logLevel;
   return (allowedLogging && allowComponent);
}

/**
 * Close the log file
 * Clear the error bits
 * Reopen the log file in append mode
 * Return the inode number of the file we opened
 */
ino_t Logger::reopenLogFile() {
   struct stat st;
   if (logFileStream_.is_open()) {
      logFileStream_.close();
   }
   logFileStream_.clear();

   logFileStream_.open(logFileFullName_, std::ios::app);
   if (logFileStream_.rdstate() == std::ios_base::goodbit) {
       stat(logFileFullName_.c_str(), &st);
       return st.st_ino;
   }
   return 0;
}

int Logger::acquireLock(int &fileDescriptor) {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;

    fileDescriptor = open(logFileFullName_.c_str(), O_RDWR, 0664);
    if (fileDescriptor < 0) {
        syslog(LOG_ERR, "%s File open fail %s %d", __FUNCTION__, strerror(errno), errno);
        return -errno;
    }
    int ret = fcntl(fileDescriptor, F_SETLK, &lock);
    if (ret < 0) {
        close(fileDescriptor);
        if(errno == EACCES || errno == EAGAIN) {
            return -EAGAIN;
        }
        syslog(LOG_ERR, "%s Can't acquire lock: %s %d", __FUNCTION__, strerror(errno), errno);
        return -errno;
    } else {
        struct stat st;
        if (stat(logFileFullName_.c_str(), &st) == STAT_FAILURE) {
            close(fileDescriptor);
            return -errno;
        }
        if (inodeNumber_ != st.st_ino) {
            close(fileDescriptor);
            syslog(LOG_DEBUG, "%s Likely new tel.log file has been created", __FUNCTION__);
            // log file has been backup and recreated by another process
            // which acquire the lock firstly
            return -EAGAIN;
        }
    }
    return 0;
}

/**
 * The backup is performed in the following-
 * 1. Acquire lock. Here flock is used to lock files. It synchronizes processes and not threads.
 *    To prevent race between threads, the entire write to file is guarded using a mutex.
 * 2. If acquire is a success, rename existing file by appending ".backup" and create a new file
 *    in the same location.
 * 3. After backup is complete, close the file descriptor.
 *    This helps in releasing the lock held by the process.
 *    Different processes have independent locks associated with an [i-node, pid] pair
 *    instead of a file object. So closing fd in one process does not affect other processes.
 *
 * If process A and B enter the backup operation, process A acquires the lock, process B bails
 * out with -EAGAIN. At the caller end, process B compares the current file's iNode with
 * the member variable iNode. If there is a difference, this implies that
 * process A successfully backed up the file.
 *
 */
bool Logger::backupLogFile() {
    int ret, fileDescriptor;
    ret = acquireLock(fileDescriptor);
    if(ret < 0) {
        if(ret == -EAGAIN) {
            syslog(LOG_ERR, "%s File locked by another process", __FUNCTION__);
        } else {
            syslog(LOG_ERR, "%s File Lock Acquire failed", __FUNCTION__);
        }
        return false;
    } else {
        std::string backupFileName = logFileFullName_ + ".backup" ;
        rename(logFileFullName_.c_str(), backupFileName.c_str());
        std::ofstream logStream;
        mode_t mask = umask(UMASK_BITS);
        logStream.open(logFileFullName_.c_str(), std::ofstream::out | std::ofstream::trunc);
        logStream.close();
        umask(mask);
        close(fileDescriptor);
        return true;
    }
}

}
}

bool Log::isLoggingEnabled(LogLevel logLevel, const int& component) {
   Logger &logger = Logger::getInstance();
   return logger.startLogger() && logger.isLoggingEnabled(logLevel, component);
}

void Log::logStream(std::ostringstream&  outputStream, LogLevel logLevel,
   const std::string &fileName, const std::string &lineNo, const int &component) {
   Logger &logger = Logger::getInstance();
   logger.writeLogMessage(outputStream, logLevel, fileName, component, lineNo);
}
