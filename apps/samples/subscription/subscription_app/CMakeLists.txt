cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SUBSCRIPTION_APP subscription_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXTEL REQUIRED telux-tel)

set(SUBSCRIPTION_APP_SOURCES SubscriptionApp.cpp)

add_executable(${TARGET_SUBSCRIPTION_APP} ${SUBSCRIPTION_APP_SOURCES})

target_include_directories(${TARGET_SUBSCRIPTION_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_SUBSCRIPTION_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_SUBSCRIPTION_APP} PRIVATE ${TELUXTEL_LIBRARIES})

install(TARGETS ${TARGET_SUBSCRIPTION_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
