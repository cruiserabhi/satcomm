# libtelux_audio.so:
#    This library contains implementation of Audio Services APIs in a simulated SDK environment.


# pkg-config support
configure_file("telux-audio.pc.in" "telux-audio.pc" @ONLY)

cmake_minimum_required(VERSION 3.10.2)

if (TELSDK_FEATURE_AUDIO)
    add_definitions(-DTELSDK_FEATURE_AUDIO_ENABLED)
endif (TELSDK_FEATURE_AUDIO)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -pthread")

set(TARGET_AUDIO_LIBRARY telux_audio)

# Audio logs filtering flag
add_definitions(-DTELUX_TECH_AREA=1)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)

set(AUDIO_STUB_SOURCES
    AudioStreamImpl.cpp
    VoiceStreamImpl.cpp
    AudioDeviceImpl.cpp
    AudioGrpcClientStub.cpp
    AudioFactoryImpl.cpp
    AudioManagerImpl.cpp
    AudioPlayerImpl.cpp
    PlayStreamImpl.cpp
    AudioBufferImpl.cpp
    StreamBufferImpl.cpp
    CaptureStreamImpl.cpp
    LoopbackStreamImpl.cpp
    ToneGeneratorStreamImpl.cpp
    TranscoderImpl.cpp
)

FILE(GLOB SOURCE "${PROTO_SRC_DIR}/*.cc")

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library (${TARGET_AUDIO_LIBRARY} SHARED ${SOURCE} ${AUDIO_STUB_SOURCES})

target_link_libraries(${TARGET_AUDIO_LIBRARY}
    ${_GRPC_GRPCPP}
    telux_protos
    telux_common
    )

install ( TARGETS ${TARGET_AUDIO_LIBRARY}
    LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR} )

install (FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-audio.pc"
DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
