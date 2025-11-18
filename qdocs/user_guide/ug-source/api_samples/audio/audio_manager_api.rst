.. _audio-manager-stream:

Audio manager and stream
=========================

Audio Manager provides APIs to create audio streams and transcoder. It also helps in knowing supported audio devices and audio calibration status.

1. Get an AudioFactory instance

.. code-block::

   #include <telux/audio/AudioFactory.hpp>
   #include <telux/audio/AudioManager.hpp>

   using namespace telux::common;
   using namespace telux::audio;

   static std::shared_ptr<IAudioManager> audioManager;
   static std::shared_ptr<IAudioVoiceStream> audioVoiceStream;
   Status status;

   auto &audioFactory = AudioFactory::getInstance();


2. Get an AudioManager instance

.. code-block::

    std::promise<ServiceStatus> prom{};
    //  Get AudioManager instance.
    audioManager = audioFactory.getAudioManager([&prom](ServiceStatus serviceStatus) {
        prom.set_value(serviceStatus);
    });
    if (!audioManager) {
        std::cout << "Failed to get AudioManager instance" << std::endl;
        return;
    }
 

3. Let audio subsystem be ready

.. code-block::

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


4. Query supported audio devices

.. code-block::

    // Callback to get supported device type details
    void getDevicesCallback(std::vector<std::shared_ptr<IAudioDevice>> devices, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getDevices() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        int i = 0;
        for (auto device_type : devices) {
            std::cout << "Device [" << i << "] type: "
                << static_cast<unsigned int>(device_type->getType()) << ", direction: "
                << static_cast<unsigned int>(device_type->getDirection()) << std::endl;
            i++;
        }
    }

    // Query Supported Device type details
    status = audioManager->getDevices(getDevicesCallback);


5. Query supported audio stream types

.. code-block::

    // Callback to get supported stream type details
    void getStreamTypesCallback(std::vector<StreamType> streams, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getStreamTypes() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        int i = 0;
        for (auto stream_type : streams) {
            std::cout << "Stream [" << i << "] type: " << static_cast<unsigned int>(stream_type)
                << std::endl;
            i++;
        }
    }

    // Query Supported stream type details
    status = audioManager->getStreamTypes(getStreamTypesCallback);


6. Create an audio stream (voice call session)

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

    // Create an Audio Stream (Voice Call Session)
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


7. Delete an audio stream

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

    // Delete the stream (voice call session), which was created earlier
    status = audioManager->deleteStream(std::dynamic_pointer_cast<IAudioStream>(audioVoiceStream),
                                        deleteStreamCallback);

