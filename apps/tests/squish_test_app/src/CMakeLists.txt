
cmake_minimum_required(VERSION 3.10.2)

project (squish_test_app)
include(FindPkgConfig)
include(GNUInstallDirs)
set(TARGET_SQUISH_TEST_APP sasquish-test-app)
FIND_PACKAGE(PkgConfig REQUIRED)
pkg_check_modules(TELUXSQUISH REQUIRED telux_squish)
pkg_check_modules(TELUXCOMMON REQUIRED telux-common)

set(SQUISH_TEST_APP_SOURCES
    Sasquish.cpp
    SquishClient.cpp
    SquishControlMenu.cpp
    SasquishUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/console_app_framework/ConsoleAppCommand.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/console_app_framework/ConsoleApp.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/console_app_framework
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_SQUISH_TEST_APP} ${SQUISH_TEST_APP_SOURCES})
set (REQUIRED_LIBS ${TELUXSQUISH_LIBRARIES} ${TELUXCOMMON_LIBRARIES} rt pthread)

target_link_libraries(${TARGET_SQUISH_TEST_APP} ${REQUIRED_LIBS})

# install to target
install ( TARGETS ${TARGET_SQUISH_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )

