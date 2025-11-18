Capture session {#audio_manager_capture}
=====================================================================================

This sample application demonstrates how to use audio APIs for recording audio.

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

    // Check if audio subsystem is ready
    // If audio subsystem is not ready, wait for it to be ready
    ServiceStatus managerStatus = audioManager->getServiceStatus();
    if (managerStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nAudio subsystem is not ready, Please wait ..." << std::endl;
        managerStatus = prom.get_future().get();
    }

    // Check the service status again
    if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Subsytem is Ready << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize audio subsystem" << std::endl;
        return;
    }
   ~~~~~~

### 3. Create a capture audio session

   ~~~~~~{.cpp}
    // Callback which provides response to createStream, with pointer to base interface IAudioStream
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() returned with error " << static_cast<int>(error)
                << std::endl;
            return;
        }
        std::cout << "createStream() succeeded." << std::endl;
        audioCaptureStream = std::dynamic_pointer_cast<IAudioCaptureStream>(stream);
    }

    // Create an audio stream
    StreamConfig config;

    config.type = StreamType::CAPTURE;
    config.sampleRate = 48000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);

    status = audioManager->createStream(config, createStreamCallback);
   ~~~~~~

### 4. Allocate stream buffers for audio capture operation

   ~~~~~~{.cpp}
    // Get an audio buffer (we can get more than one)
    auto streamBuffer = audioCaptureStream->getStreamBuffer();
    if(streamBuffer != nullptr) {
        // Setting the bytesToRead (bytes to be read from stream) as minimum size
        // required by stream. In any case if size returned is 0, using the Maximum Buffer
        // Size, any buffer size between min and max can be used
        bytesToRead = streamBuffer->getMinSize();
        if(bytesToRead == 0) {
            bytesToRead = streamBuffer->getMaxSize();
        }
    } else {
        std::cout << "Failed to get Stream Buffer " << std::endl;
        return EXIT_FAILURE;
    }
   ~~~~~~

### 5. Start read operation for the capture to start

   ~~~~~~{.cpp}
    // Callback which provides response to read operation
    void readCallback(std::shared_ptr<IStreamBuffer> buffer, ErrorCode error)
    {
        uint32_t bytesWrittenToFile = 0;
        if (error != ErrorCode::SUCCESS) {
            std::cout << "read() returned with error " << static_cast<int>(error) << std::endl;
        } else {
            uint32_t size = buffer->getDataSize();
            std::cout << "Successfully read " << size << " bytes" << std::endl;
        }
        buffer->reset();
        return;
    }
    // Read from Capture
    // Very first read starts capture session
    auto status = audioCaptureStream->read(streamBuffer, bytesToRead, readCallback);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << "read() failed with error" << static_cast<int>(status) << std::endl;
    } else {
        std::cout << "Request to read stream sent" << std::endl;
    }
   ~~~~~~

### 6. Dispose audio stream once required number of samples are captured

   ~~~~~~{.cpp}
    // Callback which provides response to deleteStream
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() returned with error " << static_cast<int>(error)
                        << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioCaptureStream.reset();
    }

    // Delete the audio stream
    Status  status = audioManager->deleteStream(
               std::dynamic_pointer_cast<IAudioStream>(audioCaptureStream), deleteStreamCallback);
    if (status != Status::SUCCESS) {
        std::cout << "deleteStream failed with error" << static_cast<int>(status) << std::endl;
    }
   ~~~~~~
