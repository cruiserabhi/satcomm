set(CMAKE_VERBOSE_MAKEFILE ON)

set(LIBQMESSENGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/./KinematicsReceive/KinematicsReceive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./RadioInterface/RadioInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./RadioReceive/RadioReceive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./RadioTransmit/RadioTransmit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./ThrottleManager/ThrottleManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./VehicleReceive/VehicleReceive.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/RadioInterface
)

if(DEFINED ENV{ASN1C_PATH} OR DEFINED ASN1C_PATH)
    set(LIBQMESSENGER_SOURCES ${LIBQMESSENGER_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/./GeoNetRouter/GeoNetRouterImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./GeoNetRouter/LocationTable.cpp
)
endif()
add_library(qmessenger ${LIBQMESSENGER_SOURCES})
target_link_libraries(qmessenger v2x_radio)
install(TARGETS qmessenger
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
