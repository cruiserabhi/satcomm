cmake_minimum_required(VERSION 3.10.2)

set(TARGET_APP generate_dtmf_tone)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXAUDIO REQUIRED telux-audio)

set(APP_SOURCES GenerateDTMF.cpp)

add_executable(${TARGET_APP} ${APP_SOURCES})

target_include_directories(${TARGET_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_APP} PRIVATE ${TELUXAUDIO_LIBRARIES})

install(TARGETS ${TARGET_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})