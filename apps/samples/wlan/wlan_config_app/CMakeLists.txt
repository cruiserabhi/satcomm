cmake_minimum_required(VERSION 3.10.2)

set(TARGET_WLAN_APP wlan_config_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXWLAN REQUIRED telux-wlan)

set(WLAN_CONF_APP_SOURCES WlanConfigApp.cpp)

add_executable(${TARGET_WLAN_APP} ${WLAN_CONF_APP_SOURCES})

target_include_directories(${TARGET_WLAN_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_WLAN_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_WLAN_APP} PRIVATE ${TELUXWLAN_LIBRARIES})

install(TARGETS ${TARGET_WLAN_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
