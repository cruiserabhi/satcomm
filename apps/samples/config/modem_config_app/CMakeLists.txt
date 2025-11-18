cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CONFIG_APP modem_config_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXCONFIG REQUIRED telux-config)

set(CONFIG_APP_SOURCES ModemConfigApp.cpp)

add_executable(${TARGET_CONFIG_APP} ${CONFIG_APP_SOURCES})

target_include_directories(${TARGET_CONFIG_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_CONFIG_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_CONFIG_APP} PRIVATE ${TELUXCONFIG_LIBRARIES})

install(TARGETS ${TARGET_CONFIG_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
