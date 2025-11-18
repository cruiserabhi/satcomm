cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CV2X_L2_ID_FILTER cv2x_l2_id_filter)

set(CV2X_L2_ID_FILTER_SRC
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    Cv2xL2IdFilterApp.cpp)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_CV2X_L2_ID_FILTER} ${CV2X_L2_ID_FILTER_SRC})
target_link_libraries(${TARGET_CV2X_L2_ID_FILTER} telux_cv2x)

# install to target
install ( TARGETS ${TARGET_CV2X_L2_ID_FILTER}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
