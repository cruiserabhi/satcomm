cmake_minimum_required(VERSION 3.10.2)

set(TARGET_CARD_APP card_services_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXTEL REQUIRED telux-tel)

set(CARD_APP_SOURCES CardServicesApp.cpp)

add_executable(${TARGET_CARD_APP} ${CARD_APP_SOURCES})

target_include_directories(${TARGET_CARD_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_CARD_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_CARD_APP} PRIVATE ${TELUXTEL_LIBRARIES})

install(TARGETS ${TARGET_CARD_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
