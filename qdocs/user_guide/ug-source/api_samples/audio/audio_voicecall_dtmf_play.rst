.. _audio-voicecall-dtmf-play:

Play DTMF tone
========================================================================================

This sample app demostrates how to use audio APIs to play DTMF tones during an active voice call.
Please note that only Rx direction is supported currently.

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


3. Create an audio stream (to be associated with a voice call session)

.. code-block::

    // Callback which provides response to createStream
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "createStream() succeeded." << std::endl;
        audioVoiceStream = std::dynamic_pointer_cast<IAudioVoiceStream>(stream);
    }

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


4. Start the voice call session

.. code-block::

    // Callback which provides response to startAudio
    void startAudioCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "startAudio() failed with error " << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "startAudio() succeeded." << std::endl;
    }

    status = audioVoiceStream->startAudio(startAudioCallback);


5. Play a DTMF tone

.. code-block::

    // Implement a response function to get the request status
    void playDtmfCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "playDtmfTone() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "playDtmfTone() succeeded." << std::endl;
    }

    // Play the DTMF tone with required configuration
    status = audioVoiceStream->playDtmfTone(dtmfTone, duration, gain, playDtmfCallback);


6. Stop the voice call session

.. code-block::

    // Callback which provides response to stopAudio
    void stopAudioCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "stopAudio() failed with error " << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "stopAudio() succeeded." << std::endl;
    }

    status = audioVoiceStream->stopAudio(stopAudioCallback);


7. Dispose the audio stream associated with the voice call session

.. code-block::

    // Implement a response callback method to get the request status
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioVoiceStream->reset();
    }

    status = audioManager->deleteStream(std::dynamic_pointer_cast<IAudioStream>(audioVoiceStream),
                                    deleteStreamCallback);

