cmake_minimum_required(VERSION 3.10.2)

set(TARGET_NTN_TEST_APP ntn_test_app)

set(SOURCES
    NtnTestApp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/SignalHandler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleAppCommand.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleApp.cpp
)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/console_app_framework
    ${CMAKE_CURRENT_LIST_DIR}/../../common
)

add_executable (${TARGET_NTN_TEST_APP} ${SOURCES})
target_compile_options(${TARGET_NTN_TEST_APP} PRIVATE -std=c++11 -Wall -Werror -g)
target_link_libraries(${TARGET_NTN_TEST_APP} pthread telux_satcom)

# install to target
install ( TARGETS ${TARGET_NTN_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
