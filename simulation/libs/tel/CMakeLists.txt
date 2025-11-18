cmake_minimum_required(VERSION 3.10.2)

# pkg-config support
configure_file("telux-tel.pc.in" "telux-tel.pc" @ONLY)

set(TARGET_TEL_LIBRARY telux_tel)

add_definitions(-DTELUX_TECH_AREA=6)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)

set(TEL_STUB_SOURCES
    PhoneFactoryImplStub.cpp
    CardManagerStub.cpp
    CardStub.cpp
    CardAppStub.cpp
    CardFileHandlerStub.cpp
    ECallMsd.cpp
    SubscriptionManagerStub.cpp
    SubscriptionStub.cpp
    SmsManagerStub.cpp
    SignalStrength.cpp
    CellInfo.cpp
    VoiceServiceInfo.cpp
    CellIBroadcastMessage.cpp
    SimProfile.cpp
    PhoneManagerStub.cpp
    PhoneStub.cpp
    CellBroadcastManagerStub.cpp
    CallManagerStub.cpp
    CallStub.cpp
    MultiSimManagerStub.cpp
    ImsServingSystemManagerStub.cpp
    ServingSystemManagerStub.cpp
    NetworkSelectionManagerStub.cpp
    ImsSettingsManagerStub.cpp
    SuppServicesManagerStub.cpp
)

find_library(JSONCPP_LIBRARY NAMES jsoncpp)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_library (${TARGET_TEL_LIBRARY} SHARED ${SOURCE} ${TEL_STUB_SOURCES})

target_link_libraries(${TARGET_TEL_LIBRARY}
    jsoncpp
    telux_protos
    telux_common
    )

install ( TARGETS ${TARGET_TEL_LIBRARY}
    LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR} )
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-tel.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/" )
