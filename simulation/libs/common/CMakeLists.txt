# pkg-config support
configure_file("telux-common.pc.in" "telux-common.pc" @ONLY)

cmake_minimum_required(VERSION 3.10.2)

set(TARGET_COMMON_LIBRARY telux_common)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# Searching for the dependent libraries.
find_library(JSONCPP_LIBRARY NAMES jsoncpp)

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

set (COMMON_STUB_SOURCES
    Version.cpp
    AsyncTaskQueue.hpp
    DeviceConfig.cpp
    Logger.cpp
    event-manager/EventParserUtil.cpp
    event-manager/ClientEventManager.cpp
    JsonParser.cpp
    SimulationConfigParser.cpp
    CsvHandler.cpp
    CommonUtils.cpp
    Utils.cpp
    BaseState.cpp
    BaseStateMachine.cpp
    CommandCallbackManager.cpp
    TaskQueue.cpp
    TaskThread.cpp
    TaskDispatcher.cpp
    EnvUtils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/therm/ThermalZoneImpl.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/therm/CoolingDeviceImpl.cpp
)

add_library (${TARGET_COMMON_LIBRARY} SHARED  ${COMMON_STUB_SOURCES})

target_link_libraries(${TARGET_COMMON_LIBRARY}
    jsoncpp
    telux_protos
    )

install ( TARGETS ${TARGET_COMMON_LIBRARY}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-common.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/" )
