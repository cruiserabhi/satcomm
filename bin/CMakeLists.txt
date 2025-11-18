#
# This file creates stub libraries for Telematics SDK
#

# NOTE: Please update newly added library in the list and add
# package configuration in the folder
set(INPUT_LIST "telux_common" "telux_data" "telux_loc" "telux_tel" "telux_cv2x"
               "telux_audio" "telux_power" "telux_therm" "telux_config" "v2x_radio" "telux_sensor"
               "telux_platform" "telux_platform_diag" "telux_sec" "telux_sec_con" "telux_wlan"
               "telux_satcom")

set(SDK_VERSION "1.73.0")
#Update SDK Version for release

# Loop to create stub library from the INPUT_LIST
foreach(STUB_LIB ${INPUT_LIST})
    # pkg-config support
    string(REPLACE "_" "-" STUB_LIB_HYPHEN ${STUB_LIB})
    configure_file("${STUB_LIB_HYPHEN}.pc.in" "${STUB_LIB_HYPHEN}.pc" @ONLY)
    add_library (${STUB_LIB} SHARED stubs.c)

    # link the sym file for the given target
    set_target_properties(${STUB_LIB} PROPERTIES LINK_FLAGS "-DFNAME=so_stub_for_lib${STUB_LIB} ${CMAKE_SOURCE_DIR}/stubs.c \@${CMAKE_SOURCE_DIR}/lib${STUB_LIB}.sym")

    install ( TARGETS ${STUB_LIB}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )

    install (FILES "${CMAKE_CURRENT_BINARY_DIR}/${STUB_LIB_HYPHEN}.pc"
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
endforeach()
