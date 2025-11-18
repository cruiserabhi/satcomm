cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SIMULATION_SERVER_APP_SRC
    sensor/SensorFeatureManagerServerImpl.cpp
    sensor/SensorClientServerImpl.cpp
    sensor/SensorReportService.cpp
)

target_sources (${TARGET_SIMULATION_SERVER_APP} PRIVATE ${TARGET_SIMULATION_SERVER_APP_SRC})