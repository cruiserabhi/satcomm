cmake_minimum_required(VERSION 3.10.2)

# pkg-config support
configure_file("telux-sensor.pc.in" "telux-sensor.pc" @ONLY)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

set(TARGET_SENSOR_LIBRARY telux_sensor)

add_definitions(-DTELUX_TECH_AREA=8)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)

set(SENSOR_TEST_LIB_SOURCES
    SensorFactoryStub.cpp
    SensorFeatureManagerStub.cpp
    SensorManagerStub.cpp
    SensorClientStub.cpp
    SensorReportListener.cpp
)

find_library(JSONCPP_LIBRARY NAMES jsoncpp)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_library(${TARGET_SENSOR_LIBRARY} SHARED ${SENSOR_TEST_LIB_SOURCES})

target_link_libraries(${TARGET_SENSOR_LIBRARY}
    jsoncpp
    telux_protos
    telux_common
    telux_power
)

install(TARGETS ${TARGET_SENSOR_LIBRARY}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-sensor.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/PRE-RECORDED_SENSOR_BUFFER_DATA.csv"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/data/telux/csv/")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/PRE-RECORDED_SENSOR_DATA.csv"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/data/telux/csv/")