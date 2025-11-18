cmake_minimum_required(VERSION 3.10.2)

project(qits)
include(GNUInstallDirs)
include(FindPkgConfig)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

message( ${CMAKE_SYSTEM_PROCESSOR} )
message(${CMAKE_HOST_SYSTEM_PROCESSOR})
if (WITH_AEROLINK)
    add_definitions( -DAEROLINK)
elseif(DEFINED ENV{AEROLINK_PATH})
    add_definitions( -DAEROLINK)
    add_definitions(-I$ENV{AEROLINK_PATH}/include/)
    message("Building with security")
endif()

# add "-DEMIT_ASN_DEBUG -DASN_THREAD_SAFE" to debug asn1 decoder
if(ASN1C_PATH)
    add_definitions(-DETSI -DWITH_WSA -D_DEFAULT_SOURCE)
    set(ASN1C_SKELETONS ${ASN1C_PATH}/skeletons)
elseif(DEFINED ENV{ASN1C_PATH})
    add_definitions(-DETSI -D_DEFAULT_SOURCE)
    set(ASN1C_SKELETONS $ENV{ASN1C_PATH}/skeletons)
endif()

find_path(GLIB_INCLUDE_DIR NAMES glibconfig.h glib.h PATH_SUFFIXES glib-2.0)

include_directories(BEFORE
    ${PROJECT_SOURCE_DIR}/include
    #Including this libraries given a bug with glib
    ${GLIB_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/src/qCoder/include
    ${PROJECT_SOURCE_DIR}/src/qMessenger/RadioInterface
    ${PROJECT_SOURCE_DIR}/src/qMessenger/RadioTransmit
    ${PROJECT_SOURCE_DIR}/src/qMessenger/RadioReceive
    ${PROJECT_SOURCE_DIR}/src/qMessenger/KinematicsReceive
    ${PROJECT_SOURCE_DIR}/src/qMessenger/VehicleReceive
    ${PROJECT_SOURCE_DIR}/src/qApplication/Application
    ${PROJECT_SOURCE_DIR}/src/qApplication/Ldm
    ${PROJECT_SOURCE_DIR}/src/qApplication/SafetyApps
    ${PROJECT_SOURCE_DIR}/src/qMessenger/ThrottleManager
    ${PROJECT_SOURCE_DIR}/src/qApplication/Monitor
    ${PROJECT_SOURCE_DIR}/src/qApplication/Utils
)
if(DEFINED ENV{ASN1C_PATH} OR DEFINED ASN1C_PATH)
    include_directories(${ASN1C_SKELETONS}
    ${PROJECT_SOURCE_DIR}/src/qCoder/src/etsi/CAM/generated
    ${PROJECT_SOURCE_DIR}/src/qCoder/src/etsi/DENM/generated
    ${PROJECT_SOURCE_DIR}/src/qCoder/src/etsi/dictionary/generated
    ${PROJECT_SOURCE_DIR}/src/qCoder/src/sae/generated
    ${PROJECT_SOURCE_DIR}/src/qMessenger/GeoNetRouter)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSIM_BUILD")
    if (NOT DEFINED ENV{TELUX_STUB_DIR})
        message(SEND_ERROR " please specify telux sub library path via ENV TELUX_STUB_DIR")
    endif()

    if (NOT DEFINED ENV{NANOPB_DIR})
        message(SEND_ERROR "  Please specify nanopb path via environment variable NANOPB_DIR")
    else()
        set(NANOPB_DIR "$ENV{NANOPB_DIR}")
        #build nanopb
        include_directories(BEFORE ${NANOPB_DIR})
        file(GLOB NANOPB_SOURCE_FILES ${NANOPB_DIR}/*.c)
        add_library(protobuf-nanopb STATIC ${NANOPB_SOURCE_FILES})
        target_compile_options(protobuf-nanopb PUBLIC "-fPIC")
    endif()

    include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
        ${NANOPB_DIR}
        )
    if (NOT DEFINED ENV{JSONC_DIR})
        message(SEND_ERROR " Please specify json-c library path via ENV JSONC_DIR")
    else()
        #build out of tree json-c lib
        include_directories(${CMAKE_CURRENT_BINARY_DIR}/json-c
            $ENV{JSONC_DIR})
        add_subdirectory($ENV{JSONC_DIR} json-c)
    endif()

    if (NOT DEFINED ENV{CV2X_VEH_LIB_DIR})
        message(SEND_ERROR "please specify cv2x vehicle librar path via ENV CV2X_VEH_LIB_DIR")
    else()
        #build vehicle sim lib locally
        include_directories(BEFORE $ENV{CV2X_VEH_LIB_DIR}/../inc)
        file(GLOB CV2X_VEH_LIB_SOURCE_FILES $ENV{CV2X_VEH_LIB_DIR}/veh_sim.c)
        add_library(v2x_veh STATIC ${CV2X_VEH_LIB_SOURCE_FILES})
        target_compile_options(v2x_veh PUBLIC "-fPIC")
    endif()
    if (DEFINED ENV{SQUISH_DIR})
        #build squish locally
        include_directories(BEFORE
            $ENV{SQUISH_DIR}/include
            $ENV{SQUISH_DIR}/src
            )
        add_definitions(-DTELUX_TECH_AREA=2)
        file(GLOB LIBSQUISH_SOURCES $ENV{SQUISH_DIR}/src/*.cpp
            $ENV{SQUISH_DIR}/include/*.hpp)
        add_library(telux_squish STATIC ${LIBSQUISH_SOURCES})
    endif()
else()
    pkg_check_modules(JSONC_LIB REQUIRED json-c)
    if (JSONC_LIB_FOUND)
        include_directories(${JSONC_LIB_INCLUDE_DIRS})
    endif (JSONC_LIB_FOUND)
endif()

add_subdirectory(src/qCoder/)
add_subdirectory(src/qMessenger/)
add_subdirectory(src/qApplication)
add_subdirectory(tests/)

if(WITH_SYSTEMD)
    install(FILES scripts/qits@.service
            DESTINATION ${CMAKE_INSTALL_PREFIX}/../lib/systemd/system)

    install(FILES scripts/qits_env_file
            DESTINATION ${CMAKE_INSTALL_PREFIX}/../etc/
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )
endif()
