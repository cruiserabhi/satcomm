cmake_minimum_required(VERSION 3.10.2)

set(TARGET_DATA_LIBRARY telux_data)

configure_file("telux-data.pc.in" "telux-data.pc" @ONLY)

set(DATA_STUB_SOURCES
    DataFactoryImplStub.cpp
    DataProfileStub.cpp
    DataProfileManagerStub.cpp
    DataCallStub.cpp
    DataEventListener.cpp
    DataConnectionManagerStub.cpp
    ServingSystemManagerStub.cpp
    DataSettingsManagerStub.cpp
    DataFilterManagerStub.cpp
    DataHelper.cpp
    IpFilterImpl.cpp
    DualDataManagerStub.cpp
    DataControlManagerStub.cpp
    DataLinkManagerStub.cpp
    TrafficFilterImpl.cpp
    KeepAliveManagerStub.cpp
    net/SocksManagerStub.cpp
    net/NatManagerStub.cpp
    net/L2tpManagerStub.cpp
    net/FirewallManagerStub.cpp
    net/FirewallEntryImpl.cpp
    net/BridgeManagerStub.cpp
    net/VlanManagerStub.cpp
    net/QoSManagerStub.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_library (${TARGET_DATA_LIBRARY} SHARED ${SOURCE} ${DATA_STUB_SOURCES})

target_link_libraries(${TARGET_DATA_LIBRARY}
    ${GRPCPP_EXECUTABLE}
    telux_protos
    telux_common
    )

install ( TARGETS ${TARGET_DATA_LIBRARY}
    LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR} )

install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-data.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/" )
