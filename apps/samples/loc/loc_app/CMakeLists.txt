cmake_minimum_required(VERSION 3.10.2)

set(TARGET_LOC_APP loc_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXLOC REQUIRED telux-loc)

set(LOC_APP_SOURCES SampleLocationApp.cpp)

add_executable(${TARGET_LOC_APP} ${LOC_APP_SOURCES})

target_include_directories(${TARGET_LOC_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_LOC_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_LOC_APP} PRIVATE ${TELUXLOC_LIBRARIES})

install(TARGETS ${TARGET_LOC_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
