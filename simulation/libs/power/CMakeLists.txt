cmake_minimum_required(VERSION 3.10.2)

configure_file("telux-power.pc.in" "telux-power.pc" @ONLY)

set(TARGET_LIBTELUX_POWER telux_power)

add_definitions(-DTELUX_TECH_AREA=5)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)

# sources
set(LIBTELUX_POWER_SOURCES
   PowerFactoryImpl.cpp
   TcuActivityManagerImpl.cpp
   TcuActivityManagerWrapper.cpp
   PowerGrpcClient.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

add_library (${TARGET_LIBTELUX_POWER} SHARED ${SOURCE} ${LIBTELUX_POWER_SOURCES})

target_link_libraries(${TARGET_LIBTELUX_POWER}
    ${GRPCPP_EXECUTABLE}
    pthread
    telux_protos
    telux_common
    )

install ( TARGETS ${TARGET_LIBTELUX_POWER}
    LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR} )

install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-power.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/" )
