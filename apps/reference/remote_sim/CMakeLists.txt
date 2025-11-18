cmake_minimum_required(VERSION 3.10.2)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

# remote-sim-daemon
add_executable(remote-sim-daemon RemoteSimDaemon.cpp RemoteSimUtils.cpp SimConnection.cpp)
target_link_libraries(remote-sim-daemon telux_tel telux_common)
install (TARGETS remote-sim-daemon RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# sap-card-provider
add_executable(sap-card-provider SapCardProvider.cpp RemoteSimUtils.cpp SimConnection.cpp
                                 CardControl.cpp)
target_link_libraries(sap-card-provider telux_tel telux_common)
install (TARGETS sap-card-provider RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
