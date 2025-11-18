/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <errno.h>
#include <thread>
#include <chrono>
#include <future>
#include <functional>

#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

#include "AudioPlayerImpl.hpp"

/* #define AUDIOPLAYERIMPL_DDBG 1 */

namespace telux {
namespace audio {

/*
 * Player states and the associated state machine.
 *
 *                                                     <Enter>
 *                                                        |
 *                                                        v
 *                             ------------         -----------
 * .--------------------------|            |       |        (1)|
 * |        .---------------> |         (2)| <---- |INIT_PLAYER|
 * |        |        .------> |REPORT_ERROR|        -----------
 * |        |        |   .--> |            |            |
 * |        |        |   |     ------------             |
 * |        |        |   |        ^                     |
 * |        |        |   |        |                     |
 * |        |        |   |    ---------                 |
 * |        |        |   |   |INIT_FILE| <----------.   |
 * |        |        |   |    ---------             |   |
 * |        |        |   |        |                 |   |
 * |        |        |   |        v                 |   V
 * |        |        |   |  -------------        -----------                          O
 * |        |        |   '-|REINIT_STREAM|   .- |SELECT_FILE| ----.                 -- --
 * |        |        |      -------------    |   -----------      |                   |
 * |        |        '--------.   |          |    |   ^   ^       |                  / \
 * |        |                 |   v          |    |   |   |       |             #Explicit stop#
 * |  -------------         ------------     |    |   |   |       v                  |
 * | |FILE_PLAY_END| <---- |PLAY_SAMPLES| <--'    |   |   |    -------------         |
 * |  -------------         ------------          |   |   |   |          (4)|        v
 * |        |   |               ^                 |   |   |   |REPORT_FINISH| --> <Exit> <- #SSR#
 * |        |   |               |                 |   |   |    -------------         ^
 * |        |   '---------------'                 |   |   |                          |
 * |        v                                     |   |   |                          |
 * |  -------------        -----------            |   |   |                          |
 * | |          (3)|      |           |           |   |   |    ---------             |
 * | |REPORT_PLAYED|      |DEINIT_FILE| <---------'   |   |   |      (5)|            |
 * |  -------------        -----------                |   |   |TERMINATE| -----------'
 * |        |                   '---------------------'   |    ---------
 * |        |                                             |        ^
 * |        '---------------------------------------------'        |
 * '---------------------------------------------------------------'
 *
 * Client callbacks; called from a particular state.
 * (1) onPlaybackStarted()
 * (2) onError()
 * (3) onFilePlayed()
 * (4) onPlaybackFinished()
 * (5) onPlaybackStopped()
 */
enum class PlayerState {

    /*
     * Resources are allocated and initialized. For example; creating audio stream,
     * allocating ping-pong buffers and registering for SSR events.
     */
    INIT_PLAYER,

    /*
     * Heart of the player. Based on how to play the file (skip, count, indefinite),
     * decide to skip or schedule the file for playback.
     */
    SELECT_FILE,

    /*
     * Open a file from the client specified path.
     */
    INIT_FILE,

    /*
     * Open an audio stream. If required, existing stream is closed.
     */
    REINIT_STREAM,

    /*
     * Play audio samples.
     */
    PLAY_SAMPLES,

    /*
     * Take final steps to conclude playback of the file currently played.
     * For example; play the last 2 buffers, handle errors as applicable and
     * stop the compressed stream.
     */
    FILE_PLAY_END,

    /*
     * Inform client that a particular file has been played successfully.
     */
    REPORT_PLAYED,

    /*
     * File currently played is closed after it has been played completely.
     */
    DEINIT_FILE,

    /*
     * Marks graceful completion of the whole playback. Stream is closed, resources
     * are released, client is informed - playback completed successfully.
     */
    REPORT_FINISH,

    /*
     * Entered whenever an error is encountered during playback. Client is informed
     * that an error has occurred.
     */
    REPORT_ERROR,

