cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SENSOR_SELF_TEST_APP sensor_self_test_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXSENSOR REQUIRED telux-sensor)

add_compile_options(${TELUXSENSOR_CFLAGS})

set(SOURCES
    SensorSelfTestSampleApp.cpp
)

add_executable (${TARGET_SENSOR_SELF_TEST_APP} ${SOURCES})
target_compile_options(${TARGET_SENSOR_SELF_TEST_APP} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_SENSOR_SELF_TEST_APP} ${TELUXSENSOR_LIBRARIES} pthread)

# install to target
install ( TARGETS ${TARGET_SENSOR_SELF_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
