# libtelux-squish.so:
#    This library contains API of Distributed Congestion Control
#

cmake_minimum_required(VERSION 3.10.2)

include(GNUInstallDirs)
include(FindPkgConfig)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

FIND_PACKAGE(PkgConfig REQUIRED)

add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/src)