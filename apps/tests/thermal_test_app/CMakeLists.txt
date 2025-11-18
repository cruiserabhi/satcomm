cmake_minimum_required(VERSION 3.10.2)

set(TARGET_THERMAL_TEST_APP thermal_test_app)

find_package(PkgConfig REQUIRED)

pkg_check_modules(TELUXTHERM REQUIRED telux-therm)
pkg_check_modules(TELUXCOMMON REQUIRED telux-common)

add_compile_options(${TELUXTHERM_CFLAGS} ${TELUXCOMMON_CFLAGS})

set(TELUX_LIBRARIES ${TELUXTHERM_LIBRARIES} ${TELUXCOMMON_LIBRARIES})

set(THERMAL_APP_SOURCES
    ThermalHelper.cpp
    ThermalTestApp.cpp
    ThermalListener.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleAppCommand.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleApp.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../common
)

add_executable (${TARGET_THERMAL_TEST_APP} ${THERMAL_APP_SOURCES})
target_compile_options(${TARGET_THERMAL_TEST_APP} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_THERMAL_TEST_APP} ${TELUX_LIBRARIES})

# install to target
install ( TARGETS ${TARGET_THERMAL_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
         )
