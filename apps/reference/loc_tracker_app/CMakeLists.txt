cmake_minimum_required(VERSION 3.10.2)

set(TARGET_LOC_TRACKER_APP loc_tracker_app)

set(LOC_TRACKER_APP_SOURCES
    LocationTrackerApp.cpp
    MyLocationListener.cpp
    MySmsListener.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_LOC_TRACKER_APP} ${LOC_TRACKER_APP_SOURCES})
target_link_libraries(${TARGET_LOC_TRACKER_APP} telux_loc telux_tel)

# install to target
install ( TARGETS ${TARGET_LOC_TRACKER_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
