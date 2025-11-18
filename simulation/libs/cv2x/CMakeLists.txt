cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CV2X_LIBRARY telux_cv2x)

configure_file("telux-cv2x.pc.in" "telux-cv2x.pc" @ONLY)
configure_file("v2x-radio.pc.in" "v2x-radio.pc" @ONLY)

set(CV2X_STUB_SOURCES
    Cv2xFactoryStub.cpp
    Cv2xRadioManagerStub.cpp
    Cv2xRxSubscriptionStub.cpp
    Cv2xTxFlowStub.cpp
    Cv2xRadioStub.cpp
    Cv2xThrottleManagerStub.cpp
    Cv2xConfigStub.cpp
    Cv2xTxRxSocketStub.cpp
    Cv2xUtil.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_library (${TARGET_CV2X_LIBRARY} SHARED ${SOURCE} ${CV2X_STUB_SOURCES})

target_link_libraries(${TARGET_CV2X_LIBRARY}
    telux_protos
    telux_common
    )

# target libv2x_radio
#
set(TARGET_LIBV2X_RADIO v2x_radio)

include_directories(BEFORE
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(LIBV2X_RADIO_SOURCES
        legacy/radio_telux.cpp
        legacy/v2x_config.cpp
        legacy/v2x_packet.cpp
        legacy/log.c)

# build library
add_library(${TARGET_LIBV2X_RADIO} SHARED
    ${LIBV2X_RADIO_SOURCES})

target_link_libraries(${TARGET_LIBV2X_RADIO} ${TARGET_CV2X_LIBRARY})

install ( TARGETS ${TARGET_CV2X_LIBRARY} ${TARGET_LIBV2X_RADIO}
    LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR} )

install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-cv2x.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/" )
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/v2x-radio.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/" )

