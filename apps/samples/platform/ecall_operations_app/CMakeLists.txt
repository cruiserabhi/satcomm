cmake_minimum_required(VERSION 3.10.2)

set(TARGET_ECALL_APP ecall_operations_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXPLATFORM REQUIRED telux-platform)

set(ECALL_APP_SOURCES EcallOperationsApp.cpp)

add_executable(${TARGET_ECALL_APP} ${ECALL_APP_SOURCES})

target_include_directories(${TARGET_ECALL_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_ECALL_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_ECALL_APP} PRIVATE ${TELUXPLATFORM_LIBRARIES})

install(TARGETS ${TARGET_ECALL_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
