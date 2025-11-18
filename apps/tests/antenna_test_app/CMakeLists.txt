cmake_minimum_required(VERSION 3.10.2)

set(TARGET_ANTENNA_TEST_APP antenna_test_app)

set(ANTENNA_TEST_APP
    AntennaTestApp.cpp
    MyAntennaListener.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleAppCommand.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleApp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
)

add_definitions(-DAPP_NAME=\"\\033[1;32mantenna_test_app\\033[0m\")

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/console_app_framework
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/utils
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_ANTENNA_TEST_APP} ${ANTENNA_TEST_APP})
target_link_libraries(${TARGET_ANTENNA_TEST_APP} telux_platform)

# install to target
install ( TARGETS ${TARGET_ANTENNA_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
