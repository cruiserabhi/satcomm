cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CV2X_MAC_CLONE_ATTACK_APP cv2x_mac_clone_attack_app)

set(CV2X_MAC_CLONE_ATTACK_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/SignalHandler.cpp
    Cv2xMacCloneAttackTest.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_compile_options(${GLIB_CFLAGS})

add_executable (${TARGET_CV2X_MAC_CLONE_ATTACK_APP} ${CV2X_MAC_CLONE_ATTACK_SOURCES})
target_link_libraries(${TARGET_CV2X_MAC_CLONE_ATTACK_APP} ${GLIB_LIBRARIES} telux_cv2x pthread)

# install to target
install ( TARGETS ${TARGET_CV2X_MAC_CLONE_ATTACK_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
