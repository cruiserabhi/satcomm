cmake_minimum_required(VERSION 3.10.2)

set(TARGET_DATA_CALL_APP data_call_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXDATA REQUIRED telux-data)

set(DATA_CALL_APP_SOURCES DataCallApp.cpp)

add_executable (${TARGET_DATA_CALL_APP} ${DATA_CALL_APP_SOURCES})

target_include_directories(${TARGET_DATA_CALL_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# set global variables
target_compile_options(${TARGET_DATA_CALL_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_DATA_CALL_APP} PRIVATE ${TELUXDATA_LIBRARIES})

# install to target
install ( TARGETS ${TARGET_DATA_CALL_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
