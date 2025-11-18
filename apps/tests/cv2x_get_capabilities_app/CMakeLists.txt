cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CV2X_GET_CAPABILITIES_APP cv2x_get_capabilities)

set(CV2X_GET_CAPABILITIES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}../../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}../../../common/utils/SignalHandler.cpp
    Cv2xGetCapabilitiesApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_CV2X_GET_CAPABILITIES_APP} ${CV2X_GET_CAPABILITIES_SOURCES})
target_link_libraries(${TARGET_CV2X_GET_CAPABILITIES_APP} telux_cv2x pthread)

# install to target
install ( TARGETS ${TARGET_CV2X_GET_CAPABILITIES_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
