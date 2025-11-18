cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SIMULATION_SERVER_APP_SRC
    audio/AudioGrpcServiceImpl.cpp
    audio/AudioServiceImpl.cpp
    audio/StreamCache.cpp
    audio/ClientCache.cpp
    audio/Stream.cpp
    audio/AudioClient.cpp
    audio/AudioRequest.cpp
    audio/Alsa.cpp
    audio/AudioJsonHelper.cpp
)

target_sources (${TARGET_SIMULATION_SERVER_APP} PRIVATE ${TARGET_SIMULATION_SERVER_APP_SRC})
