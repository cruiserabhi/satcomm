cmake_minimum_required(VERSION 3.10.2)

set(TARGET_DIAG_APP diag_mdm_dev_callback)

pkg_check_modules(TELUXDIAG REQUIRED telux-platform-diag)

set(DIAG_APP_SOURCES DiagMDMCallbackMethod.cpp)

add_executable(${TARGET_DIAG_APP} ${DIAG_APP_SOURCES})

target_include_directories(${TARGET_DIAG_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_DIAG_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_DIAG_APP} PRIVATE ${TELUXDIAG_LIBRARIES})

install(TARGETS ${TARGET_DIAG_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
