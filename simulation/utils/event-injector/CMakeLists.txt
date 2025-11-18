cmake_minimum_required(VERSION 3.10.2)

set(TARGET_EVENT_INJECTOR_APP telsdk_event_injector)

set(TARGET_EVENT_INJECTOR_APP_SRC
    EventInjector.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -pthread")

add_executable (${TARGET_EVENT_INJECTOR_APP} ${TARGET_EVENT_INJECTOR_APP_SRC})
target_link_libraries(${TARGET_EVENT_INJECTOR_APP}
    telux_common
    )

# install to target
install ( TARGETS ${TARGET_EVENT_INJECTOR_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )