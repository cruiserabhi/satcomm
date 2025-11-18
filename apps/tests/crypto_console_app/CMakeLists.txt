cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CRYPTO_CONSOLE_APP telux_crypto_console_app)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../common)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(TARGET_CRYPTO_CONSOLE_APP_SRCS
    CryptoConsoleApp.cpp
    CommandProcessor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleAppCommand.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/console_app_framework/ConsoleApp.cpp)

add_executable(${TARGET_CRYPTO_CONSOLE_APP} ${TARGET_CRYPTO_CONSOLE_APP_SRCS})

target_compile_options(${TARGET_CRYPTO_CONSOLE_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_CRYPTO_CONSOLE_APP}
    telux_sec
    telux_common)

install(TARGETS ${TARGET_CRYPTO_CONSOLE_APP}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
