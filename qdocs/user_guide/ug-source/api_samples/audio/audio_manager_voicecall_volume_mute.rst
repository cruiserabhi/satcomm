.. _audio-manager-voicecall-volume-mute:

Volume and mute controls 
==============================================================================

This sample application demonstrates how to set audio volume level, mute and unmute audio during an active voice session.

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

    // Create an audio stream (voice call session)
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
            std::cout << "startAudio() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "startAudio() succeeded." << std::endl;
    }

    status = audioVoiceStream->startAudio(startAudioCallback);


5. Set volume level for specified direction

.. code-block::

    // Callback which provides response to setVolume
    void setStreamVolumeCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "setVolume() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "setVolume() succeeded." << std::endl;
    }

    // Set volume on an audio stream for RX direction
    StreamVolume streamVol;
    ChannelVolume channelVol;
    streamVol.dir = StreamDirection::RX;
    channelVol.channelType = ChannelType::LEFT;
    channelVol.vol = 0.5;
    streamVol.volume.emplace_back(channelVol);

    status = audioVoiceStream->setVolume(streamVol, setStreamVolumeCallback);


6. Get current volume level of the audio stream

.. code-block::

    // Callback which provides response to getVolume
    void getStreamVolumeCallback(StreamVolume volume, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getVolume() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "Volume direction: " << static_cast<uint32_t>(volume.dir) << std::endl;

        int i = 0;
        for (auto channel_volume : volume.volume) {
            std::cout << "ChannelVolume [" << i << "] channel type: "
                << static_cast<uint32_t>(channel_volume.channelType) << ", " << "volume: "
                << channel_volume.vol << std::endl;
        }
    }

    status = audioVoiceStream->getVolume(StreamDirection::RX, getStreamVolumeCallback);


7. Mute the audio for the specified direction

.. code-block::

    // Callback which provides response to setMute
    void setStreamMuteCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "setMute() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "setMute() succeeded." << std::endl;
    }

    // Mute audio stream for TX direction
    StreamMute mute;
    mute.dir = StreamDirection::TX;
    mute.enable = true; //true: enable, false: disable

    status = audioVoiceStream->setMute(mute, setStreamMuteCallback);


8 Get mute state of stream for specified direction

.. code-block::

    // Callback which provides response to getMute
    void getStreamMuteCallback(StreamMute mute, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getMute() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "Mute enable: " << mute.enable << ", direction: "
            << static_cast<uint32_t>(mute.dir) << std::endl;

    }

    // Get mute state of stream for TX direction
    status = audioVoiceStream->getMute(StreamDirection::TX, getStreamMuteCallback);


9. Stop voice session

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

    // Stop voice session (which was started earlier)
    status = audioVoiceStream->stopAudio(stopAudioCallback);


10. Dispose the audio stream

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

    // Delete audio stream
    status = audioManager->deleteStream(std::dynamic_pointer_cast<IAudioStream>(audioVoiceStream),
                                        deleteStreamCallback);

