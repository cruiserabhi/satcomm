/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOGRPCCLIENTSTUB_HPP
#define AUDIOGRPCCLIENTSTUB_HPP

#include <grpcpp/grpcpp.h>
#include <unordered_map>

#include "protos/proto-src/audio_simulation.grpc.pb.h"
#include "common/TaskDispatcher.hpp"
#include "AudioDefinesLibInternal.hpp"
#include "ICommunicator.hpp"
#include "common/ListenerManager.hpp"
#include "common/CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;

using audioStub::AudioService;

/* std::unordered_map is implemented by means of a hash table. std::hash is not defined for std::set
   Therefore, adding hashing function using the third template parameter of unordered_map.
 */
struct keyPairHash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);

        return hash1 ^ hash2;
    }
};

namespace telux {
namespace audio {

/*
 * When sending request, converts telsdk specific data format to protobuf format.
 *
 * When receiving response/indication, converts protobuf format data to telsdk
 * specific format.
 *
 * It uses APIs from StubInterface of AudioService to exchange messages with GRPC framework.
 */
class AudioGrpcClientStub : public ICommunicator,
                            public std::enable_shared_from_this<AudioGrpcClientStub> {

 public:
    AudioGrpcClientStub();
    ~AudioGrpcClientStub();

    telux::common::Status setup() override;

    /* Sending grpc requests */

    telux::common::Status getDevices(
        std::shared_ptr<telux::audio::IGetDevicesCb> resultListener, int cmdId) override;

    telux::common::Status getStreamTypes(
        std::shared_ptr<telux::audio::IGetStreamsCb> resultListener, int cmdId) override;

    telux::common::Status getCalibrationInitStatus(
        std::shared_ptr<telux::audio::IGetCalInitStatusCb> resultListener, int cmdId) override;

    telux::common::Status createStream(telux::audio::StreamConfig streamConfig,
        std::shared_ptr<telux::audio::ICreateStreamCb> resultListener, int cmdId) override;

    telux::common::Status deleteStream(uint32_t streamId,
        std::shared_ptr<telux::audio::IDeleteStreamCb> resultListener, int cmdId) override;

    telux::common::Status createTranscoder(telux::audio::FormatInfo inInfo,
        telux::audio::FormatInfo outInfo,
        std::shared_ptr<telux::audio::ITranscodeCreateCb> resultListener, int cmdId) override;

    telux::common::Status deleteTranscoder(uint32_t inStreamId, uint32_t outStreamId,
        std::shared_ptr<telux::audio::ITranscodeDeleteCb> resultListener, int cmdId) override;

    telux::common::Status startStream(uint32_t streamId,
        std::shared_ptr<telux::audio::IStartStreamCb> resultListener, int cmdId) override;

    telux::common::Status stopStream(uint32_t streamId,
        std::shared_ptr<telux::audio::IStopStreamCb> resultListener, int cmdId) override;

    telux::common::Status playDtmfTone(telux::audio::DtmfTone dtmfTone, uint16_t duration,
        uint16_t gain, uint32_t streamId,
        std::shared_ptr<telux::audio::IDTMFCb> resultListener, int cmdId) override;

    telux::common::Status stopDtmfTone(telux::audio::StreamDirection direction,
        uint32_t streamId, std::shared_ptr<telux::audio::IDTMFCb> resultListener,
        int cmdId) override;

    telux::common::Status setDevice(uint32_t streamId,
        std::vector<telux::audio::DeviceType> devices,
        std::shared_ptr<telux::audio::ISetGetDeviceCb> resultListener, int cmdId) override;

    telux::common::Status getDevice(uint32_t streamId,
        std::shared_ptr<telux::audio::ISetGetDeviceCb> resultListener, int cmdId) override;

    telux::common::Status setVolume(uint32_t streamId, telux::audio::StreamVolume volume,
        std::shared_ptr<telux::audio::ISetGetVolumeCb> resultListener, int cmdId) override;

    telux::common::Status getVolume(uint32_t streamId, telux::audio::StreamDirection direction,
        std::shared_ptr<telux::audio::ISetGetVolumeCb> resultListener, int cmdId) override;

    telux::common::Status setMute(uint32_t streamId, telux::audio::StreamMute mute,
        std::shared_ptr<telux::audio::ISetGetMuteCb> resultListener, int cmdId) override;

    telux::common::Status getMute(uint32_t streamId, telux::audio::StreamDirection direction,
        std::shared_ptr<telux::audio::ISetGetMuteCb> resultListener, int cmdId) override;

    telux::common::Status write(uint32_t streamId, uint8_t *transportBuffer,
        uint32_t isLastBuffer, std::shared_ptr<telux::audio::IWriteCb> resultListener,
        telux::audio::AudioUserData *userData, uint32_t dataLength) override;

    telux::common::Status read(uint32_t streamId, uint32_t numBytesToRead,
        uint8_t *transportBuffer, std::shared_ptr<telux::audio::IReadCb> resultListener,
        telux::audio::AudioUserData *audioUserData) override;

    telux::common::Status playTone(uint32_t streamId, std::vector<uint16_t> frequency,
        uint16_t duration, uint16_t gain,
        std::shared_ptr<telux::audio::IToneCb> resultListener, int cmdId) override;

    telux::common::Status stopTone(uint32_t streamId,
        std::shared_ptr<telux::audio::IToneCb> resultListener, int cmdId) override;

    telux::common::Status registerForVoiceStreamEvents(uint32_t streamId,
        std::weak_ptr<telux::audio::IVoiceStreamEventsCb> listener) override;

    telux::common::Status registerForServiceStatusEvents(
        std::weak_ptr<telux::audio::IServiceStatusEventsCb> listener) override;

    telux::common::Status registerForPlayStreamEvents(
        std::weak_ptr<telux::audio::IPlayStreamEventsCb> listener) override;

    telux::common::Status flush(uint32_t streamId,
        std::shared_ptr<telux::audio::IFlushCb> resultListener, int cmdId) override;

    telux::common::Status drain(uint32_t streamId,
        std::shared_ptr<telux::audio::IDrainCb> resultListener, int cmdId) override;

    bool waitForInitialization();
    /**
    * Checks the status of grpc Service and returns the result.
    *
    * @returns True if grpc Service is ready for service otherwise false.
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to
    *         change and could break backwards compatibility.
    */
    bool isReady() override;

   /**
    * Wait for grpc Service to be ready.
    *
    * @returns  A future that caller can wait on to be notified when
    *           subsystem is ready.
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to
    *         change and could break backwards compatibility.
    */
    std::future<bool> onReady() override;

    /* Receiving GRPC responses */

    void onGetDevices(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onGetStreamTypes(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onCreateStream(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onDeleteStream(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onCreateTranscoder(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onDeleteTranscoder(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onStartStream(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onStopStream(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onSetDevice(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onGetDevice(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onSetVolume(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onGetVolume(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onSetMuteState(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onGetMuteState(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onPlayDtmfTone(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onStopDtmfTone(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onGetCalibrationInitStatus(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onWrite(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onRead(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onPlayTone(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onStopTone(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onDrain(google::protobuf::Any any, int cmdId, ErrorCode ec);

    void onFlush(google::protobuf::Any any, int cmdId, ErrorCode ec);

    /* Receiving GRPC indication */

    void onDtmfToneDetected(::audioStub::DtmfTone dtmfTone);
    void onSSRUpdate(commonStub::GetServiceStatusReply serviceStatus);
    void onDrainDone(audioStub::DrainEvent drainEvent);
    void onWriteReady(audioStub::WriteReadyEvent writeReadyEvent);

    /* Set to true to indicate - destruction is started */
    static std::atomic<bool> exitNow_;
    /* Protect against concurrent SSR and AudioGrpcClient destruction */
    static std::mutex destructorGuard_;

 private:
    void createServerStreaming();
    uint32_t inTranscodeStreamId_;
    uint32_t outTranscodeStreamId_;

    std::shared_ptr<telux::common::ListenerManager<
        telux::audio::IVoiceStreamEventsCb>> voiceListenerMgr_;

    std::shared_ptr<telux::common::ListenerManager<
        telux::audio::IPlayStreamEventsCb>> playListenerMgr_;

    std::shared_ptr<telux::common::ListenerManager<
         telux::audio::IServiceStatusEventsCb>> serviceStatusListenerMgr_;

    std::unique_ptr<telux::common::TaskDispatcher> serverMsgProcessor_;
    std::unique_ptr<::audioStub::AudioService::Stub> stub_;
    //Stores a resultListener for a request on a particular stream.
    std::unordered_map<std::pair<int ,int>, std::weak_ptr<telux::common::ICommandCallback>,
        keyPairHash> callbackMap_;
    //Stores audio user data for read/write requests.
    std::unordered_map<std::pair<int ,int>, telux::audio::AudioUserData*, keyPairHash> userDataMap_;
    std::mutex update_;
    // Check the readiness of the service
    std::mutex grpcClientMutex_;
    std::condition_variable cv_;
    // Check the readiness of the service
    telux::common::ServiceStatus serviceReady_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::vector<std::thread> runningThreads_;
    std::atomic<bool> exiting_;

    AudioGrpcClientStub(const AudioGrpcClientStub &) = delete;
    AudioGrpcClientStub &operator=(const AudioGrpcClientStub &) = delete;
};

}  // end of namespace audio
}  // end of namespace telux

#endif //AUDIOGRPCCLIENTSTUB_HPP
