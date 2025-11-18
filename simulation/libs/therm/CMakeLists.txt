cmake_minimum_required(VERSION 3.10.2)

# pkg-config support
configure_file("telux-therm.pc.in" "telux-therm.pc" @ONLY)

set(TARGET_THERM_LIBRARY telux_therm)

add_definitions(-DTELUX_TECH_AREA=7)

set(THERM_SIM_SOURCES
    ThermalFactoryImplStub.cpp
    ThermalManagerImplStub.cpp
    ThermalShutdownManagerImplStub.cpp
)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_library(${TARGET_THERM_LIBRARY} SHARED ${THERM_SIM_SOURCES})

target_link_libraries(${TARGET_THERM_LIBRARY}
   ${GRPCPP_EXECUTABLE}
   telux_protos
   telux_common
)

install(TARGETS ${TARGET_THERM_LIBRARY}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-therm.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
