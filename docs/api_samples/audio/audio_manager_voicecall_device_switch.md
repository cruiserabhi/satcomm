Switching audio device {#audio_audio_manager_voicecall_device_switch}
=================================================================================================================

This sample application demonstrates how to use audio APIs for switching audio devices during active voice session.

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

### 3. Create a voice call session

   ~~~~~~{.cpp}
    // Callback which provides response to createStream, with pointer to base interface IAudioStream
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "createStream() succeeded." << std::endl;
        audioVoiceStream = std::dynamic_pointer_cast<IAudioVoiceStream>(stream);
    }

    // Create an audio stream representing a voice call session
    StreamConfig config;

    config.type = StreamType::VOICE_CALL;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = 16000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    // For StreamType::VOICE_CALL, sink and source device are required to be passed.
    // First device requires to sink (speaker) then source (mic) should be passed.
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_MIC);

    status = audioManager->createStream(config, createStreamCallback);
   ~~~~~~

### 4. Start voice call session

   ~~~~~~{.cpp}
    // Callback which provides response to startAudio
    void startAudioCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "startAudio() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "startAudio() succeeded." << std::endl;
    }

    status = audioVoiceStream->startAudio(startAudioCallback);
   ~~~~~~

### 5. Device switch on Started Audio Stream (Voice Call Session)

   ~~~~~~{.cpp}
    // Callback which provides response to setDevice
    void setStreamDeviceCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "setDevice() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "setDevice() succeeded." << std::endl;
    }

    // Switch to new device for the given audio stream
    std::vector<DeviceType> devices;
    /*For StreamType::VOICE_CALL, sink and source device are required to be passed.
      First device should be sink (speaker) and then source (mic) should be passed for stream creation.*/
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_MIC);
    status = audioVoiceStream->setDevice(devices, setStreamDeviceCallback);
   ~~~~~~

### 6. Query device used for a given audio stream

   ~~~~~~{.cpp}
    // Callback which provides response to getDevice
    void getStreamDeviceCallback(std::vector<DeviceType> devices, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getDevice() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        int i = 0;
        for (auto device_type : devices) {
            std::cout << "Device [" << i << "] type: " << static_cast<uint32_t>(device_type)
                << std::endl;
            i++;
        }
    }

    status = audioVoiceStream->getDevice(getStreamDeviceCallback);
   ~~~~~~

### 7. Stop voice call session

   ~~~~~~{.cpp}

    // Callback which provides response to stopAudio
    void stopAudioCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "stopAudio() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "stopAudio() succeeded." << std::endl;
    }

    status = audioVoiceStream->stopAudio(stopAudioCallback);
   ~~~~~~

### 8. Dispose audio stream

   ~~~~~~{.cpp}
    //Callback which provides response to deleteStream
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "deleteStream() succeeded." << std::endl;
        audioVoiceStream.reset();
    }

    status = audioManager->deleteStream(std::dynamic_pointer_cast<IAudioStream>(audioVoiceStream),
                                        deleteStreamCallback);
   ~~~~~~
