cmake_minimum_required(VERSION 3.10.2)

set(TARGET_LOCATION_TEST_APP location_test_app)

set(LOCATION_TEST_APP_SOURCES
    DgnssMenu.cpp
    LocationMenu.cpp
    MyLocationListener.cpp
    MyLocationCommandCallback.cpp
    LocationUtils.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/utils
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_LOCATION_TEST_APP} ${LOCATION_TEST_APP_SOURCES})
target_link_libraries(${TARGET_LOCATION_TEST_APP} telux_loc telux_common)

# install to target
install ( TARGETS ${TARGET_LOCATION_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
install ( FILES ${CMAKE_CURRENT_LIST_DIR}/config_files/OSNMA_MerkleTree.xml
          DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
          PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )
install ( FILES config_files/rtcmConfig.conf
          DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
          PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )
