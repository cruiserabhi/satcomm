cmake_minimum_required(VERSION 3.10.2)
set(CMAKE_VERBOSE_MAKEFILE ON)

# set the project name
project(v2xc)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )

# find all sources
file(GLOB SOURCE_FILES_TOP ${CMAKE_CURRENT_SOURCE_DIR}/src/wsmp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsm_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/btp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/j2735.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ieee1609.2.c
    )
file(GLOB ASN1C_FILES
    ${ASN1C_SKELETONS}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etsi/dictionary/generated/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etsi/CAM/generated/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etsi/DENM/generated/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sae/generated/*.c
    )
list(REMOVE_ITEM ASN1C_FILES ${ASN1C_SKELETONS}/converter-sample.c)

set(SOURCE_FILES ${SOURCE_FILES_TOP})

if(DEFINED ENV{ASN1C_PATH} OR DEFINED ASN1C_PATH)
    set(SOURCE_FILES ${SOURCE_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/src/etsi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/wsa.c)
    add_library(asn1c_codec ${ASN1C_FILES})
endif()


add_library(v2xcodec SHARED  ${SOURCE_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/src/v2x_codec.c)
if(DEFINED ENV{ASN1C_PATH} OR DEFINED ASN1C_PATH)
    target_link_libraries(v2xcodec asn1c_codec glib-2.0 m)
else()
    target_link_libraries(v2xcodec glib-2.0 m)
endif()

set_property(TARGET v2xcodec PROPERTY PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/include/asnbuf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/btp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/etsi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ieee1609.2.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/j2735.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/v2x_codec.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/v2x_msg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/wsmp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/wsa.h)


add_executable(v2xc v2xc.c)
target_link_libraries(v2xc v2xcodec)
install(TARGETS v2xc
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION bin)

install(TARGETS v2xcodec
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

