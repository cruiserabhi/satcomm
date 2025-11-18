cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SS_APP serving_system_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXTEL REQUIRED telux-tel)

set(SS_APP_SOURCES ServingSystemApp.cpp)

add_executable(${TARGET_SS_APP} ${SS_APP_SOURCES})

target_include_directories(${TARGET_SS_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_SS_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_SS_APP} PRIVATE ${TELUXTEL_LIBRARIES})

install(TARGETS ${TARGET_SS_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
