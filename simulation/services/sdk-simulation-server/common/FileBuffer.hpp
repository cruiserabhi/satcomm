/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file FileBuffer.hpp
 *
 * @brief This class performs file buffering for example csv buffering. After parsing
 *        configured number of lines (threshold) lines from the file,
 *        it updates the read buffer and waits for a signal from the streaming
 *        thread to prepare the next buffer.
 *
 */

#ifndef FILE_BUFFER_HPP
#define FILE_BUFFER_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include "libs/common/AsyncTaskQueue.hpp"

class FileBuffer {
  public:
    /**
     * Constructor to initialize file path and threshold value.
     *
     * @param [in] filePath - Complete file path to be buffered.

     * @param [in] thresholdValue - The max size of the read buffer.
     *
     */
    FileBuffer(std::string filePath, int thresholdValue);

    ~FileBuffer();

    /**
     * This API starts the buffering operation for the given file
     * asynchronously via the startBufferingSync() API.
     *
     */
    void startBuffering();

    void cleanup();

    /**
     * This API fetches the read buffer for the streaming thread.
     */
    inline std::vector<std::string>& getReadBuffer() {
        return readBuffer_;
    }

    /**
     * This API sets the read buffer for the streaming thread.
     */
    void setReadBuffer(std::vector<std::string>& readBuffer) {
        readBuffer_ = readBuffer;
    }

    /**
     * This API sets whether the buffer thread can read the next batch.
     */
    void setReadNextBatch(bool readNextBatch) {
        readNextBatch_ = readNextBatch;
    }

    /**
     * This API fetches whether the EOF is reached.
     */
    inline bool& getReachedEOF() {
        return reachedEOF_;
    }

    /**
     * This API sets the EOF value.
     */
    void setReachedEOF(bool reachedEOF) {
        reachedEOF_ = reachedEOF;
    }

    /**
     * This API fetches whether the streaming thread can stream the read buffer.
     */
    inline bool& getStreamCurrentBuffer() {
        return streamCurrentBatch_;
    }

    /**
     * Mutexes and condition variables to synchronize streaming and buffering.
     */
    inline std::mutex& getStreamBufferMtx() {
        return streamBufferMtx_;
    }

    inline std::condition_variable& getStreamBufferCv() {
        return streamBufferCv_;
    }

    inline std::mutex& getNextBatchBufferMtx() {
        return nextBatchBufferMtx_;
    }

    inline std::condition_variable& getNextBatchBufferCv() {
        return nextBatchBufferCv_;
    }

    /**
     * This API returns the result after synchronizing buffering and streaming.
     */
    bool getNextBuffer(std::vector<std::string> &requestBuffer);

  private:
    /**
     * This API starts filling the read buffer line by line until the threshold lines are parsed
     * or if the EOF is reached. After filling the read buffer, it waits for a signal from the
     * streaming thread to read the next set of buffer.
     */
    void startBufferingSync();
    std::vector<std::string> readBuffer_;
    std::string fileName_;
    int threshold_ = 0;
    telux::common::AsyncTaskQueue<void> taskQ_;
    bool readNextBatch_ = false;
    bool streamCurrentBatch_ = false;
    bool reachedEOF_ = false;
    //Synchronization for streaming to be performed ONLY after the buffer data is ready.
    std::mutex streamBufferMtx_;
    std::condition_variable streamBufferCv_;

    //Synchronization for next batch of buffering to begin ONLY after the read buffer is swapped.
    std::mutex nextBatchBufferMtx_;
    std::condition_variable nextBatchBufferCv_;
};


#endif //FILE_BUFFER_HPP