    /*
     * Represents a fatal error situation. Stream is closed and resources are released.
     * Player thread is terminated.
     */
    TERMINATE
};

/*
 * Life cycle of the effective volume/mute state is tied to the player instance therefore,
 * set defaults here. An application can get/set volume/mute before calling startPlayback().
 */
AudioPlayerImpl::AudioPlayerImpl(std::shared_ptr<IAudioManager> audioManager) {

    audioManager_      = audioManager;
    applyCachedVolume_ = false;
    applyCachedMute_   = false;
    cachedDevices_.resize(0);
    lastUsedDevices_.resize(0);
}

AudioPlayerImpl::~AudioPlayerImpl() {
    LOG(DEBUG, __FUNCTION__);

    /*
     * It is not expected that a client will release AudioPlayerImpl instance
     * during an ongoing playback. But if it happens, initiate cleanup and exit.
     */
    hasUserRequestedStop_ = true;

    unblockPlayerThread(false);

    /*
     * Ensure all background threads are terminated before releasing AudioPlayerImpl
     * fully to maintain correct order of destruction and cleanup.
     */
    asyncTaskQ_.shutdown();
}

/*
 * Places a request to start playback.
 */
telux::common::ErrorCode AudioPlayerImpl::startPlayback(
    std::vector<PlaybackConfig> &playbackConfigs, std::weak_ptr<IPlayListListener> statusListener) {

    telux::common::Status status;

    {
        std::lock_guard<std::mutex> lock(playerMtx_);

        if (isPlayInProgress_) {
            /* A playback is already running */
            LOG(ERROR, __FUNCTION__, " playback in progress");
            return telux::common::ErrorCode::INVALID_STATE;
        }

        if (playbackConfigs.empty()) {
            LOG(ERROR, __FUNCTION__, " empty files list");
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }

        statusListener_  = statusListener;
        playbackConfigs_ = playbackConfigs;

        resetState();

        /* Player thread */
        auto future
            = std::async(std::launch::deferred, [this]() { this->executePlayback(); }).share();

        status = asyncTaskQ_.add(future);
        if (status != telux::common::Status::SUCCESS) {
            return telux::common::CommonUtils::toErrorCode(status);
        }

        isPlayInProgress_ = true;
        return telux::common::ErrorCode::SUCCESS;
    }
}

/*
 * Places a request to terminate the playback.
 */
telux::common::ErrorCode AudioPlayerImpl::stopPlayback() {

    {
        std::lock_guard<std::mutex> playerLock(playerMtx_);

        if (!isPlayInProgress_) {
            LOG(ERROR, __FUNCTION__, " no playback running");
            return telux::common::ErrorCode::INVALID_STATE;
        }

        hasUserRequestedStop_ = true;
        return telux::common::ErrorCode::SUCCESS;
    }
}

/*
 * Reset internal state since client can start playback again without
 * releasing the IAudioPlayer instance it already has.
 */
void AudioPlayerImpl::resetState() {
    bufferSize_            = 0;
    contentOffset_         = 0;
    curFileName_           = "";
    bufferPool_            = {};
    curFile_               = nullptr;
    isFileOpened_          = false;
    hasSsrOccurred_        = false;
    isStreamOpened_        = false;
    isCompressed_          = false;
    isAdspWriteReady_      = true;
    isStopAudioReady_      = false;
    hasUserRequestedStop_  = false;
    isStopResponseReady_   = false;
    isCreateResponseReady_ = false;
    isDeleteResponseReady_ = false;
    buffersAllocated_      = false;
    errToReport_           = telux::common::ErrorCode::SUCCESS;
}

/*
 * Identify how many bytes to skip in the given file before passing audio data to the PAL.
 * Multi-channel ("#!AMR_MC1.0\n"/"#!AMR-WB_MC1.0\n") playback is not supported.
 */
telux::common::ErrorCode AudioPlayerImpl::setFormatAndOffset(AudioFormat audioFormat) {

    switch (audioFormat) {
        case AudioFormat::AMRWB_PLUS:
            isCompressed_ = true;
            /* As per ETSI TS 126 290 V8.0.0 (2009-01) section 8.3 */
            contentOffset_ = 2;
            break;
        case AudioFormat::AMRWB:
            isCompressed_ = true;
            /* First 9 bytes in the file header are #!AMR-WB\n as per RFC4867 */
            contentOffset_ = 9;
            break;
        case AudioFormat::AMRNB:
            isCompressed_ = true;
            /* First 6 bytes in the file header are #!AMR\n as per RFC4867 */
            contentOffset_ = 6;
            break;
        case AudioFormat::PCM_16BIT_SIGNED:
            isCompressed_ = false;
            /* Every byte is data byte */
            contentOffset_ = 0;
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid fmt ", static_cast<int>(audioFormat));
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Directs what to play and how.
 */
void AudioPlayerImpl::executePlayback() {

    uint32_t curFileIdx         = 0;
    uint32_t numTimesFilePlayed = 0;

    PlaybackConfig curPbCfg{};
    PlayerState nextState       = PlayerState::INIT_PLAYER;
    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    while (!hasUserRequestedStop_ && !hasSsrOccurred_) {

#ifdef AUDIOPLAYERIMPL_DDBG
        LOG(DEBUG, __FUNCTION__, " nextState ", static_cast<int>(nextState), ", contentOffset ",
            contentOffset_, ", curFileIdx ", curFileIdx, ", numTimesFilePlayed ",
            numTimesFilePlayed, ", isFileOpened ", static_cast<int>(isFileOpened_),
            ", isStreamOpened ", static_cast<int>(isStreamOpened_), ", isCompressed ",
            isCompressed_, ", isAdspWriteReady ", isAdspWriteReady_, ", ec ", static_cast<int>(ec));
#endif

        switch (nextState) {
            case PlayerState::INIT_PLAYER:
                reportPlaybackStarted();

                curPbCfg = playbackConfigs_[curFileIdx];

                ec = registerForSSREvent();
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    errToReport_ = ec;
                    nextState    = PlayerState::REPORT_ERROR;
                    break;
                }

                writeCompleteCb_ = std::bind(&AudioPlayerImpl::writeComplete, this,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

                nextState = PlayerState::SELECT_FILE;
                break;

            case PlayerState::SELECT_FILE:
                if (curFileIdx >= playbackConfigs_.size()) {
                    /* All files played successfully */
                    nextState = PlayerState::REPORT_FINISH;
                    break;
                }

                curPbCfg = playbackConfigs_[curFileIdx];

                if (curPbCfg.repeatInfo.type == RepeatType::COUNT) {
                    if (curPbCfg.repeatInfo.count == 0) {
                        /* Zero count is same as skip */
                        ++curFileIdx;
                    } else if (numTimesFilePlayed == 0) {
                        /* First time playing this file */
                        curFileName_ = curPbCfg.absoluteFilePath;
                        nextState    = PlayerState::INIT_FILE;
                    } else if (numTimesFilePlayed < curPbCfg.repeatInfo.count) {
                        /*
                         * Play this file again.
                         * Move the file position indicator to the 1st audio sample
                         * and clear end-of-file and error indicators.
                         */
                        ec = adjustFileAndState();
                        if (ec != telux::common::ErrorCode::SUCCESS) {
                            errToReport_ = ec;
                            nextState    = PlayerState::REPORT_ERROR;
                            break;
                        }
                        nextState = PlayerState::PLAY_SAMPLES;
                    } else {
                        /* File has been played for the given iterations */
                        numTimesFilePlayed = 0;
                        ++curFileIdx;
                        nextState = PlayerState::DEINIT_FILE;
                    }
                } else if (curPbCfg.repeatInfo.type == RepeatType::SKIP) {
                    /* Skip this file */
                    ++curFileIdx;
                } else {
                    /* Play the file indefinitely */
                    if (!isFileOpened_) {
                        /* Playing file for the very first time, open it */
                        curFileName_ = curPbCfg.absoluteFilePath;
                        nextState    = PlayerState::INIT_FILE;
                    } else {
                        /* Play this file again */
                        ec = adjustFileAndState();
                        if (ec != telux::common::ErrorCode::SUCCESS) {
                            errToReport_ = ec;
                            nextState    = PlayerState::REPORT_ERROR;
                            break;
                        }
                        nextState = PlayerState::PLAY_SAMPLES;
                    }
                }
                break;

            case PlayerState::INIT_FILE:
                ec = initFileToPlay();
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    errToReport_ = ec;
                    nextState    = PlayerState::REPORT_ERROR;
                    break;
                }

                nextState = PlayerState::REINIT_STREAM;
                break;

            case PlayerState::REINIT_STREAM:
                ec = reinitAudioStream(curFileIdx);
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    nextState = PlayerState::REPORT_ERROR;
                    break;
                }

                nextState = PlayerState::PLAY_SAMPLES;
                break;

            case PlayerState::PLAY_SAMPLES:
                ec = playAudioSamples();
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    nextState = PlayerState::REPORT_ERROR;
                    break;
                }

                if (std::feof(curFile_)) {
                    nextState = PlayerState::FILE_PLAY_END;
                }
                break;

            case PlayerState::FILE_PLAY_END:
                ec = finalizePlayback();
                if (ec == telux::common::ErrorCode::SUCCESS) {
                    nextState = PlayerState::REPORT_PLAYED;
                } else if (ec == telux::common::ErrorCode::REQUEST_RATE_LIMITED) {
                    /*
                     * ADSP pipeline is full, need to resend same buffer again once
                     * ADSP is ready to accept next buffer.
                     */
                    nextState = PlayerState::PLAY_SAMPLES;
                } else {
                    nextState = PlayerState::REPORT_ERROR;
                }
                break;

            case PlayerState::REPORT_PLAYED:
                reportPlayed();

                if (curPbCfg.repeatInfo.type == RepeatType::COUNT) {
                    ++numTimesFilePlayed;
                }

                nextState = PlayerState::SELECT_FILE;
                break;

            case PlayerState::DEINIT_FILE:
                deinitFileToPlay();
                nextState = PlayerState::SELECT_FILE;
                break;

            case PlayerState::REPORT_ERROR:
                reportError(errToReport_, curFileName_);
                nextState = PlayerState::TERMINATE;
                break;

            case PlayerState::REPORT_FINISH: {
                std::lock_guard<std::mutex> playerLock(playerMtx_);

                if (hasUserRequestedStop_) {
                    /* User placed request to stop playing before we can finish playback */
                    break;
                }

                /*
                 * First cleanup internal state and then report to maintain
                 * correct order of execution and sanity of variables.
                 */
                deinitAudioStream();
                deregisterForSSREvent();
                bufferPool_           = {};
                isPlayInProgress_     = false;
                hasUserRequestedStop_ = false;
                reportPlaybackFinished();
                return;
            }

            case PlayerState::TERMINATE:
                terminatePlayback();
                reportPlaybackStopped();
                return;

            default:
                LOG(ERROR, __FUNCTION__, " invalid state ", static_cast<int>(nextState));
                errToReport_ = telux::common::ErrorCode::INVALID_STATE;
                nextState    = PlayerState::REPORT_ERROR;
                break;
        }
    }

#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__, " hasUserRequestedStop_ ", static_cast<int>(hasUserRequestedStop_),
        ", hasSsrOccurred_ ", static_cast<int>(hasSsrOccurred_));
#endif

