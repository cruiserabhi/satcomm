cmake_minimum_required(VERSION 3.10.2)

set(TARGET_RSP_APP rsp_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXTEL REQUIRED telux-tel)

set(RSP_APP_SOURCES
   RspApp.cpp
   RspListener.cpp
   ${CMAKE_CURRENT_LIST_DIR}/../../../common/utils/Utils.cpp
)

add_executable(${TARGET_RSP_APP} ${RSP_APP_SOURCES})

target_include_directories(${TARGET_RSP_APP} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/utils)

target_compile_options(${TARGET_RSP_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_RSP_APP} PRIVATE ${TELUXTEL_LIBRARIES} telux_common)

install(TARGETS ${TARGET_RSP_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
