cmake_minimum_required(VERSION 3.10.2)

set(TARGET_TELUX_VERSION_APP telux_version_app)

find_package(PkgConfig REQUIRED)

pkg_check_modules(TELUXPLATFORM REQUIRED telux-platform)
add_compile_options(${TELUXPLATFORM_CFLAGS})
set(TELUX_VERSION_APP_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    TeluxVersionApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_TELUX_VERSION_APP} ${TELUX_VERSION_APP_SOURCES})
target_link_libraries(${TARGET_TELUX_VERSION_APP} telux_common ${TELUXPLATFORM_LIBRARIES})

# install to target
install ( TARGETS ${TARGET_TELUX_VERSION_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
