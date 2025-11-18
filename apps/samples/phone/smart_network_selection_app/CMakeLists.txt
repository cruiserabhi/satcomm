cmake_minimum_required(VERSION 3.10.2)

set(TARGET_TEL_APP smart_network_selection_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXTEL REQUIRED telux-tel)

set(TEL_APP_SOURCES
        SmartNetworkSelectionApp.cpp
)

add_executable(${TARGET_TEL_APP} ${TEL_APP_SOURCES})

target_include_directories(${TARGET_TEL_APP} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common)

target_compile_options(${TARGET_TEL_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_TEL_APP} PRIVATE ${TELUXTEL_LIBRARIES})

install(TARGETS ${TARGET_TEL_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
