cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SIMULATION_SERVER_APP_SRC
    loc/LocationManagerServerImpl.cpp
    loc/LocationConfiguratorServerImpl.cpp
    loc/LocationReportService.cpp
)

target_sources (${TARGET_SIMULATION_SERVER_APP} PRIVATE ${TARGET_SIMULATION_SERVER_APP_SRC})