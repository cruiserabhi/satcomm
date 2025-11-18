.. _audio-manager-loopback:

Loopback session 
=======================================================================================

This sample application demonstrates how to create a loopback audio stream, start and stop the loopback session.

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
        std::cout << "Failed to get AudioManager object" << std::endl;
        return;
    }

    // Check if audio subsystem is ready
    // If audio subsystem is not ready, wait for it to be ready
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


3. Create an audio Stream (to be associated with loopback)

.. code-block::

    // Implement a response callback method to get the request status
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "createStream() succeeded." << std::endl;
        audioLoopbackStream = std::dynamic_pointer_cast<IAudioLoopbackStream>(stream);
    }

    // Create a loopback stream with required configuration
    StreamConfig config;

    config.type = telux::audio::StreamType::LOOPBACK;
    config.sampleRate = 48000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_MIC);

    status = audioManager->createStream(config, createStreamCallback);


4. Start loopback between the specified source and sink devices

.. code-block::

    // Implement a response callback method to get the request status
    void startLoopbackCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "startLoopback() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "startLoopback() succeeded." << std::endl;
    }

    // start loopback
    status = audioLoopbackStream->startLoopback(startLoopbackCallback);


5. Stop loopback between the specified source and sink devices

.. code-block::

    // Implement a response callback method to get the request status
    void stopLoopbackCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "stopLoopback() failed with error " << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "stopLoopback() succeeded." << std::endl;
    }

    // Stop the loopback session (which was started earlier)
    status = audioLoopbackStream->stopLoopback(stopLoopbackCallback);


6. Dispose the audio stream associated with the loopback session

.. code-block::

    // Implement a response callback method to get the request status
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioLoopbackStream.reset();
    }

    // Delete the audio stream
    status = audioManager->deleteStream(std::dynamic_pointer_cast<IAudioStream>(audioLoopbackStream),
                                    deleteStreamCallback);

