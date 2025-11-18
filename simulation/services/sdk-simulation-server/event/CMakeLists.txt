cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SIMULATION_SERVER_APP_SRC
    event/EventService.cpp
    event/ServerEventManager.cpp
)

target_sources (${TARGET_SIMULATION_SERVER_APP} PRIVATE ${TARGET_SIMULATION_SERVER_APP_SRC})