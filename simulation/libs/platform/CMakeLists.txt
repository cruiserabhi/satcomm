cmake_minimum_required(VERSION 3.10.2)

# pkg-config support
configure_file("telux-platform.pc.in" "telux-platform.pc" @ONLY)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

set(TARGET_PLATFORM_LIBRARY telux_platform)

add_definitions(-DTELUX_TECH_AREA=9)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)

set(PLATFORM_TEST_LIB_SOURCES
    PlatformFactoryStub.cpp
    SubsystemFactoryStub.cpp
    DeviceInfoManagerStub.cpp
    AntennaManagerStub.cpp
    FsManagerStub.cpp
    TimeManagerStub.cpp
    SubsystemManagerStub.cpp
)

find_library(JSONCPP_LIBRARY NAMES jsoncpp)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_library(${TARGET_PLATFORM_LIBRARY} SHARED ${PLATFORM_TEST_LIB_SOURCES})

target_link_libraries(${TARGET_PLATFORM_LIBRARY}
    jsoncpp
    telux_protos
    telux_common
    telux_loc
)

install(TARGETS ${TARGET_PLATFORM_LIBRARY}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-platform.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")