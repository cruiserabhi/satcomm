cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CV2X_GET_STATUS_APP cv2x_get_status)

set(CV2X_GET_STATUS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/utils/Utils.cpp
    Cv2xGetStatusApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_CV2X_GET_STATUS_APP} ${CV2X_GET_STATUS_SOURCES})
target_link_libraries(${TARGET_CV2X_GET_STATUS_APP} telux_cv2x)

# install to target
install ( TARGETS ${TARGET_CV2X_GET_STATUS_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
