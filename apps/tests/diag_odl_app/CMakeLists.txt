cmake_minimum_required(VERSION 3.10.2)

set(TARGET_DIAG_ODL_APP diag_odl_app)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(TARGET_DIAG_ODL_APP_SRCS
    DiagODLApp.cpp
    CollectionMethod.cpp
    FileMethod.cpp
    CallbackMethod.cpp
    UserInput.cpp
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
    ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp
)

add_executable(${TARGET_DIAG_ODL_APP} ${TARGET_DIAG_ODL_APP_SRCS})

target_compile_options(${TARGET_DIAG_ODL_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_DIAG_ODL_APP}
    telux_platform_diag
    telux_common)

install(TARGETS ${TARGET_DIAG_ODL_APP}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})