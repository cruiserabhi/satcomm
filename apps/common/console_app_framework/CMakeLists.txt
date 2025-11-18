# telux_app_framework:
#    Shared Library that encapsulates basic console app framework.

cmake_minimum_required(VERSION 3.10.2)
# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)

set(TARGET_CONSOLE_APP_FRAMEWORK telux_app_framework)

set(CONSOLE_APP_FRAMEWORK_SOURCES
    ConsoleAppCommand.cpp
    ConsoleApp.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

# headers
set(CONSOLEAPP_FRAMEWORK_HEADERS
    ConsoleApp.hpp
    ConsoleAppCommand.hpp
)

add_library(${TARGET_CONSOLE_APP_FRAMEWORK} STATIC
    ${CONSOLE_APP_FRAMEWORK_SOURCES})

set_target_properties(${TARGET_CONSOLE_APP_FRAMEWORK} PROPERTIES
    PRIVATE_HEADER "${CONSOLEAPP_FRAMEWORK_HEADERS}")

# install to target
install ( TARGETS ${TARGET_CONSOLE_APP_FRAMEWORK}
        PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/telux/common
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )

