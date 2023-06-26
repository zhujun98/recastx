add_executable(rpc_server main.cpp rpc_server.cpp)
target_link_libraries(rpc_server
    PUBLIC
        recastx_grpc_proto
        spdlog::spdlog
)
target_include_directories(rpc_server
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../../../common/include)