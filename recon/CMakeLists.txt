project(recastx-recon LANGUAGES CXX)

# Caveat: sequence
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(FFTW REQUIRED COMPONENTS FLOAT_LIB)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
message(STATUS "Found Eigen3 ${Eigen3_VERSION}")

find_package(nlohmann_json REQUIRED)

find_package(TBB 2021.5 REQUIRED)
message(STATUS "Found TBB ${TBB_VERSION}")

add_library(astra-toolbox SHARED IMPORTED)
set_target_properties(
        astra-toolbox PROPERTIES
        IMPORTED_LOCATION ${CMAKE_PREFIX_PATH}/lib/libastra.so
        INTERFACE_COMPILE_DEFINITIONS "ASTRA_CUDA")
set(ASTRA_INCLUDE_DIR ${CMAKE_PREFIX_PATH}/include)

set(RECON_SOURCES
        "src/utils.cpp"
        "src/filter.cpp"
        "src/phase.cpp"
        "src/reconstructor.cpp"
        "src/slice_mediator.cpp"
        "src/daq_client.cpp"
        "src/zmq_server.cpp"
        "src/application.cpp"
)

set(PROTOS "${CMAKE_CURRENT_LIST_DIR}/../common/reconstruction.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
list(APPEND RECON_SOURCES ${PROTO_SRCS} ${PROTO_HDRS})

set(RECON_LIB lib-recastx-recon)
add_library(${RECON_LIB} ${RECON_SOURCES})

target_include_directories(${RECON_LIB}
        PRIVATE
                ${ASTRA_INCLUDE_DIR}
        PUBLIC
                ${CMAKE_CURRENT_BINARY_DIR}
                ${CMAKE_CURRENT_LIST_DIR}/include
                ${CMAKE_CURRENT_LIST_DIR}/../common/include
        )

target_link_libraries(${RECON_LIB}
        PRIVATE
                nlohmann_json::nlohmann_json
                spdlog::spdlog
                TBB::tbb
                ${FFTW_FLOAT_LIB}
                protobuf::libprotobuf
        PUBLIC 
                cppzmq
                spdlog::spdlog
                Eigen3::Eigen
                astra-toolbox
        )

target_compile_options(${RECON_LIB}
        PUBLIC -Wall -Wextra -Wfatal-errors -fPIC -static)

target_compile_definitions(${RECON_LIB} PUBLIC VERBOSITY=${VERBOSITY})

if (BUILD_TEST)
    add_subdirectory(tests)
endif()

set(EXEC_NAME "recastx-recon")
add_executable(${EXEC_NAME} "src/main.cpp")
target_link_libraries(${EXEC_NAME} PUBLIC ${RECON_LIB} Boost::program_options)

install(TARGETS ${EXEC_NAME} RUNTIME DESTINATION bin)