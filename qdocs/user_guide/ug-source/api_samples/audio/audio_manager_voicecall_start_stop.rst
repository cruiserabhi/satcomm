.. _audio-manager-voicecall-start-stop:

Voice call session 
========================================================================

This sample application demonstrates how to use audio APIs for a voice call session.

1. Get the AudioFactory instance

.. code-block::

    auto &audioFactory = AudioFactory::getInstance();


2. Get the AudioManager instance and check for audio subsystem readiness

.. code-block::

    std::promise<ServiceStatus> prom{};
    // Get AudioManager instance.
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


3. Create a voice call session

.. code-block::

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
    /*For StreamType::VOICE_CALL, sink and source device are required to be passed.
      First device should be sink (speaker) and then source (mic) should be passed
      for stream creation.*/
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_MIC);

    status = audioManager->createStream(config, createStreamCallback);


4. Start voice call session

.. code-block::

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


5. Stop voice call session

.. code-block::

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


6. Dispose the audio stream

.. code-block::

    // Callback which provides response to deleteStream
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "deleteStream() succeeded." << std::endl;
        audioVoiceStream.reset();
    }

    // Delete the audio stream (which was created earlier)
    status = audioManager->deleteStream(std::dynamic_pointer_cast<IAudioStream>(audioVoiceStream),
                                        deleteStreamCallback);

