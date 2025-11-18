cmake_minimum_required(VERSION 3.10.2)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_UTC_INFO_LISTENER_APP utc_info_listener_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXPLATFORM REQUIRED telux-platform)
pkg_check_modules(TELUXLOC REQUIRED telux-loc)

set(UTC_INFO_LISTENER_APP_SOURCES ${CMAKE_CURRENT_LIST_DIR}/../../../common/utils/SignalHandler.cpp
UTCInfoListener.cpp)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/utils/
)

add_executable(${TARGET_UTC_INFO_LISTENER_APP} ${UTC_INFO_LISTENER_APP_SOURCES})

target_include_directories(${TARGET_UTC_INFO_LISTENER_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_UTC_INFO_LISTENER_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_UTC_INFO_LISTENER_APP} PRIVATE ${TELUXPLATFORM_LIBRARIES} ${TELUXLOC_LIBRARIES} pthread)

install(TARGETS ${TARGET_UTC_INFO_LISTENER_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})