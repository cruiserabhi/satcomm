cmake_minimum_required(VERSION 3.10.2)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)
set(GRPCPP_EXECUTABLE gRPC::grpc++)

FILE(GLOB SOURCE "${PROTO_SRC_DIR}/*.cc")

find_package (Git)

if (GIT_FOUND)
    # By excuting release-name-info.sh, Fetching the telsdk tag and store in SDKTAG
    # If tag does not exist, then we update the last commit tag and short commit ID of the tip separated with "-"
    execute_process(COMMAND /bin/bash -c "${CMAKE_SOURCE_DIR}/libs/release-name-info.sh ${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE RESULT OUTPUT_VARIABLE SDKTAG)
    # remove \n
    string(STRIP "${SDKTAG}" SDKTAG)
    message("Telsdk tag - ${SDKTAG} ")
endif (GIT_FOUND)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

# set release name
set(RELEASE_NAME ${SDKTAG} CACHE INTERNAL "" FORCE)

# set sdk version
file(READ "${CMAKE_SOURCE_DIR}/libs/VERSION" VERSION_NUM)
string(STRIP "${VERSION_NUM}" VERSION_NUM)

if(VERSION_NUM)
    string(REGEX REPLACE "^.*_*_v" "" SDK_VERSION "${VERSION_NUM}")
    string(STRIP "${SDK_VERSION}" SDK_VERSION)
    set(SDK_VERSION ${SDK_VERSION} CACHE INTERNAL "" FORCE)
endif(VERSION_NUM)

configure_file("${CMAKE_SOURCE_DIR}/libs/common/VersionInfo.hpp.in"
              "${CMAKE_SOURCE_DIR}/libs/common/VersionInfo.hpp" @ONLY)

add_subdirectory( common )
add_subdirectory( data )
add_subdirectory( loc )
add_subdirectory( tel )
add_subdirectory( sensor )
add_subdirectory( audio )
add_subdirectory( therm )
add_subdirectory( power )
add_subdirectory( cv2x )
add_subdirectory( platform )
add_subdirectory( satcom )
