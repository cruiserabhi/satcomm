cmake_minimum_required(VERSION 3.10.2)

set(TARGET_AUDIO_CONSOLE_APP audio_console_app)

set(AUDIO_CONSOLE_APP_SOURCES
    AudioClient.cpp
    AudioConsoleApp.cpp
    VoiceMenu.cpp
    PlayMenu.cpp
    CaptureMenu.cpp
    LoopbackMenu.cpp
    ToneMenu.cpp
    TransCodeMenu.cpp
    HpcmMenu.cpp
    RepeatedPlaybackMenu.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleAppCommand.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleApp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/Audio/VoiceSession.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/Audio/AudioSession.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/Audio/AudioHelper.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/console_app_framework
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_AUDIO_CONSOLE_APP} ${AUDIO_CONSOLE_APP_SOURCES})
target_link_libraries(${TARGET_AUDIO_CONSOLE_APP} telux_audio telux_common pthread)

# install to target
install ( TARGETS ${TARGET_AUDIO_CONSOLE_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
