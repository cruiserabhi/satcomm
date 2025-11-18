/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef IAUDIOCALLBACKS_HPP
#define IAUDIOCALLBACKS_HPP

#include "AudioDefinesLibInternal.hpp"
#include "AudioDeviceImpl.hpp"

namespace telux {
namespace audio {

/*
 * These callbacks are used for passing responses and indications from AudioGrpcClientStub
 * in upward direction, for finally passing them to applications after processing.
 */

class IGetDevicesCb : public telux::common::ICommandCallback {
 public:
    virtual void onGetDevicesResult(telux::common::ErrorCode ec,
                    std::vector<DeviceType> deviceTypes,
                    std::vector<DeviceDirection> deviceDirections, int cmdId) = 0;
};

class IGetStreamsCb : public telux::common::ICommandCallback {
 public:
    virtual void onGetStreamsResult(telux::common::ErrorCode ec,
                    std::vector<StreamType> streams, int cmdId) = 0;
};

class IGetCalInitStatusCb : public telux::common::ICommandCallback {
 public:
    virtual void onGetCalInitStatusResult(telux::common::ErrorCode ec,
                    CalibrationInitStatus calibrationStatus, int cmdId) = 0;
};

class ICreateStreamCb : public telux::common::ICommandCallback {
 public:
    virtual void onCreateStreamResult(telux::common::ErrorCode ec,
                    CreatedStreamInfo createdStreamInfo, int cmdId) = 0;
};

class IDeleteStreamCb : public telux::common::ICommandCallback {
 public:
    virtual void onDeleteStreamResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;
};

class IStartStreamCb : public telux::common::ICommandCallback {
 public:
    virtual void onStreamStartResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;
};

class IStopStreamCb : public telux::common::ICommandCallback {
 public:
    virtual void onStreamStopResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;
};

class IToneCb : public telux::common::ICommandCallback {
 public:
    virtual void onToneStartResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;

    virtual void onToneStopResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;
};

class IWriteCb : public telux::common::ICommandCallback {
 public:
    virtual void onWriteResult(telux::common::ErrorCode ec, uint32_t streamId,
                    uint32_t bytesWritten, AudioUserData *userData) = 0;
};

class IReadCb : public telux::common::ICommandCallback {
 public:
    virtual void onReadResult(telux::common::ErrorCode ec, uint32_t streamId,
                    uint32_t numBytesActuallyRead, AudioUserData *userData) = 0;
};

class IFlushCb : public telux::common::ICommandCallback {
 public:
    virtual void onFlushResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;
};

class IDrainCb : public telux::common::ICommandCallback {
 public:
    virtual void onDrainResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;
};

class ISetGetDeviceCb : public telux::common::ICommandCallback {
 public:
    virtual void onSetDeviceResult(telux::common::ErrorCode ec, uint32_t streamId,
                    int cmdId) = 0;

    virtual void onGetDeviceResult(telux::common::ErrorCode ec, uint32_t streamId,
                    std::vector<DeviceType> devices, int cmdId) = 0;
};

class ISetGetVolumeCb : public telux::common::ICommandCallback {
 public:
    virtual void onSetVolumeResult(telux::common::ErrorCode ec, uint32_t streamId,
                   int cmdId) = 0;

    virtual void onGetVolumeResult(telux::common::ErrorCode ec, uint32_t streamId,
                    StreamVolume volume, int cmdId) = 0;
};

class ISetGetMuteCb : public telux::common::ICommandCallback {
 public:
    virtual void onSetMuteResult(telux::common::ErrorCode ec, uint32_t streamId,
                   int cmdId) = 0;

    virtual void onGetMuteResult(telux::common::ErrorCode ec, uint32_t streamId,
                    StreamMute streamMute, int cmdId) = 0;
};

class IDTMFCb : public telux::common::ICommandCallback {
 public:
    virtual void onPlayDtmfResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;

    virtual void onStopDtmfResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;
};

class ITranscodeCreateCb : public telux::common::ICommandCallback {
 public:
    virtual void onCreateTranscoderResult(telux::common::ErrorCode ec,
        CreatedTranscoderInfo transcoderInfo, int cmdId) = 0;
};

class ITranscodeDeleteCb : public telux::common::ICommandCallback {
 public:
    virtual void onDeleteTranscoderResult(telux::common::ErrorCode ec,
        uint32_t inStreamId, uint32_t outStreamId, int cmdId) = 0;
};

class IIndicationCb : public telux::common::ICommandCallback {
 public:
    virtual void onIndicationRegisterResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;

    virtual void onIndicationDeRegisterResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) = 0;
};

class IPlayStreamEventsCb {
 public:
    virtual void onWriteReady(uint32_t streamId) = 0;

    virtual void onDrainDone(uint32_t streamId) = 0;
};

class IVoiceStreamEventsCb {
 public:
    virtual void onDtmfToneDetected(DtmfTone dtmfTone) = 0;
};

class IServiceStatusEventsCb {
 public:
    virtual void onQ6SSRUpdate(telux::common::ServiceStatus newStatus) = 0;

    virtual void onTransportStatusUpdate(telux::common::ServiceStatus newStatus) = 0;
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // IAUDIOCALLBACKS_HPP
