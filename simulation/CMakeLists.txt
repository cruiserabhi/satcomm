cmake_minimum_required(VERSION 3.10.2)

project (telsdk-simulation)

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

configure_file("${CMAKE_SOURCE_DIR}/FileInfo.hpp.in"
              "${CMAKE_SOURCE_DIR}/FileInfo.hpp" @ONLY)

include_directories(BEFORE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/common/
    ${CMAKE_SOURCE_DIR}/protos
    ${CMAKE_SOURCE_DIR}/services
    ${CMAKE_SOURCE_DIR}/utils
)

add_definitions(-DDEFAULT_SIM_CONFIG_FILE_NAME="tel.conf")
add_definitions(-DDEFAULT_SIM_CONFIG_FILE_PATH="/etc/telux/")
add_definitions(-DDEFAULT_SIM_CSV_FILE_PATH="/data/telux/csv/")
add_definitions(-DDEFAULT_JSON_FILE_PATH="/data/telux/json/")

add_subdirectory( protos )
add_subdirectory( utils/event-injector )
add_subdirectory( services/sdk-simulation-server )
add_subdirectory( libs )
add_subdirectory( conf )
add_subdirectory( json )

file (GLOB SCRIPT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.sh")

foreach (SCRIPT ${SCRIPT_FILES})
install(FILES "${SCRIPT}"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/")
endforeach()


