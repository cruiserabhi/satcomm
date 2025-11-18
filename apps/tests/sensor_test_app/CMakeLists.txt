cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SENSOR_TEST_APP sensor_test_app)

find_package(PkgConfig REQUIRED)

pkg_check_modules(TELUXSENSOR REQUIRED telux-sensor)
pkg_check_modules(TELUXCOMMON REQUIRED telux-common)


set(SENSOR_TEST_APP_SOURCES
    SensorTestApp.cpp
    SensorUtils.cpp
    SensorClient.cpp
    SensorControlMenu.cpp
    SensorFeatureControlMenu.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleAppCommand.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleApp.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/console_app_framework
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_SENSOR_TEST_APP} ${SENSOR_TEST_APP_SOURCES})
set (REQUIRED_LIBS ${TELUXSENSOR_LIBRARIES} ${TELUXCOMMON_LIBRARIES} pthread)
if (TELSDK_FEATURE_POWER)
    set (REQUIRED_LIBS ${REQUIRED_LIBS} telux_power)
endif(TELSDK_FEATURE_POWER)

target_link_libraries(${TARGET_SENSOR_TEST_APP} ${REQUIRED_LIBS})

# install to target
install ( TARGETS ${TARGET_SENSOR_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
