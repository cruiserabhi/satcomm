cmake_minimum_required(VERSION 3.10.2)

set(TARGET_ECALL_APP ecall_app)

set(ECALL_APP_SOURCES
    ECallApp.cpp
    EcallOverIms.cpp
    ECallManager.cpp
    ThermClient.cpp
    AudioClient.cpp
    LocationClient.cpp
    TelClient.cpp
    TelClientUtils.cpp
    EcallScanFailHandler.cpp
    MsdProvider.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleAppCommand.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleApp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/ConfigParser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/SignalHandler.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils
    ${CMAKE_CURRENT_LIST_DIR}/../../common
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_ECALL_APP} ${ECALL_APP_SOURCES})
target_link_libraries(${TARGET_ECALL_APP} telux_tel telux_loc telux_therm telux_audio telux_common
                      pthread)

# install to target
install ( TARGETS ${TARGET_ECALL_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
install ( FILES ${CMAKE_CURRENT_LIST_DIR}/config_files/eCall.conf
          DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
          PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )
