.. _audio-manager-playback:

Playback session
=======================================================================================

This sample application demonstrates how to use audio APIs for a playback session.

1. Get the AudioFactory instance

.. code-block::

    auto &audioFactory = AudioFactory::getInstance();


2. Get the AudioManager instance and check for audio subsystem readiness

.. code-block::

    std::promise<ServiceStatus> prom{};
    // Get the AudioManager instance
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

    // Check the audio service status again
    if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Subsytem is Ready << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize audio subsystem" << std::endl;
        return;
    }


3. Create an audio playback session

.. code-block::

   // Callback which provides response to createStream, with pointer to base interface IAudioStream
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() returned with error " << static_cast<int>(error)
                << std::endl;
            return;
        }
        std::cout << "createStream() succeeded." << std::endl;
        audioPlayStream = std::dynamic_pointer_cast<IAudioPlayStream>(stream);
    }

    // Create an audio stream
    StreamConfig config;

    config.type = StreamType::PLAY;
    config.sampleRate = 48000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);

    status = audioManager->createStream(config, createStreamCallback);


4. Allocate stream buffers for playback operation

.. code-block::

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


5. Start write operation for playback to start

.. code-block::

    // Callback which provides response to write operation
    void writeCallback(std::shared_ptr<IStreamBuffer> buffer, uint32_t size, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "write() returned with error " << static_cast<int>(error) << std::endl;
        } else {
            std::cout << "Successfully written " << size << " bytes" << std::endl;
        }
        buffer->reset();
        return;
    }

    // Write desired data into the buffer
    // Very first write starts the playback session
    memset(streamBuffer->getRawBuffer(),0x1,size);

    auto status = audioPlayStream->write(streamBuffer, writeCallback);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << "write() failed with error" << static_cast<int>(status) << std::endl;
    } else {
        std::cout << "Request to write to stream sent" << std::endl;
    }


6. Dispose the audio stream

.. code-block::

    // Callback which provides response to deleteStream
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() returned with error " << static_cast<int>(error)
                << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioPlayStream.reset();
    }

    // Delete the audio stream
    Status  status = audioManager->deleteStream(
               std::dynamic_pointer_cast<IAudioStream>(audioPlayStream), deleteStreamCallback);
    if (status != Status::SUCCESS) {
        std::cout << "deleteStream failed with error" << static_cast<int>(status) << std::endl;
    }

