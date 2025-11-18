/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "SasquishUtils.hpp"

bool SasquishUtils::sasquishVerbose = false;
SasquishInputHandler::SasquishInputHandler(){}
SasquishInputHandler::SasquishInputHandler(std::string fileName){
    openFile(fileName);
}
SasquishInputHandler::~SasquishInputHandler(){
    closeFile();
}
bool SasquishInputHandler::openFile(std::string fileName){
    // make sure path is valid
    // attempt to open but close first
    closeFile();
    logFile.open(fileName);
    return true;
}
bool SasquishInputHandler::clearFile(std::string fileName){
    if(!logFile.is_open()){
        return false;
    }
    return true;
}

bool SasquishInputHandler::closeFile(){
    if(!logFile.is_open()){
        return false;
    }
    logFile.close();
    return true;
}

bool SasquishInputHandler::readLineFromFile(std::string& line){
    if(!logFile.is_open()){
        return false;
    }
    // read a line from filename into the provided string
    if(std::getline(logFile, line)){
        return true;
    }
    return false;
}

SasquishOutputHandler::SasquishOutputHandler(){}
SasquishOutputHandler::SasquishOutputHandler(std::string fileName){
    openFile(fileName);
}
SasquishOutputHandler::~SasquishOutputHandler(){
    closeFile();
}
bool SasquishOutputHandler::openFile(std::string fileName){
    // make sure path is valid
    // attempt to open but close first
    closeFile();
    logFile.open(fileName);
    return true;
}
bool SasquishOutputHandler::clearFile(std::string fileName){
    if(!logFile.is_open()){
        return false;
    }
    return true;
}

bool SasquishOutputHandler::closeFile(){
    if(!logFile.is_open()){
        return true;
    }
    logFile.close();
    return true;
}

bool SasquishOutputHandler::writeLineToFile(std::string line){
    if(!logFile.is_open()){
        return false;
    }
    // may need some boundary check for the line
    logFile << line;
    return true;
}

/*
 * Class with generic utility functions related to
 * time, command line handling, and others.
 * Logging functionality (debug purposes) as well.
 */

void SasquishUtils::setSasquishVerbose(bool verbose){
    sasquishVerbose = verbose;
}

bool SasquishUtils::getSasquishVerbose(){
    return sasquishVerbose;
}

uint16_t SasquishUtils::delimiterPos(string line, vector<string> delimiters) {
    uint16_t pos = MAX_DELIMIT_VALUE; //Largest possible value of 16 bits.
    for (long unsigned int i = 0; i < delimiters.size(); i++)
    {
        uint16_t delimiterPos = line.find(delimiters[i]);
        if (pos > delimiterPos) {
            pos = delimiterPos;
        }
    }
    return pos;
}
void SasquishUtils::getInput(std::string prompt, std::string &input) {
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

uint64_t SasquishUtils::getTimeStampNs() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000LL;
}

uint64_t SasquishUtils::getTimeStampMs() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int SasquishUtils::setTimerFd(int timerfd, long long interval_ns) {
    struct itimerspec its = { 0 };
    if (timerfd < 0) {
        std::cerr << "Bad timer file descriptor\n";
        return -1;
    }
    its.it_value.tv_sec = interval_ns / 1000000000LL;
    its.it_value.tv_nsec = interval_ns % 1000000000LL;
    its.it_interval = its.it_value;
    if (timerfd_settime(timerfd, 0, &its, NULL) < 0) {
        std::cerr << "Error setting time\n";
        close(timerfd);
        return -1;
    }
    return 0;
}

/*
 * Creates and returns a timer
 */
int SasquishUtils::createTimer(long long interval_ns){
    int timerfd;
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    setTimerFd(timerfd, interval_ns);
    return timerfd;
}

std::string SasquishUtils::getCurrentTimestampStr()
{
    using std::chrono::system_clock;
    auto currentTime = std::chrono::system_clock::now();
    char buffer[MAX_TIMESTAMP_BUFFER_SIZE];
    auto sinceEpoch = currentTime.time_since_epoch().count() / 1000000;
    auto millis = sinceEpoch % 1000;
    std::time_t tt = system_clock::to_time_t ( currentTime );
    auto timeinfo = localtime (&tt);
    int ret = strftime (buffer,80,"%F-%H:%M:%S.",timeinfo);
    snprintf(&buffer[ret], MAX_TIMESTAMP_BUFFER_SIZE, "%03d", (int)millis);
    return std::string(buffer);
}