Playback on voice paths {#compressed_audio_format_playback_on_voice_paths}
==================================================================================================

This sample app demonstrates how to use the audio APIs for compressed audio format playback on voice paths.

### 1. Get the AudioFactory instance

   ~~~~~~{.cpp}
    auto &audioFactory = AudioFactory::getInstance();
   ~~~~~~

### 2. Get the AudioManager instance and check for audio subsystem readiness

   ~~~~~~{.cpp}
    std::promise<ServiceStatus> prom{};
    //  Get AudioManager instance.
    audioManager = audioFactory.getAudioManager([&prom](ServiceStatus serviceStatus) {
        prom.set_value(serviceStatus);
    });
    if (!audioManager) {
        std::cout << "Failed to get AudioManager instance" << std::endl;
        return;
    }

    //  Check if audio subsystem is ready
    //  If audio subsystem is not ready, wait for it to be ready
    ServiceStatus managerStatus = audioManager->getServiceStatus();
    if (managerStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nAudio subsystem is not ready, Please wait ..." << std::endl;
        managerStatus = prom.get_future().get();
    }

    //  Check the service status again
    if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Subsytem is Ready << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize audio subsystem" << std::endl;
        return;
    }
   ~~~~~~

### 3. Create an audio playback session with voice path direction

   ~~~~~~{.cpp}
    StreamConfig config;

    config.type = StreamType::PLAY;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = SAMPLE_RATE;
    config.format = AudioFormat::AMRWB_PLUS;

    // here both channel selected, this can be selected according to requirement
    config.channelTypeMask = (ChannelType::LEFT | ChannelType::RIGHT);

    // Since the voice path is selected, we don't need to provide any device
    // Voice path direction TX is for Voice uplink while direction RX is for Voice downlink
    config.voicePaths.emplace_back(Direction::TX);

    // Passing Decoder Specific Configuration, refer header file for more details.
    AmrwbpParams amrParams{};
    if (config.format == AudioFormat::AMRWB_PLUS) {
        amrParams.bitWidth = 16;
        amrParams.frameFormat = AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
        config.formatParams = &amrParams;
    } else {
        config.formatParams = nullptr;
    }

    std::promise<bool> p;
    auto status = audioManager->createStream(config,
        [&p,this](std::shared_ptr<IAudioStream> &audioStream, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                audioPlayStream_ = std::dynamic_pointer_cast<IAudioPlayStream>(audioStream);
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "failed to Create a stream" <<std::endl;
            }
        });
    if (status == Status::SUCCESS) {
        std::cout << "Request to create stream sent" << std::endl;
    } else {
        std::cout << "Request to create stream failed"  << std::endl;
    }

    if (p.get_future().get()) {
        std::cout<< "Audio Play Stream is Created" << std::endl;
    }
   ~~~~~~

### 4. Allocate stream buffers for playback operation

   ~~~~~~{.cpp}
    // Get an audio buffer (we can get more than one)
    auto streamBuffer = audioPlayStream->getStreamBuffer();
    if (streamBuffer != nullptr) {
        // Setting the size that is to be written to stream as the minimum size
        // required by stream. In any case if size returned is 0, using the Maximum
        // Buffer Size, any buffer size between min and max can be used
        size = streamBuffer->getMinSize();
        if (size == 0) {
            size =  streamBuffer->getMaxSize();
        }
        streamBuffer->setDataSize(size);
    } else {
        std::cout << "Failed to get Stream Buffer " << std::endl;
    }
   ~~~~~~

### 5. Start write operation for playback to start

   ~~~~~~{.cpp}
    // We need an active voice session to play on voice paths.
    // Callback which provides response to write operation.
    void writeCallback(std::shared_ptr<IStreamBuffer> buffer, uint32_t bytes, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "write() returned with error " << static_cast<int>(error) << std::endl;
            // Application needs to resend the Bitstream buffer from leftover position if bytes
            // consumed are not equal to requested number of bytes to be written.
            pipeLineEmpty_ = false;
        }
        buffer->reset();
        return;
    }

    // Indication Received only when callback returns with error that bytes written
    // are not equal to bytes requested to write. It notifies that pipeline is ready
    // to accept new buffer to write
    void onReadyForWrite() {
        pipeLineEmpty_ = true;
    }

    // Write desired data into the buffer, the bytes sent as 0x1 for example purpose only.
    // First write starts Playback Session.
    memset(streamBuffer->getRawBuffer(),0x1,size);

    auto status = audioPlayStream->write(streamBuffer, writeCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "write() failed with error" << static_cast<int>(status) << std::endl;
    } else {
        std::cout << "Request to write to stream sent" << std::endl;
    }
   ~~~~~~

### 6.1 Stop playback operation(STOP_AFTER_PLAY : Stops after playing pending buffers in pipeline)

   ~~~~~~{.cpp}
    std::promise<bool> p;
    auto status = audioPlayStream_->stopAudio(StopType::STOP_AFTER_PLAY, [&p](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to stop after playing buffers" << std::endl;
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to stop playback after pending buffers Sent" << std::endl;
    } else {
        std::cout << "Request to stop playback after pending buffers failed" << std::endl;
    }
    if (p.get_future().get()) {
        std::cout << "Pending buffers played successful !!" << std::endl;
    }
   ~~~~~~

### 6.2 Stop playback operation(FORCE_STOP : Stops immediately, all buffers in pipeline are flushed)

   ~~~~~~{.cpp}
    std::promise<bool> p;
        auto status = audioPlayStream_->stopAudio(
            StopType::FORCE_STOP, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to force stop" << std::endl;
            }
            });
        if(status == telux::common::Status::SUCCESS){
            std::cout << "Request to force stop Sent" << std::endl;
        } else {
            std::cout << "Request to force stop failed" << std::endl;
        }
        if (p.get_future().get()) {
                std::cout << "Force Stop successful !!" << std::endl;
        }
   ~~~~~~

### 7. Dispose the audio stream, once end of operation is reached

   ~~~~~~{.cpp}
    std::promise<bool> p;
    Status status = audioManager-> deleteStream(
    audioPlayStream_, [&p,this](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to delete a stream" << std::endl;
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to delete stream sent" << std::endl;
    } else {
        std::cout << "Request to delete stream failed"  << std::endl;
    }
    if (p.get_future().get()) {
        audioPlayStream_= nullptr;
        std::cout << "Audio Play Stream is Deleted" << std::endl;
    }
   ~~~~~~
