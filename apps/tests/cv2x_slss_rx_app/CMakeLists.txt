cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CV2X_SLSS_RX_APP cv2x_slss_rx_app)

set(CV2X_SLSS_RX_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    Cv2xSlssRxTest.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_compile_options(${GLIB_CFLAGS})

add_executable (${TARGET_CV2X_SLSS_RX_APP} ${CV2X_SLSS_RX_SOURCES})
target_link_libraries(${TARGET_CV2X_SLSS_RX_APP} ${GLIB_LIBRARIES} telux_cv2x pthread)

# install to target
install ( TARGETS ${TARGET_CV2X_SLSS_RX_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
