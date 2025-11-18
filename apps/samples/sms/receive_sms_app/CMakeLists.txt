cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SMS_APP receive_sms_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXTEL REQUIRED telux-tel)

set(SMS_APP_SOURCES SimpleSmsListener.cpp)

add_executable(${TARGET_SMS_APP} ${SMS_APP_SOURCES})

target_include_directories(${TARGET_SMS_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_SMS_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_SMS_APP} PRIVATE ${TELUXTEL_LIBRARIES})

install(TARGETS ${TARGET_SMS_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
