cmake_minimum_required(VERSION 3.10.2)

set(TARGET_NTN_APP ntn_sample_app)

set(SOURCES
    NtnApp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/console_app_framework/ConsoleApp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/console_app_framework/ConsoleAppCommand.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/console_app_framework
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_NTN_APP} ${SOURCES})
target_link_libraries(${TARGET_NTN_APP} telux_satcom)

# install to target
install ( TARGETS ${TARGET_NTN_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
