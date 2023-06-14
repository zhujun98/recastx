add_library(recastx_grpc_proto OBJECT
        parameter.proto
        reconstruction.proto
        state.proto
        message.proto
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
        PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>")