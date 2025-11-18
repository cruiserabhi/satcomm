cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CV2X_SET_TX_PWR cv2x_set_tx_power)

set(CV2X_SET_TX_PWR_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    Cv2xSetTxPowerApp.cpp)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_CV2X_SET_TX_PWR} ${CV2X_SET_TX_PWR_SOURCES})
target_link_libraries(${TARGET_CV2X_SET_TX_PWR} telux_cv2x)

# install to target
install ( TARGETS ${TARGET_CV2X_SET_TX_PWR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
