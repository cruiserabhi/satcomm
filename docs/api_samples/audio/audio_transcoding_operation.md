Transcoding samples {#audio_transcoding_operation}
==========================================================

This sample app demonstrates how to use the audio APIs for transcoding audio samples.

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

### 3. Create an audio transcoder

   ~~~~~~{.cpp}
    FormatInfo inputConfig_;
    FormatInfo outputConfig_;
    // input and output parameters are configured for AMRWB_PLUS
    // to PCM_16BIT_SIGNED transcoding operation as an example.
    AmrwbpParams inputParams{};
    inputConfig_.sampleRate = SAMPLE_RATE;
    inputConfig_.mask = CHANNEL_MASK;
    inputConfig_.format = AudioFormat::AMRWB_PLUS;
    inputParams.bitWidth = 16;
    inputParams.frameFormat = AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
    inputConfig_.params = &inputParams;

    inputConfig_.sampleRate = SAMPLE_RATE;
    outputConfig_.mask = CHANNEL_MASK;
    outputConfig_.format = AudioFormat::PCM_16BIT_SIGNED;
    outputConfig_.params = nullptr;

    audioManager->createTranscoder(inputConfig_, outputConfig_,
    [&p,this](std::shared_ptr<telux::audio::ITranscoder> &transcoder,
        telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            transcoder_ = transcoder;
            registerListener();
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "failed to create transcoder" <<std::endl;
        }
    });
    if (p.get_future().get()) {
        std::cout<< "Transcoder Created" << std::endl;
    }
   ~~~~~~

### 4.1 Allocate audio buffers for write operation

   ~~~~~~{.cpp}
    // Get an audio buffer for write operation (we can get more than one)
    auto audioBuffer = transcoder_->getWriteBuffer();
    if (audioBuffer != nullptr) {
        // Setting the size of buffer that need to be supplied for write operation as the minimum
        // size required by transcoder. In any case if size returned is 0, using the Maximum
        // Buffer Size, any buffer size between min and max can be used
        size = audioBuffer->getMinSize();
        if (size == 0) {
            size =  audioBuffer->getMaxSize();
        }
        audioBuffer->setDataSize(size);
    } else {
        std::cout << "Failed to get Audio Buffer for write operation " << std::endl;
    }
   ~~~~~~

### 4.2 Allocate audio buffers for read operation

   ~~~~~~{.cpp}
    // Get an audio buffer for read operation (we can get more than one)
    auto audioBuffer = transcoder_->getReadBuffer();
    if (audioBuffer != nullptr) {
        // Setting the size of buffer that need to be supplied for read operation as the minimum
        // size required by transcoder. In any case if size returned is 0, using the Maximum
        // Buffer Size, any buffer size between min and max can be used
        size = audioBuffer->getMinSize();
        if (size == 0) {
            size =  audioBuffer->getMaxSize();
        }
        audioBuffer->setDataSize(size);
    } else {
        std::cout << "Failed to get Audio Buffer for read operation " << std::endl;
    }
   ~~~~~~

### 5. Start write operation in one thread for transcoding

   ~~~~~~{.cpp}
    // Callback which provides response to write operation
    void writeCallback(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t bytes, ErrorCode error) {
        std::cout << "Bytes Written : " << bytes << std::endl;
        if (error != ErrorCode::SUCCESS || buffer->getDataSize() != bytes) {
            // Application needs to resend the Bitstream buffer from leftover position if bytes
            // consumed are not equal to requested number of bytes to be written.
            pipeLineEmpty_ = false;
        }
        buffer->reset();
        writeBuffers_.push(buffer);
        cv_.notify_all();
        return;
    }

    // Indiction Received only when callback returns with error that bytes written are not equal to
    // bytes requested to write. It notifies that pipeline is ready to accept new buffer to write.

    void onReadyForWrite() {
        pipeLineEmpty_ = true;
    }

    // Write request for transcoding
    auto writeCb = std::bind(&TranscoderApp::writeCallback, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3);

    telux::common::Status status = telux::common::Status::FAILED;

    // EOF_REACHED denotes flag which indicated EOF is reached
    // EOF_NOT_REACHED denotes flag which indicated EOF is not reached
    if (EOF_REACHED) {
        status = transcoder_->write(audioBuffer, EOF_REACHED,  writeCb);
    } else {
        status = transcoder_->write(audioBuffer, EOF_NOT_REACHED,  writeCb);
    }

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "write() failed with error" << static_cast<unsigned int>(status) << std::endl;
    } else {
        std::cout << "Request to transcode buffers sent " << std::endl;
    }
   ~~~~~~

### 6. Start read operation in another thread for transcoding

   ~~~~~~{.cpp}
    // Callback which provides response to read operation
    void readCallback(std::shared_ptr<telux::audio::IAudioBuffer> buffer,
        uint32_t isLastBuffer, telux::common::ErrorCode error) {
        if (isLastBuffer) {
            // Stop reading from now onwards as this is the last transcoded buffer
        }
        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "read() returned with error " << static_cast<unsigned int>(error)
                    << std::endl;
        } else {
            // readed buffer can be stored on a file if required.
        }
        buffer->reset();
        readBuffers_.push(buffer);
        cv_.notify_all();
        return;
    }

    // Read request for transcoding
    auto readCb =  std::bind(&TranscoderApp::readCallback, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    telux::common::Status status = transcoder_->read(audioBuffer, bytesToRead, readCb);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "read() failed with error" << static_cast<unsigned int>(status) << std::endl;
    }
   ~~~~~~

### 7. Tear down the audio transcoder instance

   ~~~~~~{.cpp}
    // Tear down is supposed to be called after the last buffer is received
    // for write operation. It is supposed to be called after every transcoding
    // operation as transcoder instance can not be used for multiple transcoding
    // operations.

    std::promise<bool> p;
    auto status = transcoder_->tearDown([&p](telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to tear down" << std::endl;
        }
    });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Request to Teardown transcoder sent" << std::endl;
    } else {
        std::cout << "Request to Teardown transcoder failed" << std::endl;
    }

    if (p.get_future().get()) {
        transcoder_ = nullptr;
        std::cout << "Tear Down successful !!" << std::endl;
    }
   ~~~~~~
