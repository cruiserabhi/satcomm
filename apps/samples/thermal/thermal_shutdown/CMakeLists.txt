cmake_minimum_required(VERSION 3.10.2)

set(TARGET_THERMAL_APP thermal_shutdown_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXTHERM REQUIRED telux-therm)

set(THERMAL_APP_SOURCES ThermalShutdownApp.cpp)

add_executable (${TARGET_THERMAL_APP} ${THERMAL_APP_SOURCES})

target_include_directories(${TARGET_THERMAL_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_THERMAL_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_THERMAL_APP} ${TELUXTHERM_LIBRARIES})

install(TARGETS ${TARGET_THERMAL_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
