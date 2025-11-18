# CMakeList.txt : CMake project for RadioInterface, include source and define
# project specific logic here.

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

set(TARGET_QITS qits)

include_directories(BEFORE
    ${CMAKE_CURRENT_LIST_DIR}/../../../../common/utils/
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qApplication/Application/
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qMessenger/KinematicsReceive/
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qMessenger/RadioReceive
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qMessenger/RadioInterface
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qMessenger/RadioTransmit
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qMessenger/VehicleReceive
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qApplication/Ldm
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qCoder/include
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qMessenger/ThrottleManager
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qApplication/SafetyApps
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qApplication/Monitor
    ${CMAKE_CURRENT_LIST_DIR}/../../src/qApplication/Utils
)
set(QITS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../../common/utils/SignalHandler.cpp
    ApplicationTest.cpp
)


# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -pthread")

add_executable (${TARGET_QITS} ${QITS_SOURCES})
if (DEFINED ENV{TELUX_STUB_DIR})
    if (DEFINED ENV{SQUISH_DIR})
        #use local bulid squish lib
        set(SQUISH_LIB "telux_squish")
    else()
        #use squish stub lib.
        set(SQUISH_LIB "$ENV{TELUX_STUB_DIR}/libtelux_squish.so")
    endif()
    target_link_libraries(${TARGET_QITS}
        qapplication qmessenger v2xcodec
        $ENV{TELUX_STUB_DIR}/libtelux_cv2x.so
        $ENV{TELUX_STUB_DIR}/libtelux_common.so
        $ENV{TELUX_STUB_DIR}/libtelux_loc.so
        ${SQUISH_LIB}
        $ENV{TELUX_STUB_DIR}/libtelux_sec.so
        v2x_veh
        json-c rt pthread)
else()
    target_link_libraries(${TARGET_QITS} qapplication qmessenger v2xcodec
        telux_cv2x telux_loc telux_sec telux_squish telux_common
        v2x_veh rt pthread json-c)

endif()

if(DEFINED ENV{AEROLINK_PATH} OR DEFINED WITH_AEROLINK)
    target_link_libraries(${TARGET_QITS} viicsec -Wl,--allow-shlib-undefined)
endif()

# install to target
install ( TARGETS ${TARGET_QITS}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../ObeConfig.conf
    DESTINATION ${CMAKE_INSTALL_PREFIX}/../etc)
