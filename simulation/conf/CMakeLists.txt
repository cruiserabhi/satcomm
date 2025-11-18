cmake_minimum_required(VERSION 3.10.2)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/supervisord.conf"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/etc/")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/telsdk_simulation_server.conf"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/etc/supervisor/conf.d/")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/tel.conf"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/etc/telux/")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/eCall.conf"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/msdsettings.txt"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/updated_msdsettings.txt"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/")

