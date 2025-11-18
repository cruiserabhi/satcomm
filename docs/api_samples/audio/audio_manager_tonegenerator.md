Tone generation {#audio_manager_tonegenerator}
====================================================

This sample application demonstrates how to use audio APIs to play a tone using tone generator stream.

### 1. Get the AudioFactory instance

   ~~~~~~{.cpp}
    auto &audioFactory = AudioFactory::getInstance();
   ~~~~~~

### 2. Get the AudioManager instance and check for audio subsystem readiness

   ~~~~~~{.cpp}
    std::promise<ServiceStatus> prom{};
    // Get AudioManager instance
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

    // Check the service status again
    if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Subsytem is Ready << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize audio subsystem" << std::endl;
        return;
    }
   ~~~~~~

### 3. Create an audio stream (to be associated with tone generator)

   ~~~~~~{.cpp}
    // Implement a response callback method to get the request status
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "createStream() succeeded." << std::endl;
        audioToneGeneratorStream = std::dynamic_pointer_cast<IAudioToneGeneratorStream>(stream);
    }
    // Create a tone generator stream with required configuration
    StreamConfig config;

    config.type = telux::audio::StreamType::TONE_GENERATOR;
    config.sampleRate = 48000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);

    status = audioManager->createStream(config, createStreamCallback);
   ~~~~~~

### 4. Play the audio tone on a sink device

   ~~~~~~{.cpp}
    // Implement a response callback method to get the request status
    void playToneCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "playTone() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "playTone() succeeded." << std::endl;
    }

    // Play the tone with required configuration
    status = audioToneGeneratorStream->playTone(freq, duration, gain, playToneCallback);
   ~~~~~~

### 5. Optionally, you can stop the tone being played before the specified duration elapses

   ~~~~~~{.cpp}
    // Implement a response callback method to get the request status
    void stopToneCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "stopTone() failed with error " << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "stopTone() succeeded." << std::endl;
    }

    // Stop playing the tone (which was started earlier)
    status = audioToneGeneratorStream->stopTone(stopToneCallback);
   ~~~~~~

### 6. Dispose the audio stream associated with the tone generator session

   ~~~~~~{.cpp}
    // Implement a response callback method to get the request status
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioToneGeneratorStream.reset();
    }

    // Delete the Audio Stream
    status = audioManager->deleteStream(
        std::dynamic_pointer_cast<IAudioStream>(audioToneGeneratorStream), deleteStreamCallback);
   ~~~~~~
