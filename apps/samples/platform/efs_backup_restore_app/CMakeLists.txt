cmake_minimum_required(VERSION 3.10.2)

set(TARGET_EFS_APP efs_backup_restore_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXPLATFORM REQUIRED telux-platform)

set(EFS_APP_SOURCES EfsBackupRestoreApp.cpp)

add_executable(${TARGET_EFS_APP} ${EFS_APP_SOURCES})

target_include_directories(${TARGET_EFS_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_EFS_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_EFS_APP} PRIVATE ${TELUXPLATFORM_LIBRARIES})

install(TARGETS ${TARGET_EFS_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
