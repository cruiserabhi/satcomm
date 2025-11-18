cmake_minimum_required(VERSION 3.10.2)

set(TARGET_OTA_APP ota_operations_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXPLATFORM REQUIRED telux-platform)

set(OTA_APP_SOURCES OtaOperationsApp.cpp)

add_executable(${TARGET_OTA_APP} ${OTA_APP_SOURCES})

target_include_directories(${TARGET_OTA_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_OTA_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_OTA_APP} PRIVATE ${TELUXPLATFORM_LIBRARIES})

install(TARGETS ${TARGET_OTA_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
