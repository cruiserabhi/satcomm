# CMakeList.txt : CMake project for qMonitorTest, include source and define
# project specific logic here.

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

set(TARGET_QIMC qimc)

if (NOT DEFINED ENV{JSONC_DIR})
    pkg_check_modules(JSONC_LIB REQUIRED json-c)
    if (JSONC_LIB_FOUND)
        include_directories(${JSONC_LIB_INCLUDE_DIRS})
    endif (JSONC_LIB_FOUND)
else()
    message("JSONC_DIR defined in ENV")
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/json-c
        $ENV{JSONC_DIR})
endif()

set(QIMC_SOURCES
    qimc.cpp
    qimcTest.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -pthread")

add_executable (${TARGET_QIMC} ${QIMC_SOURCES})
target_link_libraries(${TARGET_QIMC} json-c qapplication)


# install to target
install (TARGETS ${TARGET_QIMC}
         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
         ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR})

