cmake_minimum_required(VERSION 3.10.2)

set(TARGET_DATA_QOS_APP data_qos_test_app)

set(SOURCES
    DataQosTestApp.cpp
    DataUtils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
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
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_DATA_QOS_APP} ${SOURCES})
target_link_libraries(${TARGET_DATA_QOS_APP} telux_data)

# install to target
install ( TARGETS ${TARGET_DATA_QOS_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
