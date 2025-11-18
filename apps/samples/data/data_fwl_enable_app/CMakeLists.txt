cmake_minimum_required(VERSION 3.10.2)

set(TARGET_DATA_APP fwl_enable_sample_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXDATA REQUIRED telux-data)

set(DATA_APP_SOURCES DataFwlEnblApp.cpp)

add_executable(${TARGET_DATA_APP} ${DATA_APP_SOURCES})

target_include_directories(${TARGET_DATA_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_DATA_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_DATA_APP} PRIVATE ${TELUXDATA_LIBRARIES})

install(TARGETS ${TARGET_DATA_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
