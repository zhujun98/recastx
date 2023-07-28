# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut PSI
# Author: Jun Zhu
#
# Distributed under the terms of the GPLv3 License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
add_library(recastx_grpc_proto OBJECT
        control.proto
        imageproc.proto
        projection.proto
        reconstruction.proto
)
target_link_libraries(recastx_grpc_proto
    PUBLIC
        gRPC::grpc++_reflection
        gRPC::grpc++
        protobuf::libprotobuf
)
target_include_directories(recastx_grpc_proto
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
)

protobuf_generate(
        TARGET recastx_grpc_proto
        LANGUAGE cpp
        IMPORT_DIRS ${PROJECT_SOURCE_DIR}/protos
)
protobuf_generate(
        TARGET recastx_grpc_proto
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        IMPORT_DIRS ${PROJECT_SOURCE_DIR}/protos
        PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
)