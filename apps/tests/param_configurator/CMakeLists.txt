cmake_minimum_required(VERSION 3.10.2)

set(TARGET_PARAM_CONFIGURATOR param_configurator)

set(PARAM_CONFIGURATOR_SOURCES
    ConfiguratorMenu.cpp
    ConfiguratorListener.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleAppCommand.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleApp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/ConfigParser.cpp
)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_PARAM_CONFIGURATOR} ${PARAM_CONFIGURATOR_SOURCES})
target_link_libraries(${TARGET_PARAM_CONFIGURATOR} telux_config telux_common)

# install to target
install ( TARGETS ${TARGET_PARAM_CONFIGURATOR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
