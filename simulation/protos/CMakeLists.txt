cmake_minimum_required(VERSION 3.10.2)

set(TARGET_PROTOS_LIBRARY telux_protos)

include(FindPkgConfig)

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

pkg_check_modules(PROTOBUF_LIB protobuf)
pkg_check_modules(GRPC_LIB grpc)
pkg_check_modules(GRPCPP_LIB grpc++)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/proto-src)
set(PROTOC_COMPILER $<TARGET_FILE:protobuf::protoc>)
set(GRPCPP_EXECUTABLE gRPC::grpc++)
set(GRPCPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

set (PROTOS "")
file(GLOB files "*.proto")
foreach(file ${files})
    get_filename_component(filename ${file} NAME_WE)
    get_filename_component(file_path ${file} PATH)

    set(telsdk_message_source "${PROTO_SRC_DIR}/${filename}.pb.cc")
    set(telsdk_message_header "${PROTO_SRC_DIR}/${filename}.pb.h")
    set(telsdk_service_source "${PROTO_SRC_DIR}/${filename}.grpc.pb.cc")
    set(telsdk_service_header "${PROTO_SRC_DIR}/${filename}.grpc.pb.h")

    add_custom_command(
        OUTPUT "${telsdk_message_source}" "${telsdk_message_header}" "${telsdk_service_source}" "${telsdk_service_header}"
        COMMAND ${PROTOC_COMPILER}
        ARGS --grpc_out "${PROTO_SRC_DIR}"
          --cpp_out "${PROTO_SRC_DIR}"
          -I "${file_path}"
          --plugin=protoc-gen-grpc="${GRPCPP_PLUGIN_EXECUTABLE}"
          "${file}"
        DEPENDS "${file}")
    list(APPEND PROTOS
        ${telsdk_message_source}
        ${telsdk_message_header}
        ${telsdk_service_source}
        ${telsdk_service_header}
    )
endforeach()

add_library(${TARGET_PROTOS_LIBRARY} SHARED ${PROTOS})

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

target_link_libraries(${TARGET_PROTOS_LIBRARY}
    ${PROTOBUF_LIB_LIBRARIES}
    ${GRPC_LIB_LIBRARIES}
    ${GRPCPP_LIB_LIBRARIES}
    ${GRPCPP_EXECUTABLE}
    )

install ( TARGETS ${TARGET_PROTOS_LIBRARY}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )