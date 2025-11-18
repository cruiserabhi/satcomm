cmake_minimum_required(VERSION 3.10.2)

# pkg-config support
configure_file("telux-satcom.pc.in" "telux-satcom.pc" @ONLY)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

set(TARGET_LOCATION_LIBRARY telux_satcom)

add_definitions(-DTELUX_TECH_AREA=11)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)

set(LOCATION_TEST_LIB_SOURCES
    NtnManagerStub.cpp
    SatcomFactoryStub.cpp
)

find_library(JSONCPP_LIBRARY NAMES jsoncpp)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_library(${TARGET_LOCATION_LIBRARY} SHARED ${LOCATION_TEST_LIB_SOURCES})

target_link_libraries(${TARGET_LOCATION_LIBRARY}
    jsoncpp
    telux_protos
    telux_common
)

install(TARGETS ${TARGET_LOCATION_LIBRARY}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-satcom.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")

