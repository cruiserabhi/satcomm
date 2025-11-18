# CMakeList.txt : CMake project for qMonitorTest, include source and define
# project specific logic here.

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

if (NOT DEFINED ENV{JSONC_DIR})
    pkg_check_modules(JSONC_LIB REQUIRED json-c)
    if (JSONC_LIB_FOUND)
        include_directories(${JSONC_LIB_INCLUDE_DIRS})
    endif (JSONC_LIB_FOUND)
else()
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/json-c
        $ENV{JSONC_DIR})
endif()

set(TARGET_QMONITOR qMonitorTest)

set(QMONITOR_SOURCES
    qMonitorTest.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -pthread")

add_executable (${TARGET_QMONITOR} ${QMONITOR_SOURCES})
if (DEFINED ENV{TELUX_STUB_DIR})
    target_link_libraries(${TARGET_QMONITOR}
        qapplication qmessenger v2xcodec
        $ENV{TELUX_STUB_DIR}/libtelux_cv2x.so
        $ENV{TELUX_STUB_DIR}/libtelux_loc.so
        $ENV{TELUX_STUB_DIR}/libtelux_squish.so
        $ENV{TELUX_STUB_DIR}/libtelux_sec.so
        v2x_veh
        json-c rt pthread)
else()
    target_link_libraries(${TARGET_QMONITOR} qapplication qmessenger v2xcodec
        telux_cv2x telux_loc telux_sec telux_squish telux_common
        v2x_veh rt pthread json-c)

endif()

# install to target
install ( TARGETS ${TARGET_QMONITOR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR})

