cmake_minimum_required(VERSION 3.10.2)

set(TARGET_MONITOR_APP mon_local_mpss)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXMON REQUIRED telux-platform)

set(MONITOR_APP_SOURCES MonitorLocalMPSS.cpp)

add_executable(${TARGET_MONITOR_APP} ${MONITOR_APP_SOURCES})

target_include_directories(${TARGET_MONITOR_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_MONITOR_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_MONITOR_APP} PRIVATE ${TELUXMON_LIBRARIES})

install(TARGETS ${TARGET_MONITOR_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
