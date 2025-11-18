cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CV2X_INJECT_SPEED cv2x_inject_speed)

set(CV2X_INJECT_SPEED_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    Cv2xInjectSpeed.cpp)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_CV2X_INJECT_SPEED} ${CV2X_INJECT_SPEED_SOURCES})
target_link_libraries(${TARGET_CV2X_INJECT_SPEED} telux_cv2x)

# install to target
install ( TARGETS ${TARGET_CV2X_INJECT_SPEED}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