    /*
     * (1) An error occurs, TERMINATE state is entered, cleanup is done, onPlaybackStopped()
     * is called. Player thread is pre-empted. Client calls stopPlayback() which sets
     * hasUserRequestedStop_. Since we return from TERMINATE state, player thread will not
     * execute below code. Therefore, there is no race between clieny and player thread.
     * (2) An error occurs, player thread is pre-empted. Client calls stopPlayback()
     * which sets hasUserRequestedStop_. While loop breaks, and code below will be executed.
     * TERMINATE state is never entered. Therefore, no race between client and player
     * thread.
     * (3) Client calls startPlay() immediately followed by stopPlay() such that player
     * thread doesn't get chance to execute state machine (enter while loop). When the player
     * thread is scheduled control reaches here and termination occurs as expected. Check for
     * valid object/pointer/value maintains sanity of the cleanup.
     */
    terminatePlayback();
    deregisterForSSREvent();
    reportPlaybackStopped();
}

/*
 * Terminate playback completely.
 */
void AudioPlayerImpl::terminatePlayback() {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    {
        /*
         * Protect from player thread terminating playback and client giving up
         * this class instance. Also prevents against accessing invalid variables,
         * objects and pointers.
         */
        std::lock_guard<std::mutex> playerLock(playerMtx_);

        if (!isPlayInProgress_) {
            return;
        }

        if (!hasSsrOccurred_) {
            waitAllWriteResponse();
            deinitAudioStream();
        }

        deinitFileToPlay();

        bufferPool_           = {};
        isPlayInProgress_     = false;
        hasUserRequestedStop_ = false;
    }
}

/*
 * When playing file for the next time, reset state variable and update file
 * pointer at the 1st audio sample.
 */
