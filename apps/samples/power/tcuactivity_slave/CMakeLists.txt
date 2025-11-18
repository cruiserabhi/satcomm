cmake_minimum_required(VERSION 3.10.2)

set(TARGET_POWER_APP tcuactivity_slave)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXPOWER REQUIRED telux-power)

set(POWER_APP_SOURCES TcuActivitySlave.cpp)

add_executable(${TARGET_POWER_APP} ${POWER_APP_SOURCES})

target_include_directories(${TARGET_POWER_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_POWER_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_POWER_APP} PRIVATE ${TELUXPOWER_LIBRARIES})

install(TARGETS ${TARGET_POWER_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
