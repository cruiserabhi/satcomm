/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "AudioStreamImpl.hpp"

namespace telux {
namespace audio {

AudioStreamImpl::AudioStreamImpl(uint32_t streamId, StreamType streamType,
        std::shared_ptr<ICommunicator> transportClient) {

    streamId_ = streamId;
    streamType_ = streamType;
    transportClient_ = transportClient;
}

AudioStreamImpl::~AudioStreamImpl() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Receives audio SSR updates.
 */
void AudioStreamImpl::onServiceStatusChange(){

    /*
     * Playback & capture use cases uses ping-pong buffers (two buffers). When
     * SSR occurs, following sequence of messages from server are possible:
     * -------------------------------------------------------------------------
     *  cases |        1st msg      |       2nd msg       |    3rd msg
     * -------------------------------------------------------------------------
     *   (a)  | SSR update          | result of 1st write | result of 2nd write
     *   (b)  | SSR update          | result of 1st write |
     *   (c)  | SSR update          |                     |
     *   (d)  | result of 1st write | result of 2nd write | SSR update
     *   (e)  | result of 1st write | SSR update          | result of 2nd write
     *   (f)  | result of 1st write | SSR update          |
     *
     * 1. We don't know whether messages after SSR will come or not, therefore
     *    application can't take a deterministic action.
     * 2. Application's provided callback may become non-existent in memory since
     *    application started cleaning up when it becomes aware of SSR
     *
     * To achieve deterministic behavior and prevent access to invalid memory,
     * reset the CommandCallbackManager.
     */
    cmdCallbackMgr_.reset();
}

/*
 * Gives the unique numerical identifier assigned to this audio stream to
 * represent it on the audio server side.
 */
uint32_t AudioStreamImpl::getStreamId() {
    return streamId_;
}

/*
 * Gives type of the audio stream like playback or capture etc.
 */
StreamType AudioStreamImpl::getType() {
    return streamType_;
}

/*
 * Sets audio device like mic or speaker to be used with given audio stream.
 * This defines the physical path where audio samples will be sent or received.
 *
 *   -------------------------------------------------------
 *  |  Stream type   | Get/Set Device                       |
 *   -------------------------------------------------------
 *  | Voice call     | Y                                    |
 *  | Playback       | Y                                    |
 *  | Capture        | Y                                    |
 *  | Loopback       | N/A                                  |
 *  | Tone generator | N/A                                  |
 *   -------------------------------------------------------
 *
 * For playback, if invalid device is given, audio packets AFE routing will not happen.
 * For capture, if invalid device is given, default mic will be used.
 * For voice call, stream must be started to make the set device effective.
 */
telux::common::Status AudioStreamImpl::setDevice(std::vector<DeviceType> devices,
        telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    if (devices.size() == 0) {
        LOG(ERROR, __FUNCTION__, " no devices provided");
        return telux::common::Status::INVALIDPARAM;
    }

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->setDevice(streamId_, devices, shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of AudioStreamImpl::setDevice
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void AudioStreamImpl::onSetDeviceResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Gives the audio device currently associated with the given stream.
 */
telux::common::Status AudioStreamImpl::getDevice(GetStreamDeviceResponseCb callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->getDevice(streamId_, shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of AudioStreamImpl::getDevice
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void AudioStreamImpl::onGetDeviceResult(telux::common::ErrorCode ec,
        uint32_t streamId, std::vector<DeviceType> devices, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, devices, ec);
}

/*
 * Sets volume level of the stream (audio device).
 *
 *   -------------------------------------------------------
 *  |  Stream type   | Get/Set Volume                       |
 *   -------------------------------------------------------
 *  | Voice call     | Y - direction RX, N/A - direction TX |
 *  | Playback       | Y                                    |
 *  | Capture        | Y                                    |
 *  | Loopback       | N/A                                  |
 *  | Tone generator | N/A                                  |
 *   -------------------------------------------------------
 *
 * ADSP/Q6 sets volume in step of 0.2 for voice call stream type. For other stream
 * types any valid value can be given. Given value is rounded to the nearest ceil
 * or floor value. Valid range for volume's value is 0.0 <= volume <= 1.0.
 *
 * For playback and capture stream types, get/set volume can be called any time because
 * volume is set directly with command in kernel. However, for voice call stream type,
 * the stream has to be started first and then set/get volume operation must be performed.
 * This is because we use volume based on ACDB calibration as volume change needs to
 * change other PP parameters.
 */
telux::common::Status AudioStreamImpl::setVolume(StreamVolume volume,
        telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->setVolume(streamId_, volume, shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of AudioStreamImpl::setVolume
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void AudioStreamImpl::onSetVolumeResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Gives volume level of the stream (audio device).
 */
telux::common::Status AudioStreamImpl::getVolume(StreamDirection direction,
        GetStreamVolumeResponseCb callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->getVolume(streamId_, direction, shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of AudioStreamImpl::getVolume
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void AudioStreamImpl::onGetVolumeResult(telux::common::ErrorCode ec,
        uint32_t streamId, StreamVolume volume, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, volume, ec);
}

/*
 * Mute or unmute audio stream (audio device) based on the value of streamMute.enable.
 *
 *   -------------------------------------------------------
 *  |  Stream type   | Get/Set Mute state                   |
 *   -------------------------------------------------------
 *  | Voice call     | Y - direction RX, N/A - direction TX |
 *  | Playback       | Y                                    |
 *  | Capture        | Y                                    |
 *  | Loopback       | N/A                                  |
 *  | Tone generator | N/A                                  |
 *   -------------------------------------------------------
 *
 *  Mute/Unmute given stream based on the value of 'muteInfo.enable'.
 *
 *  For voice call stream, stream has to be started before get/set mute. It is
 *  because mute information is fetched from lower layers, whereas for playback
 *  and capture, cached info is returned.
 */
telux::common::Status AudioStreamImpl::setMute(StreamMute streamMute,
        telux::common::ResponseCallback callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->setMute(streamId_, streamMute, shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of AudioStreamImpl::setMute
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void AudioStreamImpl::onSetMuteResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Gives current mute state of the audio stream/device.
 */
telux::common::Status AudioStreamImpl::getMute(StreamDirection direction,
        GetStreamMuteResponseCb callback) {

    intptr_t cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->getMute(streamId_, direction, shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * If application provided a callback to receive the result of AudioStreamImpl::getMute
 * invocation, it calls that callback method otherwise simply drops the result.
 */
void AudioStreamImpl::onGetMuteResult(telux::common::ErrorCode ec,
        uint32_t streamId, StreamMute streamMute, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;


    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, streamMute, ec);
}

}  // End of namespace audio
}  // End of namespace telux