telux::common::ErrorCode AudioPlayerImpl::adjustFileAndState() {
    int ret = 0;

    std::rewind(curFile_);

    if (isCompressed_) {
        ret = std::fseek(curFile_, contentOffset_, SEEK_CUR);
        if (ret) {
            LOG(ERROR, __FUNCTION__, " can't fseek");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Returns true, if the previously played audio stream and the current stream
 * have same configuration, otherwise false.
 */
bool AudioPlayerImpl::isStreamConfigurationSame(uint32_t curFileIdx) {

    StreamConfig oldCfg{};
    StreamConfig curCfg{};
    telux::audio::AmrwbpParams *amrPrmsOld = NULL;
    telux::audio::AmrwbpParams *amrPrmsCur = NULL;

    oldCfg = playbackConfigs_[curFileIdx - 1].streamConfig;
    curCfg = playbackConfigs_[curFileIdx].streamConfig;

    if ((oldCfg.type != curCfg.type) || (oldCfg.sampleRate != curCfg.sampleRate)
        || (oldCfg.format != curCfg.format) || (oldCfg.channelTypeMask != curCfg.channelTypeMask)
        || (oldCfg.deviceTypes != curCfg.deviceTypes)) {
        return false;
    }

    if (curCfg.format == telux::audio::AudioFormat::PCM_16BIT_SIGNED) {
        /* AMR checks are not needed for PCM, so return from here */
        return true;
    }

    if ((!oldCfg.formatParams) || (!curCfg.formatParams)) {
        return false;
    }

    amrPrmsOld = static_cast<telux::audio::AmrwbpParams *>(oldCfg.formatParams);
    amrPrmsCur = static_cast<telux::audio::AmrwbpParams *>(curCfg.formatParams);

    if ((amrPrmsOld->bitWidth != amrPrmsCur->bitWidth)
        || (amrPrmsOld->frameFormat != amrPrmsCur->frameFormat)) {
        return false;
    }

    return true;
}

/*
 * Open an audio stream, if required, close existing stream.
 */
telux::common::ErrorCode AudioPlayerImpl::reinitAudioStream(uint32_t curFileIdx) {
    bool ret = false;
    telux::common::Status status;
    telux::common::ErrorCode ec;

    /* Reset write ready state */
    isAdspWriteReady_ = true;

    if (isStreamOpened_) {
        ret = isStreamConfigurationSame(curFileIdx);
        if (ret) {
            /*
             * Optimization; if previous and current stream have the same configuration,
             * reuse the existing stream to save time spent in closing and then opening
             * new stream again.
             */
            return telux::common::ErrorCode::SUCCESS;
        }
        if (isCompressed_) {
            try {
                audioPlayStream_->deRegisterListener(shared_from_this());
            } catch (const std::bad_weak_ptr &e) {
                LOG(ERROR, __FUNCTION__, " can't deregister for play events");
                /* Don't treat as fatal */
            }
        }
        deinitAudioStream();
        bufferPool_       = {};
        buffersAllocated_ = false;
    }

    ec = initAudioStream(playbackConfigs_[curFileIdx].streamConfig);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        return ec;
    }

    ec = prepareBuffers();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        deinitAudioStream();
        return ec;
    }

    ec = setFormatAndOffset(playbackConfigs_[curFileIdx].streamConfig.format);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        deinitAudioStream();
        bufferPool_       = {};
        buffersAllocated_ = false;
        return ec;
    }

    if (isCompressed_) {
        /* Register for onReadyForWrite() and onPlayStopped() callbacks */
        try {
            ec     = telux::common::ErrorCode::SUCCESS;
            status = telux::common::Status::SUCCESS;
            status = audioPlayStream_->registerListener(shared_from_this());
        } catch (const std::bad_weak_ptr &e) {
            ec = telux::common::ErrorCode::INVALID_STATE;
            LOG(ERROR, __FUNCTION__, " can't register compresscb, err ", static_cast<int>(ec));
        }

        if ((ec != telux::common::ErrorCode::SUCCESS)
            || (status != telux::common::Status::SUCCESS)) {
            if (status != telux::common::Status::SUCCESS) {
                ec = telux::common::CommonUtils::toErrorCode(status);
                LOG(ERROR, __FUNCTION__, " can't register compresscb, err ", static_cast<int>(ec));
            }
            deinitAudioStream();
            bufferPool_       = {};
            buffersAllocated_ = false;
            return ec;
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * CreateStreamResponseCb callback must be valid/existent in memory if the response
 * comes after this timeout. Therefore, define a method whose lifetime is more than
 * that of the AudioManagerImpl.
 */
void AudioPlayerImpl::createStreamComplete(
    std::shared_ptr<IAudioStream> &stream, telux::common::ErrorCode result) {
    {
        std::lock_guard<std::mutex> streamLock(streamMtx_);

        if (result == telux::common::ErrorCode::SUCCESS) {
            audioPlayStream_ = std::dynamic_pointer_cast<telux::audio::IAudioPlayStream>(stream);
            isStreamOpened_  = true;
        }

        errToReport_           = result;
        isCreateResponseReady_ = true;
        asyncResponse_.notify_all();
    }
}

/*
 * Creates an audio stream.
 */
telux::common::ErrorCode AudioPlayerImpl::initAudioStream(StreamConfig streamConfig) {

    bool waitResult = false;
    telux::common::ErrorCode ec;
    telux::common::Status status;

    auto createStreamResponseCb = std::bind(
        &AudioPlayerImpl::createStreamComplete, this, std::placeholders::_1, std::placeholders::_2);

    isCreateResponseReady_ = false;
    status                 = audioManager_->createStream(streamConfig, createStreamResponseCb);
    if (status != telux::common::Status::SUCCESS) {
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " failed create stream ", static_cast<int>(ec));
        return ec;
    }

    {
        std::unique_lock<std::mutex> streamLock(streamMtx_);

        /*
         * When an async request to create stream is sent to the audio server,
         * we don't know whether the response from server will come or not in
         * error scenarios for example, SSR. If it comes, how much time it will
         * take. Use a TIME_10_SECONDS second timeout to prevent player thread from
         * remaining blocked forever if response doesn't come.
         */
        waitResult = asyncResponse_.wait_for(streamLock, std::chrono::seconds(TIME_10_SECONDS),
            [=] { return (isCreateResponseReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

        if (!waitResult) {
            LOG(ERROR, __FUNCTION__, " timedout");
            return telux::common::ErrorCode::OPERATION_TIMEOUT;
        }

        if (hasSsrOccurred_) {
            LOG(ERROR, __FUNCTION__, " ssr occurred");
            return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
        }

        if (hasUserRequestedStop_) {
            LOG(ERROR, __FUNCTION__, " user stopped");
            return telux::common::ErrorCode::CANCELLED;
        }

        if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't create stream ", static_cast<int>(errToReport_));
            return errToReport_;
        }

        if (streamConfig.voicePaths.empty()) {
            /* Regular playback */
            lastUsedDevices_ = streamConfig.deviceTypes;
        } else {
            /* Incall playback */
            lastUsedDevices_ = {};
        }

        /* Cache mask for currently playing stream so that volume can be applied correctly */
        curChannelTypeMask_ = streamConfig.channelTypeMask;

        /*
         * Cached attributes must be applied before releasing streamMtx_ to avoid
         * race between player thread trying to set cached attribute and application
         * thread trying to update same attribute at the same time.
         */
        if (!cachedDevices_.empty()) {
            ec = updateDevice(cachedDevices_, streamLock);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                return ec;
            }
        }

        if (applyCachedVolume_) {
            ec = updateVolume(cachedVolumeLevel_, streamLock);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                return ec;
            }
        }

        /* If the stream was originally muted, then mute it else skip
         * setting mute state since it is already unmuted. */
        if (applyCachedMute_ && isStreamMuted_) {
            ec = updateMute(isStreamMuted_, streamLock);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                return ec;
            }
        }

        return telux::common::ErrorCode::SUCCESS;
    }
}

/*
 * Receives response of the delete stream.
 */
void AudioPlayerImpl::deleteStreamComplete(telux::common::ErrorCode result) {
    {
        std::lock_guard<std::mutex> streamLock(streamMtx_);

        errToReport_           = result;
        isDeleteResponseReady_ = true;
        asyncResponse_.notify_all();
    }
}

/*
 * Deletes audio stream.
 */
telux::common::ErrorCode AudioPlayerImpl::deinitAudioStream() {

    bool waitResult = false;
    telux::common::ErrorCode ec;
    telux::common::Status status;

    auto deleteStreamResponseCb
        = std::bind(&AudioPlayerImpl::deleteStreamComplete, this, std::placeholders::_1);

    {
        std::unique_lock<std::mutex> streamLock(streamMtx_);

        if (!isStreamOpened_ || !audioPlayStream_) {
            /* Stream already closed or doesn't exist */
            LOG(ERROR, __FUNCTION__, " no stream");
            return telux::common::ErrorCode::SUCCESS;
        }

        isDeleteResponseReady_ = false;
        status = audioManager_->deleteStream(audioPlayStream_, deleteStreamResponseCb);
        if (status != telux::common::Status::SUCCESS) {
            ec = telux::common::CommonUtils::toErrorCode(status);
            LOG(ERROR, __FUNCTION__, " failed delete stream ", static_cast<int>(ec));
            return ec;
        }

        waitResult = asyncResponse_.wait_for(streamLock, std::chrono::seconds(TIME_10_SECONDS),
            [=] { return (isDeleteResponseReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

        if (!waitResult) {
            LOG(ERROR, __FUNCTION__, " timedout");
            return telux::common::ErrorCode::OPERATION_TIMEOUT;
        }

        if (hasSsrOccurred_) {
            LOG(ERROR, __FUNCTION__, " ssr occurred");
            return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
        }

        if (hasUserRequestedStop_) {
            LOG(ERROR, __FUNCTION__, " user stopped");
            return telux::common::ErrorCode::CANCELLED;
        }

        if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't delete stream ", static_cast<int>(errToReport_));
            return errToReport_;
        }

        isStreamOpened_ = false;
        return telux::common::ErrorCode::SUCCESS;
    }
}

/*
 * Allocate buffers for writing audio samples.
 */
telux::common::ErrorCode AudioPlayerImpl::prepareBuffers() {

    bufferSize_ = 0;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    for (size_t x = 0; x < BUFFER_POOL_SIZE; x++) {

        /* Allocate 2 buffers (ping-pong) and cache buffer pointers */
        streamBuffer = audioPlayStream_->getStreamBuffer();
        if (!streamBuffer) {
            LOG(ERROR, __FUNCTION__, " can't allocate buffers");
            bufferPool_       = {};
            buffersAllocated_ = false;
            return telux::common::ErrorCode::NO_MEMORY;
        }

        bufferPool_.push(streamBuffer);

        /*
         * Identify optimal buffer size for this stream type and set it. The getMinSize()
         * gives optimal buffer size for given stream (use case). If it doesn't give us
         * any value, set buffer size to the maximum value to minimize playback latency.
         */
        bufferSize_ = streamBuffer->getMinSize();
        if (!bufferSize_) {
            bufferSize_ = streamBuffer->getMaxSize();
        }
        streamBuffer->setDataSize(bufferSize_);
    }

    buffersAllocated_ = true;
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Opens a file for playback from the user specified path.
 */
telux::common::ErrorCode AudioPlayerImpl::initFileToPlay() {

    if (curFileName_.empty()) {
        LOG(ERROR, __FUNCTION__, " missing file name");
        return telux::common::ErrorCode::MISSING_RESOURCE;
    }

    curFile_ = std::fopen(curFileName_.c_str(), "rb");
    if (!curFile_) {
        LOG(ERROR, __FUNCTION__, " can't open file ", curFileName_, ", lnx err ",
            static_cast<int>(errno));
        return telux::common::ErrorCode::NO_SUCH_ELEMENT;
    }

    isFileOpened_ = true;
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Closes the file.
 */
telux::common::ErrorCode AudioPlayerImpl::deinitFileToPlay() {
    int ret;

    /*
     * 1. File closed when in DEINIT_FILE state.
     * 2. Explicit stop playback request received, termination sequence started.
     * 3. This method is called again.
     * Since curFile_ becomes invalid after step 1, prevent access to it.
     */
    if (!isFileOpened_ || !curFile_) {
        LOG(ERROR, __FUNCTION__, " no opened file");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ret = std::fclose(curFile_);
    if (ret) {
        LOG(ERROR, __FUNCTION__, " can't close file");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    isFileOpened_ = false;
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Actually sends samples to the server for playback.
 */
telux::common::ErrorCode AudioPlayerImpl::playAudioSamples() {

    bool waitResult = false;
    std::cv_status waitResultNoPredicate{};
    uint32_t numBytesRead = 0;
    telux::common::ErrorCode ec;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    {
        std::unique_lock<std::mutex> writeLock(writeMtx_);

        if (bufferPool_.empty()) {
            /* Wait for a free buffer. Predicate is not used because of ping-pong */
            waitResultNoPredicate
                = bufferAvailable_.wait_for(writeLock, std::chrono::seconds(TIME_10_SECONDS));

            if (waitResultNoPredicate == std::cv_status::timeout) {
                LOG(ERROR, __FUNCTION__, " timedout");
                errToReport_ = telux::common::ErrorCode::OPERATION_TIMEOUT;
                return errToReport_;
            }

            if (hasSsrOccurred_) {
                LOG(ERROR, __FUNCTION__, " ssr occurred");
                errToReport_ = telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
                return errToReport_;
            }

            if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
                /* Error encountered while playing previously sent buffer, bail out */
                return errToReport_;
            }
        }

        if (isCompressed_ && !isAdspWriteReady_) {
            /* Although buffer is available but ADSP can't accept at the moment */
            waitResult = adspReady_.wait_for(writeLock, std::chrono::seconds(TIME_10_SECONDS),
                [=] { return (isAdspWriteReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

            if (!waitResult) {
                LOG(ERROR, __FUNCTION__, " timedout");
                errToReport_ = telux::common::ErrorCode::OPERATION_TIMEOUT;
                return errToReport_;
            }

            if (hasSsrOccurred_) {
                LOG(ERROR, __FUNCTION__, " ssr occurred");
                errToReport_ = telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
                return errToReport_;
            }

            if (hasUserRequestedStop_) {
                LOG(ERROR, __FUNCTION__, " user stopped");
                errToReport_ = telux::common::ErrorCode::CANCELLED;
                return errToReport_;
            }
        }

        streamBuffer = bufferPool_.front();
        bufferPool_.pop();

        numBytesRead = std::fread(streamBuffer->getRawBuffer(), 1, bufferSize_, curFile_);

#ifdef AUDIOPLAYERIMPL_DDBG
        LOG(DEBUG, __FUNCTION__, " bytes read from file ", numBytesRead);
#endif

        if ((numBytesRead == 0) && std::feof(curFile_)) {
            /* Complete file has been played */
            bufferPool_.push(streamBuffer);
            return telux::common::ErrorCode::SUCCESS;
        }

        if ((numBytesRead != bufferSize_) && !std::feof(curFile_)) {
            /* Can't read requested number of bytes from the file system */
            bufferPool_.push(streamBuffer);
            LOG(ERROR, __FUNCTION__, " can't read file, numBytesRead ", numBytesRead);
            return telux::common::ErrorCode::SYSTEM_ERR;
        }

        streamBuffer->setDataSize(numBytesRead);

#ifdef AUDIOPLAYERIMPL_DDBG
        LOG(DEBUG, __FUNCTION__, " writing size ", numBytesRead);
#endif

        status = audioPlayStream_->write(streamBuffer, writeCompleteCb_);
        if (status != telux::common::Status::SUCCESS) {
            bufferPool_.push(streamBuffer);
            ec = telux::common::CommonUtils::toErrorCode(status);
            LOG(ERROR, __FUNCTION__, " can't write, err ", static_cast<int>(ec));
            return ec;
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Response callback to confirm samples played actually or playback failed.
 */
void AudioPlayerImpl::writeComplete(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
    uint32_t bytesWritten, telux::common::ErrorCode ec) {

#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__, " bytesWritten ", bytesWritten, ", ec ", static_cast<int>(ec));
#endif

    long rewindLength = 0;

    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " write failed, err ", static_cast<int>(ec));
    } else if (buffer->getDataSize() != bytesWritten) {
        /*
         * Whole buffer can't be played successfully. Calculate how many bytes
         * (rewindLength) we need to play again and rewind to it.
         */
        rewindLength = (-1) * (static_cast<long>((buffer->getDataSize() - bytesWritten)));
        std::fseek(curFile_, rewindLength, SEEK_CUR);
    } else {
        /* Success, send next buffer to play */
    }

    {
        std::lock_guard<std::mutex> writeLock(writeMtx_);

        /* Let the player thread know play success/failure for this buffer */
        errToReport_ = ec;

        if (isCompressed_ && rewindLength) {
            /* ADSP pipeline can't accept more buffers to play at the moment */
            isAdspWriteReady_ = false;
        }

        bufferPool_.push(buffer);
        bufferAvailable_.notify_all();
    }
}

/*
 * Wait for responses to all the write request sent to the audio server
 * till now. This ensures writeComplete() exist in memory till the time
 * it will be accessed to deliver the write results from the server.
 */
telux::common::ErrorCode AudioPlayerImpl::waitAllWriteResponse() {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    std::cv_status waitResult{};

    {
        std::unique_lock<std::mutex> bufferWaitLock(writeMtx_);

        while (buffersAllocated_ && (bufferPool_.size() != BUFFER_POOL_SIZE)) {
            /*
             * Predicate is not used since there are two writes active at any time
             * instant and response of latest write will overwrite response of
             * previous write. Therefore, practically, predicate will be true always
             * and will not server its actual purpose.
             */
            waitResult
                = bufferAvailable_.wait_for(bufferWaitLock, std::chrono::seconds(TIME_10_SECONDS));

            if (waitResult == std::cv_status::timeout) {
                LOG(ERROR, __FUNCTION__, " timedout");
                return telux::common::ErrorCode::OPERATION_TIMEOUT;
            }

            if (hasSsrOccurred_) {
                LOG(ERROR, __FUNCTION__, " ssr occurred");
                return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
            }
        }

        return telux::common::ErrorCode::SUCCESS;
    }
}

/*
 * Updates client that an error has occurred.
 */
void AudioPlayerImpl::reportError(telux::common::ErrorCode ec, std::string file) {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__, " ec ", static_cast<int>(ec));
#endif

    auto playListListener = statusListener_.lock();
    if (playListListener) {
        playListListener->onError(ec, file);
    }
}

/*
 *  Updates client that the file has been played.
 */
void AudioPlayerImpl::reportPlayed() {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    auto playListListener = statusListener_.lock();
    if (playListListener) {
        playListListener->onFilePlayed(curFileName_);
    }
}

/*
 *  Updates client that all the given files have been played in the
 *  manner specified by the client.
 */
void AudioPlayerImpl::reportPlaybackFinished() {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    auto playListListener = statusListener_.lock();
    if (playListListener) {
        playListListener->onPlaybackFinished();
    }
}

/*
 *  Updates client that the playback is started.
 */
void AudioPlayerImpl::reportPlaybackStarted() {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    auto playListListener = statusListener_.lock();
    if (playListListener) {
        playListListener->onPlaybackStarted();
    }
}

/*
 *  Updates client that the playback is terminated.
 */
void AudioPlayerImpl::reportPlaybackStopped() {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    auto playListListener = statusListener_.lock();
    if (playListListener) {
        playListListener->onPlaybackStopped();
    }
}

/*
 * Player sends last 2 buffers (ping-pong) to the audio server for playback.
 * Handle below possible cases:
 *
 *  Case | 2nd last buffer | Last buffer
 * ----------------------------------------
 *   1         Played          Played
 *   2         Failed          Failed
 *   3         Failed          Played
 *   4         Played          Failed
 */
telux::common::ErrorCode AudioPlayerImpl::finalizePlayback() {

    bool finalizeAMR = false;
    telux::common::ErrorCode ec;

#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    /* For all cases wait for the response from server for both the buffers */
    ec = waitAllWriteResponse();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        return ec;
    }

    {
        std::unique_lock<std::mutex> writeLock1(writeMtx_);

        /* Case 1 */
        if ((errToReport_ == telux::common::ErrorCode::SUCCESS) && isAdspWriteReady_) {
            if (!isCompressed_) {
                /* PCM playback completed successfully */
                return telux::common::ErrorCode::SUCCESS;
            }
            finalizeAMR = true;
        }
    }

    if (finalizeAMR) {
        return finalizeCompressedPlayback();
    }

    {
        std::unique_lock<std::mutex> writeLock2(writeMtx_);
        /*
         * Case 2,3,4.
         * As per the current design, audio server's response to the last write overwrites
         * response to the 2nd last write call. If a real error occurred return it. If the
         * ADSP couldn't play buffers, inform player thread to resend them.
         */
        if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
            /* Error occurred while trying to play the last two buffers */
            return errToReport_;
        }

        if (!isAdspWriteReady_) {
            /* Need to resend the buffer */
            return telux::common::ErrorCode::REQUEST_RATE_LIMITED;
        }

        return telux::common::ErrorCode::SUCCESS;
    }
}

/*
 * Receives response of the stop audio for compressed playback.
 */
void AudioPlayerImpl::stopAudioComplete(telux::common::ErrorCode result) {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__, " result ", static_cast<int>(result));
#endif

    {
        std::lock_guard<std::mutex> streamLock(streamMtx_);

        errToReport_         = result;
        isStopResponseReady_ = true;
        asyncResponse_.notify_all();
    }
}

/*
 * When playing AMR formatted audio, ADSP needs to be instructed to stop after
 * playing all the pending buffers it has in the pipeline.
 */
telux::common::ErrorCode AudioPlayerImpl::finalizeCompressedPlayback() {

    bool waitResult = false;
    telux::common::ErrorCode ec;
    telux::common::Status status;

    auto stopAudioResponseCb
        = std::bind(&AudioPlayerImpl::stopAudioComplete, this, std::placeholders::_1);

#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    isStopAudioReady_    = false;
    isStopResponseReady_ = false;

    status = audioPlayStream_->stopAudio(StopType::STOP_AFTER_PLAY, stopAudioResponseCb);
    if (status != telux::common::Status::SUCCESS) {
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " failed stop, err ", static_cast<int>(ec));
        return ec;
    }

    {
        std::unique_lock<std::mutex> streamLock(streamMtx_);

        waitResult = asyncResponse_.wait_for(streamLock, std::chrono::seconds(TIME_10_SECONDS),
            [=] { return (isStopResponseReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

        if (!waitResult) {
            LOG(ERROR, __FUNCTION__, " timedout");
            return telux::common::ErrorCode::OPERATION_TIMEOUT;
        }

        if (hasSsrOccurred_) {
            LOG(ERROR, __FUNCTION__, " ssr occurred");
            return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
        }

        if (hasUserRequestedStop_) {
            LOG(ERROR, __FUNCTION__, " user stopped");
            return telux::common::ErrorCode::CANCELLED;
        }

        if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't stop stream ", static_cast<int>(errToReport_));
            return errToReport_;
        }
    }

    {
        std::unique_lock<std::mutex> writeLock(writeMtx_);

        /* Wait for response from ADSP confirming it stopped (drain done) */
        waitResult
            = compressedPlayStopped_.wait_for(writeLock, std::chrono::seconds(TIME_10_SECONDS),
                [=] { return (isStopAudioReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

        if (!waitResult) {
            LOG(ERROR, __FUNCTION__, " timedout");
            return telux::common::ErrorCode::OPERATION_TIMEOUT;
        }

        if (hasSsrOccurred_) {
            LOG(ERROR, __FUNCTION__, " ssr occurred");
            return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
        }

        if (hasUserRequestedStop_) {
            LOG(ERROR, __FUNCTION__, " user stopped");
            return telux::common::ErrorCode::CANCELLED;
        }

        /* Compressed playback completed successfully */
        return telux::common::ErrorCode::SUCCESS;
    }
}

/*
 * Register for SSR onServiceStatusChange() callback.
 */
telux::common::ErrorCode AudioPlayerImpl::registerForSSREvent() {

    telux::common::ErrorCode ec;
    telux::common::Status status;

    try {
        status = audioManager_->registerListener(shared_from_this());
    } catch (const std::bad_weak_ptr &e) {
        /* Destruction of AudioPlayerImpl triggered (client released AudioPlayerImpl) */
        LOG(ERROR, __FUNCTION__, " can't register for ssr");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if ((status != telux::common::Status::SUCCESS) && (status != telux::common::Status::ALREADY)) {
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " can't register ssrcb, err ", static_cast<int>(ec));
        if (isCompressed_) {
            try {
                status = audioManager_->deRegisterListener(shared_from_this());
            } catch (const std::bad_weak_ptr &e) {
                LOG(ERROR, __FUNCTION__, " can't deregister for ssr");
                return telux::common::ErrorCode::INVALID_STATE;
            }
        }
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 *  Deregisters for SSR onServiceStatusChange() callback.
 */
telux::common::ErrorCode AudioPlayerImpl::deregisterForSSREvent() {

    telux::common::ErrorCode ec;
    telux::common::Status status;

    try {
        status = audioManager_->deRegisterListener(shared_from_this());
    } catch (const std::bad_weak_ptr &e) {
        LOG(ERROR, __FUNCTION__, " can't deregister for ssr");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if (status != telux::common::Status::SUCCESS) {
        /* Don't treat as fatal */
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " can't deregister ssrcb, err ", static_cast<int>(ec));
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 *  Called when the audio server can accept the next buffer for compressed play.
 */
void AudioPlayerImpl::onReadyForWrite() {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    {
        std::lock_guard<std::mutex> writeLock(writeMtx_);

        isAdspWriteReady_ = true;
        adspReady_.notify_all();
    }
}

/*
 *  Called to confirm all buffers of compressed playback have been processed.
 */
void AudioPlayerImpl::onPlayStopped() {
#ifdef AUDIOPLAYERIMPL_DDBG
    LOG(DEBUG, __FUNCTION__);
#endif

    {
        std::lock_guard<std::mutex> compressStopLock(writeMtx_);

        isStopAudioReady_ = true;
        compressedPlayStopped_.notify_all();
    }
}

/*
 * SSR handling flow:
 * 1. SSR occurs, audio server sends service unavailable.
 * 2. Player thread is unblocked from waits which will never be over now.
 * 3. Player thread does the cleanup, reports play stopped and terminates.
 */
void AudioPlayerImpl::onServiceStatusChange(telux::common::ServiceStatus status) {

    LOG(DEBUG, __FUNCTION__, " SSR status ", static_cast<int>(status));

    if (status != telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        /*
         * Only service unavailable awareness is needed to exit the player
         * thread, therefore, just reset SSR state.
         */
        hasSsrOccurred_ = false;
        return;
    }

    unblockPlayerThread(true);
}

/*
 * During playback, the player thread waits for the async responses from the audio
 * server at various stages. Unblock the player thread so that it can execute next
 * expected step.
 *
 * The playerMtx_ is used to ensure integrity of the implementation during
 * starting, stopping, playing and destruction. This mutex ensures following
 * five cases remain handled gracefully:
 *
 * 1. App gives up IAudioPlayer instance without actually starting the playback.
 * 2. App gives up IAudioPlayer instance during an on-going playback.
 * 3. SSR occurs during an on-going playback.
 * 4. App calls stopPlayback() explicitly to terminate the playback.
 * 5. Player thread exits due to a fatal error.
 */
void AudioPlayerImpl::unblockPlayerThread(bool setSSRStatus) {
    {
        /*
         * This lock synchronizes player thread, AudioPlayerImpl destruction and
         * caller thread of this method.
         */
        std::lock_guard<std::mutex> playerLock(playerMtx_);
        {
            /* This lock prevents spurious/false wake ups */
            std::lock_guard<std::mutex> ssrLock(writeMtx_);

            if (setSSRStatus) {
                hasSsrOccurred_ = true;
            }

            asyncResponse_.notify_all();
            adspReady_.notify_all();
            bufferAvailable_.notify_all();
            compressedPlayStopped_.notify_all();
        }
    }
}

/*
 *  Receives async response for the setVolume().
 */
SetVolumeResponseListener::SetVolumeResponseListener(std::mutex &streamMtx)
   : streamMutex_(streamMtx) {
}
void SetVolumeResponseListener::setVolumeComplete(telux::common::ErrorCode errorCode) {
    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't set volume");
    }
    {
        std::lock_guard<std::mutex> streamLock(streamMutex_);
        this->errorCode     = errorCode;
        this->responseReady = true;
        this->cv.notify_one();
    }
}

/*
 *  Helper to set volume to the given level.
 */
telux::common::ErrorCode AudioPlayerImpl::updateVolume(
    float volumeLevel, std::unique_lock<std::mutex> &streamLock) {

    bool waitResult = false;
    telux::common::Status status;
    StreamVolume streamVol{};
    ChannelVolume channelVolume{};
    SetVolumeResponseListener listener(streamMtx_);

    auto responseCb = std::bind(
        &SetVolumeResponseListener::setVolumeComplete, &listener, std::placeholders::_1);

    /* Based on the number of channels currently playing stream has, set channels for volume */
    switch (curChannelTypeMask_) {
        case telux::audio::ChannelType::LEFT:
            channelVolume.vol         = volumeLevel;
            channelVolume.channelType = telux::audio::ChannelType::LEFT;
            streamVol.volume.emplace_back(channelVolume);
            break;
        case telux::audio::ChannelType::RIGHT:
            channelVolume.vol         = volumeLevel;
            channelVolume.channelType = telux::audio::ChannelType::RIGHT;
            streamVol.volume.emplace_back(channelVolume);
            break;
        default:
            channelVolume.vol         = volumeLevel;
            channelVolume.channelType = telux::audio::ChannelType::LEFT;
            streamVol.volume.emplace_back(channelVolume);
            channelVolume.vol         = volumeLevel;
            channelVolume.channelType = telux::audio::ChannelType::RIGHT;
            streamVol.volume.emplace_back(channelVolume);
    }
    streamVol.dir = StreamDirection::RX;

    status = audioPlayStream_->setVolume(streamVol, responseCb);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't set volume");
        return telux::common::CommonUtils::toErrorCode(status);
    }

    waitResult = listener.cv.wait_for(streamLock, std::chrono::seconds(TIME_10_SECONDS),
        [&] { return (listener.responseReady || hasSsrOccurred_ || hasUserRequestedStop_); });

    if (!waitResult) {
        LOG(ERROR, __FUNCTION__, " timed out");
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    }

    if (hasSsrOccurred_) {
        LOG(ERROR, __FUNCTION__, " ssr occurred");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (hasUserRequestedStop_) {
        LOG(ERROR, __FUNCTION__, " user stopped");
        return telux::common::ErrorCode::CANCELLED;
    }

    return listener.errorCode;
}

/*
 *  Sets volume to the given level.
 */
telux::common::ErrorCode AudioPlayerImpl::setVolume(float volumeLevel) {
    telux::common::ErrorCode ec;

    {
        std::unique_lock<std::mutex> streamLock(streamMtx_);

        if (!isStreamOpened_) {
            /*
             * Application tried to set the volume just after starting the playback
             * but before player thread gets a chance to create an audio stream.
             * Cache the volume to be applied later when the stream is created.
             */
            if ((volumeLevel > 1.0) || (volumeLevel < 0.0)) {
                LOG(ERROR, __FUNCTION__, " out of range volume level");
                return telux::common::ErrorCode::INVALID_ARGUMENTS;
            }
            cachedVolumeLevel_ = volumeLevel;
            applyCachedVolume_ = true;
            return telux::common::ErrorCode::SUCCESS;
        }

        ec = updateVolume(volumeLevel, streamLock);
        if (ec == telux::common::ErrorCode::SUCCESS) {
            /*
             * If the volume is set, cache it so that it can be applied to all
             * the new streams if the current stream on which it is currently
             * applied is closed and a new one is created.
             */
            cachedVolumeLevel_ = volumeLevel;
            applyCachedVolume_ = true;
        }
        return ec;
    }
}

/*
 *  Receives async response for the getVolume().
 */
GetVolumeResponseListener::GetVolumeResponseListener(std::mutex &streamMtx)
   : streamMutex_(streamMtx) {
}
void GetVolumeResponseListener::getVolumeComplete(
    StreamVolume volume, telux::common::ErrorCode errorCode) {

    float volumeLevelFetched = 0.0;

    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't get volume");
    } else {
        /* If the stream was mono the 0th element will contain volume level.
         * If the stream was stereo both 0th and 1st element will have same
         * level. Therefore, just use 0th element. */
        volumeLevelFetched = volume.volume[0].vol;
    }

    {
        std::lock_guard<std::mutex> streamLock(streamMutex_);

        this->volumeLevel   = volumeLevelFetched;
        this->errorCode     = errorCode;
        this->responseReady = true;
        this->cv.notify_one();
    }
}

/*
 * Retrieves the current volume.
 */
telux::common::ErrorCode AudioPlayerImpl::getVolume(float &volumeLevel) {

    bool waitResult = false;
    telux::common::Status status;
    GetVolumeResponseListener listener(streamMtx_);

    auto responseCb = std::bind(&GetVolumeResponseListener::getVolumeComplete, &listener,
        std::placeholders::_1, std::placeholders::_2);

    {
        std::unique_lock<std::mutex> streamLock(streamMtx_);

        if (!isStreamOpened_) {
            /*
             * Either stream was not created at all or the getVolume() is called
             * just after deleting the last stream but before creating the next stream.
             */
            LOG(DEBUG, __FUNCTION__, " no stream");

            if (applyCachedVolume_) {
                /* If the volume was set previously by the application return it */
                volumeLevel = cachedVolumeLevel_;
                return telux::common::ErrorCode::SUCCESS;
            }

            /* Return system's default volume, if it was never set */
            volumeLevel = 1.0;
            return telux::common::ErrorCode::SUCCESS;
        }

        /* There exist a valid audio stream, fetch and return latest volume */
        status = audioPlayStream_->getVolume(StreamDirection::RX, responseCb);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't get volume");
            return telux::common::CommonUtils::toErrorCode(status);
        }

        waitResult = listener.cv.wait_for(streamLock, std::chrono::seconds(TIME_10_SECONDS),
            [&] { return (listener.responseReady || hasSsrOccurred_ || hasUserRequestedStop_); });

        if (!waitResult) {
            LOG(ERROR, __FUNCTION__, " timed out");
            return telux::common::ErrorCode::OPERATION_TIMEOUT;
        }

        if (hasSsrOccurred_) {
            LOG(ERROR, __FUNCTION__, " ssr occurred");
            return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
        }

        if (hasUserRequestedStop_) {
            LOG(ERROR, __FUNCTION__, " user stopped");
            return telux::common::ErrorCode::CANCELLED;
        }

        volumeLevel = listener.volumeLevel;
        return listener.errorCode;
    }
}

/*
 *  Helper to set mute state to the given state.
 */
telux::common::ErrorCode AudioPlayerImpl::updateMute(
    bool enable, std::unique_lock<std::mutex> &streamLock) {

    bool waitResult = false;
    StreamMute streamMute{};
    telux::common::Status status;
    SetMuteResponseListener listener(streamMtx_);

    auto responseCb
        = std::bind(&SetMuteResponseListener::setMuteComplete, &listener, std::placeholders::_1);

    streamMute.enable = enable;
    streamMute.dir    = StreamDirection::RX;

    status = audioPlayStream_->setMute(streamMute, responseCb);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't set mute state");
        return telux::common::CommonUtils::toErrorCode(status);
    }

    waitResult = listener.cv.wait_for(streamLock, std::chrono::seconds(TIME_10_SECONDS),
        [&] { return (listener.responseReady || hasSsrOccurred_ || hasUserRequestedStop_); });

    if (!waitResult) {
        LOG(ERROR, __FUNCTION__, " timed out");
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    }

    if (hasSsrOccurred_) {
        LOG(ERROR, __FUNCTION__, " ssr occurred");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (hasUserRequestedStop_) {
        LOG(ERROR, __FUNCTION__, " user stopped");
        return telux::common::ErrorCode::CANCELLED;
    }

    return listener.errorCode;
}

/*
 *  Receives async response for the setMute().
 */
SetMuteResponseListener::SetMuteResponseListener(std::mutex &streamMtx)
   : streamMutex_(streamMtx) {
}
void SetMuteResponseListener::setMuteComplete(telux::common::ErrorCode errorCode) {
    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't set mute state");
    }
    {
        std::lock_guard<std::mutex> streamLock(streamMutex_);
        this->errorCode     = errorCode;
        this->responseReady = true;
        this->cv.notify_one();
    }
}

/*
 *  Mute/Unmute the audio stream.
 */
telux::common::ErrorCode AudioPlayerImpl::setMute(bool enable) {
    telux::common::ErrorCode ec;

    {
        std::unique_lock<std::mutex> streamLock(streamMtx_);

        if (!isStreamOpened_) {
            isStreamMuted_   = enable;
            applyCachedMute_ = true;
            return telux::common::ErrorCode::SUCCESS;
        }

        ec = updateMute(enable, streamLock);
        if (ec == telux::common::ErrorCode::SUCCESS) {
            /* If the mute state is set, cache it so that it can be applied to all
             * the new streams if the current stream on which it is currently
             * applied is closed and a new one is created. */
            isStreamMuted_   = enable;
            applyCachedMute_ = true;
        }
        return ec;
    }
}

/*
 * Retrieves the current mute state.
 */
telux::common::ErrorCode AudioPlayerImpl::getMute(bool &enable) {
    {
        std::unique_lock<std::mutex> streamLock(streamMtx_);
        enable = isStreamMuted_;
        return telux::common::ErrorCode::SUCCESS;
    }
}

/*
 *  Receives async response for the setDevice().
 */
SetDeviceResponseListener::SetDeviceResponseListener(std::mutex &streamMtx)
   : streamMutex_(streamMtx) {
}
void SetDeviceResponseListener::setDeviceComplete(telux::common::ErrorCode errorCode) {
    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't set device");
    }
    {
        std::lock_guard<std::mutex> streamLock(streamMutex_);
        this->errorCode     = errorCode;
        this->responseReady = true;
        this->cv.notify_one();
    }
}

/*
 *  Helper to set device.
 */
telux::common::ErrorCode AudioPlayerImpl::updateDevice(
    std::vector<DeviceType> devices, std::unique_lock<std::mutex> &streamLock) {

    bool waitResult = false;
    telux::common::Status status;
    SetDeviceResponseListener listener(streamMtx_);

    auto responseCb = std::bind(
        &SetDeviceResponseListener::setDeviceComplete, &listener, std::placeholders::_1);

    status = audioPlayStream_->setDevice(devices, responseCb);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't set device");
        return telux::common::CommonUtils::toErrorCode(status);
    }

    waitResult = listener.cv.wait_for(streamLock, std::chrono::seconds(TIME_10_SECONDS),
        [&] { return (listener.responseReady || hasSsrOccurred_ || hasUserRequestedStop_); });

    if (!waitResult) {
        LOG(ERROR, __FUNCTION__, " timed out");
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    }

    if (hasSsrOccurred_) {
        LOG(ERROR, __FUNCTION__, " ssr occurred");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (hasUserRequestedStop_) {
        LOG(ERROR, __FUNCTION__, " user stopped");
        return telux::common::ErrorCode::CANCELLED;
    }

    return listener.errorCode;
}

/*
 *  Sets device for a the playback audio stream.
 */
telux::common::ErrorCode AudioPlayerImpl::setDevice(std::vector<DeviceType> devices) {

    telux::common::ErrorCode ec;

    {
        std::unique_lock<std::mutex> streamLock(streamMtx_);

        if (!isStreamOpened_) {
            cachedDevices_ = devices;
            return telux::common::ErrorCode::SUCCESS;
        }

        ec = updateDevice(devices, streamLock);
        if (ec == telux::common::ErrorCode::SUCCESS) {
            /* On success cache the device to be applied automatically
             * again, if the current stream is closed and a new stream
             * is created. */
            cachedDevices_ = devices;
        }

        return ec;
    }
}

/*
 *  Receives async response for the getDevice().
 */
GetDeviceResponseListener::GetDeviceResponseListener(std::mutex &streamMtx)
   : streamMutex_(streamMtx) {
}
void GetDeviceResponseListener::getDeviceComplete(
    std::vector<DeviceType> devices, telux::common::ErrorCode errorCode) {

    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't get device");
    }
    {
        std::lock_guard<std::mutex> streamLock(streamMutex_);
        this->devices       = devices;
        this->errorCode     = errorCode;
        this->responseReady = true;
        this->cv.notify_one();
    }
}

/*
 * Retrieves list of the audio devices associated with the audio stream.
 */
telux::common::ErrorCode AudioPlayerImpl::getDevice(std::vector<DeviceType> &devices) {

    bool waitResult = false;
    telux::common::Status status;
    GetDeviceResponseListener listener(streamMtx_);

    auto responseCb = std::bind(&GetDeviceResponseListener::getDeviceComplete, &listener,
        std::placeholders::_1, std::placeholders::_2);

    {
        std::unique_lock<std::mutex> streamLock(streamMtx_);

        if (!isStreamOpened_) {
            if (lastUsedDevices_.empty() && cachedDevices_.empty()) {
                /* Neither application setted the device explicitly
                 * nor stream created at-least once */
                devices.push_back(DEVICE_TYPE_NONE);
                return telux::common::ErrorCode::SUCCESS;
            }

            if (!cachedDevices_.empty()) {
                /* Application setted the device previously */
                devices = cachedDevices_;
                return telux::common::ErrorCode::SUCCESS;
            }

            /* Device is specified as part of create stream configuration only */
            devices = lastUsedDevices_;
            return telux::common::ErrorCode::SUCCESS;
        }

        status = audioPlayStream_->getDevice(responseCb);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't get device");
            return telux::common::CommonUtils::toErrorCode(status);
        }

        waitResult = listener.cv.wait_for(streamLock, std::chrono::seconds(TIME_10_SECONDS),
            [&] { return (listener.responseReady || hasSsrOccurred_ || hasUserRequestedStop_); });

        if (!waitResult) {
            LOG(ERROR, __FUNCTION__, " timed out");
            return telux::common::ErrorCode::OPERATION_TIMEOUT;
        }

        if (hasSsrOccurred_) {
            LOG(ERROR, __FUNCTION__, " ssr occurred");
            return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
        }

        if (hasUserRequestedStop_) {
            LOG(ERROR, __FUNCTION__, " user stopped");
            return telux::common::ErrorCode::CANCELLED;
        }

        devices = listener.devices;
        return listener.errorCode;
    }
}

}  // end of namespace audio
}  // end of namespace telux
