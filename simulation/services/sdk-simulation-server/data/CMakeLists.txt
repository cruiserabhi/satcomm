cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SIMULATION_SERVER_APP_SRC
    data/DataConnectionServerImpl.cpp
    data/DataProfileServerImpl.cpp
    data/DataSettingsServerImpl.cpp
    data/ServingSystemServerImpl.cpp
    data/DataFilterServerImpl.cpp
    data/DualDataServerImpl.cpp
    data/DataControlServerImpl.cpp
    data/DataLinkServerImpl.cpp
    data/net/SocksServerImpl.cpp
    data/net/NatServerImpl.cpp
    data/net/L2tpServerImpl.cpp
    data/net/FirewallServerImpl.cpp
    data/net/BridgeServerImpl.cpp
    data/net/VlanServerImpl.cpp
)

target_sources (${TARGET_SIMULATION_SERVER_APP} PRIVATE ${TARGET_SIMULATION_SERVER_APP_SRC})
